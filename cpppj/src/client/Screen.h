// 원본 Screen.cpp의 팔레트 파일 읽기 부분. 화면 장치·전체화면 전환은 후속 복원 대상이다.
#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace netstorm::client {
struct PaletteColor { std::uint8_t red{}, green{}, blue{}; };
class GamePalette {
public:
    // 004a4850 ↔ CD 00424760: 0x308바이트 COL 또는 0x400바이트 BGRX 파일을 읽는다.
    explicit GamePalette(std::span<const std::uint8_t> bytes);
    // 원본 팔레트 번호에 대응하는 RGB 색을 반환한다.
    PaletteColor Color(std::uint8_t index) const;
private:
    std::array<PaletteColor, 256> colors_{};
};
}
