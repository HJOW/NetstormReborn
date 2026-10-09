// 사제 배치 계약과 일반 자산 픽셀 계산을 연결한다.
#include "o/RawPriestPlacementShape.h"
#include "o/RawCanonPixelShape.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::o {
// 같은 타입 번호 체계로 프레임 표·SHP·기준점을 받는다. 실제 참조 수명은 호출자가 보장한다.
RawPriestPlacementShape::RawPriestPlacementShape(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const PriestPlainCanonType> frames,std::span<const SquidDisplayShape> shapes,
    std::span<const PriestTypeHotspot> hotspots,PriestPlacementGeometryState& geometryState,PriestPlacementShapeState& state)
    :pool_(pool),types_(types),frames_(frames),shapes_(shapes),hotspots_(hotspots),geometryState_(geometryState),state_(state) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) ||
        frames.size()!=types.size() || shapes.size()!=types.size() || hotspots.size()!=types.size())
        throw std::invalid_argument("사제 픽셀 모양 타입/메타/SHP/기준점 자료 누락");
}
// 사제 배치의 비패턴 계약을 유지하면서 공통 픽셀 getter로 계산한다.
PriestPlacementPixelShape RawPriestPlacementShape::Measure(std::uint32_t type,std::uint32_t argument,std::uint32_t direction,bool explicitFrame) const {
    if (type>=types_.size() || !(types_[type].flags2&0x200000) ||
        std::find(geometryState_.patternTypes.begin(),geometryState_.patternTypes.end(),type)!=geometryState_.patternTypes.end())
        throw std::invalid_argument("사제 픽셀 모양 비패턴 타입 계약 오류");
    return RawCanonPixelShape(pool_,types_,frames_,shapes_,hotspots_,geometryState_,state_).Measure(type,argument,direction,explicitFrame);
}
// 같은 풀에 연결하는 어댑터의 구성 검사에 사용한다.
const SidPool& RawPriestPlacementShape::Pool() const { return pool_; }
// 배치 경로는 명시 프레임을 켜지 않고 타입 번호를 첫 인자로 그대로 전달한다.
PriestPlacementHooks MakePriestShapePlacementHooks(const SidPool& pool,const RawPriestPlacementShape& shape,PriestPlacementHooks hooks) {
    if (&pool!=&shape.Pool()) throw std::invalid_argument("사제 픽셀 모양/배치의 SID 풀이 다릅니다");
    hooks.shape=[&shape](std::uint32_t type,std::uint32_t argument,std::uint32_t direction) { return shape.Measure(type,argument,direction).bounds; };return hooks;
}
}
