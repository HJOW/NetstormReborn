// 선택 필터와 별도로 실행되는 문자열 변환·현재값 재조회·실제 문맥을 원문 순서로 유지한다.
#include "client/RawChatReceive.h"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>
#include <string>

namespace netstorm::client { // 일반 채팅 패킷의 수신/표시 경계다.
namespace { // 실제 비정렬 필드·C문자열과 원문 텍스트 문맥만 처리한다.
constexpr std::array<std::string_view,10> kOptions{"ShowBattle","ShowEveryone","ShowAllies","ShowObserver","ShowWisper","ShowMyAllies","ShowMyEmenys","ShowMyNeutrals","ShowChatWindow","PlaySoundMessage"}; // 원문 키의 오타도 그대로 조회한다.
// 미선택 하위를 미리 요구하지 않으며 실제 장치 누락은 해당 위치에서 중단한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("Chat 수신의 선택된 실제 장치 미연결");return function; }
// 실제 물리 안의 BYTE/WORD/DWORD를 낮은 BYTE부터 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("Chat 수신 필드의 실제 물리 부족");std::uint32_t value{};
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(bytes[at+index])<<(8*index); // 지정 폭 밖은 읽지 않는다.
    return value;
}
// nameLen은 원문 주소 getter의 signed WORD이며 음수 인덱스의 호스트 접근은 거부한다.
std::size_t BodyOffset(std::span<const std::uint8_t> packet) {
    const auto offset=15+std::int32_t(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Read(packet,10,2))));
    if (offset<0 || std::size_t(offset)>packet.size()) throw std::out_of_range("Chat 수신 본문의 실제 signed 주소 범위");return static_cast<std::size_t>(offset);
}
// 첫NUL 앞 문자열을 같은 현재 패킷에서 빌린다.
std::string_view Text(std::span<const std::uint8_t> bytes,std::size_t at) {
    if (at>bytes.size()) throw std::out_of_range("Chat 수신 문자열 시작 범위");const auto begin=bytes.begin()+at,end=std::find(begin,bytes.end(),0);
    if (end==bytes.end()) throw std::out_of_range("Chat 수신 문자열의 실제 NUL 없음");return {reinterpret_cast<const char*>(bytes.data()+at),static_cast<std::size_t>(end-begin)};
}
// font5/현재색·기본 두1을 넣는004248a0의 같은8DWORD 문맥이다.
std::array<std::uint32_t,8> Context(std::uint32_t color) { return {0,5,color,0,0,0,1,1}; }
}

// 일반 수신의 무시/채널/관계 관문과 UI/소리 호출의 현재값 읽기를 분리한다.
void ReceiveChatPacket(std::uint32_t chat,std::span<std::uint8_t> packet,const ChatReceiveHooks& hooks) {
    const auto global=[&](auto at) { return Need(hooks.read)(at); }; // 원문이 도달한 현재 슬롯만 순수 조회한다.
    if (Read(packet,0,1)>3) { Need(hooks.log)(29,"Received new format chat packet.\n");return; }
    if ((Read(packet,5,1)&15U)!=1) { Need(hooks.log)(29,"Received non-ASCII chat packet.\n");return; }
    const auto body=BodyOffset(packet);const auto length=Read(packet,12,2);
    if (body+length==0 || body+length>packet.size()) throw std::out_of_range("Chat 수신 마지막 본문 BYTE의 실제 물리 부족");packet[body+length-1]=0;
    if ((Read(packet,5,1)&0x80U) && !Read(packet,6)) {
        if (std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Read(packet,10,2)))>1 && Read(packet,15,1)==1) {
            const auto stamp=Read(packet,19);if (stamp<=global(0x548748)) return;Need(hooks.setNewsTime)(stamp);
        }
        Need(hooks.news)(Text(packet,BodyOffset(packet)),400,30);return;
    }
    if (Need(hooks.ignored)(Read(packet,6))) return;
    if (!global(0x594fbc)) {
        Need(hooks.wake)(global(0x5ca92c));
        if (Read(packet,6)!=global(0x540c74) && !global(0x594fc8)) Need(hooks.sound)("newChatMessage.wav");
        const auto context=Context(global(0x5b5dd8));Need(hooks.append)(chat,Text(packet,BodyOffset(packet)),context);Need(hooks.show)();return;
    }
    std::array<std::int32_t,10> options;
    for (std::size_t index=0;index<options.size();++index) options[index]=Need(hooks.config)(kOptions[index]); // 숨기는 메시지도 열 개 옵션을 모두 원문 순서대로 읽는다.
    const auto mask=global(0x548710);bool visible=((mask&8U) && options[0]) || ((mask&32U) && options[1]) || ((mask&64U) && options[2]) || ((mask&128U) && options[3]) || ((mask&256U) && options[4]);
    if (visible) {
        const auto owner=Need(hooks.owner)(Read(packet,6));const auto relation=0x595200U+(owner+global(0x540c70)*9U)*4U;
        if (!global(relation)) visible=options[6]!=0;
        else if (options[5]) { if (!global(relation)) visible=options[6]!=0; }else visible=false;
        if (visible && global(0x52f81eU+owner) && !options[7]) visible=false;
    }
    std::string text(Text(packet,BodyOffset(packet))); // 비표시 메시지도 현재 원문 문자열 지역을 구성한다.
    if (global(0x594fbc)) { if (text.size()<3) throw std::out_of_range("Chat 수신의 실제 세자리 접두 물리 부족");text.erase(0,3);text+='\n'; }
    if (visible) {
        Need(hooks.wake)(global(0x5ca92c));
        if (Read(packet,6)!=global(0x540c74) && !global(0x594fc8) && options[9]) Need(hooks.sound)("newChatMessage.wav");
        const auto context=Context(global(0x5b5dd8));Need(hooks.append)(chat,text,context);if (options[8]) Need(hooks.show)();
    }
}

// wrapper는 이 시점의 현재 Chat0에 대해서만 조용히 종료한다.
void DispatchChatPacket(std::span<std::uint8_t> packet,const ChatReceiveHooks& hooks) {
    const auto chat=Need(hooks.read)(0x548740);if (chat) ReceiveChatPacket(chat,packet,hooks);
}
}
