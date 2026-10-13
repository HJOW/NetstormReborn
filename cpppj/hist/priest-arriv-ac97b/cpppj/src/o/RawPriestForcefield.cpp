// 좌표 helper의 절삭과 기본 true 가상 필터의 일반 탐색을 대체 없이 합성한다.
#include "o/RawPriestForcefield.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 두 판본의 공통 raw 좌표/가상 표 위치다.
constexpr std::size_t kX=14,kY=18;
// raw DWORD를 호스트 정렬과 무관하게 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t result=0;
    // 낮은 바이트부터 원본 32비트 값을 조합한다.
    for (std::size_t i=0;i<4;++i) result|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return result;
}
// float→signed int→float→signed int의 정상 범위에서는 처음 0방향 절삭한 값이 유지된다.
int Cell(std::uint32_t bits) {
    const double value=std::bit_cast<float>(bits);
    if (!std::isfinite(value) || value<std::numeric_limits<std::int32_t>::min() || value>std::numeric_limits<std::int32_t>::max())
        throw std::out_of_range("사제 보호막 조회 좌표 절삭 범위 오류");
    return static_cast<int>(value);
}
}
// 판본별 타입 수를 확인한다. 공간 자료를 미복원 표시/Pop 몸체와 연결하지 않는다.
RawPriestForcefield::RawPriestForcefield(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,PriestForcefieldState& state):
    pool_(pool),hash_(hash),types_(types.begin(),types.end()),state_(state) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("사제 보호막 조회 타입 판본 오류");
}
// 매 조회마다 독립 커서를 만든다. 공간 순서는 level→y→x→버킷의 next 순서다.
Sid RawPriestForcefield::Find(Sid priest) const {
    if (priest.value<5 || priest.value>=pool_.Capacity()) throw std::out_of_range("사제 보호막 조회 SID 오류");
    const auto raw=pool_.Slot(priest);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (raw[10]!=kPriestType || Read(raw,0)!=(patch ? kPatchPriestVtable : kCdPriestVtable) || (raw[11]&9))
        throw std::invalid_argument("사제 보호막 조회 raw 상태 오류");
    const int x=Cell(Read(raw,kX)),y=Cell(Read(raw,kY));const auto ownerOffset=patch ? 34U : 32U;
    RawSquidFinder finder(pool_,hash_,types_);
    // 원본 기본 true 필터와 flags=0을 사용한다. 후보 state/free/dead/void를 추가로 제외하지 않는다.
    for (Sid current=finder.Begin({x,y,x,y});current.value;current=finder.Next()) {
        const auto candidate=pool_.Slot(current);
        if (candidate[10]==state_.type && candidate[ownerOffset]==pool_.Slot(priest)[ownerOffset]) return current;
    }
    return {};
}
// 삭제 준비 훅의 풀 연결을 읽기 전용으로 확인한다.
const SidPool& RawPriestForcefield::Pool() const { return pool_; }
// lookup과 생성이 같은 현재 타입 전역을 공유한다.
std::uint32_t RawPriestForcefield::Type() const { return state_.type; }
// 보호막 lookup만 교체해 다른 파생 효과를 암묵적으로 생략하지 않는다.
PriestPreDestroyHooks MakePriestForcefieldHooks(const SidPool& pool,const RawPriestForcefield& lookup,PriestPreDestroyHooks hooks) {
    if (&pool!=&lookup.Pool()) throw std::invalid_argument("사제 보호막 조회/삭제 준비 풀이 다릅니다");
    hooks.findForcefield=[&lookup](Sid sid) { return lookup.Find(sid); };
    return hooks;
}
}
