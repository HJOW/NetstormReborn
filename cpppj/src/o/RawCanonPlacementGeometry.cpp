// 픽셀 getter와 별개인 배치의 정수 범위/칸별 충돌 범위를 복원한다.
#include "o/RawCanonPlacementGeometry.h"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 00502310 / CD 005019b0의 단정도 올림 보정값이다.
constexpr float kNearCeil=0.99999f;
// 원본 CRT의 범위 밖/비유한 변환을 C++의 정의되지 않은 동작으로 보내지 않는다.
int Truncate(double value) {
    value=std::trunc(value);
    if (!std::isfinite(value) || value<std::numeric_limits<int>::min() || value>std::numeric_limits<int>::max())
        throw std::out_of_range("배치 모양 좌표 절삭 범위 오류");
    return static_cast<int>(value);
}
}
// 모든 타입 번호와 실제 프레임 메타 자료를 같은 판본으로 받는다.
RawCanonPlacementGeometry::RawCanonPlacementGeometry(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const PriestPlainCanonType> frames,PriestPlacementGeometryState& state)
    :pool_(pool),types_(types),frames_(frames),state_(state) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || frames.size()!=types.size())
        throw std::invalid_argument("일반 배치 모양 타입/프레임 자료 누락");
}
// 일반 배치의 첫 인자는 타입 번호와 별개인 패턴 번호이며 비패턴 기본 모드에서는 무시한다.
CanonDecoder RawCanonPlacementGeometry::Decode(CanonTypeQuery query) const {
    if (query.type>=types_.size()) throw std::out_of_range("일반 배치 모양 타입 번호 오류");
    const auto& meta=frames_[query.type];
    return DecodeCanonType(meta.frames,meta.defaultFrame,state_.patternTypes,query,pool_.Edition());
}
// 빈 decoder에서도 원본처럼 첫 현재 좌표와 순회 개수로 미리보기 범위를 계산한다.
SquidSearchArea RawCanonPlacementGeometry::Bounds(CanonTypeQuery query) const {
    const auto decoder=Decode(query);const auto& type=types_[query.type];const auto area=decoder.Bounds(type.footX,type.footY);
    return {area[0],area[1],area[2],area[3]};
}
// 왼쪽 위는 snapped 좌표, 오른쪽 아래는 원래 소수 좌표에 넓은 정밀도로 보정을 더한다. dude의 0 발자국도 계산한다.
SquidSearchArea RawCanonPlacementGeometry::CollisionArea(float x,float y,int footX,int footY) {
    if (footX<0 || footY<0) throw std::out_of_range("배치 모양 발자국 오류");
    const float snappedX=static_cast<float>(Truncate(x)),snappedY=static_cast<float>(Truncate(y));
    return {Truncate(static_cast<double>(snappedX)-(static_cast<std::int64_t>(footX)-1)),
        Truncate(static_cast<double>(snappedY)-(static_cast<std::int64_t>(footY)-1)),
        Truncate(static_cast<double>(x)+kNearCeil),Truncate(static_cast<double>(y)+kNearCeil)};
}
// 현재 발자국은 칸마다 다시 읽고 decoder 생성 때의 첫 프레임/패턴 선택은 유지한다.
bool RawCanonPlacementGeometry::Inspect(CanonTypeQuery query,const std::function<bool(const CanonPlacementCell&)>& scan,
    const CanonPlacementGeometryHooks& hooks) const {
    if (!scan || !hooks.beginRegions || !hooks.endShape || !hooks.finishRegions)
        throw std::invalid_argument("일반 배치 모양 finder/지역 경계 누락");
    auto decoder=Decode(query);hooks.beginRegions(query);
    // 실제 decoder의 빈 칸 건너뛰기와 전체 회전 순서를 유지한다.
    while (decoder.Valid()) {
        const auto& type=types_[query.type];const float x=decoder.X(),y=decoder.Y();
        const auto frame=decoder.Frame();const auto codes=frames_[query.type].frames.Codes();
        // MayPlace는 방향 코드를 요구하지 않는다. 비패턴 기본 프레임의 개수 검사 생략 계약을 유지한다.
        const auto side=frame>=0 && static_cast<std::size_t>(frame)<codes.size() ? codes[static_cast<std::size_t>(frame)].side : std::uint8_t{};
        const CanonPlacementCell cell{frame,decoder.Label(),side,x,y,
            static_cast<float>(Truncate(x)),static_cast<float>(Truncate(y)),CollisionArea(x,y,type.footX,type.footY)};
        if (hooks.beginShape) hooks.beginShape(query,cell);
        if (!scan(cell) || !hooks.endShape(query,cell)) return false;
        decoder.Advance();
    }
    return hooks.finishRegions(query);
}
// 같은 판본/풀에 연결할 때 비교할 원래 풀이다.
const SidPool& RawCanonPlacementGeometry::Pool() const { return pool_; }
}
