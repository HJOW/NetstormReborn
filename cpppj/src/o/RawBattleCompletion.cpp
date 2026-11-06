// Native의 현재 재조회/짧은 반환과 두 방향 동맹 조건을 별도 UI 존재로 대체하지 않는다.
#include "o/RawBattleCompletion.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace { // 선택한 외부 부모만 요구하는 호스트 경계다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("일반 전투 종료의 실제 조회 미연결");return hook; } // inactive 분기는 콜백 없이 반환한다.
}
RawBattleCompletion::RawBattleCompletion(BattleCompletionGlobals globals,BattleCompletionHooks hooks):globals_(globals),hooks_(std::move(hooks)) {
    if (globals.players.size()!=9*180 || globals.options.size()!=87 || globals.alliances.size()!=81) throw std::invalid_argument("일반 전투 종료의 실제9 Player/동맹/옵션 누락");
} // 실제 정해진 물리만 받으며 이름/참여를 새 목록으로 복제하지 않는다.
std::uint32_t RawBattleCompletion::Read(std::uint32_t owner,std::size_t offset) const {
    std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(globals_.players[owner*180+offset+byte])<<(8*byte);return value; // 현재 DWORD의 정확 네BYTE만 조립한다.
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
        ++logged;if (!Need(hooks_.priestDead)(owner)) { if (globals_.players[owner*180+0x50]&0x80U) flagged=true;else all=false; }
    }
    if (globals_.boss && mode==0 && all) return true;
    if (flagged && all && logged>0) { if (globals_.boss && mode!=0) return logged==1;return logged>1; }return false;
}
bool RawBattleCompletion::Finished() const { return TeamFinished() || FlagFinished(1); } // 원문의 왼쪽 참이면 뒤 조회/효과를 생략한다.
}
