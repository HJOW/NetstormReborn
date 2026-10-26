// 연결 정리의 독립 원문580관측과 실제 Client 장기 보관·Editor 결합을 검증한다.
#include "DudeSpawnSupport.h"
#include "o/RawConnectionCleanup.h"
#include "client/ClientConnectionGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientMain.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace { //최초 입력/출력을 별도 TSV 열에서 읽고 제품으로 기대값을 계산하지 않는다.
using namespace netstorm::o;
constexpr std::uint32_t kNodes=0x12000100; //Native 격리 입력의 첫 연결 토큰이다.
constexpr std::size_t kNodeOffset=316; //양수176BYTE/pending128BYTE/세DWORD 뒤 실제 노드 입력이다.
struct ConnectionStop {}; //선택 외부 장치의 명시 중단이며 정상 반환으로 삼지 않는다.
// 마지막 빈 열까지 구분해 원문 관측의 열을 유지한다.
std::vector<std::string> CleanupFields(std::string_view line) { std::vector<std::string> result;std::size_t at{};for (;;) { const auto end=line.find('\t',at);result.emplace_back(line.substr(at,end==line.npos?line.npos:end-at));if (end==line.npos) return result;at=end+1; } } //각 반복은 한 원문 열만 소비한다.
// 최초16진 BYTE 배열을 복원한다.
std::vector<std::uint8_t> CleanupBytes(std::string_view text) { std::vector<std::uint8_t> result;for (std::size_t at=0;at<text.size();at+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(at,2)),nullptr,16)));return result; } //TSV 입력과 출력에 각각 적용한다.
// little endian의 정확한4BYTE만 읽는다.
std::uint32_t CleanupRead(std::span<const std::uint8_t> bytes,std::size_t at) { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } //이웃 필드는 포함하지 않는다.
// 접근한 DWORD만 원래 전체 패턴에 반영한다.
void CleanupPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } //노드의 미사용 BYTE는 그대로 둔다.
// 필요한 실제 장치가 빠진 위치의 실패만 확인한다.
template<class Function> bool CleanupRejects(Function&& function) { try { function();return false; }catch (const std::exception&) { return true; } }
}

// 실제 여섯 PE 몸체의 전체 표/노드/동기화/개수/효과열과 중단 prefix를 비교한다.
TEST_CASE(ConnectionCleanup_IndependentNative580AndCurrentSlotEffects) {
    std::ifstream file(std::filesystem::path(__FILE__).parent_path()/"fixtures/connection-cleanup-x86.tsv");CHECK(bool(file));std::string line;std::size_t rows{},stopped{};
    while (std::getline(file,line)) { //두 FPU 환경과 callback의 뒤 슬롯/깊이/개수 변경까지 최초 모든 행을 재생한다.
        if (line.empty() || line.front()=='#') continue;if (line.back()=='\r') line.pop_back();const auto row=CleanupFields(line);CHECK(row.size()==11);if (row.size()!=11) continue;auto bytes=CleanupBytes(row[6]);CHECK(bytes.size()==892);if (bytes.size()!=892) continue;
        std::array<std::uint32_t,22> main{},buffers{};std::array<std::uint32_t,32> pending{};std::array<PacketConnection,3> nodes{};
        for (std::size_t index=0;index<22;++index) { main[index]=CleanupRead(bytes,index*4);buffers[index]=CleanupRead(bytes,88+index*4); } //원문 양수 연결과 별도 버퍼표다.
        for (std::size_t index=0;index<32;++index) pending[index]=CleanupRead(bytes,176+index*4); //전체 정리의0칸도 보존한다.
        for (std::size_t index=0;index<3;++index) { auto& node=nodes[index];const auto at=kNodeOffset+192*index;node.flags=CleanupRead(bytes,at+0x34);node.sender=CleanupRead(bytes,at+0x40);node.received=CleanupRead(bytes,at+0x44);node.heartbeat=CleanupRead(bytes,at+0x48);node.depth=CleanupRead(bytes,at+0x8c); } //정리 대상과 보존해야 할 현재 sender/두 시각을 같은 실제 필드에 입력한다.
        auto sync=CleanupRead(bytes,304),count=CleanupRead(bytes,308),server=CleanupRead(bytes,312);ConnectionCleanupGlobals globals{main,buffers,pending,sync,count};std::string calls;bool changed{},interrupted{};ConnectionCleanupHooks hooks;
        hooks.node=[&](auto token)->PacketConnection& { if (token<kNodes || (token-kNodes)%192 || (token-kNodes)/192>=nodes.size()) throw std::out_of_range("원문 연결 토큰 범위");return nodes[(token-kNodes)/192]; };
        const auto effect=[&](char kind,std::uint32_t token) {
            if (!calls.empty()) calls+=';';calls+=std::string(1,kind)+':'+std::to_string(token);
            if (std::stoul(row[2]) && !changed && kind=='D') { changed=true;main[21]=kNodes+384;pending[31]=kNodes+384;sync=kNodes+384;count=0xffffffffU;server=0;hooks.node(token).depth=7; }
            if (row[3]==std::string(1,kind)) throw ConnectionStop{};
        };
        hooks.destroy=[&](auto token) { effect('D',token); };hooks.release=[&](auto token) { effect('F',token); };hooks.assertion=[&](auto line) { effect('A',line); };
        const auto argument=static_cast<std::int32_t>(std::stoll(row[5]));
        try { switch (std::stoul(row[0])) {
            case 0:DeferConnectionClose(nodes[0],[&] { effect('D',kNodes); },[&] { effect('F',kNodes); });break;
            case 1:ClosePendingConnection(argument,globals,hooks);break;case 2:DisconnectTransceiver(argument,globals,hooks);break;
            case 3:CloseAllTransceivers(globals,hooks);break;case 4:EndPlayerConnections(server,globals,hooks);break;case 5:CloseAllPendingConnections(globals,hooks);break;default:CHECK(false);break;
        } }catch (const ConnectionStop&) { interrupted=true;++stopped; }
        for (std::size_t index=0;index<22;++index) { CleanupPut(bytes,index*4,main[index]);CleanupPut(bytes,88+index*4,buffers[index]); } //모든 표를 기대 출력과 독립적으로 직렬화한다.
        for (std::size_t index=0;index<32;++index) CleanupPut(bytes,176+index*4,pending[index]); //pending0의 자연 보존/정리를 둘 다 확인한다.
        CleanupPut(bytes,304,sync);CleanupPut(bytes,308,count);CleanupPut(bytes,312,server);
        for (std::size_t index=0;index<3;++index) { const auto& node=nodes[index];const auto at=kNodeOffset+192*index;CleanupPut(bytes,at+0x34,node.flags);CleanupPut(bytes,at+0x40,node.sender);CleanupPut(bytes,at+0x44,node.received);CleanupPut(bytes,at+0x48,node.heartbeat);CleanupPut(bytes,at+0x8c,node.depth);CHECK(!node.delayed); } //C++가 표현하는 보존 필드도 직접 대조하며 미표현 BYTE는 처음 패턴 그대로다.
        CHECK(bytes==CleanupBytes(row[7]) && (calls.empty()?"-":calls)==row[8] && interrupted==(row[9]=="1"));++rows;
    }
    CHECK(rows==580 && stopped==268);
}

// 누락 장치를 선행 검사로 덮지 않으며 flags/sync와 실제 소멸 효과의 완료 prefix를 남긴다.
TEST_CASE(ConnectionCleanup_LateMissingEffectsPreserveNativePrefix) {
    std::array<std::uint32_t,22> main{},buffers{};std::array<std::uint32_t,32> pending{};std::uint32_t sync=71,count=1;ConnectionCleanupGlobals globals{main,buffers,pending,sync,count};PacketConnection node;ConnectionCleanupHooks hooks;main[9]=71;buffers[9]=72;hooks.node=[&](auto token)->PacketConnection& { CHECK(token==71);return node; };
    CHECK(CleanupRejects([&] { DisconnectTransceiver(9,globals,hooks); }) && !sync && main[9]==71 && buffers[9]==72 && (node.flags&0x40));
    std::uint32_t destroyed{};hooks.destroy=[&](auto token) { CHECK(token==71);++destroyed;node.depth=7; };node.depth=0;
    CHECK(CleanupRejects([&] { DisconnectTransceiver(9,globals,hooks); }) && destroyed==1 && node.depth==7 && main[9]==71 && buffers[9]==72);
    node.depth=1;CHECK(CleanupRejects([&] { DisconnectTransceiver(9,globals,hooks); }) && main[9]==0 && buffers[9]==72 && destroyed==1); //수신 중 연결은 실제로 분리하지만 뒤 버퍼 free 장치의 실패는 남긴다.
}

// Native 음수 경계의 자연 반환과 양수 진단이 실제 선택 위치에서만 장치를 요구한다.
TEST_CASE(ConnectionCleanup_EmptyAndNegativeBoundsDemandNoDevices) {
    std::array<std::uint32_t,22> main{},buffers{};std::array<std::uint32_t,32> pending{};std::uint32_t sync=19,count=7;ConnectionCleanupGlobals globals{main,buffers,pending,sync,count};
    for (const std::int32_t id:{-2147483647-1,-33,-32,0,1,21}) DisconnectTransceiver(id,globals,{}); //없는 대상과 범위 밖 음수는 종료 장치를 요구하지 않는다.
    CloseAllTransceivers(globals,{});EndPlayerConnections(1,globals,{});CloseAllPendingConnections(globals,{});CHECK(sync==19 && count==7);
    CHECK(CleanupRejects([&] { DisconnectTransceiver(22,globals,{}); }));std::uint32_t line{};ConnectionCleanupHooks hooks;hooks.assertion=[&](auto value) { line=value; };CHECK(CleanupRejects([&] { DisconnectTransceiver(22,globals,hooks); }) && line==250); //진단 복귀 뒤 부족한 실제 span을 호스트에서 명시 거부한다.
}

// 같은 Client의 표/개수는 게임 전역 대여/반납과 독립이며 실제 동기화 DWORD만 빌린다.
TEST_CASE(ConnectionCleanup_ClientLongTablesPreserveAddressAndCurrentValues) {
    using namespace netstorm::client;const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));const auto owner=client.ConnectionGlobals();auto globals=owner->Cleanup(client.LocalStateGlobals()->ClockSynchronization().connection);CHECK(globals.transceivers.size()==22 && globals.buffers.size()==22 && globals.pending.size()==32);const auto* address=globals.transceivers.data();globals.transceivers[21]=71;globals.buffers[21]=72;globals.pending[31]=73;globals.pendingCount=0x89abcdefU;globals.synchronization=74;
    { auto use=client.GameplayGlobals()->AcquirePrepared();CHECK(client.ConnectionGlobals()==owner); }
    const auto current=client.ConnectionGlobals()->Cleanup(client.LocalStateGlobals()->ClockSynchronization().connection);CHECK(current.transceivers.data()==address && current.transceivers[21]==71 && current.buffers[21]==72 && current.pending[31]==73 && current.pendingCount==0x89abcdefU && current.synchronization==74);
}
