// 복원 결과를 외부 이미지 도구에서 검토할 수 있게 하는 새 출력 어댑터.
#pragma once
#include <cstdint>
#include <filesystem>
#include <span>

namespace netstorm::platform {
// RGBA 픽셀을 투명도를 갖는 BMP V4(BGRA, 위에서 아래로 저장) 파일로 기록한다.
void WriteBitmap(const std::filesystem::path& path, std::uint32_t width, std::uint32_t height,
    std::span<const std::uint8_t> rgba);
}
