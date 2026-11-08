// Native의 현재 재조회/짧은 반환과 두 방향 동맹 조건을 별도 UI 존재로 대체하지 않는다.
#include "o/RawBattleCompletion.h"
#include <stdexcept>
#include <utility>
#include <array> // Player의네실제SID/debugSID쌍을원문순서로비교한다.
#include <bit> //55a44c의signedBYTE비교와BYTE증가감김을구별한다.

namespace netstorm::o {
namespace { // 선택한 외부 부모만 요구하는 호스트 경계다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("일반 전투 종료의 실제 조회 미연결");return hook; } // inactive 분기는 콜백 없이 반환한다.
}
RawBattleCompletion::RawBattleCompletion(BattleCompletionGlobals globals,BattleCompletionHooks hooks):globals_(globals),hooks_(std::move(hooks)) {
    if (globals.players.size()!=9*180 || globals.options.size()!=87 || globals.alliances.size()!=81) throw std::invalid_argument("일반 전투 종료의 실제9 Player/동맹/옵션 누락");
} // 실제 정해진 물리만 받으며 이름/참여를 새 목록으로 복제하지 않는다.
std::uint32_t RawBattleCompletion::Read(std::uint32_t owner,std::size_t offset) const {
    const auto players=hooks_.currentPlayers ? std::span<const std::uint8_t>(hooks_.currentPlayers()) : globals_.players;if (owner>=9 || players.size()!=9*180 || offset>176) throw std::out_of_range("전투 종료의 현재 Player DWORD 물리 범위");
    std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(players[owner*180+offset+byte])<<(8*byte);return value; // 현재 DWORD의 정확 네BYTE만 조립한다.
}
bool RawBattleCompletion::Allies(std::uint32_t first,std::uint32_t second) const { return globals_.alliances[second*9+first]!=0 && globals_.alliances[first*9+second]!=0; } // 원문의 대표 행→현재 행 순서이며 한 방향 동맹만으로 묶지 않는다.
bool RawBattleCompletion::AllAllied() const {
    std::uint32_t subscribers{},watchingExcluded{},representative{};
    // subscriber가 있는 현재 기록만 세고 관전자는 동맹 대표/수에서 제외한다.
    for (std::uint32_t owner=1;owner<9;++owner) if (Read(owner,0x2c)) {
        ++subscribers;if (globals_.options[0x3e+owner]) continue;++watchingExcluded;
        if (!representative) representative=owner;else if (!Allies(owner,representative)) return false;
    }
    return subscribers>=2 && watchingExcluded>1;
}
bool RawBattleCompletion::TeamFinished() const {
    if (!globals_.battle || globals_.missionPost || globals_.scriptBreak || !Need(hooks_.serverConnection)()) return false;
    if (AllAllied()) return true;std::uint32_t subscribers{},representative{},enemy{};
    // 사제 판정은 watcher BYTE보다 먼저 호출하며 살아 있는 적을 마지막 별도 번호로 보존한다.
    for (std::uint32_t owner=1;owner<9;++owner) if (Read(owner,0x2c)) {
        ++subscribers;if (!Need(hooks_.priestDead)(owner) && !globals_.options[0x3e+owner]) {
            if (!representative) representative=owner;else if (!Allies(owner,representative)) enemy=owner;
        }
    }
    return subscribers>1 && (!representative || !enemy);
}
bool RawBattleCompletion::FlagFinished(std::int32_t mode) const {
    if (!globals_.battle || globals_.missionPost || globals_.scriptBreak || !Need(hooks_.serverConnection)() || globals_.loginEnd!=0.0) return false;
    bool all=true,flagged=false;std::uint32_t logged{};
    // subscriber/flags1인 기록마다 사제 효과 뒤 같은 flags BYTE를 다시 읽는다.
    for (std::uint32_t owner=1;owner<9;++owner) if (Read(owner,0x2c) && (Read(owner,0x50)&1U)) {
        ++logged;if (!Need(hooks_.priestDead)(owner)) { if (Read(owner,0x50)&0x80U) flagged=true;else all=false; }
    }
    if (globals_.boss && mode==0 && all) return true;
    if (flagged && all && logged>0) { if (globals_.boss && mode!=0) return logged==1;return logged>1; }return false;
}
bool RawBattleCompletion::Finished() const { return TeamFinished() || FlagFinished(1); } // 원문의 왼쪽 참이면 뒤 조회/효과를 생략한다.
//44ab20은현재서버객체→현재로컬사제사망→현재boss뒤에만실제subscriber행을순회한다.
bool RawBattleCompletion::UnloggedFinished(const std::uint32_t& localOwner) const {
    if (!Need(hooks_.serverConnection)()) return false;
    if (!Need(hooks_.priestDead)(localOwner) || !globals_.boss) return false;
    std::uint32_t subscribers{},alive{},offlineFlagged{},offlineTeam{},nonWatching{},offlineNonWatching{}; //원문의서로다른여섯집계를합치지않는다.
    for (std::uint32_t owner=1;owner<9;++owner) if (Read(owner,0x2c)) { //모든하위사제/팀판정효과뒤다음현재행을다시읽는다.
        ++subscribers;if (!globals_.options[62+owner]) ++nonWatching;
        if (!(Read(owner,0x50)&1U) && !globals_.options[62+owner]) ++offlineNonWatching;
        if (!Need(hooks_.priestDead)(owner)) { ++alive;const auto flags=Read(owner,0x50);if (!(flags&1U) && (flags&0x80U)) ++offlineFlagged; }
        if (TeamFinished() && !(Read(owner,0x50)&1U)) ++offlineTeam;
    }
    return subscribers>1 && (offlineFlagged==alive || nonWatching==offlineNonWatching || offlineTeam==subscribers-1U);
}
//44ac30의모든자연관문/진단/현재기한을같은원문순서로소비하고상태전환은입력큐에예약한다.
void RawBattleCompletion::Frame(BattleCompletionFrameGlobals g,const BattleCompletionFrameHooks& h) const {
    constexpr std::array<std::string_view,4> expressions{"p->firstTempleSid == p->debugFirstTempleSid","p->inProcessTempleSid == p->debugInProcessTempleSid","p->firstAltarSid == p->debugFirstAltarSid","p->inProcessAltarSid == p->debugInProcessAltarSid"}; //Dais.cpp69e..6a1의정확한네식이다.
    for (std::uint32_t owner=1;owner<9;++owner) { //메뉴/미션에서도원문진단을먼저실행하며별도World를생성하지않는다.
        for (std::size_t pair=0;pair<expressions.size();++pair) if (Read(owner,0x7c+pair*8)!=Read(owner,0x80+pair*8)) Need(h.assertion)(expressions[pair],".\\Dais.cpp",0x69eU+static_cast<std::uint32_t>(pair)); //앞진단복귀뒤다음쌍은현재필드다.
    }
    if (!globals_.battle || globals_.missionPost || !Need(hooks_.serverConnection)() || g.phase!=14) return;
    if (g.captureEnd!=0.0) { const auto wall=Need(h.wallSeconds)();if (wall>=g.captureEnd) g.captureEnd=0.0; } //현재wall효과뒤같은captureEnd를다시읽는다.
    if (std::bit_cast<std::int8_t>(g.settleFrames)<5) { g.settleFrames=static_cast<std::uint8_t>(g.settleFrames+1U);return; } //PE44ad69의JGE는signedBYTE이며다른State/Session에서임의리셋하지않는다.
    bool finished;
    if (!TeamFinished()) finished=FlagFinished(1);
    else {
        finished=true;if (g.server) { std::uint32_t logged{};for (std::uint32_t owner=1;owner<9;++owner) if (Read(owner,0x2c) && (Read(owner,0x50)&1U)) ++logged;if (logged>1) finished=false; } //팀판정참에서만서버의현재접속자수를추가확인한다.
    }
    if (globals_.boss && !finished) finished=UnloggedFinished(g.localOwner);
    if (!finished) {
        if (g.drawDeadline==0.0 || !(g.gameSeconds>=g.drawDeadline)) return; //ordered>=만진행하므로NaN기한/시각에서거부를만들지않는다.
        if (FlagFinished(0)) return;
        g.drawDeadline=0.0;if (!globals_.boss) Need(h.assertion)("boss",".\\Dais.cpp",0x70e);Need(h.rejectDraw)();return; //진단정상복귀뒤같은로컬6fh처리→방송을실행한다.
    }
    if (Need(h.currentDialog)()) return;
    const auto clearDraw=[&] { //각현재행의flag80만지우며안내/수신후다른flag를보존한다.
        for (std::uint32_t owner=1;owner<9;++owner) {
            auto players=hooks_.currentPlayers ? hooks_.currentPlayers() : g.players;if (players.size()!=9*180) throw std::out_of_range("Draw해제의현재Player9물리범위");std::uint32_t flags{};
            for (std::size_t byte=0;byte<4;++byte) flags|=std::uint32_t(players[owner*180+0x50+byte])<<(8*byte);flags&=~0x80U; //같은현재행에서DWORD를읽고그행만쓴다.
            for (std::size_t byte=0;byte<4;++byte) players[owner*180+0x50+byte]=static_cast<std::uint8_t>(flags>>(8*byte)); //실제DWORD의네BYTE만변경한다.
        }
    };
    if (TeamFinished()) clearDraw();
    Need(h.deleteSavedState)();g.settleFrames=0;
    if (!Need(hooks_.priestDead)(g.localOwner)) g.result=FlagFinished(1) ? 5U:1U;
    else { if (g.localOwner>=9) throw std::out_of_range("전투결과의현재관전자BYTE물리범위");g.result=globals_.options[62+g.localOwner] ? 6U:2U; }
    Need(h.queueMainMenu)();
}
}
