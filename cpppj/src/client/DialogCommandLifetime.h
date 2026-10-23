// 원본 HTML 부모의 선택 명령을 같은 입력 큐에 먼저 예약한 뒤 그 부모만 Kill한다.
#pragma once
#include "client/DialogExitCommand.h"
#include <functional>

namespace netstorm::client {
// c79b0의 발화 경계다. command/parameter는 c83b0 메타 처리 때 이미 해석한 값이다.
// Instant 여부와 입력 종류 판정은 호출자가 맡으며, 모든 발화는 같은 예약/부모 반납 순서를 사용한다.
void FireDialogCommand(std::uint32_t parent,DialogExitCommandState selected,
    const std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)>& queue,
    const std::function<void(std::uint32_t)>& kill);
// 실제 40칸 입력 큐에 DWORD 두 인자를 보존하여 예약한다. OnExit는 kill 안에서 즉시 실행하지 않는다.
void FireDialogCommand(std::uint32_t parent,DialogExitCommandState selected,InputQueue& queue,
    const std::function<void(std::uint32_t)>& kill);
}
