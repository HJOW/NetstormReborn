// 원본 Ztrans의 현재 Player/연결 표를 읽는 두 송신 분배 몸체다.
#pragma once
#include "o/RiftType.h"
#include "o/SidPool.h"
#include <array>
#include <functional>
#include <span>
namespace netstorm::o {
// 연결 객체의 가상+4에 전달할 현재 전체 packet 바이트다. 빈 함수는 NULL 연결이다.
using BattlePacketSend=std::function<void(std::span<const std::uint8_t>)>;
// 현재0..8 연결 객체 표다. 0번은 원본 두 분배 함수에서 순회하지 않는다.
using BattlePacketPeers=std::array<BattlePacketSend,9>;
// 실제40a890/45c94d/45cc4e의 헤더·SID·명령 DWORD만 쓴다. caller가 쓰지 않는+8/+16은 입력 바이트를 유지한다.
void WriteFlyerCommandPacket(std::span<std::uint8_t> packet,Sid parent,std::uint32_t command);
// 실제46e6e7/40a740의20BYTE packet35를 쓴다. 좌표는 기존DropIsland가 만든16.16 DWORD 비트다.
void WriteDropIslandPacket(std::span<std::uint8_t> packet,std::uint32_t x,std::uint32_t y,std::uint32_t flags,std::uint32_t owner);
// 같은 Player 기록/연결 객체를 매 순회 다시 읽는다. 단독 캠페인은 실제 NULL 연결 표를 사용한다.
class RawBattleBroadcast {
public:
    // players/peers/localOwner/server는 이 객체보다 오래 살아야 한다.
    RawBattleBroadcast(OriginalEdition edition,std::span<const std::uint8_t> players,
        BattlePacketPeers& peers,const std::uint32_t& localOwner,const bool& server);
    //변환된BYTE/임시DWORD를현재전역참조로보관하는수명오류를컴파일단계에서거부한다.
    RawBattleBroadcast(OriginalEdition,std::span<const std::uint8_t>,BattlePacketPeers&,std::uint32_t&&,const bool&)=delete;
    RawBattleBroadcast(OriginalEdition,std::span<const std::uint8_t>,BattlePacketPeers&,const std::uint32_t&&,const bool&)=delete;
    // PATCH004e2e30/CD004c0420: 현재 접속한1..8의 NULL 아닌 연결 중 로컬을 제외한다.
    void Send(std::span<const std::uint8_t> packet) const;
    // PATCH004e2e80/CD004c0480: 서버를 확인하고 시작 시 로컬 owner와 지정 제외 번호를 사용한다.
    void SendExcept(std::int32_t excluded,std::span<const std::uint8_t> packet) const;
private:
    std::span<const std::uint8_t> players_; // 현재 원본9 Player 기록을 직접 읽는다.
    BattlePacketPeers& peers_; // 실제 송신 객체가 존재하는 현재 연결 표다.
    const std::uint32_t& localOwner_; // 현재 원본 로컬 Player 번호다.
    const bool& server_; // 제외 송신의 실제 서버 assertion 입력이다.
    std::size_t stride_,connectedOffset_; // 판본별 Player 폭과 접속 BYTE 위치다.
};
}
