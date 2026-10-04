// 원본: 0043fd70(A) ↔ CD 0042a270, 00440150(A) ↔ CD 0042ab70, 00440380(A) ↔ CD 0042ad70,
//       0043fc20/0043fcd0/0043fab0(객체 등록), 00440760 ↔ CD 0042a6b0(조회), 00440a00 ↔ CD 0042a540(괄호),
//       00440b60 ↔ CD 0042a960(치환), 00440240 ↔ CD 0042ac50(줄 삭제), 004404e0 ↔ CD 0042af30(값 쓰기),
//       0043fe60 ↔ CD 0042a320·00440570·00440640(섹션).
// 범위: 원시 조회·객체 층·치환·값 쓰기·섹션. 고정 길이 버퍼(패치 8KB, CD 4KB)는 동적 문자열로 바꿨다.
// 검증: 조회·치환·삭제·값 쓰기 대상 선택·섹션 검색은 두 판본 x86 기대값과 대조한다(tests/fixtures/config-x86.tsv).
#include "o/Config.h"
#include "o/BaseFile.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 00440a00의 키·기본값 수집 한도(패치판 지역 버퍼 0x2000바이트, CD판은 0x1000).
constexpr std::size_t kLongestSymbol = 0x2000;
// 새 보호 장치: 자기 자신을 참조하는 값은 원본에서 스택이 넘친다. 이 깊이부터는 찾지 못한 것으로 처리한다.
constexpr int kMaxSubstitutionDepth = 32;
// 원본 줄 이동에서 건너뛰는 글자(LF·CR·공백·탭).
bool IsLineGap(char c) { return c == '\n' || c == '\r' || c == ' ' || c == '\t'; }
// 줄 앞·키 뒤에서 건너뛰는 글자(공백·탭).
bool IsBlank(char c) { return c == ' ' || c == '\t'; }
// CRT toupper의 "C" 로케일 동작: ASCII 소문자만 바꾼다.
char AsciiUpper(char c) { return c >= 'a' && c <= 'z' ? static_cast<char>(c - ('a' - 'A')) : c; }
// CRT strnicmp(left, right, right.size()) == 0 에 해당하는 접두어 비교. left가 짧으면 다르다.
bool StartsWithFolded(std::string_view left, std::string_view right) {
    return right.size() <= left.size() && AsciiLower(left.substr(0, right.size())) == AsciiLower(right);
}
// C 문자열처럼 첫 NUL 앞까지만 본다.
std::string_view UntilNul(std::string_view text) {
    const auto nul = text.find('\0');
    return nul == std::string_view::npos ? text : text.substr(0, nul);
}
// pos가 가리키는 줄의 다음 줄 첫 글자 위치: LF까지 간 뒤 LF·CR·공백·탭을 모두 건너뛴다. LF가 없으면 끝.
std::size_t NextLine(std::string_view text, std::size_t pos) {
    const auto newline = text.find('\n', pos);
    if (newline == std::string_view::npos) return text.size();
    pos = newline;
    // 빈 줄과 다음 줄의 들여쓰기까지 함께 건너뛴다.
    while (pos < text.size() && IsLineGap(text[pos])) ++pos;
    return pos;
}
}

// FUN_0043fd70의 키 길이 비교와 LF 기준 줄 이동을 복원한다.
std::optional<std::size_t> ConfigFindKey(std::string_view text, std::string_view key) {
    text = UntilNul(text);
    if (key.empty()) return std::nullopt; // 원본에서도 빈 키는 어느 줄과도 맞지 않는다.
    std::size_t pos = 0;
    // 버퍼 앞에서부터 첫 일치만 반환한다. CR 단독은 원본처럼 새 줄로 취급하지 않는다.
    while (pos < text.size()) {
        // 줄 앞 공백·탭을 건너뛴다.
        while (pos < text.size() && IsBlank(text[pos])) ++pos;
        const auto start = pos;
        if (StartsWithFolded(text.substr(pos), key)) {
            pos += key.size();
            // 키 뒤 공백·탭을 건너뛰어 '='를 확인한다.
            while (pos < text.size() && IsBlank(text[pos])) ++pos;
            if (pos < text.size() && text[pos] == '=') return start;
        }
        pos = NextLine(text, start);
    }
    return std::nullopt;
}
// FUN_00440150의 원시 값 읽기. 최대 버퍼 길이 assert는 C++ 동적 문자열로 대체한다.
std::optional<std::string> ConfigGetRaw(std::string_view text, std::string_view key) {
    text = UntilNul(text);
    const auto found = ConfigFindKey(text, key);
    if (!found) return std::nullopt;
    auto pos = text.find('=', *found);
    // 원본은 연속 '='도 값 앞에서 건너뛴다.
    while (pos < text.size() && (text[pos] == '=' || IsBlank(text[pos]))) ++pos;
    std::string result;
    bool quoted = false;
    char previous = '\0';
    // 줄 끝·따옴표 밖 주석 전까지 복사한다.
    for (; pos < text.size(); ++pos) {
        const char c = text[pos];
        if (c == '\r' || c == '\n' || (!quoted && c == '/' && pos + 1 < text.size() && text[pos+1] == '/')) break;
        if (c == '"' && previous != kConfigEscape) quoted = !quoted;
        else result.push_back(c);
        previous = c;
    }
    return result;
}

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
// 버퍼 전체에서 첫 일치 키를 찾는다.
std::optional<std::size_t> ConfigText::FindKey(std::string_view key) const { return ConfigFindKey(text_, key); }
// 버퍼 전체에서 첫 일치 키의 원시 값을 읽는다.
std::optional<std::string> ConfigText::GetRaw(std::string_view key) const { return ConfigGetRaw(text_, key); }
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

// 등록 순서를 그대로 공개한다. 조회는 뒤에서부터 한다.
std::span<Config* const> ConfigRegistry::Objects() const { return objects_; }
// 원본 0043fb90: 주어진 이름에 "."을 붙여 객체의 접두어와 대소문자 무시로 비교한다.
Config* ConfigRegistry::FindByName(std::string_view name) const {
    const auto wanted = AsciiLower(std::string(name) + kConfigObjectSeparator);
    // 먼저 등록한 객체부터 찾는다(원본의 0 → 개수 순서).
    for (auto* config : objects_) if (AsciiLower(config->Prefix()) == wanted) return config;
    return nullptr;
}
// 원본은 한도를 넘으면 assert 뒤 배열 밖에 쓴다. 새 구현은 예외로 멈춘다.
void ConfigRegistry::Register(Config* config) {
    if (objects_.size() >= kMaxConfigs) throw std::length_error("totalConfigs < MAX_CONFIGS");
    objects_.push_back(config);
}
// 같은 포인터를 모두 빼고 나머지의 순서를 유지한다.
void ConfigRegistry::Unregister(Config* config) { std::erase(objects_, config); }

// 이름이 있으면 "이름."을 접두어로 만든다. 이름 없는 객체의 접두어 길이는 0이다.
Config::Config(ConfigRegistry& registry, std::optional<std::string_view> name) : registry_(registry) {
    if (name) prefix_ = std::string(UntilNul(*name)) + kConfigObjectSeparator;
    registry_.Register(this);
}
// 버퍼는 문자열이 스스로 해제한다. 원본처럼 전역 목록에서 뺀다.
Config::~Config() { registry_.Unregister(this); }
// 원본의 `*this != 0` 검사.
bool Config::IsValid() const { return text_.has_value(); }
// 버퍼만 버린다.
void Config::Clear() { text_.reset(); }
// C 문자열처럼 첫 NUL 앞까지만 남긴다.
void Config::Assign(std::string text) {
    text.resize(UntilNul(text).size());
    text_ = std::move(text);
}
// 버퍼가 없으면 새로 만든다. 빈 입력도 CRLF 하나를 남긴다.
void Config::Append(std::string_view text) {
    text = UntilNul(text);
    auto end = text.size();
    // 입력 끝의 CR/LF만 정리한다.
    while (end > 0 && (text[end-1] == '\r' || text[end-1] == '\n')) --end;
    if (!text_) text_.emplace();
    text_->append(text.substr(0, end));
    text_->append("\r\n");
}
// 원본은 이름이 '['로 시작하면 assert 한다("sectionName[0] != startSection"). 새 구현은 예외로 거부한다.
void Config::AppendSection(std::string_view name) {
    if (!name.empty() && name.front() == kConfigOpenSection) throw std::invalid_argument("sectionName[0] != startSection");
    Append(std::string(1, kConfigOpenSection) + std::string(name) + kConfigCloseSection);
}
// 원본은 복호화한 내용을 그대로 더하므로 서명 `mQdsT`가 첫 줄 앞에 남아 그 줄의 키는 조회되지 않는다.
void Config::LoadBytes(std::span<const std::uint8_t> bytes) {
    std::vector<std::uint8_t> plain(bytes.begin(), bytes.end());
    if (plain.size() >= 3 && plain[0] == 0 && plain[2] == 0) ApplyXor(plain);
    // 원본은 C 문자열로 다루므로 첫 NUL에서 끝난다.
    plain.erase(std::find(plain.begin(), plain.end(), std::uint8_t{0}), plain.end());
    const bool hasSignature = plain.size() >= kConfigSignature.size()
        && std::equal(kConfigSignature.begin(), kConfigSignature.end(), plain.begin());
    // 새 UTF-8 파일은 서명 뒤(또는 파일 맨 앞)의 BOM으로 구분한다. 서명 자체는 ASCII라 그대로 둔다.
    const std::size_t skip = hasSignature ? kConfigSignature.size() : 0;
    Append(std::string(plain.begin(), plain.begin() + static_cast<std::ptrdiff_t>(skip))
        + DecodeOriginalText(std::span<const std::uint8_t>(plain).subspan(skip)));
}
// 원본 00440410: `@`로 시작하는 낱말은 그 이름('@' 포함)의 파일이 있으면 먼저 읽어 넣고, 나머지 글은 뒤에 더한다.
void Config::AppendArguments(std::string_view text,
    const std::function<std::optional<std::vector<std::uint8_t>>(std::string_view)>& readFile) {
    text = UntilNul(text);
    if (text.empty()) return;
    std::string inlineText;
    std::size_t pos = 0;
    // 글자마다 `@파일` 낱말과 `;` 구분을 처리한다.
    while (pos < text.size()) {
        if (text[pos] == '@') {
            const auto begin = pos;
            // 낱말은 공백·탭·`;` 또는 끝에서 끝난다. 낱말 자체는 버퍼에 복사하지 않는다.
            while (pos < text.size() && !IsBlank(text[pos]) && text[pos] != ';') ++pos;
            if (readFile) if (const auto bytes = readFile(text.substr(begin, pos - begin))) LoadBytes(*bytes);
            if (pos == text.size()) break; // 원본은 여기서 문자열 끝 너머를 읽는다. 새 구현은 멈춘다.
        }
        if (text[pos] == ';') inlineText += "\r\n";
        else inlineText.push_back(text[pos]);
        ++pos;
    }
    if (!inlineText.empty()) Append(inlineText);
}
// 버퍼가 없는 객체는 원본 0043fd70의 널 검사에 걸려 찾지 못한다.
std::optional<std::string> Config::GetRaw(std::string_view key) const {
    if (!text_) return std::nullopt;
    return ConfigGetRaw(*text_, key);
}
// 원본 004409d0: 버퍼가 없으면 0. 찾지 못해도 "Not Found" 표시를 남기지 않는다.
std::optional<std::string> Config::Get(std::string_view key) {
    if (!text_) return std::nullopt;
    std::string result;
    if (!Lookup(std::string(UntilNul(key)), {}, result, false, 0)) return std::nullopt;
    return result;
}
// 새 문자열에 치환 결과를 만든다.
std::string Config::Substitute(std::string_view source) {
    std::string result;
    SubstituteInto(result, UntilNul(source), 0);
    return result;
}
// 원본 00440b60: 버퍼가 없거나 입력이 비면 아무것도 쓰지 않는다.
void Config::SubstituteInto(std::string& dest, std::string_view source, int depth) {
    const auto start = dest.size();
    if (!text_ || source.empty()) return;
    std::size_t pos = 0;
    // 입력을 한 글자씩 옮기며 이스케이프와 괄호를 푼다.
    while (pos < source.size()) {
        const char c = source[pos];
        if (c == kConfigEscape) {
            ++pos;
            if (pos < source.size()) { // 끝의 외톨이 이스케이프는 버린다.
                const char escaped = source[pos++];
                dest.push_back(escaped == 'n' ? '\n' : escaped == 'r' ? '\r' : escaped == 't' ? '\t' : escaped);
            }
        } else if (c == kConfigOpenBrace) {
            ExpandBrace(dest, source, pos, depth);
        } else {
            dest.push_back(c);
            ++pos;
        }
    }
    // 끝의 공백·탭을 자른다. 이 호출이 쓴 첫 글자는 공백이어도 남긴다.
    auto end = dest.size();
    // 첫 글자만 남을 때까지 끝의 공백·탭을 지운다.
    while (end > start + 1 && IsBlank(dest[end-1])) --end;
    dest.resize(end);
}
// 원본 00440a00: 키는 대문자로 모으고(안쪽 괄호의 결과는 그대로), 기본값은 글자 그대로 모은다.
void Config::ExpandBrace(std::string& dest, std::string_view source, std::size_t& pos, int depth) {
    ++pos; // '{' 다음부터.
    std::string key;
    // 키: 끝·`}`·`|` 전까지. 길이 한도는 원본 지역 버퍼 크기다.
    while (pos < source.size() && source[pos] != kConfigCloseBrace && source[pos] != kConfigDefaultSeparator
           && key.size() < kLongestSymbol) {
        if (source[pos] == kConfigOpenBrace) ExpandBrace(key, source, pos, depth);
        else key.push_back(AsciiUpper(source[pos++]));
    }
    std::string fallback;
    if (pos < source.size() && source[pos] == kConfigDefaultSeparator) {
        ++pos;
        // 기본값: 끝·`}` 전까지. `|`는 여기서는 보통 글자다.
        while (pos < source.size() && source[pos] != kConfigCloseBrace && fallback.size() < kLongestSymbol) {
            if (source[pos] == kConfigOpenBrace) ExpandBrace(fallback, source, pos, depth);
            else fallback.push_back(source[pos++]);
        }
    }
    Lookup(std::move(key), fallback, dest, true, depth + 1);
    if (pos < source.size()) ++pos; // 닫는 `}`(또는 한도에서 멈춘 글자)를 건너뛴다.
}
// 원본 00440760. 반환값은 "찾았는가"이며, 기본값으로 채운 경우는 거짓이다.
bool Config::Lookup(std::string key, std::string_view fallback, std::string& dest, bool reportMissing, int depth) {
    bool found = false;
    std::string raw;
    if (depth > kMaxSubstitutionDepth) {
        // 새 보호 장치: 여기서 멈추고 아래의 "찾지 못함" 처리로 간다.
    } else if (!key.empty() && key.front() == '&') {
        // 레지스트리 값: `{`·`}` 앞에 이스케이프를 붙여 그대로 쓴다(원본 00440090). 치환하지 않는다.
        reportMissing = false;
        if (const auto value = registry_.registryValue ? registry_.registryValue(std::string_view(key).substr(1)) : std::nullopt) {
            // 값의 괄호가 다시 치환되지 않게 한다.
            for (const char c : *value) {
                if (c == kConfigOpenBrace || c == kConfigCloseBrace) dest.push_back(kConfigEscape);
                dest.push_back(c);
            }
            return true;
        }
    } else if (!key.empty() && key.front() == '@') {
        const auto dot = key.find(kConfigObjectSeparator, 1);
        if (dot == std::string::npos) return true; // 원본: 빈 값을 찾은 것으로 처리한다.
        const auto mission = key.substr(1, dot - 1);
        const auto missionKey = key.substr(dot + 1);
        key.resize(dot); // 원본은 '.' 자리에 NUL을 써서, 못 찾았을 때 표시되는 키가 여기서 잘린다.
        {
            // 임시 객체는 미션 이름을 접두어로 전역 목록에 잠시 올라간다.
            Config temporary(registry_, mission);
            if (registry_.loadMission) registry_.loadMission(temporary, mission);
            if (auto value = temporary.GetRaw(missionKey)) { raw = std::move(*value); found = true; }
        }
    } else {
        // 환경 변수: `_`로 시작하고 이 객체가 허용할 때만.
        if (!key.empty() && key.front() == '_' && environmentEnabled && registry_.environment) {
            if (auto value = registry_.environment(std::string_view(key).substr(1))) { raw = std::move(*value); found = true; }
        }
        const auto objects = registry_.Objects();
        // 마지막에 등록한 객체부터. 이름 있는 객체는 접두어가 맞는 키만 받고 접두어를 뗀 나머지로 찾는다.
        for (auto index = objects.size(); !found && index-- > 0;) {
            const Config& object = *objects[index];
            std::optional<std::string> value;
            if (object.prefix_.empty()) value = object.GetRaw(key);
            else if (StartsWithFolded(key, object.prefix_)) value = object.GetRaw(std::string_view(key).substr(object.prefix_.size()));
            if (value) { raw = std::move(*value); found = true; }
        }
    }
    if (found) { SubstituteInto(dest, raw, depth); return true; }
    if (!fallback.empty()) SubstituteInto(dest, fallback, depth);
    else if (reportMissing) {
        // 결과는 `{키}`다(키는 괄호 해석에서 이미 대문자). 원본은 `{` 뒤에 "Not Found:"(00506b50)를 쓰지만
        // 쓰는 위치를 옮기지 않고 같은 자리에 키를 복사해 덮어쓴다. 두 판본 x86 실행으로 확인했다.
        dest.push_back(kConfigOpenBrace);
        dest += key;
        dest.push_back(kConfigCloseBrace);
    }
    return false;
}
// 원본 00440240: 찾을 때마다 그 줄(키 위치부터 다음 줄 첫 글자 앞까지)을 지운다.
bool Config::RemoveKey(std::string_view key) {
    if (!text_) return false;
    bool removed = false;
    // 같은 키가 여러 줄이면 모두 지운다.
    while (const auto found = ConfigFindKey(*text_, key)) {
        text_->erase(*found, NextLine(*text_, *found) - *found);
        removed = true;
    }
    return removed;
}
// 원본 004404e0. '.'이 있으면 위에서부터 접두어가 맞는 첫 객체에 쓰고, 키는 첫 '.' 뒤로 바꾼다.
// 이름 없는 객체는 길이 0 비교라 항상 맞는다(원본 그대로).
bool Config::Set(std::string_view key, std::string_view value) {
    key = UntilNul(key);
    Config* target = this;
    const auto dot = key.find(kConfigObjectSeparator);
    if (dot != std::string_view::npos) {
        const auto objects = registry_.Objects();
        // 마지막에 등록한 객체부터 찾는다. 맞는 객체가 없으면 이 객체에 온전한 키로 쓴다.
        for (auto index = objects.size(); index-- > 0;) {
            if (StartsWithFolded(key, objects[index]->prefix_)) {
                target = objects[index];
                key = key.substr(dot + 1);
                break;
            }
        }
    }
    const bool existed = target->RemoveKey(key);
    // 원본 서식 "%s = \"%s\""(004402f0). 원본의 256바이트 서식 버퍼 한도는 옮기지 않았다.
    target->Append(std::string(key) + " = \"" + std::string(UntilNul(value)) + "\"");
    target->changed = true;
    return existed;
}
// 원본 0043fe60. 줄의 첫 글자가 '['인 줄에서만, 그 줄 안의 모든 '[' 위치를 header와 비교한다.
std::optional<std::size_t> ConfigFindSection(std::string_view text, std::size_t from,
    std::string_view header, std::string* matchedName) {
    text = UntilNul(text);
    std::size_t pos = from;
    // 줄 단위로 훑는다.
    while (pos < text.size()) {
        // 줄 앞 공백·탭을 건너뛴다.
        while (pos < text.size() && IsBlank(text[pos])) ++pos;
        const auto lineStart = pos;
        bool matched = false;
        if (pos < text.size() && text[pos] == kConfigOpenSection) {
            // 이 줄 안의 '['마다 비교한다.
            while (true) {
                if (StartsWithFolded(text.substr(pos), header)) { matched = true; break; }
                ++pos;
                // 같은 줄의 다음 '['까지 간다.
                while (pos < text.size() && text[pos] != '\n' && text[pos] != kConfigOpenSection) ++pos;
                if (pos >= text.size() || text[pos] == '\n') break;
            }
        }
        if (!matched) { pos = NextLine(text, lineStart); continue; }
        if (matchedName) {
            // '[' 다음부터 ']' 앞까지. 원본은 ']'가 없으면 끝없이 읽는다. 새 구현은 버퍼 끝에서 멈춘다.
            auto close = text.find(kConfigCloseSection, pos + 1);
            if (close == std::string_view::npos) close = text.size();
            *matchedName = std::string(text.substr(pos + 1, close - pos - 1));
            pos = close; // 원본은 이름을 복사한 자리(']')부터 줄 끝을 찾는다.
        }
        const auto newline = text.find('\n', pos);
        if (newline == std::string_view::npos) return std::nullopt; // 원본은 여기서 assert 한다.
        pos = newline + 1;
        if (pos < text.size() && text[pos] == '\r') ++pos; // LF 바로 뒤의 CR 하나도 건너뛴다.
        // 본문이 비어 버퍼 끝이면 원본 호출자들은 찾지 못한 것으로 다룬다.
        if (pos >= text.size()) return std::nullopt;
        return pos;
    }
    return std::nullopt;
}
// 원본 00440570: 이름이 '['로 시작하면 그대로, 아니면 `[이름]`으로 찾는다.
std::optional<std::string_view> Config::Section(std::string_view name) const {
    if (!text_) return std::nullopt;
    const std::string header = !name.empty() && name.front() == kConfigOpenSection
        ? std::string(name) : std::string(1, kConfigOpenSection) + std::string(name) + kConfigCloseSection;
    const std::string_view text = *text_;
    const auto start = ConfigFindSection(text, 0, header);
    if (!start) return std::nullopt;
    auto end = *start;
    // 줄 첫 글자(앞 글자가 LF·CR)가 '['인 곳이 다음 섹션이다.
    while (end < text.size() && !(text[end] == kConfigOpenSection && (text[end-1] == '\n' || text[end-1] == '\r'))) ++end;
    return text.substr(*start, end - *start);
}
// 원본 00440640: `[접두어`로 찾은 섹션마다 이름과 `;`를 붙인다.
std::string Config::SectionNames(std::string_view prefix) const {
    std::string result;
    if (!text_) return result;
    const std::string header = std::string(1, kConfigOpenSection) + std::string(prefix);
    std::size_t pos = 0;
    std::string name;
    // 찾은 줄의 다음 위치에서 이어서 찾는다.
    while (const auto next = ConfigFindSection(*text_, pos, header, &name)) {
        result += name;
        result += ';'; // 원본 DAT_00506b44.
        pos = *next;
    }
    return result;
}
// 접두어를 그대로 돌려준다.
const std::string& Config::Prefix() const { return prefix_; }
// 버퍼가 없으면 빈 문자열을 돌려준다.
const std::string& Config::Text() const {
    static const std::string empty;
    return text_ ? *text_ : empty;
}
}
