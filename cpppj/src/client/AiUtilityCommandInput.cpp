// 세 일반 입력은 별도 mode/demo/정지 관문 없이 각 기존 부모로 전달한다.
#include "client/AiUtilityCommandInput.h"
#include "o/OriginalText.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 실제 명령 이름과 전체DWORD 입력을 보존한다.
namespace { // 선택된 실제 부모만 연결 여부를 확인한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("AI/간헐천 일반 명령의 실제 부모 미연결");return hook; } // 빈 성공으로 명시 효과를 건너뛰지 않는다.
}
// 원문 이름 lookup의 ASCII 대소문자 무시 계약을 사용한다.
std::uint32_t AiUtilityCommandScriptCode(std::string_view name) {
    const auto key=o::AsciiLower(name);if (key=="createinitialgeysers") return kCreateInitialGeysersCommand;if (key=="braincrankmoney") return kBrainCrankMoneyCommand;if (key=="aiparamset") return kAiParamSetCommand;return 0xffffffffU;
}
// 호출자가 modifier/outside를 제거한 원문 명령값 세 개만 분류한다.
bool IsAiUtilityCommand(std::uint32_t code) { return code==kCreateInitialGeysersCommand || code==kBrainCrankMoneyCommand || code==kAiParamSetCommand; }
// x는 원문 세 명령에서 사용하지 않으며 parameter만 y의 전체32비트를 사용한다.
bool DispatchAiUtilityCommandInput(InputEvent event,const AiUtilityCommandHooks& hooks) {
    const auto code=event.code&0x03ffffffU;if (!IsAiUtilityCommand(code)) return false;if (event.code&InputCode::kRelease) return true;
    if (code==kCreateInitialGeysersCommand) Need(hooks.initialGeysers)();else if (code==kBrainCrankMoneyCommand) Need(hooks.crankMoney)();else Need(hooks.parameter)(std::bit_cast<std::uint32_t>(event.y));return true;
}
}
