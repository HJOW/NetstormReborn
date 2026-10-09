// 지역/지형 효과와 후보 충돌 판정을 분리하면서 원본의 후보 검사 순서와 표시 효과를 보존한다.
#include "o/RawPriestPlacementCollision.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 공통 요청에서 사제 API의 원래 여섯 값 인자를 복구한다.
PriestPlacementQuery PriestQuery(const CanonPlacementQuery& query) {
    return {query.type,query.x,query.y,query.flags,query.owner,query.mode};
}
// 기존 필수 경계를 진단한 뒤 공통 계산기의 콜백으로 연결한다.
CanonPlacementCollisionHooks AdaptHooks(const PriestPlacementCollisionHooks& hooks) {
    if (!hooks.inspectRegions || !hooks.inspectCandidateTerrain) throw std::invalid_argument("사제 충돌 지역 경계 누락");
    return {[regions=hooks.inspectRegions](const CanonPlacementQuery& query,const std::function<bool(SquidSearchArea)>& scan) { return regions(PriestQuery(query),scan); },
        [terrain=hooks.inspectCandidateTerrain](const CanonPlacementQuery& query,Sid sid) { terrain(PriestQuery(query),sid); }};
}
}
// 미리보기 상태를 직접 공유하여 후보 검사 이전의 표시를 유지한다.
RawPriestPlacementCollision::RawPriestPlacementCollision(const SidPool& pool,const SquidHash& hash,
    std::span<const RiftTypeRecord> types,PriestPlacementCollisionState& state,PriestPlacementPreviewState& preview,
    PriestPlacementCollisionHooks hooks):pool_(pool),hash_(hash),types_(types),hooks_(std::move(hooks)),collision_(pool,hash,types,state,preview,AdaptHooks(hooks_)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || !hooks_.inspectRegions || !hooks_.inspectCandidateTerrain)
        throw std::invalid_argument("사제 충돌 자료/지역 경계 누락");
}
// 사제 전용 경로를 일반 자산 전체 배치 판정으로 오해하지 않도록 진입 계약을 확인한다.
const RiftTypeRecord& RawPriestPlacementCollision::Placing(const PriestPlacementQuery& query) const {
    if (query.type>=types_.size() || !(types_[query.type].flags2&0x200000) || !std::isfinite(query.x) || !std::isfinite(query.y))
        throw std::invalid_argument("사제 충돌 요청 타입/좌표 오류");
    return types_[query.type];
}
// 매 후보의 현재 사제 genus 계약을 확인하고 공통 무시/표시/거부 본문을 사용한다.
bool RawPriestPlacementCollision::InspectCandidate(PriestPlacementQuery query,bool localOwner,Sid candidate) const {
    Placing(query);
    return collision_.InspectCandidate({query.type,query.type,query.x,query.y,query.flags,query.owner,query.mode},localOwner,candidate);
}
// 일반 finder의 기본 가상 필터와 flag 0을 사용하므로 buried만 finder 내부에서 제외한다.
bool RawPriestPlacementCollision::InspectArea(PriestPlacementQuery query,bool localOwner,SquidSearchArea area) const {
    Placing(query);RawSquidFinder finder(pool_,hash_,types_);
    // Next를 호출하기 전에 현재 후보가 거부하면 그대로 종료한다.
    for (Sid sid=finder.Begin(area);sid.value;sid=finder.Next()) {
        if (!InspectCandidate(query,localOwner,sid)) return false;
        hooks_.inspectCandidateTerrain(query,sid);
    }
    return true;
}
// 지역 경계가 거부 이후 scan을 다시 요청하거나 성공을 덮어쓰지 못하도록 거부 상태를 유지한다.
bool RawPriestPlacementCollision::Inspect(PriestPlacementQuery query,bool localOwner) const {
    Placing(query);bool rejected=false;
    const auto allowed=hooks_.inspectRegions(query,[this,query,localOwner,&rejected](SquidSearchArea area) {
        if (rejected) return false;
        rejected=!InspectArea(query,localOwner,area);return !rejected;
    });return allowed && !rejected;
}
// 연결 검사에 사용할 실제 풀을 반환한다.
const SidPool& RawPriestPlacementCollision::Pool() const { return pool_; }
// 기존 미리보기의 모양 범위 조회는 그대로 둔다.
PriestPlacementPreviewHooks MakePriestPlacementCollisionHooks(const SidPool& pool,const RawPriestPlacementCollision& collision,PriestPlacementPreviewHooks hooks) {
    if (&pool!=&collision.Pool()) throw std::invalid_argument("사제 충돌/미리보기의 SID 풀이 다릅니다");
    hooks.inspectCollisions=[&collision](const PriestPlacementQuery& query,bool localOwner) { return collision.Inspect(query,localOwner); };return hooks;
}
}
