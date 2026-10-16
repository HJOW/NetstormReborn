// 실제사제피격의현재필드재조회와동일Host의감시제거순서를 유지한다.
#include "o/RawPriestDamage.h"
#include <bit>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace netstorm::o {
namespace {
// 원본HP/frame/종속체인 필드의 실제byte위치다.
constexpr std::size_t kHealth=26,kHead=6,kNext=4;
// 패치0041c5b0의Challenge rank BYTE를실제피해레벨로변환하는24칸표다.
constexpr std::array<std::int32_t,24> kPatchRankLevels{0,1,2,3,4,5,6,7,8,9,10,15,20,25,30,35,40,45,50,60,70,80,90,100};
// 원본DWORD곱셈/뺄셈의감김을signed결과로 읽는다.
std::int32_t Multiply(std::int32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(first)*std::bit_cast<std::uint32_t>(second)); }
// 원본DWORD 덧셈/뺄셈도 signed overflow 없이 같은 하위32비트를 보존한다.
std::int32_t Difference(std::int32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(first)-std::bit_cast<std::uint32_t>(second)); }
// 원본raw폭의little endian을 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t result=0;
    // 원본필드폭만조합한다.
    for (std::size_t i=0;i<width;++i) result|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return result;
}
}
// 같은월드의판본/프레임/감시저장소를 검사한다.
RawPriestDamage::RawPriestDamage(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
    PriestDamageState& state,PriestDamageHooks hooks,DependFormHost* depend)
    :pool_(pool),types_(types),frames_(frames),state_(state),hooks_(std::move(hooks)),depend_(depend) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || frames.size()!=types.size() || (depend && &depend->Pool()!=&pool)) throw std::invalid_argument("Priest 피해 타입/프레임/감시 풀 오류");
}
// 플레이어0또는보정차단은원본처럼 피해값을그대로 보존한다.
std::int32_t RawPriestDamage::Adjust(std::int32_t damage,std::uint32_t attackerOwner,std::uint32_t victimOwner,const PriestDamageState& state,OriginalEdition edition) {
    if ((state.levelCorrectionDisabledRead ? state.levelCorrectionDisabledRead() : state.levelCorrectionDisabled) || !attackerOwner || !victimOwner) return damage;
    const bool patch=edition==OriginalEdition::Patch1078;
    if (!patch && (attackerOwner>=state.levels.size() || victimOwner>=state.levels.size())) throw std::out_of_range("피해 레벨 소유자 범위 오류");
    // 원본패치getter는실제플레이어1..8 밖의owner에레벨0을반환한다.
    const auto level=[&](std::uint32_t owner) -> std::int32_t {
        if (patch && owner>8) return 0;const auto raw=static_cast<std::uint8_t>(state.levels[owner]);if (!patch) return raw;
        if (raw>=kPatchRankLevels.size()) throw std::out_of_range("피해 Challenge rank 범위 오류");return kPatchRankLevels[raw];
    };
    const auto attacker=level(attackerOwner),victim=level(victimOwner);const std::int32_t scale=patch ? 20 : 8;
    if (victim<attacker) return Multiply(Difference(Difference(attacker,victim),-scale),damage)/scale;
    if (attacker<victim) { const auto divisor=Difference(Difference(victim,attacker),-scale);if (!divisor) throw std::out_of_range("피해 레벨 나눗셈 범위 오류");const auto product=Multiply(damage,scale);if (product==std::numeric_limits<std::int32_t>::min() && divisor==-1) throw std::out_of_range("피해 레벨 나눗셈 overflow");return product/divisor; }
    return damage;
}
// 현재HP쓰기뒤의원본current-field재조회에사용한다.
std::int32_t RawPriestDamage::Health(Sid victim) const {
    const auto raw=pool_.Slot(victim);return pool_.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(raw,kHealth)) : std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Read(raw,kHealth,2)));
}
// 같은의존Host가통지에서현재쌍을없애도선저장한next로계속한다.
void RawPriestDamage::DropDependents(Sid victim) const {
    Sid current{static_cast<std::uint16_t>(Read(pool_.Slot(victim),kHead,2))};std::unordered_set<std::uint16_t> visited;
    // 원본FormFinder처럼현재후보를반환하기전에다음SID를 저장한다.
    while (current.value) {
        if (current.value>=pool_.Capacity() || !visited.insert(current.value).second) throw std::logic_error("Priest 피해 의존 체인 오류");const auto raw=pool_.Slot(current);const Sid next{static_cast<std::uint16_t>(Read(raw,kNext,2))};
        if (raw[10]==1) {
            if (!depend_) throw std::logic_error("Priest HP1 실제 감시 통지 Host 누락");depend_->Notify(current,victim);
            if (!(pool_.Slot(current)[11]&1)) { const auto watcher=depend_->Watcher(current);depend_->Remove(watcher,victim,0); }
        }
        current=next;
    }
}
// 원본HP는1을최솟값으로하며피격직후현재소유자/HP/감시를다시 읽는다.
bool RawPriestDamage::Hit(Sid victim,std::int32_t damage,std::uint32_t attackerOwner,Sid attacker) const {
    if (!victim.value || victim.value>=pool_.Capacity()) throw std::out_of_range("Priest 피해 SID 오류");const auto raw=pool_.Slot(victim);if (raw[11]&2) { if (pool_.Edition()==OriginalEdition::Patch1078) throw std::logic_error("Priest 피해 dead 원본 assert");return true; }
    if (raw[11]&1 || raw[10]>=types_.size()) throw std::logic_error("Priest 피해 live/타입 계약 오류");const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const std::size_t owner=patch ? 34U : 32U;
    if (!hooks_.hasRegenEvent || !hooks_.setHitPoints || !hooks_.ownerDamageNotice || !hooks_.damageableRefresh) throw std::logic_error("Priest 실제 피격 하위 몸체 누락");
    if (state_.invulnerable) damage=0;
    bool cancel=hooks_.hasRegenEvent(victim);
    if (!cancel) { const auto frame=static_cast<std::int32_t>(Read(raw,patch ? 36U : 34U,patch ? 4U : 1U));const auto direction=RawPathAnimation::Direction(frames_[raw[10]],frame);cancel=direction>9 && direction<15; }
    if (cancel) {
        if (!hooks_.cancelEvent || !hooks_.defaultFrame || !hooks_.setFrame) throw std::logic_error("Priest 실제 회복 취소/프레임 경계 누락");hooks_.cancelEvent(victim,0x25c);hooks_.cancelEvent(victim,0x25d);hooks_.setFrame(victim,hooks_.defaultFrame(victim),0);
    }
    damage=Adjust(damage,attackerOwner,pool_.Slot(victim)[owner],state_,pool_.Edition());const auto next=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(Health(victim))-std::bit_cast<std::uint32_t>(damage));hooks_.setHitPoints(victim,next<1 ? 1 : next);
    hooks_.ownerDamageNotice(pool_.Slot(victim)[owner],victim,attacker);hooks_.damageableRefresh(victim);
    if (state_.boss && Health(victim)==1) { DropDependents(victim);return false; }return true;
}
// 같은raw월드에대한피격분배만 허용한다.
const SidPool& RawPriestDamage::Pool() const { return pool_; }
// actual사제body만분배하고다른미복원표적을empty성공으로 바꾸지 않는다.
GunHitHooks MakePriestGunHitHooks(const SidPool& pool,const RawPriestDamage& priest,GunHitHooks hooks,std::function<void(Sid,std::int32_t,std::uint8_t,Sid)> fallback) {
    if (&pool!=&priest.Pool()) throw std::invalid_argument("Gun/Priest 피해 SID 풀 불일치");hooks.damage=[&pool,&priest,fallback=std::move(fallback)](Sid victim,std::int32_t damage,std::uint8_t owner,Sid attacker) {
        if (pool.Slot(victim)[10]==kPriestType) { priest.Hit(victim,damage,owner,attacker);return; }if (!fallback) throw std::logic_error("비사제 실제 피해 분배 미복원");fallback(victim,damage,owner,attacker);
    };return hooks;
}
// 회복프로세스삭제와HP경계전환도같은실제원본저장소를공유한다.
PriestDamageHooks MakePriestDamageWorldHooks(const SidPool& pool,SquidProcessHost& host,const RawPriestState& hp,
    const PriestHitPointMode& mode,PriestHitPointHooks space,const RawDamageableBurning& burning,PriestDamageHooks hooks) {
    if (&host.Pool()!=&pool || &hp.Pool()!=&pool || &burning.Pool()!=&pool) throw std::invalid_argument("Priest 피해 실제월드 풀 불일치");
    hooks.hasRegenEvent=[&host](Sid victim) { return host.FindEvent(victim,0x25d)!=nullptr; };
    hooks.cancelEvent=[&host](Sid victim,std::uint32_t event) {
        // 원본cancel helper처럼첫현재일치Regular를반복조회하며실제Form/Kernel을삭제한다.
        while (auto* process=host.FindEvent(victim,event)) host.Kill(*process,0);
    };
    hooks.setHitPoints=[&hp,&mode,space=std::move(space)](Sid victim,std::int32_t value) { hp.SetHitPoints(victim,value,mode,space); };
    hooks.damageableRefresh=[&burning](Sid victim) { burning.Refresh(victim); };return hooks;
}
}
