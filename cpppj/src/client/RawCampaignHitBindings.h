// 실제 캠페인 충돌/사제 피격/주변 Dude 반응을 같은 raw 저장소에 조립한다.
#pragma once
#include "client/RawSurfaceWorld.h"
#include "client/RawPriestWorldBindings.h"
#include "o/RawPriestDamage.h"
#include "o/RawDudeNotice.h"
#include "o/RawGunImpact.h"
#include "o/RawDamageableHit.h"
#include "o/RawSpecialDamage.h"
#include <memory>
namespace netstorm::client {
// 기존 실제 몸체가 아직 다른 소비자에 맡기는 선택 경계다. 미연결 효과는 도달 시 거부한다.
struct RawCampaignHitEffects {
    o::PriestDamageHooks priest; // 현재 defaultFrame/소유자 피해 통지의 실제 외부 경계다.
    o::DamageableBurningHooks burning; // nonzero damageEffect의 실제 생성 및 소리 소비자다.
    o::DudeNoticeHooks dudes; // 실제 Burning0/기존 경로 endpoint/대피 경로 생성 경계다.
    o::DamageableHitHooks general; // 실제 가상 방어·현재 소유자 피해 통지·섬 경고 UI/음향 경계다.
    o::VortexDamageHooks vortex; // 같은 실제 건설30/끝 시각/표시의 신전 피해 경계다.
};
// 기존 피격/충돌 계산을 재구현하지 않고 같은 World/Host/HP/난수/감시에 연결한다.
class RawCampaignHitBindings {
public:
    // TYPE/목록/속도/현재 상태와 월드/사제/HP는 이 연결 객체보다 오래 살아야 한다.
    RawCampaignHitBindings(RawSurfaceWorld& world,const RawPriestWorldBindings& priests,const o::SquidReward& hp,
        o::GameRandom& random,std::span<o::DudeNoticeTypeData> speeds,const o::SquidPostPopList& residences,
        o::PriestDamageState& priestState,o::DamageableHitState& generalState,o::DamageableBurningState& burningState,o::GunImpactState& impactState,
        RawCampaignHitEffects effects);
    // callback 사용을 끝내고 Kernel을 비운 뒤 해제해야 한다.
    ~RawCampaignHitBindings();
    RawCampaignHitBindings(const RawCampaignHitBindings&)=delete; // 실제 callback 주소를 보존한다.
    RawCampaignHitBindings& operator=(const RawCampaignHitBindings&)=delete; // 다른 월드의 저장소와 혼용하지 않는다.
    // 실제 사제·공통 일반 자산 피격을 분배하고 다른 파생 표적의 기존 경계를 유지한다.
    o::GunHitHooks DamageHooks(o::GunHitHooks hooks) const;
    // 실제 충돌 효과와 총의 현재 시각 피해 계산을 같은 Missile에 연결한다.
    o::MissileHooks MissileHooks(const o::RawGunHit& hit,const o::MissileState& clock,o::MissileHooks hooks) const;
    // 실제 anim160 예약 종료/프레임 처리만 추가한다.
    o::RegularHandler Handler(o::RegularHandler fallback={}) const;
    // 실제 썬 디스크/anim의 공통 Pre/Post 경로만 추가하며 나머지 파생 삭제는 보존한다.
    o::SquidDestroyHooks DestroyHooks(o::SquidDestroyHooks fallback) const;
    // 실제 가상 삭제 몸체를 같은 Form 분배기의 최상위 자산 경계에 연결한다.
    o::SquidAssetDestroy Destroy(o::SquidAssetDestroy fallback={}) const;
    // 후속 caller도 같은 실제 몸체/풀을 사용한다.
    const o::RawPriestDamage& PriestDamage() const;
    const o::RawGunImpact& Impact() const;
    const o::RawDudeNotice& Dudes() const;
    const o::RawDamageableBurning& Burning() const; // 회복 caller도 같은 현재 효과 판정을 재사용한다.
    const o::RawDamageableHit& GeneralDamage() const; // 같은 실제 일반 HP/죽음/섬 경고 몸체를 빌린다.
    const o::SidPool& Pool() const;
private:
    struct Impl; // 모든 내부 callback 대상 주소를 고정하는 실제 모듈 소유자다.
    std::unique_ptr<Impl> impl_; // 외부 상태의 수명과 Kernel 해제 순서를 상위가 보장한다.
};
}
