// 새로 쓰는 코드: 호출자가 정한 위치에 바이트를 쓴다. 원본 폴더의 쓰기 허용 여부는 호출자가 검사한다.
#include "platform/FileOutput.h"
#include <fstream>
#include <stdexcept>

namespace netstorm::platform {
// 줄바꿈 변환 없이 쓴다.
void WriteFileBytes(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) {
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) throw std::runtime_error("Unable to open for writing: " + path.string());
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file) throw std::runtime_error("Unable to write: " + path.string());
}
}
