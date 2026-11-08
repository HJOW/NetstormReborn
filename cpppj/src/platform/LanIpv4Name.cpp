// 원문의 첫 gethostbyname 주소 선택과11.01의 로컬 서버 접속 범위를 함께 보존한다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Winsock1/2의 중복 선언을 피한다.
#endif
#ifndef NOMINMAX
#define NOMINMAX // Windows 매크로가 표준 C++ 선언을 바꾸지 않게 한다.
#endif
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 원문과 같은 첫 h_addr_list 순서를 위해 이 조회에서만 gethostbyname을 사용한다.
#endif
#include <winsock2.h>
#include "platform/LanIpv4Name.h"
#include <cstring>
#include <string>

namespace netstorm::platform { // Client/R은 주소 해석 뒤 각자의 기존 실패·소켓 생성 효과를 계속한다.
namespace { // 공유 전송 수명과 독립인 이름 조회의 임시 WSA 참조만 소유한다.
constexpr std::size_t kOriginalHostNameBytes=79; // Client/R 원문80BYTE 지역 문자열의 NUL 앞 최대 폭이다.
// 성공한 WSAStartup 한 번만 반납한다. 기존 TCP/UDP/R owner의 참조는 그대로 남는다.
struct NameSocketScope {
    WSADATA data{}; // 이 이름 조회가 요청한 Winsock2.2의 실제 반환 자료다.
    std::int32_t error{WSAStartup(MAKEWORD(2,2),&data)}; // 시작 실패 코드는 정리 API 호출 전에 보존한다.
    ~NameSocketScope() { if (!error) WSACleanup(); } // 정상 반환과 예외 복귀 모두 자기 성공 참조만 반납한다.
};
// 이미 선택된 첫 IPv4만 검사하며 뒤의 사설 주소를 찾아 결과 순서를 바꾸지 않는다.
bool IsLanAddress(const std::array<std::uint8_t,4>& address) {
    return address[0]==127 || address[0]==10 || (address[0]==172 && address[1]>=16 && address[1]<=31) ||
        (address[0]==192 && address[1]==168) || (address[0]==169 && address[1]==254);
}
}
// 원문 char 이름으로 OS 조회를 한 번 수행하고 반환된 첫 주소를 WSA 정리 전에 복사한다.
std::optional<std::array<std::uint8_t,4>> ResolveLanIpv4Name(std::string_view name,std::int32_t& error) {
    error=0;
    if (name.empty() || name.size()>kOriginalHostNameBytes || name.find('\0')!=name.npos) { error=WSAEINVAL;return std::nullopt; }
    const std::string terminated(name); // view의 뒤 메모리를 읽지 않도록 실제 NUL 종료 C문자열을 소유한다.
    const NameSocketScope sockets;if (sockets.error) { error=sockets.error;return std::nullopt; }
    const auto* host=gethostbyname(terminated.c_str());
    if (!host) { error=WSAGetLastError();return std::nullopt; }
    if (host->h_addrtype!=AF_INET || host->h_length!=4 || !host->h_addr_list || !host->h_addr_list[0]) { error=WSANO_DATA;return std::nullopt; }
    std::array<std::uint8_t,4> address{}; // h_addr_list[0]의 network 순서 네 바이트를 그대로 보관한다.
    std::memcpy(address.data(),host->h_addr_list[0],address.size());
    if (!IsLanAddress(address)) { error=WSAEACCES;return std::nullopt; }
    return address;
}
}
