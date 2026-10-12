// 원본 VFX 변환 커널의 회전 없는 16.16 확대·축소를 현재 8비트 표면에 복원한다.
#pragma once
#include "client/Renderer.h"
#include "client/Screen.h"
#include "client/VFXDraw.h"

namespace netstorm::client {
// 004028cb/CD 004662ab의 angle=0, scratch 새로 작성 분기다. position은 프레임 기준점이다.
// 배율은 signed 16.16이며 음수는 축 반전이다. clip은 half-open 화면 좌표다.
// 일반 변환은 프레임/변환 끝점 signed WORD, 각 축 간격 32767 이하만 지원한다.
// 원본 고정소수점 감김으로 버퍼 밖을 읽는 극단 입력은 범위 오류로 거부한다.
void DrawScaledVfxImage(std::span<std::uint8_t> pixels,int pitch,int height,
    const IndexedImage& image,const ShapeFrame& frame,ScreenPoint position,
    std::int32_t scaleX,std::int32_t scaleY,ScreenRect clip,const ColorMap* colors=nullptr);
}
