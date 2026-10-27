// 새 활성화/피해를 만들지 않고 원문4d6eca..4da477의 해당 명령 소비를 유지한다.
#include "client/DetonateInput.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { //10.78의 일반 입력 관문이다.
// 대상은y의낮은WORD이며 현재 선택·소유자·메뉴 체인을 새 조건으로 사용하지 않는다.
bool DispatchDetonateInput(const InputEvent& event,const std::function<void(o::Sid,std::uint32_t)>& objectInput) {
    if ((event.code&0x03ffffffU)!=kDetonateBombCommand) return false;
    if (event.code&InputCode::kRelease) return true;
    const o::Sid target{static_cast<std::uint16_t>(std::bit_cast<std::uint32_t>(event.y))};
    if (target.value) { if (!objectInput) throw std::logic_error("Detonate의 실제 가상 입력 미연결");objectInput(target,event.code); }
    return true; // 원문 Spell→Object→Form의0반환은 종료 예외나 임의5c 요청으로 바꾸지 않는다.
}
}
