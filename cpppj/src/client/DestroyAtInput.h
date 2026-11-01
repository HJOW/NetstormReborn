// 대본 DestroyAt의 일반 입력을 같은 현재 월드의 문자열 삭제 부모에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>

namespace netstorm::client { // 원문 명령 큐와 실제 Session 사이의 입력 관문이다.
inline constexpr std::uint32_t kDestroyAtCommand=0x020000e1U; // 원본00542a78/DestroyAt의 현재 코드다.
// Edit/미션 중 누름만 y의 문자열 토큰을 부모에 전달하고 다른 명령은 false다.
bool DispatchDestroyAtInput(const InputEvent& event,bool editor,bool mission,const std::function<void(std::uint32_t)>& destroyAt);
}
