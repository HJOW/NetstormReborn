// 접두 마스크와 수신자 첫 항목은 원문이 쓰는 위치에서만 변경한다.
#include "client/RawChatRelay.h"
#include "client/RawChatPacket.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 현재 Chat 전송·수신의 공통 중계 부품이다.
namespace { // 실제 패킷 필드와 세자리 CRT 십진 변환만 처리한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("Chat 중계의 선택된 실제 장치 미연결");return function; } // 미선택 장치는 요구하지 않는다.
// 실제 비정렬 BYTE/WORD/DWORD를 지정 폭으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("Chat 중계의 실제 필드 범위 부족");std::uint32_t value{};
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(bytes[at+index])<<(8*index);return value; // 낮은 BYTE부터 조립한다.
}
// 현재 DWORD만 기록하며 뒤의 수신자 항목을 지우지 않는다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Chat 중계의 실제 DWORD 쓰기 범위 부족");
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(8*index)); // 지정한 네 BYTE만 쓴다.
}
// getter0040a970/80의 signed nameLen과 unsigned bufferLen을 같은 물리 주소로 바꾼다.
std::size_t Body(std::span<const std::uint8_t> packet) {
    const auto at=15+std::int32_t(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Read(packet,10,2))));
    if (at<0 || std::size_t(at)>packet.size()) throw std::out_of_range("Chat 중계의 실제 signed 본문 주소");return static_cast<std::size_t>(at);
}
// 처음NUL 앞의 최대3BYTE만 좁은 CRT atol 규칙으로 읽는다.
std::uint32_t PrefixMask(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),0);if (end==bytes.end()) throw std::out_of_range("Chat 중계 접두의 실제 NUL 없음");const auto length=std::min<std::size_t>(3,end-bytes.begin());std::size_t at{};
    while (at<length && (bytes[at]==' ' || bytes[at]=='\r' || bytes[at]=='\n' || bytes[at]=='\t' || bytes[at]=='\v' || bytes[at]=='\f')) ++at; // 실제 CRT 앞 공백만 건너뛴다.
    bool negative{};if (at<length && (bytes[at]=='-' || bytes[at]=='+')) { negative=bytes[at]=='-';++at; }std::uint32_t value{};
    while (at<length && bytes[at]>='0' && bytes[at]<='9') { value=value*10U+bytes[at]-'0';++at; }return negative ? 0U-value : value; // 첫 비숫자에서 변환을 끝낸다.
}
}
// 단독도 같은 서버 부모를 사용하며 현재 subscriber와 일치한 경우에만 자기 로그로 전달한다.
void RelayChatPacket(std::uint32_t sender,std::span<std::uint8_t> packet,std::uint32_t& mask,const ChatRelayHooks& hooks) {
    const auto address=Need(hooks.address)(sender);const auto global=[&](auto at) { return Need(hooks.read)(at); }; // 원문 위치의 현재 전역을 순수 조회한다.
    if (global(0x594fbc)) {
        const auto body=Body(packet);const auto length=Read(packet,12,2);if (body+length==0 || body+length>packet.size()) throw std::out_of_range("Chat 중계의 실제 마지막 본문 BYTE 없음");packet[body+length-1]=0;mask=PrefixMask(packet.subspan(body));
    }
    bool local{};
    if (global(0x540bc0)) {
        const auto count=Read(packet,14,1);packet[14]=1;if (!count) return;
        for (std::uint32_t index=0;index<count;++index) { // 처음 읽은 BYTE 수만큼 현재 수신자 배열을 다시 조회한다.
            const auto start=Body(packet)+Read(packet,12,2);const auto subscriber=Read(packet,start+std::size_t(index)*4);Put(packet,start,subscriber);
            if (subscriber==global(0x540c74)) local=true;
            else { const auto player=Need(hooks.player)(subscriber);if (player && Need(hooks.connected)(Need(hooks.channel)(player))) Need(hooks.send)(Need(hooks.channel)(player),packet); }
        }
        if (!local) return;
    }
    Need(hooks.receive)(packet,address);
}
// packet의 mutable this를 가진 observer를 등록해 길이4인 원시 패킷을 실제 전송 전에 확정한다.
void RegisterChatPacket(o::RawPacketDispatch& dispatch,std::uint32_t& mask,ChatRelayHooks hooks) {
    dispatch.Version(0x41,1,0);dispatch.ObservePacket(0x41,[assertion=hooks.assertion](auto bytes,auto sending) { ObserveChatPacket(bytes,sending,assertion); });
    dispatch.Register(0x41,[&mask,hooks=std::move(hooks)](auto sender,auto&,auto bytes) { RelayChatPacket(sender,{const_cast<std::uint8_t*>(bytes.data()),bytes.size()},mask,hooks); }); // 실제 전송은 소유 mutable vector를 수신하며 같은 바이트에 원문 변경을 보존한다.
}
}
