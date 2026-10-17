// 원본의 표시 전/후 선택 비트를 구분하고 큰 표시·Regular는 실제 하위로 연결한다.
#include "o/RawSquidSelection.h"
#include <array>
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 생성자 VT와 선택/표시 슬롯의 주소 기록이다. 새 독립 PE 도구가 생성한다.
struct SelectionVtable {
    std::uint32_t table,notice,refresh; // 원본+0/+7c/+88이며 호스트 주소로 사용하지 않는다.
};
#include "SelectionVtables.inc"
// 도달한 실제 효과가 없으면 선행 원본 쓰기를 가장하지 않는다.
template<class F> const F& Need(const F& hook) {
    if (!hook) throw std::logic_error("객체 선택 실제 하위 미연결");return hook;
}
}
// 기존 생성자83개에 존재하는 두+7c만 지원한다. 임의 VT를 기본 몸체로 대체하지 않는다.
std::uint32_t SquidSelectionEntry(std::uint32_t table) {
    // 현재 기록값과 일치하는 원본 VT를 찾아 실제 몸체 주소를 반환한다.
    for (const auto& entry:kSelectionVtables) if (entry.table==table) return entry.notice;return 0;
}
// 474240은 이동 가능 판정이 아니라 사정거리 종류 판정이다.
bool SelectionUsesRange(const RiftTypeRecord& type) { return type.group==0 || type.group==1 || (type.flags2&0x100U) || type.group==6 || type.group==3; }
// 원본10.78의 extra+28 배치만 허용한다.
RawSquidSelection::RawSquidSelection(SidPool& pool,std::function<void(Sid)> refresh):pool_(pool),refresh_(std::move(refresh)) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("객체 선택10.78 풀 오류");
}
// Damageable의 표시가 extra/VT를 바꿔도 표시 후 현재 extra를 읽어 한 비트만 쓴다.
void RawSquidSelection::Mark(Sid sid,std::uint32_t value) const {
    if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("객체 선택 SID 범위 오류");
    const auto raw=pool_.Slot(sid);const auto table=std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);const auto entry=SquidSelectionEntry(table);
    if (!entry) throw std::logic_error("객체 선택 가상+7c VT 미연결");
    if (entry==0x44b2b0U) Need(refresh_)(sid);
    else if (entry!=0x41c0b0U) throw std::logic_error("객체 선택 파생 몸체 미연결");
    auto& extra=pool_.bytes_[pool_.Offset(sid)+40];extra=static_cast<std::uint8_t>((extra&~4U)|((value&1U)<<2));
}
// 진단 callback이 타입을 고치면 현재 값으로 계속하며 여전히Form이면 생성하지 않는다.
void StartSelectionContentPreview(const SidPool& pool,Sid parent,std::uint32_t loadingDepth,
    const std::function<void(Sid,std::uint32_t,float)>& regular,const std::function<void()>& assertion) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("선택 내용물 표시10.78 풀 오류");
    if (loadingDepth) return;const auto raw=pool.Slot(parent);if (raw[40]&9) return;
    if (raw[10]<70) { Need(assertion)();if (pool.Slot(parent)[10]<70) return; }
    Need(regular)(parent,0x491,std::bit_cast<float>(0x3e4ccccdU));
}
}
