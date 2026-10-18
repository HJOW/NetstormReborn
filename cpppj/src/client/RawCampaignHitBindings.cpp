// 실제 Host/HP/감시/공간 효과를 연결하며 필요한 외부 생성 경계는 원래 요청 시점에 남긴다.
#include "client/RawCampaignHitBindings.h"
#include "o/RawContainedLookup.h"
#include "o/RawPathSearch.h"
#include "o/RawSquidDefense.h"
#include <stdexcept>
namespace netstorm::client {
namespace {
#include "o/CampaignDamageVtables.inc"
// 현재 raw 가상 표의 little endian DWORD만 읽는다.
std::uint32_t HitTable(std::span<const std::uint8_t> raw) {
    std::uint32_t value=0;
    // 실제 네 바이트를 정렬 가정 없이 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[i])<<(8*i);
    return value;
}
}
// 복원된 몸체와 단일 Core의 참조를 소유하며 별도 HP/표적 사본을 만들지 않는다.
struct RawCampaignHitBindings::Impl {
    RawSurfaceWorld& world; // 실제 효과 생성/공간/삭제를 수행하는 현재 월드다.
    RawWorldCore core; // 현재 Pool/Host/감시/장부/표시의 참조만 보관한다.
    const RawPriestWorldBindings& priests; // 기존 실제 HP/공간 경계 전환 처리기다.
    o::RawContainedLookup contained; // 같은 Form 체인의 현재 typed 프로세스 조회다.
    std::unique_ptr<o::RawDamageableBurning> burning; // 현재 HP/효과 분기의 실제 몸체다.
    std::unique_ptr<o::RawPriestDamage> priestDamage; // 실제 회복 취소/HP1/감시 해제를 수행한다.
    std::unique_ptr<o::RawDamageableHit> generalDamage; // 현재 일반 HP·방어·경고·서버 죽음의 실제 몸체다.
    o::VortexDamageHooks vortex; // 같은 현재30 Form과 일반 피해를 사용하는 신전 분배 경계다.
    std::unique_ptr<o::RawDudeNotice> dudes; // 실제 finder/현재task/거주 구역 대피 요청 처리기다.
    std::unique_ptr<o::RawGunImpact> impact; // 같은 Factory/Host의 실제 anim 생성/예약/삭제다.
    // 모든 callback은 동일 Core와 외부 장기 상태만 읽는다.
    Impl(RawSurfaceWorld& input,const RawPriestWorldBindings& priestWorld,const o::SquidReward& hp,
        o::GameRandom& random,std::span<o::DudeNoticeTypeData> speeds,const o::SquidPostPopList& residences,
        o::PriestDamageState& priestState,o::DamageableHitState& generalState,o::DamageableBurningState& burningState,o::GunImpactState& impactState,
        RawCampaignHitEffects effects)
        :world(input),core(input.Core()),priests(priestWorld),contained(core.pool,[this](o::Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); }) {
        if (&priests.PriestState().Pool()!=&core.pool || &hp.Pool()!=&core.pool) throw std::invalid_argument("캠페인 피격 현재 Pool 불일치");
        auto burnHooks=std::move(effects.burning);
        burnHooks.find=[this](o::Sid victim,std::uint32_t type,std::uint32_t index) { return contained.Find(victim,type,index,0); };
        burnHooks.destroy=[this](o::Sid form,std::uint32_t flags) { world.Destroy(form,flags); };
        burning=std::make_unique<o::RawDamageableBurning>(core.pool,core.types,hp,burningState,std::move(burnHooks));
        auto damage=std::move(effects.priest);
        damage.hasRegenEvent=[this](o::Sid victim) { return core.processes.FindEvent(victim,0x25d)!=nullptr; };
        damage.cancelEvent=[this](o::Sid victim,std::uint32_t event) {
            // 원본 취소 helper처럼 현재 첫 일치 프로세스를 반복 찾아 실제 Host에서 삭제한다.
            while (auto* process=core.processes.FindEvent(victim,event)) core.processes.Kill(*process,0);
        };
        damage.setFrame=[this](o::Sid victim,std::int32_t frame,std::uint32_t flags) { core.frame.Set(victim,frame,flags); };
        damage.setHitPoints=[this](o::Sid victim,std::int32_t value) { priests.SetHitPoints(victim,value); };
        damage.damageableRefresh=[this](o::Sid victim) { burning->Refresh(victim); };
        priestDamage=std::make_unique<o::RawPriestDamage>(core.pool,core.types,core.frames,priestState,std::move(damage),&core.dependencies);
        auto general=std::move(effects.general); // 현재 일반 피해만 원본 가상 분배에 연결한다.
        general.destroy=[this](o::Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        general.unrealize=[this](o::Sid sid) { core.display.Update(core.pool.Slot(sid),0); };
        general.islandAt=[this](float x,float y) { return o::RawPathSurfaceAt(core.hash,{x,y}); };
        // 실제48ef70 가상 표인 윈드 타워만 방향 방어로 연결하고 다른 파생 경계는 보존한다.
        general.rejectDamage=[this,fallback=std::move(general.rejectDamage)](o::Sid victim,o::Sid attacker) {
            if (core.pool.Edition()==o::OriginalEdition::Patch1078 && HitTable(core.pool.Slot(victim))==0x50ead0U)
                return o::RawWindTowerDefense(core.pool,core.types,core.frames,victim,attacker);
            if (!fallback) throw std::logic_error("캠페인 다른 파생 실제 방어 미연결");return fallback(victim,attacker);
        };
        general=o::MakeRawSquidDefenseHooks(core.pool,kCampaignDamageDefenseEntries,std::move(general));
        general=o::MakeDamageableHitBurningHooks(core.pool,*burning,std::move(general));
        generalDamage=std::make_unique<o::RawDamageableHit>(core.pool,core.types,generalState,std::move(general));
        vortex=std::move(effects.vortex);
        vortex.find=[this](o::Sid victim,std::uint32_t type,std::uint32_t index,bool remove) { return contained.Find(victim,type,index,remove); };
        vortex.general=[this](o::Sid victim,std::int32_t amount,std::uint32_t owner,o::Sid attacker) { return generalDamage->Hit(victim,amount,owner,attacker); };
        auto notice=std::move(effects.dudes);
        notice.setFrame=[this](o::Sid sid,std::int32_t frame,std::uint32_t flags) { core.frame.Set(sid,frame,flags); };
        notice.notifySurface=[this](o::Sid sid) { core.building.NotifySurface(sid); };
        dudes=std::make_unique<o::RawDudeNotice>(core.pool,core.hash,core.types,core.frames,speeds,residences,
            o::DudeNoticeState{core.processState},o::MakeDudeNoticeHostHooks(core.pool,core.processes,std::move(notice)));
        o::GunImpactHooks visual;
        visual.pop=[this](o::Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        visual.destroy=[this](o::Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        visual.notifyDudes=[this](float x,float y) { dudes->Around(x,y); };
        impact=std::make_unique<o::RawGunImpact>(core.pool,core.frames,core.frame,random,impactState,
            o::MakeGunImpactRuntimeHooks(core.pool,core.factory,core.kernel,core.processes,std::move(visual)));
    }
    // 실제 가상 표가 공통 Pre/Post를 쓰는 두 자산만 현재 수명 분배에 포함한다.
    bool CommonAsset(o::Sid sid) const {
        const auto raw=core.pool.Slot(sid);const bool patch=core.pool.Edition()==o::OriginalEdition::Patch1078;
        // PATCH132는83과 같은0046a6c0 생성자/0050b570 가상 표를 사용하므로 같은 실제 공통 삭제를 따른다.
        // PATCH80/81은 기본00501dd0이며80에 부착된 실제 Fiery Tail도 같은 종속 삭제가 정리한다.
        return ((raw[10]==83 || (patch && raw[10]==132)) && HitTable(raw)==(patch ? 0x50b570U : 0x501440U)) ||
            (patch && (raw[10]==80 || raw[10]==81) && HitTable(raw)==0x501dd0U) || impact->Handles(sid);
    }
};
// 초기 객체 생성 전에 실제 하위 모듈의 동일 Pool 계약을 확인한다.
RawCampaignHitBindings::RawCampaignHitBindings(RawSurfaceWorld& world,const RawPriestWorldBindings& priests,const o::SquidReward& hp,
    o::GameRandom& random,std::span<o::DudeNoticeTypeData> speeds,const o::SquidPostPopList& residences,
    o::PriestDamageState& priestState,o::DamageableHitState& generalState,o::DamageableBurningState& burningState,o::GunImpactState& impactState,RawCampaignHitEffects effects)
    :impl_(std::make_unique<Impl>(world,priests,hp,random,speeds,residences,priestState,generalState,burningState,impactState,std::move(effects))) {}
// 상위가 같은 Kernel을 비운 뒤 callback 대상 모듈을 해제한다.
RawCampaignHitBindings::~RawCampaignHitBindings()=default;
// 현재 raw VT의 실제 피격 진입을 대조하고 사제·일반·특수 파생을 구별한다.
o::GunHitHooks RawCampaignHitBindings::DamageHooks(o::GunHitHooks hooks) const {
    hooks=o::MakeDamageableGunHitHooks(*impl_->generalDamage,kCampaignDamageHitEntries,std::move(hooks));
    // 원본 대응표에서 가상98이4dba40인 신전만 실제 특수 분배로 보낸다.
    hooks.damage=[this,fallback=std::move(hooks.damage)](o::Sid victim,std::int32_t amount,std::uint8_t owner,o::Sid attacker) {
        const auto table=HitTable(impl_->core.pool.Slot(victim)); // 호출 시점의 현재 가상 표다.
        // 타입 이름 대신 검증된 실제 가상 진입 주소를 대조한다.
        for (const auto& entry:kCampaignDamageHitEntries) if (entry.vtable==table && entry.address==0x4dba40U) {
            static_cast<void>(o::RawVortexDamage(impl_->core.pool,victim,amount,owner,attacker,impl_->vortex));return;
        }
        if (!fallback) throw std::logic_error("캠페인 다른 실제 피해 미연결");fallback(victim,amount,owner,attacker);
    };
    auto fallback=std::move(hooks.damage);return o::MakePriestGunHitHooks(impl_->core.pool,*impl_->priestDamage,std::move(hooks),std::move(fallback));
}
// 실제 충돌 생성/주변 통지/피해 시간 압축을 기존 Missile 몸체에 연결한다.
o::MissileHooks RawCampaignHitBindings::MissileHooks(const o::RawGunHit& hit,const o::MissileState& clock,o::MissileHooks hooks) const {
    if (&hit.Pool()!=&impl_->core.pool) throw std::invalid_argument("캠페인 Missile/GunHit Pool 불일치");
    return o::MakeMissileGunHitHooks(hit,clock,o::MakeMissileGunImpactHooks(*impl_->impact,std::move(hooks)));
}
// 같은 Kernel 예약에 실제 anim Handle만 추가한다.
o::RegularHandler RawCampaignHitBindings::Handler(o::RegularHandler fallback) const { return impl_->impact->Handler(std::move(fallback)); }
// 원본 가상14/18이 실제 공통몸체인 자산만 기존 수명 처리기를 사용한다.
o::SquidDestroyHooks RawCampaignHitBindings::DestroyHooks(o::SquidDestroyHooks fallback) const {
    auto hooks=fallback;hooks.emit=[this,common=impl_->core.surfaces.Hooks(),fallback=std::move(fallback)](const o::SquidDestroyEvent& event) {
        if (impl_->CommonAsset(event.sid) && (event.effect==o::SquidDestroyEffect::PreDestroy || event.effect==o::SquidDestroyEffect::PostDestroy)) { common.emit(event);return; }
        if (!fallback.emit) throw std::logic_error("캠페인 다른 실제 자산 삭제 효과 미연결");fallback.emit(event);
    };return hooks;
}
// Form 분배기가 준 공통 훅으로 실제 가상 삭제를 수행한다. 다른 자산은 별도 파생 처리기가 필요하다.
o::SquidAssetDestroy RawCampaignHitBindings::Destroy(o::SquidAssetDestroy fallback) const {
    return [this,fallback=std::move(fallback)](o::Sid sid,std::uint32_t flags,const o::SquidDestroyHooks& hooks) {
        if (impl_->CommonAsset(sid)) return impl_->core.virtualDestroy.Destroy(sid,flags,hooks);
        if (!fallback) throw std::logic_error("캠페인 다른 실제 자산 가상 삭제 미연결 TYPE"+std::to_string(impl_->core.pool.Slot(sid)[10])+" SID"+std::to_string(sid.value));return fallback(sid,flags,hooks);
    };
}
// 후속 실제 소비자는 같은 모듈/저장소를 재사용한다.
const o::RawPriestDamage& RawCampaignHitBindings::PriestDamage() const { return *impl_->priestDamage; }
const o::RawGunImpact& RawCampaignHitBindings::Impact() const { return *impl_->impact; }
const o::RawDudeNotice& RawCampaignHitBindings::Dudes() const { return *impl_->dudes; }
const o::RawDamageableBurning& RawCampaignHitBindings::Burning() const { return *impl_->burning; }
const o::RawDamageableHit& RawCampaignHitBindings::GeneralDamage() const { return *impl_->generalDamage; }
const o::SidPool& RawCampaignHitBindings::Pool() const { return impl_->core.pool; }
}
