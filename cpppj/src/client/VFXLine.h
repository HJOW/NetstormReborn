// 원본 VFX 일반 선의 단색 8비트 경로를 클립/끝점/32비트 분수 위상까지 복원한다.
#pragma once
#include "client/Screen.h"

namespace netstorm::client {
// 004011f1/CD 00464bd1의 mode 0이다. clip은 half-open, 선 끝점은 양 끝 포함이며 색은 BYTE로 쓴다.
// 기울기 min/max의 0.32 분수·원래 시작점 기준 클립/위상을 유지한다. 색 표/콜백 mode는 포함하지 않는다.
void DrawVfxLine(std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip,
    ScreenPoint first,ScreenPoint last,std::uint32_t color);
}
