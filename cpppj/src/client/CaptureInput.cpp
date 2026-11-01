// 원문4d7da3..4d7dd3의 active→현재 param2 판정 순서를 보존한다.
#include "client/CaptureInput.h"
#include <stdexcept>
namespace netstorm::client { //Guide/Gump가 소비하지 않은 일반 입력만 전달받는다.
namespace { //선택하지 않은 실제 장치를 선행 요구하지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("Capture 일반 입력의 실제 선택 장치 미연결");return hook; } //파일/프레임 부작용은 선택한 Client 부모에 있다.
}
bool DispatchCaptureInput(InputEvent event,const std::uint32_t& active,const CaptureInputHooks& hooks) {
    if ((event.code&0x03ffffffU)!=kCaptureCommand) return false;
    if (event.code&InputCode::kRelease) return true;
    if (active) Need(hooks.stop)();else if (event.y==1) Need(hooks.start)();else Need(hooks.tell)("BeginCapture");
    return true;
}
}
