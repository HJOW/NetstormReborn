// 디컴파일에서 빠진 인자는 원본 PE004b7d07..004b7e70의 실제 push/주소로 보완한다.
#include "client/NativeMetaState.h"
#include "client/ClientChallengeGlobals.h"
#include "o/RawAmbientSound.h"
#include <stdexcept>

namespace netstorm::client { // 메타 부모와 실제 장기 Client 데이터의 접합 계층이다.
namespace { // 선택된 장치의 누락만 호출 위치에서 진단한다.
// 미연결 하위에서 앞선 원문 쓰기/부분효과를 되돌리지 않는다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("Native State19 선택 장치 미연결");return function; }
}
// CS 할당 실패도 원문처럼 후속 요청/파일/UI를 계속하며 공통 확정을 대신하지 않는다.
o::LocalMissionParentResult InstallNativeMetaState(NativeMetaStateGlobals g,const NativeMetaStateHooks& h) {
    if (g.state.current[0]!=19) throw std::logic_error("Native State19 실제 current 계약");
    if (g.background.empty()) throw std::out_of_range("Native State19 배경 첫BYTE 물리 부족");g.background[0]=0;
    const auto palette=Need(h.fortPalette)();Need(h.applyPalette)(palette);Need(h.cleanup)(0);
    g.cdPresent=Need(h.checkCd)();g.metamap=true;g.server=false;g.boss=false;
    Need(h.initializeRenderer)();g.pending=0;
    static_cast<void>(g.challenge.Prepare([&h] { Need(h.assertion)(0x52a); })); //실제 기존CS/플래그0 분기에서만 진단한다.
    if (g.state.current[0x40/4]) Need(h.request)(25);
    else Need(h.request)(g.state.current[0x44/4]==0xffffffffU ? 34U : 21U);
    g.state.immediate[0x40/4]=0;Need(h.selectCore)(1,1);
    const auto path=Need(h.currentFortPath)();Need(h.loadFort)(g.player.templateOwner,path);
    Need(h.loadCore)(g.player.templateOwner,0); //파일 읽기가 바꾼 현재 owner를 다시 읽는다.
    Need(h.hud)(g.hudCommand,4,0,1);g.player.playerKnown=0;
    o::ResetAmbientSoundDeadline(g.activityEnd,h.wallSeconds);Need(h.loadNewsTime)();Need(h.battleChannel)(0);Need(h.inputVisible)(1);
    return o::LocalMissionParentResult::Commit;
}
}
