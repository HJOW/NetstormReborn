// 원본의 표시 차단 전역과 내부 거부값, 마지막 방향 관계를 각각 같은 시점에 처리한다.
#include "o/RawCanonPlacementRelations.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 디컴파일에 생략된 현재 표면 좌표 보정과 원본 지도 크기다.
constexpr float kBias=0.9999F;
constexpr int kSide=256;
// 0 방향 CRT 절삭을 재현하며 비정상 정수 변환은 진단한다.
int Coordinate(float value) {
    const double result=std::trunc(static_cast<double>(value)+static_cast<double>(kBias));
    if (!std::isfinite(result) || result<std::numeric_limits<int>::min() || result>std::numeric_limits<int>::max())
        throw std::out_of_range("최종 표면 좌표 절삭 오류");
    return static_cast<int>(result);
}
}
// 표면 지도/타입과 기존 관계 helper가 같은 풀을 사용하는지 확인한다.
RawCanonPlacementRelations::RawCanonPlacementRelations(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const std::uint16_t> surfaces,std::span<const CanonPlacementIslandRegion> islands,
    const CanonPlacementRelationsState& state,PriestPlacementState& placement,const RawCanonPlacementPermission& permission)
    :pool_(pool),types_(types),surfaces_(surfaces),islands_(islands),state_(state),placement_(placement),permission_(permission) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || surfaces.size()!=65536 || &pool!=&permission.Pool())
        throw std::invalid_argument("최종 배치 관계 자료/풀 오류");
}
// 지도 범위 밖이면 슬롯 0으로 돌려주고 지도 안의 번호 유효성은 별도 검사한다.
Sid RawCanonPlacementRelations::SurfaceAt(float x,float y) const {
    const int px=Coordinate(x),py=Coordinate(y);if (px<0 || py<0 || px>=kSide || py>=kSide) return {};
    return Sid{surfaces_[static_cast<std::size_t>(py)*kSide+static_cast<std::size_t>(px)]};
}
// 원본 isValid는 주소 범위만 검사하므로 free/dead/void 상태를 추가하지 않는다.
bool RawCanonPlacementRelations::Valid(Sid sid) const { return sid.value && sid.value<pool_.Capacity(); }
// 후반 직접 관계는 활성 플래그나 편집기 조건을 보지 않는다.
bool RawCanonPlacementRelations::Alliance(std::uint32_t currentOwner,std::uint32_t owner) const {
    return permission_.Alliance(currentOwner,owner);
}
// 원본은 없는 지역에서 assert한다. 새 코드에서도 임의 허용/0으로 대신하지 않는다.
std::uint32_t RawCanonPlacementRelations::Restricted(std::int32_t region) const {
    const auto count=std::bit_cast<std::int32_t>(state_.islandCount);
    if (region<0 || region>=count || static_cast<std::size_t>(region)>=islands_.size() || !islands_[static_cast<std::size_t>(region)].exists)
        throw std::out_of_range("최종 배치 특수 섬 지역 오류");
    return islands_[static_cast<std::size_t>(region)].restricted;
}
// 지면 누적/권한 교정은 기존 Terrain::Finish가 수행하며 이 함수는 원본 후반만 처리한다.
CanonPlacementRelationResult RawCanonPlacementRelations::Inspect(const CanonPlacementQuery& query,
    CanonPlacementTerrainState& terrain,std::uint32_t priorRejection) const {
    if (query.type>=types_.size()) throw std::out_of_range("최종 배치 타입 오류");
    const auto genus=types_[query.type].flags2;const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto owner=std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(query.owner))));
    CanonPlacementRelationResult result;result.priorRejection=priorRejection;result.foreignRejected=priorRejection;
    if ((genus&0x34000) && !(genus&0x200000)) {
        const Sid surface=SurfaceAt(query.x,query.y);
        if (!Valid(surface)) result.relationRejected=true;
        else {
            const auto current=pool_.Slot(surface)[patch ? 34 : 32];
            if (!(genus&0x4000) || current) result.relationRejected=!permission_.Related(current,owner);
        }
        placement_.blockedRelation=result.relationRejected ? 1U : 0U;
        if (genus&0x4200) placement_.blockedRelation=0;
    }
    if (patch && terrain.groundComplete && (genus&0x200)) {
        const auto region=static_cast<std::int32_t>(std::bit_cast<std::int8_t>(terrain.regions[0]));
        if (std::bit_cast<std::uint32_t>(region)!=terrain.emptyRegion && Restricted(region)) terrain.canPlaceGround=false;
    }
    if (genus&0x200000) { result.allowed=true;return result; }
    const Sid surface=SurfaceAt(query.x,query.y);
    if (!(genus&4) && Valid(surface)) {
        const auto current=pool_.Slot(surface)[patch ? 34 : 32];
        if (current && !Alliance(current,owner) && !(genus&0x20a000)) {
            result.foreignRejected=1;
            if (!patch) result.priorRejection=1;
        }
    }
    result.allowed=(state_.bypassGroundPermission || (terrain.permission && terrain.canPlaceGround)) && !result.relationRejected && !result.foreignRejected;
    return result;
}
// 같은 풀의 지형 훅만 연결한다.
const SidPool& RawCanonPlacementRelations::Pool() const { return pool_; }
// 이전 후보/주변 정책과 공통 상태를 유지하며 최종 반환만 실제 관계 구현으로 결정한다.
CanonPlacementTerrainHooks MakeCanonRelationsTerrainHooks(const SidPool& pool,
    const RawCanonPlacementRelations& relations,CanonPlacementTerrainHooks hooks) {
    if (&pool!=&relations.Pool()) throw std::invalid_argument("최종 배치 관계/지형 SID 풀 불일치");
    hooks.finishRelations=[&relations](const CanonPlacementQuery& query,CanonPlacementTerrainState& terrain) { return relations.Inspect(query,terrain).allowed; };return hooks;
}
}
