// 원본 VFX 타원 윤곽의 두 정수 구간·클리핑·퇴화 선을8비트 표면에 복원한다.
#pragma once
#include "client/Screen.h"

namespace netstorm::client { // 실제 원본 픽셀 래스터만 제공한다.
// PATCH00405071/CD00468a51의 단색 경로다. 중심은 절대 좌표, clip은 반개방이며 색의 하위 BYTE만 쓴다.
// signed 음수 반경도 원문 정수 분기로 처리한다. 하나의 반경이0이면 기존 원문 VFX 선 경로를 사용한다.
void DrawVfxEllipse(std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip,
    ScreenPoint center,std::int32_t radiusX,std::int32_t radiusY,std::uint32_t color);
}
