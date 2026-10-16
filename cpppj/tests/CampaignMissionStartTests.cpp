// 보호8미션의같은Script/Session/SP/기술/사제/AI생성으로실제공통Start를검사한다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignMissionStartBindings.h"
#include "o/RawSunAviaryProcess.h"
#include "o/RawRainAviaryProcess.h"
#include "o/RawFlyerProcess.h"
#include <unordered_set>
#include <map>

using namespace netstorm::client;
namespace {
// 실제게임계산을대신하지않는장치/시간/설정외부입력과요청기록이다.
struct StartExternal {
    SoundList sounds{OriginalEdition::Patch1078}; // 실제원본음향이름표이며장치는열지않는다.
    std::unordered_set<SoundHandle> playing; // 재생계층외부입력의현재핸들이다.
    std::vector<SoundHandle> once; // 자연 발사에서 실제 전달된 한 번 재생 토큰을 보존한다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 원본번역키의명시영어입력이다.
    std::size_t refreshed{},commanded{},camera{},settings{},randomGeysers{},ui{}; // 실제선택된외부요청만관찰한다.
    // 실제월드설치에필요한장치/표시경계를공급하고그외게임규칙은Session몸체에맡긴다.
    RawCampaignSessionServices Services() {
        sounds.Initialize();RawCampaignSessionServices s;s.soundNames=&sounds;s.unixSeconds=[] { return 946684800; };
        s.sound.wallSeconds=[] { return 100.0; };
        s.sound.playLoopAt=[this](float,float,SoundHandle current,SoundHandle sound) { const auto token=current ? current : sound;playing.insert(token);return token; };
        s.sound.playOnceAt=[this](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { once.push_back(sound);return sound; };
        s.sound.play=[this](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { playing.insert(sound);return sound; };
        s.sound.stop=[this](SoundHandle sound) { playing.erase(sound); };s.sound.isPlaying=[this](SoundHandle sound) { return playing.contains(sound) ? 1U : 0U; };
        s.priest.ownerNotice=[this](Sid) { ++ui; };s.priest.shield.noticeSound=[this](const PriestShieldNotice&) { ++ui; };
        s.priest.shield.translate=[](std::string_view key) { return std::string(key); };s.priest.shield.tell=[this](std::string_view) { ++ui; };
        s.ownership.refreshProduction=[this] { ++ui; };s.ownership.themeMode=[] { return 0U; };
        s.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };s.install.setConfig=[this](std::string_view,std::span<const std::uint8_t>) { ++ui; };
        s.install.refreshWindowTitle=[this] { ++ui; };s.install.resetCamera=[this] { ++ui; };
        s.assets.postPop.playImpactSound=[this](std::uint32_t) { ++ui; };s.assets.queueCommand=[this](std::uint32_t,std::int32_t,std::int32_t) { ++ui; };
        s.hit.general.warningEvent=[this] { ++ui; }; // 실제event12 경고 표시의 외부 요청만 관찰한다.
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
        for (const auto& object:session.World().Objects()) if (object.object.type==72) { // 저장 훨리베이스는 같은 실제 타입32 Form과 Kernel 객체를 소유해야 한다.
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

namespace {
// 원본 자원을 바꾸지 않고 실제 Start 뒤 같은 Session의 Kernel을10초 진행한다. Guide/GUI 경로 검사는 별도다.
void RunProtectedKernelFrames(std::string_view name,bool demo) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });ConfigureStart(config);MissionScript script(config,name);
    StartExternal external;auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),external.Services());session.Load();
    GameClock clock;MissionFrameGateState gate;MissionStartState state;MissionStartGlobals globals;MissionAiSettingsState ai;gate.demo=demo ? 1U : 0U;ai.stuffRefreshRate=4;
    auto effects=external.Effects();effects.gameSeconds=[&] { return clock.GameSeconds(0); }; // 같은 세션의 초기 시각0을 실제 Start에 공급한다.
    RawCampaignMissionStartBindings start(session,script,gate,state,globals,ai,std::move(effects));start.Start();CHECK(gate.started==1);
    for (std::uint32_t frame=1;frame<=400;++frame) { //25ms는 명시 외부 시계 입력이며 원본 고정 FPS를 뜻하지 않는다.
        const auto time=clock.Capture(frame*25);
        try { session.RunFrame(time,false); }
        catch (const std::exception& error) { throw std::runtime_error(std::string(name)+" Kernel frame"+std::to_string(frame)+" game"+std::to_string(time.game)+": "+error.what()); }
    }
    if (name=="Tutorial6" || name=="GuardMyBack") {
        const auto core=session.Core();std::size_t markers{},shields{}; //현재 실제로 Pop된 두 연결 객체의 불변식을 검사한다.
        for (std::uint32_t number=5;number<core.pool.Capacity();++number) { //별도 관찰 객체를 만들지 않고 현재 전체 풀만 읽는다.
            const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(number)});if (raw[11]&5U) continue;
            if (raw[10]==90) {
                ++markers;const auto first=Get(raw,12,2),second=(Get(raw,26)<<8)|raw[30]; //원본00456460의 두 독립 끝점 태그다.
                CHECK(first>=5 && first<core.pool.Capacity() && second>=5 && second<core.pool.Capacity());
                if (first>=5 && first<core.pool.Capacity() && second>=5 && second<core.pool.Capacity()) {
                    CHECK(core.pool.Slot(Sid{static_cast<std::uint16_t>(first)})[10]==136 && core.pool.Slot(Sid{static_cast<std::uint16_t>(second)})[10]==136);
                }
            } else if (raw[10]==170) {
                ++shields;const auto frame=Get(raw,36);const auto codes=core.frames[170].Codes();CHECK(frame<codes.size());
                if (frame<codes.size()) CHECK(codes[frame].side=='J' || codes[frame].side=='K'); //원본mode1의 현재 실제 J/K 프레임이어야 한다.
            }
        }
        std::fprintf(stderr,"현재 펜스 연결 객체 %s: 표식%zu 포스필드%zu\n",std::string(name).c_str(),markers,shields); //단순 프레임 정상 반환과 실제 생성 상태를 구별한다.
    }
    CHECK(session.Core().bookkeeping.depth==0 && gate.started==1);std::fprintf(stderr,"실제 Kernel400프레임 완료: %s\n",std::string(name).c_str()); // 전체 정상 완료와 첫 예외 위치를 구별한다.
}
}
// 각각 독립 실행해 첫 미복원 경계가 다른 목표 미션을 감추지 않게 한다.
TEST_CASE(CampaignKernelFrames_Tutorial1) { RunProtectedKernelFrames("Tutorial1",false); }
// Early Missions2의 실제 예약/내용물/사제 프레임이다.
TEST_CASE(CampaignKernelFrames_Tutorial2) { RunProtectedKernelFrames("Tutorial2",false); }
// Early Missions3의 실제 월드/프로세스 프레임이다.
TEST_CASE(CampaignKernelFrames_Tutorial3) { RunProtectedKernelFrames("Tutorial3",false); }
// Early Missions4의 같은 원본 배치다.
TEST_CASE(CampaignKernelFrames_Tutorial4) { RunProtectedKernelFrames("Tutorial4",false); }
// Early Missions5의 매립/예약/총 프레임이다.
TEST_CASE(CampaignKernelFrames_Tutorial5) { RunProtectedKernelFrames("Tutorial5",false); }
// Early Missions6의 실제 썬 바리케이드 사건도 실행한다.
TEST_CASE(CampaignKernelFrames_Tutorial6) { RunProtectedKernelFrames("Tutorial6",false); }
// 캠페인1-1의 같은 AI 생성 뒤 실제 프로세스다.
TEST_CASE(CampaignKernelFrames_TheWarBegins) { RunProtectedKernelFrames("TheWarBegins",false); }
// 캠페인1-2의 실제 훨리베이스/총 등 현재 배치다.
TEST_CASE(CampaignKernelFrames_MasterOfWhirligigs) { RunProtectedKernelFrames("MasterOfWhirligigs",false); }
// 데모1의 로컬AI/맨 오 워 풀 등 실제 현재 프레임이다.
TEST_CASE(CampaignKernelFrames_TheStormRages) { RunProtectedKernelFrames("DemoStormRages",true); }
// 데모2의 네 참여자 배치와 실제 색 변경 뒤 상태다.
TEST_CASE(CampaignKernelFrames_GuardMyBack) { RunProtectedKernelFrames("GuardMyBack",true); }
// 데모3의 실제 현재 동맹/프로세스다.
TEST_CASE(CampaignKernelFrames_DissolvedAlliance) { RunProtectedKernelFrames("DissolvedAlliance",true); }

// 실제 데모에서 자연 발사된 썬 캐논의 같은 Form/Kernel/표시를 실제 픽셀까지 연결한다.
TEST_CASE(CampaignParticles_DissolvedAllianceNaturalShot) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });ConfigureStart(config);MissionScript script(config,"DissolvedAlliance");
    StartExternal external;Renderer renderer(128,128);auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),external.Services());session.Load();
    GameClock clock;MissionFrameGateState gate;MissionStartState state;MissionStartGlobals globals;MissionAiSettingsState ai;gate.demo=1;ai.stuffRefreshRate=4;
    auto effects=external.Effects();effects.gameSeconds=[&] { return clock.GameSeconds(0); };RawCampaignMissionStartBindings start(session,script,gate,state,globals,ai,std::move(effects));start.Start();
    // 총/표적/위치/HP/난수를 고치지 않고 최초 자연 발사와 다음 실제 시간 조각까지 진행한다.
    for (std::uint32_t frame=1;frame<=115;++frame) session.RunFrame(clock.Capture(frame*25),false);
    const auto core=session.Core();const auto& rendering=session.Particles().Rendering();CHECK(rendering.Size()>0);
    const RenderProcessFields* fields=nullptr;
    // 실제 단일 표시 목록의 현재 첫 유효 효과만 선택하며 별도 효과를 생성하지 않는다.
    for (const auto token:rendering.Slots()) if (const auto* current=rendering.Lookup(token)) { fields=current;break; }
    CHECK(fields!=nullptr);if (!fields) return;
    const Sid parent{static_cast<std::uint16_t>(fields->parent)};const auto raw=core.pool.Slot(parent);CHECK(raw[10]==80);
    auto* fiery=dynamic_cast<FieryTailVisualProcess*>(core.processes.FromForm(Sid{static_cast<std::uint16_t>(fields->form)}));
    auto* missile=core.processes.FromForm(FindAttachedProcess(core.pool,parent,28));CHECK(fiery && missile);
    if (!fiery || !missile) return;
    CHECK(fiery->Fields().first && fiery->Fields().total>=10 && core.kernel.Find(*fiery)<core.kernel.Find(*missile));
    const auto x=std::bit_cast<float>(Get(raw,14)),y=std::bit_cast<float>(Get(raw,18));const auto cameraX=static_cast<std::int32_t>(x*16)-64,cameraY=static_cast<std::int32_t>(y*11)-64;
    session.SetParticleView({cameraX,cameraY,0,0,128,128},{[&] { return renderer.FullRedrawPending(); },[&](SquidDisplayRect rect,std::uint32_t flags) { renderer.Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }});
    auto sprites=session.ParticleSprites({0,0,128,128});CHECK(!sprites.empty());renderer.SetScene(std::move(sprites));const auto image=renderer.SceneImage();
    CHECK(std::count(image.indices.begin(),image.indices.end(),196)>3 && core.bookkeeping.depth==0);
    const auto token=fields->token; // 부모80의 자연 삭제 뒤에도 역참조하지 않을 원래 표시 토큰이다.
    // 최초 썬 미사일이 자연 종료하는159프레임까지 같은 Kernel/종속 삭제를 실행한다.
    for (std::uint32_t frame=116;frame<=159;++frame) session.RunFrame(clock.Capture(frame*25),false);
    CHECK(rendering.Lookup(token)==nullptr && core.bookkeeping.depth==0);
}

// 같은 보호 데모의 최초 썬더 발사가 실제132 자산/28 Form과 같은 Kernel을 사용해야 한다.
TEST_CASE(CampaignThunderMissile_DissolvedAllianceNaturalShot) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });ConfigureStart(config);MissionScript script(config,"DissolvedAlliance");
    StartExternal external;auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),external.Services());session.Load();
    GameClock clock;MissionFrameGateState gate;MissionStartState state;MissionStartGlobals globals;MissionAiSettingsState ai;gate.demo=1;ai.stuffRefreshRate=4;
    auto effects=external.Effects();effects.gameSeconds=[&] { return clock.GameSeconds(0); };RawCampaignMissionStartBindings start(session,script,gate,state,globals,ai,std::move(effects));start.Start();
    // 실제 총/표적/시각/HP/난수를 고치지 않고 기존145프레임의 자연 생성 경계를 실행한다.
    for (std::uint32_t frame=1;frame<=145;++frame) session.RunFrame(clock.Capture(frame*25),false);
    const auto core=session.Core();std::size_t found=0;std::vector<std::pair<ProcessId,const BaseProcess*>> tracked; // 원래 프로세스의 삭제 여부만 주소 비교로 확인한다.
    // 관찰용 투사체를 만들지 않고 현재 실제 Kernel에서 원래 Missile만 찾는다.
    for (ProcessId id=1;id<=core.kernel.Capacity();++id) {
        const auto* process=dynamic_cast<const Missile*>(core.kernel.Get(id));if (!process) continue;const auto& fields=process->Fields();
        if (core.pool.Slot(fields.Firing())[10]!=131) continue;
        const auto display=core.pool.Slot(fields.Parent());CHECK(display[10]==132 && Get(display,0)==0x50b570 && Get(display,36)<12);
        CHECK(core.pool.Slot(fields.Form())[10]==28 && core.processes.FromForm(fields.Form())==process);
        CHECK(FindAttachedProcess(core.pool,fields.Parent(),11).value==0);tracked.emplace_back(id,process);++found;
    }
    CHECK(found>0 && core.bookkeeping.depth==0 && gate.started==1);
    const auto fire=external.sounds.Lookup("thunderCannon.wav");CHECK(fire!=0 && std::find(external.once.begin(),external.once.end(),fire)!=external.once.end());
    bool removed=false; // 선행 썬 미사일의159프레임 종료와 썬더 자신의 자연 삭제를 혼동하지 않는다.
    // 실제 썬더가 원래 Kernel 슬롯에서 삭제될 때까지 진행하며 전체400프레임 안에서 종료해야 한다.
    for (std::uint32_t frame=146;frame<=400 && !removed;++frame) {
        session.RunFrame(clock.Capture(frame*25),false);
        removed=std::any_of(tracked.begin(),tracked.end(),[&](const auto& item) { return core.kernel.Get(item.first)!=item.second; });
        if (removed) std::fprintf(stderr,"실제 썬더 투사체 자연 삭제: frame%u game%.6f\n",frame,clock.GameSeconds(frame*25));
    }
    CHECK(removed && core.bookkeeping.depth==0);
}

// 실제 데모 배치의 생산 건물/동일 factory/공간/Host로 최초 비행 Pop만 분리해 검사한다. Frame 완주 검사와 구별한다.
TEST_CASE(CampaignManOWar_ActualPostPopAndBaseProcess) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });ConfigureStart(config);MissionScript script(config,"DemoStormRages");CHECK(script.Loaded());
    auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());StartExternal external;
    RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),external.Services());session.Load();auto core=session.Core();Sid home{};
    for (const auto& object:session.World().Objects()) if (object.object.type==143) { home=object.sid;break; } // 실제 배치의 첫 맨 오 워 풀을 사용한다.
    CHECK(home.value!=0);if (!home.value) return;const auto rawHome=core.pool.Slot(home);
    const auto x=std::bit_cast<float>(Get(rawHome,14))-.5f,y=std::bit_cast<float>(Get(rawHome,18))-2.0f;const auto owner=rawHome[34];
    const auto sid=session.World().Create(144,0);Put(core.pool.AllocatedBytes(sid),36,0);core.owner.Set(sid,owner);Put(core.pool.AllocatedBytes(sid),12,home.value,2);session.World().Pop(sid,x,y,0);
    const auto form=FindAttachedProcess(core.pool,sid,31);CHECK(form.value!=0);auto* process=form.value ? dynamic_cast<FlyerProcess*>(core.processes.FromForm(form)) : nullptr;
    CHECK(process && process->Parent()==sid && process->Fields().Form()==form && process->Fields().Parent()==sid && process->Fields().Home()==home);
    if (process) { CHECK(Get(process->Fields().bytes,0)==0x509f60 && Get(process->Fields().bytes,48)==0x509f40);CHECK(process->Fields().bytes[8]==0);CHECK(Get(process->Fields().bytes,84)==0 && Get(process->Fields().bytes,96)==0 && Get(process->Fields().bytes,104)==0); }
    const auto raw=core.pool.Slot(sid);const Sid shadow{static_cast<std::uint16_t>(Get(raw,8,2))};CHECK(shadow.value!=0);
    if (shadow.value) {
        const auto back=core.pool.Slot(shadow);CHECK(back[10]==144 && (back[40]&8U) && !(back[11]&5U));CHECK(Get(back,36)==core.frames[144].Codes().size());
        CHECK(std::bit_cast<float>(Get(back,14))==x+.5f && std::bit_cast<float>(Get(back,18))==y+.5f);CHECK(FindAttachedProcess(core.pool,shadow,31).value==0);
    }
    CHECK(core.bookkeeping.depth==0); // 재귀 그림자 Pop 뒤 부모의 공통 장부도 한 번만 완료되어야 한다.
}

// 실제 데모 Start·전체 Kernel이 만든 화살의 이동과 공통 가상 삭제/Host 정리를 확인한다.
TEST_CASE(CampaignCrossbow_ActualArrowFlightAndDestroy) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });ConfigureStart(config);MissionScript script(config,"DissolvedAlliance");
    StartExternal external;auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),external.Services());session.Load();
    GameClock clock;MissionFrameGateState gate;MissionStartState state;MissionStartGlobals globals;MissionAiSettingsState ai;gate.demo=1;ai.stuffRefreshRate=4;
    auto effects=external.Effects();effects.gameSeconds=[&] { return clock.GameSeconds(0); };
    RawCampaignMissionStartBindings start(session,script,gate,state,globals,ai,std::move(effects));start.Start();
    auto core=session.Core();Sid projectile{},missileForm{};std::array<std::uint32_t,2> initial{};bool moved=false;
    // 현재20프레임의 별도 피해 처리 경계 앞에서 실제 첫 화살의 생성·이동만 관찰한다.
    for (std::uint32_t frame=1;frame<=19 && !moved;++frame) {
        session.RunFrame(clock.Capture(frame*25),false);
        // 등록된 실제 Missile에서 TYPE140 부모·TYPE81 표시를 가진 첫 자산만 찾는다.
        for (std::uint32_t number=5;number<core.pool.Capacity();++number) {
            const Sid form{static_cast<std::uint16_t>(number)};const auto raw=core.pool.Slot(form);
            if ((raw[11]&7) || raw[10]!=28) continue;
            auto* missile=dynamic_cast<Missile*>(core.processes.FromForm(form));CHECK(missile!=nullptr);if (!missile) continue;
            if (core.pool.Slot(missile->Fields().Firing())[10]!=140) continue;
            const auto parent=missile->Parent();const auto asset=core.pool.Slot(parent);CHECK(asset[10]==81);
            CHECK(Get(asset,0)==0x501dd0 && Get(asset,36)<core.frames[81].Codes().size());
            const std::array<std::uint32_t,2> position{Get(asset,14),Get(asset,18)};
            if (!projectile.value) { projectile=parent;missileForm=form;initial=position; }
            else if (parent==projectile && position!=initial) moved=true;
        }
    }
    CHECK(projectile.value && missileForm.value && moved);if (!projectile.value || !missileForm.value) return;
    // 명시 삭제 입력으로 공통81 가상 Pre/Post와 실제 공간/연결 Form 해제를 검사한다. 피해 완주로 세지 않는다.
    session.World().Destroy(projectile,0);CHECK((core.pool.Slot(projectile)[11]&1)!=0);
    CHECK((core.pool.Slot(missileForm)[11]&1)!=0 && core.processes.FromForm(missileForm)==nullptr);
    CHECK(core.bookkeeping.depth==0 && gate.started==1);
}

// Start 전 실제 저장 배치의 같은 Kernel이 화살 자연 충돌·일반HP·효과·삭제를 완료해야 한다.
TEST_CASE(CampaignCrossbow_ActualNaturalDamageBeforeAiStart) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078); // 보호10.78 자원을 읽는 동일 입력이다.
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });ConfigureStart(config);MissionScript script(config,"DissolvedAlliance");
    StartExternal external;auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable()); // 실제 저장 객체를 재배치하거나 HP를 바꾸지 않는다.
    RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),external.Services());session.Load();auto core=session.Core();
    std::vector<std::pair<Sid,std::int32_t>> initialTargets; // Load 직후 실제 썬더 캐논 HP를 독립 입력으로 보존한다.
    // 같은 원본 배치의 TYPE131만 읽으며 관찰용 자산이나 가짜 GunProcess를 생성하지 않는다.
    for (const auto& object:session.World().Objects()) if (object.object.type==131) {
        CHECK(session.HitPoints().HasHitPoints(object.sid));initialTargets.emplace_back(object.sid,std::bit_cast<std::int32_t>(Get(core.pool.Slot(object.sid),26)));
    }
    CHECK(!initialTargets.empty() && session.AiMemory().Allocations().empty());
    GameClock clock;Sid projectile{},missileForm{},victim{},firing{}; // 동일 실제 프로세스/자산을 충돌 뒤까지 추적한다.
    std::array<std::uint32_t,2> initialPosition{};std::int32_t before{},after{}; // 실제 포구·초기HP·피격 뒤HP 입력이다.
    bool moved=false,damaged=false,naturallyDeleted=false,impactCreated=false;std::uint32_t collisionFrame{}; // 생성/비행/피해/효과/삭제의 서로 다른 관찰이다.
    // 최초 자연 충돌이 끝나면 멈춘다. 이후 미복원 전투나 실제Start/AI를 이 검사에서 대신하지 않는다.
    for (std::uint32_t frame=1;frame<=80 && !naturallyDeleted;++frame) {
        session.RunFrame(clock.Capture(frame*25),false);
        // 실제 Missile과 같은 source GunProcess를 찾아 원본131 표적을 가진 첫 화살을 관찰한다.
        for (std::uint32_t number=5;number<core.pool.Capacity();++number) {
            const Sid form{static_cast<std::uint16_t>(number)};const auto raw=core.pool.Slot(form);if ((raw[11]&7) || raw[10]!=28) continue;
            auto* missile=dynamic_cast<Missile*>(core.processes.FromForm(form));CHECK(missile!=nullptr);if (!missile) continue;
            const auto source=missile->Fields().Firing(),target=missile->Fields().Target();if (core.pool.Slot(source)[10]!=140 || core.pool.Slot(target)[10]!=131) continue;
            const auto parent=missile->Parent();const auto asset=core.pool.Slot(parent);CHECK(asset[10]==81 && Get(asset,0)==0x501dd0);
            if (!projectile.value) {
                projectile=parent;missileForm=form;victim=target;firing=source;initialPosition={Get(asset,14),Get(asset,18)};
                // Load 때 보존한 같은 실제 표적의 HP를 찾아 피격 결과와 구별한다.
                for (const auto& [sid,health]:initialTargets) if (sid==victim) before=health;
                const auto gunForm=FindAttachedProcess(core.pool,firing,34);auto* gun=gunForm.value ? dynamic_cast<GunProcess*>(core.processes.FromForm(gunForm)) : nullptr; // 같은 부모의 실제 총 상태 기계다.
                CHECK(gun && gun->Parent()==firing && gun->Fields().Parent()==firing && before>1);
            } else if (parent==projectile && std::array<std::uint32_t,2>{Get(asset,14),Get(asset,18)}!=initialPosition) moved=true;
        }
        if (!projectile.value) continue;
        after=std::bit_cast<std::int32_t>(Get(core.pool.Slot(victim),26));damaged=after>0 && after<before;
        naturallyDeleted=(core.pool.Slot(projectile)[11]&1)!=0;
        if (naturallyDeleted) {
            collisionFrame=frame;CHECK(damaged && moved && !(core.pool.Slot(victim)[11]&3));
            CHECK((core.pool.Slot(missileForm)[11]&1)!=0 && core.processes.FromForm(missileForm)==nullptr);
            // 원본 풀의 실제 반납 기록에서도81 자산과28 Form의 자연 삭제를 확인한다.
            bool assetRecorded=false,formRecorded=false;
            // 두 번호 영역의 최근 삭제 기록을 모두 읽어 예측/서버 배치 차이를 허용한다.
            for (const bool client:{false,true}) {
                // 타입까지 대조해 같은 SID의 다른 과거 삭제를 혼동하지 않는다.
                for (const auto& record:core.pool.Deletions(client)) {
                    if (record.sid==projectile.value && record.type==81) assetRecorded=true;
                    if (record.sid==missileForm.value && record.type==28) formRecorded=true;
                }
            }
            CHECK(assetRecorded && formRecorded);
            // 충돌이 만든 실제 D구간 anim160과 같은 Regular 예약을 검사한다.
            for (const auto& object:session.World().Objects()) if (object.object.type==160) {
                const auto raw=core.pool.Slot(object.sid);const auto physical=Get(raw,36);const auto codes=core.frames[160].Codes();if (physical>=codes.size() || codes[physical].side!='D') continue;
                const auto* regular=core.processes.FindEvent(object.sid,3);CHECK(regular && regular->Parent()==object.sid && std::bit_cast<std::uint32_t>(regular->Payload())==0x3ca3d70a);impactCreated=true;
            }
        }
    }
    CHECK(projectile.value && victim.value && firing.value && moved && damaged && naturallyDeleted && impactCreated);
    CHECK(session.AiMemory().Allocations().empty() && core.bookkeeping.depth==0);
    std::fprintf(stderr,"실제 Start 전 화살 자연 피격: frame%u source%u target%u HP%d→%d projectile%u Form%u\n",collisionFrame,firing.value,victim.value,before,after,projectile.value,missileForm.value);
}
namespace {
// 실제 StartDemo의 자연 피격에서 명시 감시 목록 입력의 유무를 구별한다.
void CheckStartedDemoAiReaction(bool tracked) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });ConfigureStart(config);MissionScript script(config,"DissolvedAlliance");
    StartExternal external;auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),external.Services());session.Load();auto core=session.Core();
    std::map<std::uint16_t,std::int32_t> health; // 실제 저장 썬더 캐논의 시작 HP이며 쓰기 없이 비교한다.
    std::map<std::uint16_t,Sid> sources; // 자연 Missile이 이미 기록한 표적별 실제 크로스보우 SID다.
    // 저장된 자연 배치에서만 표적 HP를 읽으며 새 공격 객체를 만들지 않는다.
    for (const auto& object:session.World().Objects()) if (object.object.type==131) health.emplace(object.sid.value,std::bit_cast<std::int32_t>(Get(core.pool.Slot(object.sid),26)));
    CHECK(!health.empty());GameClock clock;MissionFrameGateState gate;MissionStartState state;MissionStartGlobals globals;MissionAiSettingsState ai;gate.demo=1;ai.stuffRefreshRate=4;
    auto effects=external.Effects();effects.gameSeconds=[&] { return clock.GameSeconds(0); };RawCampaignMissionStartBindings start(session,script,gate,state,globals,ai,std::move(effects));start.Start();
    Sid trackedTarget{}; // 실제 첫 화살에서 확인한 표적 한 개만 용량2의 원본 감시 목록에 명시한다.
    bool observed=false; // 첫 자연 피격 뒤 같은 현재 AI 목록에서 공격자가 확인됐는지 기록한다.
    // 이번 검사는 최초 피해와 목록 반응까지만 실행하며 이후 전투 완주를 주장하지 않는다.
    for (std::uint32_t frame=1;frame<=40 && !observed;++frame) {
        // 현재 Missile의 실제 발사/표적 관계를 피격 전 같은 Pool에서 보존한다.
        for (std::uint32_t number=5;number<core.pool.Capacity();++number) {
            const Sid form{static_cast<std::uint16_t>(number)};const auto raw=core.pool.Slot(form);if ((raw[11]&7U) || raw[10]!=28) continue;
            auto* missile=dynamic_cast<Missile*>(core.processes.FromForm(form));if (!missile || core.pool.Slot(missile->Fields().Firing())[10]!=140) continue;
            sources[missile->Fields().Target().value]=missile->Fields().Firing();
            if (tracked && !trackedTarget.value && health.contains(missile->Fields().Target().value)) {
                // 피격 전 검사 입력이며 AI 전략의 자동 감시 선택을 구현한 것으로 세지 않는다. HP/좌표/발사는 그대로다.
                trackedTarget=missile->Fields().Target();auto& memory=const_cast<RawAiBrainMemory&>(session.AiMemory());
                const auto token=session.Participation().Brain(core.pool.Slot(trackedTarget)[34]);CHECK(token!=0);if (!token) continue;
                auto brain=memory.Bytes(token);const auto count=Get(brain,0x38c),capacity=Get(brain,0x388);CHECK(count<capacity);if (count>=capacity) continue;
                Put(memory.Bytes(Get(brain,0x384)),count*4,trackedTarget.value);Put(brain,0x38c,count+1);
            }
        }
        session.RunFrame(clock.Capture(frame*25),false);
        // HP가 자연 감소한 첫 썬더 캐논의 실제 소유자 AI를 같은 Player+0xb0에서 조회한다.
        for (const auto& [number,before]:health) {
            if (tracked && number!=trackedTarget.value) continue;
            const auto raw=core.pool.Slot(Sid{number});if (std::bit_cast<std::int32_t>(Get(raw,26))>=before) continue;
            CHECK(sources.contains(number));if (!sources.contains(number)) continue;const auto attacker=sources.at(number);
            const auto token=session.Participation().Brain(raw[34]);CHECK(token!=0);if (!token) continue;const auto brain=session.AiMemory().Bytes(token);
            // 두 원본 목록의 실제 backing에 이번 공격자가 들어갔는지 검사한다.
            for (const auto offset:{0x36cU,0x378U}) {
                const auto count=Get(brain,offset+8);const auto list=session.AiMemory().Bytes(Get(brain,offset));bool found=false;
                // 논리 count 범위만 검색하여 보존된 padding을 데이터로 세지 않는다.
                for (std::uint32_t index=0;index<count;++index) if (Get(list,index*4)==attacker.value) found=true;
                if (tracked) CHECK(found);
                else { CHECK(!found && count==0);CHECK(Get(brain,0x38c)==0 && core.types[raw[10]].flags2==0x08040010U); }
            }
            CHECK(core.pool.Slot(attacker)[10]==140 && core.bookkeeping.depth==0);observed=true;break;
        }
    }
    CHECK(observed && gate.started==1 && gate.demo==1);
}
}
// 실제 기본 AI의 비감시 캐논 피격은 원문대로 목록을 바꾸지 않고 HP·충돌만 진행한다.
TEST_CASE(CampaignDamage_StartedDemoUntrackedAiNaturalReturn) { CheckStartedDemoAiReaction(false); }
// 명시 감시 목록 초기 입력 뒤 실제 자연 피격이 같은 AI의 두 공격자 목록을 갱신한다.
TEST_CASE(CampaignDamage_StartedDemoTrackedAiReaction) { CheckStartedDemoAiReaction(true); }
