// 반복 키의 카운터는 bool이나 프레임별 상태가 아니라 Client 수명의 원문005caf40 DWORD다.
#include "client/PriestSelectionShortcut.h"
#include <stdexcept>

namespace netstorm::client { //P/R는 같은 조회/카운터를 공유하고 별도 화면 클릭을 만들지 않는다.
namespace { //도달한 명령 장치만 요구한다.
template<class F> const F& PriestNeed(const F& hook) { if (!hook) throw std::logic_error("사제 선택 단축키 장치 미연결");return hook; } //선행 count 쓰기와 선택 효과를 오류로 되돌리지 않는다.
}
// P/R 키를 처리한다: 조회 전 선택 SID를 캡처하고, 후보가 다르면 반복 카운터를 0으로 두며, 같고 카운터가 비0이면 View 명령을 큐에 넣는다.
// 사용법: selected/repeated는 Client 수명의 선택 SID와 누적 카운터(005caf40)이며 hooks는 같은 현재 Session의 조회·선택 장치다.
// 10차 NB20: 건설 준비 조각이 커서에 붙어 있으면 사제를 고르기 전에 먼저 취소해 조각이 남지 않게 했다(원문 10.78은 취소 없이 선택만 했다).
void SelectPriestByKey(std::uint32_t& selected,std::uint32_t& repeated,const PriestSelectionShortcutHooks& hooks) { //조회가 현재 selected를 바꿔도 비교는 조회 전에 캡처한 SID다.
    if (hooks.pieceHeld && hooks.cancelPiece && hooks.pieceHeld()) hooks.cancelPiece(); //NB20: 취소 뒤의 현재 선택 상태를 아래에서 읽는다.
    const auto owner=PriestNeed(hooks.localOwner)();const auto previous=selected;const auto candidate=PriestNeed(hooks.find)(owner);
    if (candidate!=previous) repeated=0;
    else if (repeated) { const auto currentOwner=PriestNeed(hooks.localOwner)();const auto command=PriestNeed(hooks.viewCommand)();PriestNeed(hooks.queue)(command,currentOwner,currentOwner);return; }
    PriestNeed(hooks.select)(candidate,0,1);++repeated;
}
}
