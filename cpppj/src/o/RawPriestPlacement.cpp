// x87 중간 float 저장 위치와 패치/CD의 서로 다른 여백 계산 순서를 보존한다.
#include "o/RawPriestPlacement.h"
#include "o/RawCanonPlacement.h"
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 판본별 타입 개수와 필요한 두 경계를 확인한다.
RawPriestPlacement::RawPriestPlacement(const SidPool& pool,std::span<const RiftTypeRecord> types,
    PriestPlacementState& state,PriestPlacementHooks hooks):pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)) {
    const auto count=pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if (types.size()!=count || !hooks_.shape || !hooks_.inspectGeometry) throw std::invalid_argument("사제 배치 자료/하위 경계 누락");
}
// 진입 초기화는 즉시 허용·지도 밖 거부에도 적용한다. 모양 조회 뒤 전역을 다시 비교하지 않는다.
bool RawPriestPlacement::MayPlace(PriestPlacementQuery query) const {
    if (query.type<kFirstAssetTypeNumber || query.type>=types_.size() || !(types_[query.type].flags2&0x200000))
        throw std::invalid_argument("사제 배치 타입/genus 범위 오류");
    if (!std::isfinite(query.x) || !std::isfinite(query.y)) throw std::invalid_argument("사제 배치의 비유한 지도 좌표");
    const RawCanonPlacement placement(pool_,types_,state_,{hooks_.shape,
        [this,query](const CanonPlacementQuery&,bool localOwner) { return hooks_.inspectGeometry(query,localOwner); }});
    return placement.MayPlace({query.type,query.type,query.x,query.y,query.flags,query.owner,query.mode});
}
// 연결 판본과 풀의 동일성을 검사한다.
const SidPool& RawPriestPlacement::Pool() const { return pool_; }
// 원본 사제 생성의 여섯 배치 인자를 그대로 전달한다.
PriestSpawnHooks MakePriestPlacementHooks(const SidPool& pool,const RawPriestPlacement& placement,PriestSpawnHooks hooks) {
    if (&pool!=&placement.Pool()) throw std::invalid_argument("사제 배치/생성의 SID 풀이 다릅니다");
    hooks.mayPlace=[&placement](const PriestPlacementQuery& query) { return placement.MayPlace(query); };return hooks;
}
}
