// 원본 송신은 접속 플래그만으로 보내지 않고 연결 객체의 존재도 확인한다.
#include "o/RawBattleBroadcast.h"
#include <stdexcept>
namespace netstorm::o {
// 원본 packet86의 실제20BYTE 계약이며 미정 payload를 새 의미로 해석하지 않는다.
void WriteFlyerCommandPacket(std::span<std::uint8_t> packet,Sid parent,std::uint32_t command) {
    constexpr std::size_t kLength=20; // 보호 PE0052edf4의 WORD 길이다.
    if (packet.size()<kLength) throw std::out_of_range("훨리기그 방송 패킷20BYTE 필요");
    packet[0]=1;packet[1]=0x86;packet[2]=static_cast<std::uint8_t>(kLength);packet[3]=0; // 보호 PE의 version1/message86/length20이다.
    // 원본이 정의한 두 DWORD만 little endian으로 기록한다.
    for (std::size_t index=0;index<4;++index) {
        packet[4+index]=static_cast<std::uint8_t>(static_cast<std::uint32_t>(parent.value)>>(index*8));
        packet[12+index]=static_cast<std::uint8_t>(command>>(index*8));
    }
}
// 원본이 정의한 네DWORD 전체를 쓰고20BYTE 바깥의 기존 저장소는 유지한다.
void WriteDropIslandPacket(std::span<std::uint8_t> packet,std::uint32_t x,std::uint32_t y,std::uint32_t flags,std::uint32_t owner) {
    constexpr std::size_t kLength=20; // 보호PE0052ede0의 WORD 길이다.
    if (packet.size()<kLength) throw std::out_of_range("섬 정리 방송 패킷20BYTE 필요");
    packet[0]=1;packet[1]=0x35;packet[2]=static_cast<std::uint8_t>(kLength);packet[3]=0; // 보호메타데이터 version1/message35/length20이다.
    const std::array<std::uint32_t,4> payload{x,y,flags,owner}; // 원본 caller의+4/+8/+12/+16 순서다.
    // 각 원문DWORD의 낮은 바이트부터 쓰며 좌표를 다시 실수로 변환하지 않는다.
    for (std::size_t field=0;field<payload.size();++field) {
        // 정확히 해당DWORD의 네 바이트만 변경한다.
        for (std::size_t index=0;index<4;++index) packet[4+field*4+index]=static_cast<std::uint8_t>(payload[field]>>(index*8));
    }
}
// 두 원본 판본의 Player9 물리 폭을 확인한다.
RawBattleBroadcast::RawBattleBroadcast(OriginalEdition edition,std::span<const std::uint8_t> players,
    BattlePacketPeers& peers,const std::uint32_t& localOwner,const bool& server)
    :players_(players),peers_(peers),localOwner_(localOwner),server_(server),
    stride_(edition==OriginalEdition::Patch1078 ? 0xb4U : 0xacU),connectedOffset_(0x50U) {
    if (players.size()!=9*stride_) throw std::invalid_argument("송신 분배 Player9 크기 오류");
}
// 현재 localOwner는 각 후보에서 다시 읽어 가상 송신의 상태 변경을 다음 후보에 반영한다.
void RawBattleBroadcast::Send(std::span<const std::uint8_t> packet) const {
    // 원본 순회는1부터8까지이며 Player0은 건너뛴다.
    for (std::uint32_t owner=1;owner<9;++owner) {
        if (!(players_[owner*stride_+connectedOffset_]&1) || !peers_[owner] || owner==localOwner_) continue;
        const auto send=peers_[owner];send(packet);
    }
}
// 원본 제외 송신은 localOwner를 시작 때 보관하며 이후 접속/연결 표만 다시 읽는다.
void RawBattleBroadcast::SendExcept(std::int32_t excluded,std::span<const std::uint8_t> packet) const {
    if (!server_) throw std::logic_error("원본 제외 송신의 server assertion");
    const auto local=localOwner_;
    // 원본 후보 순서와 지정 제외/로컬 제외 조건을 유지한다.
    for (std::uint32_t owner=1;owner<9;++owner) {
        if (!(players_[owner*stride_+connectedOffset_]&1) || static_cast<std::int32_t>(owner)==excluded || owner==local || !peers_[owner]) continue;
        const auto send=peers_[owner];send(packet);
    }
}
}
