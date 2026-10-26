// 저장/해제/이름 조회/선택 모드의 순서를 원본004d81d0과 같이 유지한다.
#include "client/EditorTestBattle.h"
#include <stdexcept>

namespace netstorm::client { // 실제 큰 소비자는 호출자가 같은 Client 수명에 연결한다.
namespace { // 선택되지 않은 경계에는 빈 성공 함수를 요구하지 않는다.
template<class Function> const Function& Need(const Function& function) { //선택된 실제 하위가 없으면 앞선 부분 효과를 유지한 채 진단한다.
    if (!function) throw std::logic_error("편집 Test Battle의 실제 수명 경계 미연결");return function;
}
}
bool DispatchEditorTestBattle(const InputEvent& event,std::uint32_t& selectionMode,const EditorTestBattleHooks& hooks) { //x가 아닌 y의 모든 비영 비트를 같은 복귀 분기로 처리한다.
    if ((event.code&0x03ffffffU)!=kEditorTestBattleCommand) return false;
    if (event.code&InputCode::kRelease) return true;
    if (!event.y) {
        Need(hooks.snapshot)();
        BeginGuideMission(Need(hooks.currentFortName)(),hooks.guide);
        Need(hooks.guide.mode)(1); //공장 정상 반환 뒤의 현재 포인터를 쓰며 생성자 인자로 앞당기지 않는다.
    } else {
        CloseGuideMission(hooks.guide);
        Need(hooks.pending)(Need(hooks.currentFortName)());
        selectionMode=2;Need(hooks.requestEdit)(); //pending 실패 때는 이전 모드가 남고, 요청 실패 때는 이미 쓴2가 남는다.
    }
    return true;
}
}
