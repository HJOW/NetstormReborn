// 4차 로컬 MCP용 최소 JSON 구현이다. 깊이 제한이 있는 재귀 하강 파서와 결정적 직렬화기를 제공한다.
#include "platform/Json.h"
#include <charconv>
#include <cmath>
#include <cstdio>

namespace netstorm::platform { // MCP 메시지를 다루는 범용 부품이다.
namespace { // 파서/직렬화기 내부 보조다.
constexpr int kMaxDepth = 64; // 악의적/손상 입력의 스택 고갈을 막는 중첩 한도다.
const Json kNullJson; // 없는 키 조회가 돌려주는 공유 null이다.

// 문자열 하나를 JSON 규칙으로 이스케이프해 out에 덧붙인다. 큰따옴표/역슬래시/제어문자만 바꾸고 UTF-8은 그대로 둔다.
void AppendString(std::string& out, const std::string& text) {
    out.push_back('"');
    for (const unsigned char ch : text) { // 바이트 단위로 이스케이프가 필요한 문자만 바꾼다.
        switch (ch) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        default:
            if (ch < 0x20) { char buffer[8]; std::snprintf(buffer, sizeof(buffer), "\\u%04x", ch); out += buffer; }
            else out.push_back(static_cast<char>(ch));
        }
    }
    out.push_back('"');
}

// 재귀 하강 파서다. 위치와 깊이를 들고 있으며 실패하면 ok를 내린다.
struct Parser {
    std::string_view text; // 파싱할 전체 텍스트다.
    std::size_t at{};      // 다음에 읽을 위치다.
    bool ok{true};         // 지금까지 문법 오류가 없었는지다.
    void Whitespace() { while (at < text.size() && (text[at] == ' ' || text[at] == '\t' || text[at] == '\n' || text[at] == '\r')) ++at; } // 토큰 사이 공백을 건너뛴다.
    bool Take(char expected) { Whitespace(); if (at < text.size() && text[at] == expected) { ++at; return true; } return false; } // 기대한 한 글자를 소비한다.
    bool Literal(std::string_view word) { if (text.substr(at, word.size()) == word) { at += word.size(); return true; } return false; } // 키워드(true/false/null)를 소비한다.
    // \uXXXX의 네 자리 16진수를 읽는다.
    bool Hex4(std::uint32_t& value) {
        if (at + 4 > text.size()) return false;
        value = 0;
        for (int index = 0; index < 4; ++index) { // 네 자리를 상위부터 누적한다.
            const char ch = text[at++];
            value <<= 4;
            if (ch >= '0' && ch <= '9') value |= static_cast<std::uint32_t>(ch - '0');
            else if (ch >= 'a' && ch <= 'f') value |= static_cast<std::uint32_t>(ch - 'a' + 10);
            else if (ch >= 'A' && ch <= 'F') value |= static_cast<std::uint32_t>(ch - 'A' + 10);
            else return false;
        }
        return true;
    }
    // 코드포인트를 UTF-8로 덧붙인다.
    static void AppendUtf8(std::string& out, std::uint32_t code) {
        if (code < 0x80) out.push_back(static_cast<char>(code));
        else if (code < 0x800) { out.push_back(static_cast<char>(0xc0 | (code >> 6))); out.push_back(static_cast<char>(0x80 | (code & 0x3f))); }
        else if (code < 0x10000) { out.push_back(static_cast<char>(0xe0 | (code >> 12))); out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3f))); out.push_back(static_cast<char>(0x80 | (code & 0x3f))); }
        else { out.push_back(static_cast<char>(0xf0 | (code >> 18))); out.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3f))); out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3f))); out.push_back(static_cast<char>(0x80 | (code & 0x3f))); }
    }
    // 큰따옴표로 시작하는 문자열을 읽는다.
    std::optional<std::string> String() {
        if (!Take('"')) return std::nullopt;
        std::string out;
        while (at < text.size()) { // 닫는 따옴표까지 문자를 하나씩 해석한다.
            const unsigned char ch = static_cast<unsigned char>(text[at++]);
            if (ch == '"') return out;
            if (ch < 0x20) return std::nullopt; // 이스케이프되지 않은 제어문자는 JSON 위반이다.
            if (ch != '\\') { out.push_back(static_cast<char>(ch)); continue; }
            if (at >= text.size()) return std::nullopt;
            const char escape = text[at++];
            switch (escape) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                std::uint32_t code{};
                if (!Hex4(code)) return std::nullopt;
                if (code >= 0xd800 && code <= 0xdbff) { // 상위 대리쌍이면 뒤따르는 하위 대리쌍과 합쳐 한 코드포인트로 만든다.
                    std::uint32_t low{};
                    if (!Literal("\\u") || !Hex4(low) || low < 0xdc00 || low > 0xdfff) return std::nullopt;
                    code = 0x10000 + ((code - 0xd800) << 10) + (low - 0xdc00);
                } else if (code >= 0xdc00 && code <= 0xdfff) return std::nullopt; // 짝 없는 하위 대리쌍이다.
                AppendUtf8(out, code);
                break;
            }
            default: return std::nullopt;
            }
        }
        return std::nullopt; // 닫는 따옴표가 없다.
    }
    // 수를 읽는다. 정수 표기는 int64, 소수/지수는 double이다.
    std::optional<Json> Number() {
        const auto begin = at;
        if (at < text.size() && text[at] == '-') ++at;
        bool real = false;
        while (at < text.size()) { // 숫자와 소수점/지수 문자를 연속으로 모은다.
            const char ch = text[at];
            if (ch >= '0' && ch <= '9') ++at;
            else if (ch == '.' || ch == 'e' || ch == 'E' || ch == '+' || ch == '-') { real = true; ++at; }
            else break;
        }
        const auto token = text.substr(begin, at - begin);
        if (token.empty() || token == "-") return std::nullopt;
        if (!real) { std::int64_t value{}; const auto result = std::from_chars(token.data(), token.data() + token.size(), value); if (result.ec == std::errc() && result.ptr == token.data() + token.size()) return Json(value); }
        double value{};
        const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
        if (result.ec != std::errc() || result.ptr != token.data() + token.size() || !std::isfinite(value)) return std::nullopt;
        return Json(value);
    }
    // 값 하나를 읽는다.
    std::optional<Json> Value(int depth) {
        if (depth > kMaxDepth) return std::nullopt;
        Whitespace();
        if (at >= text.size()) return std::nullopt;
        const char ch = text[at];
        if (ch == '{') { // 객체: 키 문자열, 콜론, 값을 쉼표로 이어 읽는다.
            ++at; Json object = Json::MakeObject();
            if (Take('}')) return object;
            while (true) {
                auto key = String(); if (!key || !Take(':')) return std::nullopt;
                auto value = Value(depth + 1); if (!value) return std::nullopt;
                object.Set(std::move(*key), std::move(*value));
                if (Take(',')) continue;
                if (Take('}')) return object;
                return std::nullopt;
            }
        }
        if (ch == '[') { // 배열: 값을 쉼표로 이어 읽는다.
            ++at; Json array = Json::MakeArray();
            if (Take(']')) return array;
            while (true) {
                auto value = Value(depth + 1); if (!value) return std::nullopt;
                array.Push(std::move(*value));
                if (Take(',')) continue;
                if (Take(']')) return array;
                return std::nullopt;
            }
        }
        if (ch == '"') { auto text_value = String(); if (!text_value) return std::nullopt; return Json(std::move(*text_value)); }
        if (ch == 't') return Literal("true") ? std::optional<Json>(Json(true)) : std::nullopt;
        if (ch == 'f') return Literal("false") ? std::optional<Json>(Json(false)) : std::nullopt;
        if (ch == 'n') return Literal("null") ? std::optional<Json>(Json()) : std::nullopt;
        return Number();
    }
};
// 값 하나를 out에 직렬화한다.
void DumpTo(const Json& value, std::string& out) {
    switch (value.GetKind()) {
    case Json::Kind::Null: out += "null"; break;
    case Json::Kind::Bool: out += value.AsBool() ? "true" : "false"; break;
    case Json::Kind::Integer: out += std::to_string(value.AsInt()); break;
    case Json::Kind::Real: { char buffer[40]; const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value.AsDouble()); out.append(buffer, result.ptr); break; }
    case Json::Kind::String: AppendString(out, value.AsString()); break;
    case Json::Kind::Array: {
        out.push_back('[');
        bool first = true;
        for (const auto& item : value.Items()) { if (!first) out.push_back(','); first = false; DumpTo(item, out); } // 항목을 쉼표로 잇는다.
        out.push_back(']');
        break;
    }
    case Json::Kind::Object: {
        out.push_back('{');
        bool first = true;
        for (const auto& [key, item] : value.Members()) { if (!first) out.push_back(','); first = false; AppendString(out, key); out.push_back(':'); DumpTo(item, out); } // 키 사전순으로 멤버를 쓴다.
        out.push_back('}');
        break;
    }
    }
}
}

std::int64_t Json::AsInt(std::int64_t fallback) const {
    if (kind_ == Kind::Integer) return integer_;
    if (kind_ == Kind::Real) return static_cast<std::int64_t>(real_);
    return fallback;
}
double Json::AsDouble(double fallback) const {
    if (kind_ == Kind::Real) return real_;
    if (kind_ == Kind::Integer) return static_cast<double>(integer_);
    return fallback;
}
const Json& Json::operator[](std::string_view key) const {
    if (kind_ != Kind::Object) return kNullJson;
    const auto found = object_.find(std::string(key));
    return found == object_.end() ? kNullJson : found->second;
}
bool Json::Has(std::string_view key) const { return kind_ == Kind::Object && object_.contains(std::string(key)); }
Json& Json::Set(std::string key, Json value) {
    if (kind_ != Kind::Object) { *this = MakeObject(); }
    object_[std::move(key)] = std::move(value);
    return *this; // 연쇄 호출을 위해 객체 자신을 돌려준다.
}
Json& Json::Push(Json value) {
    if (kind_ != Kind::Array) { *this = MakeArray(); }
    array_.push_back(std::move(value));
    return *this; // 연쇄 호출을 위해 배열 자신을 돌려준다.
}
std::string Json::Dump() const { std::string out; DumpTo(*this, out); return out; }
std::optional<Json> Json::Parse(std::string_view text) {
    Parser parser{text};
    auto value = parser.Value(0);
    if (!value) return std::nullopt;
    parser.Whitespace();
    if (parser.at != text.size()) return std::nullopt; // 값 뒤의 잡문자는 거부한다.
    return value;
}
}
