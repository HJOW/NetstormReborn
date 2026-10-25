// 창 없는 기존 LocalMissionTransport 본체에서 Client9 객체 존재와 접속 flags를 구분한다.
#include "TestSupport.h"
#include "zacket/LocalMissionTransport.h"
#include <array>
#include <cstdint>
#include <stdexcept>

namespace { // 실제 전송 본체가 선택한 시계 호출과 부분 생성 효과만 관측한다.
using netstorm::o::PacketConnection;
using netstorm::o::PacketDispatchHooks;
using netstorm::zacket::LocalMissionTransport;

// 선택한 현재 시계 효과가 중단됐음을 다른 host 계약 예외와 구별한다.
struct ActivityTransportClockStop {};

// 실제 Player 물리와 시계 입력을 전송 객체보다 오래 보관하는 독립 장치 fixture다.
struct ActivityTransportDevice {
    std::array<std::uint8_t,9*180> players{}; // 기존 전송이 직접 빌리는 같은9 Player의 현재 BYTE다.
    bool server{true}; // 접속 성공 flags와 별개인 현재 서버 모드 입력이다.
    std::uint32_t clockCalls{},stopAt{}; // 실제 시계 호출 수와 선택된 예외의 한 번 호출 위치다.

    // 이 검사는 패킷을 보내지 않는다. 시계 이외의 효과가 선택되면 실패를 기록한다.
    PacketDispatchHooks Hooks() {
        PacketDispatchHooks hooks; // 현재 실제 전송 생성자가 요구하는 다섯 장치를 제공한다.
        hooks.now=[this] { ++clockCalls;if (stopAt==clockCalls) throw ActivityTransportClockStop{};return 100U+clockCalls; };
        hooks.record=[](PacketConnection&,std::span<const std::uint8_t>,std::uint16_t) { CHECK(false); };
        hooks.terminate=hooks.close=hooks.release=[](PacketConnection&) { CHECK(false); };
        return hooks;
    }
};

// Open의 실제 로그인 등록 계약만 충족하며 수신 호출을 성공으로 대신하지 않는다.
void RegisterActivityTransportLogin(LocalMissionTransport& transport) {
    transport.RegisterLogin([](std::uint32_t,PacketConnection&,std::span<const std::uint8_t>) { CHECK(false); });
}
}

// 서버 Open과 Client9 생성은 별개이며 성공 뒤 flags 변경은 실제 객체의 부재를 뜻하지 않는다.
TEST_CASE(ConnectionActivityTransport_ClientObjectPresenceIsSeparateFromFlags) {
    ActivityTransportDevice device;LocalMissionTransport transport(device.players,device.server,device.Hooks()); // 창/소켓 대신 기존 두 끝의 실제 전송 본체를 사용한다.
    CHECK(!transport.IsOpen() && !transport.HasClientConnection() && device.clockCalls==0);
    RegisterActivityTransportLogin(transport);transport.Open(6799,false);
    CHECK(transport.IsOpen() && !transport.HasClientConnection() && transport.Client().flags==0 && device.clockCalls==2);
    transport.Connect();CHECK(transport.HasClientConnection() && transport.Client().flags==0x26U && device.clockCalls==4);
    const auto received=transport.Client().received,heartbeat=transport.Client().heartbeat; // 존재 조회가 변경해서는 안 되는 같은 실제 연결 시각이다.
    for (const auto flags:{0x24U,0x66U,0U}) { // flags2 제거/종료flag40/전체0도 생성 완료한 객체를 반납하지 않는다.
        transport.Client().flags=flags;CHECK(transport.HasClientConnection() && transport.Client().flags==flags && device.clockCalls==4);
        CHECK(transport.Client().received==received && transport.Client().heartbeat==heartbeat && transport.Owner()==0 && transport.QueuedFrames()==0);
    }
    device.server=false;CHECK(transport.HasClientConnection() && device.clockCalls==4); // 서버 모드 변경도 같은 Client9의 생성 수명과 별개다.
    ActivityTransportDevice immediate;LocalMissionTransport connected(immediate.players,immediate.server,immediate.Hooks()); // 즉시 접속 경로는 별도 현재 전송 수명에서 확인한다.
    CHECK(!connected.HasClientConnection());RegisterActivityTransportLogin(connected);connected.Open(6799,true);
    CHECK(connected.IsOpen() && connected.HasClientConnection() && connected.Client().flags==0x26U && immediate.clockCalls==4);
}

// 생성 전 계약 실패와 Connect 중 시계 예외는 존재를 조기 공개하지 않으며 이미 쓴 시각은 보존한다.
TEST_CASE(ConnectionActivityTransport_FailedConnectKeepsPartialEffectsAndUnpublishedPresence) {
    ActivityTransportDevice device;LocalMissionTransport transport(device.players,device.server,device.Hooks());bool rejected{}; // 예외는 정상 객체 생성 완료와 구별해 관측한다.
    try { transport.Open(6799,false); }catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && !transport.IsOpen() && !transport.HasClientConnection() && device.clockCalls==0);
    RegisterActivityTransportLogin(transport);transport.Open(6799,false);device.stopAt=device.clockCalls+2;
    bool stopped{};try { transport.Connect(); }catch (const ActivityTransportClockStop&) { stopped=true; }
    CHECK(stopped && transport.IsOpen() && !transport.HasClientConnection() && transport.Client().flags==0 && device.clockCalls==4);
    CHECK(transport.Client().received==103 && transport.Client().heartbeat==0); // 첫 장치 성공의 실제 쓰기를 두 번째 장치 예외 뒤 임의 rollback하지 않는다.
    device.stopAt=0;transport.Connect();CHECK(transport.HasClientConnection() && transport.Client().flags==0x26U && device.clockCalls==6);
    CHECK(transport.Client().received==105 && transport.Client().heartbeat==106);
    rejected=false;try { transport.Connect(); }catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && transport.HasClientConnection() && transport.Client().received==105 && transport.Client().heartbeat==106 && device.clockCalls==6); // 완료 수명의 중복 Connect 거부는 시계 효과나 존재 해제를 추가하지 않는다.
}
