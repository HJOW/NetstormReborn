// 상태/동맹/가상 효과 뒤 재조회 위치를 원본과 같은 순서로 유지한다.
#include "o/RawBombStatus.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 선택한 실제 효과의 누락을 빈 성공으로 숨기지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("상태 주문 실제 하위 미연결");return hook; }
// 원본의 비정렬 WORD/DWORD 폭만 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t result{};for (std::size_t i=0;i<width;++i) result|=std::uint32_t(raw[offset+i])<<(8*i); // 주변 필드로 읽기 폭을 넓히지 않는다.
    return result;
}
}
RawBombStatus::RawBombStatus(SidPool& pool,std::span<const RiftTypeRecord> types,BombStatusState& state,BombStatusHooks hooks)
    :pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("상태 주문 PATCH/188TYPE 필요");
}
std::uint32_t RawBombStatus::Kind(Sid parent) const {
    const auto table=Read(pool_.Slot(parent),0);if (table==0x502510U || table==0x502e68U || table==0x502f30U) return 1;if (table==0x502ff8U) return 2;throw std::invalid_argument("상태 주문의 현재 VT 불일치");
}
std::uint32_t RawBombStatus::Genus(Sid sid) const {
    const auto kind=pool_.Slot(sid)[10];if (kind>=types_.size()) throw std::out_of_range("상태 주문의 현재 TYPE 범위");return types_[kind].flags2;
}
bool RawBombStatus::MayTarget(Sid parent,Sid target) const {
    const auto table=Read(pool_.Slot(parent),0);if (table!=0x502510U && table!=0x502ff8U) throw std::invalid_argument("상태 주문 자격의 현재 VT 불일치"); // Wind의 같은 은신 적용 함수와 별개인 소환 자격을 혼용하지 않는다.
    if (Kind(parent)==2) return (Genus(target)&0x170000U)!=0;
    if (Genus(target)&0x1570004U) return true;const auto kind=pool_.Slot(target)[10];return kind==state_.altar || kind==state_.outpost;
}
void RawBombStatus::Apply(Sid parent,Sid target) const {
    const auto effect=Kind(parent);if (target.value==Read(pool_.Slot(parent),12,2)) return;
    if (effect==2 && pool_.Slot(target)[10]==state_.priest) {
        const auto owner=pool_.Slot(parent)[34],other=pool_.Slot(target)[34];if (owner>=9 || other>=9 || state_.alliances.size()!=81) throw std::out_of_range("상태 주문의 현재 동맹 범위");
        if (state_.alliances[owner*9U+other]) return;
    }
    const auto current=pool_.Slot(target);const_cast<std::uint8_t*>(current.data())[40]|=effect==1 ? 0x10U : 0x20U; // free/dead를 추가 검사하지 않는 원본 물리 쓰기다.
    if (effect==1) {
        Need(hooks_.refresh)(target);
        if (Genus(target)&0x1000004U) Need(hooks_.flare)(target,1,1,state_.flare,2);
        return;
    }
    if (Genus(target)&0x200000U) { Need(hooks_.unpop)(target,0);Need(hooks_.repop)(target,0x40); }
    else Need(hooks_.refresh)(target);
    if (pool_.Slot(target)[10]==state_.fence) Need(hooks_.fence)(target);
    Need(hooks_.flare)(target,1,3,state_.flare,3);
}
}
