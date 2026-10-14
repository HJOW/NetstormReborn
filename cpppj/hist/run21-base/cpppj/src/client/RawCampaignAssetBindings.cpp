// 현재 Core/Player 원문을 다시 읽고 선택된 미복원 효과만 명시적으로 거부한다.
#include "client/RawCampaignAssetBindings.h"
#include "o/RawContainedLookup.h"
#include "o/RawPathSearch.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 초기 총과 원본 fire/rotate ProcessForm 번호이며 Flags는 실제 factory에 둔다.
constexpr std::uint32_t kGunProcess=34,kFireProcess=35,kRotateProcess=36,kNoIsland=127;
// 비정렬 원본 BYTE/WORD/DWORD를 현재 raw 원문에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 지정한 저장폭의 낮은 바이트부터 조합한다.
    for (std::size_t index=0;index<width;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return value;
}
// 실제 finder/island getter의 유한 signed32 범위에 있는 plain ftol 입력이다.
std::int32_t Truncate(float value) {
    if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("캠페인 자산 조회 좌표 원본 정수 범위 오류");
    return static_cast<std::int32_t>(value);
}
// 누락된 선택 효과를 성공/noop로 바꾸지 않고 해당 실제 호출에서 거부한다.
template<class Result,class... Arguments>
void RejectMissing(std::function<Result(Arguments...)>& callback,const char* message) {
    if (!callback) callback=[message](Arguments...)->Result { throw std::logic_error(message); };
}
}
// 콜백이 참조하는 모듈의 주소는 Impl 수명 동안 고정된다.
struct RawCampaignAssetBindings::Impl {
    RawSurfaceWorld& world; // 실제 Pop/Unpop/Repop/Destroy를 담당하는 상위 공유 월드다.
    RawWorldCore core; // 같은 장기 월드의 pool/hash/TYPE/Kernel 등 참조 묶음이며 사본 저장소가 아니다.
    const o::RawCarrierPostPop& damageable; // 실제 비가상 Damageable→공통 PostPop 사슬이다.
    o::AssetPostPopState& assets; // 현재 실제 목록/로딩/신전 상태는 상위 캠페인이 소유한다.
    o::GameRandom& random; // 다른 월드 기능과 동일 LCG 전역을 함께 소비한다.
    o::RawIslandRuntime& islands; // 같은 CoreData의 현재100개/32BYTE 섬 기록이다.
    std::span<std::uint8_t> players; // 실제9 Player BYTE이며 AI/신전 원문은 같은 저장소를 읽는다.
    o::PlayerWorldState& playerState; // 현재 battleOwner/fortMode를 매 실제 호출에서 조회한다.
    RawCampaignAssetGlobals globals; // 장기 전역/TYPE 원소 배열을 참조로만 보관한다.
    RawCampaignAssetEffects effects; // 외부 UI/음향/아직 없는 하위의 실제 소비자다.
    o::RawGeyserAiRegistration geyser; // 같은9 Player의 현재AI 포인터를 번호순으로 직접 읽는다.
    o::RawContainedLookup contained; // 현재 Form 체인의 실제 타입/순번 조회 몸체다.
    o::RawFrameAdvance advance; // 현재 Core.frame/프레임 표의 실제 진행기다.
    std::unique_ptr<o::RawGunProcess> gun; // 원본 초기 총 생성자/상태 기계와 실제 Host 소비자다.
    std::unique_ptr<o::RawAssetPostPop> postPop; // 실제 초기 자산 가상 PostPop 분배 몸체다.
    std::unique_ptr<o::RawFactoryPostPop> factory; // 같은 Damageable/프레임/목록의 실제 Factory/아웃포스트 몸체다.
    std::unique_ptr<o::RawSquidLevel> level; // 같은 Pool의 baseLevel과 signed 범위 진단 몸체다.
    std::unique_ptr<o::RawAssetRegular> regular; // 실제 초기 예약의 가상 Handle 몸체다.
    std::array<std::uint8_t,64> packet{}; // 실제 Gun packet writer가 현재 한 요청을 작성하는 임시 저장소다.
    bool writingGun{},active{}; // 송신 분배와 공통 PostPop 등록의 현재 상태다.
    // 저장객체 Pop 전에 같은 Pool/TYPE/9기록 계약을 확인하고 실제 모듈을 구성한다.
    Impl(RawSurfaceWorld& input,const o::RawCarrierPostPop& carrier,o::AssetPostPopState& state,
        o::GameRandom& rng,o::RawIslandRuntime& list,std::span<std::uint8_t> records,
        o::PlayerWorldState& playerGlobals,RawCampaignAssetGlobals current,RawCampaignAssetEffects hooks)
        :world(input),core(input.Core()),damageable(carrier),assets(state),random(rng),islands(list),players(records),playerState(playerGlobals),globals(current),effects(std::move(hooks)),
        geyser(core.pool.Edition(),players),contained(core.pool,[this](o::Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); }),advance(core.pool,core.frames,core.frame) {
        const auto count=core.pool.Edition()==o::OriginalEdition::Patch1078 ? 188U : 171U;
        if (&damageable.Pool()!=&core.pool || &islands.Pool()!=&core.pool || core.types.size()!=count || core.frames.size()!=count || core.shapes.size()!=count || globals.elements.size()!=count || players.size()!=9*(core.pool.Edition()==o::OriginalEdition::Patch1078 ? 0xb4U : 0xacU)) throw std::invalid_argument("캠페인 자산 현재 Core/TYPE/Player 저장소 불일치");
        gun=std::make_unique<o::RawGunProcess>(core.pool,core.types,globals.gun,GunHooks());
        postPop=std::make_unique<o::RawAssetPostPop>(core.pool,core.types,core.postPop,damageable,assets,PostHooks());
        postPop->BindPlayerRecords(players,&playerState.battleOwner);
        factory=std::make_unique<o::RawFactoryPostPop>(core.pool,core.hash,core.types,core.frames,core.postPop,damageable,assets,FactoryHooks());
        auto assertion=effects.levelAssertion;RejectMissing(assertion,"캠페인 Factory 실제 Level 진단 미연결");
        level=std::make_unique<o::RawSquidLevel>(core.pool,core.types,std::move(assertion));
        regular=std::make_unique<o::RawAssetRegular>(core.pool,core.hash,core.types,core.frames,globals.elements,core.frame,advance,random,assets,globals.regular,RegularHooks());
        Prepare(core.processState.now);
    }
    // 기존 Factory body의 예약/공간/프레임만 같은 Core에 연결한다. 큰 진단/지역 효과는 제공한 경계를 쓴다.
    o::FactoryPostPopHooks FactoryHooks() {
        auto hooks=effects.factory;hooks.reserveRegular=effects.postPop.reserveRegular;
        hooks.addRegular=[this](o::Sid sid,std::uint32_t event,float payload) { if (!core.processes.AddRegular(sid,event,payload)) throw std::logic_error("캠페인 Factory Regular 부착 실패"); };
        hooks.destroy=[this](o::Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        hooks.updateState=[this](o::Sid sid) { core.display.Update(core.pool.Slot(sid)); };
        RejectMissing(hooks.reserveRegular,"캠페인 Factory 실제 Regular 확보 미연결");
        RejectMissing(hooks.warnDuplicate,"캠페인 Factory 중복 진단 미연결");
        RejectMissing(hooks.regionOwnership,"캠페인 아웃포스트 실제 지역 소유권 미연결");
        return o::MakeFactoryPostPopHooks(core.pool,core.frame,std::move(hooks));
    }
    // 현재 게임시각과 local/fort 모드를 같은 Player 전역에서 조회한다. 다른 상태/목록은 복사하지 않는다.
    void Prepare(double now) {
        if (!std::isfinite(now) || playerState.battleOwner>o::kPlayerCount) throw std::out_of_range("캠페인 자산 현재 clock/Player owner 오류");
        globals.gun.now=now;globals.gun.fortMode=playerState.fortMode;globals.regular.fortMode=playerState.fortMode;assets.fortMode=playerState.fortMode;
    }
    // 실제 getter0046f530/CD004bdc50은 plain 절삭한 hash0을 읽고 bridge/없음이면127이다.
    std::int32_t IslandNumber(float x,float y) const {
        const auto cellX=Truncate(x),cellY=Truncate(y);
        if (cellX<0 || cellY<0 || cellX>=256 || cellY>=256) return kNoIsland;
        const auto sid=o::Sid{core.hash.Entries(0)[static_cast<std::size_t>(cellY)*256+cellX]};
        if (!sid.value) return kNoIsland;
        const auto raw=core.pool.Slot(sid);const auto type=static_cast<std::size_t>(raw[10]);
        if (type>=core.types.size() || !(core.types[type].flags2&0x1000006U)) throw std::logic_error("캠페인 자산 island getter 원본 genus assert");
        return core.types[type].flags2&4U ? kNoIsland : static_cast<std::int32_t>(Read(raw,8,2));
    }
    // 원본 one-point 일반finder의 현재순서/중복을 유지한다. 표면전용 finder로 바꾸지 않는다.
    std::vector<o::Sid> AtPosition(float x,float y) const {
        const auto cellX=Truncate(x),cellY=Truncate(y);o::RawSquidFinder finder(core.pool,core.hash,core.types);std::vector<o::Sid> result;
        // 현재후보/다음링크의 원본 순서를 그대로 수집한다.
        for (auto sid=finder.Begin({cellX,cellY,cellX,cellY});sid.value;sid=finder.Next()) result.push_back(sid);
        return result;
    }
    // Residence의 일반owner는 실제 Core.owner다. Vortex 파생은 제공한 실제 가상몸체를 요구한다.
    void SetOwner(o::Sid sid,std::uint32_t owner) const {
        if (effects.postPop.setOwner) { effects.postPop.setOwner(sid,owner);return; }
        const auto type=static_cast<std::size_t>(core.pool.Slot(sid)[10]);
        if (type>=core.types.size() || (core.types[type].flags2&0x200U)) throw std::logic_error("캠페인 자산 실제 Vortex 가상 owner 미연결");
        core.owner.Set(sid,owner);
    }
    // 부모 Form 체인에서 실제fire35/rotate36의 첫번호를 찾는 기존 Gun helper 계약이다.
    o::Sid AnimationForm(o::Sid parent) const {
        auto sid=o::Sid{static_cast<std::uint16_t>(Read(core.pool.Slot(parent),6,2))};std::uint32_t visited=0;
        // 원본 첫일치 순서를 보존하며 손상된 무한체인은 유한풀 경계에서 거부한다.
        while (sid.value) {
            if (sid.value>=core.pool.Capacity() || ++visited>=core.pool.Capacity()) throw std::logic_error("캠페인 총 현재 Animation Form 체인 오류");
            const auto raw=core.pool.Slot(sid);if (raw[10]==kFireProcess || raw[10]==kRotateProcess) return sid;sid=o::Sid{static_cast<std::uint16_t>(Read(raw,4,2))};
        }
        return {};
    }
    // 현재 실제 Gun Form과 raw fields가 같은 Kernel 객체인지 확인한다.
    o::GunProcess& CurrentGun(const o::GunProcessFields& fields) const {
        auto* process=dynamic_cast<o::GunProcess*>(core.processes.FromForm(fields.Form()));
        if (!process || &process->Fields()!=&fields) throw std::logic_error("캠페인 총 현재 Host/raw 이미지 불일치");return *process;
    }
    // 초기 생성·현재 Form 전송·차단·난수를 실제 현재 Core에 연결한다.
    o::GunProcessHooks GunHooks() {
        auto hooks=effects.gun;
        hooks.repopForm=[this](o::GunProcessFields& fields) { core.processes.RefreshForm(CurrentGun(fields)); };
        hooks.animationForm=[this](o::Sid parent) { return AnimationForm(parent); };
        hooks.killAnimation=[this](o::Sid form,std::uint32_t flags) { auto* process=core.processes.FromForm(form);if (!process || (core.pool.Slot(form)[10]!=kFireProcess && core.pool.Slot(form)[10]!=kRotateProcess)) throw std::logic_error("캠페인 총 Animation 현재 Host 오류");core.processes.Kill(*process,flags); };
        hooks.parentBlocked=[this](o::Sid parent) { return (core.pool.Slot(parent)[core.pool.Edition()==o::OriginalEdition::Patch1078 ? 40U : 35U]&0x20U)!=0; };
        hooks.randomBetween=[this](float low,float high) { return static_cast<double>(random.Next(65536))*(1.0/65536.0)*std::abs(static_cast<double>(high)-low)+low; };
        RejectMissing(hooks.removeDependency,"캠페인 총 실제 표적감시 제거 미연결");RejectMissing(hooks.addDependency,"캠페인 총 실제 표적감시 생성 미연결");RejectMissing(hooks.refreshRange,"캠페인 총 현재시각 압축/공간통지 미연결");RejectMissing(hooks.targetingMode,"캠페인 총 현재 공격방향 getter 미연결");RejectMissing(hooks.findTarget,"캠페인 총 실제 표적 finder 미연결");RejectMissing(hooks.startAnimation,"캠페인 총 실제 파생 Animation factory 미연결");RejectMissing(hooks.validateIslandOwners,"캠페인 총 현재 지면소유자 진단 미연결");RejectMissing(hooks.checkForms,"캠페인 총 현재 Form 진단 미연결");RejectMissing(hooks.shotContext,"캠페인 총 현재8DWORD 발사문맥 미연결");RejectMissing(hooks.shoot,"캠페인 총 실제 MayFire 미연결");RejectMissing(hooks.firingInterval,"캠페인 총 실제 TYPE 발사간격 미연결");RejectMissing(hooks.attach,"캠페인 총 단독 Construct 대신 실제 CreateGunProcess가 필요합니다");return hooks;
    }
    // 현재자산 PostPop의 Core 몸체만 설치하고 UI/음향/기타 파생은 선택시거부한다.
    o::AssetPostPopHooks PostHooks() {
        auto hooks=effects.postPop;
        hooks.addRegular=[this](o::Sid sid,std::uint32_t event,float payload) { if (!core.processes.AddRegular(sid,event,payload)) throw std::logic_error("캠페인 자산 실제 Regular 부모 부착 실패"); };
        hooks.createGunProcess=[this](o::Sid sid) { Prepare(core.processState.now);if (!o::CreateGunProcess(core.processes,*gun,sid,kGunProcess)) throw std::logic_error("캠페인 자산 실제 Gun 부모 부착 실패"); };
        hooks.removeAnimation=[this](o::Sid sid) { if (!effects.queueCommand) throw std::logic_error("캠페인 발사기 실제 입력명령 큐 미연결");effects.queueCommand(globals.archerNoticeCommand,sid.value,0); };
        hooks.surface=[this](float x,float y) { return o::RawPathSurfaceAt(core.hash,{x,y}); };
        hooks.setOwner=[this](o::Sid sid,std::uint32_t owner) { Prepare(core.processState.now);SetOwner(sid,owner); };
        hooks.notifySurface=[this](o::Sid sid) { core.building.NotifySurface(sid); };
        hooks.geyserRegistration=[this](o::Sid sid,std::uint32_t state) { geyser.Register(sid.value,state); };
        hooks.atPosition=[this](float x,float y) { return AtPosition(x,y); };
        hooks.unpop=[this](o::Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };hooks.repop=[this](o::Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
        const auto typeDword=hooks.typeDword;
        hooks.typeDword=[this,typeDword](o::Sid sid,std::uint32_t offset) { const auto type=static_cast<std::size_t>(core.pool.Slot(sid)[10]);if (type>=core.frames.size()) throw std::out_of_range("캠페인 자산 현재TYPE 프레임 표 오류");if (offset==0x130 || offset==0x134) return std::bit_cast<std::uint32_t>(core.frames[type].Run(offset==0x130 ? 'A' : 'B').first);if (!typeDword) throw std::logic_error("캠페인 자산 실제 TYPE 추가DWORD 미연결");return typeDword(sid,offset); };
        hooks.islandNumber=[this](float x,float y) { return IslandNumber(x,y); };hooks.islandExists=[this](std::int32_t island) { return islands.Exists(island)!=0; };hooks.islandTemple=[this](std::int32_t island) { return o::Sid{static_cast<std::uint16_t>(islands.Temple(island))}; };hooks.registerTemple=[this](std::int32_t island,o::Sid sid) { islands.Temple(island,sid.value); };hooks.destroy=[this](o::Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        RejectMissing(hooks.reserveRegular,"캠페인 자산 실제 Regular 확보 경계 미연결");RejectMissing(hooks.checkForms,"캠페인 자산 현재 Form 진단 미연결");RejectMissing(hooks.gunFrame,"캠페인 다른 총 실제 각도→Graph표 미연결");RejectMissing(hooks.setGraph,"캠페인 다른 총 실제 Graph BYTE 쓰기 미연결");RejectMissing(hooks.startBlocker,"캠페인 배리케이드 실제 프로세스 미연결");RejectMissing(hooks.playImpactSound,"캠페인 신전 실제 impactSound 미연결");RejectMissing(hooks.clearPlacement,"캠페인 실제 배치커서 취소 미연결");RejectMissing(hooks.regionNumber,"캠페인 실제 지역BYTE 지도 미연결");RejectMissing(hooks.hasContent,"캠페인 신전 실제 Slot 내용물 조회 미연결");RejectMissing(hooks.createSlot,"캠페인 신전 실제 Slot 생성 미연결");RejectMissing(hooks.refreshProduction,"캠페인 신전 실제 생산 갱신 미연결");return hooks;
    }
    // 현재레지던스/간헐천/신전 예약은 기존Handle를 사용하고 외부사건은같은fallback에남긴다.
    o::AssetRegularHooks RegularHooks() {
        auto hooks=effects.regular;
        hooks.notifySurface=[this](o::Sid sid) { core.building.NotifySurface(sid); };hooks.surface=[this](float x,float y) { return o::RawPathSurfaceAt(core.hash,{x,y}); };hooks.setOwner=[this](o::Sid sid,std::uint32_t owner) { SetOwner(sid,owner); };hooks.displayUpdate=[this](o::Sid sid,std::uint32_t flags) { core.display.Update(core.pool.Slot(sid),flags); };hooks.islandNumber=[this](float x,float y) { return static_cast<std::uint32_t>(IslandNumber(x,y)); };hooks.findAttached=[this](o::Sid parent,std::uint32_t type) { return contained.Find(parent,type); };
        RejectMissing(hooks.residenceEvent2,"캠페인 레지던스 실제 사건2 미연결");RejectMissing(hooks.ascendTick,"캠페인 신전 실제 Ascend 진행 미연결");RejectMissing(hooks.playerLeavingNotice,"캠페인 현재 Player 이탈 UI 통지 미연결");RejectMissing(hooks.countForeignOwnedIslands,"캠페인 현재 다른소유자 섬 조회 미연결");RejectMissing(hooks.requestLeave,"캠페인 실제 신전 이탈 packet 미연결");RejectMissing(hooks.regionNumber,"캠페인 실제 신전 지역BYTE 조회 미연결");RejectMissing(hooks.removeAnimation,"캠페인 실제 신전 입력명령 통지 미연결");RejectMissing(hooks.damageGround,"캠페인 실제 지형피해/효과 사슬 미연결");RejectMissing(hooks.hitPoints,"캠페인 실제 가상피해 분배 미연결");RejectMissing(hooks.startConstruction,"캠페인 실제 건설 프로세스 시작 미연결");return hooks;
    }
};
// 상위장기상태와실제모듈의참조만구성하며아직현재PostPop등록을바꾸지않는다.
RawCampaignAssetBindings::RawCampaignAssetBindings(RawSurfaceWorld& world,const o::RawCarrierPostPop& damageable,
    o::AssetPostPopState& assets,o::GameRandom& random,o::RawIslandRuntime& islands,
    std::span<std::uint8_t> playerRecords,o::PlayerWorldState& playerState,
    RawCampaignAssetGlobals globals,RawCampaignAssetEffects effects)
    :impl_(std::make_unique<Impl>(world,damageable,assets,random,islands,playerRecords,playerState,globals,std::move(effects))) {}
// 상위가현재Kernel/반환callback수명을끝낸후활성자산분배만해제한다.
RawCampaignAssetBindings::~RawCampaignAssetBindings() { if (impl_->active) { impl_->core.postPop.SetAssetPostPop(nullptr);impl_->core.postPop.SetFactoryPostPop(nullptr); } }
// 실제초기Pop전에한번등록하며동일객체의재활성은허용한다.
void RawCampaignAssetBindings::ActivatePostPop() { impl_->core.postPop.SetAssetPostPop(impl_->postPop.get());impl_->core.postPop.SetFactoryPostPop(impl_->factory.get());impl_->active=true; }
// 원본clock과현재Player모드를조회하여다음실제프레임에사용한다.
void RawCampaignAssetBindings::PrepareFrame(double now) { impl_->Prepare(now); }
// 실제부모로더의중첩값을사본상태없이기존자산모듈에전달한다.
void RawCampaignAssetBindings::SetLoadingDepth(std::uint32_t depth) { impl_->assets.loadingDepth=depth; }
// actual 가상84의 Level 저장 후 기본 표시 갱신을 생략하지 않는다.
void RawCampaignAssetBindings::SetState(o::Sid sid,std::uint32_t value) const { impl_->factory->SetState(*impl_->level,sid,value); }
// 실제지원가상표만처리하고다른부모는호출자가준실제fallback에넘긴다.
o::RegularHandler RawCampaignAssetBindings::Handler(o::RegularHandler fallback) const { return impl_->regular->Handler(std::move(fallback)); }
// 같은서버flags0의Gun packet만분배하고다른현재Process packet은기존경계를유지한다.
o::ProcessFormTransmission RawCampaignAssetBindings::Transmission(o::ProcessFormTransmission fallback) {
    o::ProcessFormTransmission hooks;
    hooks.writeData=[this,fallback](o::SquidProcess& process,bool parentChanged) { auto* gun=dynamic_cast<o::GunProcess*>(&process);impl_->writingGun=gun!=nullptr;if (gun) return static_cast<std::int32_t>(gun->WritePacket(impl_->packet,parentChanged));if (!fallback.writeData) throw std::logic_error("캠페인 다른실제Process packet writer 미연결");return fallback.writeData(process,parentChanged); };
    hooks.transmit=[this,fallback](std::int32_t length) { if (impl_->writingGun) { if (!impl_->effects.sendGunPacket) throw std::logic_error("캠페인 Gun flags0 실제 packet 송신 미연결");if (length<0 || static_cast<std::size_t>(length)>impl_->packet.size()) throw std::out_of_range("캠페인 Gun 실제 packet 길이 오류");impl_->effects.sendGunPacket(std::span<const std::uint8_t>(impl_->packet).first(static_cast<std::size_t>(length)));return; }if (!fallback.transmit) throw std::logic_error("캠페인 다른실제Process packet 송신 미연결");fallback.transmit(length); };return hooks;
}
// 상위설치/검사가같은원본PostPop몸체를조회한다.
const o::RawAssetPostPop& RawCampaignAssetBindings::PostPop() const { return *impl_->postPop; }
// 상위같은월드Regular분배가실제Handle를조회한다.
const o::RawAssetRegular& RawCampaignAssetBindings::Regular() const { return *impl_->regular; }
// 초기총의현재생성/상태기계와같은모듈을조회한다.
const o::RawGunProcess& RawCampaignAssetBindings::Gun() const { return *impl_->gun; }
}
