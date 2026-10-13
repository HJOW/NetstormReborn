// 조회의 제외/지면/가상 낙하 순서를 유지하고 불필요한 좌표 읽기를 생략한다.
#include "o/RawCarrierCheck.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 raw 타입/좌표 필드와 타입 제외/지면 마스크다.
constexpr std::size_t kType=10,kX=14,kY=18;
constexpr std::uint32_t kExcludedGenus=0x320000;
constexpr std::uint8_t kGround=6;
// 두 판본의 실제 3f7ff972 상수다. CD 주소는 00506d78이며 값은 패치와 같다.
constexpr float kCoordinateBias=0.9999f;
// 정렬되지 않은 좌표를 원본 비트 그대로 읽는다.
float ReadFloat(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits=0;
    // 낮은 바이트부터 조합한다.
    for (std::size_t i=0;i<4;++i) bits|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return std::bit_cast<float>(bits);
}
// 원본 x87 합을 float으로 좁히지 않고 절삭한다. 범위 밖 spot 읽기는 진단한다.
std::size_t Cell(float coordinate) {
    const double value=std::trunc(static_cast<double>(coordinate)+static_cast<double>(kCoordinateBias));
    if (!std::isfinite(value) || value<0 || value>=256) throw std::out_of_range("Carrier 지면 좌표 오류");
    return static_cast<std::size_t>(value);
}
}
// 표 크기와 필수 가상 경계를 검증한다. 특정 자산 지원 여부는 낙하 분배기가 결정한다.
RawCarrierCheck::RawCarrierCheck(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const std::uint8_t> spots,std::function<bool(Sid)> fallWalker)
    :pool_(pool),types_(types),spots_(spots),fallWalker_(std::move(fallWalker)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || spots.size()!=65536 || !fallWalker_)
        throw std::invalid_argument("Carrier 지면 조회 연결 오류");
}
// 원본은 free/dead/void/extra 비트를 조회 조건으로 추가하지 않는다.
bool RawCarrierCheck::Check(Sid sid) const {
    const auto raw=pool_.Slot(sid);
    if (raw[kType]>=types_.size()) throw std::out_of_range("Carrier 지면 조회 타입 오류");
    if (types_[raw[kType]].flags2&kExcludedGenus) return false;
    const auto y=Cell(ReadFloat(raw,kY)),x=Cell(ReadFloat(raw,kX));
    if (spots_[y*256+x]&kGround) return false;
    return fallWalker_(sid);
}
// 다른 풀 연결을 효과 발생 전에 거부할 때 사용한다.
const SidPool& RawCarrierCheck::Pool() const { return pool_; }
// postPop은 원본처럼 +0xcc 반환값을 버리고 같은 flags로 부모 처리를 계속한다.
CarrierPostPopHooks MakeCarrierCheckPostPopHooks(const SidPool& pool,const RawCarrierCheck& check,CarrierPostPopHooks hooks) {
    if (&pool!=&check.Pool()) throw std::invalid_argument("Carrier postPop/조회 풀 불일치");
    hooks.carrierCheck=[&check](Sid sid) { (void)check.Check(sid); };return hooks;
}
// 나선 생성의 검사 순서는 그대로 두고 가상 조회 경계만 연결한다.
PriestSpawnHooks MakeCarrierCheckSpawnHooks(const SidPool& pool,const RawCarrierCheck& check,PriestSpawnHooks hooks) {
    if (&pool!=&check.Pool()) throw std::invalid_argument("사제 생성/Carrier 조회 풀 불일치");
    hooks.checkCarrier=[&check](Sid sid) { (void)check.Check(sid); };return hooks;
}
}
