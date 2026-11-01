// 반복 키의 카운터는 bool이나 프레임별 상태가 아니라 Client 수명의 원문005caf40 DWORD다.
#include "client/PriestSelectionShortcut.h"
#include <stdexcept>

namespace netstorm::client { //P/R는 같은 조회/카운터를 공유하고 별도 화면 클릭을 만들지 않는다.
namespace { //도달한 명령 장치만 요구한다.
template<class F> const F& PriestNeed(const F& hook) { if (!hook) throw std::logic_error("사제 선택 단축키 장치 미연결");return hook; } //선행 count 쓰기와 선택 효과를 오류로 되돌리지 않는다.
}
void SelectPriestByKey(std::uint32_t& selected,std::uint32_t& repeated,const PriestSelectionShortcutHooks& hooks) { //조회가 현재 selected를 바꿔도 비교는 조회 전에 캡처한 SID다.
    const auto owner=PriestNeed(hooks.localOwner)();const auto previous=selected;const auto candidate=PriestNeed(hooks.find)(owner);
    if (candidate!=previous) repeated=0;
    else if (repeated) { const auto currentOwner=PriestNeed(hooks.localOwner)();const auto command=PriestNeed(hooks.viewCommand)();PriestNeed(hooks.queue)(command,currentOwner,currentOwner);return; }
    PriestNeed(hooks.select)(candidate,0,1);++repeated;
}
}
