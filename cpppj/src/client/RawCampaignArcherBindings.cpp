// 실제 모듈의 동일 풀·공간·시간 계약을 연결하고 미공급 효과는 선택 시 거부한다.
#include "client/RawCampaignArcherBindings.h"
#include "o/RawDiscRegular.h"
#include "o/RawCrossbowAnimation.h"
#include "o/RawCannonFire.h"
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
// 현재 Core와 기존 실제 Archer/Missile 몸체의 참조만 조립한다.
struct RawCampaignArcherBindings::Impl {
    RawSurfaceWorld& world; // 모든 자산 공간과 삭제가 사용하는 같은 월드다.
    RawWorldCore core; // 원본 저장소의 참조 뷰이며 별도 풀/해시를 만들지 않는다.
    const o::RawGunTarget& target; // 동일 현재 자산의 실제 중심 조회다.
    o::GameRandom& random; // 다른 프로세스와 공유하는 같은 난수다.
    o::ArcherAnimationState& animationState; // 현재 원본 게임 시각/경과다.
    o::MissileState& missileState; // 투사체가 읽는 같은 게임 시각이다.
    std::unique_ptr<o::RawMissile> missile; // 같은 자산 생성/Host에 연결한 기존 실제 몸체다.
    std::unique_ptr<o::RawArcherAnimation> animation; // 같은 타입 방향수와 Fire 각상태 몸체다.
    o::GunAnimationState cannonState; // 같은 Kernel 시각을 읽는 실제 썬 캐논 전용 전역이다.
    std::unique_ptr<o::RawGunAnimation> cannon; // 기존 검증된 썬 캐논 회전/원위치의 같은 월드 본체다.
    o::GunAnimationState thunderRotateState; // TYPE131의 실제 공통 회전 시계와 부모 TYPE다.
    std::unique_ptr<o::RawGunAnimation> thunderRotate; // group1이 공유하는 실제00467130 회전만 소비한다.
    o::CannonFireState sunFireState,thunderFireState; // 실제 공통 발사의 두 파생 현재 시계/getter다.
    std::unique_ptr<o::RawCannonFire> sunFire,thunderFire; // 초기화 이후 공통 Fire와 별도 가상 단계 본체다.
    o::ArcherAnimationState crossbowRotateState; // TYPE140의 실제 공유 회전이 읽는 같은 현재 시계다.
    o::CrossbowAnimationState crossbowState; // 실제 크로스보우 발사 getter와 현재 시계다.
    std::unique_ptr<o::RawArcherAnimation> crossbowRotate; //00467940의 실제 공통 회전만 재사용한다.
    std::unique_ptr<o::RawCrossbowAnimation> crossbow; // 별도004674c0의 크로스보우 발사 본체다.
    std::unique_ptr<o::RawDiscRegular> disc; // 실제 투사체 예약의 방향별 modulo 처리기다.
    // 모든 실제 공간/Host를 구성한 뒤 같은 원본 모듈에 공급한다.
    Impl(RawSurfaceWorld& input,const o::RawGunTarget& finder,o::GameRandom& rng,
        std::span<const o::ArcherTypeData> animationTypes,std::span<const o::PriestPlainCanonType> frameTypes,std::span<const o::MissileTypeData> missileTypes,
        o::ArcherAnimationState& animationGlobals,o::MissileState& missileGlobals,RawCampaignArcherEffects effects)
        :world(input),core(input.Core()),target(finder),random(rng),animationState(animationGlobals),missileState(missileGlobals) {
        if (&target.Pool()!=&core.pool) throw std::invalid_argument("캠페인 Archer 현재 표적 Pool 불일치");
        disc=std::make_unique<o::RawDiscRegular>(core.pool,animationTypes,core.frame);
        auto flight=std::move(effects.missile);
        flight.owner=[this](o::Sid sid,std::uint8_t owner) { core.owner.Set(sid,owner); };
        flight.unpop=[this](o::Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };
        flight.pop=[this](o::Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        flight.center=[this](o::Sid sid) { return target.Center(sid); };
        flight.destroy=[this](o::Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        missile=std::make_unique<o::RawMissile>(core.pool,core.types,missileTypes,missileState,
            o::MakeMissileFactoryHooks(core.pool,core.factory,std::move(flight)));
        auto hooks=std::move(effects.animation);
        hooks.common.center=[this](o::Sid sid) { return target.Center(sid); };
        hooks.common.unpop=[this](o::Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };
        hooks.common.repop=[this](o::Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
        hooks.common.randomInteger=[this](std::int32_t low,std::int32_t high) {
            return low+static_cast<std::int32_t>(random.Next(static_cast<std::uint32_t>(high-low)));
        };
        hooks.notifySurface=[this](o::Sid sid) { core.building.NotifySurface(sid); };
        auto cannonHooks=o::MakeGunAnimationHostHooks(core.processes,hooks.common); // 실제 Cannon의 notify/kill 타입을 Archer와 구별한다.
        cannonHooks=o::MakeGunMissileHooks(core.processes,*missile,std::move(cannonHooks));
        thunderRotateState.sunCannonType=131;
        thunderRotate=std::make_unique<o::RawGunAnimation>(core.pool,core.types,thunderRotateState,cannonHooks);
        cannon=std::make_unique<o::RawGunAnimation>(core.pool,core.types,cannonState,std::move(cannonHooks));
        auto fireHooks=o::MakeCannonFireHostHooks(core.processes,hooks.common); // 실제 Fire 클래스의 notify/kill을 공통 Missile과 연결한다.
        fireHooks=o::MakeGunMissileHooks(core.processes,*missile,std::move(fireHooks));
        sunFireState.kind=o::CannonFireKind::Sun;sunFireState.type=128;sunFireState.displayType=80;sunFireState.effectType=11;sunFireState.delay=.07f;
        sunFire=std::make_unique<o::RawCannonFire>(core.pool,core.types,frameTypes,sunFireState,fireHooks);
        thunderFire=std::make_unique<o::RawCannonFire>(core.pool,core.types,frameTypes,thunderFireState,std::move(fireHooks));
        auto crossbowHooks=o::MakeCrossbowAnimationHostHooks(core.processes,hooks.common); // 같은 공간/Missile과 별도 발사 클래스의 수명을 연결한다.
        crossbowHooks=o::MakeGunMissileHooks(core.processes,*missile,std::move(crossbowHooks));
        crossbow=std::make_unique<o::RawCrossbowAnimation>(core.pool,core.types,animationTypes,crossbowState,std::move(crossbowHooks));
        hooks=o::MakeArcherAnimationHostHooks(core.processes,std::move(hooks));
        hooks=o::MakeArcherMissileHooks(core.processes,*missile,std::move(hooks));
        crossbowRotateState.type=140;
        crossbowRotate=std::make_unique<o::RawArcherAnimation>(core.pool,core.types,animationTypes,crossbowRotateState,hooks);
        animation=std::make_unique<o::RawArcherAnimation>(core.pool,core.types,animationTypes,animationState,std::move(hooks));
    }
};
// 동일 객체 주소를 사용하는 내부 구현만 소유하며 callback을 아직 실행하지 않는다.
RawCampaignArcherBindings::RawCampaignArcherBindings(RawSurfaceWorld& world,const o::RawGunTarget& target,o::GameRandom& random,
    std::span<const o::ArcherTypeData> animationTypes,std::span<const o::PriestPlainCanonType> frameTypes,std::span<const o::MissileTypeData> missileTypes,
    o::ArcherAnimationState& animation,o::MissileState& missile,RawCampaignArcherEffects effects)
    :impl_(std::make_unique<Impl>(world,target,random,animationTypes,frameTypes,missileTypes,animation,missile,std::move(effects))) {}
// 상위가 Kernel 및 callback 사용을 끝낸 뒤 실제 모듈을 해제한다.
RawCampaignArcherBindings::~RawCampaignArcherBindings()=default;
// 원본 경과는 Archer에서 float로 읽으므로 여기서는 double 입력을 임의 고정하지 않는다.
void RawCampaignArcherBindings::PrepareFrame(o::FrameTime time) {
    if (!std::isfinite(time.game) || !std::isfinite(time.delta) || time.delta<0) throw std::invalid_argument("Archer 현재 clock/경과 오류");
    impl_->animationState.now=time.game;impl_->animationState.delta=time.delta;impl_->missileState.now=time.game;impl_->cannonState.now=time.game;
    impl_->crossbowRotateState.now=time.game;impl_->crossbowRotateState.delta=time.delta;impl_->crossbowState.now=time.game;
    impl_->thunderRotateState.now=time.game;impl_->sunFireState.now=time.game;impl_->thunderFireState.now=time.game;
}
// 실제71/128/131/140을 부모 TYPE/단계로 분배하고 원래 다른 소비자는 보존한다.
o::GunProcessHooks RawCampaignArcherBindings::GunHooks(o::GunProcessHooks hooks) const {
    hooks.startAnimation=[this,fallback=std::move(hooks.startAnimation)](std::uint32_t stage,o::Sid form,o::Sid target) {
        const auto* gun=dynamic_cast<const o::GunProcess*>(impl_->core.processes.FromForm(form));if (!gun) throw std::logic_error("캠페인 총 애니메이션의 현재 GunProcess 조회 실패");
        const auto type=impl_->core.pool.Slot(gun->Parent())[10];
        if (type==impl_->sunFire->Type() && stage==5) { if (!o::CreateCannonFire(impl_->core.processes,*impl_->sunFire,form,target)) throw std::logic_error("캠페인 썬 캐논 발사 부착 실패");return; }
        if (type==impl_->cannon->SunCannonType() && (stage==2 || stage==8)) { if (!o::CreateGunAnimation(impl_->core.processes,*impl_->cannon,stage,form,target)) throw std::logic_error("캠페인 썬 캐논 애니메이션 부착 실패");return; }
        if (type==impl_->thunderFire->Type() && stage==5) { if (!o::CreateCannonFire(impl_->core.processes,*impl_->thunderFire,form,target)) throw std::logic_error("캠페인 썬더 캐논 발사 부착 실패");return; }
        if (type==impl_->thunderFire->Type() && stage==2) { if (!o::CreateGunAnimation(impl_->core.processes,*impl_->thunderRotate,stage,form,target)) throw std::logic_error("캠페인 썬더 캐논 회전 부착 실패");return; }
        if (type==impl_->crossbow->Type() && stage==2) { if (!o::CreateArcherAnimation(impl_->core.processes,*impl_->crossbowRotate,stage,form,target)) throw std::logic_error("캠페인 크로스보우 회전 부착 실패");return; }
        if (type==impl_->crossbow->Type() && stage==5) { if (!o::CreateCrossbowAnimation(impl_->core.processes,*impl_->crossbow,form,target)) throw std::logic_error("캠페인 크로스보우 발사 부착 실패");return; }
        if (!fallback) throw std::logic_error("캠페인 총 애니메이션 미복원 TYPE"+std::to_string(type)+" stage"+std::to_string(stage));fallback(stage,form,target);
    };
    return o::MakeArcherAnimationProcessHooks(impl_->core.processes,*impl_->animation,std::move(hooks));
}
// 같은 Kernel의 예약 처리에서 실제 투사체 가상 Handle만 추가한다.
o::RegularHandler RawCampaignArcherBindings::Handler(o::RegularHandler fallback) const { return impl_->disc->Handler(std::move(fallback)); }
// 같은 몸체를 후속 연결/검사에 제공한다.
const o::RawArcherAnimation& RawCampaignArcherBindings::Animation() const { return *impl_->animation; }
const o::RawMissile& RawCampaignArcherBindings::Missile() const { return *impl_->missile; }
const o::SidPool& RawCampaignArcherBindings::Pool() const { return impl_->core.pool; }
}
