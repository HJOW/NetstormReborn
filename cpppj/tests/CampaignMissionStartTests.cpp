// 보호8미션의같은Script/Session/SP/기술/사제/AI생성으로실제공통Start를검사한다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignMissionStartBindings.h"
#include "o/RawSunAviaryProcess.h"
#include "o/RawRainAviaryProcess.h"
#include <unordered_set>

using namespace netstorm::client;
namespace {
// 실제게임계산을대신하지않는장치/시간/설정외부입력과요청기록이다.
struct StartExternal {
    SoundList sounds{OriginalEdition::Patch1078}; // 실제원본음향이름표이며장치는열지않는다.
    std::unordered_set<SoundHandle> playing; // 재생계층외부입력의현재핸들이다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 원본번역키의명시영어입력이다.
    std::size_t refreshed{},commanded{},camera{},settings{},randomGeysers{},ui{}; // 실제선택된외부요청만관찰한다.
    // 실제월드설치에필요한장치/표시경계를공급하고그외게임규칙은Session몸체에맡긴다.
    RawCampaignSessionServices Services() {
        sounds.Initialize();RawCampaignSessionServices s;s.soundNames=&sounds;s.unixSeconds=[] { return 946684800; };
        s.sound.wallSeconds=[] { return 100.0; };
        s.sound.playLoopAt=[this](float,float,SoundHandle current,SoundHandle sound) { const auto token=current ? current : sound;playing.insert(token);return token; };
        s.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };
        s.sound.play=[this](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { playing.insert(sound);return sound; };
        s.sound.stop=[this](SoundHandle sound) { playing.erase(sound); };s.sound.isPlaying=[this](SoundHandle sound) { return playing.contains(sound) ? 1U : 0U; };
        s.priest.ownerNotice=[this](Sid) { ++ui; };s.priest.shield.noticeSound=[this](const PriestShieldNotice&) { ++ui; };
        s.priest.shield.translate=[](std::string_view key) { return std::string(key); };s.priest.shield.tell=[this](std::string_view) { ++ui; };
        s.ownership.refreshProduction=[this] { ++ui; };s.ownership.themeMode=[] { return 0U; };
        s.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };s.install.setConfig=[this](std::string_view,std::span<const std::uint8_t>) { ++ui; };
        s.install.refreshWindowTitle=[this] { ++ui; };s.install.resetCamera=[this] { ++ui; };
        s.assets.postPop.playImpactSound=[this](std::uint32_t) { ++ui; };s.assets.queueCommand=[this](std::uint32_t,std::int32_t,std::int32_t) { ++ui; };
        s.player.log=[this](std::uint32_t,std::string_view) { ++ui; };s.deletion.selected=[] { return Sid{}; };
        s.dependencyTransmit=[this](Sid,std::uint32_t) { ++ui; };s.playerContent.gump=[](std::uint32_t) { return 0U; };
        s.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xb3}); };
        s.ai.defaultName=[] { return std::string("Default AI"); };s.ai.localTransport=[] { return false; };
        s.ai.participation.clearChat=[this](std::uint32_t) { ++ui; };s.ai.participation.addChat=[this](std::uint32_t,std::string_view,std::uint32_t) { ++ui; };
        s.ai.participation.sendAlliance=[this](std::uint32_t,std::uint32_t,std::uint32_t) { ++ui; };
        s.ai.participation.assertion=[](PlayerParticipationAssertion) { throw std::logic_error("보호Start의예상하지않은원본참여진단"); };
        return s;
    }
    // 미초기화600BYTE 입력을 명시 패턴으로 공급하고 기존 실제 초기화 순서를 적용한다.
    CampaignMissionStartEffects Effects() {
        CampaignMissionStartEffects e;e.settingsBytes=[this](std::uint32_t,std::span<std::uint8_t> bytes) { ++settings;std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };
        e.gameSeconds=[] { return 123.5; };e.refreshSp=[this] { ++refreshed; };e.refreshCommandGump=[this] { ++commanded; };e.resetCamera=[this] { ++camera; };
        e.randomGeysers=[this] { ++randomGeysers;throw std::logic_error("보호8미션에없는세계랜덤간헐천요청"); };return e;
    }
};
// 실제 읽기 전용 VFS를 같은 Config/MissionScript에 연결한다.
void ConfigureStart(ConfigInterface& config) {
    config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");
}
// 보호미션의기대초기SP와기술목록이다.구현의출력으로기대값을계산하지않는다.
struct StartMissionInput { std::string_view name;float money;std::string_view technology; }; // 세입력은스크립트독립감사결과다.
constexpr std::array<StartMissionInput,8> kStartMissions{{
    {"Tutorial1",0,""},{"Tutorial2",10000,"sunArcher"},{"Tutorial3",2000,"sunArcher;sunWalker"},
    {"Tutorial4",2000,"sunArcher;sunWalker;windBattery"},{"Tutorial5",4000,"sunArcher;sunWalker;sunCannon;sunBlocker;windBattery"},
    {"Tutorial6",7500,"windBattery;windBalloon"},{"TheWarBegins",3000,"sunWalker;rainBattery;sunAviary;sunCannon"},
    {"MasterOfWhirligigs",2000,"sunWalker;windBattery;sunArcher;sunCannon"}}};
}
// 실제사제기준AI생성과같은SP/기술/시작표식이8미션에서정확히연결되어야한다.
TEST_CASE(CampaignMissionStart_ProtectedEightMissions) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    // 매미션은새Session/Script/원문상태를소유하며직전월드의현재값을재사용하지않는다.
    for (const auto& input:kStartMissions) {
        std::fprintf(stderr,"실제 미션 Start 검사: %s\n",input.name.data()); // 예외로 중단돼도 마지막 실제 입력 미션을 로그에서 식별한다.
        ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });ConfigureStart(config);MissionScript script(config,input.name);CHECK(script.Loaded());
        auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());StartExternal external;
        RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),external.Services());session.Load();
        std::fprintf(stderr,"실제 미션 Load 완료: %s\n",input.name.data()); // Start 이전 저장객체 Pop 실패와 시작 효과 실패를 구별한다.
        for (const auto& object:session.World().Objects()) if (object.object.type==72) { // 저장 썬 아비어리는 같은 실제 타입32 Form과 Kernel 객체를 소유해야 한다.
            const auto core=session.Core();const auto form=FindAttachedProcess(core.pool,object.sid,32);CHECK(form.value!=0);auto* process=form.value ? dynamic_cast<SunAviaryProcess*>(core.processes.FromForm(form)) : nullptr;
            CHECK(process && process->Parent()==object.sid && process->Fields().Form()==form && process->Fields().Stage()==1 && process->Fields().Dirty()==0);
        }
        std::size_t fences{}; // 저장된 실제 펜스의 최초 후처리와 같은 Kernel 사건을 확인한다.
        for (const auto& object:session.World().Objects()) if (object.object.type==136) { // 다른 타입을 펜스의 대체 객체로 만들지 않는다.
            ++fences;auto core=session.Core();const auto raw=core.pool.Slot(object.sid);const auto* regular=core.processes.FindEvent(object.sid,2);
            CHECK(raw[42]<=8 && raw[43]==0 && raw[44]==0 && raw[45]==0);
            CHECK(regular && regular->Parent()==object.sid && regular->Count()==0 && std::bit_cast<std::uint32_t>(regular->Payload())==0x3dcccccd);
        }
        if (input.name=="Tutorial6") CHECK(fences>0 && session.Core().bookkeeping.depth==0);
        if (input.name=="Tutorial5") {
            auto core=session.Core();std::size_t buried{}; // 보호 원문은 QA114의 buried 두 개를 저장한다.
            // 실제 풀에서만 대상을 찾고 별도 관찰용 객체/예약 목록을 만들지 않는다.
            for (std::uint32_t number=5;number<core.pool.Capacity();++number) {
                const Sid sid{static_cast<std::uint16_t>(number)};const auto raw=core.pool.Slot(sid);
                if (raw[10]!=112 || (raw[11]&1U)) continue;
                ++buried;CHECK(raw[12]==114);const auto* first=core.processes.FindEvent(sid,0);const auto* second=core.processes.FindEvent(sid,1);
                CHECK(first && second && first!=second);
                if (first && second) CHECK(first->Parent()==sid && second->Parent()==sid && first->Count()==0 && second->Count()==0 && first->Payload()==0 && std::bit_cast<std::uint32_t>(second->Payload())==0x3c23d70a);
            }
            CHECK(buried==2 && core.bookkeeping.depth==0);
        }
        MissionFrameGateState gate;MissionStartState state;MissionStartGlobals globals;MissionAiSettingsState ai;state.sequence=3;state.fieldC0=7;state.fieldC4=8;ai.stuffRefreshRate=4;
        RawCampaignMissionStartBindings start(session,script,gate,state,globals,ai,external.Effects());CHECK(&start.Gate()==&gate && gate.started==0 && session.AiMemory().Allocations().empty());
        std::array<bool,9> live{};std::size_t requested{}; // 실제 초기 사제 입력을 Start 효과 전에 보존한다.
        // live 조회 결과는 원본 별도 검증 몸체를 사용하며 Start 뒤의 삭제/AI 결과로 재계산하지 않는다.
        for (std::uint32_t owner=1;owner<9;++owner) if (owner!=session.LocalOwner()) { live[owner]=start.FindLivePriest(owner)!=0;if (live[owner]) ++requested; }
        start.Start();CHECK(gate.started==1 && gate.startSeconds==123.5 && state.sequence==4 && state.fieldC0==0 && state.fieldC4==0);
        CHECK(session.Sp(session.LocalOwner())==input.money && external.refreshed==1 && external.commanded==1 && external.camera==1 && external.randomGeysers==0);
        // 각각의실제기술ContentForm을같은현재Player체인에서조회한다.
        for (const auto& name:Split(input.technology,';')) if (!name.empty()) { const auto type=assets.game.TypeTable().Find(name);CHECK(type>0 && session.FindTechnology(static_cast<std::int32_t>(session.LocalOwner()),type).value!=0); }
        CHECK(external.settings==requested && session.Core().bookkeeping.depth==0);
        // 감사한 두 캠페인의 명시 적 사제 owner2는 실제 AI 생성/초기 갱신까지 도달해야 한다.
        if (input.name=="TheWarBegins" || input.name=="MasterOfWhirligigs") CHECK(live[2] && session.Participation().Brain(2)!=0);
    }
}
// 같은 현재 미션의 owner별 이름은 원본 좁은문자열로 쓰며 미등록 능력도 진단 뒤 계속 적용한다.
TEST_CASE(CampaignMissionStart_CurrentAiNameEncodingAndDiagnostics) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });ConfigureStart(config);
    MissionScript script(config,"TheWarBegins");script.Set("ai2Name","Fürst des Bösen");script.Set("ai2Ability","UNKNOWN_EXTRA_ABILITY");
    StartExternal external;auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),external.Services());session.Load();
    MissionFrameGateState gate;MissionStartState state;MissionStartGlobals globals;MissionAiSettingsState ai;ai.stuffRefreshRate=4;ai.checkingAsserts=true;
    auto effects=external.Effects();std::size_t assertions{},logs{}; // 원본의 서로 다른 두 진단을 순서대로 관찰한다.
    effects.unknownAbilityAssertion=[&](std::string_view name) { CHECK(name=="UNKNOWN_EXTRA_ABILITY" && logs==0);++assertions; };
    effects.unknownAbilityLog=[&](std::string_view name) { CHECK(name=="UNKNOWN_EXTRA_ABILITY" && assertions==1);++logs; };
    RawCampaignMissionStartBindings start(session,script,gate,state,globals,ai,std::move(effects));start.Start();
    CHECK(gate.started==1 && assertions==1 && logs==1 && session.Participation().Brain(2)!=0);
    const auto records=session.PlayerRecords();CHECK(std::string(reinterpret_cast<const char*>(records.data()+2*0xb4+12))==std::string("F\xfcrst des B\xf6sen"));
}
// 실제 UI 소비자가 실패하면 앞선 SP/단계 쓰기만 남고 AI 생성과 started 성공을 앞당기지 않는다.
TEST_CASE(CampaignMissionStart_RequiredEffectFailurePreservesOriginalPartialWrites) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });ConfigureStart(config);MissionScript script(config,"TheWarBegins");
    StartExternal external;auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),external.Services());session.Load();
    MissionFrameGateState gate;MissionStartState state;MissionStartGlobals globals;MissionAiSettingsState ai;state.sequence=3;ai.stuffRefreshRate=4;auto effects=external.Effects();
    effects.refreshSp=[] { throw std::logic_error("실제 SP 표시 소비자의 명시 실패 입력"); };
    RawCampaignMissionStartBindings start(session,script,gate,state,globals,ai,effects);CHECK(Throws([&] { start.Start(); }));
    CHECK(gate.started==0 && state.sequence==4 && session.Sp(session.LocalOwner())==3000 && external.settings==0 && session.AiMemory().Allocations().empty());
    effects.settingsBytes={};CHECK(Throws([&] { RawCampaignMissionStartBindings missing(session,script,gate,state,globals,ai,effects); }));
}

namespace {
// 실제 데모 미션의 같은 Session/Script/게이트를 시작한다. GUI 명령 분배 자체의 검사는 별도다.
void StartProtectedDemo(std::string_view name) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });ConfigureStart(config);
    MissionScript script(config,name);CHECK(script.Loaded());StartExternal external;auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());
    std::fprintf(stderr,"실제 데모 Load 검사: %s\n",std::string(name).c_str()); // 미복원 파생 몸체의 실제 진입 미션을 보존한다.
    RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),external.Services());session.Load();
    if (name=="DemoStormRages") {
        std::size_t pools{};auto core=session.Core(); // 실제 저장 맨 오 워 풀마다 타입60 Form과 같은 Kernel 객체가 있어야 한다.
        for (const auto& object:session.World().Objects()) if (object.object.type==143) { // 기술/관찰 전용 대체 객체를 새로 만들지 않는다.
            ++pools;const auto form=FindAttachedProcess(core.pool,object.sid,60);auto* process=form.value ? dynamic_cast<RainAviaryProcess*>(core.processes.FromForm(form)) : nullptr;
            CHECK(process && process->Parent()==object.sid && process->Fields().Parent()==object.sid && process->Fields().Form()==form && process->Fields().Stage()==1 && process->Fields().Dirty()==0);
        }
        CHECK(pools>0 && core.bookkeeping.depth==0);
    }
    MissionFrameGateState gate;MissionStartState state;MissionStartGlobals globals;MissionAiSettingsState ai;ai.stuffRefreshRate=4;
    // 원본 StartDemo는00463b70의 현재 미션 생성 뒤00463a00으로 실제+68에1을 쓴다. 스크립트 demo 키로 대체하지 않는다.
    gate.demo=1;RawCampaignMissionStartBindings start(session,script,gate,state,globals,ai,external.Effects());
    CHECK(start.FindLivePriest(session.LocalOwner())!=0);std::array<bool,9> live{};std::size_t requested{}; // Start 이전 실제 사제의 참여 입력이다.
    for (std::uint32_t owner=1;owner<9;++owner) { live[owner]=start.FindLivePriest(owner)!=0;if (live[owner]) ++requested; } // 데모는 로컬 소유자도 AI 생성에 포함한다.
    std::vector<std::pair<Sid,std::uint32_t>> coloredSupports; // 시작 전 실제4번 받침의 프레임 입력이며 기대 출력은 독립 색 red=2에서 정한다.
    if (name=="GuardMyBack") for (std::uint32_t number=2;number<session.Core().pool.Capacity();++number) { // 자연 지형97/98을 별도 받침94/95로 오인하지 않는다.
        const Sid sid{static_cast<std::uint16_t>(number)};const auto raw=session.Core().pool.Slot(sid);
        if (!(raw[11]&1U) && raw[34]==4 && (raw[10]==94 || raw[10]==95)) coloredSupports.emplace_back(sid,Get(raw,36));
    }
    std::fprintf(stderr,"실제 데모 Start 검사: %s\n",std::string(name).c_str());start.Start();
    CHECK(gate.demo==1 && gate.started==1 && gate.startSeconds==123.5 && gate.demoEndSeconds==483.5);
    CHECK(external.settings==requested && session.Participation().Brain(session.LocalOwner())!=0 && session.Core().bookkeeping.depth==0);
    for (std::uint32_t owner=1;owner<9;++owner) if (live[owner]) CHECK(session.Participation().Brain(owner)!=0); // 같은 실제 사제 소유자는 모두 현재 AI에 연결되어야 한다.
    if (name=="GuardMyBack") {
        constexpr std::array<std::int32_t,9> expected{0,1,5,8,2,5,6,7,8}; // 원본 ai2Color=purple/ai3=orange/ai4=red와 나머지 초기 색이다.
        auto core=session.Core();CHECK(core.bridgeState.ownerColors==expected);std::array<std::size_t,9> islands{}; // 실제 받침/종유석까지 같은 표를 소비했는지 센다.
        for (std::uint32_t number=2;number<core.pool.Capacity();++number) { // 별도 화면 snapshot 대신 현재 raw 풀의 모든 유효 대상만 읽는다.
            const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(number)});if ((raw[11]&1U) || (raw[10]!=94 && raw[10]!=95)) continue;
            const auto owner=raw[34];CHECK(owner<9);if (owner>=9) continue;++islands[owner];
            CHECK(std::bit_cast<std::int32_t>(netstorm::test::rawscene::Get(raw,36))==(owner ? expected[owner]-1 : 8));
        }
        CHECK(islands[4]>0 && !coloredSupports.empty());std::size_t changed{}; // 원본 Fort의4번 별도 받침에서 실제 프레임 변화가 있어야 한다.
        for (const auto& [sid,before]:coloredSupports) { const auto after=Get(core.pool.Slot(sid),36);CHECK(after==1);if (before!=after) ++changed; } // red=2의 기대 프레임1과 시작 전 입력을 함께 확인한다.
        CHECK(changed>0);
    }
}
}
// 원본 DEMOVILLE의 세 StartDemo 입력은 독립 검사로 실행해 첫 실패가 뒤 미션을 감추지 않게 한다.
TEST_CASE(CampaignDemoStart_TheStormRages) { StartProtectedDemo("DemoStormRages"); }
// 실제 ai1과 동맹/색 설정이 있는 두 번째 원본 데모다.
TEST_CASE(CampaignDemoStart_GuardMyBack) { StartProtectedDemo("GuardMyBack"); }
// 세 소유자와 배신 동맹 설정이 있는 세 번째 원본 데모다.
TEST_CASE(CampaignDemoStart_DissolvedAlliance) { StartProtectedDemo("DissolvedAlliance"); }
