// 실제 원본 Zone/로그인 전체 작성 바이트와 같은 Player/분배기의 로컬 연결을 검사한다.
#include "RawSceneSupport.h"
#include "zacket/LocalMissionTransport.h"
#include "o/RawPlayerParticipation.h"
#include <set>
using namespace netstorm::o;
using namespace netstorm::zacket;
using namespace netstorm::test::rawscene;
namespace {
// 원본 비정렬 DWORD만 같은 현재 기록에 쓴다.
void TransportPut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    // 주변 바이트를 변경하지 않고 낮은 바이트부터 기록한다.
    for (std::size_t index=0;index<4;++index) bytes[offset+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// 외부 시계/기록/종료/반납의 실제 호출을 관찰하며 패킷 판단을 대신하지 않는다.
struct TransportEffects {
    std::uint32_t now{100},records{},terminations{},closes{},releases{}; // 현재 시각 입력과 각 실제 효과 호출 수다.
    std::vector<std::uint32_t> senders,types; // 실제 분배가 기록한 현재 연결 번호와 메시지다.
    // 같은 관찰 저장소를 빌리는 실제 분배 효과를 만든다.
    PacketDispatchHooks Hooks() {
        PacketDispatchHooks hooks;hooks.now=[this] { return ++now; };
        hooks.record=[this](PacketConnection& connection,std::span<const std::uint8_t> bytes,std::uint16_t length) { CHECK(length==Get(bytes,2,2));++records;senders.push_back(connection.sender);types.push_back(bytes[1]); };
        hooks.terminate=[this](PacketConnection& connection) { ++terminations;connection.flags|=0x40; };
        hooks.close=[this](PacketConnection& connection) { ++closes;CHECK((connection.flags&0x40)!=0 && connection.depth==0); };
        hooks.release=[this](PacketConnection&) { ++releases; };return hooks;
    }
};
}
// 독립 기계어66입력의 실제 작성/설치와 집합 분배 상태/효과/반환을 전수 대조한다.
TEST_CASE(LocalMissionTransport_Native66) {
    std::size_t count=0;std::set<std::string> unique;
    // 원본 관찰 행을 순서대로 읽으며 자체 계산한 기대 packet/Player 모델을 만들지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_LOCAL_MISSION_TRANSPORT_FIXTURE).rows) {
        if (row[0]=="kind") continue;CHECK(row.size()==6 && unique.insert(row[0]+":"+row[1]).second);const auto fields=Split(row[1],',');const auto seed=Number(fields[0]),nameLength=Number(fields[1]),imageLength=Number(fields[2]);
        if (row[0]=="Zone") {
            const auto packet=MakeLocalMissionZone();CHECK(Hex(packet.storage)==row[2] && Hex(packet.Bytes())==row[3]);
            CHECK(packet.storage.size()==512 && packet.Bytes().size()==86 && packet.storage[14]==1 && packet.storage[15]==0);
        } else if (row[0]=="Login") {
            std::vector<std::uint8_t> image(imageLength); // 원본과 C++에 공급하는 외부 파일 이미지다.
            // 동일 입력 패턴만 만들며 패킷 헤더/배치 기대값을 계산하지 않는다.
            for (std::size_t index=0;index<image.size();++index) image[index]=static_cast<std::uint8_t>(index*17+seed);
            const std::string name(nameLength,'N');const auto packet=MakeLocalMissionLogin(1,name,image);CHECK(Hex(packet.storage)==row[2] && Hex(packet.Bytes())==row[3]);
            const auto view=ReadLocalMissionLogin(packet.Bytes());CHECK(view.subscriber==1 && view.handle==name && std::equal(image.begin(),image.end(),view.fortImage.begin(),view.fortImage.end()));
        } else if (row[0]=="Batch" || row[0]=="BatchClosed") {
            std::array<std::uint8_t,1620> players{};bool server=true;std::uint32_t now=104;bool mutated=false;std::string trace;PacketDispatchHooks hooks; // 원본과 같은 즉시 연결/외부 시계 입력이다.
            const auto add=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value; }; // 실제 외부 효과의 원래 순서만 보관한다.
            hooks.now=[&] { ++now;add("Now,"+std::to_string(now));return now; };
            hooks.record=[&](PacketConnection& connection,std::span<const std::uint8_t> packet,std::uint16_t length) { add("Record,"+std::to_string(packet[1])+","+std::to_string(length)+","+std::to_string(connection.sender)); };
            hooks.terminate=[&](PacketConnection& connection) { add("Terminate");connection.flags|=0x40; };
            hooks.close=[&](PacketConnection&) { add("Close"); };hooks.release=[&](PacketConnection&) { add("Free"); };
            LocalMissionTransport transport(players,server,std::move(hooks));
            const auto receive=[&](std::uint32_t sender,PacketConnection& connection,std::span<const std::uint8_t> packet) {
                add("Receive,"+std::to_string(sender)+","+std::to_string(packet[1])+","+std::to_string(connection.depth));if (!mutated && imageLength) { connection.sender=1;mutated=true; }
            }; // 원본 native의 등록 receive 외부 경계와 같은 현재 sender 변경 입력이다.
            transport.RegisterLogin(receive);transport.Dispatch().Version(0x27,1,static_cast<std::uint8_t>(nameLength));transport.Dispatch().Version(0x24,1,0);transport.Dispatch().Register(0x24,receive);
            auto& connection=transport.Server();connection.sender=0xffffffffU;connection.flags=row[0]=="BatchClosed" ? 0x64U : 0x24U;connection.received=103;connection.heartbeat=104;
            std::vector<std::uint8_t> frame;
            // 원본이 보존한 전체 프레임을 읽으며 기대 헤더나 dispatch 결과를 새로 계산하지 않는다.
            for (std::size_t index=0;index<row[2].size();index+=2) frame.push_back(static_cast<std::uint8_t>(std::stoul(row[2].substr(index,2),nullptr,16)));
            const bool result=transport.DispatchFrame(connection,frame);const auto state=Split(row[4],',');CHECK(state.size()==6);
            CHECK(result==(Number(state[0])!=0) && connection.flags==Number(state[1]) && connection.sender==Number(state[2]) && connection.received==Number(state[3]) && connection.heartbeat==Number(state[4]) && connection.depth==Number(state[5]));
            CHECK((trace.empty() ? "-" : trace)==row[5]);
        } else {
            CHECK(row[0]=="Install" || row[0]=="InstallMutate");const bool mutate=row[0]=="InstallMutate";bool mutated=false;std::vector<std::uint8_t> zone(512,std::uint8_t{0xff});zone[0]=1;zone[1]=0x1a;zone[2]=86;zone[3]=0;
            TransportPut(zone,4,0x82000004);zone[8]=zone[9]=0;TransportPut(zone,10,0);zone[14]=1;zone[15]=0;TransportPut(zone,16,1);TransportPut(zone,20,seed&1);
            const std::string name(nameLength,'N');std::copy(name.begin(),name.end(),zone.begin()+24);zone[24+name.size()]=0;zone[56]=0;
            if (mutate) { zone[86]=1;TransportPut(zone,87,42);TransportPut(zone,91,0);const std::string next="Second";std::copy(next.begin(),next.end(),zone.begin()+95);zone[101]=zone[127]=0; }
            std::array<std::uint8_t,1620> players{};TransportPut(players,180+0x50,seed<<12);PlayerWorldState player;player.battleOwner=player.templateOwner=8;
            bool server=false,boss=false,adjust=false,mission=true,metamap=false;std::uint32_t subscriber=1,pending=seed+3,active=0,word=0,flags=0;std::uint8_t override=0;double early=0;std::span<std::uint8_t> core;std::array<std::uint8_t,10> cached{};std::string trace; // 실제 현재 Zone 전역 입력이다.
            LocalMissionZoneHooks hooks;hooks.loginTimeout=[&] { trace="Timeout";return 30; };hooks.seconds=[&] { trace+=";Time";return 100.5; };
            hooks.storeName=[&](std::uint32_t sub,std::string_view current) { trace+=";Name,"+std::to_string(sub)+","+Hex({reinterpret_cast<const std::uint8_t*>(current.data()),current.size()});if (mutate && !mutated) { TransportPut(zone,16,9);zone[14]=2;mutated=true; } };
            hooks.serve=[&](std::uint32_t port) { trace+=";Serve,"+std::to_string(port); };
            InstallLocalMissionZone(zone,players,player,{server,boss,subscriber,pending,active,word,flags,early,adjust,mission,override,metamap,core,cached},std::move(hooks),6799);
            CHECK(Hex(players)==row[2] && trace==row[5]);const auto state=Split(row[4],',');CHECK(state.size()==9);
            CHECK(player.battleOwner==Number(state[0]) && player.templateOwner==Number(state[1]) && player.playerKnown==Number(state[2]));
            CHECK(server==(Number(state[3])!=0) && boss==(Number(state[4])!=0) && active==Number(state[5]) && word==Number(state[6]) && flags==Number(state[7]));
            CHECK(std::bit_cast<std::uint64_t>(early)==std::stoull(state[8]) && core.data()==players.data()+180+0x6c && core.size()==10);
        }
        ++count;
    }
    CHECK(count==66);
}
// 같은 Zone/Player와 실제 CheckLogin이 pending-1을 owner1로 이동한 뒤 진짜0x27 바이트를 분배한다.
TEST_CASE(LocalMissionTransport_ZonePendingLoginAndReply) {
    std::array<std::uint8_t,1620> players{};std::array<std::uint32_t,81> allies{};std::array<std::uint8_t,9> hidden{};PlayerWorldState player;player.battleOwner=player.templateOwner=8;
    bool server=false,boss=false,adjust=false,mission=true,metamap=false;std::uint32_t subscriber=1,pending=7,active=0,word=0,flags=0,assignments=0,logins=0,replies=0;std::uint8_t override=0;double early=0;std::span<std::uint8_t> core;std::array<std::uint8_t,10> cached{}; // 같은 실제 현재 Zone/Player 저장소를 공유한다.
    TransportEffects effects;LocalMissionTransport transport(players,server,effects.Hooks());PlayerParticipationState participationState;participationState.localTransport=false;
    PlayerParticipationHooks participationHooks;participationHooks.assignConnection=[&](std::int32_t id,std::int32_t owner) { CHECK(id==-1 && owner==1);transport.AssignConnection(id,owner);++assignments; };
    RawPlayerParticipation participation(OriginalEdition::Patch1078,players,allies,hidden,participationState,std::move(participationHooks));
    const std::array<std::uint8_t,7> image{3,1,4,1,5,9,2};const auto login=MakeLocalMissionLogin(subscriber,"Local",image);
    transport.RegisterLogin([&](std::uint32_t sender,PacketConnection& connection,std::span<const std::uint8_t> bytes) {
        CHECK(sender==0xffffffffU && connection.sender==sender && connection.depth==1);const auto view=ReadLocalMissionLogin(bytes);
        CHECK(view.subscriber==subscriber && view.handle=="Local" && std::equal(image.begin(),image.end(),view.fortImage.begin(),view.fortImage.end()));
        CHECK(participation.CheckLogin(std::bit_cast<std::int32_t>(sender),1));CHECK(connection.sender==1 && participation.Brain(1)==0);++logins;
        const std::array<std::uint8_t,5> reply{1,0x24,5,0,77};CHECK(transport.Send(1,reply));transport.Flush();
    });
    transport.Dispatch().Version(0x24,1,0);transport.Dispatch().Register(0x24,[&](std::uint32_t sender,PacketConnection& connection,std::span<const std::uint8_t> packet) { CHECK(sender==9 && connection.depth==1 && packet[4]==77);++replies; });
    LocalMissionZoneHooks hooks;hooks.loginTimeout=[] { return 30; };hooks.seconds=[] { return 100.5; };
    hooks.storeName=[&](std::uint32_t sub,std::string_view name) { CHECK(sub==1 && name=="zsimpson"); };
    hooks.serve=[&](std::uint32_t port) { transport.Open(port); };
    const auto zone=MakeLocalMissionZone();InstallLocalMissionZone(zone.Bytes(),players,player,{server,boss,subscriber,pending,active,word,flags,early,adjust,mission,override,metamap,core,cached},std::move(hooks),6799);
    CHECK(server && boss && transport.IsOpen() && transport.Owner()==0 && player.battleOwner==1 && player.playerKnown==1 && Get(players,180+0x50)==8);
    CHECK(transport.Send(9,login.Bytes()) && transport.QueuedFrames()==0 && logins==0);transport.Flush();CHECK(transport.QueuedFrames()==1);
    CHECK(transport.Poll()==2 && assignments==1 && logins==1 && replies==1 && transport.Owner()==1 && transport.QueuedFrames()==0);
    CHECK(Get(players,180+0x50)==11 && (Get(players,180+0x50)&0x40U)==0 && transport.Server().depth==0 && transport.Client().depth==0);
    CHECK(effects.senders==std::vector<std::uint32_t>({0xffffffffU,9}) && effects.types==std::vector<std::uint32_t>({0x27,0x24}) && effects.records==2 && effects.terminations==0);
}
// 미연결 등록/현재 flags1/예외 중 depth와 callback 이후의 현재 sender를 함께 검사한다.
TEST_CASE(LocalMissionTransport_SelectedBoundariesAndDepth) {
    std::array<std::uint8_t,1620> players{};bool server=true;TransportEffects effects;LocalMissionTransport transport(players,server,effects.Hooks());
    bool rejected=false;try { transport.Open(6799); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && !transport.IsOpen());
    rejected=false;try { transport.RegisterLogin({}); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    transport.RegisterLogin([&](std::uint32_t,PacketConnection&,std::span<const std::uint8_t>) { throw std::logic_error("실제 상위 로그인 경계 실패 입력"); });transport.Open(6799);
    const std::array<std::uint8_t,1> image{1};const auto login=MakeLocalMissionLogin(1,"A",image);CHECK(!transport.Send(1,login.Bytes()));
    CHECK(transport.Send(9,login.Bytes()));transport.Flush();rejected=false;try { transport.Poll(); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && transport.Server().depth==0 && transport.Owner()==0);
    rejected=false;try { transport.AssignConnection(1,1); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    transport.AssignConnection(-1,1);CHECK(!transport.Send(1,login.Bytes()));players[180+0x50]=1;CHECK(transport.Send(1,login.Bytes()));
    auto shortPacket=login.storage;shortPacket.resize(17);rejected=false;try { ReadLocalMissionLogin(shortPacket); } catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);
}


//서버만열었을때현재클라이언트/Player는미접속이며Connect뒤실제pending큐로만요청이간다.
TEST_CASE(LocalMissionTransport_ServerOpenAndClientConnectAreSeparate) {
    std::array<std::uint8_t,9*0xb4> players{};bool server=true;TransportEffects effects;LocalMissionTransport wire(players,server,effects.Hooks());std::size_t received{}; //동일물리Player의0flag는끝까지입력하지않는다.
    wire.RegisterLogin([&](std::uint32_t sender,PacketConnection& connection,std::span<const std::uint8_t> packet) { CHECK(sender==0xffffffff && &connection==&wire.Server() && packet[1]==0x27);++received; });wire.Open(6799,false);CHECK(wire.Client().flags==0 && wire.Server().flags==0x24 && wire.Owner()==0 && effects.now==102);CHECK(wire.Server().received==101 && wire.Server().heartbeat==102);
    const std::array<std::uint8_t,2> image{1,2};const auto packet=MakeLocalMissionLogin(1,"You",image);CHECK(!wire.Send(9,packet.Bytes()) && wire.QueuedFrames()==0);wire.Connect();CHECK(wire.Client().flags==0x26 && wire.Client().received==103 && wire.Client().heartbeat==104);CHECK(wire.Owner()==0 && players[0xb4+0x50]==0);CHECK(wire.Send(9,packet.Bytes()));CHECK(!received && !wire.QueuedFrames());wire.Flush();CHECK(wire.QueuedFrames()==1 && wire.Poll()==1 && received==1);CHECK(wire.Server().depth==0 && wire.Owner()==0 && players[0xb4+0x50]==0);
    bool rejected{};try { wire.Connect(); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
}
