// 원본 현재 후보/전역 재조회와 finder의 next 선저장을 그대로 사용한다.
#include "o/RawConstructionArrival.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// abstract/needsBuilder 비트와 경로 반환 코드다. 반환 코드의 상위 경로 효과는 이 몸체 밖이다.
constexpr std::uint32_t kNeedsBuilder=0x8000,kWaiting=1002,kFinished=1001;
// 원본 raw 타입/좌표·판본별 abstract 필드 위치다.
constexpr std::size_t kType=10,kX=14,kY=18,kPatchExtra=40,kCdExtra=35;
// 비정렬 좌표 DWORD를 little endian으로 읽는다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits=0;
    // 좌표 비트를 바이트 순서대로 조합한다.
    for (std::size_t byte=0;byte<4;++byte) bits|=static_cast<std::uint32_t>(raw[offset+byte])<<(byte*8);
    return std::bit_cast<float>(bits);
}
// 원본 __ftol의 64비트 절삭 결과 하위 signed DWORD다. 비유한/64비트 범위 밖은 불확정값의 하위0이다.
std::int32_t Truncate(float value) {
    if (!std::isfinite(value) || std::fabs(static_cast<double>(value))>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
}
// 첫 공간/건설 효과 전에 같은 판본 자료와 필수 경계를 확인한다.
RawConstructionArrival::RawConstructionArrival(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    const ConstructionArrivalState& state,ConstructionArrivalHooks hooks)
    :pool_(pool),hash_(hash),types_(types),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) ||
        !hooks_.start || !hooks_.removeContained || !hooks_.destroy) throw std::invalid_argument("건설 도착 자료/효과 연결 오류");
}
// 현재 좌표를 원본처럼 ftol→float 저장→ftol한 칸으로 만들고 실제 finder를 끝까지 방문한다.
std::uint32_t RawConstructionArrival::Arrive(Sid builder) const {
    const auto raw=pool_.Slot(builder);
    const auto x=Truncate(static_cast<float>(Truncate(Coordinate(raw,kX))));
    const auto y=Truncate(static_cast<float>(Truncate(Coordinate(raw,kY))));
    const auto extra=pool_.Edition()==OriginalEdition::Patch1078 ? kPatchExtra : kCdExtra;
    bool found=false,started=false;RawSquidFinder finder(pool_,hash_,types_);
    // 이전 반환의 next는 actual finder가 저장한다. 시작 callback의 현재 raw 변화도 후속 후보에 반영한다.
    for (auto candidate=finder.Begin({x,y,x,y});candidate.value;candidate=finder.Next()) {
        const auto current=pool_.Slot(candidate);if (!(current[extra]&1)) continue;
        if (current[kType]>=types_.size()) throw std::out_of_range("건설 도착 후보 타입 오류");
        if (!(types_[current[kType]].flags1&kNeedsBuilder)) continue;
        found=true;
        if (candidate.value<5 || candidate.value>=pool_.Layout().serverFirst) {
            started=true;hooks_.start(candidate,state_.duration);
        }
    }
    if (!started && found) {
        if (!state_.checking) hooks_.removeContained(builder,state_.streakProcessType);
        return kWaiting;
    }
    hooks_.destroy(builder,0);return kFinished;
}
// 실제 연결의 풀 일치 확인용 참조다.
const SidPool& RawConstructionArrival::Pool() const { return pool_; }
// 같은 풀의 현재 ConstructionProcess 시작을 실제로 실행한다.
ConstructionArrivalHooks MakeConstructionArrivalHooks(const SidPool& pool,const RawConstructionProcess& process,
    ConstructionArrivalHooks hooks) {
    if (&pool!=&process.Pool()) throw std::invalid_argument("건설 도착/진행 SID 풀 불일치");
    hooks.start=[&process](Sid sid,float duration) { process.Start(sid,duration); };
    return hooks;
}
}
