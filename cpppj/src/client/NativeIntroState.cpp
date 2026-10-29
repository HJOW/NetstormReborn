// 원문 004b6ebd..004b6fa9의 분기/현재 전역 쓰기/하위 호출 순서를 보존한다.
#include "client/NativeIntroState.h"
#include <stdexcept>

namespace netstorm::client { // Native State와 호스트 영상 장치의 경계를 구분한다.
namespace { // 도달하지 않은 영상/준비 장치를 빈 성공으로 채우지 않는다.
// 현재 선택된 하위가 없으면 앞선 부분효과를 보존하고 명시 중단한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("Native 인트로의 선택된 실제 장치 미연결");return hook; }
}
// State39 준비 뒤의 원문 bool 쓰기는 cleanup이 끝난 현재 같은 물리에 적용한다.
o::LocalMissionParentResult InstallNativeIntroState(NativeIntroStateGlobals g,const NativeIntroStateHooks& h) {
    switch (g.state.current[0]) {
    case 39:
        if (g.state.current[35]) {
            Need(h.cleanup)();Need(h.hideChatInput)();Need(h.hideChat)();
            g.fort=true;g.server=true;g.boss=true;
            Need(h.core)();Need(h.renderer)();Need(h.hud)();
            const auto path=Need(h.moviePath)(); // 원문의 초기 NUL 버퍼에 ConfigPath가 작성한 실제 이름이다.
            if (!path.empty()) { Need(h.markIntro)();Need(h.openMovie)(path);Need(h.request)(40);break; }
        }
        Need(h.request)(41);break;
    case 40:
        if (Need(h.movieActive)()) g.state.next[0]=40;else Need(h.request)(41);
        break;
    case 41:
        if (g.pendingName.empty()) throw std::out_of_range("Native 인트로 복귀의 pending 이름 물리 없음");
        g.pendingName[0]=0;Need(h.request)(3);break;
    default:throw std::logic_error("Native 인트로 State39/40/41 계약");
    }
    return o::LocalMissionParentResult::Commit;
}
}
