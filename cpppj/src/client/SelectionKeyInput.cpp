// 직접 Home 호출을 원문의 일반 View 큐로 바꾸고 N/P/R의 입력 부모 정지 관문을 복원한다.
#include "client/SelectionKeyInput.h"
#include <stdexcept>

namespace netstorm::client { //원문은 Alt를 이 키의 별도 거부 조건으로 사용하지 않는다.
namespace { //도달한 실제 장치만 요구한다.
template<class F> const F& SelectionKeyNeed(const F& hook) { if (!hook) throw std::logic_error("선택 키의 실제 장치 미연결");return hook; } //누락을 빈 성공이나 다른 카메라 경로로 대체하지 않는다.
}
bool DispatchSelectionKeyInput(InputEvent event,const SelectionKeyInputHooks& hooks) { //이 부모 뒤의 Next/사제 몸체가 가진 독립 정지 동작은 그대로 유지한다.
    const auto key=event.code&0x03ffffffU;const bool home=key=='H' || key=='h',next=key=='N' || key=='n',priest=key=='P' || key=='p' || key=='R' || key=='r'; //문자 하위16비트만 비교해 일반 명령이나 마우스를 키로 오인하지 않는다.
    if ((event.code&InputCode::kRelease) || (!home && !next && !priest && key!=0x730000U && key!=0x740000U && key!=0x750000U)) return false;
    if (SelectionKeyNeed(hooks.paused)()) return true;
    if (key==0x750000U) { // 원문4d9a00의 F6는 공통 정지 뒤 수정키를 검사하고 일반 ShowTechnology 명령만 예약한다.
        if (event.code&(InputCode::kShift|InputCode::kControl)) return true;
        const auto command=SelectionKeyNeed(hooks.command)(0x00542974U);SelectionKeyNeed(hooks.queue)(command,0,0);
    } else if (next || (key==0x740000U && (event.code&InputCode::kControl))) SelectionKeyNeed(hooks.next)(event.code&InputCode::kShift);
    else if (priest) SelectionKeyNeed(hooks.priest)();
    else if (home || key==0x730000U) {
        if (key==0x730000U && (event.code&(InputCode::kShift|InputCode::kControl))) return true;
        const auto command=SelectionKeyNeed(hooks.command)(0x005429acU);SelectionKeyNeed(hooks.queue)(command,0,0);
    } else {
        const auto owner=SelectionKeyNeed(hooks.localOwner)();const auto command=SelectionKeyNeed(hooks.command)(0x005429d4U);SelectionKeyNeed(hooks.queue)(command,owner,owner);
    }
    return true;
}
}
