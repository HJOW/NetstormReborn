// 대본 AddRandom의 실제 타입 해석과 일반 입력 관문을 제공한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>

namespace netstorm::client { // HTML/대본과 현재 월드의 배치 부모를 연결한다.
inline constexpr std::uint32_t kAddRandomCommand=0x02000098U; // 원본00542954의 명령 번호다.
// 키 뗌과 일반 모드는 소비만 하고, signed y>255일 때만 문자열 타입 조회를 한다.
bool DispatchAddRandomInput(const InputEvent& event,bool editor,bool mission,
    const std::function<std::int32_t(std::uint32_t)>& lookup,
    const std::function<void(std::uint32_t,std::uint32_t)>& add);
}
