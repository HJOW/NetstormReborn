// 원본 설명 수신은 복사한 길이의 마지막 바이트에 NUL을 쓰고 남은 꼬리를 지우지 않는다.
#include "client/NativeMetaBattleDescription.h"
#include "client/CurrentAllocationBytes.h" //가변 요청의 명시 필드 밖 바이트는 실제 할당 입력을 유지한다.
#include <algorithm>
#include <bit>
#include <vector>
#include <stdexcept>
#include <string>

namespace netstorm::client { // 범위 외 Native 메모리 접근은 선택된 위치의 호스트 오류로 구별한다.
namespace { // 현재 packet/CS의 실제 비정렬 BYTE 폭만 읽고 쓴다.
constexpr std::uint32_t kDescriptionCapacity=41; //원문 전투 설명 길이 상한29h이며 마지막 복사 BYTE를 NUL로 덮어쓴다.
template<class F> const F& Need(const F& value) { if (!value) throw std::logic_error("전투 설명 수신의 선택된 실제 장치 미연결");return value; } //조기 반환에서 미사용 장치를 요구하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("전투 설명 수신의 실제 읽기 범위");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } //필드 폭만 낮은BYTE부터 조립한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("전투 설명 수신의 실제 쓰기 범위");for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } //부분 효과를 보존하고 전체 설명/packet을 선검사하지 않는다.
}
// 무효 전투는 로그만, 초과 길이는 로그→현재 번호 갱신만 수행한다.
void ReceiveNativeMetaBattleDescription(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const NativeMetaBattleDescriptionHooks& hooks) {
    const auto battle=challenge.Battle(Read(packet,4,1));
    if (!(Read(battle,0,1)&1U)) { Need(hooks.log)("ERROR in handleChatBattleUpdate: battle "+std::to_string(Read(packet,4,1))+" is not valid!\n");return; }
    const auto length=Read(packet,9,1);
    if (length>kDescriptionCapacity) { Need(hooks.log)("ERROR in handleChatBattleUpdate: desc too long\n[bufferLen == 41, optionLen == "+std::to_string(length)+"]\n");Need(hooks.updateBattle)(Read(packet,4,1));return; }
    std::size_t at{};
    for (auto words=length>>2;words;--words,at+=4) Put(battle,0xf9+at,Read(packet,10+at)); //먼저 캐시한 길이의 전체DWORD를 순서대로 복사한다.
    for (auto bytes=length&3U;bytes;--bytes,++at) Put(battle,0xf9+at,Read(packet,10+at,1),1); //남은 BYTE도 같은 순서로 복사한다.
    Put(battle,0xf8+Read(packet,9,1),0,1); //현재 길이를 다시 읽는다.0이면 설명 앞 f8을 쓰고 f9를 빈 문자열로 만들지 않는다.
    at=0xf9;auto value=Read(battle,at,1);
    while (value) { //현재 NUL까지 일곱 ASCII 제어 문자만 공백으로 바꾸며 locale 분류를 추가하지 않는다.
        switch (value) { case 7:case 8:case 12:case 10:case 13:case 9:case 11:Put(battle,at,0x20,1);break;default:break; }++at;value=Read(battle,at,1);
    }
    Need(hooks.refreshObject)(Read(battle,0x20));Need(hooks.updateBattle)(Read(packet,4,1)); //실제 가상88 뒤에는 캐시한 첫 번호 대신 현재 packet 번호를 읽는다.
}
//원문 확정은 첫 LF를 자르고 NUL부터 isspace를 검사하므로 후행 공백은 남는다.
std::string ConfirmNativeMetaBattleDescription(std::span<const char> copied) {
    if (copied.size()<256) throw std::out_of_range("설명 확정의256BYTE 실제 복사 버퍼 부족");
    std::array<char,256> text;std::copy_n(copied.begin(),256,text.begin());text.back()=0;
    const auto end=std::find(text.begin(),text.end(),0);const auto line=std::find(text.begin(),end,'\n');
    return {text.data(),static_cast<std::size_t>(line-text.begin())};
}
//권한 관문 뒤에만40BYTE 잘림/NUL을 작성하고 현재71h 관찰기/실제 전송을 호출한다.
void SendNativeMetaBattleDescription(ClientChallengeGlobals& challenge,const std::uint32_t& localOwner,std::int32_t battle,std::optional<std::string_view> description,std::uint32_t parameter,const NativeMetaDescriptionSendHooks& hooks) {
    if (battle<-1 || battle>=10) Need(hooks.assertion)("battleId >= -1 && battleId < CL_NUM_CHAL_BATTLES","\\Ns\\O\\Ch.cpp",0x580);
    const auto own=challenge.Participant(localOwner);if (battle==-1) battle=std::bit_cast<std::int32_t>(Read(own,0xa4));
    if (!Read(own,0xa4) || Read(own,0xa4)!=std::bit_cast<std::uint32_t>(battle) || !(Read(own,0x64,1)&4U)) { Need(hooks.denied)();return; }
    if (!description) return;const auto text=description->substr(0,std::min(description->find('\0'),std::size_t{40}));
    const auto length=text.size()+1;std::vector<std::uint8_t> packet(10+length);CurrentAllocationBytes(packet);
    Put(packet,0,1,1);Put(packet,1,0x71,1);Put(packet,2,0,2);Put(packet,4,std::bit_cast<std::uint32_t>(battle),1);Put(packet,9,static_cast<std::uint32_t>(length),1);
    std::copy(text.begin(),text.end(),packet.begin()+10);packet[10+text.size()]=0;Put(packet,5,parameter);Need(hooks.send)(packet);
}
}
