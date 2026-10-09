// 미리보기 범위와 충돌 탐색 범위의 원본 계산 차이를 보존한다.
#include "o/RawPriestPlacementGeometry.h"
#include "o/CanonTypeDecoder.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 00502310 / CD 005019b0의 단정도 올림 보정값이다.
constexpr float kNearCeil=0.99999f;
// 원본 CRT의 int 범위 밖 입력은 C++의 정의되지 않은 변환으로 보내지 않는다.
int Truncate(double value) {
    value=std::trunc(value);
    if (!std::isfinite(value) || value<std::numeric_limits<int>::min() || value>std::numeric_limits<int>::max())
        throw std::out_of_range("사제 모양 좌표 절삭 범위 오류");
    return static_cast<int>(value);
}
}
// 각 단계의 지역 효과를 필수 경계로 두어 미복원 지형 판단을 임의로 허용하지 않는다.
RawPriestPlacementGeometry::RawPriestPlacementGeometry(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const PriestPlainCanonType> frames,PriestPlacementGeometryState& state,PriestPlacementGeometryHooks hooks)
    :pool_(pool),types_(types),frames_(frames),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || frames.size()!=types.size() ||
        !hooks_.beginRegions || !hooks_.endShape || !hooks_.finishRegions) throw std::invalid_argument("사제 모양 자료/지역 경계 누락");
}
// 타입 번호 자체를 첫 인자로 전달한다. 배치 경로는 명시 프레임 모드를 켜지 않는다.
CanonDecoder RawPriestPlacementGeometry::Decode(const PriestPlacementQuery& query) const {
    if (query.type>=types_.size() || !(types_[query.type].flags2&0x200000) ||
        std::find(state_.patternTypes.begin(),state_.patternTypes.end(),query.type)!=state_.patternTypes.end())
        throw std::invalid_argument("사제 비패턴 CanonDecoder 타입 계약 오류");
    const auto& meta=frames_[query.type];
    return DecodeCanonType(meta.frames,meta.defaultFrame,state_.patternTypes,
        {query.type,static_cast<int>(query.type),std::bit_cast<std::int32_t>(query.flags),query.x,query.y,false},pool_.Edition());
}
// 생성자의 첫 Advance 이후 현재 좌표와 순회 개수(비패턴은 1×1)를 사용한다.
SquidSearchArea RawPriestPlacementGeometry::Bounds(PriestPlacementQuery query) const {
    const auto decoder=Decode(query);const auto& type=types_[query.type];const auto area=decoder.Bounds(type.footX,type.footY);
    return {area[0],area[1],area[2],area[3]};
}
// left/top은 절삭한 float 좌표에서 발자국-1을 빼고, right/bottom은 원래 소수 좌표에 보정을 더한다.
SquidSearchArea RawPriestPlacementGeometry::CollisionArea(float x,float y,int footX,int footY) {
    if (footX<1 || footY<1) throw std::out_of_range("사제 모양 발자국 오류");
    const float snappedX=static_cast<float>(Truncate(x)),snappedY=static_cast<float>(Truncate(y));
    return {Truncate(static_cast<double>(snappedX)-(static_cast<std::int64_t>(footX)-1)),
        Truncate(static_cast<double>(snappedY)-(static_cast<std::int64_t>(footY)-1)),
        Truncate(static_cast<double>(x)+kNearCeil),Truncate(static_cast<double>(y)+kNearCeil)};
}
// 지형 경계가 타입/발자국을 바꾸면 그 시점의 현재 값을 다음 계산에 반영한다.
bool RawPriestPlacementGeometry::Inspect(PriestPlacementQuery query,const std::function<bool(SquidSearchArea)>& scan) const {
    if (!scan) throw std::invalid_argument("사제 모양 충돌 탐색 경계 누락");
    auto decoder=Decode(query);hooks_.beginRegions(query);
    // 패턴 없는 사제는 최대 한 칸이지만 원본 Valid/Advance 순서를 그대로 사용한다.
    while (decoder.Valid()) {
        const auto& type=types_[query.type];const float x=decoder.X(),y=decoder.Y();const auto area=CollisionArea(x,y,type.footX,type.footY);
        if (hooks_.beginShape) hooks_.beginShape(query,decoder.Frame(),static_cast<float>(Truncate(x)),static_cast<float>(Truncate(y)));
        if (!scan(area)) return false;
        if (!hooks_.endShape(query,decoder.Frame(),static_cast<float>(Truncate(x)),static_cast<float>(Truncate(y)))) return false;
        decoder.Advance();
    }
    return hooks_.finishRegions(query);
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
