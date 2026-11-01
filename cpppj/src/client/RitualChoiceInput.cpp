// 원문004d82ed/004d8351/004d83b7의 요청 작성과 송신 순서를 정적으로 복원한다.
#include "client/RitualChoiceInput.h"
#include <array>
#include <bit>
#include <stdexcept>

namespace netstorm::client { //이 경로의 테스트/통신 실행은 추가하지 않는다.
namespace { //도달한 실제 경계만 요구하고 비정렬 필드 폭을 보존한다.
template<class F> const F& Required(const F& hook) { if (!hook) throw std::logic_error("의식 선택 입력의 실제 장치 미연결");return hook; } //미선택 장치에 성공값을 공급하지 않는다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t count) { //낮은 바이트부터 지정 폭만 쓴다.
    for (std::size_t index=0;index<count;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(8*index)); //SID는WORD이며기술은DWORD다.
}
}
bool IsRitualChoiceCommand(std::uint32_t code) { return code==kGetNewTechCommand || code==kRequestStormpowerCommand || code==kUpgradeAltarCommand; } //임의 lowBYTE 키를 명령으로 취급하지 않는다.
void ConstructRitualChoicePacket(std::span<std::uint8_t> packet) { //PE0052ee64=6b/65=1/68=WORD10이며 BYTE0은버전이다.
    if (packet.size()<10) throw std::out_of_range("의식 선택 요청10BYTE 부족");packet[0]=0;packet[1]=0;Put(packet,2,0,2);packet[1]=0x6b;Put(packet,2,10,2);packet[0]=1;
}
bool DispatchRitualChoiceInput(const InputEvent& event,const RitualChoiceInputHooks& hooks) { //원문 일반 입력의 공통 release 관문 뒤에 있는 세 분기다.
    const auto code=event.code&0x03ffffffU;if (!IsRitualChoiceCommand(code)) return false;if (event.code&InputCode::kRelease) return true;
    std::array<std::uint8_t,10> packet{};Required(hooks.construct)(packet);std::uint32_t dais{},kind{}; //생성자는대상진단보다먼저실행한다.
    if (code==kUpgradeAltarCommand) { dais=std::bit_cast<std::uint32_t>(event.y)&0xffffU;kind=Required(hooks.daisType)(); }
    else { if (!Required(hooks.dais)()) Required(hooks.assertion)(code==kGetNewTechCommand ? 0x82fU : 0x838U);dais=Required(hooks.dais)()&0xffffU;kind=code==kGetNewTechCommand ? std::bit_cast<std::uint32_t>(event.y) : Required(hooks.nuggetType)(); }
    Put(packet,4,dais,2);Put(packet,6,kind,4);Required(hooks.send)(9,packet);return true;
}
}
