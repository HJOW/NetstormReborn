// 기존 완료 몸체를 동일 사제/Carrier/Graph/공간/현재Path에 연결한다.
#include "client/RawCampaignMotionBindings.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// 현재 세계의 실제 module 주소만 보관하며 gameplay 자료를 복사하지 않는다.
struct RawCampaignMotionBindings::Impl {
    RawSurfaceWorld& world; // 가상 Pop/Unpop/삭제와 Kernel의 실제 소유자다.
    RawWorldCore core; // 실제 Pool/TYPE/장부/공간/시계 참조다.
    const RawPriestWorldBindings& priests; // 사제 삭제/PostPop과 같은 Carrier 및 지면 검사를 공유한다.
    const o::RawPlayerPlacementAnchor& anchor; // 현재 raw GraphFor getter다.
    RawCampaignMotionState& state; // 상위가 소유한 mutable 모드/타입/동맹 전역이다.
    RawCampaignMotionEffects effects; // 선택된 큰 외부 하위의 실제 경계다.
    std::unique_ptr<o::RawCanonPlacementTerrain> terrain; // 중립 도착의 실제 plain 지역 getter다.
    std::unique_ptr<o::RawPathCleanup> cleanup; // 부모 공간 접두 이후 정리 몸체다.
    std::unique_ptr<o::RawPriestHalt> halt; // 같은 spot/HP/Carrier의 실제 Halt다.
    std::unique_ptr<o::RawCarrierArrival> carrier; // 현재 Path를 매 조회하는 공통 도착 몸체다.
    std::unique_ptr<o::RawPriestArrival> arrival; // 실제 사제 도착 wrapper다.
    std::unique_ptr<RawCampaignPathBindings> paths; // 기존 실제 BFS/Start/Host54/프레임 연결이다.

    // 다른 월드의 하위를 현재 첫 명령 전에 거부한다.
    Impl(RawSurfaceWorld& source,const RawPriestWorldBindings& bindings,const o::RawIslandRuntime& islands,
        const o::RawContainedCreate& contained,const o::RawPlayerPlacementAnchor& graph,const o::SquidReward& hp,
        std::span<const o::PriestPlainCanonType> frames,RawCampaignMotionState& globals,
        o::RawPathStartState& start,o::RawPathProcessState& process,RawCampaignMotionEffects hooks)
        :world(source),core(source.Core()),priests(bindings),anchor(graph),state(globals),effects(std::move(hooks)) {
        if (&priests.PriestState().Pool()!=&core.pool || &islands.Pool()!=&core.pool ||
            &contained.Pool()!=&core.pool || &anchor.Pool()!=&core.pool || &hp.Pool()!=&core.pool ||
            &hp.Bookkeeping()!=&core.bookkeeping || frames.size()!=core.frames.size() ||
            &start.clock!=&core.processState || &process.clock!=&core.processState)
            throw std::invalid_argument("캠페인 이동의 동일 Pool/장부/현재시계/프레임 불일치");
        o::CanonPlacementTerrainHooks regionHooks;
        // 여기서는 실제 RegionAt만 쓴다. 배치 명령을 이 getter 인스턴스로 실행할 수 없다.
        regionHooks.candidatePermission=[](const o::CanonPlacementQuery&,o::Sid)->bool { throw std::logic_error("캠페인 도착 getter의 배치 권한 미연결"); };
        regionHooks.shapePermission=[](const o::CanonPlacementQuery&,const o::CanonPlacementCell&)->bool { throw std::logic_error("캠페인 도착 getter의 배치 Shape 미연결"); };
        regionHooks.finishRelations=[](const o::CanonPlacementQuery&,o::CanonPlacementTerrainState&)->bool { throw std::logic_error("캠페인 도착 getter의 배치 Finish 미연결"); };
        terrain=std::make_unique<o::RawCanonPlacementTerrain>(core.pool,core.types,frames,core.hash.Entries(0),state.terrain,std::move(regionHooks));
        auto cleanupHooks=effects.cleanup; // 의식/환불/할당은 현재 제공한 큰 실제 하위로 남긴다.
        cleanupHooks.contained=[this](o::Sid sid,std::uint32_t mask,std::uint32_t type,std::uint32_t index) { return priests.Carrier().FindContained(sid,mask,type,index); };
        cleanupHooks.graphFor=[this](o::Sid sid) { return anchor.GraphFor(sid.value); };
        cleanupHooks.addShared=[this](o::Sid sid,std::uint32_t event,float payload) { core.processes.AddSharedRegular(sid,event,payload); };
        cleanupHooks.destroy=[this](o::Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        cleanup=std::make_unique<o::RawPathCleanup>(core.pool,core.hash,core.types,state.cleanup,std::move(cleanupHooks));
        auto haltHooks=effects.halt; // 선택된 큰 포획/전달은 제공한 실제 몸체다.
        haltHooks.unpop=[this](o::Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };
        haltHooks.pop=[this](o::Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        halt=std::make_unique<o::RawPriestHalt>(core.pool,core.hash,core.types,core.spots,priests.CarrierCheck(),priests.PriestState(),hp,state.halt,std::move(haltHooks));
        auto carrierHooks=effects.carrier; // 운반/회피/경제/조회의 큰 하위는 필수 제공 경계다.
        carrierHooks.path=[this](o::Sid sid) { if (!paths) throw std::logic_error("캠페인 Path 구성 전 도착 요청");return paths->CurrentRuntime(sid); };
        carrierHooks.destroy=[this](o::Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        carrier=std::make_unique<o::RawCarrierArrival>(core.pool,core.hash,core.types,priests.Carrier(),*cleanup,priests.PriestState(),anchor,contained,core.neighbors,state.carrier,std::move(carrierHooks));
        auto priestHooks=effects.priest; // 실제 중립 귀환만 외부 큰 하위로 유지한다.
        priestHooks.destroy=[this](o::Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        arrival=std::make_unique<o::RawPriestArrival>(core.pool,priests.PriestState(),hp,*terrain,islands,core.neighbors,contained,*carrier,state.priest,std::move(priestHooks));
        effects.path.carrierArrival=[this](o::Sid sid,std::uint32_t first,std::uint32_t second) { return carrier->Arrival(sid,first,second); }; // 현재 동일429800 도착 몸체다.
        paths=std::make_unique<RawCampaignPathBindings>(world,core,priests,*halt,*arrival,*cleanup,
            Lookup(),start,process,state.search,std::move(effects.path));
    }
    std::unique_ptr<o::RawContainedLookup> currentLookup; // 현재54 Form을 같은 원본 종속 체인에서 찾는다.
    // 구성 후 현재 조회에만 사용하며 별도 Path 포인터를 캐시하지 않는다.
    const o::RawContainedLookup& Lookup() {
        currentLookup=std::make_unique<o::RawContainedLookup>(core.pool,[this](o::Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); });return *currentLookup;
    }
};
// 순환된 Carrier→현재Path 경계를 마지막 Path 구성으로 닫는다.
RawCampaignMotionBindings::RawCampaignMotionBindings(RawSurfaceWorld& world,const RawPriestWorldBindings& priests,
    const o::RawIslandRuntime& islands,const o::RawContainedCreate& contained,const o::RawPlayerPlacementAnchor& anchor,
    const o::SquidReward& hp,std::span<const o::PriestPlainCanonType> frames,RawCampaignMotionState& state,
    o::RawPathStartState& start,o::RawPathProcessState& process,RawCampaignMotionEffects effects)
    :impl_(std::make_unique<Impl>(world,priests,islands,contained,anchor,hp,frames,state,start,process,std::move(effects))) {}
// 외부가 먼저 Kernel을 비우므로 여기서 현재 Path를 다시 삭제하지 않는다.
RawCampaignMotionBindings::~RawCampaignMotionBindings()=default;
// 기존 연결부가 현재 Host54를 매번 조회한다.
RawCampaignPathBindings& RawCampaignMotionBindings::Paths() const { return *impl_->paths; }
// 같은 실제 공통 도착 몸체를 빌린다.
const o::RawCarrierArrival& RawCampaignMotionBindings::CarrierArrival() const { return *impl_->carrier; }
// 187의 네 번 중 세 번은 같은 기존 공통 몸체를 실행한다.
std::uint32_t RawCampaignMotionBindings::CommonHalt(o::Sid parent) const { return impl_->halt->CommonHalt(parent); }
// 같은 실제 공간 이후 정리 몸체를 빌린다.
const o::RawPathCleanup& RawCampaignMotionBindings::Cleanup() const { return *impl_->cleanup; }
// 대피/귀환이 기존 현재 도착·영역 getter를 복제하지 않고 빌린다.
const o::RawPriestArrival& RawCampaignMotionBindings::PriestArrival() const { return *impl_->arrival; }
// 같은 실제 지면 조회를 다른 현재 부모 처리에도 제공한다.
const o::RawCanonPlacementTerrain& RawCampaignMotionBindings::Terrain() const { return *impl_->terrain; }
// 다른 runtime 하위가 같은 Pool인지 확인한다.
const o::SidPool& RawCampaignMotionBindings::Pool() const { return impl_->core.pool; }
}
