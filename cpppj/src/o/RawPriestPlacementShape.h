// 사제 비패턴 픽셀 범위 getter와 실제 SHP 프레임 메타 자료를 연결한다.
#pragma once
#include "o/RawPriestPlacementGeometry.h"
#include "o/SquidDisplay.h"

namespace netstorm::o {
struct PriestTypeHotspot {
    std::int32_t x{},y{}; // 타입 패치 +0x1dc/+0x1e0, CD +0x1bc/+0x1c0의 보정 기준점이다.
};
struct PriestPlacementShapeState {
    float scaleX{16},scaleY{11}; // 0059a92c/0059a930, CD 00565c44/00565c48의 현재 픽셀 배율이다.
};
struct PriestPlacementPixelShape {
    PriestPlacementRect bounds; // 빈 decoder는 (1000,1000,0,0)을 그대로 반환한다.
    float anchorX{},anchorY{}; // 최초 최솟값을 갱신한 프레임의 기준점 보정값이다.
};
class RawPriestPlacementShape {
public:
    // 자산 로더가 제공한 기준점·기존 SquidRenderer::Shapes의 SHP 메타 자료를 참조한다.
    RawPriestPlacementShape(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const PriestPlainCanonType> frames,std::span<const SquidDisplayShape> shapes,
        std::span<const PriestTypeHotspot> hotspots,PriestPlacementGeometryState& geometryState,
        PriestPlacementShapeState& state);
    // 0043cbd0 / CD 004ed7c0의 비패턴 사제 경로다. 원본처럼 decoder 좌표는 항상 (0,0)이다.
    PriestPlacementPixelShape Measure(std::uint32_t type,std::uint32_t argument,std::uint32_t direction,bool explicitFrame=false) const;
    // 배치 접두와 같은 풀/판본에 연결하는지 확인한다.
    const SidPool& Pool() const;
private:
    const SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    std::span<const PriestPlainCanonType> frames_;
    std::span<const SquidDisplayShape> shapes_;
    std::span<const PriestTypeHotspot> hotspots_;
    PriestPlacementGeometryState& geometryState_;
    PriestPlacementShapeState& state_;
};
// 초기 shape 경계만 실제 픽셀 범위 getter에 연결하고 미리보기/충돌 연결은 유지한다.
PriestPlacementHooks MakePriestShapePlacementHooks(const SidPool& pool,const RawPriestPlacementShape& shape,PriestPlacementHooks hooks);
}
