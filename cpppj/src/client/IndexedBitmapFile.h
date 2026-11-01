// 실제 파일 장치를 원문8비트 BMP 본체에 연결한다.
#pragma once
#include "client/RawIndexedBitmap.h"
#include <filesystem>

namespace netstorm::client { // 파일 경로는 호출자가 원문 root/설정 정책으로 선택한다.
struct IndexedBitmapArea { // 같은 픽셀 물리 안의 원문 자르기와 줄 간격이다.
    std::uint32_t x{},y{},width{},height{},pitch{}; // 정수 곱셈/위치는 본체에서32비트로 처리한다.
};
// 부모 디렉터리를 만들거나 파일을 잘라내지 않는다. 기존 BaseFile mode1/create1·void쓰기를 공유한다.
void WriteIndexedBitmapFile(const std::filesystem::path& path,std::span<const std::uint8_t> pixels,IndexedBitmapArea area,
    std::span<const std::uint8_t> logicalRgbPalette,IndexedBitmapScratch& scratch);
}
