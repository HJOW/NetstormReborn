// 주소의 원문 서식·문법과 실제 로컬 이름 해석을 구분해 같은20BYTE에 반영한다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Windows의 구 Winsock 선언 충돌을 피한다.
#endif
#ifndef NOMINMAX
#define NOMINMAX // 표준 함수와 Windows 매크로 충돌을 피한다.
#endif
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 원문의 축약/8진수/16진수 숫자 문법을 가진 inet_addr를 이 파일에서 유지한다. 더 엄격한 InetPton으로 바꾸지 않는다.
#endif
#include <winsock2.h>
#include "root/AddressCodec.h"
#include "root/ChallengeDetail.h"
#include <cstdio>
#include <cstdlib>

namespace netstorm::root { // 동일 주소 저장소와 원문5칸 문자열 고리를 사용한다.
using namespace challenge_detail; // 비정렬 little endian 필드의 지정 폭을 공유한다.
namespace { // 원문 strchr/포트와 hex 문자의 물리 경계다.
std::optional<std::size_t> Find(const std::array<char,80>& bytes,std::size_t from,char needle) {
    if (from>=bytes.size()) throw std::out_of_range("R 주소 strchr의 실제80BYTE 지역 범위");
    for (std::size_t at=from;at<bytes.size();++at) { if (bytes[at]==needle) return at;if (!bytes[at]) return std::nullopt; } // 현재 첫NUL 전까지 실제 지역 바이트만 탐색한다.
    throw std::out_of_range("R 주소 문자열의 실제 NUL 없음");
} // 미정 지역 읽기를 호스트 문자열로 합성하지 않는다.
void Port(std::span<std::uint8_t,20> address,std::int32_t port) { const auto at=Read(address,0,2)==6 ? 12U:2U;Put(address,at,static_cast<std::uint32_t>(port)>>8,1);Put(address,at+1,static_cast<std::uint32_t>(port),1); } // 원문의 u_short 절삭/htons 두BYTE다.
int Hex(unsigned char value) { if (value>='0' && value<='9') return value-'0';if (value>='A' && value<='F') return value-'A'+10;if (value>='a' && value<='f') return value-'a'+10;return -1; } // 원문 잘못된 nibble의 -1도 후속 BYTE 모듈러 쓰기에 사용한다.
}
std::string_view AddressCodec::Format(std::span<const std::uint8_t,20> address,bool port,bool protocol,bool flags) {
    auto& buffer=buffers_[next_];buffer[0]=0;next_=(next_+1)%buffers_.size();const auto mode=Read(address,16);std::string text;
    if (protocol) { if (mode&0x10U) text="tcp,";else if (mode&0x20U) text="udp,";else if (mode&0x400U) text="spx,";else if (mode&0x200U) text="ipx,";else if (mode&8U) text="zlocal,"; }
    if (Read(address,0,2)==6) {
        std::array<char,32> digits{};std::snprintf(digits.data(),digits.size(),"%X.%02X%02X%02X%02X%02X%02X",Read(address,2),unsigned(address[6]),unsigned(address[7]),unsigned(address[8]),unsigned(address[9]),unsigned(address[10]),unsigned(address[11]));text+=digits.data();
    } else if (!Read(address,4)) text+='*';else text+=std::to_string(address[4])+'.'+std::to_string(address[5])+'.'+std::to_string(address[6])+'.'+std::to_string(address[7]); // 원문 inet_ntoa와 같은 network BYTE 순서이며 주소0은 '*'다.
    if (port) { const auto at=Read(address,0,2)==6 ? 12U:2U;text+=':';text+=std::to_string((std::uint32_t(address[at])<<8)|address[at+1]); }
    if (flags && mode) { text+=',';if (mode&4U) text+='c';if (mode&2U) text+='s';if (mode&0x100U) text+='n';if (mode&1U) text+='b';if (mode&0x800U) text+='r'; }
    if (text.size()+1>buffer.size()) throw std::out_of_range("R 주소 서식의 실제30BYTE 고리 범위");std::copy(text.begin(),text.end(),buffer.begin());buffer[text.size()]=0;return {buffer.data(),text.size()};
}
std::array<std::uint8_t,20> AddressCodec::Parse(const std::optional<std::string>& text,std::int32_t port) {
    if (!text) throw std::out_of_range("R 주소 strncpy의 원문 NULL 읽기 거부");std::array<char,80> bytes{};const auto input=std::string_view(*text).substr(0,text->find('\0'));std::copy_n(input.begin(),std::min(input.size(),std::size_t{79}),bytes.begin());std::array<std::uint8_t,20> result{};std::size_t host{};
    if (const auto comma=Find(bytes,0,',')) { bytes[*comma]=0;host=*comma+1; }std::string_view extra;
    if (const auto comma=Find(bytes,host+1,',')) { bytes[*comma]=0;extra=bytes.data()+*comma+1; }
    const std::string_view protocol(bytes.data());std::uint32_t mode{};if (protocol=="tcp") mode=0x10;else if (protocol=="udp") mode=0x20;else if (protocol=="spx") mode=0x400;else if (protocol=="ipx") mode=0x200;else if (protocol=="zlocal") mode=8;
    if (extra.find('c')!=extra.npos) mode|=4;if (extra.find('s')!=extra.npos) mode|=0x82;if (extra.find('n')!=extra.npos) mode|=0x100;if (extra.find('b')!=extra.npos) mode|=1;if (extra.find('r')!=extra.npos) mode|=0x800;Put(result,16,mode);
    std::int32_t parsedPort{};if (const auto colon=Find(bytes,host,':')) { bytes[*colon]=0;parsedPort=std::atoi(bytes.data()+*colon+1); }if (!port) port=parsedPort;
    if (mode&0x30U) Put(result,0,2,2);else if (mode&0x600U) Put(result,0,6,2);else if (std::string_view(bytes.data())=="zlocal") Put(result,0,20,2); // 원문의 family 판정은 ':' 분리 뒤의 현재 C문자열을 다시 읽는다.
    if (bytes[host]=='*') { if (!Read(result,0,2)) { Put(result,0,20,2);Put(result,16,mode|8U); }Port(result,port);return result; }
    const std::string_view address(bytes.data()+host);
    if (std::count(address.begin(),address.end(),'.')==1) {
        Put(result,0,6,2);const auto dot=Find(bytes,host,'.'); // 원문 strtol의 시작은 network 앞부분이 아닌 '.' 포인터 자체다. 정상 문자열에서도 network 값은0이 된다.
        const auto network=static_cast<std::uint32_t>(std::strtol(bytes.data()+*dot,nullptr,16));for (std::size_t byte=0;byte<4;++byte) Put(result,2+byte,network>>(24-byte*8),1); // htonl의 network BYTE 순서다.
        const auto node=*dot+1;if (node+12>bytes.size()) throw std::out_of_range("R IPX node의 실제80BYTE 지역 범위");
        for (std::size_t byte=0;byte<6;++byte) result[6+byte]=static_cast<std::uint8_t>(Hex(static_cast<unsigned char>(bytes[node+byte*2]))*16+Hex(static_cast<unsigned char>(bytes[node+byte*2+1]))); // 정상 hex/잘못된 -1 nibble의 BYTE 결과를 모두 보존한다.
    } else {
        auto ip=static_cast<std::uint32_t>(::inet_addr(address.data()));if (ip==0xffffffffU) ip=ResolveLocalName(address).value_or(0);Put(result,0,2,2);Put(result,4,ip);
    }
    Port(result,port);return result;
}
void AddressCodec::Bind(LoginRequestHooks& login,ChallengeRuntimeHooks& challenge) {
    login.localAddress=[this] { std::array<std::uint8_t,20> address{};Local(address);return address; };challenge.addressText=[this](auto address,bool port) { return std::string(Format(address,port)); };challenge.parseAddress=[this](const auto& text) { return Parse(text); }; // callback 반환 전에 고리 문자열을 소유 복사한다.
}
}
