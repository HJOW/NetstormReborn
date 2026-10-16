// 원본 finder의 순서·동률 유지·한 방향 동맹과 이웃의 현재 extra를 보존한다.
#include "o/RawFenceNeighbors.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 0052f83c/0052f85c의 실제 여덟 방향이다. 이웃 검색은 짝수 방향만 사용한다.
constexpr std::array<int,8> kFenceDx{0,1,1,1,0,-1,-1,-1},kFenceDy{-1,-1,0,1,1,1,0,-1};
// 비정렬 원본 DWORD를 읽는다.
std::uint32_t FenceDword(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t result{}; // 현재 네 바이트의 little endian 결합 결과다.
    for (std::size_t index=0;index<4;++index) result|=std::uint32_t(raw[offset+index])<<(index*8); // 지정한 폭만 읽는다.
    return result;
}
// 원본 BoardCoord의 정상 보드 내부 조건이다.
bool FenceValid(std::array<float,2> point) { return point[0]>0 && point[0]<256 && point[1]>0 && point[1]<256; }
// 현재 Rect의 양 끝을 별도로 검사하고 새 점만1..255에 제한한다.
void FenceInclude(std::array<float,4>& rect,std::array<float,2> point) {
    point={std::clamp(point[0],1.0f,255.0f),std::clamp(point[1],1.0f,255.0f)};
    if (FenceValid({rect[0],rect[1]})) { rect[0]=std::min(rect[0],point[0]);rect[1]=std::min(rect[1],point[1]); }
    else { rect[0]=point[0];rect[1]=point[1]; }
    if (FenceValid({rect[2],rect[3]})) { rect[2]=std::max(rect[2],point[0]);rect[3]=std::max(rect[3],point[1]); }
    else { rect[2]=point[0];rect[3]=point[1]; }
}
// 원본 유효 CRT 입력에서 signed64 절삭의 낮은32비트를 보존한다.
std::int32_t FenceTruncate(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) throw std::out_of_range("펜스 CRT 절삭 범위 오류");
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
}
// 다른 판본이나 복사한 불완전 동맹 표를 허용하지 않는다.
RawFenceNeighbors::RawFenceNeighbors(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const float> ranges,std::span<const std::uint32_t> alliances):pool_(pool),hash_(hash),types_(types),ranges_(ranges),alliances_(alliances) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || ranges.size()!=188 || alliances.size()!=81)
        throw std::invalid_argument("펜스 이웃의 PATCH 타입/동맹 크기 불일치");
}
// free를 임의 필터링하지 않고 원본처럼 SID1부터 유효한 현재 슬롯을 허용한다.
std::span<const std::uint8_t> RawFenceNeighbors::Slot(Sid sid) const {
    if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("펜스 이웃 SID 범위 오류");
    const auto raw=pool_.Slot(sid);if (raw[10]>=types_.size()) throw std::out_of_range("펜스 이웃 TYPE 범위 오류");return raw;
}
// 캡처 위치와 현재 타입 중심은 원본의 서로 다른 읽기 위치를 유지한다.
std::array<float,2> RawFenceNeighbors::Position(Sid sid) const { const auto raw=Slot(sid);return {std::bit_cast<float>(FenceDword(raw,14)),std::bit_cast<float>(FenceDword(raw,18))}; }
// 004adea0은 각 성분을float에 저장한 뒤 거리 helper에 전달한다.
std::array<float,2> RawFenceNeighbors::Center(Sid sid) const {
    const auto point=Position(sid);const auto& type=types_[Slot(sid)[10]];return {static_cast<float>(static_cast<double>(point[0])-type.footX*0.5),static_cast<float>(static_cast<double>(point[1])-type.footY*0.5)};
}
// 현재 TYPE 사거리를 매번 읽는다.
std::int32_t RawFenceNeighbors::Range(Sid sid) const { return FenceTruncate(ranges_[Slot(sid)[10]]); }
// float Rect 구성은 타입 발자국을 뺀 지점에 원본 정수 사거리의 방향 벡터를 더한다.
std::array<float,4> RawFenceNeighbors::SearchBounds(Sid sid,std::uint32_t direction) const {
    if (direction>=8) throw std::out_of_range("펜스 방향 범위 오류");
    const auto point=Position(sid);const auto& type=types_[Slot(sid)[10]];const auto range=Range(sid); // 현재 검색을 만드는 원문 입력이다.
    const auto dx=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(range)*static_cast<std::uint32_t>(kFenceDx[direction]));
    const auto dy=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(range)*static_cast<std::uint32_t>(kFenceDy[direction])); // signed 곱셈의 낮은DWORD를 보존한다.
    std::array<float,4> bounds{point[0],point[1],point[0],point[1]}; // 원본의 동일 두 끝점으로 시작한다.
    FenceInclude(bounds,{static_cast<float>(static_cast<double>(point[0])-static_cast<float>(type.footX-1)+static_cast<float>(dx)),
        static_cast<float>(static_cast<double>(point[1])-static_cast<float>(type.footY-1)+static_cast<float>(dy))});return bounds;
}
// 같은 거리의 첫 후보를 유지한다. 동맹 검사는 가장 가까운 후보를 고른 뒤에만 수행한다.
Sid RawFenceNeighbors::Select(Sid sid,std::span<const Sid> candidates,bool allied) const {
    const auto position=Position(sid);Sid result{}; // 정렬 축 비교는 원문 진입 시 Pop 좌표를 사용한다.
    auto best=static_cast<float>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(Range(sid))*2U)); // 원본 최대 거리는 정수 사거리의 두 배다.
    for (const auto candidate:candidates) { // finder가 준 순서와 중복을 그대로 소비한다.
        const auto raw=Slot(candidate),source=Slot(sid);const auto point=Position(candidate);
        if (raw[10]!=source[10] || candidate==sid || (raw[11]&2U) || (raw[40]&9U) || (point[0]!=position[0] && point[1]!=position[1])) continue;
        const auto a=Center(sid),b=Center(candidate);const auto dx=static_cast<double>(b[0])-a[0],dy=static_cast<double>(b[1])-a[1]; // 실제 float 중심에서 x87 거리 계산으로 확장한다.
        const auto distance=std::sqrt(dx*dx+dy*dy);if (best<=distance) continue;best=static_cast<float>(distance);result=candidate;
    }
    if (result.value && allied) {
        const auto owner=Slot(sid)[34],other=Slot(result)[34]; // 최근접 선택 뒤 현재 원문 소유자를 읽는다.
        if (owner>=9 || other>=9) throw std::out_of_range("펜스 동맹 소유자 범위 오류");
        if (owner!=other && alliances_[other*9+owner]==0) return {};
    }
    return result;
}
// flags1의 실제 일반 finder가 선택한 후보만 거리 선택기에 전달한다.
Sid RawFenceNeighbors::Neighbor(Sid sid,std::uint32_t direction,bool allied) const {
    const auto bounds=SearchBounds(sid,direction);RawSquidFinder finder(pool_,hash_,types_);std::vector<Sid> candidates; // 읽기 전용 탐색 중 같은 후보 순서만 보존한다.
    for (auto candidate=finder.Begin({FenceTruncate(bounds[0]),FenceTruncate(bounds[1]),FenceTruncate(bounds[2]),FenceTruncate(bounds[3])},1);candidate.value;candidate=finder.Next()) candidates.push_back(candidate); // 표면을 건너뛰고 현재 해시의 순서대로 수집한다.
    return Select(sid,candidates,allied);
}
// 수직이면 Y차, 그 외에는 X차의 절댓값을 절삭한 뒤 원본float 사거리와 비교한다.
bool RawFenceNeighbors::WithinRange(Sid sid,std::array<float,4> endpoints) const {
    const auto distance=endpoints[0]==endpoints[2] ? std::abs(static_cast<double>(endpoints[1])-endpoints[3]) : std::abs(static_cast<double>(endpoints[0])-endpoints[2]);
    return static_cast<float>(FenceTruncate(distance))<=ranges_[Slot(sid)[10]];
}
// 로컬/적대의 다른 가까운 후보로 대체하지 않고 각 실제 동맹 이웃을 한 번씩 센다.
std::uint32_t RawFenceNeighbors::Connections(Sid sid) const {
    if (Slot(sid)[40]&9U) return 0;
    const auto position=Position(sid);std::uint32_t result{}; // 실제 원문 네 방향의 누계다.
    for (std::uint32_t direction=0;direction<8;direction+=2) { // 원본은 방향0/2/4/6을 순서대로 검사한다.
        const auto neighbor=Neighbor(sid,direction,true);if (!neighbor.value) continue;const auto point=Position(neighbor);
        if (WithinRange(sid,{position[0],position[1],point[0],point[1]}) && !(Slot(neighbor)[40]&1U)) ++result;
    }
    return result;
}
}
