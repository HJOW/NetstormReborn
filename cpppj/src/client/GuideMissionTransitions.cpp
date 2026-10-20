// 큰 수명 효과 뒤 현재 재조회·문자열 소유 복사·마지막 필드 복원을 원본 순서대로 실행한다.
#include "client/GuideMissionTransitions.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 실제 도달한 효과만 필수로 요구한다. 선택되지 않은 효과를 빈 성공으로 채우지 않는다.
template<class Function> const Function& TransitionEffect(const Function& function) {
    if (!function) throw std::logic_error("Guide 교체의 실제 수명 효과 연결 누락");return function;
}
}
// Begin은 기존 해제 중 새 현재가 생겨도 재확인 루프 없이 한 번의 공장으로 덮어쓴다.
void BeginGuideMission(std::string_view name,const GuideMissionTransitionHooks& hooks) {
    if (TransitionEffect(hooks.current)()) { TransitionEffect(hooks.finish)();TransitionEffect(hooks.release)(); }
    const std::string copied(name);TransitionEffect(hooks.create)(copied); // 기존 해제 뒤 Guide 이름을 복사하는 원본 위치다.
}
// Fork의 설정 결과는 첫 해제 전에 반드시 소유 복사한다.
void ForkGuideMission(std::uint32_t parameter,const GuideMissionTransitionHooks& hooks) {
    const auto key="fork"+std::to_string(std::bit_cast<std::int32_t>(parameter)); // 원본 fork%d는 signed DWORD 서식이다.
    const auto target=TransitionEffect(hooks.get)(key);
    TransitionEffect(hooks.finish)();TransitionEffect(hooks.release)();BeginGuideMission(target,hooks);
}
// Restart의 첫 해제에는 finish가 없다. 재진입 현재만 Begin이 끝내며 필드는 공장 뒤 복원한다.
void RestartGuideMission(const MissionReloadRequest& request,const GuideMissionTransitionHooks& hooks) {
    TransitionEffect(hooks.release)();BeginGuideMission(request.name,hooks);
    TransitionEffect(hooks.mode)(request.mode);if (TransitionEffect(hooks.current)()) TransitionEffect(hooks.demo)(request.demo);
}
// End는 해제 재진입 현재를 추가로 제거하지 않는 원본 한 번의 동작이다.
void EndGuideMission(const GuideMissionTransitionHooks& hooks) { TransitionEffect(hooks.finish)();TransitionEffect(hooks.release)(); }
// 잘못된 Begin 주소의 진단도 원본처럼 별도 효과이며 반환 후 같은 인자로 계속한다.
bool DispatchGuideMissionCommand(const InputEvent& event,const GuideMissionCommandHooks& hooks) {
    const auto command=event.code&0x03ffffffU,parameter=std::bit_cast<std::uint32_t>(event.y); // x와 무관한 원본 세 번째 DWORD다.
    if (command==kMissionBeginCommand) {
        if (event.y<=100) TransitionEffect(hooks.invalidBegin)(parameter);
        TransitionEffect(hooks.begin)(parameter);return true;
    }
    if (command==kMissionReloadCommand) { TransitionEffect(hooks.restart)();return true; }
    if (command==kMissionForkCommand) { TransitionEffect(hooks.fork)(parameter);return true; }
    if (command==kMissionEndCommand) { TransitionEffect(hooks.end)();return true; }
    return false;
}
}
