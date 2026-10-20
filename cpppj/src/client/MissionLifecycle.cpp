// 원본 Guide의 단계/현재 플래그와 외부 효과 뒤 최종 쓰기 순서를 보존한다.
#include "client/MissionLifecycle.h"
#include <stdexcept>

namespace netstorm::client {
// 도달한 큰 효과만 필수로 요구한다. 빈 단계6은 원본004437f0의 실제 무동작이다.
void RunTutorialMissionStage(o::MissionStartState& state,std::uint32_t sequence,const TutorialMissionStageHooks& hooks) {
    switch (sequence) {
    case 0:MissionFirst(state,hooks.showStage);return;
    case 1:
        if (!hooks.initialize) throw std::logic_error("Guide 초기화의 실제 월드 설치 연결 누락");hooks.initialize();return;
    case 2:
        if (!hooks.tellGlobal) throw std::logic_error("Guide init Tell 연결 누락");hooks.tellGlobal("init",1);state.flags|=1U;return;
    case 3:
        if (!hooks.start) throw std::logic_error("Guide 단계3 실제 Start 연결 누락");hooks.start();return;
    case 4:
        if (!hooks.frame) throw std::logic_error("Guide 단계4 실제 공통 프레임 연결 누락");hooks.frame();return;
    case 5: {
        if (!hooks.tellMission) throw std::logic_error("Guide 종료 안내의 현재 미션 조회 연결 누락");
        const auto shown=hooks.tellMission((state.flags&4U) ? "Success" : "Failure"); // 원본 BYTE의 성공 bit4만 읽는다.
        if (shown) {
            if (!hooks.closeTransient || !hooks.pause) throw std::logic_error("Guide 종료 안내의 실제 팝업/정지 연결 누락");
            hooks.closeTransient();hooks.pause();
        }
        state.flags|=1U;return;
    }
    case 6:return;
    default:throw std::logic_error("Guide 가상 단계0..6 범위 위반");
    }
}
// 원본의 현재 미션 포인터 조회는 한 번이며 없음이면 가상 함수를 읽지 않는다.
void PauseMissionWorldEntry(const std::function<bool()>& current,const std::function<void()>& pause) {
    if (!current) throw std::logic_error("월드 진입의 현재 Guide 조회 누락");
    if (current()) { if (!pause) throw std::logic_error("월드 진입의 실제 미션 Pause 연결 누락");pause(); }
}
}
