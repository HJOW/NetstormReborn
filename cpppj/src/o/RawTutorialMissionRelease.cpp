// 현재 전역 참조와 본문 예외 뒤 기본 Mission 정리까지 원문 소멸의 순서대로 실행한다.
#include "o/RawTutorialMissionRelease.h"
#include <stdexcept>

namespace netstorm::o { // 같은 Tutorial 소멸을 Guide 교체와 명시 종료에서 공유한다.
namespace { // 도달한 원문 하위 장치만 확인하는 지역 경계다.
template<class Function> const Function& Need(const Function& function) { // 미연결 효과는 해당 호출 지점에서 중단하며 앞선 쓰기를 되돌리지 않는다.
    if (!function) throw std::logic_error("Tutorial 소멸의 실제 하위 효과 미연결");return function;
}
struct BaseMissionRetirement { // 원문 state0의 unwind와 정상 꼬리가 같은 기본 Mission을 정확히 한 번 반납하게 한다.
    const std::function<void()>& release; // 호출자와 같은 현재 기본 Mission 해제 장치를 빌린다.
    ~BaseMissionRetirement() noexcept(false) { Need(release)(); } // 정상 예외는 전파하며, 본문 unwind 중 다시 던지면 C++ 중첩 예외 규칙을 따른다.
};
}
void ReleaseTutorialMission(TutorialMissionReleaseState state,const TutorialMissionReleaseHooks& hooks) { // 원문 004c323c 이후 직접 쓰기와 004c3268 호출을 실행한다.
    BaseMissionRetirement base{hooks.releaseBaseMission}; // 본문 실패도 004fb940처럼 기본 Mission 정리를 먼저 수행한다.
    Need(hooks.setAllowAll)(1);
    state.denySalvage=0;state.denyAscend=0;state.missionNumber=0;state.tutorialNumber=0;state.stage=0;state.stageDirty=0;
    Need(hooks.removeAllBrains)();
}
}
