// 원본 배경 GIF 경로는 파일의 RGB 표가 아니라 해독된 8비트 색 번호를 화면에 복사한다.
#include "client/GifImage.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 자료보다 큰 손상 파일의 무제한 할당을 막는 이미지 픽셀 한도.
constexpr std::size_t kMaxGifPixels = 16 * 1024 * 1024;
// 파일 경계를 검사하며 GIF의 바이트·리틀 엔디언 정수·서브 블록을 읽는다.
class GifReader {
public:
    // 읽기 전용 파일 버퍼를 참조한다.
    explicit GifReader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}
    // 다음 바이트를 읽는다.
    unsigned Byte() { if (position_ >= bytes_.size()) throw std::runtime_error("Truncated GIF"); return bytes_[position_++]; }
    // GIF의 16비트 정수.
    unsigned Word() { const auto low = Byte(); return low | Byte() << 8; }
    // 색 표 등 사용하지 않는 바이트도 파일 경계를 검사한다.
    void Skip(std::size_t count) { if (count > bytes_.size() - position_) throw std::runtime_error("Truncated GIF table"); position_ += count; }
    // 길이 바이트가 0인 끝까지 서브 블록을 합친다.
    std::vector<std::uint8_t> Blocks() {
        std::vector<std::uint8_t> result;
        // GIF 이미지 데이터와 확장 데이터의 공통 길이 규칙.
        for (auto size = Byte(); size != 0; size = Byte()) {
            if (size > bytes_.size() - position_) throw std::runtime_error("Truncated GIF block");
            result.insert(result.end(), bytes_.begin() + position_, bytes_.begin() + position_ + size); position_ += size;
        }
        return result;
    }
private:
    std::span<const std::uint8_t> bytes_;
    std::size_t position_{};
};
// LSB부터 읽는 GIF LZW. 사전 재설정·코드 폭 증가·자기 참조 코드를 처리한다.
std::vector<std::uint8_t> Lzw(std::span<const std::uint8_t> bytes, unsigned minimum, std::size_t pixels) {
    if (minimum < 2 || minimum > 8) throw std::runtime_error("Invalid GIF LZW code size");
    const unsigned clear = 1u << minimum, end = clear + 1;
    std::array<unsigned, 4096> prefix{}; std::array<std::uint8_t, 4096> suffix{}, stack{};
    unsigned next = end + 1, width = minimum + 1, previous = 4096, first = 0;
    std::size_t bit = 0;
    std::vector<std::uint8_t> output; output.reserve(pixels);
    // 종료 코드까지 읽으며 예상 픽셀 수를 넘는 출력은 거부한다.
    while (bit + width <= bytes.size() * 8) {
        unsigned code = 0;
        // 가변 길이 코드를 낮은 비트부터 모은다.
        for (unsigned i = 0; i < width; ++i, ++bit) code |= ((bytes[bit / 8] >> (bit % 8)) & 1u) << i;
        if (code == clear) { next = end + 1; width = minimum + 1; previous = 4096; continue; }
        if (code == end) { if (output.size() != pixels) throw std::runtime_error("GIF pixel count mismatch"); return output; }
        const unsigned input = code; std::size_t count = 0;
        if (code == next && previous != 4096) { stack[count++] = static_cast<std::uint8_t>(first); code = previous; }
        else if (code >= next) throw std::runtime_error("Invalid GIF LZW code");
        // 접두 사전을 역순으로 풀며 순환·범위 밖 참조를 막는다.
        while (code >= clear) {
            if (code <= end || code >= next || count >= stack.size() - 1) throw std::runtime_error("Invalid GIF LZW dictionary");
            stack[count++] = suffix[code]; code = prefix[code];
        }
        first = code; stack[count++] = static_cast<std::uint8_t>(code);
        if (count > pixels - output.size()) throw std::runtime_error("GIF output overflow");
        // 역순 스택을 실제 픽셀 순서로 기록한다.
        while (count != 0) output.push_back(stack[--count]);
        if (previous != 4096 && next < 4096) {
            prefix[next] = previous; suffix[next] = static_cast<std::uint8_t>(first); ++next;
            if (next == (1u << width) && width < 12) ++width;
        }
        previous = input;
    }
    throw std::runtime_error("Missing GIF LZW end code");
}
}
// 첫 이미지까지 확장을 건너뛴다. 게임 배경에는 애니메이션 GIF를 사용하지 않는다.
IndexedImage DecodeGif(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 13 || (std::memcmp(bytes.data(), "GIF87a", 6) != 0 && std::memcmp(bytes.data(), "GIF89a", 6) != 0))
        throw std::runtime_error("Invalid GIF signature");
    GifReader reader(bytes); reader.Skip(6);
    const auto width = reader.Word(), height = reader.Word(), packed = reader.Byte(), background = reader.Byte(); reader.Byte();
    if (width == 0 || height == 0 || static_cast<std::size_t>(width) * height > kMaxGifPixels) throw std::runtime_error("Invalid GIF dimensions");
    if (packed & 0x80) reader.Skip(3u << ((packed & 7) + 1));
    int transparent = -1;
    // 그래픽 제어 확장의 투명 번호를 첫 이미지에 적용한다.
    while (true) {
        const auto marker = reader.Byte();
        if (marker == 0x21) {
            const auto kind = reader.Byte(); const auto data = reader.Blocks();
            if (kind == 0xf9 && data.size() == 4 && (data[0] & 1)) transparent = data[3];
            continue;
        }
        if (marker != 0x2c) throw std::runtime_error("Missing GIF image");
        const auto left = reader.Word(), top = reader.Word(), imageWidth = reader.Word(), imageHeight = reader.Word(), flags = reader.Byte();
        if (imageWidth == 0 || imageHeight == 0 || left + imageWidth > width || top + imageHeight > height) throw std::runtime_error("Invalid GIF image bounds");
        if (flags & 0x80) reader.Skip(3u << ((flags & 7) + 1));
        const auto minimum = reader.Byte(); const auto compressed = reader.Blocks();
        const auto decoded = Lzw(compressed, minimum, static_cast<std::size_t>(imageWidth) * imageHeight);
        IndexedImage result{width, height, std::vector<std::uint8_t>(static_cast<std::size_t>(width) * height, static_cast<std::uint8_t>(background)),
            std::vector<std::uint8_t>(static_cast<std::size_t>(width) * height, transparent < 0 ? 255 : 0)};
        std::size_t source = 0;
        // 한 행의 색 번호와 투명 여부를 논리 화면에 옮긴다.
        const auto row = [&](unsigned y) {
            // 이미지 사각형의 실제 픽셀만 채운다.
            for (unsigned x = 0; x < imageWidth; ++x) {
                const auto index = static_cast<std::size_t>(top + y) * width + left + x;
                const auto color = decoded[source++]; result.indices[index] = color; result.opacity[index] = color == transparent ? 0 : 255;
            }
        };
        if (flags & 0x40) {
            // GIF 인터레이스의 네 순회 시작 행과 간격.
            constexpr std::array<unsigned, 4> starts{0, 4, 2, 1}, steps{8, 8, 4, 2};
            // 네 패스를 순서대로 실제 행에 배치한다.
            for (std::size_t pass = 0; pass < starts.size(); ++pass)
                // 현재 패스의 행.
                for (unsigned y = starts[pass]; y < imageHeight; y += steps[pass]) row(y);
        } else {
            // 일반 GIF는 위에서 아래로 이어진다.
            for (unsigned y = 0; y < imageHeight; ++y) row(y);
        }
        return result;
    }
}
}
