// 가상 carrier 확인 뒤에도 현재 타입/extra/좌표/소유자를 다시 읽는다.
#include "o/RawCarrierPostPop.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 사제 전체 postPop의 연결 검사를 위한 실제 풀 참조다.
const SidPool& RawCarrierPostPop::Pool() const { return pool_; }
namespace {
// raw 공통 타입/상태/좌표와 지면 소유자 갱신을 허용하는 타입 flags1 비트다.
constexpr std::size_t kType=10,kState=11,kX=14,kY=18;
constexpr std::uint32_t kClaimGround=0x400;
constexpr std::uint8_t kFreeContained=9,kAbstractBuried=9;
// 정렬되지 않은 좌표의 IEEE float를 네 바이트로 읽는다.
float Float(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits=0;
    // 낮은 바이트부터 조합하고 비트 패턴을 보존한다.
    for (std::size_t i=0;i<4;++i) bits|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return std::bit_cast<float>(bits);
}
}
// 효과와 타입 표를 모두 연결한 경우만 인스턴스를 구성한다.
RawCarrierPostPop::RawCarrierPostPop(SidPool& pool,std::span<const RiftTypeRecord> types,const CarrierPostPopState& state,CarrierPostPopHooks hooks)
    :pool_(pool),types_(types.begin(),types.end()),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("carrier postPop 타입 판본/크기 오류");
    if (!hooks_.carrierCheck || !hooks_.claimGround || !hooks_.validateBase || !hooks_.base)
        throw std::invalid_argument("carrier postPop 효과 누락");
}
// 일반 Pop의 지원 범위를 바꾸지 않고 직접 base 호출 계약만 확인한다.
void RawCarrierPostPop::Validate(Sid sid,std::uint32_t flags) const {
    const auto raw=pool_.AllocatedBytes(sid);
    if ((raw[kState]&kFreeContained) || raw[kType]<kFirstAssetTypeNumber || raw[kType]>=types_.size())
        throw std::invalid_argument("carrier postPop raw 자산 상태/타입 오류");
    hooks_.validateBase(sid,flags);
}
// 원본의 carrier 확인은 extra 차단과 무관하며 최초 flag가 있을 때는 호출하지 않는다.
void RawCarrierPostPop::PostPop(Sid sid,std::uint32_t flags) const {
    Validate(sid,flags);
    if (!(flags&1) && !state_.boss) hooks_.carrierCheck(sid);
    RunDamageable(sid,flags);
}
// Damageable의 직접 호출은 carrier 가상 확인을 거치지 않는다.
void RawCarrierPostPop::DamageablePostPop(Sid sid,std::uint32_t flags) const {
    Validate(sid,flags);RunDamageable(sid,flags);
}
// 첫 flag의 genus & 0x130000 조회는 원본에서도 반환값을 쓰지 않는 순수 읽기다.
void RawCarrierPostPop::RunDamageable(Sid sid,std::uint32_t flags) const {
    const auto raw=pool_.Slot(sid);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (state_.loadingDepth && raw[kType]!=state_.geyserType && !(raw[patch ? 40 : 35]&kAbstractBuried)
        && (types_.at(raw[kType]).flags1&kClaimGround))
        hooks_.claimGround(Float(raw,kX),Float(raw,kY),raw[patch ? 34 : 32]);
    hooks_.base(sid,flags);
}
// 자동 가상 등록 없이 공통 body와 그 보호 검사를 연결한다.
CarrierPostPopHooks MakeCarrierPostPopHooks(SidPool& pool,SquidPostPop& base,
    std::function<void(Sid)> carrierCheck,std::function<void(float,float,std::uint8_t)> claimGround) {
    if (&base.Pool()!=&pool) throw std::invalid_argument("carrier/공통 postPop의 SID 풀이 다릅니다");
    return {
        std::move(carrierCheck),std::move(claimGround),
        [&base](Sid sid,std::uint32_t flags) { base.ValidateBase(sid,flags); },
        [&base](Sid sid,std::uint32_t flags) { base.PostPopBase(sid,flags); }
    };
}
}
