// 원본 일반 피해의 현재 raw 재조회와 공통 비가상 HP 저장을 그대로 사용한다.
#include "o/RawDamageableHit.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 일반 HP 필드의 위치와 피격/무피격 상태·타입 비트다.
constexpr std::size_t kHealth=26;
constexpr std::uint32_t kDais=0x400000,kHasHp=0x10,kIsland=2,kIsland3x3=0x1000000,kBridge=4;
// 원본 signed overflow를 하위 DWORD로 보존한다.
std::int32_t Subtract(std::int32_t value,std::int32_t damage) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(value)-std::bit_cast<std::uint32_t>(damage)); }
// 주변 비정렬 raw 바이트를 지정한 원본 폭으로 읽는다.
std::uint32_t ReadHit(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t result=0;
    // 요청한 원본 필드 폭만 낮은 바이트부터 조합한다.
    for (std::size_t i=0;i<width;++i) result|=static_cast<std::uint32_t>(raw[offset+i])<<(i*8);
    return result;
}
}
// 같은 판본의 현재 타입/실제 전역만 연결한다.
RawDamageableHit::RawDamageableHit(SidPool& pool,std::span<const RiftTypeRecord> types,DamageableHitState& state,DamageableHitHooks hooks)
    :pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("일반 피해 타입 판본 오류");
}
// 현재 raw 유효 범위를 검사하며 실제 free/dead 분기를 대체하지 않는다.
std::span<const std::uint8_t> RawDamageableHit::Slot(Sid sid) const {
    if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("일반 피해 SID 오류");const auto raw=pool_.Slot(sid);if (raw[10]>=types_.size()) throw std::out_of_range("일반 피해 타입 오류");return raw;
}
// 사제 setter의 절반 경계 전환 없이 일반 원본 HP 폭만 읽는다.
std::int32_t RawDamageableHit::Health(Sid victim) const { const auto raw=Slot(victim);return pool_.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(ReadHit(raw,kHealth)) : std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(ReadHit(raw,kHealth,2))); }
// dais genus는 extra bit0이며 다른 타입은 실제 TYPE_HAS_HP 비트다.
bool RawDamageableHit::HasHitPoints(Sid victim) const { const auto raw=Slot(victim);const auto& type=types_[raw[10]];return type.flags2&kDais ? (raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40U : 35U]&1)!=0 : (type.flags1&kHasHp)!=0; }
// 현재 실제 주 섬에 대한 경고만 표시하고 원본 double 시계+float 간격의 저장 순서를 유지한다.
void RawDamageableHit::WarnOwner(Sid victim) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto raw=Slot(victim);const auto owner=raw[patch ? 34U : 32U];if (owner!=state_.localOwner) return;if (owner>=state_.primaryIslands.size() || !hooks_.islandAt) throw std::logic_error("일반 피해 실제 주 섬 조회 경계 누락");
    const auto island=hooks_.islandAt(std::bit_cast<float>(ReadHit(raw,14)),std::bit_cast<float>(ReadHit(raw,18)));if (!island.value) return;const auto ground=Slot(island);const auto genus=types_[ground[10]].flags2;if (!(genus&kIsland) || (genus&kIsland3x3)) return;
    const auto islandId=genus&kBridge ? -1 : static_cast<std::int32_t>(ReadHit(ground,8,2));if (islandId!=state_.primaryIslands[owner]) return;if (!hooks_.realTime) throw std::logic_error("일반 피해 실제 경고 시계 누락");
    if (state_.warningTime<=hooks_.realTime()) { if (!hooks_.warningSound || (patch && (!hooks_.warningEvent || !hooks_.warningText))) throw std::logic_error("일반 피해 실제 섬 피격 알림 경계 누락");if (patch) hooks_.warningEvent();hooks_.warningSound();if (patch) hooks_.warningText(); }
    const auto now=hooks_.realTime();state_.warningTime=now+static_cast<double>(state_.warningDelay);
}
// 실제 서버 사망은 공통 destroy에 원본 owner-mask/유닛 공격 플래그를 전달한다.
bool RawDamageableHit::BaseHit(Sid victim,std::int32_t damage,std::uint32_t attackerOwner,Sid attacker) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto raw=Slot(victim);if ((raw[11]&2) && patch) throw std::logic_error("일반 피해 dead 원본 assert");if (raw[11]&3) return true;const auto extra=raw[patch ? 40U : 35U];if (extra&9) { if (patch && state_.checking) throw std::logic_error("일반 피해 held 원본 assert");return true; }
    if (!hooks_.rejectDamage) throw std::logic_error("일반 피해 실제 가상 방어 판정 누락");if (state_.levels.invulnerable) damage=0;if (hooks_.rejectDamage(victim,attacker)) return true;
    const auto owner=Slot(victim)[patch ? 34U : 32U];damage=RawPriestDamage::Adjust(damage,attackerOwner,owner,state_.levels,pool_.Edition());const auto next=Subtract(Health(victim),damage);
    if (next<1 && state_.server) { if (!hooks_.destroy) throw std::logic_error("일반 피해 실제 서버 삭제 경계 누락");state_.killedVictim=victim;state_.killerOwner=attackerOwner;const auto ownerMask=attackerOwner ? attackerOwner<<16 : 0U;std::uint32_t flags=ownerMask|0x100000U;if (attacker.value && (types_[Slot(attacker)[10]].flags2&0x100)) flags=ownerMask|0x4100000U;hooks_.destroy(victim,flags);return false; }
    if (!hooks_.ownerDamageNotice) throw std::logic_error("일반 피해 실제 현재 owner 통지 누락");SetRawSquidHitPoints(pool_,victim,next<1 ? 1 : next);hooks_.ownerDamageNotice(Slot(victim)[patch ? 34U : 32U],victim,attacker);return true;
}
// 실제 Damageable의 HAS_HP·주 섬·unrealize·기본 피해·burning 순서를 수행한다.
bool RawDamageableHit::Hit(Sid victim,std::int32_t damage,std::uint32_t attackerOwner,Sid attacker) const {
    if (!HasHitPoints(victim)) return false;WarnOwner(victim);const auto raw=Slot(victim);if (raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40U : 35U]&4) { if (!hooks_.unrealize) throw std::logic_error("일반 피해 실제 unrealize 경계 누락");hooks_.unrealize(victim); }const bool alive=BaseHit(victim,damage,attackerOwner,attacker);if (alive) { if (!hooks_.refresh) throw std::logic_error("일반 피해 실제 burning 몸체 누락");hooks_.refresh(victim); }return alive;
}
// 동일 월드 분배기가 실제 풀과 일치하는지 검사할 때 사용한다.
const SidPool& RawDamageableHit::Pool() const { return pool_; }
// 실제 vtable+98 주소 자료를 제공받아 원본 몸체만 분배하고 다른 파생 피해는 기존 callback이다.
GunHitHooks MakeDamageableGunHitHooks(const RawDamageableHit& damage,std::span<const DamageableHitEntry> hitEntries,GunHitHooks hooks) {
    const auto& pool=damage.Pool();if (hitEntries.empty()) throw std::invalid_argument("일반 피해 실제 가상 진입 표 오류");const bool patch=pool.Edition()==OriginalEdition::Patch1078;hooks.damage=[&damage,&pool,hitEntries,patch,fallback=std::move(hooks.damage)](Sid victim,std::int32_t amount,std::uint8_t owner,Sid attacker) {
        if (!victim.value || victim.value>=pool.Capacity()) throw std::out_of_range("일반 피해 분배 SID 오류");const auto table=ReadHit(pool.Slot(victim),0);std::uint32_t entry=0;
        // raw의 현재 vtable와 일치하는 실제 진입만 사용한다.
        for (const auto& item:hitEntries) if (item.vtable==table) { entry=item.address;break; }
        if (entry==(patch ? 0x44c8e0U : 0x462300U)) { damage.Hit(victim,amount,owner,attacker);return; }if (entry==(patch ? 0x4add10U : 0x4aeda0U)) { damage.BaseHit(victim,amount,owner,attacker);return; }if (!fallback) throw std::logic_error("다른 파생 실제 피해 미복원");fallback(victim,amount,owner,attacker);
    };return hooks;
}
// 현재 burning 판정 객체의 수명은 이 callback보다 오래 유지해야 한다.
DamageableHitHooks MakeDamageableHitBurningHooks(const SidPool& pool,const RawDamageableBurning& burning,DamageableHitHooks hooks) { if (&pool!=&burning.Pool()) throw std::invalid_argument("일반 피해/burning 실제 풀 불일치");hooks.refresh=[&burning](Sid victim) { burning.Refresh(victim); };return hooks; }
}
