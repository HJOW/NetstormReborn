// 실제4d6eca..4d6f0d의 공통 객체 입력을 메뉴 생성 수명과 분리한다.
#include "client/ScenarioObjectInput.h"
#include "o/OriginalText.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 시나리오와 메뉴가 같은 입력 큐/현재 Session을 사용한다.
namespace { // 보호 PE에서 확인한 이름과 명령 번호만 지역화한다.
// 실제 이름표의 다섯 항목이며 친절한 별칭이나 시나리오 오자를 추가하지 않는다.
constexpr std::array<std::pair<std::string_view,std::uint32_t>,5> kObjectCommands{{
    {"makemine",0x030000afU},{"inflictdamage",0x030000e2U},{"destroypiece",0x03000015U},
    {"turntoobelisk",0x030000ceU},{"meltdown",0x030000f8U}}};
}
// 원본 스크립트 이름 비교는 ASCII 대소문자를 무시한다.
std::uint32_t ScenarioObjectScriptCode(std::string_view name) {
    const auto key=o::AsciiLower(name);
    for (const auto& [label,command]:kObjectCommands) if (key==label) return command; // 원문 이름만 같은 번호로 변환한다.
    return 0xffffffffU;
}
// 메뉴 버튼의 명령 허용과 일반 입력 분배가 같은 표를 사용한다.
bool IsScenarioObjectCommand(std::uint32_t command) {
    command&=0x03ffffffU;
    for (const auto& [label,value]:kObjectCommands) if (command==value) return true; // 같은 번호의 입력 플래그만 제거한다.
    return false;
}
// 현재 대상은 y의 낮은 WORD이며 메뉴·현재 선택·소유자는 추가 실행 조건이 아니다.
bool DispatchScenarioObjectInput(const InputEvent& event,const std::function<std::uint32_t(o::Sid,std::uint32_t)>& objectInput) {
    if (!IsScenarioObjectCommand(event.code)) return false;
    if (event.code&InputCode::kRelease) return true;
    const o::Sid target{static_cast<std::uint16_t>(std::bit_cast<std::uint32_t>(event.y))};
    if (target.value) {
        if (!objectInput) throw std::logic_error("시나리오 객체 명령의 현재 가상 입력 미연결");
        static_cast<void>(objectInput(target,event.code));
    }
    return true; // 원문4d6f09는 객체 반환값을 읽지 않고 사건의 y를 다시 읽는다.
}
}
