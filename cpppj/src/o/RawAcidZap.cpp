// 포획 중의 현재 패킷/원래 슬롯/owner 재조회 지점을 합치지 않는다.
#include "o/RawAcidZap.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kPriest=0x200000,kDestroyFlags=0x400048; //사제 제외 genus와 원본 포획 삭제 플래그다.
// 비정렬 little endian 필드의 지정 폭만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value{};
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[offset+index])<<(index*8); //주변 raw를 읽지 않는다.
    return value;
}
// 포획 패킷의 WORD 하나를 낮은 바이트부터 쓴다.
void Put(std::span<std::uint8_t> packet,std::size_t offset,std::uint16_t value) { packet[offset]=static_cast<std::uint8_t>(value);packet[offset+1]=static_cast<std::uint8_t>(value>>8); }
}
// 효과 경계 누락은 소리나 메모리 생성 전에 확인한다.
RawAcidZap::RawAcidZap(const SidPool& pool,std::span<const RiftTypeRecord> types,const AcidZapState& state,const FormPacketHeader& header,AcidZapHooks hooks)
    :pool_(pool),types_(types),state_(state),header_(header),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || !hooks_.sound || !hooks_.dissolve || !hooks_.destroy) throw std::invalid_argument("애시드 수신 판본/실제 하위 경계 누락");
}
// wire는protocol/message 순서이며 등록 표의message/protocol 저장 순서와 구분한다.
std::array<std::uint8_t,11> RawAcidZap::Build(Sid victim,Sid first,Sid second,std::int8_t orientation) const {
    std::array<std::uint8_t,11> packet{static_cast<std::uint8_t>(header_.protocol&0x7f),header_.message};
    Put(packet,2,header_.length);Put(packet,4,victim.value);Put(packet,6,first.value);Put(packet,8,second.value);packet[10]=std::bit_cast<std::uint8_t>(orientation);return packet;
}
// 원본 수신은 둘째 폴/동맹/boss/editor를 별도 검사하지 않는다.
void RawAcidZap::Receive(std::uint32_t owner,std::uint32_t flags,std::span<const std::uint8_t> packet) const {
    (void)owner;(void)flags;if (packet.size()<11) throw std::invalid_argument("애시드 포획 명령 길이 오류");
    const Sid victim{static_cast<std::uint16_t>(Read(packet,4,2))},first{static_cast<std::uint16_t>(Read(packet,6,2))};const auto raw=pool_.Slot(victim);
    if ((raw[11]&3) || (pool_.Slot(first)[11]&1) || raw[10]==state_.altar || raw[10]==state_.growingIceTower) return;
    if (raw[10]>=types_.size()) throw std::out_of_range("애시드 대상 TYPE 범위 오류");if (types_[raw[10]].flags2&kPriest) return;
    hooks_.sound(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)),"dissolve.wav",0);
    hooks_.dissolve(Sid{static_cast<std::uint16_t>(Read(packet,4,2))},Sid{static_cast<std::uint16_t>(Read(packet,6,2))},Sid{static_cast<std::uint16_t>(Read(packet,8,2))},std::bit_cast<std::int8_t>(packet[10]));
    hooks_.destroy(victim,(std::uint32_t(pool_.Slot(first)[34])<<16)|kDestroyFlags);
}
}
