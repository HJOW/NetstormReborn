// 4차 로컬 MCP용 최소 JSON 값/파서/직렬화기다. 원본에는 없는 새 장치이며 외부 라이브러리 없이 MCP(JSON-RPC 2.0) 메시지만 다룬다.
#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::platform { // OS/프로토콜 경계의 범용 부품이다.
// JSON 값 하나다. 수는 정수 표기면 int64, 그 밖에는 double로 보존한다.
class Json {
public:
    // 값의 종류다.
    enum class Kind { Null, Bool, Integer, Real, String, Array, Object };
    using Array = std::vector<Json>;               // 배열 값이다.
    using Object = std::map<std::string, Json>;    // 객체 값이다. 직렬화 순서는 키 사전순으로 고정해 검사가 결정적이다.
    Json() = default;                              // null이다.
    Json(std::nullptr_t) {}                        // 명시 null이다.
    Json(bool value) : kind_(Kind::Bool), integer_(value ? 1 : 0) {}
    Json(int value) : kind_(Kind::Integer), integer_(value) {}
    Json(std::int64_t value) : kind_(Kind::Integer), integer_(value) {}
    Json(std::uint32_t value) : kind_(Kind::Integer), integer_(value) {}
    Json(std::uint64_t value) : kind_(Kind::Integer), integer_(static_cast<std::int64_t>(value)) {}
    Json(double value) : kind_(Kind::Real), real_(value) {}
    Json(const char* value) : kind_(Kind::String), text_(value) {}
    Json(std::string value) : kind_(Kind::String), text_(std::move(value)) {}
    Json(std::string_view value) : kind_(Kind::String), text_(value) {}
    Json(Array value) : kind_(Kind::Array), array_(std::move(value)) {}
    Json(Object value) : kind_(Kind::Object), object_(std::move(value)) {}
    // 빈 배열/객체를 만든다.
    static Json MakeArray() { return Json(Array{}); }
    static Json MakeObject() { return Json(Object{}); }
    Kind GetKind() const { return kind_; }                                // 현재 종류다.
    bool IsNull() const { return kind_ == Kind::Null; }
    bool IsObject() const { return kind_ == Kind::Object; }
    bool IsArray() const { return kind_ == Kind::Array; }
    bool IsString() const { return kind_ == Kind::String; }
    bool IsNumber() const { return kind_ == Kind::Integer || kind_ == Kind::Real; }
    bool IsBool() const { return kind_ == Kind::Bool; }
    // 값 접근자다. 종류가 다르면 fallback을 돌려주며 예외를 던지지 않는다(외부 입력을 방어적으로 읽는다).
    std::string AsString(std::string fallback = {}) const { return kind_ == Kind::String ? text_ : fallback; }
    std::int64_t AsInt(std::int64_t fallback = 0) const;                  // 정수/실수를 정수로 읽는다.
    double AsDouble(double fallback = 0.0) const;                          // 정수/실수를 실수로 읽는다.
    bool AsBool(bool fallback = false) const { return kind_ == Kind::Bool ? integer_ != 0 : fallback; }
    const Array& Items() const { return array_; }                          // 배열 항목이다(배열이 아니면 빈 배열).
    const Object& Members() const { return object_; }                      // 객체 멤버다(객체가 아니면 빈 객체).
    // 객체 멤버를 찾는다. 없거나 객체가 아니면 null 값을 돌려준다.
    const Json& operator[](std::string_view key) const;
    bool Has(std::string_view key) const;                                  // 객체에 키가 있는지다.
    // 객체에 멤버를 쓴다(객체가 아니면 객체로 바꾼다). 연쇄 호출을 위해 객체 자신을 돌려준다.
    Json& Set(std::string key, Json value);
    // 배열 끝에 항목을 더한다(배열이 아니면 배열로 바꾼다). 연쇄 호출을 위해 배열 자신을 돌려준다.
    Json& Push(Json value);
    // 압축된 JSON 텍스트로 직렬화한다. 비ASCII는 UTF-8 그대로 두고 제어문자만 이스케이프한다.
    std::string Dump() const;
    // 텍스트를 파싱한다. 문법 오류나 뒤따르는 잡문자가 있으면 빈 값을 돌려준다.
    static std::optional<Json> Parse(std::string_view text);
private:
    Kind kind_{Kind::Null};     // 값의 종류다.
    std::int64_t integer_{};    // Bool(0/1)과 Integer의 값이다.
    double real_{};             // Real의 값이다.
    std::string text_;          // String의 값(UTF-8)이다.
    Array array_;               // Array의 항목이다.
    Object object_;             // Object의 멤버다.
};
}
