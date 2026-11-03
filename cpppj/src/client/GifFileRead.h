// 원문004dae60의 정확한 입력할당·치수 선행쓰기·decoder 전후 File 수명을 분리한다.
#pragma once
#include "client/VFXDraw.h"
#include "o/RawBaseFileIO.h"
#include <functional>
#include <string_view>

namespace netstorm::o { class BaseFileSystem; } // 현재 VFS의 같은 고리와 출처 정책을 빌린다.
namespace netstorm::client { // 값형 GIF 해독과 실제 File 부모를 구별한다.
struct GifFileAllocation { // 실제 malloc 반환 토큰과 요청 길이의 같은 물리 할당이다.
    std::uintptr_t token{}; // 0은 원문 NULL 할당이며 빈 길이와 구별한다.
    std::span<std::uint8_t> bytes; // 최소4나 추가 NUL 없이 정확한 입력 길이를 빌린다.
};
struct GifFileReadHooks { // 원문 부모가 실제 호출하는 할당/열기/해독 경계만 제공한다.
    std::function<void(o::RawBaseFileState&,std::string_view,std::uint32_t,std::uint32_t)> open; //41b4c0(name,0,1)의 현재 File이다.
    std::function<GifFileAllocation(std::uint32_t)> allocateInput; //43fa80(exactLength)의 토큰과 같은 물리다.
    std::function<void(std::uintptr_t)> releaseInput; // 정상 Decode 뒤에만 입력43fa90을 호출한다.
    std::function<std::uintptr_t(std::uint32_t)> allocateBitmap; // 치수 출력 뒤 width*height의 별도 할당이다.
    std::function<void(std::uintptr_t,std::span<const std::uint8_t>,std::uint32_t,std::uint32_t)> decode; //406ef7 경계이며 호출 중에도 File/key는 살아 있다.
    std::function<void(std::uint32_t)> assertion; //Vfxdraw.cpp의fb/102/10c 원문 행 번호다.
};
std::uint32_t GifFileDimensions(std::span<const std::uint8_t> bytes); //407171의 descriptor WORD 폭/높이를 packed 반환한다. 서명/marker/LZW 검사는 decoder가 유지한다.
std::uintptr_t ReadGifFile(o::RawBaseFileState& state,std::string_view name,std::int32_t& width,std::int32_t& height,const o::RawBaseFileIOHooks& io,const GifFileReadHooks& hooks,std::span<std::uint8_t> scratch={}); // 정확 길이/0fill/Read(-1)→두 출력→bitmap→Decode→input free→File 소멸이다.
struct GifBitmapHooks { // 현재 Kencloud 주소 공간 또는 Background 이미지 소유자를 빌린다.
    std::function<std::uintptr_t(std::uint32_t)> allocate; // 이미 쓴 width/height를 보는 실제 bitmap 할당이다.
    std::function<void(std::uintptr_t,std::span<const std::uint8_t>,std::uint32_t,std::uint32_t)> decode; // 기존 색 번호 decoder를 같은 할당에 연결한다.
};
std::uintptr_t ReadBackgroundGifFile(const o::BaseFileSystem& files,std::string_view name,std::int32_t& width,std::int32_t& height,const GifBitmapHooks& bitmap); // 현재 Unicode FILE/key를 Decode 뒤까지 보유하는 Native 전용 부모다.
IndexedImage ReadBackgroundGifFileImage(const o::BaseFileSystem& files,std::string_view name,std::int32_t& width,std::int32_t& height); // Background의 기존 IndexedImage 소유를 유지하며 실패 전 출력 치수를 보존한다.
}
