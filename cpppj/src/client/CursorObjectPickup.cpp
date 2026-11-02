// 원문447340은 Unpop 뒤에 TYPE BYTE/argument WORD를 다시 읽고 Take로 꼬리 호출한다.
#include "client/CursorObjectPickup.h"
#include <stdexcept>

namespace netstorm::client { //현재 실제 풀과 원문 취득 부모의 짧은 순서만 소유한다.
namespace { //도달한 효과만 연결 계약을 검사한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("편집 객체 취득의 실제 하위 미연결");return hook; } //선택하지 않은 경계는 요구하지 않는다.
}
// 가상효과 뒤 TYPE/WORD가 바뀌면 바뀐 현재값을 사용하며 앞선 Unpop을 되돌리지 않는다.
void PickupObjectForCursor(const o::SidPool& pool,std::uint32_t source,std::uint32_t flags,const CursorObjectPickupHooks& hooks) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || source>=pool.Capacity() || source>65535U) throw std::out_of_range("편집 객체 취득의10.78 물리 SID 범위");
    const o::Sid sid{static_cast<std::uint16_t>(source)};Need(hooks.unpop)(sid,0);
    const auto raw=pool.Slot(sid);const auto argument=std::uint32_t(raw[12])|(std::uint32_t(raw[13])<<8);const auto type=raw[10]; //원문의 MOVZX WORD 다음 MOVZX BYTE다.
    const auto form=Need(hooks.create)(sid,type,argument,0x80,0x10,0);Need(hooks.take)(form,flags|0x100U);
}
}
