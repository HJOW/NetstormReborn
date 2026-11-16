// 비MP 미션 전환의 현재 전역 노드와 실제 flush·수신 장치·수명을 검사하며 LAN/게임 창을 실행하지 않는다.
#include "TestSupport.h"
#include "client/ClientConnectionGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientPacketSources.h"
#include "client/CompositeMissionTransport.h"
#include "o/RawConnectionRegistry.h"
#include "o/RawConnectionEnded.h" //이전 끝의 실제 type2가 현재 등록 칸을 반납하는 원문 부모를 사용한다.
#include <array>
#include <memory>
#include <stdexcept>
#include <string>

namespace { // 실제 내부 미션 장치에 전달하는 고정 Player/시계만 검사 수명으로 소유한다.
// 이전 Source와 다음 Source가 함께 빌리는 실제 전역 자료와 종료 관측을 보관한다.
struct MissionFlushGlobals {
    std::array<std::uint8_t,9*180> players{}; // Client와 전송이 함께 빌리는 아홉 Player 행이다.
    bool server{true}; // 내부 서버 생성에 필요한 실제 권한 입력이다.
    std::uint32_t now{}; // 두 끝의 접속 시각을 구별하는 명시 외부 시계다.
    std::array<std::uint32_t,netstorm::o::kConnectionActivityEntries> activity{}; //종료 통지의 원문 활동22DWORD다.
    std::size_t ended{}; //실제 종료 부모의 Player 방송 경계 도달 횟수다.
};
// 실제 내부 미션 bootstrap만 준비하고 로그인 성공·플레이 상태를 합성하지 않는다.
std::shared_ptr<netstorm::client::CompositeMissionTransport> MakeMissionFlushTransport(const std::shared_ptr<netstorm::client::ClientConnectionGlobals>& registry,const std::shared_ptr<MissionFlushGlobals>& globals) {
    netstorm::o::PacketDispatchHooks hooks;hooks.now=[globals] { return ++globals->now; };
    hooks.record=[](auto&,auto,auto) {};hooks.terminate=[](auto& node) { node.flags|=0x40U; };hooks.close=[](auto&) {};hooks.release=[](auto&) {}; // 검사에서 OS/로그 기록은 요구하지 않는 외부 경계다.
    auto transport=netstorm::client::CompositeMissionTransport::Create(netstorm::client::CompositeTransportRole::InternalMission,globals->players,globals->server,std::move(hooks),registry,globals);
    registry->PacketSources()->Publish(transport);
    transport->Dispatch().Register(0x27,[](auto,auto&,auto) { throw std::logic_error("flush 검사에서 실제 로그인 본문은 호출하지 않음"); }); // Open의 필수 등록만 제공하며 이 검사는 로그인 패킷을 보내지 않는다.
    transport->Dispatch().Register(2,[registry,globals](auto sender,auto& connection,auto) { //실제 종료 통지를 같은 원문 부모와 등록표의 현재 칸에 연결한다.
        netstorm::o::ConnectionEndedHooks effects;effects.disconnect=[registry](auto id) { registry->Disconnect(id); };effects.broadcastPlayer=[globals](auto owner) { CHECK(owner==1);++globals->ended; };
        const std::uint32_t phase=8;std::uint32_t nextPhase{};auto slots=registry->Cleanup();netstorm::o::ReceiveConnectionEnded(static_cast<std::int32_t>(sender),registry->TokenFor(connection),{globals->server,slots.synchronization,phase,nextPhase,globals->activity,[globals] { return std::span<std::uint8_t>(globals->players); }},effects);
    });
    return transport;
}
}

// State7이 옛 client9만 끊은 뒤 새 미션의 첫 flush는 남아 있는 옛 server1의 실제 장치를 호출해야 한다.
TEST_CASE(MissionConnectionFlush_NewMissionAcceptsPreviousSourceNode) {
    using namespace netstorm::client;
    auto state=std::make_shared<ClientLocalStateGlobals>();auto registry=std::make_shared<ClientConnectionGlobals>(state);auto globals=std::make_shared<MissionFlushGlobals>();
    auto previous=MakeMissionFlushTransport(registry,globals);previous->Serve(6799);const std::array<std::uint8_t,20> address{};previous->Connect(address,6799);previous->Poll();previous->Assign(-1,1);
    const auto oldToken=registry->Cleanup().transceivers[1];CHECK(oldToken && previous->Connection(1));registry->Disconnect(9);CHECK(registry->Cleanup().transceivers[1]==oldToken);
    const std::weak_ptr<CompositeMissionTransport> retained=previous;previous.reset();CHECK(!retained.expired()); // 등록된 실제 노드가 옛 Source의 수명을 계속 소유한다.
    auto next=MakeMissionFlushTransport(registry,globals);next->Serve(6799);next->Connect(address,6799);
    next->Flush();CHECK(registry->Cleanup().transceivers[1]==oldToken && next->Connection(9)); // 전역 칸을 강제로 지우거나 가짜 ACK를 만들지 않는다.
    next->Poll();CHECK(!registry->Cleanup().transceivers[1] && globals->ended==1 && retained.expired()); //옛 끝의 실제 type2를 다음 전역 ID 순회에서 소비한 뒤에만 같은 칸을 다시 쓴다.
    next->Assign(-1,1);CHECK(next->Connection(1) && registry->Cleanup().transceivers[1]!=oldToken); //정리 누락을 등록 진단 생략으로 우회하지 않는다.
    next->CloseRoom();registry->ShutdownDevices();CHECK(!registry->Cleanup().transceivers[1] && retained.expired());
}

// flush가 자기 등록과 외부 owner를 반납해도 선택한 장치/함수는 실제 호출 복귀까지 살아 있어야 한다.
TEST_CASE(MissionConnectionFlush_DirectCallbackKeepsSelectedOwnerAndFunction) {
    netstorm::o::RawConnectionRegistry registry;netstorm::o::PacketConnection node;auto owner=std::make_shared<int>(7);const std::weak_ptr<int> lifetime=owner;
    registry.BindDevicePin([&](const auto&)->std::shared_ptr<void> { return owner; },true);std::uint32_t token{};bool called{};
    token=registry.Track(node,[] {},[] {},{},{},{},[&] { called=true;registry.Forget(token);owner.reset();CHECK(!lifetime.expired()); });registry.Register(1,token);
    registry.FlushDirect(token);CHECK(called && lifetime.expired() && !registry.Owns(token));
}

// 직접 수신도 callback의 자기 반납 뒤 실제 소비/sentinel 값을 원래 그대로 반환한다.
TEST_CASE(MissionConnectionFlush_DirectReceiveKeepsOwnerAndActualResult) {
    netstorm::o::RawConnectionRegistry registry;netstorm::o::PacketConnection node;auto owner=std::make_shared<int>(7);const std::weak_ptr<int> lifetime=owner;
    registry.BindDevicePin([&](const auto&)->std::shared_ptr<void> { return owner; },true);std::uint32_t token{};
    token=registry.Track(node,[] {},[] {},{},{},{},{},[&] { registry.Forget(token);owner.reset();CHECK(!lifetime.expired());return netstorm::o::TransportPollResult{true,false}; });
    const auto result=registry.ReceiveDirect(token);CHECK(result.consumed && !result.again && lifetime.expired() && !registry.Owns(token));
}

// 미연결 장치를 성공으로 숨기지 않고 실제 선택된 토큰까지 오류에 남긴다.
TEST_CASE(MissionConnectionFlush_MissingDeviceStillFailsExplicitly) {
    netstorm::o::RawConnectionRegistry registry;netstorm::o::PacketConnection node;const auto token=registry.Track(node,[] {},[] {});bool rejected{};
    try { registry.FlushDirect(token); }catch (const std::logic_error& error) { rejected=std::string(error.what()).find("token="+std::to_string(token))!=std::string::npos; }
    CHECK(rejected && registry.Owns(token));registry.Forget(token);
}
