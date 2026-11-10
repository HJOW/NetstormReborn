// 원문 0054db38 helpWord 전역 캐시의 소유자다. 도움말 항목 문구(예: "Help", 한국어 "정보")를 표시 바이트로 보관한다.
#pragma once
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::client {
// 원문 4413c0("helpWord",&0054db38)의 갱신 규칙을 그대로 따르는 NULL 가능 문자열 캐시다.
// 사용법: Client가 장기 보유하고 옵션 해석(InterpretOptions) 때마다 Interpret를 호출한다. 메뉴/도움말 소비자는 Get으로 현재값을 읽는다.
// 추가 이유(M7.1, 1268): 소비자마다 Config.Get을 매번 다시 읽던 방식은 키 부재/빈 값/대소문자 동일 경우의 원문 수명(옛 할당 유지)을 지키지 못했다.
class ClientHelpWord {
public:
    // 최초에는 원문 BSS처럼 NULL이며 파일/GUI/Config를 읽지 않는다.
    ClientHelpWord() = default;
    // 장기 소유자의 주소를 고정하므로 복사와 이동을 금지한다.
    ClientHelpWord(const ClientHelpWord&) = delete;
    // 현재 소비자가 보유한 주소를 대입으로 바꾸지 않는다.
    ClientHelpWord& operator=(const ClientHelpWord&) = delete;
    // 현재 캐시 값을 돌려준다. nullopt는 원문 NULL 포인터이며 빈 문자열("")과 구별한다.
    const std::optional<std::string>& Get() const { return value_; }
    // 441300/4413c0의 규칙을 적용한다. 설정값이 없거나 빈 문자열이면 아무것도 바꾸지 않고 false를 돌려준다.
    // 비어 있지 않은 설정값이 있으면, 기존 캐시와 대소문자 무시로 같을 때 옛 표기를 유지하고, 다를 때만 새 문자열로 교체하며 true를 돌려준다.
    // configuredDisplay는 호출자가 EncodeDisplayText를 한 번만 적용한 표시 바이트다.
    bool Interpret(const std::optional<std::string>& configuredDisplay);
    // 실제 교체가 일어난 횟수다. 같은 표기 유지와 교체를 관측 검사가 구별하는 용도다.
    unsigned Generation() const { return generation_; }
    // ASCII 대소문자 무시 비교다(원문 _stricmp). 한글 등 상위 바이트는 그대로 비교한다.
    static bool EqualsIgnoreCase(std::string_view left, std::string_view right);
private:
    // value_: 현재 캐시 문자열이다(없으면 NULL).
    std::optional<std::string> value_;
    // generation_: 실제로 새 문자열로 교체한 횟수다.
    unsigned generation_{};
};
}
