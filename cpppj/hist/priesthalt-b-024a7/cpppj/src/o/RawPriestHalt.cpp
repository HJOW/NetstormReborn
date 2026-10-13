// 판본별 반환 조건과 최초 보관 좌표를 유지하고 기존 지면/HP/finder를 재사용한다.
#include "o/RawPriestHalt.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kGroundBias=0.9999F,kSnapBias=0.9999899864196777F; // 지면 조회와 공간 복귀 helper의 서로 다른 원본 float 상수다.
constexpr std::uint32_t kMoving=10,kStopped=11,kChanged=12; // 부모 Halt의 실제 반환값이다.
// 비정렬 현재 raw DWORD를 원본 little endian 순서로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 지정 DWORD 밖의 옆 필드를 합치지 않는다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(8*index);
    return value;
}
}
// 첫 몸체 실행 전에 모든 실제 같은 풀/타입/지도와 필수 외부 효과를 확인한다.
RawPriestHalt::RawPriestHalt(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const std::uint8_t> spots,const RawCarrierCheck& check,const RawPriestState& priest,
    const SquidReward& hitPoints,PriestHaltState& state,PriestHaltHooks hooks)
    :pool_(pool),hash_(hash),types_(types),spots_(spots),check_(check),priest_(priest),hitPoints_(hitPoints),state_(state),hooks_(std::move(hooks)) {
    if (&pool!=&check.Pool() || &pool!=&priest.Pool() || &pool!=&hitPoints.Pool()
        || types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || spots.size()!=65536
        || !hooks_.unpop || !hooks_.pop || !hooks_.carrierPickup || !hooks_.nuggetTransfer)
        throw std::invalid_argument("사제 Halt 실제 월드/하위 효과 연결 누락");
}
// 좌표 두 DWORD만 읽으며 사제 Halt가 보관할 최초 위치도 같은 함수에서 얻는다.
std::array<float,2> RawPriestHalt::Position(Sid sid) const {
    const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))};
}
// float 입력과 float bias의 합은 double에 유지하고 원본처럼0방향 절삭한다.
std::int32_t RawPriestHalt::Cell(float coordinate,float bias) {
    const double value=static_cast<double>(coordinate)+static_cast<double>(bias);
    if (!std::isfinite(value) || value<0 || value>=256) throw std::out_of_range("사제 Halt 정상 지도 좌표");
    return static_cast<std::int32_t>(value);
}
// 원본 +38은 공간 callback 전에 최초 좌표를 보관하며 이후 raw 위치를 재조회하지 않는다.
std::uint32_t RawPriestHalt::Halt(Sid sid) const {
    const auto initial=Position(sid);
    if (state_.battle) {
        const auto x=Cell(initial[0],kGroundBias),y=Cell(initial[1],kGroundBias);
        const bool ground=(spots_[static_cast<std::size_t>(y*256+x)]&6)!=0;
        if (!ground) {
            if (state_.boss) { hooks_.unpop(sid,0);hooks_.pop(sid,static_cast<float>(Cell(initial[0],kSnapBias)),static_cast<float>(Cell(initial[1],kSnapBias)),0); }
            return kStopped;
        }
        if (priest_.CurrentHitPoints(sid)<hitPoints_.MaxHitPoints(sid)/2 && state_.boss) {
            hooks_.unpop(sid,0);hooks_.pop(sid,static_cast<float>(Cell(initial[0],kSnapBias)),static_cast<float>(Cell(initial[1],kSnapBias)),0);return kStopped;
        }
    }
    return CommonHalt(sid);
}
// 가상+cc → 단순 절삭 점 finder → 각 판본 실제 특정 운반 타입 helper → base10의 순서다.
std::uint32_t RawPriestHalt::CommonHalt(Sid sid) const {
    if (check_.Check(sid)) return kChanged;
    const auto point=Position(sid);const auto x=Cell(point[0],0),y=Cell(point[1],0);RawSquidFinder finder(pool_,hash_,types_);
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    // 기본 finder의0단계/버킷 순서와 buried 제외를 그대로 재사용한다.
    for (auto candidate=finder.Begin({x,y,x,y});candidate.value;candidate=finder.Next()) {
        const auto type=pool_.Slot(candidate)[10];
        if (candidate!=sid && type==state_.carrierType) {
            const auto result=hooks_.carrierPickup(candidate);if (patch ? result==kChanged : result!=0) return kChanged;
        } else if (patch && candidate!=sid && type==state_.nuggetCarrierType) {
            if (hooks_.nuggetTransfer(candidate)==kChanged) return kChanged;
        }
    }
    return kMoving; // 실제004ac990/CD004ac460의 기본 몸체만10을 반환한다.
}
// 실제 걷기/Arrival 연결에서 서로 다른 월드의 저장소 혼합을 거부한다.
const SidPool& RawPriestHalt::Pool() const { return pool_; }
}
