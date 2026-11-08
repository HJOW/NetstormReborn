// Config의 {&경로} 치환을 원문 Windows 레지스트리 기본값 조회에 연결한다.
#pragma once
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::platform { // 내부 UTF-8 설정과 실제 Windows A API의 현재 ACP 사이를 연결한다.
// HKEY이름\키#하위키의 기본 REG_SZ를 원문 RegOpenKeyA/RegQueryValueA·80BYTE 한도로 읽는다.
// 경로/값은 UTF-8이며 조회 실패는 빈 optional이다. 잘못된 HKEY만 invalidRoot를 호출하고 복귀 뒤 NULL HKEY 조회를 계속한다.
// 근거: 10.78 004dda10, CD10.72 004bebe0, R10.71 00481600. 열린 키 반납과 물리 범위 검사는 현대 호스트 경계다.
std::optional<std::string> ReadLegacyRegistryValue(std::string_view path,const std::function<void()>& invalidRoot);
}
