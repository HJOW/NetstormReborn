// Client와 R이 같은 OS 이름 조회와 첫 IPv4의 LAN 범위 판정을 사용하게 한다.
#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace netstorm::platform { // 게임 주소 문법과 분리된 Windows 호스트 이름 조회 경계다.
// 원문 Client004e0cd0/R00486870처럼 gethostbyname의 첫 주소만 복사한다.
// loopback/RFC1918/link-local이면 주소와 error0을 반환하며, 실패는 빈 값과 실제 Winsock 코드다.
// 이름은 원문 char 바이트를 그대로 전달한다. 비어 있거나79BYTE 초과/NUL 포함이면 조회하지 않는다.
// 호출 중 자기 WSA 참조만 얻고 반납하며 소켓·전송 owner의 기존 WSA 참조를 변경하지 않는다.
std::optional<std::array<std::uint8_t,4>> ResolveLanIpv4Name(std::string_view name,std::int32_t& error);
}
