// 사제 전용 요청을 값으로 변환해 공통 미리보기의 순서와 효과를 사용한다.
#include "o/RawPriestPlacementPreview.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 기존 사제 콜백에 원래 여섯 값 인자를 전달한다.
PriestPlacementQuery PriestQuery(const CanonPlacementQuery& query) {
    return {query.type,query.x,query.y,query.flags,query.owner,query.mode};
}
// 비어 있는 기존 경계는 람다로 가리지 않고 생성 시 진단한다.
CanonPlacementPreviewHooks AdaptHooks(PriestPlacementPreviewHooks hooks) {
    if (!hooks.bounds || !hooks.inspectCollisions) throw std::invalid_argument("사제 미리보기 하위 경계 누락");
    return {[bounds=std::move(hooks.bounds)](const CanonPlacementQuery& query) { return bounds(PriestQuery(query)); },
        [collisions=std::move(hooks.inspectCollisions)](const CanonPlacementQuery& query,bool localOwner) { return collisions(PriestQuery(query),localOwner); }};
}
}
// 기존 자료/상태의 참조와 사제 콜백을 공통 계산기에 공급한다.
RawPriestPlacementPreview::RawPriestPlacementPreview(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const std::uint8_t> spots,std::span<const std::uint16_t> surfaceMap,
    PriestPlacementPreviewState& state,PriestPlacementPreviewHooks hooks)
    :preview_(pool,types,spots,surfaceMap,state,AdaptHooks(std::move(hooks))) {}
// 사제의 첫 인자는 타입 번호이며 로컬 비교를 다시 수행하지 않는다.
bool RawPriestPlacementPreview::Inspect(PriestPlacementQuery query,bool localOwner) const {
    return preview_.Inspect({query.type,query.type,query.x,query.y,query.flags,query.owner,query.mode},localOwner);
}
// 기존 연결 검사에 실제 풀 참조를 반환한다.
const SidPool& RawPriestPlacementPreview::Pool() const { return preview_.Pool(); }
// 사제 접두와 같은 풀에서만 연결한다.
PriestPlacementHooks MakePriestPlacementPreviewHooks(const SidPool& pool,const RawPriestPlacementPreview& preview,PriestPlacementHooks hooks) {
    if (&pool!=&preview.Pool()) throw std::invalid_argument("사제 미리보기/배치의 SID 풀이 다릅니다");
    hooks.inspectGeometry=[&preview](const PriestPlacementQuery& query,bool localOwner) { return preview.Inspect(query,localOwner); };return hooks;
}
}
