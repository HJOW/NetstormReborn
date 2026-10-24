// 원본 VFXDraw.cpp 004dae60·00406ef7의 배경 GIF 읽기 경계. 색 번호를 바꾸지 않는다.
#pragma once
#include "client/VFXDraw.h"

namespace netstorm::client {
// GIF87a/89a의 첫 이미지·LZW·인터레이스를 해독한다. 원본처럼 화면 팔레트를 그대로 사용한다.
IndexedImage DecodeGif(std::span<const std::uint8_t> bytes);
// 원문407171/406ef7처럼 첫 이미지의 폭·높이와 행만 해독하며 논리 화면/좌상단 오프셋은 사용하지 않는다.
IndexedImage DecodeBackgroundGif(std::span<const std::uint8_t> bytes);
}
