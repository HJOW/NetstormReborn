// 원본 설명 수신은 복사한 길이의 마지막 바이트에 NUL을 쓰고 남은 꼬리를 지우지 않는다.
#include "client/NativeMetaBattleDescription.h"
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
}
