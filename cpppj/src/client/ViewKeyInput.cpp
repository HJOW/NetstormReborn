// 원본4d9670의 테마 Tell은 정지 검사보다 앞이고4d998c 이후의 F2/F3 효과는 뒤다.
#include "client/ViewKeyInput.h"
#include <stdexcept>

namespace netstorm::client { // 원본 표시 키의 확인된 상태/장치만 사용한다.
namespace { // 원문 코드와 선택된 장치 검사를 외부 이름과 구별한다.
constexpr std::uint32_t kF2=0x710000U,kF3=0x720000U,kToggleBottom=0x02000088U; // Win32 가상 키와542914 초기 명령이다.
template<class T> const T& Required(const T& value) { // 실제 선택된 장치가 누락되면 빈 성공으로 바꾸지 않는다.
    if (!value) throw std::logic_error("표시 키의 실제 장치 미연결");return value;
}
}
// 현재 누름의 modifier/정지·ReadInt/Tell 순서와 F3 큐 인자를 원문대로 보존한다.
bool DispatchViewKeyInput(InputEvent event,ViewKeyInputGlobals globals,const ViewKeyInputHooks& hooks) {
    const auto code=event.code&0x03ffffffU; // 원본 사건의 상위 입력 표시는 키 비교에서만 제외한다.
    if ((event.code&InputCode::kRelease) || (code!=kF2 && code!=kF3)) return false;
    if (code==kF3 && (event.code&InputCode::kShift) && !(event.code&InputCode::kControl)) {
        std::int32_t mode{}; // 원문 지역값은0이며 ReadInt 실패 때도 이 값을 유지한다.
        Required(hooks.readInt)("theammode",mode);Required(hooks.tell)(mode ? "SetIslands(0)" : "SetIslands(1)");
    }
    if (Required(hooks.paused)()) return true;
    if (code==kF2) { globals.redraw=1;globals.hideBuildings=globals.hideBuildings==0; }
    else if (!(event.code&(InputCode::kControl|InputCode::kShift))) Required(hooks.queue)(kToggleBottom,0,2);
    return true;
}
}
