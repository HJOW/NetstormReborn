// OpenType cmap4·EBLC index1/2·EBDT image2/5의 단색 픽셀만 해석한다. 외부 글꼴이나 원본 게임 자료를 수정하지 않는다.
#include "client/EmbeddedBitmapFont.h"
#include "o/BaseFile.h"
#include <bit>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string_view>

namespace netstorm::client { // 파일 판독과 glyph 조회를 Windows 글꼴 대체 선택에서 분리한다.
namespace { // SFNT의 빅 엔디언 정수와 테이블 경계를 공통 검증한다.
// 지정 범위가 backing 안에 있는지 확인한 뒤 읽기 전용 뷰를 돌려준다.
std::span<const std::uint8_t> Slice(std::span<const std::uint8_t> bytes, std::size_t at, std::size_t size) {
    if (at > bytes.size() || size > bytes.size() - at) throw std::runtime_error("외부 비트맵 글꼴의 테이블 범위 오류");
    return bytes.subspan(at, size);
}
// OpenType의 2바이트 정수를 읽는다. 정렬되지 않은 원천도 허용한다.
std::uint16_t U16(std::span<const std::uint8_t> bytes, std::size_t at) {
    const auto value = Slice(bytes, at, 2);
    return static_cast<std::uint16_t>((std::uint16_t(value[0]) << 8) | value[1]);
}
// OpenType의 4바이트 정수를 읽는다. 오프셋 산술은 호출자가 size_t로 수행한다.
std::uint32_t U32(std::span<const std::uint8_t> bytes, std::size_t at) {
    return (std::uint32_t(U16(bytes, at)) << 16) | U16(bytes, at + 2);
}
// 부호 있는 1바이트 bearing을 같은 비트로 해석한다.
int Signed(std::uint8_t value) { return std::bit_cast<std::int8_t>(value); }
// SFNT 디렉터리에서 태그를 찾는다. 없는 테이블은 빈 뷰이며 잘못된 오프셋은 거부한다.
std::span<const std::uint8_t> Table(std::span<const std::uint8_t> bytes, std::string_view tag) {
    if (U32(bytes, 0) != 0x00010000U) return {}; // TTC/CFF는 기존 GDI 경로가 소유한다.
    const auto count = U16(bytes, 4);
    Slice(bytes, 12, std::size_t(count) * 16);
    for (std::size_t index = 0; index < count; ++index) { // 검증한 SFNT 디렉터리를 한 번 순회한다.
        const auto at = 12 + index * 16;
        const auto name = Slice(bytes, at, 4);
        if (std::string_view(reinterpret_cast<const char*>(name.data()), 4) == tag)
            return Slice(bytes, U32(bytes, at + 8), U32(bytes, at + 12));
    }
    return {};
}
}
// backing을 먼저 소유한 다음 테이블 뷰를 구성한다. 표의 길이·버전·Unicode 구간을 실제 파일에서 확인한다.
EmbeddedBitmapFont::EmbeddedBitmapFont(std::span<const std::uint8_t> bytes) : bytes_(bytes.begin(), bytes.end()) {
    locations_ = Table(bytes_, "EBLC"); images_ = Table(bytes_, "EBDT");
    const auto mappings = Table(bytes_, "cmap");
    if (U32(locations_, 0) != 0x00020000U || U32(images_, 0) != 0x00020000U || U16(mappings, 0) != 0)
        throw std::runtime_error("외부 비트맵 글꼴의 EBLC/EBDT/cmap 버전 미지원");
    const auto count = U16(mappings, 2);
    Slice(mappings, 4, std::size_t(count) * 8);
    for (std::size_t index = 0; index < count; ++index) { // Unicode BMP cmap4만 선택하며 Macintosh 표와 혼합하지 않는다.
        const auto at = 4 + index * 8;
        const auto platform = U16(mappings, at), encoding = U16(mappings, at + 2);
        const auto offset = U32(mappings, at + 4);
        if ((platform == 0 || (platform == 3 && encoding == 1)) && U16(mappings, offset) == 4) {
            cmap_ = Slice(mappings, offset, U16(mappings, std::size_t(offset) + 2));
            if (platform == 3) break; // 동일 문자 체계에서는 Windows Unicode 표를 우선한다.
        }
    }
    if (cmap_.empty() || !U16(cmap_, 6) || (U16(cmap_, 6) & 1U)) throw std::runtime_error("외부 비트맵 글꼴의 Unicode cmap4 부재/손상");
    Slice(cmap_, 0, 16 + std::size_t(U16(cmap_, 6) / 2) * 8);
    const auto strikes = U32(locations_, 4);
    Slice(locations_, 8, std::size_t(strikes) * 48);
    for (std::size_t index = 0; index < strikes; ++index) { // 실제 제공된 정사각 수평 1비트 strike를 고른다.
        const auto at = 8 + index * 48;
        if (locations_[at + 44] && locations_[at + 44] == locations_[at + 45] && locations_[at + 46] == 1 && locations_[at + 47] == 1) {
            strike_ = at;
            Slice(locations_, U32(locations_, at), U32(locations_, at + 4));
            Slice(locations_, U32(locations_, at), std::size_t(U32(locations_, at + 8)) * 8);
            break;
        }
    }
    if (!strike_) throw std::runtime_error("외부 비트맵 글꼴의 수평 단색 strike 부재");
}
// 같은 설정을 여러 슬롯이 사용해도 파일은 한 번만 읽는다. 캐시는 읽기 전용이며 게임 종료 때 반납한다.
std::shared_ptr<const EmbeddedBitmapFont> EmbeddedBitmapFont::Load(const std::filesystem::path& file) {
    if (file.empty()) return {};
    static std::mutex mutex; // 공유 캐시의 생성/조회를 직렬화한다.
    static std::map<std::filesystem::path, std::shared_ptr<const EmbeddedBitmapFont>> loaded; // 빈 값도 보관하여 일반 TTF를 반복 판독하지 않는다.
    const std::lock_guard lock(mutex);
    if (const auto found = loaded.find(file); found != loaded.end()) return found->second;
    const auto bytes = o::ReadFileBytes(file);
    std::shared_ptr<const EmbeddedBitmapFont> font;
    if (!Table(bytes, "EBLC").empty()) font = std::make_shared<EmbeddedBitmapFont>(bytes);
    loaded.emplace(file, font);
    return font;
}
// cmap4의 delta/상대 offset을 해석한 뒤 해당 strike의 비트 연속 데이터를 MSB부터 풀어낸다.
std::optional<EmbeddedBitmapGlyph> EmbeddedBitmapFont::Glyph(std::uint32_t codePoint) const {
    if (codePoint > 0xffffU) return {};
    const auto segments = std::size_t(U16(cmap_, 6) / 2);
    std::uint16_t glyph{}; // 0은 cmap의 글자 없음이며 시스템 글꼴로 위장하지 않는다.
    for (std::size_t index = 0; index < segments; ++index) { // 정렬된 끝 코드 중 처음으로 문자를 포함하는 구간을 찾는다.
        if (codePoint > U16(cmap_, 14 + index * 2)) continue;
        const auto first = U16(cmap_, 16 + segments * 2 + index * 2);
        if (codePoint < first) return {};
        const auto delta = U16(cmap_, 16 + segments * 4 + index * 2);
        const auto at = 16 + segments * 6 + index * 2;
        const auto offset = U16(cmap_, at);
        glyph = offset ? U16(cmap_, at + offset + (codePoint - first) * 2) : static_cast<std::uint16_t>(codePoint);
        if (glyph) glyph = static_cast<std::uint16_t>(glyph + delta);
        break;
    }
    if (!glyph) return {};
    const auto array = std::size_t(U32(locations_, strike_));
    const auto count = U32(locations_, strike_ + 8);
    for (std::size_t index = 0; index < count; ++index) { // 글리프 번호를 포함하는 실제 EBLC 연속 범위만 해석한다.
        const auto at = array + index * 8;
        const auto first = U16(locations_, at), last = U16(locations_, at + 2);
        if (glyph < first || glyph > last) continue;
        const auto sub = array + U32(locations_, at + 4);
        const auto format = U16(locations_, sub), imageFormat = U16(locations_, sub + 2);
        const auto base = std::size_t(U32(locations_, sub + 4));
        std::span<const std::uint8_t> metrics, bits; // 각각 검증한 glyph metrics와 해당 글리프의 비트 payload다.
        if (format == 1 && imageFormat == 2) {
            Slice(locations_, sub + 8, (std::size_t(last - first) + 2) * 4);
            const auto entry = sub + 8 + std::size_t(glyph - first) * 4;
            const auto begin = U32(locations_, entry), end = U32(locations_, entry + 4);
            if (end < begin) throw std::runtime_error("외부 비트맵 글꼴의 역전된 이미지 범위");
            if (begin == end) return {};
            const auto image = Slice(images_, base + begin, end - begin);
            metrics = Slice(image, 0, 5); bits = image.subspan(5);
        } else if (format == 2 && imageFormat == 5) {
            const auto size = std::size_t(U32(locations_, sub + 8));
            metrics = Slice(locations_, sub + 12, 8);
            bits = Slice(images_, base + std::size_t(glyph - first) * size, size);
        } else throw std::runtime_error("외부 비트맵 글꼴의 index/image 형식 미지원");
        EmbeddedBitmapGlyph result;
        result.height = metrics[0]; result.width = metrics[1]; result.bearingX = Signed(metrics[2]); result.bearingY = Signed(metrics[3]);
        result.advance = metrics[4]; result.ppem = locations_[strike_ + 45];
        const auto pixels = std::size_t(result.width) * result.height;
        Slice(bits, 0, (pixels + 7) / 8);
        result.pixels.resize(pixels);
        for (std::size_t pixel = 0; pixel < pixels; ++pixel) // 행 사이를 패딩하지 않는 image2/5의 연속 비트를 0/1로 바꾼다.
            result.pixels[pixel] = (bits[pixel / 8] >> (7 - pixel % 8)) & 1U;
        return result;
    }
    return {};
}
}
