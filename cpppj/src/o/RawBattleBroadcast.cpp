// 원본 송신은 접속 플래그만으로 보내지 않고 연결 객체의 존재도 확인한다.
#include "o/RawBattleBroadcast.h"
#include <stdexcept>
namespace netstorm::o {
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
