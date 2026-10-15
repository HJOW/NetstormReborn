// 실제8종원본157입력의현재Player/동맹/SP/전역/외부효과를전수대조한다.
#include "RawSceneSupport.h"
#include "o/RawMissionClientPackets.h"
#include "o/RawMissionLogin.h"
#include <set>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 독립fixture가보존한입력바이트를그대로복원한다.
std::vector<std::uint8_t> ClientPacketBytes(std::string_view text) {
    std::vector<std::uint8_t> result;
    // 두hex글자를한원본BYTE로읽는다.
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));return result;
}
// SP하위의논리쓰기호출은전체9SP바이트로대조하고나머지외부효과는모두순서대로대조한다.
std::string ClientEffectTrace(std::string_view text) {
    std::string result;
    // 실제ScrambledSpStore 직접쓰기에는가짜관찰callback을추가하지않는다.
    for (const auto& effect:Split(text,';')) if (!effect.starts_with("Set,")) { if (!result.empty()) result+=';';result+=effect; }return result.empty() ? "-" : result;
}
// 모든수신기와실제참여/Ready송신/SP가공유하는현재저장소다.
struct ClientPacketFixture {
    std::vector<std::uint8_t> players=std::vector<std::uint8_t>(1620),options=std::vector<std::uint8_t>(87,0xa5); // 현재원문배열이다.
    std::array<std::uint32_t,81> allies{}; // 현재방향별81DWORD다.
    std::span<std::uint8_t> hidden=std::span<std::uint8_t>(options).subspan(0x3e,9); // 실제옵션87안에존재하는같은숨김BYTE9다.
    std::array<std::uint8_t,6> zone{0x11,0x12,0x13,0x14,0x15,0x16}; // 현재주소6바이트다.
    std::array<std::uint8_t,10> cached{}; // 현재cachedCoreData원문이다.
    std::array<std::uint8_t,8> handle{'M','y',' ','N','a','m','e',0}; // 현재자기이름원문이다.
    std::span<std::uint8_t> core; // 실제CoreData선택포인터다.
    PlayerWorldState player; // 같은현재두owner/known이다.
    PlayerParticipationState participationState; // 같은실제참여모듈의기존상태다.
    bool server{},boss{},mission{},saved{true},metamap{},battle{true},simulconnect{},mutated{}; // 현재모드와명시관찰표식이다.
    std::uint32_t editor{},outpost{},seed{7},battleSeed{8},revision{9},option{10},begin{},clockStamp{11},baseClock{12},phase{},current{13},aux{14},pending{15},ticks{16},coreActive{17}; // 현재원본전역이다.
    std::uint32_t variant{},mutate{},type{122},energy{},subscriber{1},fortCount{},lastPlayer{},predictable{},burst{}; // 입력과실제Readyhelper가빌리는나머지전역이다.
    double capture{77.25},loginEnd{30.0}; // 실제벽시각기반double전역이다.
    GameRandom random{42}; // 실제SP저장소가소비하는동일난수다.
    ScrambledSpStore sp{random,[] { return 9U; }}; // 가짜float배열대신실제분산SP를사용한다.
    std::unique_ptr<RawPlayerParticipation> participation; // 같은Player9/동맹을직접수정한다.
    std::unique_ptr<RawMissionLogin> login; // Readymask송신의이미복원한실제몸체다.
    std::unique_ptr<RawMissionClientPackets> packets; // 현재8종수신처리기다.
    std::vector<std::uint8_t> packet; // callback변경도다음원본조회에서보이는동일수신입력이다.
    std::vector<std::string> trace; // 실제외부효과전체순서다.
    // 독립원본관찰과동일한입력저장소/실제모듈을구성한다.
    ClientPacketFixture(const std::vector<std::string>& values,std::vector<std::uint8_t> input):packet(std::move(input)) {
        server=Number(values[1])!=0;boss=Number(values[2])!=0;mission=Number(values[3])!=0;player.playerKnown=Number(values[4]);player.battleOwner=player.templateOwner=Number(values[5]);variant=Number(values[6]);mutate=Number(values[7]);editor=outpost=variant&1U;phase=std::array<std::uint32_t,3>{10,16,9}[(variant>>1)%3];loginEnd=(variant&1U) ? 0.0 : 30.0;cached.fill(0x55);core=std::span<std::uint8_t>(players).subspan(180+0x6c,10);sp.Initialize();
        // 입력값만명시하며원본결과를계산하지않는다.
        for (std::uint32_t owner=0;owner<9;++owner) {
            const auto name=std::string("N")+std::to_string(owner);std::copy(name.begin(),name.end(),players.begin()+owner*180+12);Put(players,owner*180+0x50,0x200);Put(players,owner*180+0x54,1U<<owner);allies[owner*9+owner]=1;sp.Set(owner,std::bit_cast<std::uint32_t>(900.0F+static_cast<float>(owner)));
        }
        PlayerParticipationHooks hooks;
        hooks.clearChat=[this](std::uint32_t flags) { trace.push_back("Clear,"+std::to_string(flags)); };
        hooks.addChat=[this](std::uint32_t sub,std::string_view name,std::uint32_t flags) { Chat(sub,name,flags); };
        hooks.assertion=[](PlayerParticipationAssertion) { CHECK(false); };
        participation=std::make_unique<RawPlayerParticipation>(OriginalEdition::Patch1078,players,allies,hidden,participationState,std::move(hooks));
        participation->BindCurrentInputs({[this] { return player.battleOwner; },[this] { return player.playerKnown; },[this] { return battle; },[this] { return metamap; },[this] { return simulconnect; }});
        MissionLoginHooks loginHooks;loginHooks.send=[this](std::int32_t target,std::span<const std::uint8_t> bytes) { Send(target,bytes); };loginHooks.broadcast=[this](std::span<const std::uint8_t> bytes) { Send(0,bytes); };loginHooks.assertion=[this](std::string_view text) { Assert(text); };
        login=std::make_unique<RawMissionLogin>(players,*participation,MissionLoginGlobals{server,boss,simulconnect,mission,mission,metamap,player,subscriber,fortCount,lastPlayer,predictable,begin,seed,battleSeed,burst,clockStamp,baseClock,capture,core,cached,zone,options},std::move(loginHooks));
        packets=std::make_unique<RawMissionClientPackets>(players,*participation,sp,State(),Hooks());
    }
    // 현재원본참조를정확히같은수명으로빌려준다.
    MissionClientPacketState State() { return {server,boss,mission,saved,metamap,battle,editor,outpost,player,seed,battleSeed,revision,option,begin,clockStamp,baseClock,phase,current,aux,pending,ticks,coreActive,capture,loginEnd,core,cached,zone,options,handle,hidden}; }
    // 실제외부진단을원본좁은문자열바이트로기록한다.
    void Assert(std::string_view text) { trace.push_back("Assert,"+Hex(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(text.data()),text.size()))); }
    // 실제채널요청과명시현재서버모드변경을기록한다.
    void Chat(std::uint32_t sub,std::string_view name,std::uint32_t flags) {
        trace.push_back("Chat,"+std::to_string(sub)+","+Hex(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(name.data()),name.size()))+","+std::to_string(flags));if (mutate==2 && !mutated) { server=false;mutated=true; }
    }
    // 실제송신packet을그대로관찰하며다음원본타이머조회변경을공급한다.
    void Send(std::int32_t target,std::span<const std::uint8_t> bytes) { trace.push_back("Send,"+std::to_string(target)+","+Hex(bytes));if (mutate==4) loginEnd=0.0; }
    // 미복원큰소비자의인자/호출순서만명시외부입력으로관찰한다.
    MissionClientPacketHooks Hooks() {
        MissionClientPacketHooks hooks;
        hooks.assertion=[this](std::string_view text) { Assert(text); };
        hooks.configInt=[this](std::string_view key) { trace.push_back("Config,"+std::string(key));return variant&32U ? 1 : 0; };
        hooks.tell=[this](std::string_view text) { trace.push_back("Tell,"+Hex(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(text.data()),text.size()))); };
        hooks.runState=[this] { trace.push_back("State,"+std::to_string(current)); };hooks.resetCamera=[this] { trace.push_back("Camera"); };
        hooks.refreshSp=[this] { trace.push_back("Refresh");if (mutate==3) Put(packet,4,2); };
        hooks.wallSeconds=[this] { trace.push_back("Wall");if (mutate==1) phase=16;return 100.5; };hooks.clock=[this] { trace.push_back("Clock");return 7501U; };
        hooks.outpostOptions=[this](std::uint32_t sender,std::span<const std::uint8_t>) { trace.push_back("Outpost,"+std::to_string(sender)); };
        hooks.updateRange=[this] { trace.push_back("Range"); };hooks.requiredEnergy=[this] { trace.push_back("Energy");return 1251; };
        hooks.setEnergyType=[this](float value) { trace.push_back("Type,"+std::to_string(type));energy=std::bit_cast<std::uint32_t>(value); };hooks.readOptionValue=[this] { trace.push_back("Option");return 17U; };
        hooks.reconnectUi=[this] { trace.push_back("ReconnectUI"); };
        hooks.openFortImage=[this](std::uint32_t owner) { trace.push_back("ImageName,"+std::to_string(owner));trace.push_back("Open,"+std::to_string(owner)); };
        hooks.readFortImage=[this](std::uint32_t owner,std::span<const std::uint8_t> image) { trace.push_back("Image,"+std::to_string(owner)+","+Hex(image)); };
        hooks.buildFort=[this](std::uint32_t owner) { trace.push_back("Builder");trace.push_back("Attach,"+std::to_string(owner));trace.push_back("Build"); };
        hooks.peerAddress=[this](std::uint32_t owner) { trace.push_back("IP,"+std::to_string(owner));return "127.0.0.1"; };
        hooks.addChat=[this](std::uint32_t sub,std::string_view name,std::uint32_t flags) { Chat(sub,name,flags); };
        hooks.removeChat=[this](std::uint32_t sub,std::uint32_t flags) { trace.push_back("Remove,"+std::to_string(sub)+","+std::to_string(flags)); };
        hooks.restoreCoreData=[this](std::uint32_t owner) { trace.push_back("Restore,"+std::to_string(owner)); };
        hooks.playerUiBusy=[this] { trace.push_back("Busy");return (variant&64U)!=0; };hooks.saveReconnect=[this] { trace.push_back("Save"); };
        hooks.playerNotice=[this](std::uint32_t kind,std::uint32_t owner,std::uint32_t argument) { trace.push_back("Notice,"+std::to_string(kind)+","+std::to_string(owner)+","+std::to_string(argument)); };
        hooks.send=[this](std::int32_t target,std::span<const std::uint8_t> bytes) { Send(target,bytes); };hooks.sendReady=[this](std::uint32_t target) { login->SendReady(target); };return hooks;
    }
    // 원본현재전역과double비트를정해진순서로읽어출력한다.
    std::string StateText() const {
        const auto identity=core.empty() ? 0U : core.data()==cached.data() ? 0xfffffffeU : static_cast<std::uint32_t>(core.data()-players.data()+1);
        const std::array<std::uint64_t,20> values{player.battleOwner,player.templateOwner,player.playerKnown,seed,battleSeed,revision,option,begin,clockStamp,baseClock,phase,current,aux,pending,ticks,coreActive,server ? 1U : 0U,identity,std::bit_cast<std::uint64_t>(capture),std::bit_cast<std::uint64_t>(loginEnd)};std::string result;
        // 각전역은기대모델없이현재저장값을그대로읽는다.
        for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result;
    }
    // 옵션/주소/cache/실제TYPE쓰기의전체바이트를직렬화한다.
    std::string StorageText() const { std::array<std::uint8_t,4> raw{};Put(raw,0,energy);return Hex(zone)+Hex(options)+Hex(cached)+Hex(raw); }
    // 같은실제분산SP저장소의9소유자현재비트를읽는다.
    std::string SpText() {
        std::array<std::uint8_t,36> raw{};
        // 실제원본과같은0..8소유자각각의현재float비트를읽는다.
        for (std::uint32_t owner=0;owner<9;++owner) Put(raw,owner*4,sp.Get(owner));return Hex(raw);
    }
    // 현재방향별81DWORD를원본little endian바이트로직렬화한다.
    std::string AlliesText() const {
        std::array<std::uint8_t,324> raw{};
        // 두방향을합치지않고81DWORD모두원문순서로출력한다.
        for (std::size_t index=0;index<allies.size();++index) Put(raw,index*4,allies[index]);return Hex(raw);
    }
    // 실제발생순서의전체외부효과문자열이다.
    std::string TraceText() const {
        std::string result;
        // callback이실제로발생한순서만남긴다.
        for (const auto& event:trace) { if (!result.empty()) result+=';';result+=event; }return result.empty() ? "-" : result;
    }
};
}
// 원본157정상반환입력의전체현재쓰기를다른사본없이대조한다.
TEST_CASE(MissionClientPackets_Native157) {
    std::set<std::string> unique;std::size_t count=0;
    // 모든기대값은독립기계어fixture의원문전체출력이다.
    for (const auto& row:LoadFixture(NETSTORM_MISSION_CLIENT_PACKETS_FIXTURE).rows) {
        if (row[0]=="input") continue;CHECK(row.size()==8 && unique.insert(row[0]).second);const auto values=Split(row[0],',');ClientPacketFixture fixture(values,ClientPacketBytes(row[1]));PacketConnection connection;
        fixture.packets->Receive(Number(values[8]),connection,fixture.packet);
        if (fixture.StateText()!=row[4]) std::fprintf(stderr,"수신 현재전역 불일치 입력=%s\n실제=%s\n기대=%s\n",row[0].c_str(),fixture.StateText().c_str(),row[4].c_str());
        if (fixture.TraceText()!=ClientEffectTrace(row[7])) std::fprintf(stderr,"수신 효과 불일치 입력=%s\n실제=%s\n기대=%s\n",row[0].c_str(),fixture.TraceText().c_str(),ClientEffectTrace(row[7]).c_str());
        CHECK(Hex(fixture.players)==row[2]);CHECK(fixture.AlliesText()==row[3]);CHECK(fixture.StateText()==row[4]);CHECK(fixture.StorageText()==row[5]);CHECK(fixture.SpText()==row[6]);CHECK(fixture.TraceText()==ClientEffectTrace(row[7]));++count;
    }
    CHECK(count==157);
}
// 미등록소비자와잘린입력을수신완료로바꾸지않고앞서쓴옵션은보존한다.
TEST_CASE(MissionClientPackets_RequiredEffectsAndRegistration) {
    auto packet=ClientPacketBytes("0167080000010077");ClientPacketFixture fixture(Split("103,1,1,1,1,1,0,0,9",','),packet);auto hooks=fixture.Hooks();hooks.updateRange={};RawMissionClientPackets missing(fixture.players,*fixture.participation,fixture.sp,fixture.State(),std::move(hooks));PacketConnection connection;bool rejected=false;
    try { missing.Receive(9,connection,packet); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && fixture.options[0]==0x77 && fixture.option==10);
    PacketDispatchHooks dispatchHooks;dispatchHooks.now=[] { return 1U; };dispatchHooks.record=[](PacketConnection&,std::span<const std::uint8_t>,std::uint16_t) {};dispatchHooks.terminate=[](PacketConnection&) {};dispatchHooks.close=[](PacketConnection&) {};dispatchHooks.release=[](PacketConnection&) {};
    RawPacketDispatch dispatch(OriginalEdition::Patch1078,std::move(dispatchHooks));RegisterMissionClientPackets(dispatch,*fixture.packets);
    //8종모두실제수신기가있으며수신observer의0인자는원본처럼자연반환한다.
    for (const auto type:std::array<std::uint32_t,8>{0x29,0x67,0x2b,0x2a,0x28,0x81,0x84,0x6e}) { const auto& entry=dispatch.Registration(type);CHECK(entry.version==1 && entry.mismatch==0 && entry.receive && entry.observer);entry.observer(0); }
    rejected=false;try { missing.Receive(9,connection,std::array<std::uint8_t,4>{1,0x29,34,0}); } catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);
}
