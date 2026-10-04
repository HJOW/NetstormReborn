// 새로 쓰는 코드: 클론이 만든 파일(설정 저장·검사 보고서)을 디스크에 쓴다.
#pragma once
#include <cstdint>
#include <filesystem>
#include <span>

namespace netstorm::platform {
// 바이트를 그대로 파일에 쓴다. 상위 폴더가 없으면 만든다. 실패하면 예외를 던진다.
void WriteFileBytes(const std::filesystem::path& path, std::span<const std::uint8_t> bytes);
}
