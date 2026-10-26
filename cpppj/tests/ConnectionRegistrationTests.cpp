// 원본288관측과 실제 로컬 양끝/Client 장기의 동일 표·반납 prefix를 검사한다.
#include "DudeSpawnSupport.h"
#include "o/RawConnectionRegistration.h"
#include "o/RawConnectionRegistry.h"
#include "client/ClientConnectionGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientMain.h"
#include "client/EditorLocalTransport.h"
#include "zacket/LocalMissionTransport.h"
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace { //최초 TSV 이외에 제품 출력으로 기대값을 만들지 않는다.
using namespace netstorm::o;
constexpr std::uint32_t kRegistrationNodes=0x12000100; //최초 독립 관측의 실제 가상 노드 주소다.
constexpr std::array<std::uint8_t,5> kRegistryMessage{1,0x24,5,0,77}; //현재 등록표가 검증할 실제 단일 헤더/길이 입력이다.
struct RegistrationStop {}; //외부 장치의 명시 중단이며 정상 반환과 구별한다.
// 원문 TSV 열을 마지막 열까지 읽는다.
std::vector<std::string> RegistrationFields(std::string_view line) { std::vector<std::string> result;std::size_t at{};for (;;) { const auto end=line.find('\t',at);result.emplace_back(line.substr(at,end==line.npos?line.npos:end-at));if (end==line.npos) return result;at=end+1; } } //각 반복은 현재 한 열만 소비한다.
// 최초 BYTE 배열을16진 열에서 복원한다.
std::vector<std::uint8_t> RegistrationBytes(std::string_view line) { std::vector<std::uint8_t> bytes;for (std::size_t at=0;at<line.size();at+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(line.substr(at,2)),nullptr,16)));return bytes; } //입력과 기대 출력에 따로 적용한다.
// little endian 네 BYTE를 읽는다.
std::uint32_t RegistrationRead(std::span<const std::uint8_t> bytes,std::size_t at) { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } //인접 필드를 포함하지 않는다.
// 관련 DWORD만 최초 전체 패턴에 반영한다.
void RegistrationPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } //미표현 Native BYTE는 원래 입력을 유지한다.
// 예상한 실제 계약 실패를 숨기지 않고 확인한다.
template<class Function> bool RegistrationRejects(Function&& function) { try { function();return false; }catch (const std::exception&) { return true; } }
// 실제 장치 호출 수와 순서만 관측하며 연결/게임 성공값을 계산하지 않는다.
struct RegistryDevices {
    std::uint32_t now{100},closed{},released{}; //현재 시각과 실제 close/free의 발화 수다.
    std::vector<std::string> trace; //실제 callback의 순서다.
    PacketDispatchHooks Hooks() { //실제 로컬 전송이 사용할 외부 시계·기록·소멸 장치다.
        PacketDispatchHooks hooks;hooks.now=[this] { return ++now; };hooks.record=[](auto&,auto,auto) {};hooks.terminate=[](auto& connection) { connection.flags|=0x40U; };
        hooks.close=[this](auto& connection) { CHECK((connection.flags&0x40U) && !connection.depth);++closed;trace.push_back("close"); };hooks.release=[this](auto&) { ++released;trace.push_back("release"); };return hooks;
    }
};
// 현재 실제 노드의 두 장치를 동일 Registry 토큰에 연결한다.
std::uint32_t TrackRegistryNode(RawConnectionRegistry& registry,PacketConnection& node,RegistryDevices& devices) { const auto hooks=devices.Hooks();return registry.Track(node,[&node,hooks] { hooks.close(node); },[&node,hooks] { hooks.release(node); }); }
}

// 실제 등록/제거/재배치288행을 전체 표·개수·관련 노드 BYTE·중단 효과열로 대조한다.
TEST_CASE(ConnectionRegistration_IndependentNative288AndCallbackPrefix) {
    std::ifstream file(std::filesystem::path(__FILE__).parent_path()/"fixtures/connection-registration-x86.tsv");CHECK(bool(file));std::string line;std::size_t rows{},stopped{};
    while (std::getline(file,line)) { //두 FPU 환경과 진단/버퍼 callback 중단을 포함한 최초 전체 행이다.
        if (line.empty() || line.front()=='#') continue;if (line.back()=='\r') line.pop_back();const auto row=RegistrationFields(line);CHECK(row.size()==11);if (row.size()!=11) continue;auto bytes=RegistrationBytes(row[7]);CHECK(bytes.size()==800);if (bytes.size()!=800) continue;
        std::array<std::uint32_t,22> main{},buffers{};std::array<std::uint32_t,32> pending{};std::array<PacketConnection,3> nodes{};std::uint32_t sync{};
        for (std::size_t i=0;i<main.size();++i) main[i]=RegistrationRead(bytes,i*4); //실제 양수 현재 표다.
        for (std::size_t i=0;i<pending.size();++i) pending[i]=RegistrationRead(bytes,88+i*4); //pending0을 포함한32칸을 보존한다.
        for (std::size_t i=0;i<nodes.size();++i) { nodes[i].flags=RegistrationRead(bytes,224+i*192+0x34);nodes[i].sender=RegistrationRead(bytes,224+i*192+0x40); } //Native가 쓰는 실제 연결 두 필드다.
        auto count=RegistrationRead(bytes,216),server=RegistrationRead(bytes,220);ConnectionCleanupGlobals globals{main,buffers,pending,sync,count};const auto source=static_cast<std::int32_t>(std::stoll(row[5])),target=static_cast<std::int32_t>(std::stoll(row[6]));std::string calls;bool interrupted{};ConnectionRegistrationHooks hooks;
        hooks.node=[&](auto token)->PacketConnection& { if (token<kRegistrationNodes || (token-kRegistrationNodes)%192 || (token-kRegistrationNodes)/192>=nodes.size()) throw std::out_of_range("최초 연결 등록 노드 범위");return nodes[(token-kRegistrationNodes)/192]; };
        const auto effect=[&](char kind,std::uint32_t value) { if (!calls.empty()) calls+=';';calls+=std::string(1,kind)+':'+std::to_string(value);if (kind=='B') { hooks.node(value).flags|=2;if (std::stoul(row[2])) { count=0;main[static_cast<std::size_t>(target)]=kRegistrationNodes+384;pending[static_cast<std::size_t>(-source)]=kRegistrationNodes+384; } }if (row[3]==std::string(1,kind)) throw RegistrationStop{}; };
        hooks.buffered=[&](auto token) { effect('B',token); };hooks.assertion=[&](auto value) { effect('A',value); };
        try { switch (std::stoul(row[0])) { case 0:RegisterTransceiver(target,kRegistrationNodes,globals,hooks);break;case 1:RemovePendingConnection(source,globals);break;case 2:CHECK(AssignPendingConnection(source,target,server,globals,hooks)==target);break;default:CHECK(false);break; } }catch (const RegistrationStop&) { interrupted=true;++stopped; }
        for (std::size_t i=0;i<main.size();++i) RegistrationPut(bytes,i*4,main[i]); //각 현재 표를 C++ 결과에서 직접 읽는다.
        for (std::size_t i=0;i<pending.size();++i) RegistrationPut(bytes,88+i*4,pending[i]); //제거 뒤 나머지 칸도 원문과 같아야 한다.
        RegistrationPut(bytes,216,count);RegistrationPut(bytes,220,server);
        for (std::size_t i=0;i<nodes.size();++i) { RegistrationPut(bytes,224+i*192+0x34,nodes[i].flags);RegistrationPut(bytes,224+i*192+0x40,nodes[i].sender);CHECK(!nodes[i].received && !nodes[i].heartbeat && !nodes[i].depth && !nodes[i].delayed); } //호스트의 다른 표현 필드도 바꾸지 않는다.
        CHECK(bytes==RegistrationBytes(row[8]) && (calls.empty()?"-":calls)==row[9] && interrupted==(row[10]=="1") && !sync);++rows;
    }
    CHECK(rows==288 && stopped==60);
}

// 실제 같은 노드의 임시 두 별칭과 buffered 뒤 pending 제거 순서를 확인한다.
TEST_CASE(ConnectionRegistration_RegistryMovesSameLiveNodeAfterBufferedEffect) {
    RegistryDevices devices;PacketConnection node;RawConnectionRegistry registry;const auto token=TrackRegistryNode(registry,node,devices);CHECK(registry.Accept(token)==-1 && registry.Cleanup().pendingCount==1);
    bool observed{};registry.Assign(-1,3,1,[&](auto current) { const auto view=registry.Cleanup();CHECK(current==token && &registry.Node(current)==&node && node.sender==3 && view.transceivers[3]==token && view.pending[1]==token && view.pendingCount==1);node.flags|=2;observed=true; });
    CHECK(observed && registry.Cleanup().pending[1]==0 && registry.Cleanup().pendingCount==0 && registry.Cleanup().transceivers[3]==token && !devices.closed);registry.EndPlayers(1);CHECK(!registry.Owns(token) && !registry.Cleanup().transceivers[3] && devices.trace==std::vector<std::string>({"close","release"}));
}

// 원문 부모 중 소멸/free가 실패하면 먼저 쓴 sync/flags와 현재 슬롯·장치를 유지한다.
TEST_CASE(ConnectionRegistration_RegistryFailedFreeKeepsActualPrefix) {
    RawConnectionRegistry registry;PacketConnection node;std::uint32_t destroyed{};const auto token=registry.Track(node,[&] { ++destroyed; },[] { throw std::logic_error("명시 실제 free 중단"); });registry.Register(9,token);registry.Cleanup().synchronization=token;
    CHECK(RegistrationRejects([&] { registry.Disconnect(9); }) && destroyed==1 && (node.flags&0x40U) && !registry.Cleanup().synchronization && registry.Cleanup().transceivers[9]==token && registry.Owns(token));registry.Forget(token);
}

// 중단한 buffered는 이미 등록한 양수/sender를 되돌리지 않고 pending/count도 앞당겨 지우지 않는다.
TEST_CASE(ConnectionRegistration_RegistryBufferedFailureRetainsTemporaryAliases) {
    RegistryDevices devices;PacketConnection node;RawConnectionRegistry registry;const auto token=TrackRegistryNode(registry,node,devices);registry.Accept(token);
    CHECK(RegistrationRejects([&] { registry.Assign(-1,4,1,[](auto) { throw std::logic_error("실제 buffered 중단"); }); }) && registry.Cleanup().transceivers[4]==token && registry.Cleanup().pending[1]==token && registry.Cleanup().pendingCount==1 && node.sender==4 && registry.Owns(token));registry.Forget(token);
}

// 실제 Editor 양끝이 같은 장기 표에 등록되고 Native 정리의 실제 소멸/free로 반납된다.
TEST_CASE(ConnectionRegistration_EditorUsesSharedCurrentTableAndNativeClose) {
    using namespace netstorm::client;RegistryDevices devices;std::array<std::uint8_t,1620> players{};bool server=true;auto registry=std::make_shared<RawConnectionRegistry>();EditorLocalTransport wire(players,server,devices.Hooks(),registry);wire.Open(6799);const auto client=registry->Cleanup().transceivers[9];CHECK(client && &registry->Node(client)==&wire.Client() && wire.Registry()==registry);wire.Poll();const auto accepted=registry->Cleanup().pending[1];CHECK(accepted && &registry->Node(accepted)==&wire.Pending() && registry->Cleanup().pendingCount==1);
    registry->CloseAll();CHECK(!wire.ClientAlive() && wire.PendingAlive() && !registry->Cleanup().transceivers[9] && devices.closed==1 && registry->Owns(accepted));registry->Disconnect(-1);CHECK(!wire.PendingAlive() && !registry->Cleanup().pending[1] && registry->Cleanup().pendingCount==0 && devices.closed==2 && devices.released==2);
}

// 이미 있는 진짜 pending을 보존하고 다음 빈 음수 ID에서 수신 중 종료 개수를 한 번만 줄인다.
TEST_CASE(ConnectionRegistration_EditorDeferredPendingUsesActualNegativeSlot) {
    using namespace netstorm::client;RegistryDevices devices;PacketConnection first;std::array<std::uint8_t,1620> players{};bool server=true;auto registry=std::make_shared<RawConnectionRegistry>();const auto older=TrackRegistryNode(*registry,first,devices);registry->Accept(older);EditorLocalTransport wire(players,server,devices.Hooks(),registry);wire.Open(6799);wire.Poll();const auto token=registry->Cleanup().pending[2];CHECK(token && wire.Pending().sender==0xfffffffeU && registry->Cleanup().pendingCount==2);registry->Cleanup().synchronization=token;
    wire.Dispatch().Version(0x24,1,0);wire.Dispatch().Register(0x24,[&](auto sender,auto& connection,auto) { CHECK(sender==0xfffffffeU && connection.depth==1);wire.Disconnect(-2);CHECK(wire.PendingAlive() && registry->Cleanup().pendingCount==1 && registry->Cleanup().pending[2]==0 && registry->Owns(token)); });CHECK(wire.Send(9,kRegistryMessage));wire.Poll();
    CHECK(!wire.PendingAlive() && !registry->Owns(token) && registry->Cleanup().pendingCount==1 && registry->Cleanup().pending[1]==older && registry->Cleanup().synchronization==token && devices.closed==1 && devices.released==1);registry->Disconnect(-1);
}

// 일반 미션의 실제 서버/Client9도 같은 표에 생성·재배치되고 선택된 owner만 종료된다.
TEST_CASE(ConnectionRegistration_LocalMissionSharesPendingOwnerAndClientNine) {
    RegistryDevices devices;std::array<std::uint8_t,1620> players{};bool server=true;auto registry=std::make_shared<RawConnectionRegistry>();netstorm::zacket::LocalMissionTransport wire(players,server,devices.Hooks(),registry);wire.RegisterLogin([](auto,auto&,auto) {});wire.Open(6799,false);const auto token=registry->Cleanup().pending[1];CHECK(token && &registry->Node(token)==&wire.Server() && !registry->Cleanup().transceivers[9]);wire.Connect();const auto client=registry->Cleanup().transceivers[9];CHECK(client && &registry->Node(client)==&wire.Client());wire.AssignConnection(-1,3);
    CHECK(registry->Cleanup().transceivers[3]==token && !registry->Cleanup().pending[1] && !registry->Cleanup().pendingCount && wire.Owner()==3 && (wire.Server().flags&2));registry->EndPlayers(1);CHECK(!registry->Owns(token) && wire.HasClientConnection() && !wire.Owner() && registry->Cleanup().transceivers[9]==client);registry->CloseAll();CHECK(!wire.HasClientConnection() && !registry->Owns(client) && devices.closed==2 && devices.released==2);
}

// Client보다 오래 빌린 연결표도 실제 동기화 DWORD owner를 유지하고 복사하지 않는다.
TEST_CASE(ConnectionRegistration_ClientSynchronizationOwnerSurvivesBorrower) {
    using namespace netstorm::client;std::shared_ptr<RawConnectionRegistry> registry;std::weak_ptr<ClientLocalStateGlobals> lifetime;std::uint32_t* address{};
    { const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));registry=client.ConnectionGlobals();lifetime=client.LocalStateGlobals();address=&client.LocalStateGlobals()->ClockSynchronization().connection;registry->Cleanup().synchronization=71;CHECK(*address==71 && &registry->Cleanup().synchronization==address); }
    CHECK(!lifetime.expired() && &registry->Cleanup().synchronization==address && registry->Cleanup().synchronization==71);registry.reset();CHECK(lifetime.expired());
}
