// 단일 칸의 실제 finder 순서와 도착 뒤 현재 raw/전역 읽기를 보존한다.
#include "o/RawManaBoltArrival.h"
#include <bit>
#include <cmath>
#include <stdexcept>
namespace netstorm::o {
namespace {
// 비정렬 float DWORD를 little endian으로 읽는다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{};
    // 비정렬 좌표의 네 바이트만 낮은 순서부터 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(i*8);
    return std::bit_cast<float>(value);
}
// 실제41d770의 _ftol 하위DWORD를 signed로 재해석한다.
std::int32_t Cell(float value) { if (!std::isfinite(value) || std::fabs(static_cast<double>(value))>=9223372036854775808.0) return 0;return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value)))); }
// 도달한 실제 효과가 미연결이면 생략하지 않는다.
template<class F> const F& Need(const F& f) { if (!f) throw std::logic_error("실제 마나 볼트 도착 경계 미연결");return f; }
}
// 현재 PATCH 필드만 사용하며 CD의 다른 raw 폭은 별도 복원이 필요하다.
RawManaBoltArrival::RawManaBoltArrival(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,ManaBoltArrivalState& state,ManaBoltArrivalHooks hooks)
    :pool_(pool),hash_(hash),types_(types),state_(state),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("마나 볼트 도착 판본/TYPE 오류"); }
// 추가 dead/void/HP 필터를 넣지 않는다. 현재 후보 extra와 현재 두 owner만 비교한다.
Sid RawManaBoltArrival::Target(Sid bolt) const {
    const auto raw=pool_.Slot(bolt);const auto x=Cell(Coordinate(raw,14)),y=Cell(Coordinate(raw,18));RawSquidFinder finder(pool_,hash_,types_);
    // 실제4202f0(flags1)의 일반 탐색 순서다.
    for (auto sid=finder.Begin({x,y,x,y},1);sid.value;sid=finder.Next()) { const auto candidate=pool_.Slot(sid);if ((candidate[40]&1) && !(candidate[40]&8) && candidate[34]==pool_.Slot(bolt)[34]) return sid; }return {};
}
// 부모 삭제가 경로 자신도 지울 수 있으므로 가상 삭제 뒤에는 현재 객체를 읽지 않는다.
std::uint32_t RawManaBoltArrival::Arrive(Sid bolt) const {
    const auto raw=pool_.Slot(bolt);Need(hooks_.sound)(Coordinate(raw,14),Coordinate(raw,18),"sunCannonRaiseLower5.wav");
    if (Target(bolt).value) { Need(hooks_.removeProcesses)(bolt,state_.regularType);Need(hooks_.removeProcesses)(bolt,state_.sharedRegularType);if (Need(hooks_.reserveRegular)()) Need(hooks_.addRegular)(bolt,1,state_.delay);return 1000; }
    Need(hooks_.destroy)(bolt,0);return 1001;
}
// 연결 시 같은 풀의 정체성을 확인한다.
const SidPool& RawManaBoltArrival::Pool() const { return pool_; }
}
