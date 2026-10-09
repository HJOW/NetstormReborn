// 원본 0049bb1a 이후 / CD 004457f7 이후의 일반 사제 지형 효과다.
#include "o/RawPriestPlacementTerrain.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 지역 배열의 열 간격과 후보 쓰기의 제한 폭이다.
constexpr int kPitch=12,kPaintLimit=8;
// 부호 있는 CRT 절삭을 안전한 정수 범위에서 수행한다.
int Truncate(float value) {
    const double result=std::trunc(static_cast<double>(value));
    if (!std::isfinite(result) || result<std::numeric_limits<int>::min() || result>std::numeric_limits<int>::max())
        throw std::out_of_range("사제 지형 좌표 절삭 오류");
    return static_cast<int>(result);
}
// 비정렬 raw DWORD를 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t result=0;
    // 지정한 폭의 낮은 바이트부터 합친다.
    for (std::size_t i=0;i<width;++i) result|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return result;
}
// 원본 movsx BYTE와 DWORD 비교의 비트값을 보존한다.
std::uint32_t SignedByte(std::uint8_t value) { return std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int8_t>(value))); }
}
// 원본의 타입 표와 독립 지역 SID 지도의 물리 크기를 확인한다.
RawPriestPlacementTerrain::RawPriestPlacementTerrain(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const PriestPlainCanonType> frames,std::span<const std::uint16_t> islands,PriestPlacementTerrainState& state)
    :pool_(pool),types_(types),frames_(frames),islands_(islands),state_(state) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || frames.size()!=types.size() || islands.size()!=65536)
        throw std::invalid_argument("사제 지형 자료 크기 오류");
}
// 일반 사제는 처음부터 permission=true다. bridge 및 패치의 특수 지역 helper 조합은 별도 후속 범위다.
const RiftTypeRecord& RawPriestPlacementTerrain::Placing(const PriestPlacementQuery& query) const {
    if (query.type>=types_.size()) throw std::out_of_range("사제 지형 타입 오류");
    const auto& type=types_[query.type];
    if (!(type.flags2&0x200000) || (type.flags2&4) || (pool_.Edition()==OriginalEdition::Patch1078 && (type.flags2&0x200)))
        throw std::invalid_argument("일반 사제 지형 타입 계약 오류");
    if (type.footX<1 || type.footX>kPitch || type.footY<1 || type.footY>kPitch)
        throw std::out_of_range("사제 지형 발자국 배열 범위 오류");
    return type;
}
// 동일 순회의 타입과 모양 준비 상태를 확인한다.
void RawPriestPlacementTerrain::Require(const PriestPlacementQuery& query,bool shape) const {
    if (!active_ || query.type!=activeType_ || (shape && !shape_)) throw std::logic_error("사제 지형 순회 순서 오류");
}
// 유효 모양이 없으면 원본처럼 이전 지역 배열을 지우지 않는다.
void RawPriestPlacementTerrain::Begin(const PriestPlacementQuery& query) {
    Placing(query);active_=true;shape_=false;activeType_=query.type;
    state_.bridgeOverlap=false;state_.noIslandOnly=true;state_.groundComplete=true;state_.permission=true;state_.canPlaceGround=false;
}
// snapped 좌표를 기준으로 배열의 왼쪽 위 원점을 만들며 지면 종류는 후보 순회 동안 고정한다.
void RawPriestPlacementTerrain::BeginShape(const PriestPlacementQuery& query,float x,float y) {
    Require(query,false);const auto& type=Placing(query);
    originX_=static_cast<std::int64_t>(Truncate(x))-type.footX+1;originY_=static_cast<std::int64_t>(Truncate(y))-type.footY+1;
    groundMask_=type.flags1&6;state_.regions.fill(static_cast<std::uint8_t>(state_.emptyRegion));shape_=true;
}
// 지도 번호는 unsigned WORD다. 대상이 다리면 raw +8을 읽지 않고 sentinel을 반환한다.
std::uint32_t RawPriestPlacementTerrain::RegionAt(int x,int y) const {
    if (x<0 || y<0 || x>=256 || y>=256) return 127;
    const Sid sid{islands_[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)]};if (!sid.value) return 127;
    const auto raw=pool_.Slot(sid);if (raw[10]>=types_.size()) throw std::out_of_range("지역 지도 타입 오류");
    const auto genus=types_[raw[10]].flags2;
    if (!(genus&0x1000006)) {
        if (pool_.Edition()==OriginalEdition::Patch1078) throw std::invalid_argument("지역 지도 자산 genus 오류");
        return state_.emptyRegion;
    }
    return genus&4 ? state_.emptyRegion : Read(raw,8,2);
}
// 후보가 충돌에서 무시되었더라도 bridge와 island의 효과를 순서대로 적용한다.
void RawPriestPlacementTerrain::Candidate(const PriestPlacementQuery& query,Sid candidate) {
    Require(query,true);const auto& placing=Placing(query);const auto raw=pool_.Slot(candidate);
    if (raw[10]>=types_.size()) throw std::out_of_range("사제 지형 후보 타입 오류");
    const auto& other=types_[raw[10]];const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if ((other.flags2&4) && (placing.flags2&0x208100) && !(raw[patch ? 40 : 35]&1)) {
        if (placing.footX!=placing.footY) throw std::invalid_argument("다리 위 사제 발자국 폭/높이 불일치");
        state_.bridgeOverlap=true;
    }
    if (!(other.flags2&2)) return;
    if (raw[10]!=state_.noIslandType) state_.noIslandOnly=false;
    const auto frame=patch ? std::bit_cast<std::int32_t>(Read(raw,36)) : static_cast<std::int32_t>(raw[34]);
    const auto codes=frames_[raw[10]].frames.Codes();
    if (frame<0 || static_cast<std::size_t>(frame)>=codes.size()) throw std::out_of_range("사제 지형 후보 프레임 오류");
    const std::uint32_t surface=codes[static_cast<std::size_t>(frame)].flags&4 ? 4U : 2U;
    if (!(groundMask_&surface)) return;
    const int x=Truncate(std::bit_cast<float>(Read(raw,14))),y=Truncate(std::bit_cast<float>(Read(raw,18)));
    const auto region=static_cast<std::uint8_t>(RegionAt(x,y));
    // 배열 밖의 반복은 쓰기 효과가 없으므로 8×8 교집합만 순서대로 방문한다.
    const auto left=std::max<std::int64_t>(0,static_cast<std::int64_t>(x)-other.footX-originX_+1);
    const auto top=std::max<std::int64_t>(0,static_cast<std::int64_t>(y)-other.footY-originY_+1);
    const auto right=std::min<std::int64_t>(kPaintLimit-1,static_cast<std::int64_t>(x)-originX_);
    const auto bottom=std::min<std::int64_t>(kPaintLimit-1,static_cast<std::int64_t>(y)-originY_);
    // 원본 x 바깥/y 안쪽 순서와 열 간격 12를 보존한다.
    for (auto cx=left;cx<=right;++cx) {
        // 해당 열의 지역 BYTE만 덮어쓴다.
        for (auto cy=top;cy<=bottom;++cy) state_.regions[static_cast<std::size_t>(cx*kPitch+cy)]=region;
    }
}
// 한 모양에서 false가 된 지면 완성 여부는 후속 모양에서 true로 되돌리지 않는다.
bool RawPriestPlacementTerrain::EndShape(const PriestPlacementQuery& query) {
    Require(query,true);const auto& type=Placing(query);const auto first=state_.regions[0];
    if (SignedByte(first)==state_.emptyRegion) state_.groundComplete=false;
    else {
        // 배치 발자국 전체를 검사하며 후보의 8칸 쓰기 제한과 구분한다.
        for (int x=0;x<type.footX;++x) {
            // 각 열의 BYTE 지역 번호가 첫 번호와 같은지 검사한다.
            for (int y=0;y<type.footY;++y) if (state_.regions[static_cast<std::size_t>(x*kPitch+y)]!=first) state_.groundComplete=false;
        }
    }
    shape_=false;return true;
}
// 일반 사제의 마지막 genus 분기는 지면/permission 결과에 관계없이 true를 반환한다.
bool RawPriestPlacementTerrain::Finish(const PriestPlacementQuery& query) {
    Require(query,false);const auto& type=Placing(query);
    state_.canPlaceGround=((type.flags1&0x400) && state_.noIslandOnly) || state_.groundComplete || state_.bridgeOverlap;
    if ((type.flags1&0x400) && !state_.noIslandOnly && !state_.groundComplete) state_.permission=false;
    active_=false;return true;
}
// 연결 검증용 풀을 반환한다.
const SidPool& RawPriestPlacementTerrain::Pool() const { return pool_; }
// 모양 시작은 finder/후보 순회 전에 호출되어야 한다.
PriestPlacementGeometryHooks MakePriestTerrainGeometryHooks(const SidPool& pool,RawPriestPlacementTerrain& terrain) {
    if (&pool!=&terrain.Pool()) throw std::invalid_argument("사제 지형/모양 SID 풀 불일치");
    return {[&terrain](const PriestPlacementQuery& q) { terrain.Begin(q); },
        [&terrain](const PriestPlacementQuery& q,int,float,float) { return terrain.EndShape(q); },
        [&terrain](const PriestPlacementQuery& q) { return terrain.Finish(q); },
        [&terrain](const PriestPlacementQuery& q,int,float x,float y) { terrain.BeginShape(q,x,y); }};
}
// 지형 효과만 연결하고 실제 geometry/finder 순회는 유지한다.
PriestPlacementCollisionHooks MakePriestTerrainCollisionHooks(const SidPool& pool,RawPriestPlacementTerrain& terrain,PriestPlacementCollisionHooks hooks) {
    if (&pool!=&terrain.Pool()) throw std::invalid_argument("사제 지형/충돌 SID 풀 불일치");
    hooks.inspectCandidateTerrain=[&terrain](const PriestPlacementQuery& q,Sid sid) { terrain.Candidate(q,sid); };return hooks;
}
}
