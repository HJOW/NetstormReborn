// 지역/지형 효과와 후보 충돌 판정을 분리하면서 원본의 후보 검사 순서와 표시 효과를 보존한다.
#include "o/RawCanonPlacementCollision.h"
#include "o/RawCanonPlacementGeometry.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 공통 raw 타입/float 좌표와 판본별 extra 오프셋이다.
constexpr std::size_t kType=10,kX=14,kY=18,kPatchExtra=40,kCdExtra=35;
// 예측 가능한 SID 범위의 시작과 미리보기 내부 시작 오프셋이다.
constexpr std::uint16_t kFirstClient=5;
constexpr int kSide=12,kInterior=13;
// 비정렬 little endian float를 원본 바이트로부터 읽는다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits=0;
    // 낮은 바이트부터 DWORD를 조립한다.
    for (std::size_t index=0;index<4;++index) bits|=static_cast<std::uint32_t>(raw[offset+index])<<(8*index);
    return std::bit_cast<float>(bits);
}
}
// 조기 분기의 순서와 판본별 bridge/전역 마스크 차이를 그대로 보존한다.
std::uint32_t PlacementIgnoreValue(OriginalEdition edition,const RiftTypeRecord& placing,
    const RiftTypeRecord& candidate,std::uint32_t mode,std::uint32_t ignoredGenus) {
    const auto own=placing.flags2,other=candidate.flags2;
    if (!mode && (own&0x20000)) return 1;
    if (other&0x02000000) return 1;
    if (other&0x120000) return 1;
    if ((own&4) && (other&(edition==OriginalEdition::Patch1078 ? 0x200000u : 0x208000u))) return 1;
    if ((other&2) && !(placing.flags1&0x800)) return 1;
    if ((other&0x30006) && (own&0x208100)) return 1;
    if (ignoredGenus&own) return other&(edition==OriginalEdition::Patch1078 ? 0x218000u : 0x210000u);
    if (!mode && (own&0x30000)) return 1;
    return 0;
}
// 미리보기 상태를 직접 공유하여 후보 검사 이전의 표시를 유지한다.
RawCanonPlacementCollision::RawCanonPlacementCollision(const SidPool& pool,const SquidHash& hash,
    std::span<const RiftTypeRecord> types,CanonPlacementCollisionState& state,CanonPlacementPreviewState& preview,
    CanonPlacementCollisionHooks hooks):pool_(pool),hash_(hash),types_(types),state_(state),preview_(preview),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || !hooks_.inspectRegions || !hooks_.inspectCandidateTerrain)
        throw std::invalid_argument("일반 타입 충돌 자료/지역 경계 누락");
}
// 일반 타입은 genus를 제한하지 않으며 타입 번호와 비유한 좌표를 진단한다.
const RiftTypeRecord& RawCanonPlacementCollision::Placing(const CanonPlacementQuery& query) const {
    if (query.type>=types_.size() || !std::isfinite(query.x) || !std::isfinite(query.y))
        throw std::invalid_argument("일반 타입 충돌 요청 타입/좌표 오류");
    return types_[query.type];
}
// 현재 raw 타입을 먼저 읽고 클라이언트 SID 조건, 현재 전역 무시 조건, 표시, extra/mode 순서로 검사한다.
bool RawCanonPlacementCollision::InspectCandidate(CanonPlacementQuery query,bool localOwner,Sid candidate) const {
    const auto& placing=Placing(query);const auto raw=pool_.Slot(candidate);const auto type=raw[kType];
    if (type>=types_.size()) throw std::out_of_range("일반 타입 충돌 후보 타입 오류");
    const auto& other=types_[type];const float x=Coordinate(raw,kX),y=Coordinate(raw,kY);
    const auto firstServer=pool_.Edition()==OriginalEdition::Patch1078 ? 15000U : 6000U;
    if (state_.clientMode && candidate.value>=kFirstClient && candidate.value<firstServer) return true;
    if (PlacementIgnoreValue(pool_.Edition(),placing,other,query.mode,state_.ignoredGenus)) return true;
    if (localOwner) {
        if (!std::isfinite(x) || !std::isfinite(y) || other.footX<0 || other.footY<0 || other.footX>256 || other.footY>256)
            throw std::out_of_range("일반 타입 충돌 후보 발자국/좌표 오류");
        // 후보의 아래쪽부터 y를 늘리고 각 행에서 x를 늘린다. float 저장 없이 넓은 차를 CRT처럼 절삭한다.
        for (int cy=0;cy<other.footY;++cy) {
            // 빈 가로 축은 원본처럼 좌표 계산/표시를 수행하지 않는다.
            for (int cx=0;cx<other.footX;++cx) {
                const double dx=std::trunc(static_cast<double>(query.x)-(static_cast<double>(x)-cx));
                const double dy=std::trunc(static_cast<double>(query.y)-(static_cast<double>(y)-cy));
                if (dx<0 || dy<0 || dx>=placing.footX || dy>=placing.footY) continue;
                const auto index=dx*kSide+dy+kInterior;
                if (index>=preview_.blocked.size()) throw std::out_of_range("일반 타입 충돌 미리보기 배열 오류");
                preview_.blocked[static_cast<std::size_t>(index)]=1;
            }
        }
    }
    return (raw[pool_.Edition()==OriginalEdition::Patch1078 ? kPatchExtra : kCdExtra]&1) && !query.mode;
}
// 일반 finder의 기본 가상 필터와 flag 0을 사용하므로 buried만 finder 내부에서 제외한다.
bool RawCanonPlacementCollision::InspectArea(CanonPlacementQuery query,bool localOwner,SquidSearchArea area) const {
    Placing(query);RawSquidFinder finder(pool_,hash_,types_);
    // Next를 호출하기 전에 현재 후보가 거부하면 그대로 종료한다.
    for (Sid sid=finder.Begin(area);sid.value;sid=finder.Next()) {
        if (!InspectCandidate(query,localOwner,sid)) return false;
        hooks_.inspectCandidateTerrain(query,sid);
    }
    return true;
}
// 지역 경계가 거부 이후 scan을 다시 요청하거나 성공을 덮어쓰지 못하도록 거부 상태를 유지한다.
bool RawCanonPlacementCollision::Inspect(CanonPlacementQuery query,bool localOwner) const {
    Placing(query);bool rejected=false;
    const auto allowed=hooks_.inspectRegions(query,[this,query,localOwner,&rejected](SquidSearchArea area) {
        if (rejected) return false;
        rejected=!InspectArea(query,localOwner,area);return !rejected;
    });return allowed && !rejected;
}
// 연결 검사에 사용할 실제 풀을 반환한다.
const SidPool& RawCanonPlacementCollision::Pool() const { return pool_; }
// 기존 미리보기의 모양 범위 조회는 그대로 둔다.
CanonPlacementPreviewHooks MakeCanonPlacementCollisionHooks(const SidPool& pool,const RawCanonPlacementCollision& collision,CanonPlacementPreviewHooks hooks) {
    if (&pool!=&collision.Pool()) throw std::invalid_argument("일반 타입 충돌/미리보기의 SID 풀이 다릅니다");
    hooks.inspectCollisions=[&collision](const CanonPlacementQuery& query,bool localOwner) { return collision.Inspect(query,localOwner); };return hooks;
}
// 정책은 검사마다 원래 요청으로 만들며 거부 뒤의 모양/최종 효과는 실제 geometry가 생략한다.
CanonPlacementCollisionHooks MakeCanonGeometryCollisionHooks(const SidPool& pool,const RawCanonPlacementGeometry& geometry,
    std::function<CanonPlacementGeometryHooks(const CanonPlacementQuery&)> regions,CanonPlacementCollisionHooks hooks) {
    if (&pool!=&geometry.Pool() || !regions) throw std::invalid_argument("일반 모양/충돌의 SID 풀 또는 지역 정책 오류");
    hooks.inspectRegions=[&geometry,regions=std::move(regions)](const CanonPlacementQuery& query,const std::function<bool(SquidSearchArea)>& scan) {
        const auto policy=regions(query);
        return geometry.Inspect({query.type,std::bit_cast<std::int32_t>(query.argument),std::bit_cast<std::int32_t>(query.flags),query.x,query.y,false},
            [&scan](const CanonPlacementCell& cell) { return scan(cell.area); },policy);
    };
    return hooks;
}
}
