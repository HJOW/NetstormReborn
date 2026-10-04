// 원본 팔레트 로더는 COL 헤더 내용보다 파일 크기로 포맷을 결정한다.
#include "client/Screen.h"
#include <stdexcept>

namespace netstorm::client {
// COL은 RGB 순서이고 1024바이트 파일은 Windows RGBQUAD의 BGRX 순서다.
GamePalette::GamePalette(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != 0x308 && bytes.size() != 0x400) throw std::runtime_error("Unsupported game palette size");
    // 256개의 원본 색 번호를 변경하지 않고 변환한다.
    for (std::size_t i = 0; i < colors_.size(); ++i) {
        if (bytes.size() == 0x308) colors_[i] = {bytes[8+i*3], bytes[9+i*3], bytes[10+i*3]};
        else colors_[i] = {bytes[i*4+2], bytes[i*4+1], bytes[i*4]};
    }
}
// 팔레트 내부에는 투명 색을 강제로 지정하지 않는다.
PaletteColor GamePalette::Color(std::uint8_t index) const { return colors_[index]; }
}
