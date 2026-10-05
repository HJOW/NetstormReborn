// 원본 VFXDraw.cpp 004dae60·00406ef7의 배경 GIF 읽기 경계. 색 번호를 바꾸지 않는다.
#pragma once
#include "client/VFXDraw.h"

namespace netstorm::client {
// GIF87a/89a의 첫 이미지·LZW·인터레이스를 해독한다. 원본처럼 화면 팔레트를 그대로 사용한다.
IndexedImage DecodeGif(std::span<const std::uint8_t> bytes);
}
