// 원본 Guide/Gump 앞 F12의 현재 전체화면 마우스 모드 전환을 실제 장치에 전달한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>

namespace netstorm::client { // 실제 사건과 현재 화면 장치 경계만 사용한다.
// 실제 전환 때만 true이며 호출자는 같은 사건의 Guide/Gump 처리를 계속한다. 정지/수정키 관문은 없다.
bool DispatchScreenMouseKey(const InputEvent& event,const std::function<std::uint32_t()>& screenMode,const std::function<void(std::uint32_t)>& changeMode);
}
