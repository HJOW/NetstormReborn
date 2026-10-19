// 원본의 원격 비용/현재genus/확보 실패와 순서 있는 예약을 보존한다.
#include "o/RawBombActivation.h"
#include <bit>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 실제 선택된 하위 누락을 무동작 성공으로 처리하지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("주문 활성화 실제 하위 미연결");return hook; }
}
// 외부 효과를 실행하지 않고 현재 저장소/전역만 빌린다.
RawBombActivation::RawBombActivation(const SidPool& pool,std::span<const RiftTypeRecord> types,const std::uint32_t& localOwner,BombActivationHooks hooks)
    :pool_(pool),types_(types),localOwner_(localOwner),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("주문 활성화 판본/TYPE 오류");
}
// 수신자/연결 인자는 원본이 읽지 않으며 패킷의 실제 부모WORD만 사용한다.
void RawBombActivation::Receive(std::span<const std::uint8_t> packet) const {
    if (packet.size()<6) throw std::invalid_argument("주문 활성화 패킷 길이 부족");
    const Sid parent{static_cast<std::uint16_t>(packet[4]|(std::uint16_t(packet[5])<<8))};
    if (pool_.Slot(parent)[11]&3U) return;
    const auto owner=pool_.Slot(parent)[34];
    if (owner!=localOwner_) {
        const auto cost=Need(hooks_.cost)(pool_.Slot(parent)[10]);const auto negative=std::bit_cast<std::int32_t>(0U-static_cast<std::uint32_t>(cost)); //원본neg의32비트 감김이다.
        Need(hooks_.subtract)(owner,static_cast<float>(negative));
    }
    const auto type=pool_.Slot(parent)[10];if (type>=types_.size()) throw std::out_of_range("주문 활성화 현재TYPE 범위");
    if (!(types_[type].flags2&0x100U)) Need(hooks_.assertion)(0x206);
    if (Need(hooks_.allocate)(40)) Need(hooks_.construct)(0,parent,0.1f);
}
const SidPool& RawBombActivation::Pool() const { return pool_; } //같은 Session의 저장소인지 확인한다.
}
