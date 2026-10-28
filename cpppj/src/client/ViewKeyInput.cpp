// 원본4d9670의 테마 Tell은 정지 검사보다 앞이고4d998c 이후의 F2/F3 효과는 뒤다.
#include "client/ViewKeyInput.h"
#include <stdexcept>

namespace netstorm::client { // 원본 표시 키의 확인된 상태/장치만 사용한다.
namespace { // 원문 코드와 선택된 장치 검사를 외부 이름과 구별한다.
constexpr std::uint32_t kF2=0x710000U,kF3=0x720000U,kF8=0x770000U,kToggleBottom=0x02000088U,kFirst=0x02000091U; // Win32 가상 키와542914/542938 초기 명령이다.
template<class T> const T& Required(const T& value) { // 실제 선택된 장치가 누락되면 빈 성공으로 바꾸지 않는다.
    if (!value) throw std::logic_error("표시 키의 실제 장치 미연결");return value;
}
}
// 현재 누름의 modifier/정지·ReadInt/Tell·Fort/Editor 순서와 F3/F8 큐 인자를 보존한다.
bool DispatchViewKeyInput(InputEvent event,ViewKeyInputGlobals globals,const ViewKeyInputHooks& hooks) {
    const auto code=event.code&0x03ffffffU; // 원본 사건의 상위 입력 표시는 키 비교에서만 제외한다.
    if ((event.code&InputCode::kRelease) || (code!=kF2 && code!=kF3 && code!=kF8)) return false;
    if (code==kF8 && !(event.code&(InputCode::kControl|InputCode::kShift))) {
        if (Required(hooks.fort)()) Required(hooks.queue)(kFirst,0,0);
        else if (Required(hooks.metamap)()) Required(hooks.tell)("ChalIntro"); // 원문 메타맵 분기는 정적 연결이며 이번 새 실행/검사 범위에서 제외한다.
        if (Required(hooks.editor)()) Required(hooks.highlight)(1); // First/Tell 뒤 현재 Editor를 읽으며 정지와 무관하다.
    }
    if ((code==kF3 || code==kF8) && (event.code&InputCode::kShift) && !(event.code&InputCode::kControl)) {
        std::int32_t mode{}; // 원문 지역값은0이며 ReadInt 실패 때도 이 값을 유지한다.
        Required(hooks.readInt)("theammode",mode);Required(hooks.tell)(mode ? "SetIslands(0)" : "SetIslands(1)");
    }
    if (Required(hooks.paused)()) return true;
    if (code==kF2) { globals.redraw=1;globals.hideBuildings=globals.hideBuildings==0; }
    else if (code==kF3 && !(event.code&(InputCode::kControl|InputCode::kShift))) Required(hooks.queue)(kToggleBottom,0,2);
    return true;
}
}
