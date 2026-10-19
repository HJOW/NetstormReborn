// 반경의 정수화·float 저장·경계 포함과 현재 목록 헤더 재조회를 원본대로 수행한다.
#include "o/RawBombTargets.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kSnapBias=0.9999899864196777F; // 실제502310의 원본 float 상수다. 다른 탐색기의0.9999와 구별한다.
constexpr double kSigned64Limit=9223372036854775808.0; // 원본_ftol의 유효 signed64 상한이다.
// 실제 선택한 파생 몸체 누락을 무동작 성공으로 바꾸지 않는다.
template<class F> const F& Need(const F& hook) {
    if (!hook) throw std::logic_error("Bomb 대상 실제 하위 미연결");
    return hook;
}
// 원본signed64 절삭의 하위DWORD를 signed32로 해석한다. 비유한/범위 밖의하위값은0이다.
std::int32_t Truncate(double value) {
    if (!(value>=-kSigned64Limit && value<kSigned64Limit)) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 원본41d770/41d7a0는 절삭된signed32를 float에 다시 저장한다.
float Snap(float value,bool upper) {
    return static_cast<float>(Truncate(static_cast<double>(value)+(upper ? static_cast<double>(kSnapBias) : 0.0)));
}
// 부모 좌표는 스냅 뒤에0.5를 빼고 원본41d0d0의1..255로 제한한다.
float Center(float value) { return std::clamp(Snap(value,true)-0.5F,1.0F,255.0F); }
}
// 실제10.78의 동일 현재 풀/TYPE/해시만 받는다.
RawBombTargets::RawBombTargets(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const float> ranges,BombTargetState& state,BombTargetHooks hooks)
    :pool_(pool),hash_(hash),types_(types),ranges_(ranges),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || ranges.size()!=types.size()) throw std::invalid_argument("Bomb 대상 판본/TYPE 오류");
}
// 물리 필드의 원본 폭만 읽고 상태 필터를 추가하지 않는다.
std::uint32_t RawBombTargets::Read(Sid sid,std::size_t offset,std::size_t width) const {
    const auto raw=pool_.Slot(sid);std::uint32_t value{};
    // 지정한 폭을 little endian으로 조합한다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[offset+i])<<(8*i);
    return value;
}
// 효과 뒤 같은 후보의 종류가 바뀔 수 있으므로 현재 번호를 다시 확인한다.
const RiftTypeRecord& RawBombTargets::Type(Sid sid) const {
    const auto kind=Read(sid,10,1);
    if (kind>=types_.size()) throw std::out_of_range("Bomb 대상 현재TYPE 범위");
    return types_[kind];
}
// 현재TYPE의실제반경을 정수화하며 caller의가상분배를 대체하지 않는다.
std::int32_t RawBombTargets::DefaultRadius(Sid parent) const {
    const auto kind=Read(parent,10,1);
    if (kind>=ranges_.size()) throw std::out_of_range("Bomb 반경 현재TYPE 범위");
    return Truncate(ranges_[kind]);
}
// 중심/반경은 순회 전에 캡처하고 후보와 출력 헤더는 원본 위치에서 현재 값을 읽는다.
BombTargetGeometry RawBombTargets::Find(Sid parent,BombTargetList& output) const {
    if (!state_.boss) Need(hooks_.assertion)(0x130);
    BombTargetGeometry geometry;
    geometry.center={Center(std::bit_cast<float>(Read(parent,14))),Center(std::bit_cast<float>(Read(parent,18)))};
    geometry.radius=Need(hooks_.radius)(parent,Read(parent,0));
    const float radius=static_cast<float>(geometry.radius); // 원본은 첫 upperX 계산 뒤부터 저장된float 반경을 읽는다.
    geometry.area={static_cast<float>(static_cast<double>(geometry.center[0])-radius),static_cast<float>(static_cast<double>(geometry.center[1])-radius),
        static_cast<float>(static_cast<double>(geometry.radius)+geometry.center[0]),static_cast<float>(static_cast<double>(radius)+geometry.center[1])};
    const SquidSearchArea area{Truncate(Snap(geometry.area[0],false)),Truncate(Snap(geometry.area[1],false)),Truncate(Snap(geometry.area[2],true)),Truncate(Snap(geometry.area[3],true))};
    RawSquidFinder finder(pool_,hash_,types_);
    // 실시간 머리/next·발자국·buried는 원본과 같은 네 단계 탐색기가 처리한다.
    for (Sid candidate=finder.Begin(area);candidate.value;candidate=finder.Next()) {
        if (candidate.value>=5 && candidate.value<15000) continue;
        const auto& type=Type(candidate);
        const float allowance=static_cast<float>(static_cast<double>(type.footX)*0.5+radius);
        if ((Read(candidate,40,1)&1U) || (Read(candidate,11,1)&2U)) continue;
        const float x=static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(candidate,14)))-type.footX*0.5);
        const float y=static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(candidate,18)))-type.footY*0.5);
        const double dx=static_cast<double>(geometry.center[0])-x,dy=static_cast<double>(geometry.center[1])-y;
        if (!(std::sqrt(dx*dx+dy*dy)<=static_cast<double>(allowance))) continue;
        if (!Need(hooks_.eligible)(parent,Read(parent,0),candidate)) continue;
        if (output.count<output.capacity) {
            if (output.count<0 || static_cast<std::size_t>(output.count)>=output.storage.size()) throw std::out_of_range("Bomb 대상 실제 출력 버퍼 범위");
            output.storage[static_cast<std::size_t>(output.count++)]=candidate.value;
        }
    }
    return geometry;
}
// 같은 월드의 물리 저장소인지 확인한다.
const SidPool& RawBombTargets::Pool() const { return pool_; }
}
