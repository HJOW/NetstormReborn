// 일반 좌클릭 선택 뒤 Ctrl+special의 단일 지점 가상70 순회를 복원한다.
#pragma once
#include "client/InputEvent.h"
#include <array>
#include <functional>

namespace netstorm::client { //투영/탐색/진단 하위는 같은 현재 월드에서 빌린다.
struct SpecialPointerTraceHooks {
    std::function<std::array<float,2>(std::int32_t,std::int32_t)> project; //4971e0의 사건좌표 역투영이며 폴링좌표로 바꾸지 않는다.
    std::function<std::uint32_t(std::array<float,2>)> begin; //4202f0의 단일 지점 탐색이며 처음 후보를 반환한다.
    std::function<void(std::uint32_t)> trace; //현재 객체의 직접 가상70이며 Form 재귀44dab0과 다르다.
    std::function<std::uint32_t()> next; //같은 finder의 다음 후보를 가상효과 뒤 조회한다.
};
void TraceSpecialPointer(bool control,std::uint32_t special,const InputEvent& event,const SpecialPointerTraceHooks& hooks); //4da263..2e6의 단락/순회이며 호출자는 Select 뒤의 현재special을 전달한다.
}
