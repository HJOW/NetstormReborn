// 원본 일반 자산 출력의 추상/건설/그림자 소스 색 변환표다.
#pragma once
#include "client/Renderer.h"
namespace netstorm::client {
// 세 표는 동일한 파일 팔레트에서 만들며 일시적인 날씨 RGB 변경에는 다시 계산하지 않는다.
struct SpriteColorMaps {
    ColorMap bright{},dead{},shadow{}; // 556238/556438/557738의256바이트 표다.
};
// 논리 RGB256색을 원본 거리·후보·float 최소값 순서로 변환한다.
SpriteColorMaps BuildSpriteColorMaps(std::span<const std::uint32_t,256> logical);
}
