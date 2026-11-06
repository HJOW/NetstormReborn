// 원문 효과 뒤 현재 Core/Player/옵션을 재조회하며 일반 초기화의 순서를 보존한다.
#include "o/RawOrdinaryBattleStart.h"
#include <array>
#include <stdexcept>
#include <string>
#include <utility>

namespace netstorm::o { // 게임/전송/UI 성공값을 합성하지 않는 일반 시작 부모다.
namespace { // 선택된 장치와 원문 물리 폭만 검증하는 지역 함수다.
constexpr std::size_t kStride=180,kPlayerCount=9; //10.78 Player 기록의 실제 폭과 개수다.
constexpr std::uint32_t kCoreLimit=0x7fbc; //004b4a20 PE의 unsigned WORD 증가 상한32700이다.
// 선택되지 않은 효과는 요구하지 않고 실제 도달한 장치만 실행한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("일반 시작의 실제 하위 장치 미연결");return hook; }
}
// 같은 원시 표/옵션 범위를 빌리며 초기화나 Core 선택은 수행하지 않는다.
RawOrdinaryBattleStart::RawOrdinaryBattleStart(OrdinaryBattleStartGlobals globals,OrdinaryBattleStartHooks hooks):globals_(globals),hooks_(std::move(hooks)) { if (globals.players.size()!=kStride*kPlayerCount || globals.options.size()!=87) throw std::invalid_argument("일반 시작의 실제 Player9/옵션87BYTE 필요"); }
// 낮은 바이트부터 원문 DWORD 하나만 읽는다.
std::uint32_t RawOrdinaryBattleStart::Read(std::uint32_t owner,std::size_t offset) const {
    if (owner>=kPlayerCount || offset>kStride-4) throw std::out_of_range("일반 시작 Player 읽기 범위");std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(globals_.players[owner*kStride+offset+byte])<<(byte*8);return value; // 원문 비정렬 네BYTE만 읽는다.
}
// 같은 지정 DWORD만 변경한다.
void RawOrdinaryBattleStart::Put(std::uint32_t owner,std::size_t offset,std::uint32_t value) const {
    if (owner>=kPlayerCount || offset>kStride-4) throw std::out_of_range("일반 시작 Player 쓰기 범위");for (std::size_t byte=0;byte<4;++byte) globals_.players[owner*kStride+offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 인접 필드는 건드리지 않는다.
}
// getter의 진단/효과 뒤 반환한 같은 주소를 해당 읽기에만 사용한다.
std::uint32_t RawOrdinaryBattleStart::CoreWord() const { const auto core=Need(hooks_.core)();if (core.size()<2) throw std::out_of_range("일반 시작 Core WORD 누락");return std::uint32_t(core[0])|(std::uint32_t(core[1])<<8); }
// PE의 반복 getter를 유지하며 WORD 읽기가 반환할 수 없는 음수 분기는 실행하지 않는다.
void RawOrdinaryBattleStart::StartPlayer() const {
    if (CoreWord()+1U<=kCoreLimit) static_cast<void>(CoreWord()+1U); // 원문 첫 상한 비교가 통과하면 두 번째 getter도 실행한다.
    const auto third=CoreWord()+1U;const auto value=third>kCoreLimit ? kCoreLimit : CoreWord()+1U;const auto core=Need(hooks_.core)();if (core.size()<2) throw std::out_of_range("일반 시작 Core 쓰기 WORD 누락");core[0]=static_cast<std::uint8_t>(value);core[1]=static_cast<std::uint8_t>(value>>8);
    Need(hooks_.writeCore)(globals_.templateOwner);Need(hooks_.touchFort)(globals_.templateOwner);Need(hooks_.status)(9,globals_.battleOwner,0);
}
// 미도착 판정은 참여 flags가 아닌 현재 subscriber와 +60 예측카운터만 사용한다.
void RawOrdinaryBattleStart::RemoveMissingPlayers() const {
    for (std::uint32_t owner=1;owner<kPlayerCount;++owner) if (Read(owner,0x2c)!=0 && Read(owner,0x60)==0) { Put(owner,0x2c,0);Put(owner,0x50,0);Need(hooks_.broadcastPlayer)(owner); } // 각 송신 뒤 다음 Player의 현재 값을 읽는다.
}
// 원문은 게임 중인 비관전자 subscriber 수+3을 로그에 보낸 뒤 현재 수/기한을 저장한다.
void RawOrdinaryBattleStart::ScheduleSpells() const {
    std::uint32_t count{};if (globals_.battle) for (std::uint32_t owner=1;owner<kPlayerCount;++owner) if (Read(owner,0x2c)!=0 && globals_.options[0x3e+owner]==0) ++count; // 현재 여덟 명의 BYTE와 ID만 센다.
    count+=3;Need(hooks_.log)(0x1d,"MAKEING "+std::to_string(count)+" Spells\n");globals_.spellCount=count;globals_.spellDeadline=Need(hooks_.gameSeconds)()+180.0;
}
// 팀 동맹은 변경을 직접 적용하지 않고 원문처럼 한 쌍당 한 일반 입력을 예약한다.
void RawOrdinaryBattleStart::QueueTeams() const {
    std::array<std::uint32_t,64> emitted{};
    for (std::uint32_t row=0;row<8;++row) for (std::uint32_t column=0;column<8;++column) { // 원문 행→열 순서이며 효과 뒤 옵션/Player를 재조회한다.
        const auto owner=row+1,target=column+1;if ((Read(owner,0x54)&(1U<<target)) || globals_.options[0x46+owner]!=globals_.options[0x46+target] || globals_.options[0x3e+owner]!=0 || globals_.options[0x3e+target]!=0 || emitted[column*8+row] || emitted[row*8+column] || !(Read(owner,0x50)&1U) || !(Read(target,0x50)&1U)) continue;
        Need(hooks_.queueAlliance)((owner<<16)|target);emitted[row*8+column]=1;emitted[column*8+row]=1;
    }
}
// server/옵션은 각 원문 선택 위치에서 다시 읽으며 앞선 효과를 실패 시 되돌리지 않는다.
void RawOrdinaryBattleStart::Run() const { StartPlayer();if (globals_.server) RemoveMissingPlayers();if (globals_.options[8]!=0) ScheduleSpells();if (globals_.server) QueueTeams(); }
}
