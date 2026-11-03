// 4a4850의 현재 File과 저장 팔레트 직접 기록·후단 뒤 반납을 연결한다.
#pragma once
#include "o/RawBaseFileIO.h"
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o { class BaseFileSystem; } // 실제 검색/현재 TAFF와 FILE 열기는 기존 VFS에 맡긴다.
namespace netstorm::client { // 값형 GamePalette와 별도인10.78 필수 파일 부모다.
struct PaletteFileReadHooks { // 부모가 도달한 장치만 요구하며 성공이나 전체 팔레트 사본을 합성하지 않는다.
    std::function<void(o::RawBaseFileState&,std::string_view,std::uint32_t,std::uint32_t)> open; // 같은 현재 File의 mode0/create1 열기다.
    std::function<void()> apply; // 직접 saved_ 기록 뒤 같은 Screen의 기존 clear/색 표 후단을 실행한다. 복귀까지 File은 열려 있다.
    std::function<void(std::uint32_t)> assertion; // 잘못된 길이는 Screen.cpp940 진단이며 복귀하면 이전 saved_로 후단을 계속한다.
};
// saved는 현재 Screen의 같은1024BYTE다. RGB COL은 예약BYTE를 보존하며 실패 prefix를 되돌리지 않는다.
void ReadPaletteFile(o::RawBaseFileState& state,std::string_view name,std::span<std::uint8_t,1024> saved,
    const o::RawBaseFileIOHooks& io,const PaletteFileReadHooks& hooks,std::span<std::uint8_t> scratch);
// 현재 Unicode FILE/키를 후단까지 보관하는 실제10.78 전용 어댑터다. 기존 snapshot/값형 parser를 바꾸지 않는다.
void ReadNativePaletteFile(const o::BaseFileSystem& files,std::string_view name,std::span<std::uint8_t,1024> saved,
    const std::function<void()>& apply);
}
