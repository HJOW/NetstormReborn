// 사제와 같은 SHP/기준점 자료를 사용하되 타입별 패턴 전체를 지원한다.
#pragma once
#include "o/RawPriestPlacementShape.h"

namespace netstorm::o {
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
    const SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    std::span<const PriestPlainCanonType> frames_;
    std::span<const SquidDisplayShape> shapes_;
    std::span<const PriestTypeHotspot> hotspots_;
    PriestPlacementGeometryState& geometryState_;
    PriestPlacementShapeState& state_;
};
}
