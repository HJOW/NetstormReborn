// 사제 배치의 비패턴 계약과 일반 패턴 모양 계산기를 연결한다.
#include "o/RawPriestPlacementGeometry.h"
#include "o/RawCanonPlacementGeometry.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 각 단계의 지역 효과를 필수 경계로 두어 미복원 지형 판단을 임의로 허용하지 않는다.
RawPriestPlacementGeometry::RawPriestPlacementGeometry(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const PriestPlainCanonType> frames,PriestPlacementGeometryState& state,PriestPlacementGeometryHooks hooks)
    :pool_(pool),types_(types),frames_(frames),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || frames.size()!=types.size() ||
        !hooks_.beginRegions || !hooks_.endShape || !hooks_.finishRegions) throw std::invalid_argument("사제 모양 자료/지역 경계 누락");
}
// 현재 사제 genus와 비패턴 계약은 일반 타입 계산기에 위임하기 전에 검사한다.
void RawPriestPlacementGeometry::CheckQuery(const PriestPlacementQuery& query) const {
    if (query.type>=types_.size() || !(types_[query.type].flags2&0x200000) ||
        std::find(state_.patternTypes.begin(),state_.patternTypes.end(),query.type)!=state_.patternTypes.end())
        throw std::invalid_argument("사제 비패턴 CanonDecoder 타입 계약 오류");
}
// 사제의 첫 인자는 원본 배치처럼 타입 번호이며 명시 프레임을 켜지 않는다.
SquidSearchArea RawPriestPlacementGeometry::Bounds(PriestPlacementQuery query) const {
    CheckQuery(query);
    return RawCanonPlacementGeometry(pool_,types_,frames_,state_).Bounds(
        {query.type,static_cast<int>(query.type),std::bit_cast<std::int32_t>(query.flags),query.x,query.y,false});
}
// 사제와 일반 패턴은 같은 실제 finder 사각형 산술을 사용한다.
SquidSearchArea RawPriestPlacementGeometry::CollisionArea(float x,float y,int footX,int footY) {
    if (footX<1 || footY<1) throw std::out_of_range("사제 모양 발자국 오류");
    return RawCanonPlacementGeometry::CollisionArea(x,y,footX,footY);
}
// 사제 지형/관계 경계의 인자와 순서를 공통 칸 순회에 연결한다.
bool RawPriestPlacementGeometry::Inspect(PriestPlacementQuery query,const std::function<bool(SquidSearchArea)>& scan) const {
    if (!scan) throw std::invalid_argument("사제 모양 충돌 탐색 경계 누락");
    CheckQuery(query);
    // 요청은 값으로 캡처해 외부 호출자가 바꿔도 원래 여섯 인자를 유지한다.
    CanonPlacementGeometryHooks hooks;
    hooks.beginRegions=[this,query](const CanonTypeQuery&) { hooks_.beginRegions(query); };
    hooks.endShape=[this,query](const CanonTypeQuery&,const CanonPlacementCell& cell) {
        return hooks_.endShape(query,cell.frame,cell.snappedX,cell.snappedY);
    };
    hooks.finishRegions=[this,query](const CanonTypeQuery&) { return hooks_.finishRegions(query); };
    // 일반 자산의 0 발자국 계산과 구별해 사제의 기존 양수 발자국 계약을 유지한다.
    hooks.beginShape=[this,query](const CanonTypeQuery&,const CanonPlacementCell& cell) {
        if (types_[query.type].footX<1 || types_[query.type].footY<1) throw std::out_of_range("사제 모양 발자국 오류");
        if (hooks_.beginShape) hooks_.beginShape(query,cell.frame,cell.snappedX,cell.snappedY);
    };
    return RawCanonPlacementGeometry(pool_,types_,frames_,state_).Inspect(
        {query.type,static_cast<int>(query.type),std::bit_cast<std::int32_t>(query.flags),query.x,query.y,false},
        [&scan](const CanonPlacementCell& cell) { return scan(cell.area); },hooks);
}
// 연결 검사에 사용할 실제 풀을 반환한다.
const SidPool& RawPriestPlacementGeometry::Pool() const { return pool_; }
// 이전 후보 충돌 연결을 유지하면서 미리보기 범위만 대체한다.
PriestPlacementPreviewHooks MakePriestGeometryPreviewHooks(const SidPool& pool,const RawPriestPlacementGeometry& geometry,PriestPlacementPreviewHooks hooks) {
    if (&pool!=&geometry.Pool()) throw std::invalid_argument("사제 모양/미리보기의 SID 풀이 다릅니다");
    hooks.bounds=[&geometry](const PriestPlacementQuery& query) { return geometry.Bounds(query); };return hooks;
}
// 실제 finder 호출은 기존 후보 검사기에 맡기며 모양 사각형의 생성/진행을 연결한다.
PriestPlacementCollisionHooks MakePriestGeometryCollisionHooks(const SidPool& pool,const RawPriestPlacementGeometry& geometry,PriestPlacementCollisionHooks hooks) {
    if (&pool!=&geometry.Pool()) throw std::invalid_argument("사제 모양/충돌의 SID 풀이 다릅니다");
    hooks.inspectRegions=[&geometry](const PriestPlacementQuery& query,const std::function<bool(SquidSearchArea)>& scan) { return geometry.Inspect(query,scan); };return hooks;
}
}
