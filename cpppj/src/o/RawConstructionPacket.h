// 건설 확정의 실제0x200바이트 임시 패킷과 가변길이 확정 helper를 복원한다.
#pragma once
#include "o/RawConstructionConfirm.h"

namespace netstorm::o {
// Confirm 입구에서 읽는 현재 메시지 등록값이다. 가변길이 메시지의 초기 선언 길이는0이다.
struct ConstructionPacketHeader {
    std::uint8_t message{0x3f},protocol{1}; // PATCH0052edea/b의 실제 초기BYTE다.
    std::uint16_t declaredLength{}; // PATCH00544c58에서실제MOVZX WORD로읽는현재등록길이다. helper에서최소4를적용한다.
};
// 4441b0/40a840의 같은 원문 BYTE 버퍼를 빌려 실제 소비 위치에서만 필드를 읽는다.
class ConstructionPacketView {
public:
    explicit ConstructionPacketView(std::span<const std::uint8_t> packet):packet_(packet) {} //미사용 필드나 전체 SID 목록은 생성 때 검사하지 않는다.
    std::uint32_t Type() const { return Read(4,1); } //현재 TYPE BYTE를 읽는다.
    float X() const; //현재+5의 비정렬 float 비트를 읽는다.
    float Y() const; //현재+9의 비정렬 float 비트를 읽는다.
    std::uint32_t Argument() const { return Read(13,1); } //진입 때 보관할 인자 BYTE다.
    std::uint32_t Direction() const { return Read(14,1); } //진입 때 보관할 방향 BYTE다.
    std::uint32_t Player() const { return Read(15,1); } //각 하위 효과 뒤 현재 소유자를 다시 읽는다.
    std::uint32_t Flags() const { return Read(16,1); } //Clear 뒤 abstract/Builder 선택 위치의 현재 BYTE다.
    std::uint32_t Quality() const { return Read(17,1); } //Place 호출 직전에 보관하는 품질 BYTE다.
    std::uint32_t TimeLow() const { return Read(18,4); } //Builder 이동 분기에서만 읽는 시각 하위 DWORD다.
    std::uint32_t TimeHigh() const { return Read(22,4); } //Builder 이동 분기에서만 읽는 시각 상위 DWORD다.
    std::uint32_t SidCount() const { return Read(26,1); } //Place 호출 시 고정할 선언 개수 BYTE다.
    std::uint16_t SidAt(std::uint32_t index) const; //40a840의 홀수 주소 SID WORD를 실제 칸 소비 때 읽는다.
private:
    std::uint32_t Read(std::size_t offset,std::size_t width) const; //선택된 필드의 물리 범위만 보호하며 앞선 부분 효과를 되돌리지 않는다.
    std::span<const std::uint8_t> packet_; //호출자가 방송/직접 수신이 끝날 때까지 소유하는 같은 현재 버퍼다.
};
// 00444590→004df590/0040a840. 먼저512바이트를ff로지운뒤원본필드/SID를쓴다. 반환크기는512다.
// 이단계의선언길이는가변길이최종화이전값이며전송길이라고가정하지않는다.
std::size_t WriteConstructionPacket(const ConstructionNotice& notice,const ConstructionPacketHeader& header,std::span<std::uint8_t> packet);
// 0040a800. sending이비영이면현재sidCount로선언길이를갱신한다. false면바이트를보존한다.
// count255의원본진단은callback뒤계산을계속하며물리버퍼밖을읽지는않는다.
void FinalizeConstructionPacket(std::span<std::uint8_t> packet,bool sending,const std::function<void()>& invalidCount={});
}
