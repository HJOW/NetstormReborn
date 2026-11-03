// 원문 글꼴 캐시의 순차 I/O와 부분 설치·File 정리 순서다.
#pragma once
#include "o/RawBaseFileIO.h"
#include <array>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::client { // GDI/실제 VFS와 분리한 글꼴 파일 부모다.
inline constexpr std::size_t kFontCacheOwnerBytes=4112; // 네 머리 DWORD와 네256항목 표의 원문0x1010BYTE다.
struct FontCacheFileHooks { // 같은 글꼴 저장소와 파일 장치를 빌린다.
    std::function<bool(std::string_view)> exists; // reader의 내용 없는 최초 Probe다.
    std::function<void(o::RawBaseFileState&,std::string_view,std::uint32_t,std::uint32_t)> open; // reader0/writer2·create1의 File 생성이다.
    std::function<std::span<std::uint8_t>(std::uint32_t,std::uint32_t)> allocate; // reader의 실제 code 슬롯 설치 뒤 exactLength 쓰기 물리다.
    std::function<std::span<const std::uint8_t>(std::uint32_t,std::uint32_t)> glyph; // writer가 현재 표에서 읽은 길이의 같은 글리프 물리다.
    std::function<void(std::uint32_t)> assertion; // charsetLen==256의 Screen.cpp b66 진단이다.
};
// reader는 일부 실패에서도 owner와 앞선 글리프 설치를 되돌리지 않는다. signature seed는 명시 작업 물리다.
bool ReadFontCacheFile(o::RawBaseFileState& state,std::string_view name,std::span<std::uint8_t,kFontCacheOwnerBytes> owner,
    const o::RawBaseFileIOHooks& io,const FontCacheFileHooks& hooks,std::span<std::uint8_t> scratch,
    std::array<std::uint8_t,16>& signature,std::array<std::uint8_t,4>& charset);
// writer는 서명/머리/개수/네 표/256글리프를 순서대로 쓰고 명시 Close 뒤 소멸한다.
bool WriteFontCacheFile(o::RawBaseFileState& state,std::string_view name,std::span<const std::uint8_t,kFontCacheOwnerBytes> owner,
    const o::RawBaseFileIOHooks& io,const FontCacheFileHooks& hooks);
}
