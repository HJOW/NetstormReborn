// 단독 미션의 실제 Zone/로그인 바이트와 로컬 두 연결을 같은 패킷 분배기에 연결한다.
#pragma once
#include "o/RawPacketDispatch.h"
#include "o/RawPlayerWorld.h"
#include "o/RawBattleBroadcast.h"
#include <deque>

namespace netstorm::zacket {
// 원본 예약 전체와 observer가 확정한 선언 길이를 함께 보관한다.
struct LocalMissionPacket {
    std::vector<std::uint8_t> storage; // Zone512/로그인FortImage+67의 전체 물리 바이트다.
    // 실제 송신/State 이미지 복사가 사용하는 현재 선언 WORD 길이만 빌린다.
    std::span<const std::uint8_t> Bytes() const;
};
// PATCH004b51e0의 Zone512/4슬롯 작성과0040a1e0의86바이트 길이 확정이다.
LocalMissionPacket MakeLocalMissionZone();
// PATCH004b63d0의 실제 외부 Fort 이미지 이후 패킷 작성과0040a450의 길이 확정이다.
LocalMissionPacket MakeLocalMissionLogin(std::uint32_t subscriber,std::string_view handle,std::span<const std::uint8_t> fortImage);
// 현재0x27 버퍼의 필드 참조이며 처리기 호출 동안만 유효하다.
struct LocalMissionLoginView {
    std::uint32_t subscriber{}; // +8의 실제 subscriber DWORD다.
    std::string_view handle; // +17의 현재 NUL 문자열이며 원본 nameLen도 포함해 검증한다.
    std::span<const std::uint8_t> fortImage; // +17+nameLen에서 fortFileSize만큼의 실제 파일 이미지다.
};
// 원본0040a580/0040a4b0의 위치와 실제 물리 버퍼 경계를 함께 확인한다.
LocalMissionLoginView ReadLocalMissionLogin(std::span<const std::uint8_t> packet);
// Zone 설치가 실제로 읽고 쓰는 전역을 상위 Session의 같은 저장소로 연결한다.
struct LocalMissionZoneGlobals {
    bool& server; //00540bc0의 현재 서버 전역이다.
    bool& boss; //00540bc4의 현재 권한 전역이다.
    const std::uint32_t& mySubscriber; //00540c74의 현재 subscriber이며 싱글플레이라고1을 강제하지 않는다.
    const std::uint32_t& pendingStamp; //005c85e4에서 설치 직전 읽는 현재 값이다.
    std::uint32_t& activeStamp; //005c8540에 복사하는 실제 현재 값이다.
    std::uint32_t& activeZoneWord; //005c8544에 Zone+10을 쓰는 실제 저장소다.
    std::uint32_t& zoneFlags; //005c86c0의 실제 Zone+4 DWORD다.
    double& earlyCaptureEnd; //005c8618의 실제 double 예약 기한이다.
    const bool& adjustZoneFlags; //00540a2c의 현재 마스크 보정 모드다.
    const bool& missionWorld; //00594fa0의 현재 미션 저장맵 모드다.
    const std::uint8_t& zoneFlagOverride; //005300e8의 하위6비트 보충값이다.
    const bool& metamap; //00594fc8의 현재 coreData 선택 모드다.
    std::span<std::uint8_t>& currentCoreData; //00595350의 현재10BYTE를 host 포인터 대신 실제 span으로 빌린다.
    std::span<std::uint8_t> cachedCoreData; //metamap의005953f0 실제10BYTE이며 그 분기에서만 필요하다.
};
// 파일·시간·Chat·서버 열기처럼 원본 Zone 바깥 몸체의 실제 선택 경계다.
struct LocalMissionZoneHooks {
    std::function<std::int32_t()> loginTimeout; //00441090("loginTimeout")의 현재 설정이다.
    std::function<double()> seconds; //00460d70의 실제 예약 기준 시각이다.
    std::function<void(std::uint32_t,std::string_view)> storeName; //0042b0e0의 같은 실제 Chat 이름 갱신이다.
    std::function<void(std::uint32_t)> serve; //004e2430의 현재 포트 서버 시작 소비자다.
    std::function<void(std::string_view)> assertion; //원본 진단 뒤 계속 진행하는 선택 경계다.
};
// PATCH004b4be0과 양수 owner의0048f3b0을 실제 Player9에 적용한다. flags2 로그인 성공을 쓰지 않는다.
void InstallLocalMissionZone(std::span<const std::uint8_t> zone,std::span<std::uint8_t> players,
    o::PlayerWorldState& player,LocalMissionZoneGlobals globals,LocalMissionZoneHooks hooks,std::uint32_t port);
// 실제 zlocal의 클라이언트9/서버 pending-1 두 끝과 byte 큐를 단독 소유한다.
// TCP/SPX/OS 소켓 전체를 복원하는 클래스가 아니며 simulconnect 전역을 변경하지 않는다.
class LocalMissionTransport {
public:
    // 같은 Player9와 서버 전역/실제 분배 효과를 빌린다. hooks의 모든 경계는 실제 소비자여야 한다.
    LocalMissionTransport(std::span<const std::uint8_t> players,const bool& server,o::PacketDispatchHooks hooks);
    //0x27/version1/정책0 등록이다. 실제004a7800 상위 몸체가 없으면 열기 전에 거부한다.
    void RegisterLogin(decltype(o::PacketRegistration::receive) receive);
    // 원본 zlocal 서버/접속 두 끝을 열고 서버 측을 pending-1로 등록한다. 포트는 실제 게임 설정값이다.
    void Open(std::uint32_t port);
    //004e27b0의 실제 pending-1을 같은 연결 객체 owner1..8로 이동하고 buffered 전송을 켠다.
    void AssignConnection(std::int32_t pending,std::int32_t owner);
    //004e2dc0의 현재 flags1/NULL 연결 선택 후 선언 길이 바이트를 실제 다른 끝의 송신 큐에 쓴다.
    bool Send(std::int32_t destination,std::span<const std::uint8_t> packet);
    //004e2920/004e0150의 현재 연결 순서로 실제 type4 집합 프레임을 완성해 상대 수신 큐에 넣는다.
    void Flush();
    //004e28b0처럼 pending을 먼저 한 번, 등록된 연결을 번호순으로 읽고 같은 등록표에 분배한다.
    std::size_t Poll();
    // 지연 없는 실제 수신 프레임의 단일/집합 분기다. 같은 실제 연결/등록표를 직접 사용한다.
    bool DispatchFrame(o::PacketConnection& connection,std::span<const std::uint8_t> frame);
    // 같은 실제 분배기에 건설/준비/서버 응답 처리기도 추가할 수 있다.
    o::RawPacketDispatch& Dispatch();
    // 원본 두 끝의 현재 sender/flags/depth를 실제 상위 로그인/종료 소비자에게 제공한다.
    o::PacketConnection& Client();
    o::PacketConnection& Server();
    // RawBattleBroadcast가 같은 연결 객체의 가상 송신을 사용하도록 현재 해당 owner의 소비자를 만든다.
    o::BattlePacketSend Peer(std::uint32_t owner);
    // 실제 endpoint의 존재/현재 재배치 번호와 송수신 바이트 수를 검증한다.
    bool IsOpen() const;
    std::uint32_t Owner() const;
    std::size_t QueuedFrames() const;
private:
    // 지정 끝의 실제 한 프레임을 꺼내 분배한다. callback 이후에도 고정 주소 연결은 살아 있다.
    bool ReceiveOne(bool serverSide);
    // buffered 데이터의 실제 집합 헤더를 만들고 다른 끝의 수신 큐로 옮긴다.
    void FlushOne(bool serverSide);
    std::span<const std::uint8_t> players_; //같은 현재 Player9 원문이며 active 사본을 만들지 않는다.
    const bool& serverMode_; //004e27b0이 현재 읽는 실제 서버 모드다.
    o::PacketDispatchHooks hooks_; //같은 실제 시계/기록/종료/반납 경계다.
    o::RawPacketDispatch dispatch_; //로그인/건설/응답이 공유하는 실제 등록표다.
    o::PacketConnection client_,server_; //포인터 없는 표식이 아닌 실제 서로 다른 연결 저장소다.
    bool open_{},pending_{},serverBuffered_{}; //실제 두 끝의 열림/서버 pending/서버 buffered 모드다.
    std::uint32_t owner_{},port_{}; //pending 재배치 뒤의 현재owner와 실제 열린 로컬 포트다.
    std::vector<std::uint8_t> clientOutput_,serverOutput_; //원본 buffered 전송의 아직 flush하지 않은 바이트다.
    std::deque<std::vector<std::uint8_t>> toServer_,toClient_; //다른 끝이 실제로 받을 완성된 프레임의 소유 큐다.
};
}
