// 현재 패킷의 순회 수와 최초 대상 배열의 저장 한도를 구별한다.
#include "o/RawBombEffect.h"
#include <bit>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace netstorm::o {
namespace {
constexpr double kSigned64Limit=9223372036854775808.0; // 원본_ftol의 유효 signed64 변환 범위 상한이다.
// 실제 선택한 하위의 누락을 무동작 성공으로 처리하지 않는다.
template<class F> const F& Need(const F& hook) {
    if (!hook) throw std::logic_error("Bomb 효과 실제 하위 미연결");
    return hook;
}
// 호출자 소유 패킷의현재 비정렬 값을 읽으며 실제 버퍼 밖 접근을 거부한다.
std::uint32_t Bytes(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::invalid_argument("Bomb 효과 패킷 길이 부족");
    std::uint32_t value{};
    // 지정한 폭만 little endian으로 조합한다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(bytes[offset+i])<<(8*i);
    return value;
}
// 실제 signed64 절삭의하위DWORD를 취하며 비유한/범위 밖의indefinite도하위0이다.
std::uint32_t CostLow(float value) {
    const double wide=value;
    if (!(wide>=-kSigned64Limit && wide<kSigned64Limit)) return 0;
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(wide)));
}
}
// 다른 판본과 현재TYPE/저장소를 섞지 않는다.
RawBombEffect::RawBombEffect(const SidPool& pool,std::span<const BombEffectType> types,BombEffectState& state,BombEffectHooks hooks)
    :pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("Bomb 효과 판본/TYPE 오류");
}
// 외부 효과 뒤 현재 번호가 바뀔 수 있으므로 호출마다 검사한다.
const BombEffectType& RawBombEffect::Type(std::uint32_t kind) const {
    if (kind>=types_.size()) throw std::out_of_range("Bomb 효과 현재TYPE 범위");
    return types_[kind];
}
// 물리0번 슬롯과free/dead의추가 필터 없이 원본이 지정한 폭만 읽는다.
std::uint32_t RawBombEffect::Read(Sid sid,std::size_t offset,std::size_t width) const { return Bytes(pool_.Slot(sid),offset,width); }
// 현재 가상 효과는 상태를 바꿀 수 있으며 그 뒤 조회는 원본처럼 다시 수행한다.
void RawBombEffect::Receive(std::span<const std::uint8_t> packet) const {
    const Sid parent{static_cast<std::uint16_t>(Bytes(packet,4,2))};
    if (Read(parent,11,1)&3U) return;
    const Sid carrier{static_cast<std::uint16_t>(Read(parent,12,2))};
    if (!(Read(carrier,11,1)&7U)) {
        Need(hooks_.stopCast)(carrier);
        state_.spentCounter+=CostLow(Type(Read(parent,10,1)).cost);
    }
    const auto capacity=Bytes(packet,6,4);
    if (std::bit_cast<std::int32_t>(capacity)<0 || capacity>(packet.size()-10)/2) throw std::invalid_argument("Bomb 효과 최초 대상 목록 범위");
    if (!Need(hooks_.allocate)(capacity*4U)) throw std::bad_alloc(); // 원본NULL 쓰기를 성공으로 처리하지 않고 명시적으로 실패한다.
    std::vector<std::uint32_t> targets;
    std::uint32_t saved{};
    try {
        targets.resize(capacity);
        // 순회 조건은현재 패킷을 다시 읽고, 저장 한도는 최초capacity로 유지한다.
        for (std::uint32_t index=0;std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(Bytes(packet,6,4));++index) {
            const Sid target{static_cast<std::uint16_t>(Bytes(packet,10+2*std::size_t(index),2))};
            if (Read(target,11,1)&3U) continue;
            Need(hooks_.apply)(parent,Read(parent,0,4),target);
            if (saved<capacity) targets[saved++]=target.value;
        }
        if (state_.boss && (Read(parent,10,1)==state_.firstType || Read(parent,10,1)==state_.secondType)) {
            const auto& first=Type(state_.firstType);
            const auto& second=Type(state_.secondType);
            const bool secondEffect=Read(parent,10,1)!=state_.firstType; // 두TYPE 참조와 종류 선택은new40 이전에 캡처한다.
            std::uint32_t process{};
            if (Need(hooks_.allocate)(40)) process=Need(hooks_.construct)(secondEffect ? second.delay : first.delay,capacity,std::span<const std::uint32_t>(targets).first(saved),secondEffect ? 2U : 1U);
            const auto argument=Need(hooks_.argument)(0,0);
            Need(hooks_.attach)(process,state_.processType,argument,0,0);
        }
        Need(hooks_.sound)({Type(Read(parent,10,1)).sound,0,0,0,1,0});
        Need(hooks_.destroy)(parent,Read(parent,0,4),0x100048U);
    } catch (...) {
        Need(hooks_.release)();
        throw;
    }
    Need(hooks_.release)();
}
// 현재 슬롯을 공유하는 같은 세션인지 확인한다.
const SidPool& RawBombEffect::Pool() const { return pool_; }
}
