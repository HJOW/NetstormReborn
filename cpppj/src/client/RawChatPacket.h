// 원본0042d280의 일반 채팅 패킷 작성·채널 선택과0040a8d0의 송신 길이 확정을 복원한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::client { // 본문 조합 부모와 실제 전송/수신 장치 사이의 원문 경계다.
struct ChatPacketHooks { // getter는 같은 물리의 순수 조회이며 선택된 실제 장치만 요구한다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> memory; // 같은 manager와 수신자 할당의 실제 물리다.
    std::function<std::span<const std::uint8_t>()> currentName; // 첫NUL을 포함한 현재 이름의 물리다. 추가 callback 뒤에도 처음 포인터를 읽는다.
    std::function<std::uint32_t(std::uint32_t)> read; // 현재 subscriber/세 모드/초기 패킷 헤더의 원문 전역이다.
    std::function<void(std::uint32_t,std::uint32_t)> addRecipient; //0042d1b0의 같은 대상 manager에 누락된 자기 subscriber를 추가한다.
    std::function<void(std::int32_t,std::span<std::uint8_t>)> send; //004e2dc0의 실제 선택 채널이다. 같은 패킷의 observer는 이 경계에서 실행한다.
    std::function<void(std::uint32_t,std::string_view)> log; // 채널이 하나도 없을 때만 원문 오류를 기록한다.
    std::function<void()> zeroAddress; //004e0cd0("0.0.0.0",0)의 실제 주소 장치를 준비한다.
    std::function<void(std::span<std::uint8_t>)> localReceive; // 위 zero주소로0042f0e0을 호출한다. 수신 자체를 빈 성공으로 바꾸지 않는다.
};
// body는 caller의 명시 길이 전체다. 전투 분기는 첫NUL 앞에 signed 십진/앞0 패딩을 붙이고 NUL을 제외한다.
// 원문 alloca의4BYTE 부족 대신 전체 실제 복사 범위를 소유하되 패킷 바이트/WORD/BYTE 절단은 유지한다.
std::uint32_t SendChatPacket(std::uint32_t manager,std::uint32_t recipients,std::span<const std::uint8_t> body,const ChatPacketHooks& hooks);
// 송신 비영일 때만 현재 WORD/WORD/BYTE를 합산한다. bufferLen==ffff의 도달 불가능한 원문 assertion은 추가하지 않는다.
void ObserveChatPacket(std::span<std::uint8_t> packet,std::uint32_t sending,const std::function<void(std::uint32_t)>& assertion);
}
