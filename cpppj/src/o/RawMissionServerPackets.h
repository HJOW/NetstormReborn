// 로그인 서버의 현재 Player/중앙섬 metadata 작성과 원본 송신 순서를 복원한다.
#pragma once
#include "o/RawPacketDispatch.h"
#include <string>
namespace netstorm::o {
// 여러 송수신 몸체가 공유하는 현재 원본 전역을 그대로 빌린다.
struct MissionServerPacketState {
    const bool& server; // 00540bc0이며 진단 뒤에도 원본은 작성을 계속한다.
    const bool& metamap; // 00594fc8이며 Player 번호를 읽을 때마다 진단한다.
    const std::uint32_t& localOwner; // 00540c70의 현재 자기 번호다.
    const std::uint32_t& centralCreated; // 005c85a0의 현재 중앙섬 생성 표시다.
    const std::uint32_t& archipelago; // 005c8bb4의 현재 값이며 packet에는 낮은BYTE만 쓴다.
    std::span<const std::uint8_t> zoneAddress; // 005c86c0의 현재6BYTE이며 앞DWORD의100비트를 읽는다.
};
// 큰 전송/주소 포맷과 중앙섬 생성만 실제 외부 하위다.
struct MissionServerPacketHooks {
    std::function<void(std::string_view)> assertion; // 원본 진단 뒤 계속하는 실제 소비자다.
    std::function<std::string(std::uint32_t)> peerAddress; //004e2a50→004e0a30(1,0,0)의 현재 주소 문자열이다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> send; // observer가 확정한 실제 wire바이트의 현재 송신이다.
    std::function<void(std::string_view)> log; // 중앙섬 없음의 실제004a68e0 로그다.
};
// PATCH0048f9f0/004a6b40와 미션 Zone의004b3bc0 자연 경로를 같은 Player에 연결한다.
class RawMissionServerPackets {
public:
    // 같은 Player9와 전역은 처리기보다 오래 살아야 한다. 보호 원본 이름을 직접 수정하는 API가 아니다.
    RawMissionServerPackets(std::span<std::uint8_t> players,MissionServerPacketState state,MissionServerPacketHooks hooks);
    // 현재 Player 이름을 실제 원본 정제 뒤 byte단위 packet에 쓰고 해당 target으로 보낸다.
    void SendPlayer(std::uint32_t owner,std::int32_t target) const;
    // 실제$Random metadata를 만들고 필요하면 같은 로컬 수신 몸체를 먼저 실행한다.
    void SendMetadata(std::int32_t target) const;
    // 같은 Zone의 bit100이 없으면 원본 로그 뒤 자연 반환한다. 큰 중앙섬 생성은 명시 미복원이다.
    void ReceiveMetadata(std::span<const std::uint8_t> packet) const;
private:
    // 현재 Player 번호와 비정렬 필드를 각각 원본 위치에서 읽는다.
    std::uint32_t Owner(std::uint32_t owner) const;
    std::uint32_t Read(std::uint32_t owner,std::size_t offset,std::size_t width=4) const;
    std::span<std::uint8_t> players_; // 같은9×180BYTE이며 이름 정제도 여기에 직접 반영한다.
    MissionServerPacketState state_; // 원본 현재 전역의 참조 묶음이다.
    MissionServerPacketHooks hooks_; // 실제 선택된 주소/송신/진단/로그 소비자다.
};
// 실제0x72의 observer/version1/정책0/현재 수신기를 같은 분배기에 등록한다.
void RegisterMissionMetadata(RawPacketDispatch& dispatch,RawMissionServerPackets& packets);
}
