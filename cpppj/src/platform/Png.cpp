// PNG/zlib 구현이다. 고정 Huffman 코드와 해시 체인 LZ77로 압축하며 결과는 표준 zlib/PNG 디코더가 읽는다.
#include "platform/Png.h"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace netstorm::platform { // 이미지 파일 형식 부품이다.
namespace { // 압축기 내부 표와 비트 기록기다.
// CRC-32 표다(다항식 0xEDB88320). 첫 사용 때 한 번 계산한다.
const std::array<std::uint32_t, 256>& CrcTable() {
    static const auto table = [] { // 바이트별 CRC 나머지를 미리 계산한다.
        std::array<std::uint32_t, 256> result{};
        for (std::uint32_t index = 0; index < 256; ++index) { // 한 바이트 값의 8비트 시프트를 누적한다.
            std::uint32_t value = index;
            for (int bit = 0; bit < 8; ++bit) value = (value & 1U) ? (0xedb88320U ^ (value >> 1)) : (value >> 1);
            result[index] = value;
        }
        return result;
    }();
    return table;
}
// deflate 길이 부호(257..285)의 기본 길이와 추가 비트 수다.
constexpr std::array<std::uint16_t, 29> kLengthBase{3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
constexpr std::array<std::uint8_t, 29> kLengthExtra{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
// deflate 거리 부호(0..29)의 기본 거리와 추가 비트 수다.
constexpr std::array<std::uint16_t, 30> kDistanceBase{1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
constexpr std::array<std::uint8_t, 30> kDistanceExtra{0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

// 비트를 낮은 비트부터 채워 바이트로 내보내는 기록기다.
struct BitWriter {
    std::vector<std::uint8_t>& out; // 결과 바이트열이다.
    std::uint64_t bits{};           // 아직 내보내지 않은 비트 누산기다.
    unsigned count{};               // 누산기에 든 비트 수다.
    // value의 낮은 width비트를 LSB 우선으로 쓴다(추가 비트/블록 머리용).
    void Put(std::uint32_t value, unsigned width) {
        bits |= static_cast<std::uint64_t>(value) << count;
        count += width;
        while (count >= 8) { out.push_back(static_cast<std::uint8_t>(bits & 0xffU)); bits >>= 8; count -= 8; } // 가득 찬 바이트를 내보낸다.
    }
    // Huffman 부호는 MSB부터 읽히므로 비트를 뒤집어 쓴다.
    void PutCode(std::uint32_t code, unsigned width) {
        std::uint32_t reversed = 0;
        for (unsigned bit = 0; bit < width; ++bit) reversed |= ((code >> bit) & 1U) << (width - 1 - bit); // 부호 비트 순서를 뒤집는다.
        Put(reversed, width);
    }
    // 남은 비트를 0으로 채워 바이트 경계로 맞춘다.
    void Flush() { if (count) { out.push_back(static_cast<std::uint8_t>(bits & 0xffU)); bits = 0; count = 0; } }
};
// 고정 Huffman 부호로 리터럴/길이 심볼(0..287)을 쓴다.
void PutLiteralLength(BitWriter& writer, unsigned symbol) {
    if (symbol <= 143) writer.PutCode(0x30U + symbol, 8);
    else if (symbol <= 255) writer.PutCode(0x190U + (symbol - 144), 9);
    else if (symbol <= 279) writer.PutCode(symbol - 256, 7);
    else writer.PutCode(0xc0U + (symbol - 280), 8);
}
}

std::uint32_t Crc32(std::span<const std::uint8_t> data, std::uint32_t seed) {
    std::uint32_t value = ~seed;
    for (const auto byte : data) value = CrcTable()[(value ^ byte) & 0xffU] ^ (value >> 8); // 바이트마다 표를 한 번 조회한다.
    return ~value;
}
std::uint32_t Adler32(std::span<const std::uint8_t> data) {
    std::uint32_t a = 1, b = 0;
    for (const auto byte : data) { a = (a + byte) % 65521U; b = (b + a) % 65521U; } // 두 합을 모듈로 65521로 누적한다.
    return (b << 16) | a;
}

std::vector<std::uint8_t> ZlibCompress(std::span<const std::uint8_t> data) {
    std::vector<std::uint8_t> out;
    out.reserve(data.size() / 4 + 64);
    out.push_back(0x78); out.push_back(0x9c); // zlib 머리: deflate, 32KB 창, 기본 압축 수준이다.
    BitWriter writer{out};
    writer.Put(1, 1); // BFINAL=1
    writer.Put(1, 2); // BTYPE=01(고정 Huffman)
    constexpr std::size_t kHashSize = 1U << 15, kWindow = 32768, kMaxMatch = 258, kMinMatch = 3, kMaxChain = 32;
    std::vector<std::int32_t> head(kHashSize, -1), previous(data.size(), -1); // 해시별 최근 위치와 같은 해시의 이전 위치 사슬이다.
    const auto hash = [&](std::size_t at) { return ((static_cast<std::uint32_t>(data[at]) << 10) ^ (static_cast<std::uint32_t>(data[at + 1]) << 5) ^ data[at + 2]) & (kHashSize - 1); }; // 3바이트 해시다.
    std::size_t at = 0;
    while (at < data.size()) { // 위치마다 가장 긴 앞선 일치를 찾아 (길이,거리) 또는 리터럴로 쓴다.
        std::size_t bestLength = 0, bestDistance = 0;
        if (at + kMinMatch <= data.size()) {
            const auto key = hash(at);
            std::int32_t candidate = head[key];
            for (std::size_t chain = 0; candidate >= 0 && chain < kMaxChain; ++chain) { // 같은 해시 후보를 최대 kMaxChain개 비교한다.
                const std::size_t distance = at - static_cast<std::size_t>(candidate);
                if (distance > kWindow) break;
                std::size_t length = 0;
                const std::size_t limit = std::min(kMaxMatch, data.size() - at);
                while (length < limit && data[static_cast<std::size_t>(candidate) + length] == data[at + length]) ++length; // 겹치는 일치도 허용한다.
                if (length > bestLength) { bestLength = length; bestDistance = distance; if (length == limit) break; }
                candidate = previous[static_cast<std::size_t>(candidate)];
            }
        }
        const auto advance = [&](std::size_t count) { // count개 위치를 해시 사슬에 등록하고 앞으로 간다.
            for (std::size_t step = 0; step < count; ++step, ++at) {
                if (at + kMinMatch <= data.size()) { const auto key = hash(at); previous[at] = head[key]; head[key] = static_cast<std::int32_t>(at); }
            }
        };
        if (bestLength >= kMinMatch) {
            unsigned code = 0; // 길이 부호 번호(0..28)다.
            while (code + 1 < kLengthBase.size() && kLengthBase[code + 1] <= bestLength) ++code;
            PutLiteralLength(writer, 257 + code);
            if (kLengthExtra[code]) writer.Put(static_cast<std::uint32_t>(bestLength - kLengthBase[code]), kLengthExtra[code]);
            unsigned distanceCode = 0; // 거리 부호 번호(0..29)다.
            while (distanceCode + 1 < kDistanceBase.size() && kDistanceBase[distanceCode + 1] <= bestDistance) ++distanceCode;
            writer.PutCode(distanceCode, 5);
            if (kDistanceExtra[distanceCode]) writer.Put(static_cast<std::uint32_t>(bestDistance - kDistanceBase[distanceCode]), kDistanceExtra[distanceCode]);
            advance(bestLength);
        } else {
            PutLiteralLength(writer, data[at]);
            advance(1);
        }
    }
    PutLiteralLength(writer, 256); // 블록 끝이다.
    writer.Flush();
    const auto adler = Adler32(data);
    for (int shift = 24; shift >= 0; shift -= 8) out.push_back(static_cast<std::uint8_t>(adler >> shift)); // Adler-32는 빅 엔디언이다.
    return out;
}

namespace { // PNG 청크 조립이다.
// 빅 엔디언 32비트를 덧붙인다.
void PutBe32(std::vector<std::uint8_t>& out, std::uint32_t value) { for (int shift = 24; shift >= 0; shift -= 8) out.push_back(static_cast<std::uint8_t>(value >> shift)); }
// 길이+종류+자료+CRC의 청크 하나를 덧붙인다.
void PutChunk(std::vector<std::uint8_t>& out, const char (&type)[5], std::span<const std::uint8_t> payload) {
    PutBe32(out, static_cast<std::uint32_t>(payload.size()));
    const auto crcStart = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), payload.begin(), payload.end());
    PutBe32(out, Crc32(std::span<const std::uint8_t>(out).subspan(crcStart))); // 종류와 자료에 대한 CRC다.
}
// 공통 머리(IHDR)를 만든다.
std::vector<std::uint8_t> Header(std::uint32_t width, std::uint32_t height, std::uint8_t colorType) {
    std::vector<std::uint8_t> ihdr;
    PutBe32(ihdr, width); PutBe32(ihdr, height);
    ihdr.insert(ihdr.end(), {8, colorType, 0, 0, 0}); // 비트 깊이 8, 색 종류, 압축/필터/인터레이스 0이다.
    return ihdr;
}
// 서명과 IHDR/(PLTE)/IDAT/IEND를 이어 PNG 한 장을 만든다.
std::vector<std::uint8_t> Assemble(std::uint32_t width, std::uint32_t height, std::uint8_t colorType, std::span<const std::uint8_t> palette, const std::vector<std::uint8_t>& raw) {
    std::vector<std::uint8_t> png{0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};
    PutChunk(png, "IHDR", Header(width, height, colorType));
    if (!palette.empty()) PutChunk(png, "PLTE", palette);
    PutChunk(png, "IDAT", ZlibCompress(raw));
    PutChunk(png, "IEND", {});
    return png;
}
}

std::vector<std::uint8_t> EncodePngIndexed(std::uint32_t width, std::uint32_t height, std::uint32_t pitch,
                                           std::span<const std::uint8_t> pixels, std::span<const std::uint8_t, 768> palette) {
    if (!width || !height || pitch < width || static_cast<std::uint64_t>(pitch) * (height - 1) + width > pixels.size()) throw std::invalid_argument("PNG 인덱스 이미지 크기 범위");
    std::vector<std::uint8_t> raw;
    raw.reserve(static_cast<std::size_t>(width + 1) * height);
    for (std::uint32_t y = 0; y < height; ++y) { // 행마다 필터 종류 0(없음) 한 바이트와 픽셀을 쓴다.
        raw.push_back(0);
        const auto row = pixels.subspan(static_cast<std::size_t>(y) * pitch, width);
        raw.insert(raw.end(), row.begin(), row.end());
    }
    return Assemble(width, height, 3, palette, raw);
}
std::vector<std::uint8_t> EncodePngRgba(std::uint32_t width, std::uint32_t height, std::span<const std::uint8_t> rgba) {
    if (!width || !height || static_cast<std::uint64_t>(width) * height * 4 > rgba.size()) throw std::invalid_argument("PNG RGBA 이미지 크기 범위");
    std::vector<std::uint8_t> raw;
    raw.reserve(static_cast<std::size_t>(width * 4 + 1) * height);
    for (std::uint32_t y = 0; y < height; ++y) { // 행마다 필터 0 한 바이트와 RGBA 픽셀을 쓴다.
        raw.push_back(0);
        const auto row = rgba.subspan(static_cast<std::size_t>(y) * width * 4, static_cast<std::size_t>(width) * 4);
        raw.insert(raw.end(), row.begin(), row.end());
    }
    return Assemble(width, height, 6, {}, raw);
}
}
