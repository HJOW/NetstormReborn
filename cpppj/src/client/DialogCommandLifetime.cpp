#include "client/DialogCommandLifetime.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client {
// 예약 콜백이 다른 페이지를 만들거나 메타를 바꾸어도 선택 당시 값과 원래 부모를 보존한다.
void FireDialogCommand(std::uint32_t parent,DialogExitCommandState selected,
    const std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)>& queue,
    const std::function<void(std::uint32_t)>& kill) {
    if (!queue || !kill) throw std::invalid_argument("HTML 선택의 실제 명령 큐/부모 Kill 경계 누락");
    queue(selected.command,selected.parameter,selected.parameter); // code0도 원본 c79b0처럼 예약하며 OnExit의 조건 분기를 복제하지 않는다.
    kill(parent); // 실제 소멸/OnExit 예약은 프레임 후반 Cleanup에 남긴다.
}
// 정수의 signed 표현을 바꾸지 않고 기존 InputQueue의 원본 순서/용량 정책을 그대로 사용한다.
void FireDialogCommand(std::uint32_t parent,DialogExitCommandState selected,InputQueue& queue,
    const std::function<void(std::uint32_t)>& kill) {
    FireDialogCommand(parent,selected,[&queue](auto command,auto first,auto second) {
        queue.PushCommand(command,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second));
    },kill);
}
}
