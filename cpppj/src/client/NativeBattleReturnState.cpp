// 결과 지역값을 cleanup 뒤 현재 전역으로 대신하지 않고 원문 선택 효과의 순서를 유지한다.
#include "client/NativeBattleReturnState.h"
#include "client/NativeMainMenuState.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 현재 장기 Client 전역과 실제 하위 장치를 호출 동안만 빌린다.
namespace { // 도달한 선택 효과의 누락과 배경 물리 범위를 원문 효과 뒤 명시 진단한다.
template<class F> F ReturnNeed(const F& hook) { if (!hook) throw std::logic_error("일반 전투 복귀의 실제 장치 미연결");return hook; } // 재등록 중에도 이미 선택된 callable을 보유한다.
void ReturnBackground(std::span<char> bytes,std::string_view text) { if (text.size()>=bytes.size()) throw std::out_of_range("일반 복귀 배경 이름의 실제 물리 범위");std::copy(text.begin(),text.end(),bytes.begin());bytes[text.size()]=0; } // 원문 strcpy처럼 NUL 뒤 기존 바이트는 보존한다.
}
o::LocalMissionParentResult InstallNativeBattleReturnState(NativeBattleReturnGlobals g,const NativeBattleReturnHooks& h) {
    if (g.state.current[0]!=6) throw std::logic_error("일반 복귀의 현재 State6 계약");NativeBattleReturnSnapshot saved;saved.outcome=g.outcome;saved.source=ReadNativeFortEntrySource(g.state.phase[0],g.sourceType);
    if (g.background.empty()) throw std::out_of_range("일반 복귀 배경 첫BYTE 범위");g.background[0]=0;
    if (g.outcome==1) { ReturnNeed(h.deleteSavedState)();ReturnBackground(g.background,"victory");ReturnNeed(h.requestMusic)("fanfare.mus"); }
    else if (g.outcome==2) { ReturnNeed(h.deleteSavedState)();ReturnBackground(g.background,"defeat");ReturnNeed(h.requestMusic)("defeat.mus"); }
    else if (g.outcome!=0) ReturnBackground(g.background,"titleMenu"); // State6 결과0은 원문처럼 첫BYTE만0이며 기본 메뉴 배경을 넣지 않는다.
    g.outcome=0;
    if (saved.source) { ReturnNeed(h.deleteSavedState)();ReturnNeed(h.requestMusic)("fanfare.mus");if (const auto name=ReturnNeed(h.sourceBackground)(g.sourceType)) ReturnBackground(g.background,*name);else saved.source=0; }
    NotifyNativeFortEntryLeave(g.state,g.battle,h.leaveBattle);ReturnNeed(h.prefix)();return ReturnNeed(h.installFort)(saved);
}
void CompleteNativeBattleReturn(const NativeBattleReturnSnapshot& saved,std::uint32_t& outcome,const NativeBattleReturnTailHooks& h) {
    if (!saved.outcome) { ReturnNeed(h.multiplayerOptions)();return; }
    if (!saved.source) ReturnNeed(h.tell)("BattleOver"+std::to_string(std::bit_cast<std::int32_t>(saved.outcome)));outcome=0; // Tell이 바꾼 현재 결과도 정상 복귀 뒤 원문처럼0으로 덮어쓴다.
}
}
