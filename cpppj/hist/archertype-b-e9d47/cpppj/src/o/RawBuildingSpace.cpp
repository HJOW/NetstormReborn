// 실제 옆면 부착의 조회/쓰기 폭을 보존한다. free/dead/void 상태로 표면 후보를 여과하지 않는다.
#include "o/RawBuildingSpace.h"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 raw 타입 및 WORD 필드 위치다.
constexpr std::size_t kType=10,kWord=12;
// 원본 CRT 절삭의 유효 정수 범위만 허용하며 음수 소수를 floor로 바꾸지 않는다.
int Truncate(float value) {
    const double truncated=std::trunc(static_cast<double>(value));
    if (!std::isfinite(truncated) || truncated<std::numeric_limits<int>::min() || truncated>std::numeric_limits<int>::max())
        throw std::out_of_range("건물 지면 좌표 절삭 범위 오류");
    return static_cast<int>(truncated);
}
}
// 타입 전역과 실제 전체 표를 확인한다. 같은 해시의 0단계가 원본 표면 지도다.
RawBuildingSpace::RawBuildingSpace(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,BuildingSpaceState& state)
    :pool_(pool),hash_(hash),types_(types),state_(state) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || state.islandType>=types.size() || state.bridgeType>=types.size())
        throw std::invalid_argument("건물 공간 타입 표 오류");
}
// 지도 밖은 원본처럼 0이며 조회된 WORD의 상태는 검사하지 않는다.
Sid RawBuildingSpace::Surface(int x,int y) const {
    if (x<0 || y<0 || x>=kWorldCells || y>=kWorldCells) return {};
    return Sid{hash_.Cell(0,x,y)};
}
// 원본 boolean 섬 확인은 전달 좌표의 지면 타입만 비교한다. Pop은 건물 자체의 좌표를 전달한다.
bool RawBuildingSpace::HasIsland(float x,float y) const {
    const auto sid=Surface(Truncate(x),Truncate(y));
    return sid.value && pool_.Slot(sid)[kType]==state_.islandType;
}
// 실제 옆면 SID 범위 확인 뒤 사용하는 좁은 raw 쓰기 경계다.
std::span<std::uint8_t> RawBuildingSpace::Bytes(Sid sid) const {
    return std::span(pool_.bytes_).subspan(pool_.Offset(sid),pool_.layout_.stride);
}
// side 판정과 spot 쓰기는 호출자가 처리하고, 이 몸체는 그 직후 현재 후보를 다시 조회한다.
void RawBuildingSpace::PopSide(int x,int y,float buildingX,float buildingY) const {
    const auto sid=Surface(x,y);
    if (!sid.value || sid.value>=pool_.Capacity()) return;
    const auto old=pool_.Slot(sid);
    if (old[kType]>=types_.size()) throw std::out_of_range("건물 옆면 표면 타입 오류");
    if (!(types_[old[kType]].flags1&TypeFlag1::kSurface)) return;
    const bool island=HasIsland(buildingX,buildingY);
    auto raw=Bytes(sid);
    // 섬이면 WORD의 bit2만 지우고 bit1을 켠다. 그 외에는 BYTE의 두 비트만 지운다.
    raw[kWord]=static_cast<std::uint8_t>(island ? (raw[kWord]&0xfb)|2 : raw[kWord]&0xf9);
    NotifySurface(sid);
}
// 원본 Unpop은 표면 플래그와 좌표 섬 확인을 하지 않으며 높은 WORD 바이트를 유지한다.
void RawBuildingSpace::UnpopSide(int x,int y) const {
    const auto sid=Surface(x,y);
    if (!sid.value || sid.value>=pool_.Capacity()) return;
    Bytes(sid)[kWord]&=0xf9;
    NotifySurface(sid);
}
// 전역 debug가 꺼지면 다리 수명 nibble이 잘못돼도 원본처럼 쓰기/추가 효과가 없다.
void RawBuildingSpace::NotifySurface(Sid sid) const {
    // CD는 raw 포인터 산술만 반환하며 쓰기나 debug 진단이 없다. 호출자가 반환값을 버린다.
    if (pool_.Edition()==OriginalEdition::Cd1072) return;
    const auto raw=pool_.Slot(sid);
    if (raw[kType]==state_.bridgeType && !(raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&8) &&
        state_.debug && ((raw[kWord]>>3)&15)>7) throw std::logic_error("다리 수명 진단: num >= 0 && num <= 7");
}
// 수명과 표면 지도 identity를 크기가 같은 다른 저장소로 대체할 수 없다.
void RawBuildingSpace::ValidateSpace(const SidPool& pool,const SquidHash& hash) const {
    if (&pool!=&pool_ || &hash!=&hash_) throw std::invalid_argument("건물 공간 풀/해시 불일치");
}
}
