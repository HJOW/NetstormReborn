// 실제 VFX mapped RLE의 소스 색 치환을 기존 보호 셰이프 DB에 연결한다.
#pragma once
#include "client/VFXDraw.h"

namespace netstorm::client {
//0040229f: plain과 같은 투명/양 끝 포함 pane으로 불투명 소스 색만256바이트 표에서 치환한다.
// 반환값은 DrawShape와 같은0/-1/-2/-3/-4다. 색0/255도 불투명 픽셀이면 치환한다.
int DrawMappedShape(std::span<std::uint8_t> destination,int width,int height,const ShapeDatabase& database,
    std::size_t block,std::size_t frame,int x,int y,ShapeRect pane,std::span<const std::uint8_t,256> colors);
}
