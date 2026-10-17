// 원본43c010의 커서 파괴/불법/글자 그림자 세 색 변환표다.
#pragma once
#include "client/Renderer.h"
namespace netstorm::client {
// 각 표는 현재 파일 논리 팔레트의256개 소스 번호를 출력 번호로 바꾼다.
struct PreviewColorMaps {
    ColorMap destroy; // 현재 기본 빨강의 하위 BYTE로 채운다.
    ColorMap illegal; // 빨강만2.1배/1제한 후 RGB 제곱 거리로 고른다.
    ColorMap labelShadow; // 원본처럼 모든 소스 번호를0으로 바꾼다.
};
// 원본 ColorSort와 같은 double 정규 RGB 및 float 최소 거리 저장을 사용한다.
PreviewColorMaps BuildPreviewColorMaps(std::span<const std::uint32_t,256> logical,std::uint32_t red);
}
