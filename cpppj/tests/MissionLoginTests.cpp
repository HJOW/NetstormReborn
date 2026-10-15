// 독립 원본 부모48입력의 전체 Player/현재 전역/packet/호출 순서를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawMissionLogin.h"
#include "zacket/LocalMissionTransport.h"
#include <set>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// callback 관찰과 실제 참여 처리가 같은 현재 Player 저장소를 공유한다.
struct LoginFixture {
    std::vector<std::uint8_t> players=std::vector<std::uint8_t>(1620),options=std::vector<std::uint8_t>(87); // 원본 전체 기록/옵션이다.
    std::array<std::uint32_t,81> allies{}; // 실제 참여 처리기가 빌리는 방향별 표다.
    std::array<std::uint8_t,9> hidden{}; // 같은 참여 모듈의 현재 숨김표다.
    std::array<std::uint8_t,6> zone{0x81,0x82,0x83,0x84,0x85,0x86}; // 실제 LoginBegin의6바이트 입력이다.
    std::array<std::uint8_t,10> cached{}; // metamap에서만 선택할 현재 CoreData다.
    std::span<std::uint8_t> core; // 실제00595350의 현재 선택을 관찰한다.
    PlayerWorldState player; // 현재 두 owner/known의 단일 저장소다.
    PlayerParticipationState participationState; // CheckLogin의 별도 전송 모드 입력이다.
    bool server{true},boss{true},simulconnect{},missionWorld{true},missionPost{true},metamap{},mutated{}; // 원본 현재 모드와 관찰자1회표식이다.
    std::uint32_t mySubscriber{1},fortCount{},lastPlayer{7},predictable{17},begin{},seed{0x12345678},battleSeed{0x87654321},burst{},clockStamp{0x55},baseClock{3000},clock{7501},mutate{}; // 원본 현재 입력/출력이다.
    double captureEnd{500.75}; // 현재 응답이 읽는 실제double 입력이다.
    std::vector<std::string> trace; // 실제 외부 효과의 전체 순서다.
    std::unique_ptr<RawPlayerParticipation> participation; // 같은 Player9에서 CheckLogin을 실제 실행한다.
    std::unique_ptr<RawMissionLogin> login; // 테스트 입력과 효과보다 먼저 해제되는 처리기다.
    // fixture 숫자 입력을 원본 실행기와 동일한 현재 저장소에만 배치한다.
    explicit LoginFixture(const std::vector<std::string>& values) {
        server=Number(values[0])!=0;player.playerKnown=Number(values[1]);player.battleOwner=player.templateOwner=Number(values[2]);mutate=Number(values[6]);clock=Number(values[7]);
        Put(players,180+0x2c,1);Put(players,180+0x50,Number(values[4]));
        if (const auto other=Number(values[5])) { Put(players,360+0x2c,2);Put(players,360+0x50,other);Put(players,360+0x60,88); }
        // 현재 옵션 원문을 생성하며 packet 기대값은 fixture에서만 읽는다.
        for (std::size_t index=0;index<options.size();++index) options[index]=static_cast<std::uint8_t>(index);
        PlayerParticipationHooks hooks;
        hooks.assignConnection=[this](std::int32_t pending,std::int32_t owner) { trace.push_back("Assign,"+std::to_string(pending)+","+std::to_string(owner)); };
        hooks.rejectLogin=[this](std::int32_t owner) { login->SendReply(owner,0); };
        hooks.disconnect=[this](std::int32_t owner) { trace.push_back("Disconnect,"+std::to_string(owner)); };
        participation=std::make_unique<RawPlayerParticipation>(OriginalEdition::Patch1078,players,allies,hidden,participationState,std::move(hooks));
        login=std::make_unique<RawMissionLogin>(players,*participation,Globals(),Hooks());
    }
    // 같은 장기 저장소를 복사하지 않고 실제 module에 빌려 준다.
    MissionLoginGlobals Globals() { return {server,boss,simulconnect,missionWorld,missionPost,metamap,player,mySubscriber,fortCount,lastPlayer,predictable,begin,seed,battleSeed,burst,clockStamp,baseClock,captureEnd,core,cached,zone,options}; }
    // 명시 외부 소비자만 관찰하며 원본 분기/flags/Ready 판단은 실제 모듈이 실행한다.
    MissionLoginHooks Hooks() {
        MissionLoginHooks hooks;
        hooks.disconnect=[this](std::int32_t owner) { trace.push_back("Disconnect,"+std::to_string(owner)); };
        hooks.send=[this](std::int32_t target,std::span<const std::uint8_t> bytes) {
            trace.push_back("S,"+std::to_string(target)+","+Hex(bytes));
            if (mutate==1 && !mutated && bytes[1]==0x29) { Put(players,360+0x2c,2);Put(players,360+0x50,0x143);mutated=true; }
        };
        hooks.broadcast=[this](std::span<const std::uint8_t> bytes) { trace.push_back("Broadcast,"+Hex(bytes)); };
        hooks.pendingReply=[this](PacketConnection&,std::span<const std::uint8_t> bytes) { trace.push_back("Pending,"+Hex(bytes)); };
        hooks.flush=[this] { trace.push_back("Flush"); };
        hooks.wallSeconds=[this] { trace.push_back("Wall");return 100.5; };
        hooks.clock=[this] { trace.push_back("Clock");return clock; };
        hooks.spBits=[this](std::uint32_t owner) { trace.push_back("SP,"+std::to_string(owner));return std::bit_cast<std::uint32_t>(static_cast<float>(owner)*1250.25F-1700.5F); };
        hooks.applyOptions=[this](std::span<const std::uint8_t> bytes) { trace.push_back("Options,"+Hex(bytes)); };
        hooks.clearTypes=[this] { trace.push_back("ClearTypes"); };
        hooks.sendPlayer=[this](std::uint32_t owner,std::uint32_t target) { trace.push_back("Player,"+std::to_string(owner)+","+std::to_string(target)); };
        hooks.sendFortMetadata=[this](std::uint32_t owner) { trace.push_back("Metadata,"+std::to_string(owner)); };
        hooks.loadCurrentFort=[this](std::uint32_t owner) { trace.push_back("FortName");trace.push_back("FortFile,"+std::to_string(owner)+",6d697373696f6e2e666f7274"); };
        hooks.loadFortImage=[this](std::uint32_t owner,std::span<const std::uint8_t> image) {
            trace.push_back("ImageName,"+std::to_string(owner)+",1");trace.push_back("FortOpen,"+std::to_string(owner)+",1");trace.push_back("FortImage,"+std::to_string(owner)+","+Hex(image));
        };
        hooks.peerAddress=[this](std::uint32_t owner) { trace.push_back("IP,"+std::to_string(owner));return "127.0.0.1"; };
        hooks.sendFort=[this](std::uint32_t owner,std::uint32_t target,std::uint32_t count) { trace.push_back("Fort,"+std::to_string(owner)+","+std::to_string(target)+","+std::to_string(count)); };
        hooks.synchronizePeer=[this](std::uint32_t owner) { trace.push_back("Synchronize,"+std::to_string(owner)); };
        hooks.loadWorld=[this] { trace.push_back("World");if (mutate==2) { player.battleOwner=3;player.templateOwner=4;Put(players,540+0x50,0x200); } };
        hooks.sendSettings=[this](std::uint32_t owner) { trace.push_back("Settings,"+std::to_string(owner)); };
        hooks.assertion=[this](std::string_view text) { trace.push_back("A,"+Hex(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(text.data()),text.size()))); };return hooks;
    }
    // 실제 반환 전역과 CoreData 선택 identity를 출력하며 기대 결과를 계산하지 않는다.
    std::string GlobalsText() const {
        const std::array values{player.battleOwner,player.templateOwner,player.playerKnown,fortCount,lastPlayer,predictable,begin,clockStamp,core.empty() ? 0U : static_cast<std::uint32_t>(core.data()-players.data()+1)};
        std::string result;
        // 원본 fixture가 정한 순서로 현재 결과를 직렬화한다.
        for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result;
    }
    // 전체 효과 순서를 하나의 TSV 셀 형식으로 묶는다.
    std::string Trace() const {
        std::string result;
        // 외부 효과의 실제 발생 순서대로 모든 항목을 남긴다.
        for (const auto& event:trace) { if (!result.empty()) result+=';';result+=event; }return result;
    }
};
// 실제 작성기의 requestsubscriber와 Fort image를 공급한다.
netstorm::zacket::LocalMissionPacket LoginPacket(std::uint32_t subscriber) {
    const std::array<std::uint8_t,5> image{1,3,5,7,9};return netstorm::zacket::MakeLocalMissionLogin(subscriber,"N",image);
}
}
// 독립 기계어 부모48입력과 전체 Player1620/9전역/모든 packet·효과가 일치해야 한다.
TEST_CASE(MissionLogin_Native48) {
    std::set<std::string> unique;std::size_t count=0;
    // 원본 기대 결과를 가공하지 않고 전체 입력을 동일 모듈에 실행한다.
    for (const auto& row:LoadFixture(NETSTORM_MISSION_LOGIN_FIXTURE).rows) {
        if (row[0]=="input") continue;CHECK(row.size()==4 && unique.insert(row[0]).second);const auto values=Split(row[0],',');LoginFixture fixture(values);PacketConnection connection;
        fixture.login->Receive(-1,connection,LoginPacket(Number(values[3])).Bytes());
        CHECK(Hex(fixture.players)==row[1]);CHECK(fixture.GlobalsText()==row[2]);CHECK(fixture.Trace()==row[3]);++count;
    }
    CHECK(count==48);
}
// 실제 소비자가 없으면 flags3 이후의 부분 효과는 보존하고 Ready/Begin/world 완료를 만들지 않는다.
TEST_CASE(MissionLogin_RequiredEffectsAndRegistration) {
    LoginFixture fixture(Split("1,0,8,1,8,0,0,7501",','));auto hooks=fixture.Hooks();hooks.sendFortMetadata={};RawMissionLogin missing(fixture.players,*fixture.participation,fixture.Globals(),std::move(hooks));PacketConnection connection;bool rejected=false;
    try { missing.Receive(-1,connection,LoginPacket(1).Bytes()); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && Get(fixture.players,180+0x50)==11 && fixture.fortCount==0 && fixture.begin==0);
    CHECK(std::find(fixture.trace.begin(),fixture.trace.end(),"World")==fixture.trace.end());
    PacketDispatchHooks dispatchHooks;dispatchHooks.now=[] { return 100U; };dispatchHooks.record=[](PacketConnection&,std::span<const std::uint8_t>,std::uint16_t) {};dispatchHooks.terminate=[](PacketConnection&) {};dispatchHooks.close=[](PacketConnection&) {};dispatchHooks.release=[](PacketConnection&) {};
    RawPacketDispatch dispatch(OriginalEdition::Patch1078,std::move(dispatchHooks));RegisterMissionLogin(dispatch,*fixture.login);CHECK(dispatch.Registration(0x27).version==1 && dispatch.Registration(0x27).mismatch==0 && dispatch.Validate(LoginPacket(1).Bytes()));
}
// 외부 모드의 미복원 분기를 성공 처리하지 않고 Ready broadcast는 실제 별도 소비자를 요청한다.
TEST_CASE(MissionLogin_SelectedModesAndReadyBroadcast) {
    LoginFixture fixture(Split("1,0,8,1,8,0,0,7501",','));PacketConnection connection;fixture.simulconnect=true;bool rejected=false;
    try { fixture.login->Receive(-1,connection,LoginPacket(1).Bytes()); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && Get(fixture.players,180+0x50)==8 && fixture.begin==0);
    fixture.simulconnect=false;fixture.missionWorld=false;rejected=false;
    try { fixture.login->Receive(-1,connection,LoginPacket(1).Bytes()); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    Put(fixture.players,180+0x50,0x40);Put(fixture.players,1440+0x50,0x40);fixture.login->SendReady(0);CHECK(fixture.trace.back()=="Broadcast,016e080002010000");
}
