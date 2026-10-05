// 새로 쓰는 코드: 게임 로직을 바꾸지 않고 원본 문자 인코딩을 UTF-8로 바꾼다.
#include "o/OriginalText.h"
#include <array>
#include <algorithm>
#include <stdexcept>

namespace netstorm::o {
// 원본 0x80~0x9f의 Windows-1252 코드 포인트. 정의되지 않은 바이트는 제어 문자를 보존한다.
constexpr std::array<std::uint32_t, 32> kWindows1252 = {
    0x20ac,0x81,0x201a,0x192,0x201e,0x2026,0x2020,0x2021,
    0x2c6,0x2030,0x160,0x2039,0x152,0x8d,0x17d,0x8f,
    0x90,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,
    0x2dc,0x2122,0x161,0x203a,0x153,0x9d,0x17e,0x178};

// 로케일에 영향을 받지 않는 ASCII 키 비교용 문자열을 만든다.
std::string AsciiLower(std::string_view text) {
    std::string result(text);
    // ASCII 대문자만 변환한다.
    for (char& c : result) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
    return result;
}

// 원본 자산을 읽을 때만 Windows-1252 변환을 수행한다.
std::string DecodeOriginalText(std::span<const std::uint8_t> bytes) {
    if (bytes.size() >= 3 && bytes[0] == 0xef && bytes[1] == 0xbb && bytes[2] == 0xbf)
        return std::string(bytes.begin() + 3, bytes.end());
    std::string result;
    // 각 코드 포인트를 1~3바이트 UTF-8로 기록한다.
    for (auto byte : bytes) {
        const auto cp = byte >= 0x80 && byte <= 0x9f ? kWindows1252[byte - 0x80] : byte;
        if (cp < 0x80) result.push_back(static_cast<char>(cp));
        else if (cp < 0x800) {
            result.push_back(static_cast<char>(0xc0 | (cp >> 6)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        } else {
            result.push_back(static_cast<char>(0xe0 | (cp >> 12)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        }
    }
    return result;
}

// 원본은 문자 바이트를 그대로 쓴다. 읽을 때 변환한 UTF-8을 역변환해 BOM 없이 저장한다.
std::vector<std::uint8_t> EncodeOriginalText(std::string_view text) {
    std::vector<std::uint8_t> result;
    // UTF-8 문자 하나를 읽고 Windows-1252의 같은 코드 포인트를 찾는다.
    for (std::size_t pos = 0; pos < text.size();) {
        const auto lead = static_cast<std::uint8_t>(text[pos++]);
        std::uint32_t cp = lead;
        if (lead >= 0x80) {
            const int extra = lead >= 0xc2 && lead <= 0xdf ? 1 : lead >= 0xe0 && lead <= 0xef ? 2 : 0;
            if (extra == 0 || pos + static_cast<std::size_t>(extra) > text.size())
                throw std::invalid_argument("Text cannot be encoded as Windows-1252");
            cp = lead & (extra == 1 ? 0x1fu : 0x0fu);
            // 후속 바이트의 형식과 최단 UTF-8 표현을 확인한다.
            for (int i = 0; i < extra; ++i) {
                const auto next = static_cast<std::uint8_t>(text[pos++]);
                if ((next & 0xc0u) != 0x80u) throw std::invalid_argument("Invalid UTF-8 text");
                cp = (cp << 6) | (next & 0x3fu);
            }
            if ((extra == 2 && cp < 0x800) || (cp >= 0xd800 && cp <= 0xdfff))
                throw std::invalid_argument("Invalid UTF-8 text");
        }
        if (cp < 0x80 || (cp >= 0xa0 && cp <= 0xff)) result.push_back(static_cast<std::uint8_t>(cp));
        else {
            const auto found = std::find(kWindows1252.begin(), kWindows1252.end(), cp);
            if (found == kWindows1252.end()) throw std::invalid_argument("Text cannot be encoded as Windows-1252");
            result.push_back(static_cast<std::uint8_t>(0x80 + (found - kWindows1252.begin())));
        }
    }
    return result;
}
}
