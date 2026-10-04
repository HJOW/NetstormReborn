// 원본 RiftType.cpp의 자산 문법·프레임 코드 생성 부분을 읽기 전용 로더로 복원한다.
// 버퍼·가상 주소를 직접 재현하지 않고 소유권 있는 문자열·배열을 사용한다.
#include "o/RiftType.h"
#include "o/OriginalText.h"
#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 자산 문법의 식별자·문자열·숫자·구분자·파일 끝을 구분한다.
enum class TokenKind { Word, String, Number, Punctuation, End };
struct Token { TokenKind kind{}; std::string text; double number{}; };

// 원본 식별자·플래그 비교에서 쓰는 ASCII 대소문자 규칙.
bool Equal(std::string_view left, std::string_view right) {
    return AsciiLower(left) == AsciiLower(right);
}
// 스크립트 식별자의 영문자·밑줄을 구분한다.
bool Letter(char ch) { return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_'; }
// 숫자는 원본처럼 십진수로 읽으며 00도 8진수로 바꾸지 않는다.
bool Digit(char ch) { return ch >= '0' && ch <= '9'; }

// 주석은 문자열 바깥에서만 제거한다. 문자열 안의 경로·//·세미콜론은 그대로 둔다.
class TypeLexer {
public:
    // 파싱 동안만 원문을 참조하고 NUL 뒤의 데이터는 읽지 않는다.
    explicit TypeLexer(std::string_view text) : text_(text.substr(0, text.find('\0'))) { Next(); }
    // 현재 토큰을 검사한다.
    const Token& Current() const { return token_; }
    // 예상한 문장부호를 소비하거나 잘못된 문법을 보고한다.
    void Expect(std::string_view punctuation) {
        if (token_.kind != TokenKind::Punctuation || token_.text != punctuation) Fail();
        Next();
    }
    // 식별자 또는 문자열을 소유권 있는 값으로 읽는다.
    std::string Take(TokenKind kind) {
        if (token_.kind != kind) Fail();
        auto result = token_.text;
        Next();
        return result;
    }
    // 숫자 토큰을 읽는다.
    double TakeNumber() {
        if (token_.kind != TokenKind::Number) Fail();
        const auto result = token_.number;
        Next();
        return result;
    }
    // 다음 토큰을 읽고 닫히지 않은 문자열·잘못된 숫자를 거부한다.
    void Next() {
        SkipSpace();
        token_ = {};
        if (position_ == text_.size()) { token_.kind = TokenKind::End; return; }
        const char ch = text_[position_];
        if (ch == '"') {
            token_.kind = TokenKind::String;
            const auto start = ++position_;
            // 원본 자산의 문자열에는 Windows 역슬래시가 있으므로 C 이스케이프로 해석하지 않는다.
            while (position_ < text_.size() && text_[position_] != '"') ++position_;
            if (position_ == text_.size()) Fail();
            token_.text = std::string(text_.substr(start, position_-start));
            ++position_;
        } else if (Letter(ch)) {
            token_.kind = TokenKind::Word;
            const auto start = position_++;
            // A00·sunCannon·hotFootRatioX 같은 이름을 하나로 읽는다.
            while (position_ < text_.size() && (Letter(text_[position_]) || Digit(text_[position_]))) ++position_;
            token_.text = std::string(text_.substr(start, position_-start));
        } else if (Digit(ch) || ch == '-' || ch == '+' || ch == '.') {
            token_.kind = TokenKind::Number;
            const auto* start = text_.data()+position_;
            const auto* numeric = start+(ch == '+' ? 1 : 0);
            const auto parsed = std::from_chars(numeric, text_.data()+text_.size(), token_.number, std::chars_format::general);
            if (parsed.ec != std::errc{} || parsed.ptr == numeric || !std::isfinite(token_.number)) Fail();
            position_ = static_cast<std::size_t>(parsed.ptr-text_.data());
            token_.text = std::string(start, parsed.ptr);
        } else {
            token_.kind = TokenKind::Punctuation;
            token_.text.assign(1, ch);
            ++position_;
        }
    }
private:
    // 행 끝 문자를 포함한 공백과 // 주석을 건너뛰어 다음 토큰으로 이동한다.
    void SkipSpace() {
        // 주석을 건너뛴 뒤에도 연속 공백·주석이 나올 수 있다.
        while (position_ < text_.size()) {
            const char ch = text_[position_];
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '\f' || ch == '\v') { ++position_; continue; }
            if (ch == '/' && position_+1 < text_.size() && text_[position_+1] == '/') {
                // CRCRLF 파일과 일반 LF 파일을 모두 읽는다.
                while (position_ < text_.size() && text_[position_] != '\n') ++position_;
                continue;
            }
            break;
        }
    }
    // 오류 위치를 붙여 손상된 정의 파일을 찾기 쉽게 한다.
    [[noreturn]] void Fail() const { throw std::runtime_error("Invalid .type syntax near byte " + std::to_string(position_)); }
    std::string_view text_;
    std::size_t position_{};
    Token token_;
};

// _ftol의 정상 범위에서 0 방향으로 버리고 원본 char 저장의 하위 8비트를 보존한다.
std::uint8_t FrameNumber(double number) {
    const auto truncated = std::trunc(number);
    if (truncated < std::numeric_limits<std::int32_t>::min() || truncated > std::numeric_limits<std::int32_t>::max())
        throw std::runtime_error("Frame number is outside recovered integer range");
    return static_cast<std::uint8_t>(static_cast<std::int32_t>(truncated));
}
// 클러스터 이름을 원본의 측면·변형·번호 코드로 분리한다.
FrameCode ReadFrameCode(std::string_view label, TypeLexer& lexer) {
    std::size_t digits = 0;
    // 문자가 끝나는 자리부터 프레임 번호가 시작한다.
    while (digits < label.size() && Letter(label[digits])) ++digits;
    if (digits == 0 || label.front() < 'A' || label.front() > 'Z') throw std::runtime_error("Invalid frame side orientation");
    double number = 0;
    if (digits == label.size()) number = lexer.TakeNumber();
    else {
        const auto parsed = std::from_chars(label.data()+digits, label.data()+label.size(), number);
        if (parsed.ec != std::errc{} || parsed.ptr != label.data()+label.size()) throw std::runtime_error("Invalid cluster frame number");
    }
    return {static_cast<std::uint8_t>(label[0]), static_cast<std::uint8_t>(digits == 1 ? 'P' : label[1]), FrameNumber(number), 0};
}
}

// 순서·중복·미사용 플래그·GIF 참조를 보존하며 런타임 프레임 코드를 생성한다.
RiftTypeDefinition RiftTypeDefinition::Parse(std::string_view text) {
    TypeLexer lexer(text);
    RiftTypeDefinition type;
    if (!Equal(lexer.Take(TokenKind::Word), "typename")) throw std::runtime_error("Missing typename");
    type.name = lexer.Take(TokenKind::Word);
    // constructor / client constructor 수식어와 선택적인 typeflags를 읽는다.
    while (lexer.Current().kind == TokenKind::Word) {
        const auto word = lexer.Take(TokenKind::Word);
        if (Equal(word, "typeflags")) {
            // 원본 로더의 게임별 비트 매핑은 후속 작업이며 이름 목록을 먼저 보존한다.
            while (lexer.Current().kind == TokenKind::Word) type.flags.push_back(lexer.Take(TokenKind::Word));
            lexer.Expect(";");
        } else {
            if (!type.constructor.empty()) type.constructor += ' ';
            type.constructor += word;
        }
    }
    lexer.Expect("{");
    // 속성은 원문 순서대로 저장하고 조회할 때 뒤의 지정값을 우선한다.
    while (lexer.Current().kind == TokenKind::Word) {
        TypeProperty property;
        property.name = lexer.Take(TokenKind::Word);
        lexer.Expect("=");
        if (lexer.Current().kind == TokenKind::String) property.value = lexer.Take(TokenKind::String);
        else property.value = lexer.TakeNumber();
        lexer.Expect(";");
        type.properties.push_back(std::move(property));
    }
    lexer.Expect("}");
    bool hasHelp = false;
    // 선언 순서가 SHP의 레이어별 프레임 순서이므로 정렬하지 않는다.
    while (lexer.Current().kind != TokenKind::End) {
        TypeCluster cluster;
        cluster.name = lexer.Take(TokenKind::Word);
        cluster.code = ReadFrameCode(cluster.name, lexer);
        lexer.Expect(":");
        if (type.clusters.size() >= static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::runtime_error("Too many type clusters");
        const auto index = static_cast<int>(type.clusters.size());
        // 알려진 7개 비트 외의 단어는 원본처럼 프레임 비트에 반영하지 않는다.
        while (lexer.Current().kind == TokenKind::Word) {
            const auto flag = lexer.Take(TokenKind::Word);
            if (Equal(flag, "fringe")) cluster.code.flags |= 0x01;
            if (Equal(flag, "suck")) cluster.code.flags |= 0x02;
            if (Equal(flag, "rim")) cluster.code.flags |= 0x04;
            if (Equal(flag, "lit")) cluster.code.flags |= 0x08;
            if (Equal(flag, "unlit")) cluster.code.flags |= 0x10;
            if (Equal(flag, "cracked")) cluster.code.flags |= 0x20;
            if (Equal(flag, "hard")) cluster.code.flags |= 0x40;
            if (Equal(flag, "baseframe")) type.specialFrames.baseFrame = index;
            if (Equal(flag, "gumpframe")) type.specialFrames.gumpFrame = index;
            if (Equal(flag, "default")) {
                type.specialFrames.defaultFrame = index;
                if (!hasHelp) type.specialFrames.helpFrame = index;
            }
            if (Equal(flag, "help")) { type.specialFrames.helpFrame = index; hasHelp = true; }
        }
        // GIF 이름은 참조만 기록하고 실제 그림은 원본 SHP 인덱스로 선택한다.
        do {
            lexer.Expect(":");
            auto file = lexer.Take(TokenKind::String);
            lexer.Expect("#");
            cluster.images.push_back({std::move(file), lexer.TakeNumber()});
        } while (lexer.Current().kind == TokenKind::Punctuation && lexer.Current().text == ":");
        lexer.Expect(";");
        type.clusters.push_back(std::move(cluster));
    }
    return type;
}

// 원본 로더가 속성을 순서대로 덮어쓰는 효과를 보존한다.
const TypeProperty* RiftTypeDefinition::Property(std::string_view nameToFind) const {
    // 뒤에서 찾으면 같은 속성의 마지막 지정값을 얻는다.
    for (auto it = properties.rbegin(); it != properties.rend(); ++it) if (Equal(it->name, nameToFind)) return &*it;
    return nullptr;
}
// 문자형 속성은 자동 수치 변환하지 않는다.
std::optional<double> RiftTypeDefinition::Number(std::string_view propertyName) const {
    const auto* property = Property(propertyName);
    if (property) if (const auto* value = std::get_if<double>(&property->value)) return *value;
    return {};
}
// 숫자형 속성은 자동 문자열 변환하지 않는다.
std::optional<std::string_view> RiftTypeDefinition::String(std::string_view propertyName) const {
    const auto* property = Property(propertyName);
    if (property) if (const auto* value = std::get_if<std::string>(&property->value)) return *value;
    return {};
}
// 기존 기계어 검증을 통과한 검색 클래스를 그대로 사용한다.
RiftTypeFrames RiftTypeDefinition::FrameTable() const {
    std::vector<FrameCode> codes;
    codes.reserve(clusters.size());
    // 각 클러스터가 원본 코드 배열의 원소 하나에 해당한다.
    for (const auto& cluster : clusters) codes.push_back(cluster.code);
    return RiftTypeFrames(std::move(codes));
}
// 자산에 저장된 숫자와 실제 배치 격자를 구분한다.
std::array<int, 2> RiftTypeDefinition::Footprint() const {
    std::array<int, 2> result{};
    // 정상 _ftol 범위의 두 축을 원본처럼 버림 변환한다.
    for (std::size_t i = 0; i < result.size(); ++i) {
        const auto number = std::trunc(Number(i == 0 ? "foot_x" : "foot_y").value_or(0));
        if (number < std::numeric_limits<int>::min() || number > std::numeric_limits<int>::max()) throw std::runtime_error("Footprint is outside integer range");
        result[i] = static_cast<int>(number);
    }
    if (result[1] == 6) result[1] = 8;
    return result;
}
}
