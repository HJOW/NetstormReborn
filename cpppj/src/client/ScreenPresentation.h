// 원본과 다른 Windows10/11 전체화면의 결정적인 최근접 픽셀 확대 경계다.
#pragma once
#include <cstdint>
#include <span>
namespace netstorm::client { // 장치별 GDI 확대 반올림과 논리 원본8비트 픽셀을 분리한다.
void ScrollScreenPixels(std::span<std::uint8_t> pixels,int pitch,int width,int height,int x,int y,int regionWidth,int regionHeight,int dx,int dy); //4a1ad0 DIB 경로의 겹친 행 memmove를 같은 원시 인덱스 버퍼에 수행한다.
void ScaleScreenPresentation(std::span<const std::uint8_t> source,int sourcePitch,int sourceWidth,int sourceHeight,
    std::span<std::uint8_t> destination,int destinationPitch,int destinationWidth,int destinationHeight); // 양수 크기/실제 행 범위를 검사하고 픽셀 중심의 최근접 표본을 정수로 복사한다. 목적지 padding은 유지한다.
}
