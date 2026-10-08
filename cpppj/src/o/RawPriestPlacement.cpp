// x87 중간 float 저장 위치와 패치/CD의 서로 다른 여백 계산 순서를 보존한다.
#include "o/RawPriestPlacement.h"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 사제 genus와 즉시 허용 플래그다. 일반 자산의 배치 정책은 이 접두의 범위 밖이다.
constexpr std::uint32_t kPriest=0x200000,kAnywhere=0x02000000;
// 원본 모양의 가로 16·세로 11 단위를 지도 좌표로 환산한다. 세로 계수는 원본 float 값이다.
constexpr float kHorizontalScale=0.0625f,kVerticalScale=0.09090909361839294f,kMapSide=256.0f;
// 원본 CRT가 정의하지 못하는 입력을 C++ 정수 변환 전에 진단한다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || value<std::numeric_limits<std::int32_t>::min() || value>std::numeric_limits<std::int32_t>::max())
        throw std::out_of_range("사제 배치 여백의 정수 범위 오류");
    return static_cast<std::int32_t>(value);
}
// 패치는 가로 차를 넓은 정밀도로 유지하고 세로만 float로 저장한다. CD는 두 합을 모두 저장한다.
float Margin(OriginalEdition edition,const PriestPlacementRect& rect) {
    if (!std::isfinite(rect.left) || !std::isfinite(rect.top) || !std::isfinite(rect.right) || !std::isfinite(rect.bottom))
        throw std::invalid_argument("사제 배치 모양의 비유한 좌표");
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
// 판본별 타입 개수와 필요한 두 경계를 확인한다.
RawPriestPlacement::RawPriestPlacement(const SidPool& pool,std::span<const RiftTypeRecord> types,
    PriestPlacementState& state,PriestPlacementHooks hooks):pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)) {
    const auto count=pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if (types.size()!=count || !hooks_.shape || !hooks_.inspectGeometry) throw std::invalid_argument("사제 배치 자료/하위 경계 누락");
}
// 진입 초기화는 즉시 허용·지도 밖 거부에도 적용한다. 모양 조회 뒤 전역을 다시 비교하지 않는다.
bool RawPriestPlacement::MayPlace(PriestPlacementQuery query) const {
    if (query.type<kFirstAssetTypeNumber || query.type>=types_.size() || !(types_[query.type].flags2&kPriest))
        throw std::invalid_argument("사제 배치 타입/genus 범위 오류");
    if (!std::isfinite(query.x) || !std::isfinite(query.y)) throw std::invalid_argument("사제 배치의 비유한 지도 좌표");
    state_.blockedRelation=0;
    if (state_.forcePlacement || (types_[query.type].flags2&kAnywhere)) return true;
    const auto byte=static_cast<std::uint8_t>(query.owner);
    const auto owner=byte<128 ? static_cast<std::int32_t>(byte) : static_cast<std::int32_t>(byte)-256;
    const bool localOwner=owner==state_.localPlayer;
    const auto margin=Margin(pool_.Edition(),hooks_.shape(query.type,query.type,query.flags));
    if (query.x<margin || static_cast<double>(kMapSide)-query.x<margin || query.y<margin || static_cast<double>(kMapSide)-query.y<margin) return false;
    return hooks_.inspectGeometry(query,localOwner);
}
// 연결 판본과 풀의 동일성을 검사한다.
const SidPool& RawPriestPlacement::Pool() const { return pool_; }
// 원본 사제 생성의 여섯 배치 인자를 그대로 전달한다.
PriestSpawnHooks MakePriestPlacementHooks(const SidPool& pool,const RawPriestPlacement& placement,PriestSpawnHooks hooks) {
    if (&pool!=&placement.Pool()) throw std::invalid_argument("사제 배치/생성의 SID 풀이 다릅니다");
    hooks.mayPlace=[&placement](const PriestPlacementQuery& query) { return placement.MayPlace(query); };return hooks;
}
}
