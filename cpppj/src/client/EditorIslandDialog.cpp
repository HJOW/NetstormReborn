// 원문004d7bab의 공통 release 관문 뒤 실제 전용 factory를 호출한다.
#include "client/EditorIslandDialog.h"
#include <stdexcept>

namespace netstorm::client { //메뉴 처리는 x/y를 읽지 않으며 부모가 정한 창 크기를 유지한다.
bool DispatchEditorIslandDialog(const InputEvent& event,const std::function<void()>& createAttachControlled) {
    if ((event.code&0x03ffffffU)!=kAddIslandCommand) return false;
    if (!(event.code&0x80000000U)) { if (!createAttachControlled) throw std::logic_error("Add Island의 실제00480090 창 생성 장치 미연결");createAttachControlled(); }
    return true;
}
}
