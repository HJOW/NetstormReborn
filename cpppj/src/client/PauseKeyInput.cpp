#include "client/PauseKeyInput.h"
#include <stdexcept>

namespace netstorm::client { // 실제 정지 변경은 다음 일반 Pause 명령에 남긴다.
// 004d8dd3..004d8df1의 현재 명령 조회→0/0 예약이며 정지 중에도 같은 큐를 사용한다.
bool DispatchPauseKeyInput(InputEvent event,const PauseKeyInputHooks& hooks) {
    if ((event.code&InputCode::kRelease) || (event.code&0x03ffffffU)!=0x130000U) return false;
    if (!hooks.command) throw std::logic_error("Pause 키의 현재 명령 조회 미연결");
    const auto command=hooks.command(); // 조회 장치가 실행한 효과는 이후 예약 누락 때도 보존한다.
    if (!hooks.queue) throw std::logic_error("Pause 키의 일반 입력 큐 미연결");
    hooks.queue(command,0,0);return true;
}
}
