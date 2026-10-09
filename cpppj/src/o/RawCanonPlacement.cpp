// 원본 x87 저장 순서와 타입/패턴 인자 구별을 모든 타입에 적용한다.
#include "o/RawCanonPlacement.h"
#include "o/RawCanonPixelShape.h"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 즉시 허용 비트·가로/세로 픽셀 환산·지도 한 변 길이다.
constexpr std::uint32_t kAnywhere=0x02000000;
constexpr float kHorizontalScale=0.0625f,kVerticalScale=0.09090909361839294f,kMapSide=256.0f;
// 실제 CRT가 정의하지 못하는 입력을 C++ 정수 변환 전에 진단한다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || value<std::numeric_limits<std::int32_t>::min() || value>std::numeric_limits<std::int32_t>::max())
        throw std::out_of_range("일반 배치 여백의 정수 범위 오류");
    return static_cast<std::int32_t>(value);
}
// 패치는 가로 차의 정밀도를 유지한다. CD는 가로/세로 두 합을 float에 저장한다.
float Margin(OriginalEdition edition,const PriestPlacementRect& rect) {
    if (!std::isfinite(rect.left) || !std::isfinite(rect.top) || !std::isfinite(rect.right) || !std::isfinite(rect.bottom))
        throw std::invalid_argument("일반 배치 모양의 비유한 좌표");
    double horizontal{};float vertical{};
    if (edition==OriginalEdition::Patch1078) {
        horizontal=(static_cast<double>(rect.right)-rect.left)*kHorizontalScale;
        vertical=static_cast<float>((static_cast<double>(rect.bottom)-rect.top)*kVerticalScale);
    } else {
        vertical=static_cast<float>(static_cast<double>(rect.top)*-kVerticalScale+static_cast<double>(rect.bottom)*kVerticalScale);
        horizontal=static_cast<float>(static_cast<double>(rect.left)*-kHorizontalScale+static_cast<double>(rect.right)*kHorizontalScale);
    }
    return static_cast<float>(Truncate(horizontal>vertical ? horizontal : vertical));
}
}
// 판본별 타입 개수와 필수 모양/후반 판정 경계를 확인한다.
RawCanonPlacement::RawCanonPlacement(const SidPool& pool,std::span<const RiftTypeRecord> types,
    PriestPlacementState& state,CanonPlacementHooks hooks):pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)) {
    const auto count=pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if (types.size()!=count || !hooks_.shape || !hooks_.inspectGeometry) throw std::invalid_argument("일반 배치 자료/하위 경계 누락");
}
// 전역 차단값을 먼저 지우고 현재 허용 조건·캡처한 소유자 비교·모양 여백 순서로 처리한다.
bool RawCanonPlacement::MayPlace(CanonPlacementQuery query) const {
    if (query.type<kFirstAssetTypeNumber || query.type>=types_.size()) throw std::invalid_argument("일반 배치 타입 범위 오류");
    if (!std::isfinite(query.x) || !std::isfinite(query.y)) throw std::invalid_argument("일반 배치의 비유한 지도 좌표");
    state_.blockedRelation=0;
    if (state_.forcePlacement || (types_[query.type].flags2&kAnywhere)) return true;
    const auto byte=static_cast<std::uint8_t>(query.owner);
    const auto owner=byte<128 ? static_cast<std::int32_t>(byte) : static_cast<std::int32_t>(byte)-256;
    const bool localOwner=owner==state_.localPlayer;
    const auto margin=Margin(pool_.Edition(),hooks_.shape(query.type,query.argument,query.flags));
    if (query.x<margin || static_cast<double>(kMapSide)-query.x<margin || query.y<margin || static_cast<double>(kMapSide)-query.y<margin) return false;
    return hooks_.inspectGeometry(query,localOwner);
}
// 같은 풀의 연결 검사에 사용할 실제 참조를 반환한다.
const SidPool& RawCanonPlacement::Pool() const { return pool_; }
// 패턴 전체 getter에 원본 argument/방향을 전달하고 명시 프레임을 켜지 않는다.
CanonPlacementHooks MakeCanonShapePlacementHooks(const SidPool& pool,const RawCanonPixelShape& shape,CanonPlacementHooks hooks) {
    if (&pool!=&shape.Pool()) throw std::invalid_argument("일반 픽셀 모양/배치의 SID 풀이 다릅니다");
    hooks.shape=[&shape](std::uint32_t type,std::uint32_t argument,std::uint32_t direction) { return shape.Measure(type,argument,direction).bounds; };
    return hooks;
}
}
