// 사제와 같은 SHP/기준점 자료를 사용하되 타입별 패턴 전체를 지원한다.
#pragma once
#include "o/RawPriestPlacementShape.h"

namespace netstorm::o {
// 사제와 같은 SHP/기준점 자료를 사용하되 타입별 패턴 전체를 지원한다.
class RawCanonPixelShape {
public:
    // 자산 로더가 제공한 기준점·기존 SquidRenderer::Shapes의 SHP 메타 자료를 참조한다.
    RawCanonPixelShape(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const PriestPlainCanonType> frames,std::span<const SquidDisplayShape> shapes,
        std::span<const PriestTypeHotspot> hotspots,PriestPlacementGeometryState& geometryState,
        PriestPlacementShapeState& state);
    // 0043cbd0 / CD 004ed7c0의 전체 경로다. 모든 패턴의 로컬 칸 좌표를 픽셀 배율로 합친다.
    PriestPlacementPixelShape Measure(std::uint32_t type,std::uint32_t argument,std::uint32_t direction,bool explicitFrame=false) const;
    // 배치 접두와 같은 풀/판본에 연결하는지 확인한다.
    const SidPool& Pool() const;
private:
    // 패치의 실제 frameCheck 조건과 CD의 물리 테이블 범위를 검사한다.
    const SquidDisplayFrame& Header(std::uint32_t type,int frame) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;
    // frames_: 타입별 프레임 자료.
    std::span<const PriestPlainCanonType> frames_;
    // shapes_: 프레임의 크기·그림 데이터를 조회하는 셰이프 자료.
    std::span<const SquidDisplayShape> shapes_;
    // hotspots_: 프레임 기준점을 찾는 타입별 핫스폿 자료.
    std::span<const PriestTypeHotspot> hotspots_;
    // geometryState_: 기하판정 상태.
    PriestPlacementGeometryState& geometryState_;
    // state_: 처리 중 조회·갱신하는 상태 (PriestPlacementShapeState).
    PriestPlacementShapeState& state_;
};
}
