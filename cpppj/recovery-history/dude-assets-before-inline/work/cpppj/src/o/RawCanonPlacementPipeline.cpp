// 원본의 공통 저장소를 같은 C++ 참조에 연결한다.
#include "o/RawCanonPlacementPipeline.h"

namespace netstorm::o {
// 후보/주변/최종 정책은 모두 복원한 실제 모듈에 연결하고 현재 표면은 해시와 공유한다.
RawCanonPlacementPipeline::RawCanonPlacementPipeline(const SidPool& pool,const SquidHash& hash,
    CanonPlacementPipelineData data,const SquidPostPopState& bookkeeping,const SquidPostPopList& additional,
    ContainedFinderState& contained,CanonPlacementPipelineState& state)
    :state_(state),anchor_(pool,data.types,hash.Entries(0),bookkeeping,additional,contained,state.anchor),
    permission_(pool,state.permission,MakePlayerAnchorQuery(pool,anchor_)),
    surrounding_(pool,data.types,data.frames,data.islands,data.spots,permission_),
    relations_(pool,data.types,hash.Entries(0),data.regions,state.relations,state.placement,permission_),
    terrain_(pool,data.types,data.frames,data.islands,state.terrain,MakeCanonRelationsTerrainHooks(pool,relations_,
        MakeCanonSurroundingTerrainHooks(pool,surrounding_,MakeCanonPermissionTerrainHooks(pool,permission_,{})))),
    geometry_(pool,data.types,data.frames,state.geometry),shape_(pool,data.types,data.frames,data.shapes,data.hotspots,state.geometry,state.shape),
    collision_(pool,hash,data.types,state.collision,state.preview,MakeCanonGeometryCollisionHooks(pool,geometry_,
        [this](const CanonPlacementQuery& query) { return MakeCanonTerrainGeometryHooks(terrain_.Pool(),terrain_,query); },
        MakeCanonTerrainCollisionHooks(pool,terrain_,{}))),
    preview_(pool,data.types,data.spots,hash.Entries(0),state.preview,MakeCanonGeometryPreviewHooks(pool,geometry_,MakeCanonPlacementCollisionHooks(pool,collision_,{}))),
    placement_(pool,data.types,state.placement,MakeCanonShapePlacementHooks(pool,shape_,MakeCanonPlacementPreviewHooks(pool,preview_,{}))) {}
// 원본 공통 관계/그래프/noIsland 전역이 모듈별로 어긋나지 않게 같은 값을 공급한다.
bool RawCanonPlacementPipeline::MayPlace(CanonPlacementQuery query) {
    state_.preview.editor=state_.permission.editor;state_.preview.useAlliances=state_.permission.useAlliances;
    state_.preview.alliances=state_.permission.alliances;state_.anchor.graphReady=state_.permission.graphReady;
    state_.terrain.noIslandType=state_.geometry.patternTypes[3];return placement_.MayPlace(query);
}
// 전체 배치 파이프라인에 연결된 실제 풀 참조다.
const SidPool& RawCanonPlacementPipeline::Pool() const { return placement_.Pool(); }
}
