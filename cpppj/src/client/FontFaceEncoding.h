// Windows10/11의 현재 ANSI 글꼴 이름만 UTF-8 Config/VFS 경계와 변환한다.
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::client { //원문 Enum/31BYTE/current/GDI A는 ACP 그대로 유지한다.
//현재 OS ACP(0) 또는 명시 코드 페이지의 첫NUL까지 UTF-8로 옮긴다. 잘린DBCS는 거부한다.
std::string FontFaceToUtf8(std::string_view ansi,std::uint32_t codePage=0);
//UTF-8 Config의 첫NUL까지 현재 ACP로 옮기며 best-fit/기본문자 손실대체를 거부한다.
std::string FontFaceFromUtf8(std::string_view utf8,std::uint32_t codePage=0);
//CHFNT의 raw TAFF 이름을 보존하며 디스크에만 UTF-8을 공급한다. 잘린ACP는 그 디스크 후보의 실패다.
std::optional<std::string> FontCacheDiskName(std::string_view ansi,std::uint32_t codePage=0);
}
