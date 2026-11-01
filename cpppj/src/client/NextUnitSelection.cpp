// 원본은 예약1 다음부터 시작하고 free만 건너뛴다. dead나 내용물 상태를 추가로 제외하지 않는다.
#include "client/NextUnitSelection.h"
#include <stdexcept>

namespace netstorm::client { //이동 가능 TYPE의 실제 물리 순서와 입력 부모의 호출 순서를 보존한다.
namespace { //도달한 장치만 검사하고 조기 반환에 불필요한 부수 효과를 만들지 않는다.
template<class F> const F& NextNeed(const F& hook) { if (!hook) throw std::logic_error("다음 유닛 선택 장치 미연결");return hook; } //같은 실제 경계가 누락되면 그 위치에서 실패한다.
}
void SelectNextMovingUnit(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,std::uint32_t& selected,std::uint32_t add,const NextUnitSelectionHooks& hooks) { //원본은 정지 중에도 검색과 첫 선택/가운데 이동을 수행한다.
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("다음 유닛 선택의10.78 풀/TYPE 오류");
    const auto raw=[&](std::uint32_t sid) { if (!sid || sid>=pool.Capacity() || sid>65535U) throw std::out_of_range("다음 유닛 선택의 물리 SID 범위");return pool.Slot(o::Sid{static_cast<std::uint16_t>(sid)}); }; //넓은 DWORD를 WORD로 조용히 접지 않는다.
    const auto mobile=[&](std::uint32_t sid) { const auto type=raw(sid)[10];if (type>=types.size()) throw std::out_of_range("다음 유닛 선택의 TYPE 범위");return (types[type].flags2&0x30000U)!=0; }; //현재 TYPE를 호출 시점마다 다시 읽는다.
    auto previous=selected;if (previous && (raw(previous)[11]&1U)) previous=0;
    const auto owner=NextNeed(hooks.localOwner)();const auto end=pool.Capacity(); //원문 iterator의 상한은 검색을 시작할 때 저장한다.
    const auto advance=[&](std::uint32_t sid) { ++sid;while (sid<end && (raw(sid)[11]&1U)) ++sid;return sid; }; //0040eb40과 같은 선행 증가/free 건너뛰기다.
    const auto find=[&](std::uint32_t sid) { while (sid && sid<pool.Capacity()) { if (raw(sid)[34]==owner && mobile(sid)) return sid;sid=advance(sid); }return 0U; }; //소유자 BYTE와 캡처한 전체 DWORD를 비교하고 실패 시에만 다음 후보를 읽는다.
    auto candidate=advance(1);if (previous) candidate=advance(previous);candidate=find(candidate);if (!candidate) candidate=find(advance(1));if (!candidate) return;
    NextNeed(hooks.select)(candidate,0,1);NextNeed(hooks.center)(candidate);if (NextNeed(hooks.paused)()) return;
    if (add) { if (!mobile(candidate)) return;if (selected && !(raw(selected)[11]&1U) && mobile(selected)) { NextNeed(hooks.multiple)(candidate,1);return; } }
    NextNeed(hooks.select)(candidate,0,1);
}
}
