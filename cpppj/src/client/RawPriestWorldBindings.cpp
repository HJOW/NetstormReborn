// 원본 몸체의 순서/재조회를 유지하며 새로운 월드별 계산이나 복사 사제 목록을 만들지 않는다.
#include "client/RawPriestWorldBindings.h"
#include "o/RawContainedLookup.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 판본 공통 vtable 기록값을 호스트 주소로 실행하지 않고 little endian으로 읽는다.
std::uint32_t Vtable(std::span<const std::uint8_t> raw) {
    std::uint32_t value=0;
    // 원본 DWORD의 네 바이트만 읽는다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[index])<<(index*8);
    return value;
}
}
// 콜백이 참조하는 모듈의 주소를 고정하고 순환 참조는 첫 실제 호출 전에 Bind로 해결한다.
struct RawPriestWorldBindings::Impl {
    // core: 다른 자산도 사용하는 단일 월드 자료다. 모든 참조는 연결 객체보다 오래 살아야 한다 (RawPriestWorldCore).
    RawPriestWorldCore core;
    // state: 실제 전역을 보관하는 공유 상태다. 사제 목록 저장소 크기는 월드 초기화에서 명시 설정한다 (RawPriestWorldState).
    RawPriestWorldState& state;
    // effects: 공간 효과는 월드의 실제 가상 Pop/Unpop/Destroy를 받는다. 계산을 임의 기본값으로 대체하지 않는다 (RawPriestWorldEffects).
    RawPriestWorldEffects effects;
    // host: Form이나 프로세스의 등록·통지·수명을 연결하는 호스트.
    o::SquidProcessHost* host{};
    // damageable: 피해 가능한 객체의 HP·삭제·후처리 연결.
    const o::RawDamageablePreDestroy* damageable{};
    // placement: 최종 배치 허용 여부를 판정하는 처리기 또는 상태.
    const o::RawPriestPlacement* placement{};
    // fallFrames: 추락 프레임.
    std::vector<o::PriestFallFrames> fallFrames;
    // lookup: 조건에 맞는 현재 객체를 찾는 조회 처리기.
    std::unique_ptr<o::RawPriestForcefield> lookup;
    // priest: 사제 상태를 조회·변경하는 처리기 또는 사제 경로 여부.
    std::unique_ptr<o::RawPriestState> priest;
    // owner: 소유 플레이어 번호 또는 소유권 처리 자료.
    std::unique_ptr<o::RawPriestOwner> owner;
    // shield: 사제 보호막의 생성·통지·상태 처리 자료.
    std::unique_ptr<o::RawPriestShield> shield;
    // advance: 글자를 그린 뒤 다음 글자 기준점으로 이동할 폭.
    std::unique_ptr<o::RawFrameAdvance> advance;
    // fall: 객체 추락 상태와 프레임 진행 처리 자료.
    std::unique_ptr<o::RawPriestFall> fall;
    // check: 타입/지면이 허용할 때만 walker 낙하를 요청하는 조회 객체다. 상태/HP는 자체 조건이 아니다 (std::unique_ptr<o::RawCarrierCheck>).
    std::unique_ptr<o::RawCarrierCheck> check;
    // carrierPost: Carrier 확인→Damageable 지면 처리→공통 장부를 직접 호출한다. 파생 가상 지원은 별도다 (std::unique_ptr<o::RawCarrierPostPop>).
    std::unique_ptr<o::RawCarrierPostPop> carrierPost;
    // prefix: 사제 목록/회복 예약 접두를 실행한다. 전체 postPop 조합에서도 이 접두를 재사용한다 (std::unique_ptr<o::RawPriestPostPop>).
    std::unique_ptr<o::RawPriestPostPop> prefix;
    // tail: 목록/회복 접두→Carrier→현재 지면/프레임→낙하/보호막을 같은 풀에서 실행한다 (std::unique_ptr<o::RawPriestPostPopTail>).
    std::unique_ptr<o::RawPriestPostPopTail> tail;
    // carrierPre: 캐리어 전처리.
    std::unique_ptr<o::RawCarrierPreDestroy> carrierPre;
    // priestPre: 사제 전처리.
    std::unique_ptr<o::RawPriestPreDestroy> priestPre;
    // fieldPost: 사용: 같은 풀의 실제 예약 훅을 구성한 뒤 SquidPostPop::SetForcefieldPostPop에 등록한다 (std::unique_ptr<o::RawForcefieldPostPop>).
    std::unique_ptr<o::RawForcefieldPostPop> fieldPost;
    // fieldRegular: 사용: Handle을 Regular 처리기에 연결하고 advance/destroy를 같은 월드의 실제 모듈로 공급한다 (std::unique_ptr<o::RawForcefieldRegular>).
    std::unique_ptr<o::RawForcefieldRegular> fieldRegular;
    // regen: 사제 HP 회복과 SP 갱신을 처리하는 자료.
    std::unique_ptr<o::RawPriestRegen> regen;
    // spawn: 사제의 생성·배치·등록 처리 자료.
    std::unique_ptr<o::RawPriestSpawn> spawn;
    // contained: 객체 내용물의 생성·검색·등록 처리 자료.
    std::unique_ptr<o::RawContainedLookup> contained;
    // 필수 경계는 부분 등록 전에 확인한다. 생성/소유자/프레임은 실제 core 모듈만 사용한다.
    Impl(RawPriestWorldCore input,RawPriestWorldState& globals,RawPriestWorldEffects hooks)
        :core(input),state(globals),effects(std::move(hooks)),fallFrames(core.frames.size()) {
        const auto& pool=core.pool;
        if (&core.reward.Pool()!=&pool || &core.factory.Pool()!=&pool || &core.owner.Pool()!=&pool ||
            &core.postPop.Pool()!=&pool || &core.frame.Pool()!=&pool || core.types.size()!=core.frames.size() ||
            core.spots.size()!=65536 || core.surfaces.size()!=65536 ||
            core.surfaces.data()!=core.hash.Entries(0).data() || !effects.pop || !effects.unpop ||
            !effects.repop || !effects.destroy || !effects.claimGround || !effects.ownerNotice ||
            !effects.notifySurface || !effects.dependentFound || !effects.reserveRegular || !effects.fallSound)
            throw std::invalid_argument("사제 월드의 풀/공간/필수 효과 연결 오류");
        (void)state.priests.priests.Items();
        // 실제 타입 +0x154/+0x140은 방향 J/E의 첫 프레임 표다. 저장/낙하마다 임의 프레임을 만들지 않는다.
        for (std::size_t index=0;index<core.frames.size();++index)
            fallFrames[index]={core.frames[index].Run('J').first,core.frames[index].Run('E').first};
        lookup=std::make_unique<o::RawPriestForcefield>(core.pool,core.hash,core.types,state.forcefield);
        priest=std::make_unique<o::RawPriestState>(core.pool,core.reward,core.types,core.spots);
        owner=std::make_unique<o::RawPriestOwner>(core.pool,core.ownerMode,state.owner,o::PriestOwnerHooks{
            [this](o::Sid sid,std::uint32_t player) { core.owner.Set(sid,player); },effects.ownerNotice});
        auto shieldEffects=o::MakePriestShieldCreationHooks(core.pool,core.factory,core.owner,effects.shield);
        shieldEffects.pop=effects.pop;
        shield=std::make_unique<o::RawPriestShield>(core.pool,*lookup,state.shield,std::move(shieldEffects));
        advance=std::make_unique<o::RawFrameAdvance>(core.pool,core.frames,core.frame);
        o::PriestFallHooks falling;
        falling.ensureShield=[this](o::Sid sid) { shield->Ensure(sid); };
        falling.reserveRegular=effects.reserveRegular;
        falling.addSharedRegular=[this](o::Sid sid,std::uint32_t event,float payload) { Host().AddSharedRegular(sid,event,payload); };
        falling.unpop=effects.unpop;falling.repop=effects.repop;falling.sound=effects.fallSound;
        falling.clearShield=[this](o::Sid sid) { ClearShield(sid); };
        falling=o::MakePriestFallFrameHooks(core.frame,*advance,std::move(falling));
        fall=std::make_unique<o::RawPriestFall>(core.pool,*priest,core.frames,fallFrames,core.surfaces,state.fall,std::move(falling));
        check=std::make_unique<o::RawCarrierCheck>(core.pool,core.types,core.spots,
            o::MakePriestFallWalker(core.pool,*fall,effects.otherWalkerFall));
        carrierPost=std::make_unique<o::RawCarrierPostPop>(core.pool,core.types,state.carrier,
            o::MakeCarrierPostPopHooks(core.pool,core.postPop,[this](o::Sid sid) { static_cast<void>(check->Check(sid)); },effects.claimGround));
        prefix=std::make_unique<o::RawPriestPostPop>(core.pool,core.reward,state.priests,o::PriestPostPopHooks{
            [this](o::Sid sid,std::uint32_t event) { return Host().FindEvent(sid,event)!=nullptr; },effects.reserveRegular,
            [this](o::Sid sid,std::uint32_t event,float payload) { Host().AddRegular(sid,event,payload); }});
        tail=std::make_unique<o::RawPriestPostPopTail>(core.pool,*prefix,*carrierPost,*priest,core.frames,core.spots,
            o::MakePriestPostPopTailShieldHooks(core.pool,*shield,*lookup,effects.destroy,[this](o::Sid sid) { fall->Begin(sid); }));
        carrierPre=std::make_unique<o::RawCarrierPreDestroy>(core.pool,state.contained,o::CarrierPreDestroyHooks{
            [this](o::Sid sid,std::uint32_t flags) { Damageable().Validate(sid,flags); },
            [this](o::Sid sid,std::uint32_t flags) { Damageable().PreDestroy(sid,flags); },effects.dependentFound});
        priestPre=std::make_unique<o::RawPriestPreDestroy>(core.pool,state.priests,
            o::MakeCarrierPreDestroyHooks(core.pool,*carrierPre,o::MakePriestForcefieldHooks(core.pool,*lookup,
                o::PriestPreDestroyHooks{{},effects.destroy,{},{}})));
        fieldPost=std::make_unique<o::RawForcefieldPostPop>(core.pool,o::ForcefieldPostPopHooks{
            effects.reserveRegular,[this](o::Sid sid,std::uint32_t event,float payload) { Host().AddRegular(sid,event,payload); }});
        fieldRegular=std::make_unique<o::RawForcefieldRegular>(core.pool,core.hash,core.types,state.forcefieldRegular,
            o::MakeForcefieldFrameHooks(core.pool,*advance,o::ForcefieldRegularHooks{{},effects.destroy}));
        contained=std::make_unique<o::RawContainedLookup>(core.pool,effects.destroy);
        auto regeneration=effects.regen;
        regeneration.damageable=[this](o::Sid sid) { DamageableRefresh(sid); };
        regeneration.shape=[this](std::uint32_t type) { return MovementProcessType(type); };
        regeneration.occupied=[this](o::Sid sid,std::uint32_t process) { return contained->Find(sid,process).value!=0; };
        regen=std::make_unique<o::RawPriestRegen>(core.pool,core.reward,*priest,state.hitPoints,
            o::PriestHitPointHooks{effects.unpop,effects.repop},core.random,state.regen,std::move(regeneration));
        spawn=std::make_unique<o::RawPriestSpawn>(core.pool,core.types,core.spots,state.spawn,o::PriestSpawnHooks{
            [this](const o::PriestPlacementQuery& query) { return Placement().MayPlace(query); },
            [this](std::uint32_t type,std::uint32_t flags) { return core.factory.Create(type,flags); },
            [this](o::Sid sid,std::uint32_t player) { SetOwner(sid,player); },effects.pop,effects.notifySurface,
            [this](o::Sid sid) { static_cast<void>(check->Check(sid)); }});
        // 모든 모듈 생성이 성공한 뒤에만 가상 postPop의 지원 범위를 확장한다.
        core.postPop.SetPriestPostPop(tail.get());core.postPop.SetForcefieldPostPop(fieldPost.get());
    }
    // 로딩 순환 참조가 해결되지 않은 상태에서 예약을 허용하지 않는다.
    o::SquidProcessHost& Host() const {
        if (!host) throw std::logic_error("사제 월드 ProcessHost 미연결");return *host;
    }
    // 삭제 목록 변경 전에 실제 하위 validate가 실행될 수 있게 미연결을 명시 진단한다.
    const o::RawDamageablePreDestroy& Damageable() const {
        if (!damageable) throw std::logic_error("사제 월드 Damageable 삭제 몸체 미연결");return *damageable;
    }
    // 나선 탐색이 미복원 공간 검사를 무조건 허용하는 경로로 바뀌지 않게 한다.
    const o::RawPriestPlacement& Placement() const {
        if (!placement) throw std::logic_error("사제 월드 배치 검사 몸체 미연결");return *placement;
    }
    // 조회 직후의 유효 범위만 검사한다. 실제 가상 삭제가 보호막의 Form/공간을 정리한다.
    void ClearShield(o::Sid sid) const {
        const auto field=lookup->Find(sid);
        if (field.value>0 && field.value<core.pool.Capacity()) effects.destroy(field,0);
    }
    // 사제 가상 소유자와 공통 소유자의 같은 풀을 유지한다.
    void SetOwner(o::Sid sid,std::uint32_t player) const {
        if (owner->Handles(sid)) owner->Set(sid,player);else core.owner.Set(sid,player);
    }
    // 00488fe0/CD 0047d020는 모양 번호가 아니라 이동 ProcessForm 타입 번호를 선택한다.
    std::uint32_t MovementProcessType(std::uint32_t type) const {
        const auto& selection=state.movement;
        const bool patch=core.pool.Edition()==o::OriginalEdition::Patch1078;
        // CD의 worm 특수 반환은 타입 표를 조회하기 전이며 패치는 먼저 조회한다.
        if (!patch && type==selection.sunWalker) return selection.wormProcess;
        if (type>=core.types.size()) throw std::out_of_range("이동 프로세스 선택 타입 범위 오류");
        if (patch && (type==selection.sunWalker || type==selection.empty)) return selection.wormProcess;
        if (type==selection.windWalker || type==selection.rainWalker) return selection.walkerProcess;
        if (core.types[type].flags2&o::TypeFlag2::kBalloon) return selection.balloonProcess;
        if (type==selection.teleport) return selection.teleportProcess;
        if (type==selection.bulf) return selection.bulfProcess;
        if (type==selection.dude) return selection.dudeProcess;
        if (type==selection.priest) return selection.priestProcess;
        if (type==selection.manaBolt) return selection.flyProcess;
        if (patch) return selection.wormProcess;
        throw std::invalid_argument("CD 이동 프로세스 미지원 타입(원본 assert)");
    }
    // type +0xd4는 burning 모양 포인터가 아니라 numeric damageEffect다. 0이면 후속 조회/효과가 없다.
    void DamageableRefresh(o::Sid sid) const {
        const auto raw=core.pool.Slot(sid);
        if (raw[11]&1) return;
        if (raw[10]>=core.types.size()) throw std::out_of_range("Damageable 현재 타입 범위 오류");
        if (core.types[raw[10]].damageEffect==0) return;
        if (!effects.regen.damageable) throw std::logic_error("nonzero damageEffect의 burning 전체 몸체 미연결");
        effects.regen.damageable(sid);
    }
};
// 구현 주소는 다른 월드 모듈의 콜백 수명 동안 고정된다.
RawPriestWorldBindings::RawPriestWorldBindings(RawPriestWorldCore core,RawPriestWorldState& state,RawPriestWorldEffects effects)
    :impl_(std::make_unique<Impl>(core,state,std::move(effects))) {}
// 외부 공통 postPop에 남아 있는 사제/보호막 포인터를 제거한다.
RawPriestWorldBindings::~RawPriestWorldBindings() {
    impl_->core.postPop.SetPriestPostPop(nullptr);impl_->core.postPop.SetForcefieldPostPop(nullptr);
}
// 같은 풀만 연결하고 기존 호스트의 의미가 바뀌는 재연결을 거부한다.
void RawPriestWorldBindings::BindProcesses(o::SquidProcessHost& host) {
    if (&host.Pool()!=&impl_->core.pool || impl_->host) throw std::invalid_argument("사제 ProcessHost 풀/중복 연결 오류");impl_->host=&host;
}
// 사제 preDestroy가 실제 Damageable prefix와 공통 깊이 감소를 사용하게 한다.
void RawPriestWorldBindings::BindDamageable(const o::RawDamageablePreDestroy& damageable) {
    if (&damageable.Pool()!=&impl_->core.pool || impl_->damageable) throw std::invalid_argument("사제 Damageable 풀/중복 연결 오류");impl_->damageable=&damageable;
}
// 해방하는 새 사제의 후보 검사에 실제 원본 배치 파이프라인을 사용한다.
void RawPriestWorldBindings::BindPlacement(const o::RawPriestPlacement& placement) {
    if (&placement.Pool()!=&impl_->core.pool || impl_->placement) throw std::invalid_argument("사제 Placement 풀/중복 연결 오류");impl_->placement=&placement;
}
// 전투 입력을 허용하기 전에 지연 연결의 누락을 명시 확인한다.
void RawPriestWorldBindings::ValidateReady() const { (void)impl_->Host();(void)impl_->Damageable();(void)impl_->Placement(); }
// 로딩 중/종료의 사제 소유자·지면 claim·로컬 안내가 같은 전역을 관찰하게 한다.
void RawPriestWorldBindings::SetLoadingDepth(std::uint32_t depth) {
    impl_->state.owner.loadingDepth=depth;impl_->state.shield.loadingDepth=depth;impl_->state.carrier.loadingDepth=depth;
    impl_->state.regen.neutralBlocked=depth!=0;
}
// 권한 전역을 복사하는 경계는 월드 모드 변경 때 한 번 호출한다.
void RawPriestWorldBindings::SetAuthority(bool authority) {
    impl_->state.hitPoints.authority=authority;impl_->state.fall.authority=authority;
    impl_->state.carrier.boss=authority;impl_->state.contained.boss=authority;
}
// 원본 현재 로컬 플레이어 범위만 허용한다. 로딩 소유자 0 문제와 구별한다.
void RawPriestWorldBindings::SetLocalPlayer(std::uint32_t player) {
    if (player>o::kPlayerCount) throw std::out_of_range("사제 로컬 플레이어 범위 오류");
    impl_->state.owner.battlePlayer=player;impl_->state.owner.fortPlayer=player;impl_->state.shield.localPlayer=player;
}
// 공개 소유자 경로는 사제 원래 소유자/로컬 표식 재정의를 보존한다.
void RawPriestWorldBindings::SetOwner(o::Sid sid,std::uint32_t player) const { impl_->SetOwner(sid,player); }
// HP 변경은 사제 실제 경계 판정을 거쳐 동일한 가상 공간 경로를 호출한다.
void RawPriestWorldBindings::SetHitPoints(o::Sid sid,std::int32_t value) const {
    impl_->priest->SetHitPoints(sid,value,impl_->state.hitPoints,{impl_->effects.unpop,impl_->effects.repop});
}
// Carrier genus 제외/지면 검사와 실제 walker 낙하를 함께 사용한다.
bool RawPriestWorldBindings::CheckCarrier(o::Sid sid) const { return impl_->check->Check(sid); }
// 표면 붕괴에서도 현재 사제 방향 J를 읽고 같은 공유 Regular를 예약한다.
bool RawPriestWorldBindings::FallWalker(o::Sid sid) const {
    return o::MakePriestFallWalker(impl_->core.pool,*impl_->fall,impl_->effects.otherWalkerFall)(sid);
}
// 선택하는 실제 전역은 호출마다 읽는다. 사제 이외의 중립 회복 변이에서도 원본 우선순위를 유지한다.
std::uint32_t RawPriestWorldBindings::MovementProcessType(std::uint32_t type) const { return impl_->MovementProcessType(type); }
// 원본의 자연 반환과 아직 외부에 있는 nonzero 전체 body를 구별한다.
void RawPriestWorldBindings::DamageableRefresh(o::Sid sid) const { impl_->DamageableRefresh(sid); }
// 보호막은 다른 자산과 같은 사건 번호 1/2를 사용하므로 vtable/타입을 먼저 검사한다.
o::RegularHandler RawPriestWorldBindings::RegularHandler(o::RegularHandler fallback) const {
    auto* implementation=impl_.get();
    return [implementation,fallback=std::move(fallback)](o::Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
        const auto raw=implementation->core.pool.Slot(sid);
        const bool patch=implementation->core.pool.Edition()==o::OriginalEdition::Patch1078;
        const auto table=Vtable(raw);
        if (raw[10]==o::kPriestType && table==(patch ? o::kPatchPriestVtable : o::kCdPriestVtable)) {
            if (event==o::kPriestRegenEvent) return implementation->regen->Handle(sid,event,count,payload);
            if (event==o::kPriestFallEvent) return implementation->fall->Handle(sid,event,count,payload);
            if (fallback) return fallback(sid,event,count,payload);
            throw std::invalid_argument("사제 월드 미복원 Regular 사건");
        }
        if (raw[10]==o::kForcefieldType && table==(patch ? o::kPatchForcefieldVtable : o::kCdForcefieldVtable))
            return implementation->fieldRegular->Handle(sid,event,count,payload);
        if (fallback) return fallback(sid,event,count,payload);
        return payload;
    };
}
// Form 분배기의 공통 emit/selection/재귀 경로를 보존하면서 사제 삭제 접두만 덧씌운다.
o::SquidDestroyHooks RawPriestWorldBindings::DestroyHooks(o::SquidDestroyHooks fallback) const {
    return o::MakePriestPreDestroyHooks(impl_->core.pool,*impl_->priestPre,std::move(fallback));
}
// 해방·충돌·승패 모듈이 동일한 계산/목록을 재사용한다.
const o::RawPriestSpawn& RawPriestWorldBindings::Spawn() const { return *impl_->spawn; }
// 실제 상태 계산기를 다른 월드 몸체에서 조회한다.
const o::RawPriestState& RawPriestWorldBindings::PriestState() const { return *impl_->priest; }
// 보호막 조회의 타입 전역과 hash가 전체 월드와 일치한다.
const o::RawPriestForcefield& RawPriestWorldBindings::Forcefield() const { return *impl_->lookup; }
// 목록과 원래 소유자별 상태는 미션 outcome까지 같은 객체를 읽는다.
o::PriestPostPopState& RawPriestWorldBindings::Priests() const { return impl_->state.priests; }
}
