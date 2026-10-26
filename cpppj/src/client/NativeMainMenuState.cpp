// 결과를 합성하지 않고 State3의 현재 전역/선택 하위/부분효과를 원문 순서로 보존한다.
#include "client/NativeMainMenuState.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client { //실제 장기 Client 참조를 소비하는 Native 큰 부모다.
namespace { //원문에서 선택한 장치만 도달 위치에서 요구한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("Native State3 선택 장치 미연결");return function; }
// 원문의 strcpy처럼 NUL까지만 쓰며 뒤의 기존 BYTE를 지우지 않는다.
void Background(std::span<char> target,std::string_view name) {
    if (name.size()>=target.size()) throw std::out_of_range("Native State3 배경 이름 물리 부족");std::copy(name.begin(),name.end(),target.begin());target[name.size()]=0;
}
}
// Native State 공통144BYTE 확정과 다음 Poll은 이 부모 밖의 기존 RawLocalMissionState가 수행한다.
o::LocalMissionParentResult InstallNativeMainMenuState(NativeMainMenuStateGlobals g,const NativeMainMenuStateHooks& h) {
    if (g.state.current[0]!=3) throw std::logic_error("Native State3 실제 current 계약");
    g.state.current[33]=0;g.state.immediate[33]=0;g.state.next[33]=0;
    if (!Need(h.configInt)("intro")) { Need(h.request)(39);g.state.immediate[35]=1;return o::LocalMissionParentResult::Commit; } //005c8754는 별도 사본이 아니라 immediate+8c의 같은 DWORD다.
    auto source=g.sourceType;if (std::bit_cast<std::int32_t>(g.state.phase[0])<=6 || g.state.phase[0]==11) { g.sourceType=0;source=0; } //원문004b6ff7의 signed JLE를 high-bit phase에서도 유지한다.
    const auto outcome=g.outcome;if (g.background.empty()) throw std::out_of_range("Native State3 배경 첫BYTE 물리 부족");g.background[0]=0;
    if (outcome==1) { Need(h.deleteSavedState)();Background(g.background,"victory");Need(h.requestMusic)("fanfare.mus"); }
    else if (outcome==2) { Need(h.deleteSavedState)();Background(g.background,"defeat");Need(h.requestMusic)("defeat.mus"); }
    else Background(g.background,"titleMenu");
    g.outcome=0;
    if (source) { Need(h.deleteSavedState)();Need(h.requestMusic)("fanfare.mus");if (const auto name=Need(h.sourceBackground)(g.sourceType)) Background(g.background,*name); }
    if (g.battle && g.state.phase[0]==14) Need(h.leaveBattle)();
    Need(h.prefix)();Need(h.hudPrefix)();
    if (!g.pendingName.empty()) g.pendingName[0]=0;if (g.currentName.empty()) throw std::out_of_range("Native State3 현재 Fort 첫BYTE 물리 부족");g.currentName[0]=0;
    Need(h.title)();if (g.state.current[0]!=3) { Need(h.changedState)();throw std::logic_error("Native State3 제목 뒤 다른 설치 분기의 복귀"); }
    Need(h.closeGuide)();
    if (!Need(h.configInt)("WelcomeGiven")) Need(h.tell)("Welcome");
    else {
        Need(h.tell)("Blank");
        if (!Need(h.startup)() && g.firstMenu) { if (!Need(g.userUpdated)()) Need(h.tell)("UserNotUpdated");if (Need(g.tellTips)()) Need(h.tell)("Tips"); }
        g.firstMenu=0;
    }
    if (g.editor) Need(h.editorPlayers)();
    Need(h.tail)();return o::LocalMissionParentResult::Commit;
}
}
