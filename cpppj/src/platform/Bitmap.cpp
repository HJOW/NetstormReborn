// BMP 내보내기는 원본 게임에 없는 복원 검토용 기능이다.
#include "platform/Bitmap.h"
#include <bit>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace netstorm::platform {
namespace {
// 14바이트 파일 헤더와 108바이트 BITMAPV4HEADER의 합.
constexpr std::size_t kBitmapHeaderSize = 122;
// BMP의 작은 정수를 리틀 엔디언으로 기록한다.
void Put(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value, std::size_t size = 4) {
    // 정수의 낮은 바이트부터 지정된 크기만큼 쓴다.
    for (std::size_t i = 0; i < size; ++i) bytes.at(offset+i) = static_cast<std::uint8_t>(value >> (i*8));
}
}
// 음수 높이와 명시적인 색·알파 마스크로 상하 방향·투명도를 보존한다.
void WriteBitmap(const std::filesystem::path& path, std::uint32_t width, std::uint32_t height,
    std::span<const std::uint8_t> rgba) {
    const auto pixelSize = static_cast<std::uint64_t>(width)*height*4;
    if (width == 0 || height == 0 || width > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) || height > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())
        || pixelSize > std::numeric_limits<std::uint32_t>::max()-kBitmapHeaderSize || pixelSize != rgba.size())
        throw std::invalid_argument("Invalid bitmap dimensions or pixel buffer");
    std::vector<std::uint8_t> bytes(kBitmapHeaderSize+rgba.size());
    bytes[0] = 'B'; bytes[1] = 'M';
    Put(bytes, 2, static_cast<std::uint32_t>(bytes.size())); Put(bytes, 10, kBitmapHeaderSize);
    Put(bytes, 14, 108); Put(bytes, 18, width);
    Put(bytes, 22, std::bit_cast<std::uint32_t>(-static_cast<std::int32_t>(height)));
    Put(bytes, 26, 1, 2); Put(bytes, 28, 32, 2); Put(bytes, 30, 3);
    Put(bytes, 34, static_cast<std::uint32_t>(pixelSize));
    Put(bytes, 54, 0x00ff0000); Put(bytes, 58, 0x0000ff00); Put(bytes, 62, 0x000000ff); Put(bytes, 66, 0xff000000);
    Put(bytes, 70, 0x73524742); // 색 공간을 sRGB로 지정하여 외부 도구에서 색을 유지한다.
    // 팔레트를 적용한 RGBA를 BMP의 BGRA 순서로 변환한다.
    for (std::size_t i = 0; i < rgba.size(); i += 4) {
        bytes[kBitmapHeaderSize+i] = rgba[i+2]; bytes[kBitmapHeaderSize+i+1] = rgba[i+1];
        bytes[kBitmapHeaderSize+i+2] = rgba[i]; bytes[kBitmapHeaderSize+i+3] = rgba[i+3];
    }
    std::ofstream file(path, std::ios::binary);
    if (!file || !file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("Unable to write bitmap: "+path.string());
}
}
