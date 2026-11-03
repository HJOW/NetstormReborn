// 실제 FontStore를 현재 FILE의 순차 캐시 reader/writer와 연결한다.
#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string_view>
#include <vector>
namespace netstorm::o { class BaseFileSystem; } // 현재 등록/고리/디스크 root를 빌린다.
namespace netstorm::client { // raw ACP 캐시 이름과 Unicode OS 경로의 연결이다.
std::optional<std::vector<std::uint8_t>> LoadNativeFontCache(const o::BaseFileSystem& files,std::string_view name); // 부재/서명/버전 실패는 빈 값이며 현재 FILE에서 순차 읽는다.
bool SaveNativeFontCache(const o::BaseFileSystem& files,std::string_view name,std::span<const std::uint8_t> bytes); // 같은 생성/보관 글리프를 저장한다. 보호원본/쓰기불가 설치는false이며 실제 쓰기 성공과 구별한다.
bool IsProtectedFontCachePath(const std::filesystem::path& path); // canonical 경로가 개발 저장소 originals/originalCD 안인지 읽기만 확인한다. 보호판정 실패도 쓰기를 막는다.
}
