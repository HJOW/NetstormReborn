// 원본은 같은 owner의 미등록187 캐리어를 찾고 로컬 전달을 먼저 수행한 뒤 이동을 재개한다.
#include "o/RawCarrierTransfer.h"
#include <array>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 도달한 실제 효과가 없으면 성공/빈 동작으로 대체하지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("캐리어 너겟 전달의 실제 하위 미연결");return hook; }
// 비정렬 원본 WORD/DWORD만 낮은 바이트부터 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value{};
    // 옆 필드를 합치지 않고 지정 폭만 읽는다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[offset+i])<<(8*i);return value;
}
// 실제 packet 필드의 하위 BYTE/WORD만 기록한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=2) {
    // 원본 지정 폭의 낮은 바이트부터 쓴다.
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 41d770의 signed64 절삭 뒤 하위 signed32를 사용하는 정상/무효 변환이다.
std::int32_t Cell(float value) {
    if (!std::isfinite(value) || std::fabs(static_cast<double>(value))>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
}
// PATCH 전용 전달을 CD의 다른 본문으로 간주하지 않는다.
RawCarrierTransfer::RawCarrierTransfer(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    const bool& boss,const std::uint32_t& editor,const std::uint32_t& localOwner,CarrierTransferState& state,CarrierTransferHooks hooks)
    :pool_(pool),hash_(hash),types_(types),boss_(boss),editor_(editor),localOwner_(localOwner),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("캐리어 너겟 전달 판본/TYPE 오류");
}
// 양쪽 free 비트만 검사한다. dead/owner/기증자 필터를 임의로 추가하지 않는다.
void RawCarrierTransfer::Receive(std::uint32_t owner,std::uint32_t flags,std::span<const std::uint8_t> packet) const {
    (void)owner;(void)flags;if (packet.size()<8) throw std::invalid_argument("캐리어 전달 명령 길이 오류");
    const Sid parent{static_cast<std::uint16_t>(Read(packet,4,2))},content{static_cast<std::uint16_t>(Read(packet,6,2))};
    if ((pool_.Slot(content)[11]&1) || (pool_.Slot(parent)[11]&1)) return;
    Need(hooks_.detach)(content,0);Need(hooks_.attach)(content,parent,0);Need(hooks_.preview)(parent);
}
// 첫 내용물 조회와 finder 생성 뒤에야 공유 cooldown을 줄인다.
std::uint32_t RawCarrierTransfer::Transfer(Sid parent) const {
    if (!boss_ || Need(hooks_.find)(parent,0x20,0,0).value || editor_) return 10;
    const auto raw=pool_.Slot(parent);const auto x=Cell(std::bit_cast<float>(Read(raw,14))),y=Cell(std::bit_cast<float>(Read(raw,18)));
    RawSquidFinder finder(pool_,hash_,types_);auto candidate=finder.Begin({x,y,x,y},0);
    if (std::bit_cast<std::int32_t>(state_.cooldown)>0) --state_.cooldown;
    // 실제 finder의 현재 후보와 아직 읽지 않은 체인은 효과 뒤의 현재 raw를 사용한다.
    while (candidate.value && state_.cooldown==0) {
        const auto donor=pool_.Slot(candidate);
        if (donor[34]==pool_.Slot(parent)[34] && donor[10]==state_.emptyType) {
            const auto content=Need(hooks_.find)(candidate,0x20,0,0);
            if (content.value && Read(pool_.Slot(content),18,2)==state_.nuggetType) {
                state_.lastDonor=candidate.value;state_.cooldown=5;
                std::array<std::uint8_t,10> packet{};packet[1]=state_.packet.message;Put(packet,2,state_.packet.length);packet[0]=state_.packet.protocol&0x7f;
                Put(packet,4,parent.value);Put(packet,6,content.value);Put(packet,8,candidate.value);
                Receive(localOwner_,0,packet);Need(hooks_.broadcast)(packet);
                const auto movement=Need(hooks_.movementType)(pool_.Slot(candidate)[10]);Need(hooks_.removeMovement)(candidate,movement);
                if (const auto target=Need(hooks_.chooseGeyser)(candidate);target.value) Need(hooks_.goGeyser)(candidate,target);
                const auto kind=Read(pool_.Slot(content),18,2);if (kind>=types_.size()) throw std::out_of_range("전달 뒤 현재 내용물 TYPE 범위 오류");
                if (types_[kind].flags2&0x8000U) Need(hooks_.goHome)(parent);return 12;
            }
        }
        candidate=finder.Next();
    }
    return 10;
}
// inc DWORD의 감김 뒤 signed %4이며 음수 나머지를 양수로 정규화하지 않는다.
std::uint32_t RawCarrierTransfer::Halt(Sid parent) const {
    state_.haltCounter=std::bit_cast<std::uint32_t>(std::bit_cast<std::int32_t>(state_.haltCounter+1U)%4);
    return state_.haltCounter==0 ? Transfer(parent) : Need(hooks_.commonHalt)(parent);
}
// 동일한 현재 풀의 주소만 공개한다.
const SidPool& RawCarrierTransfer::Pool() const { return pool_; }
}
