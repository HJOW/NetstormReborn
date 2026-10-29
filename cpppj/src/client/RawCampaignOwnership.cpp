// 기존 지역 getter/투표/도장/신전 몸체를 재사용하며 외부 효과를 생략하지 않는다.
#include "client/RawCampaignOwnership.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// getter는 편향 없는 원본 float 절삭을 사용한다. 투표 snap은 기존 어댑터가 따로 수행한다.
int Truncate(float value) {
    const auto result=std::trunc(static_cast<double>(value)); // 원본 plain ftol의 현재값이다.
    if (!std::isfinite(result) || result<std::numeric_limits<int>::min() || result>std::numeric_limits<int>::max())
        throw std::out_of_range("캠페인 소유권 현재 지면 좌표 범위 오류");return static_cast<int>(result);
}
}
// 내부 콜백은 이 구조체와 장기 공유 자료의 주소를 빌린다.
struct RawCampaignOwnership::Impl {
    RawSurfaceWorld& world; // 실제 생성·가상 공간·삭제의 상위 월드다.
    RawWorldCore core; // 같은 풀/해시/타입/프레임/장부 참조다.
    o::RawIslandRuntime& islands; // 실제 현재100개 기록과 Player 첫 지역을 공유한다.
    CampaignOwnershipEffects effects; // 다른 실제 파생/생산/진단 경계다.
    o::CanonPlacementTerrainState getterState; // 지면 getter의 현재 sentinel이며 배치 순회와 별도다.
    std::unique_ptr<o::RawCanonPlacementTerrain> getter; // 기존 실제 RegionAt 몸체만 사용한다.
    std::unique_ptr<o::RawRegionPaint> paint; // 동일 원본 hash0의 실제 flood·경계 도장이다.
    std::unique_ptr<o::RawRegionOwnership> ownership; //같은추가/신전목록을매순회읽는원본투표다.
    std::unique_ptr<o::RawVortexOwner> vortex; // 같은 Player/섬/도장/공통 Owner의 실제 신전 가상 몸체다.
    // 자료 동일성을 먼저 확인한 뒤 실제 하위 몸체를 구성한다.
    Impl(RawSurfaceWorld& input,o::RawIslandRuntime& list,o::AssetPostPopState& assets,
        std::span<std::uint8_t> players,std::span<const o::PriestPlainCanonType> frames,std::span<const std::uint32_t> themes,
        o::GameRandom& random,o::RegionPaintState& paintState,o::RegionOwnershipState& ownershipState,
        std::uint32_t& revision,std::uint32_t terrain,std::uint32_t edge,CampaignOwnershipEffects hooks)
        :world(input),core(input.Core()),islands(list),effects(std::move(hooks)) {
        if (&islands.Pool()!=&core.pool || frames.size()!=core.frames.size() || themes.size()!=core.types.size() ||
            !effects.otherOwner || !effects.refreshProduction || !effects.themeMode || !effects.missingIsland)
            throw std::invalid_argument("캠페인 소유권의 같은 Core/현재 표/필수 효과 누락");
        // 이 객체는 RegionAt만 사용한다. 배치 순회는 상위 실제 PlacementPipeline으로 분리한다.
        o::CanonPlacementTerrainHooks guards;
        guards.candidatePermission=[](const o::CanonPlacementQuery&,o::Sid)->bool { throw std::logic_error("소유권 getter의 별도 배치 후보 경계"); };
        guards.shapePermission=[](const o::CanonPlacementQuery&,const o::CanonPlacementCell&)->bool { throw std::logic_error("소유권 getter의 별도 배치 모양 경계"); };
        guards.finishRelations=[](const o::CanonPlacementQuery&,o::CanonPlacementTerrainState&)->bool { throw std::logic_error("소유권 getter의 별도 배치 완료 경계"); };
        getter=std::make_unique<o::RawCanonPlacementTerrain>(core.pool,core.types,frames,core.hash.Entries(0),getterState,std::move(guards));
        o::RegionPaintHooks paintHooks;
        paintHooks.setOwner=effects.otherOwner;paintHooks.boundaryAt=[this](float x,float y,std::uint32_t type) { return world.Find(x,y,type); };
        paintHooks.regionAt=[this](float x,float y) { return static_cast<std::uint32_t>(IslandNumber(x,y)); };
        paint=std::make_unique<o::RawRegionPaint>(core.pool,core.frames,core.hash.Entries(0),terrain,edge,random,paintState,ownershipState,std::move(paintHooks));
        o::RegionOwnershipHooks ownershipHooks;ownershipHooks.themeMode=effects.themeMode;
        ownershipHooks=o::MakeRegionPaintOwnershipHooks(core.pool,*paint,std::move(ownershipHooks));
        ownershipHooks=o::MakeTerrainRegionOwnershipHooks(core.pool,*getter,std::move(ownershipHooks));
        //원본455380은0055a70c와005e189c(신전)를투표한다.0055a4d4의생산목록에는이시점의삭제중인신전/일반건물이남으므로섬색/테마가재점유된다.
        ownership=std::make_unique<o::RawRegionOwnership>(core.pool,assets.Outposts(),assets.Temples(),themes,ownershipState,std::move(ownershipHooks));
        o::VortexOwnerHooks vortexHooks;
        vortexHooks.islandNumber=[this](float x,float y) { return IslandNumber(x,y); };
        vortexHooks.islandExists=[this](std::int32_t island) { return islands.Exists(island)!=0; };
        vortexHooks.setIslandOwner=[this](std::int32_t island,std::uint32_t owner) { islands.Owner(island,owner); };
        vortexHooks.regionOwnership=[this](float x,float y) { ownership->Update(x,y); };
        vortexHooks.refreshProduction=effects.refreshProduction;vortexHooks.missingIsland=effects.missingIsland;
        vortex=std::make_unique<o::RawVortexOwner>(core.pool,core.owner,players,assets.Outposts(),revision,std::move(vortexHooks));
        islands.BindPlayerRecords(players);
    }
    // plain getter는 float를 먼저 원본 정수로 변환하고 현재 hash0를 조회한다.
    std::int32_t IslandNumber(float x,float y) const { return std::bit_cast<std::int32_t>(getter->RegionAt(Truncate(x),Truncate(y))); }
};
// 내부 모듈 주소가 이동하지 않는 단일 공유 구현을 소유한다.
RawCampaignOwnership::RawCampaignOwnership(RawSurfaceWorld& world,o::RawIslandRuntime& islands,o::AssetPostPopState& assets,
    std::span<std::uint8_t> players,std::span<const o::PriestPlainCanonType> frames,std::span<const std::uint32_t> themes,
    o::GameRandom& random,o::RegionPaintState& paint,o::RegionOwnershipState& ownership,std::uint32_t& revision,
    std::uint32_t terrain,std::uint32_t edge,CampaignOwnershipEffects effects)
    :impl_(std::make_unique<Impl>(world,islands,assets,players,frames,themes,random,paint,ownership,revision,terrain,edge,std::move(effects))) {}
// 상위 Kernel/Asset 콜백이 먼저 종료된 뒤 같은 내부 모듈만 해제한다.
RawCampaignOwnership::~RawCampaignOwnership()=default;
// 원본 신전 가상 재정의의 Player/섬/지역 효과를 공통 Owner로 축약하지 않는다.
void RawCampaignOwnership::SetOwner(o::Sid sid,std::uint32_t owner) const {
    if (impl_->vortex->Handles(sid)) impl_->vortex->Set(sid,owner);else impl_->effects.otherOwner(sid,owner);
}
// 실제 getter를 상위 같은 월드의 자산 조회에 제공한다.
std::int32_t RawCampaignOwnership::IslandNumber(float x,float y) const { return impl_->IslandNumber(x,y); }
// Factory/신전의 같은 mode0 지역 갱신을 제공한다.
void RawCampaignOwnership::UpdateRegion(float x,float y) const { impl_->ownership->Update(x,y); }
// 같은 Player 주소/Pool과 원본 몸체를 읽기 전용으로 제공한다.
const o::RawVortexOwner& RawCampaignOwnership::Vortex() const { return *impl_->vortex; }
}
