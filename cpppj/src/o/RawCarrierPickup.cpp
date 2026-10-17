// 서버는임시dead로원객체의부모통지를억제하고다음종속을먼저읽은뒤옮긴다.
#include "o/RawCarrierPickup.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 선택된실제효과누락을빈동작으로바꾸지않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("일반운반체포획의실제하위미연결");return hook; }
// 비정렬WORD/DWORD를지정폭만큼읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value{};
    // 원본의낮은바이트부터조합한다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[offset+i])<<(8*i);return value;
}
// 지정WORD의상위값은버리고실제두바이트만쓴다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) { raw[offset]=static_cast<std::uint8_t>(value);raw[offset+1]=static_cast<std::uint8_t>(value>>8); }
}
//10.72의다른포획수신을PATCH와같다고추정하지않는다.
RawCarrierPickup::RawCarrierPickup(SidPool& pool,std::span<const RiftTypeRecord> types,const ContainedFinderState& contained,
    const bool& boss,const bool& single,CarrierPickupState& state,CarrierPickupHooks hooks)
    :pool_(pool),types_(types),contained_(contained),boss_(boss),single_(single),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("일반운반체포획의판본/TYPE 오류");
}
// 현재TYPE getter를호출전순수조회로대체하며반환genus를보관하지않는다.
std::uint32_t RawCarrierPickup::Genus(Sid sid) const { const auto type=pool_.Slot(sid)[10];if (type>=types_.size()) throw std::out_of_range("포획현재TYPE 범위오류");return types_[type].flags2; }
// 생성효과가free/void 등을바꾸어도원본처럼현재물리BYTE의dead만해제한다.
void RawCarrierPickup::Dead(Sid sid,bool value) const { auto& state=pool_.bytes_[pool_.Offset(sid)+11];state=static_cast<std::uint8_t>(value ? state|2U : state&~2U); }
// 후보는권한검사전에조회한다.내부이름/후보bool로다른프로토콜을고르지않는다.
void RawCarrierPickup::Request(Sid parent) const {
    const auto raw=pool_.Slot(parent);const auto found=Need(hooks_.candidate)(parent,std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))).first;
    if (!found.value || !boss_) return;const auto target=pool_.Slot(found);
    if (!single_) static_cast<void>(Genus(found));if (!boss_) return;
    std::array<std::uint8_t,9> packet{static_cast<std::uint8_t>(state_.request.protocol&0x7f),state_.request.message,static_cast<std::uint8_t>(state_.request.length),static_cast<std::uint8_t>(state_.request.length>>8)};
    Put(packet,4,parent.value);Put(packet,6,found.value);packet[8]=target[10];Need(hooks_.send)(9,packet);
}
// 삭제한Form의kind는보관하지만비교전역은삭제효과뒤현재값을읽는다.
void RawCarrierPickup::Discard(Sid parent) const {
    const auto old=Need(hooks_.find)(parent,1,0,0);if (!old.value) return;
    const auto kind=Read(pool_.Slot(old),18,2);Need(hooks_.destroy)(old,0);if (kind!=state_.silentType) Need(hooks_.afterDiscard)();
}
// 최초parent는보관하며입력명령의parentWORD만reveal뒤같은packet에서다시읽는다.
void RawCarrierPickup::Receive(std::uint32_t owner,std::uint32_t flags,std::span<const std::uint8_t> packet) const {
    (void)owner;(void)flags;if (packet.size()<9) throw std::invalid_argument("포획요청packet길이오류");
    const Sid parent{static_cast<std::uint16_t>(Read(packet,4,2))};Sid target{static_cast<std::uint16_t>(Read(packet,6,2))};
    if (!boss_) { Need(hooks_.assertion)(0x20c);if (!boss_) return; }
    if ((pool_.Slot(target)[11]&3) || (pool_.Slot(parent)[11]&3) || pool_.Slot(target)[10]!=packet[8]) return;
    if (Genus(target)&0x2000U) target=Need(hooks_.reveal)(target);
    Need(hooks_.queue)(state_.command,target.value,static_cast<std::int32_t>(Read(packet,4,2)));
    if (Genus(target)&0x2100U) Discard(parent);
    const auto raw=pool_.Slot(target);const auto argument=Read(raw,12,2);const auto kind=raw[10];Dead(target,true);
    const auto created=Need(hooks_.create)(parent,kind,argument,(Genus(target)&0x100U) ? 1U : 0x20U,0,0);Dead(target,false);
    RawContainedFinder finder(pool_,contained_);auto current=finder.Begin(target);
    // 현재종속의다음을먼저조회해야현재종속의Unpop/부모통지가다음링크를바꾸어도순서가유지된다.
    while (current.value) { const auto saved=current;current=finder.Next();Need(hooks_.detach)(saved,0);Need(hooks_.attach)(saved,created,0); }
    Need(hooks_.destroy)(target,0);
}
// 실제같은Pool만공개한다.
const SidPool& RawCarrierPickup::Pool() const { return pool_; }
}
