// 원본: 0043fd70(A) ↔ CD 0042a270, 00440150(A) ↔ CD 0042ab70, 00440380(A) ↔ CD 0042ad70.
// 범위: 원시 키 조회까지. 치환 전의 뒤 공백·이스케이프를 임의로 제거하지 않는다.
#include "o/Config.h"
#include "o/BaseFile.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::o {
// 원본 인코딩 설정 버퍼의 평문 서명.
constexpr std::string_view kConfigSignature = "mQdsT";
// 원본 DAT_00532538에 저장된 따옴표 이스케이프 문자.
constexpr char kConfigEscape = '`';

// 원본 설정 버퍼를 옮긴다. NUL 이후는 원본 C 문자열처럼 조회하지 않는다.
ConfigText::ConfigText(std::string text) : text_(std::move(text)) {
    const auto nul = text_.find('\0');
    if (nul != std::string::npos) text_.resize(nul);
}
// 원본 감지 규칙대로 복호화하고, 인코딩을 UTF-8로 바꾼다.
ConfigText ConfigText::FromBytes(std::span<const std::uint8_t> bytes) {
    std::vector<std::uint8_t> plain(bytes.begin(), bytes.end());
    if (plain.size() >= 3 && plain[0] == 0 && plain[2] == 0) {
        ApplyXor(plain);
        if (plain.size() < kConfigSignature.size() || !std::equal(kConfigSignature.begin(), kConfigSignature.end(), plain.begin()))
            throw std::runtime_error("Invalid encoded config signature");
        plain.erase(plain.begin(), plain.begin() + static_cast<std::ptrdiff_t>(kConfigSignature.size()));
    }
    return ConfigText(DecodeOriginalText(plain));
}
// FUN_0043fd70의 키 길이 비교와 LF 기준 줄 이동을 복원한다.
std::optional<std::size_t> ConfigText::FindKey(std::string_view key) const {
    if (key.empty()) return std::nullopt; // 빈 키는 원본의 비정상 입력이므로 새 API에서 거부한다.
    const auto foldedKey = AsciiLower(key);
    std::size_t pos = 0;
    // 버퍼 앞에서부터 첫 일치만 반환한다. CR 단독은 원본처럼 새 줄로 취급하지 않는다.
    while (pos < text_.size()) {
        // 줄 앞 공백·탭을 건너뛴다.
        while (pos < text_.size() && (text_[pos] == ' ' || text_[pos] == '\t')) ++pos;
        const auto start = pos;
        if (key.size() <= text_.size() - pos && AsciiLower(std::string_view(text_).substr(pos, key.size())) == foldedKey) {
            pos += key.size();
            // 키 뒤 공백·탭을 건너뛰어 '='를 확인한다.
            while (pos < text_.size() && (text_[pos] == ' ' || text_[pos] == '\t')) ++pos;
            if (pos < text_.size() && text_[pos] == '=') return start;
        }
        const auto newline = text_.find('\n', start);
        if (newline == std::string::npos) return std::nullopt;
        pos = newline;
        // 원본의 다음 줄 이동은 CR·LF·공백·탭을 함께 건너뛴다.
        while (pos < text_.size() && (text_[pos] == '\n' || text_[pos] == '\r' || text_[pos] == ' ' || text_[pos] == '\t')) ++pos;
    }
    return std::nullopt;
}
// FUN_00440150의 원시 값 읽기. 최대 버퍼 길이 assert는 C++ 동적 문자열로 대체한다.
std::optional<std::string> ConfigText::GetRaw(std::string_view key) const {
    const auto found = FindKey(key);
    if (!found) return std::nullopt;
    auto pos = text_.find('=', *found);
    // 원본은 연속 '='도 값 앞에서 건너뛴다.
    while (pos < text_.size() && (text_[pos] == '=' || text_[pos] == ' ' || text_[pos] == '\t')) ++pos;
    std::string result;
    bool quoted = false;
    char previous = '\0';
    // 줄 끝·따옴표 밖 주석 전까지 복사한다.
    for (; pos < text_.size(); ++pos) {
        const char c = text_[pos];
        if (c == '\r' || c == '\n' || (!quoted && c == '/' && pos + 1 < text_.size() && text_[pos+1] == '/')) break;
        if (c == '"' && previous != kConfigEscape) quoted = !quoted;
        else result.push_back(c);
        previous = c;
    }
    return result;
}
// 원본 0043ffa0(A) ↔ CD 0042a430: 입력 끝의 CR/LF를 모두 제거한 뒤 CRLF 하나를 붙인다.
void ConfigText::Append(std::string_view text) {
    auto end = text.find('\0');
    if (end == std::string_view::npos) end = text.size();
    // 원본은 입력 끝의 줄바꿈만 정리하며 기존 버퍼와 입력 앞부분은 바꾸지 않는다.
    while (end > 0 && (text[end-1] == '\r' || text[end-1] == '\n')) --end;
    text_.append(text.substr(0, end));
    text_.append("\r\n"); // 원본 DAT_00506ae8/e9의 CRLF.
}
// 조회 대상 텍스트를 반환한다.
const std::string& ConfigText::Text() const { return text_; }
}
