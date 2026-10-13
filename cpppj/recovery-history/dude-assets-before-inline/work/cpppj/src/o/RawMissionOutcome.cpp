// 원본 사건 조건·현재 상태 재조회·최종 사건 잠금의 순서를 보존한다.
#include "o/RawMissionOutcome.h"
#include <bit>
#include <stdexcept>
#include <string>
#include <utility>

namespace netstorm::o {
namespace {
// Player 활성 mask와 raw Squid의 사망/HP/좌표 위치다. void/free는 이 조건 검사에서 사용하지 않는다.
constexpr std::uint8_t kActive=0x22,kDead=2;
constexpr std::size_t kRawState=11,kHp=26,kX=14,kY=18;
// 정렬되지 않은 raw 값을 지정 폭만큼 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // CD HP는 2바이트이므로 그 뒤 필드를 함께 읽지 않는다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 판본별 현재 HP를 부호 확장한다. 타입의 최대 HP와 다른 값이다.
std::int32_t Hp(std::span<const std::uint8_t> raw,OriginalEdition edition) {
    return edition==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(raw,kHp)) :
        static_cast<std::int32_t>(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Read(raw,kHp,2))));
}
}
// 사제 목록과 상태를 현재 월드에 그대로 연결한다. 필요한 실제 효과가 빠졌으면 동작 전에 거부한다.
RawMissionOutcome::RawMissionOutcome(const SidPool& pool,const PriestPostPopState& priests,MissionOutcomeState& state,MissionOutcomeHooks hooks)
    :pool_(pool),priests_(priests),state_(state),hooks_(std::move(hooks)),lookup_(pool,priests) {
    if (!hooks_.maxHitPoints || !hooks_.regionOwner || !hooks_.section || !hooks_.closeTransientGumps || !hooks_.onSection)
        throw std::invalid_argument("미션 승패의 필수 효과 연결 누락");
}
// 패치 원본 assert는 결과 조건과 별개인 진단 효과다. CD는 범위 밖에서 바로 기본 결과를 사용한다.
bool RawMissionOutcome::ValidPlayer(std::uint32_t player) const {
    const bool valid=player>=1 && player<=kPlayerCount;
    if (!valid && pool_.Edition()==OriginalEdition::Patch1078 && hooks_.invalidPlayerAssert) hooks_.invalidPlayerAssert(player);
    return valid;
}
// 활성 플래그/ownerState를 매 조회마다 읽는다. captured 상태1도 생존으로 본다.
bool RawMissionOutcome::PriestDead(std::uint32_t player) const {
    return !ValidPlayer(player) || !(state_.players[player].flags&kActive) || priests_.ownerState[player]==2;
}
// 범위 밖 참이라는 원본 특수 결과와 inactive 거짓을 구분한다.
bool RawMissionOutcome::PriestCaptured(std::uint32_t player) const {
    if (!ValidPlayer(player)) return true;
    return (state_.players[player].flags&kActive) && priests_.ownerState[player]==1;
}
// bool 조건 외의 원본 DWORD 잠금값은 비제로 전체를 처리 완료로 본다. false 조건은 잠금을 지우지 않는다.
bool RawMissionOutcome::Event(std::uint32_t& latch,bool condition,std::string_view name) const {
    if (!condition || latch) return false;
    const std::string section=(state_.demo ? "Demo" : "")+std::string(name);
    if (state_.tellBlocked) return false;
    if (hooks_.section(section)) { hooks_.closeTransientGumps();hooks_.onSection(); }
    latch=1;return true;
}
// 원본 DWORD 번호를 먼저 줄이지 않아 손상된 높은 번호가 다른 슬롯을 참조하지 않는다.
std::span<const std::uint8_t> RawMissionOutcome::Raw(std::uint32_t sid) const {
    if (sid>=pool_.Capacity()) throw std::out_of_range("미션 승패 raw SID 범위 오류");
    return pool_.Slot(Sid{static_cast<std::uint16_t>(sid)});
}
// 원본 정수 나눗셈은 0 방향 절삭이며 정확히 절반 HP일 때 halfDead가 아니다.
bool RawMissionOutcome::TempleHalfDead(std::uint32_t player) const {
    const auto sid=state_.players[player].temple;if (!sid) return true;
    const auto raw=Raw(sid);
    if (pool_.Edition()==OriginalEdition::Patch1078) {
        const auto hp=Hp(raw,pool_.Edition());return hp<hooks_.maxHitPoints(Sid{static_cast<std::uint16_t>(sid)})/2;
    }
    const auto maximum=hooks_.maxHitPoints(Sid{static_cast<std::uint16_t>(sid)});
    return Hp(raw,pool_.Edition())<maximum/2;
}
// 앞선 halfDead 사건 콜백이 바꾼 현재 신전 SID와 상태를 새로 읽는다.
bool RawMissionOutcome::TempleDead(std::uint32_t player) const {
    const auto sid=state_.players[player].temple;return !sid || (Raw(sid)[kRawState]&kDead);
}
// 영역 조회의 callback이 localPlayer를 바꾸어도 원본에서 미리 읽은 비교값은 유지한다.
bool RawMissionOutcome::PriestSaved(std::uint32_t player) const {
    const auto sid=lookup_.Find(player);if (!sid) return false;
    const auto local=state_.localPlayer;const auto raw=Raw(sid);
    const auto x=std::bit_cast<float>(Read(raw,kX)),y=std::bit_cast<float>(Read(raw,kY));
    return hooks_.regionOwner(x,y)==local;
}
// 상태가 재진입 중 바뀌어도 현재 플레이어의 진입 시 활성 조건을 다시 검사하지 않는다.
void RawMissionOutcome::Check() const {
    std::uint32_t good=0,bad=0;
    // 원본 번호 순서이며 사건마다 현재 입력을 다시 조회한다. 두 팀 집계는 그 플레이어의 saved 사건 뒤다.
    for (std::uint32_t player=1;player<=kPlayerCount;++player) {
        if (!(state_.players[player].flags&kActive)) continue;
        auto& latch=state_.latches[player];const std::string prefix="ai"+std::to_string(player);
        Event(latch.templeHalfDead,TempleHalfDead(player),prefix+"TempleHalfDead");
        Event(latch.templeDead,TempleDead(player),prefix+"TempleDead");
        Event(latch.priestDead,PriestDead(player),prefix+"PriestDead");
        Event(latch.priestCaptured,PriestCaptured(player),prefix+"PriestCaptured");
        Event(latch.priestSaved,PriestSaved(player),prefix+"PriestSaved");
        if (!PriestDead(player)) {
            const auto local=state_.localPlayer;
            if (local>kPlayerCount) throw std::out_of_range("미션 승패 동맹의 로컬 번호 범위 오류");
            if (state_.allies[local][player] && state_.allies[player][local]) ++good;else ++bad;
        }
    }
    Event(state_.goodTeamDead,good==0,"GoodTeamDead");
    Event(state_.badTeamDead,bad==0,"BadTeamDead");
    Event(state_.failed,PriestDead(state_.localPlayer),"Failed");
}
// 실제 객체 조회/상태 연결이 동일한 월드를 사용하는지 확인한다.
const SidPool& RawMissionOutcome::Pool() const { return pool_; }
// 기존 최대 HP의 사제 특수 모드/알타 조건도 재구현 없이 실제 몸체를 사용한다.
MissionOutcomeHooks MakeMissionOutcomeHooks(const SidPool& pool,const SquidReward& reward,MissionOutcomeHooks hooks) {
    if (&reward.Pool()!=&pool) throw std::invalid_argument("미션 승패와 최대 HP의 SID 풀이 다릅니다");
    hooks.maxHitPoints=[&reward](Sid sid) { return reward.MaxHitPoints(sid); };return hooks;
}
}
