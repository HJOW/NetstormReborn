// 원문 Detonate 입력은 실제 객체 가상6c를 호출한 뒤 그0반환도 일반 사건 끝으로 보낸다.
#pragma once
#include "client/InputEvent.h"
#include "o/SidPool.h"
#include <functional>

namespace netstorm::client { // 메뉴의 입력 수명과 실제 객체 입력을 연결한다.
inline constexpr std::uint32_t kDetonateBombCommand=0x03000074U; // 원본5428c8/DetonateBomb의 초기 토큰이다.
// release와0번 대상에는 하위를 요구하지 않고, 선택된 가상6c의 반환은 원문처럼 무시한다.
bool DispatchDetonateInput(const InputEvent& event,const std::function<void(o::Sid,std::uint32_t)>& objectInput);
}
