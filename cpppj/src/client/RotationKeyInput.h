// C/c가 현재 회전 방향 DWORD를 먼저 반전하고 같은 Config에 기록하는 원문 입력이다.
#pragma once
#include "client/InputEvent.h"
#include <functional>
#include <string_view>

namespace netstorm::client { // 메뉴/커서와 동일한 Client 옵션 수명을 사용한다.
struct RotationKeyInputHooks { // 큰 정지/설정 장치는 실제 입력 호출자가 공급한다.
    std::function<bool()> paused; // 원문00460de0의 현재 정지 깊이 판정이다.
    std::function<void(std::string_view)> configure; // 원문00441580에 전달하는 동일 rotmode=%d 할당문이다.
};
// 뗌/다른 사건은 미소비이며 정지된 C는 효과 없이 소비한다. 설정 실패 때도 앞선 DWORD 쓰기를 되돌리지 않는다.
bool DispatchRotationKeyInput(InputEvent event,std::uint32_t& rotationMode,const RotationKeyInputHooks& hooks);
}
