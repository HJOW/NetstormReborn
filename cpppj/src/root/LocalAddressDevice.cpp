// 외부 DNS를 사용하지 않고 실제 Windows 로컬 어댑터에서 LAN 주소를 공급한다.
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

namespace netstorm::root { // 구형 gethostname/gethostbyname의 외부 조회 경계를 Windows10 로컬 장치로 대체한다.
using namespace challenge_detail; // 실제20BYTE의 family/IP/flags 지정 폭을 공유한다.
namespace { // 실제 어댑터와 현재 컴퓨터 이름만 선택한다.
bool LocalIP(std::uint32_t network) { const auto value=ntohl(network),first=value>>24,second=(value>>16)&255U;return first==127 || first==10 || (first==172 && second>=16 && second<=31) || (first==192 && second==168) || (first==169 && second==254); } // 현재 SocketDevice와 같은 loopback/RFC1918/link-local 범위다.
bool SameName(std::wstring_view first,std::wstring_view second) { return CompareStringOrdinal(first.data(),static_cast<int>(first.size()),second.data(),static_cast<int>(second.size()),TRUE)==CSTR_EQUAL; } // DNS 이름의 대소문자 차이만 구별하지 않는다.
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
    if (name.size()>79) return std::nullopt;const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,name.data(),static_cast<int>(name.size()),nullptr,0);if (count<=0) return std::nullopt;std::wstring wide(static_cast<std::size_t>(count),L'\0');if (MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,name.data(),static_cast<int>(name.size()),wide.data(),count)!=count) return std::nullopt;if (SameName(wide,L"localhost")) return 0x0100007fU;bool matches=false; // 표준 localhost IPv4는 DNS 없이 loopback으로 해석하며 실제 로컬 주소 조회의 실패 기본값으로 쓰지 않는다.
    for (const auto format:{ComputerNameDnsHostname,ComputerNameDnsFullyQualified,ComputerNameNetBIOS}) { // 현재 OS의 컴퓨터 이름을 읽는 API이며 DNS 서버에 이름을 질의하지 않는다.
        DWORD length=256;std::array<wchar_t,256> buffer{};if (GetComputerNameExW(format,buffer.data(),&length) && SameName(wide,{buffer.data(),length})) matches=true;
    }
    if (!matches) return std::nullopt;std::array<std::uint8_t,20> address{};if (!Local(address)) return std::nullopt;return Read(address,4);
}
}
