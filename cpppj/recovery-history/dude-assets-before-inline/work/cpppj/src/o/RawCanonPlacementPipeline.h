// 배치의 모든 복원 모듈을 같은 현재 표면/공통 전역 상태에 연결한다.
#pragma once
#include "o/RawCanonPlacementRelations.h"
#include "o/RawCanonPlacementSurrounding.h"
#include "o/RawCanonPixelShape.h"
#include "o/RawPlayerPlacementAnchor.h"

namespace netstorm::o {
// 일반 타입·패턴 배치 처리 흐름 처리 중 조회·갱신하는 상태 묶음이다.
struct CanonPlacementPipelineState {
    // placement: 최종 배치 허용 여부를 판정하는 처리기 또는 상태.
    PriestPlacementState placement;
    // preview: 실제 배치 전의 미리보기 판정·출력 처리기 또는 상태.
    CanonPlacementPreviewState preview;
    // collision: 후보 객체의 배치 충돌을 판정하는 처리기 또는 상태.
    CanonPlacementCollisionState collision;
    // terrain: 지형.
    CanonPlacementTerrainState terrain;
    // permission: 소유권·지역 관계에 따른 배치 허용 판정.
    CanonPlacementPermissionState permission;
    // relations: 표면·섬·지역 사이의 배치 관계 판정.
    CanonPlacementRelationsState relations;
    // anchor: 앵커.
    PlayerPlacementAnchorState anchor;
    // geometry: 프레임 발자국·사각형·칸 순회를 계산하는 기하 처리기 또는 상태.
    PriestPlacementGeometryState geometry;
    // shape: 프레임 그림을 찾는 셰이프 식별자 또는 셰이프 자료.
    PriestPlacementShapeState shape;
};
// 일반 타입·패턴 배치 처리 흐름 관련 처리기가 공유하는 입력 자료이다.
struct CanonPlacementPipelineData {
    // types: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types;
    // frames: 타입별 프레임 자료.
    std::span<const PriestPlainCanonType> frames;
    // shapes: 프레임의 크기·그림 데이터를 조회하는 셰이프 자료.
    std::span<const SquidDisplayShape> shapes;
    // hotspots: 프레임 기준점을 찾는 타입별 핫스폿 자료.
    std::span<const PriestTypeHotspot> hotspots;
    // spots: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<const std::uint8_t> spots;
    // islands: 현재 섬의 배치·소유권 자료.
    std::span<const std::uint16_t> islands;
    // regions: 지역.
    std::span<const CanonPlacementIslandRegion> regions;
};
// 배치의 모든 복원 모듈을 같은 현재 표면/공통 전역 상태에 연결한다.
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
    // 서버 건설 요청 어댑터가 같은 풀을 사용하는지 확인한다.
    const SidPool& Pool() const;
private:
    // state_: 처리 중 조회·갱신하는 상태 (CanonPlacementPipelineState).
    CanonPlacementPipelineState& state_;
    // anchor_: 앵커.
    RawPlayerPlacementAnchor anchor_;
    // permission_: 소유권·지역 관계에 따른 배치 허용 판정.
    RawCanonPlacementPermission permission_;
    // surrounding_: 주변.
    RawCanonPlacementSurrounding surrounding_;
    // relations_: 표면·섬·지역 사이의 배치 관계 판정.
    RawCanonPlacementRelations relations_;
    // terrain_: 지형.
    RawCanonPlacementTerrain terrain_;
    // geometry_: 프레임 발자국·사각형·칸 순회를 계산하는 기하 처리기 또는 상태.
    RawCanonPlacementGeometry geometry_;
    // shape_: 프레임 그림을 찾는 셰이프 식별자 또는 셰이프 자료.
    RawCanonPixelShape shape_;
    // collision_: 후보 객체의 배치 충돌을 판정하는 처리기 또는 상태.
    RawCanonPlacementCollision collision_;
    // preview_: 실제 배치 전의 미리보기 판정·출력 처리기 또는 상태.
    RawCanonPlacementPreview preview_;
    // placement_: 최종 배치 허용 여부를 판정하는 처리기 또는 상태.
    RawCanonPlacement placement_;
};
}
