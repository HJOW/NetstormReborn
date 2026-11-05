// 연소점 목표 개수의 원문 DWORD 감김과 signed IDIV를 보관한다.
#pragma once
#include "o/RiftType.h"

namespace netstorm::o { // 실제 입자/픽셀 접근과 순수 정수 부모를 구별한다.
std::int32_t BurningSourceCount(OriginalEdition edition,std::int32_t hitPoints,std::int32_t maximum,std::int32_t width,std::int32_t height); // HP는 getter 전 저장값이다. flag400 강제1은 호출자가 이 계산 뒤 현재 TYPE에서 선택한다.
}
