// 실제 모듈의 동일 풀·공간·시간 계약을 연결하고 미공급 효과는 선택 시 거부한다.
#include "client/RawCampaignArcherBindings.h"
#include "o/RawDiscRegular.h"
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
    std::unique_ptr<o::RawDiscRegular> disc; // 실제 투사체 예약의 방향별 modulo 처리기다.
    // 모든 실제 공간/Host를 구성한 뒤 같은 원본 모듈에 공급한다.
    Impl(RawSurfaceWorld& input,const o::RawGunTarget& finder,o::GameRandom& rng,
        std::span<const o::ArcherTypeData> animationTypes,std::span<const o::MissileTypeData> missileTypes,
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
        hooks=o::MakeArcherAnimationHostHooks(core.processes,std::move(hooks));
        hooks=o::MakeArcherMissileHooks(core.processes,*missile,std::move(hooks));
        animation=std::make_unique<o::RawArcherAnimation>(core.pool,core.types,animationTypes,animationState,std::move(hooks));
    }
};
// 동일 객체 주소를 사용하는 내부 구현만 소유하며 callback을 아직 실행하지 않는다.
RawCampaignArcherBindings::RawCampaignArcherBindings(RawSurfaceWorld& world,const o::RawGunTarget& target,o::GameRandom& random,
    std::span<const o::ArcherTypeData> animationTypes,std::span<const o::MissileTypeData> missileTypes,
    o::ArcherAnimationState& animation,o::MissileState& missile,RawCampaignArcherEffects effects)
    :impl_(std::make_unique<Impl>(world,target,random,animationTypes,missileTypes,animation,missile,std::move(effects))) {}
// 상위가 Kernel 및 callback 사용을 끝낸 뒤 실제 모듈을 해제한다.
RawCampaignArcherBindings::~RawCampaignArcherBindings()=default;
// 원본 경과는 Archer에서 float로 읽으므로 여기서는 double 입력을 임의 고정하지 않는다.
void RawCampaignArcherBindings::PrepareFrame(o::FrameTime time) {
    if (!std::isfinite(time.game) || !std::isfinite(time.delta) || time.delta<0) throw std::invalid_argument("Archer 현재 clock/경과 오류");
    impl_->animationState.now=time.game;impl_->animationState.delta=time.delta;impl_->missileState.now=time.game;
}
// 기존 factory fallback을 보존하고 실제71 생성/부착/각초기화 순서를 재사용한다.
o::GunProcessHooks RawCampaignArcherBindings::GunHooks(o::GunProcessHooks hooks) const { return o::MakeArcherAnimationProcessHooks(impl_->core.processes,*impl_->animation,std::move(hooks)); }
// 같은 Kernel의 예약 처리에서 실제 투사체 가상 Handle만 추가한다.
o::RegularHandler RawCampaignArcherBindings::Handler(o::RegularHandler fallback) const { return impl_->disc->Handler(std::move(fallback)); }
// 같은 몸체를 후속 연결/검사에 제공한다.
const o::RawArcherAnimation& RawCampaignArcherBindings::Animation() const { return *impl_->animation; }
const o::RawMissile& RawCampaignArcherBindings::Missile() const { return *impl_->missile; }
const o::SidPool& RawCampaignArcherBindings::Pool() const { return impl_->core.pool; }
}
