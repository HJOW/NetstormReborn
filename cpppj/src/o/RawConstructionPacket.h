// 건설 확정의 실제0x200바이트 임시 패킷과 가변길이 확정 helper를 복원한다.
#pragma once
#include "o/RawConstructionConfirm.h"

namespace netstorm::o {
// Confirm 입구에서 읽는 현재 메시지 등록값이다. 가변길이 메시지의 초기 선언 길이는0이다.
struct ConstructionPacketHeader {
    std::uint8_t message{0x3f},protocol{1}; // PATCH0052edea/b의 실제 초기BYTE다.
    std::uint16_t declaredLength{}; // PATCH00544c58에서실제MOVZX WORD로읽는현재등록길이다. helper에서최소4를적용한다.
};
// 00444590→004df590/0040a840. 먼저512바이트를ff로지운뒤원본필드/SID를쓴다. 반환크기는512다.
// 이단계의선언길이는가변길이최종화이전값이며전송길이라고가정하지않는다.
std::size_t WriteConstructionPacket(const ConstructionNotice& notice,const ConstructionPacketHeader& header,std::span<std::uint8_t> packet);
// 0040a800. sending이비영이면현재sidCount로선언길이를갱신한다. false면바이트를보존한다.
// count255의원본진단은callback뒤계산을계속하며물리버퍼밖을읽지는않는다.
void FinalizeConstructionPacket(std::span<std::uint8_t> packet,bool sending,const std::function<void()>& invalidCount={});
}
