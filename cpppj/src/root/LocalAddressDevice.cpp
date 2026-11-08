// 실제 Windows 로컬 어댑터 주소와 첫 IPv4의 LAN 범위를 지키는 OS 이름 조회를 공급한다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Winsock1/2 선언 충돌을 피한다.
#endif
#ifndef NOMINMAX
#define NOMINMAX // 표준 함수와 Windows 매크로 충돌을 피한다.
#endif
#include <winsock2.h>
#include <iphlpapi.h>
#include "root/AddressCodec.h"
#include "root/ChallengeDetail.h"
#include "platform/LanIpv4Name.h" // Client와 같은 첫 gethostbyname 주소/개별 WSA 참조를 사용한다.
#include <cstring> // 첫 IPv4의 network 바이트를 별도 순서 변환 없이 DWORD로 복사한다.

namespace netstorm::root { // 로컬 자기 주소와 명시 hostname의 첫 LAN IPv4 조회를 구별한다.
using namespace challenge_detail; // 실제20BYTE의 family/IP/flags 지정 폭을 공유한다.
namespace { // 현재 Up 어댑터에서 선택할 LAN 주소 범위다.
bool LocalIP(std::uint32_t network) { const auto value=ntohl(network),first=value>>24,second=(value>>16)&255U;return first==127 || first==10 || (first==172 && second>=16 && second<=31) || (first==192 && second==168) || (first==169 && second==254); } // 현재 SocketDevice와 같은 loopback/RFC1918/link-local 범위다.
}
bool AddressCodec::Local(std::span<std::uint8_t,20> address) {
    ULONG size=15000;std::vector<std::uint8_t> bytes;
    for (;;) { // 어댑터 변경으로 더 큰 버퍼가 필요하면 실제 반환 크기로 다시 조회한다. WSA/소켓/DNS를 시작하지 않는다.
        bytes.resize(size);auto* adapters=reinterpret_cast<IP_ADAPTER_ADDRESSES*>(bytes.data());const auto status=GetAdaptersAddresses(AF_INET,GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER|GAA_FLAG_SKIP_FRIENDLY_NAME,nullptr,adapters,&size);if (status==ERROR_BUFFER_OVERFLOW) continue;if (status!=NO_ERROR) break;
        for (auto* adapter=adapters;adapter;adapter=adapter->Next) { // 실제 OS 반환 순서의 현재 Up 어댑터만 선택한다.
            if (adapter->OperStatus!=IfOperStatusUp) continue;
            for (auto* unicast=adapter->FirstUnicastAddress;unicast;unicast=unicast->Next) { // 첫 실제 LAN IPv4를 사용하며 임의 loopback 기본값을 만들지 않는다.
                if (!unicast->Address.lpSockaddr || unicast->Address.iSockaddrLength<static_cast<int>(sizeof(sockaddr_in)) || unicast->Address.lpSockaddr->sa_family!=AF_INET) continue;const auto ip=reinterpret_cast<const sockaddr_in*>(unicast->Address.lpSockaddr)->sin_addr.s_addr;if (!ip || !LocalIP(ip)) continue;std::fill(address.begin(),address.end(),std::uint8_t{});Put(address,0,2,2);Put(address,4,ip);return true;
            }
        }
        break;
    }
    Put(address,0,0,2);return false; // 원문 실패는 주소 전부가 아닌 family WORD만0으로 만든다.
}
std::optional<std::uint32_t> AddressCodec::ResolveLocalName(std::string_view name) {
    std::int32_t error{};const auto address=platform::ResolveLanIpv4Name(name,error);if (!address) return std::nullopt; // Parse는 원문처럼 이름 실패를IP0으로 바꾸며 기존20BYTE/포트/flags를 계속 작성한다.
    std::uint32_t network{};std::memcpy(&network,address->data(),address->size());return network; // 원문 첫 h_addr_list DWORD의 network 바이트를 host 연산으로 재정렬하지 않는다.
}
}
