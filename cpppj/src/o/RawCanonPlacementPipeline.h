// 배치의 모든 복원 모듈을 같은 현재 표면/공통 전역 상태에 연결한다.
#pragma once
#include "o/RawCanonPlacementRelations.h"
#include "o/RawCanonPlacementSurrounding.h"
#include "o/RawCanonPixelShape.h"
#include "o/RawPlayerPlacementAnchor.h"

namespace netstorm::o {
struct CanonPlacementPipelineState {
    PriestPlacementState placement;
    CanonPlacementPreviewState preview;
    CanonPlacementCollisionState collision;
    CanonPlacementTerrainState terrain;
    CanonPlacementPermissionState permission;
    CanonPlacementRelationsState relations;
    PlayerPlacementAnchorState anchor;
    PriestPlacementGeometryState geometry;
    PriestPlacementShapeState shape;
};
struct CanonPlacementPipelineData {
    std::span<const RiftTypeRecord> types;
    std::span<const PriestPlainCanonType> frames;
    std::span<const SquidDisplayShape> shapes;
    std::span<const PriestTypeHotspot> hotspots;
    std::span<const std::uint8_t> spots;
    std::span<const std::uint16_t> islands;
    std::span<const CanonPlacementIslandRegion> regions;
};
class RawCanonPlacementPipeline {
public:
    // 모든 자료/장부/상태는 호출자가 소유하고 이 객체보다 오래 살아야 한다.
    // 현재 표면은 hash의 0단계 배열을 직접 공유한다.
    RawCanonPlacementPipeline(const SidPool& pool,const SquidHash& hash,CanonPlacementPipelineData data,
        const SquidPostPopState& bookkeeping,const SquidPostPopList& additional,ContainedFinderState& contained,
        CanonPlacementPipelineState& state);
    // 내부 훅이 모듈 주소를 참조하므로 복사/이동으로 참조를 무효화하지 않는다.
    RawCanonPlacementPipeline(const RawCanonPlacementPipeline&)=delete;
    RawCanonPlacementPipeline& operator=(const RawCanonPlacementPipeline&)=delete;
    RawCanonPlacementPipeline(RawCanonPlacementPipeline&&)=delete;
    RawCanonPlacementPipeline& operator=(RawCanonPlacementPipeline&&)=delete;
    // 공통 전역의 중복 표현을 현재 값으로 맞춘 뒤 실제 접두부터 최종 반환까지 실행한다.
    bool MayPlace(CanonPlacementQuery query);
private:
    CanonPlacementPipelineState& state_;
    RawPlayerPlacementAnchor anchor_;
    RawCanonPlacementPermission permission_;
    RawCanonPlacementSurrounding surrounding_;
    RawCanonPlacementRelations relations_;
    RawCanonPlacementTerrain terrain_;
    RawCanonPlacementGeometry geometry_;
    RawCanonPixelShape shape_;
    RawCanonPlacementCollision collision_;
    RawCanonPlacementPreview preview_;
    RawCanonPlacement placement_;
};
}
