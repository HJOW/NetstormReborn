// 원문 Pause 가상 키의 일반 Pause 명령 예약을 정지 조회와 분리한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>

namespace netstorm::client { // Guide/Gump/Console을 지난 현재 일반 입력만 받는다.
struct PauseKeyInputHooks { // 원문의 현재 명령 DWORD와 같은 일반 FIFO를 빌린다.
    std::function<std::uint32_t()> command; // 00542a30의 현재 Pause 명령을 예약 직전에 조회한다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> queue; // 00452fe0에 원문 두 인자0/0을 전달한다.
};
bool DispatchPauseKeyInput(InputEvent event,const PauseKeyInputHooks& hooks); // 누름130000만 예약한다. Shift/Control/Alt/outside/pressed는 거부하지 않으며 현재 정지를 읽지 않는다.
}
