// 원문47BYTE 존/친구3BYTE 접근을 보존하고 실제10.71 compact 응답만 별도로 읽는다.
#include "o/RawRootLoginReply.h"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 상위 전송은 실제 패킷 경계를 제공하며 성공 상태를 합성하지 않는다.
namespace { // 비정렬 필드/부호와 선택된 장치의 물리 경계를 공유한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("Root0Fh의 실제 수신 장치 미연결");return function; } // 앞서 완료한 효과를 되돌리지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { // 현재 읽는 x86 폭만 요구한다.
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("Root0Fh의 실제 필드 물리 부족");
    std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; // 낮은 BYTE부터 조합한다.
}
void Write(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) { // 현재 기록의 지정 폭만 변경한다.
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("Root0Fh의 실제 기록 쓰기 부족");
    for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); // x86 BYTE/WORD/DWORD 폭을 보존한다.
}
std::int32_t SignedByte(std::span<const std::uint8_t> bytes,std::size_t at) { return std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(Read(bytes,at,1))); } // 원문의 char 필드를 부호 확장한다.
std::uint32_t NetworkWord(std::span<const std::uint8_t> bytes,std::size_t at) { return (Read(bytes,at,1)<<8)|Read(bytes,at+1,1); } // 서버 문자열 개수/길이만 ntohs 순서를 사용한다.
bool Compact1071(std::span<const std::uint8_t> packet) { // 제공 R의 버전과 실제 선언 길이가 모두 맞을 때만3BYTE 호환을 선택한다.
    if (Read(packet,4,2)!=10 || Read(packet,6,2)!=71 || SignedByte(packet,10)<=0 || SignedByte(packet,11)<0) return false;
    const auto length=std::uint64_t(Read(packet,12))+20U+std::uint64_t(Read(packet,10,1))*3U+std::uint64_t(Read(packet,11,1))*5U;
    return length==Read(packet,2,2) && length<=packet.size(); // 다른10.71 확장 응답을 compact로 추정하지 않는다.
}
}
void ObserveRootLoginReply(std::span<std::uint8_t> packet,std::uint32_t sending,const std::function<void(std::string_view,std::uint32_t)>& assertion) {
    if (!sending) return;
    if (Read(packet,12)==0xffffffffU) Need(assertion)("rootServerLen != (int)-1",0x3ac);
    if (Read(packet,10,1)==255) Need(assertion)("numChals != (char)-1",0x3ad);
    if (Read(packet,11,1)==255) Need(assertion)("numFriendsInChal != (char)-1",0x3ae);
    Write(packet,2,Read(packet,12)+20U+std::uint32_t(SignedByte(packet,10))*47U+std::uint32_t(SignedByte(packet,11))*5U,2); // 진단 복귀 뒤 현재 필드를 다시 읽고 하위WORD만 쓴다.
}
void ReceiveRootLoginReply(std::span<const std::uint8_t> packet,RootLoginReplyGlobals g,const RootLoginReplyHooks& h) {
    if (!Need(h.version)(Read(packet,4,2),Read(packet,6,2))) { g.state.next[0]=12;g.state.next[0x50/4]=0;return; }
    Need(h.clearZones)();Need(h.checksum)(Read(packet,16));
    const auto count=NetworkWord(packet,20);std::size_t cursor=22;
    for (std::uint32_t index=0;index<count;++index) { // 원문은 count를 읽은 뒤 현재 rootServerLen 범위일 때만 다음 문자열을 소비한다.
        if (std::bit_cast<std::int32_t>(Read(packet,12))<=static_cast<std::int64_t>(cursor-20)) break;
        const auto length=NetworkWord(packet,cursor);cursor+=2;std::array<char,256> text{};const auto copied=std::min(length,255U);
        for (std::uint32_t byte=0;byte<copied;++byte) text[byte]=static_cast<char>(Read(packet,cursor+byte,1)); // 최대255BYTE만 복사하고 미리0인 마지막BYTE를 남긴다.
        Need(h.server)(text.data());cursor+=length; // 저장된 길이가255를 넘어도 다음 항목은 원래 전체 길이 뒤다.
    }
    Need(h.saveServers)();const bool compact=Compact1071(packet);const auto stride=compact ? 3U:47U;std::uint32_t records=1;
    for (std::int32_t index=0;index<SignedByte(packet,10);++index) { // 원문은 기록1부터 순서대로 쓰고 존 번호를 배열 인덱스로 사용하지 않는다.
        const auto source=20U+std::size_t(Read(packet,12))+std::size_t(index)*stride;const auto target=std::size_t(index+1)*144U;
        Write(g.zones,target,std::bit_cast<std::uint32_t>(SignedByte(packet,source)));Write(g.zones,target+4,std::bit_cast<std::uint32_t>(SignedByte(packet,source+1)));Write(g.zones,target+8,std::bit_cast<std::uint32_t>(SignedByte(packet,source+2)));
        if (!compact) {
            Write(g.zones,target+132,Read(packet,source+43));
            for (std::size_t byte=0;;++byte) { // 원문 strcpy 순서를 유지하고 실제 패킷/표 밖의 읽기·쓰기만 거부한다.
                const auto value=Read(packet,source+3+byte,1);Write(g.zones,target+92+byte,value,1);if (!value) break;
            }
        } //10.71에 없는 이름/+84 값은 앞선 실제 정리 결과를 보존하며 값을 만들지 않는다.
        ++records;
    }
    for (std::int32_t index=0;index<SignedByte(packet,11);++index) { //47BYTE 형식도 원문0040a130의3BYTE 기반 친구 위치를 그대로 읽는다.
        const auto position=20+std::int64_t(Read(packet,12))+SignedByte(packet,10)*3+index*5;if (position<0) throw std::out_of_range("Root0Fh의 실제 패킷 앞 친구 위치");const auto source=static_cast<std::size_t>(position);
        for (std::uint32_t zone=0;zone<records;++zone) { // 빈0번을 포함해 첫 존 번호 일치만 선택한다.
            const auto target=std::size_t(zone)*144U;if (Read(g.zones,target)!=std::bit_cast<std::uint32_t>(SignedByte(packet,source))) continue;
            for (std::uint32_t slot=0;slot<16;++slot) { // 현재0인 첫 친구 슬롯에만 subscriber DWORD를 넣는다.
                if (Read(g.zones,target+12+slot*4U)) continue;Write(g.zones,target+12+slot*4U,Read(packet,source+1));break;
            }break;
        }
    }
    if (g.state.phase[0]==23) { g.state.next[0]=24;g.state.next[0x34/4]=std::bit_cast<std::uint32_t>(SignedByte(packet,9)); }else ++g.refresh;
}
}
