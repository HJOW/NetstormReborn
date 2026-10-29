// 원본0042dc90의 수신 마스크 해석·서버 수신자 축소·자기 로컬 전달을 복원한다.
#pragma once
#include "o/RawPacketDispatch.h"
#include <array>

namespace netstorm::client { // 같은 mutable 패킷과 Client 수명 마스크를 원문 중계에 빌린다.
struct ChatRelayHooks { // 실제 도달한 경계만 요구하며 원격 송신은 별도 장치다.
    std::function<std::uint32_t(std::uint32_t)> read; // 현재 battle/server/subscriber의 원문 DWORD다.
    std::function<std::array<std::uint8_t,20>(std::uint32_t)> address; //004e2a50의 현재 전송 ID 주소 또는 zero주소다.
    std::function<std::uint32_t(std::uint32_t)> player; //0048f430의 실제 Player 토큰 또는 실패0이다.
    std::function<std::int32_t(std::uint32_t)> channel; //0048f550의 현재 Player 번호이며 송신 직전 다시 읽는다.
    std::function<bool(std::int32_t)> connected; //004e2b10의 현재 transceiver 관문이다.
    std::function<void(std::int32_t,std::span<std::uint8_t>)> send; // 선택한 원격 현재 번호로 같은 packet을 전송한다.
    std::function<void(std::span<std::uint8_t>,std::span<const std::uint8_t>)> receive; //0042f0e0의 패킷과 준비한20BYTE 주소다.
    std::function<void(std::uint32_t)> assertion; // 송신 observer의 현재 원본 줄 번호 진단이다.
};
// 주소 준비→전투 접두 해석→서버 수신자 축소/전달→선택된 로컬 수신 순서다.
void RelayChatPacket(std::uint32_t sender,std::span<std::uint8_t> packet,std::uint32_t& mask,const ChatRelayHooks& hooks);
// 같은 원본0x41/버전1·정책0과 실제 패킷 observer/중계를 한 분배 표에 등록한다.
void RegisterChatPacket(o::RawPacketDispatch& dispatch,std::uint32_t& mask,ChatRelayHooks hooks);
}
