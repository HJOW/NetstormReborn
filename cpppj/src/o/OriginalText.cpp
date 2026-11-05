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

namespace { // 확장 시퀀스의 숫자 표와 코드 포인트 구간 표다.
// 숫자 25개의 바이트다. 순서가 곧 값(0..24)이다.
constexpr std::array<std::uint8_t, 25> kExtendedDigits{1, 2, 3, 4, 5, 6, 7, 8, 0x0b, 0x0c, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1c, 0x1d, 0x1e};
// 확장 대상 코드 포인트 구간이다. 각 구간은 이어 붙인 인덱스 공간의 연속 구간이 된다(합 11424 < 25^3).
struct ExtendedRange { std::uint32_t first, last; };
constexpr std::array<ExtendedRange, 4> kExtendedRanges{{{0xac00, 0xd7a3}, {0x3131, 0x318e}, {0x3000, 0x303f}, {0xff01, 0xff5e}}}; // 한글 음절, 호환 자모, CJK 기호, 전각 ASCII다.
// 숫자 바이트의 값(0..24)이다. 숫자가 아니면 -1이다.
int DigitValue(std::uint8_t byte) {
    for (std::size_t value = 0; value < kExtendedDigits.size(); ++value) if (kExtendedDigits[value] == byte) return static_cast<int>(value); // 25개 중 일치하는 위치가 값이다.
    return -1;
}
}

bool IsExtendedTextDigit(std::uint8_t byte) { return DigitValue(byte) >= 0; }

std::optional<std::array<std::uint8_t, kExtendedTextLength>> EncodeExtendedCodePoint(std::uint32_t codePoint) {
    std::uint32_t base = 0; // 이전 구간들이 차지한 인덱스 수다.
    for (const auto& range : kExtendedRanges) { // 구간을 순서대로 훑어 코드 포인트의 인덱스를 구한다.
        if (codePoint >= range.first && codePoint <= range.last) {
            const std::uint32_t index = base + (codePoint - range.first);
            return std::array<std::uint8_t, kExtendedTextLength>{kExtendedTextLead, kExtendedDigits[index / 625], kExtendedDigits[(index / 25) % 25], kExtendedDigits[index % 25]};
        }
        base += range.last - range.first + 1;
    }
    return std::nullopt;
}

std::optional<std::uint32_t> DecodeExtendedSequence(std::span<const std::uint8_t> bytes, std::size_t position) {
    if (position + kExtendedTextLength > bytes.size() || bytes[position] != kExtendedTextLead) return std::nullopt;
    const int high = DigitValue(bytes[position + 1]), middle = DigitValue(bytes[position + 2]), low = DigitValue(bytes[position + 3]);
    if (high < 0 || middle < 0 || low < 0) return std::nullopt;
    std::uint32_t index = static_cast<std::uint32_t>(high) * 625 + static_cast<std::uint32_t>(middle) * 25 + static_cast<std::uint32_t>(low);
    for (const auto& range : kExtendedRanges) { // 인덱스가 속한 구간을 찾아 코드 포인트로 되돌린다.
        const std::uint32_t size = range.last - range.first + 1;
        if (index < size) return range.first + index;
        index -= size;
    }
    return std::nullopt; // 구간 밖의 숫자 조합은 시퀀스가 아니다.
}

// 로케일에 영향을 받지 않는 ASCII 키 비교용 문자열을 만든다.
std::string AsciiLower(std::string_view text) {
    std::string result(text);
    // ASCII 대문자만 변환한다.
    for (char& c : result) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
    return result;
}

// 원본 바이트를 UTF-8로 바꾸는 공통 구현이다. extendedText가 참이면(6차 표시 경계) 정확한 한글 확장 시퀀스도 한 문자로 읽는다.
static std::string DecodeBytes(std::span<const std::uint8_t> bytes, bool extendedText) {
    if (bytes.size() >= 3 && bytes[0] == 0xef && bytes[1] == 0xbb && bytes[2] == 0xbf)
        return std::string(bytes.begin() + 3, bytes.end());
    std::string result;
    // 각 코드 포인트를 1~3바이트 UTF-8로 기록한다. 6차: 정확한 확장 시퀀스는 한 코드 포인트로 읽는다.
    for (std::size_t position = 0; position < bytes.size(); ++position) {
        const auto byte = bytes[position];
        std::uint32_t cp = byte >= 0x80 && byte <= 0x9f ? kWindows1252[byte - 0x80] : byte;
        if (const auto extended = extendedText && byte == kExtendedTextLead ? DecodeExtendedSequence(bytes, position) : std::nullopt) { cp = *extended; position += kExtendedTextLength - 1; }
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

// UTF-8을 원본 바이트로 바꾸는 공통 구현이다. extendedText가 참이면(6차 표시 경계) 한글 등을 4바이트 확장 시퀀스로 만든다.
static std::vector<std::uint8_t> EncodeBytes(std::string_view text, bool extendedText) {
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
        else if (const auto extended = extendedText ? EncodeExtendedCodePoint(cp) : std::nullopt) result.insert(result.end(), extended->begin(), extended->end()); // 6차: 표시 경계에서만 한글 등이 확장 시퀀스다.
        else {
            const auto found = std::find(kWindows1252.begin(), kWindows1252.end(), cp);
            if (found == kWindows1252.end()) throw std::invalid_argument("Text cannot be encoded as Windows-1252");
            result.push_back(static_cast<std::uint8_t>(0x80 + (found - kWindows1252.begin())));
        }
    }
    return result;
}

// 원본 자산을 읽을 때만 Windows-1252 변환을 수행한다. 확장 시퀀스는 해독하지 않아 파일/설정 경계는 원래 계약 그대로다.
std::string DecodeOriginalText(std::span<const std::uint8_t> bytes) { return DecodeBytes(bytes, false); }
// 원본은 문자 바이트를 그대로 쓴다. 읽을 때 변환한 UTF-8을 역변환해 BOM 없이 저장한다. 1252로 표현할 수 없는 문자는 거부한다(설정/경로 경계가 이 거부에 의존한다).
std::vector<std::uint8_t> EncodeOriginalText(std::string_view text) { return EncodeBytes(text, false); }
// 6차 한국어: 화면에 그릴 글꼴 바이트로 읽는다. DecodeOriginalText와 같되 정확한 한글 확장 시퀀스를 한 문자로 되돌린다.
std::string DecodeDisplayText(std::span<const std::uint8_t> bytes) { return DecodeBytes(bytes, true); }
// 6차 한국어: 화면에 그릴 글꼴 바이트로 만든다. EncodeOriginalText와 같되 한글·호환 자모·CJK 기호·전각 ASCII는 4바이트 확장 시퀀스가 된다.
std::vector<std::uint8_t> EncodeDisplayText(std::string_view text) { return EncodeBytes(text, true); }
}
