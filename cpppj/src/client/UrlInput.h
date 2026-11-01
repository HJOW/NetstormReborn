// 원문 URL 명령의 현재 문자열을 기존4a00c0 링크 분배기에 전달한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>
#include <string_view>

namespace netstorm::client { // HTML/대본과 일반 입력이 같은 원문 번호를 공유한다.
inline constexpr std::uint32_t kUrlCommand=0x0200005bU; //00542e60의URL→00542864 전역 초기값이다.
// Guide/Gump 뒤 명령만 받는다. 키 뗌은 소비하며 나머지는두번째DWORD를문자열주소로해석한다.
bool DispatchUrlInput(const InputEvent& event,const std::function<std::string_view(std::uint32_t)>& text,
    const std::function<void(std::string_view)>& activate);
}
