// QuickHelp 최초 파일만 명시41baa0의 필수 읽기/추가2BYTE 계약에 연결한다.
#pragma once
#include "o/RawBaseFileWholeRead.h"
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::client { // 값형 VFS snapshot과 구별한 실제 Help 파일 접합 경계다.
class Client; // 파일/이름 고리의 같은 장기 소유자를 빌린다.
std::uintptr_t ReadQuickHelpFileBytes(o::RawBaseFileState& state,std::span<const std::uint8_t> name,std::int32_t& length,const o::RawBaseFileIOHooks& io,const o::BaseFileWholeReadHooks& hooks,std::span<std::uint8_t> scratch={}); // 4c5380처럼 length0/check0/extra2로41baa0을 호출한다. LF/NUL과 성공 캐시는 기존 QuickHelpWindow가 소유한다.
std::optional<std::string> ReadClientQuickHelpFile(const Client& client,std::string_view name); // 실제 Client의 현재 root/TAFF FILE/키를 읽고 논리 본문만 반환한다. 미발견 typed 예외를 optional 없음으로 바꾸지 않는다.
}
