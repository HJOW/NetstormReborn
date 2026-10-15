// 실제 시작/600바이트 설정/AI 생성 요청의 전체 원본 출력과 효과 순서를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawMissionStart.h"
#include "o/RawMissionAiSettings.h"
#include "o/RawAiCreate.h"
#include <cstring>
#include <set>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// TSV의 문자열 바이트를 변경 없이 복원한다.
std::string StartUnhex(std::string_view value) {
    std::string result;
    // 두 hex 글자가 한 실제 문자열 바이트다.
    for (std::size_t index=0;index<value.size();index+=2) result+=static_cast<char>(std::stoul(std::string(value.substr(index,2)),nullptr,16));return result;
}
// 실제 효과 인자에 쓸 문자열 바이트 hex다.
std::string StartHex(std::string_view value) { return Hex(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(value.data()),value.size())); }
// 실제 double 필드를 메모리 순서로 직렬화한다.
std::string StartDouble(double value) { return Hex(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(&value),sizeof(value))); }
// 모든 입력 저장소/효과는 원본과 동일하게 준비하고 기대 분기를 계산하지 않는다.
struct StartScene {
    std::vector<std::int64_t> numbers; // 현재 행의 명시 정수 입력이다.
    std::map<std::string,std::string> config; // 외부 Config 조회가 반환할 문자열 입력이다.
    std::array<std::uint8_t,600> settings{}; // 초기 미정 바이트를 포함한 실제 설정 전체다.
    std::string trace; // 실제 발생한 조회/효과의 순서다.
    MissionAiSettingsState ai; // 실제 기본값 전역이다.
    MissionAiSettingsHooks aiHooks; // 원본 CRT/설정/진단의 외부 관찰이다.
    // 입력 문자열/바이트만 복원하며 설정의 기본값/덮어쓰기 결과를 계산하지 않는다.
    explicit StartScene(std::string_view input) {
        const auto fields=Split(input,'|');
        // DWORD와 음수 입력을 손실 없이 저장한다.
        for (const auto& field:Split(fields.at(0),',')) numbers.push_back(std::stoll(field));
        // 설정 값은 세미콜론을 포함할 수 있으므로 hex로 보존한다.
        for (std::size_t index=1;index<fields.size();++index) if (!fields[index].empty()) { const auto equal=fields[index].find('=');config[fields[index].substr(0,equal)]=StartUnhex(std::string_view(fields[index]).substr(equal+1)); }
        // 원본과 같은 caller 미정 바이트 패턴이며 기대 출력은 아니다.
        for (std::size_t index=0;index<settings.size();++index) settings[index]=static_cast<std::uint8_t>(index*37+numbers[0]);
        aiHooks.get=[this](std::string_view key) { return Get(key); };aiHooks.parseFloat=[this](std::string_view value) { Add("F,"+StartHex(value));return std::stod(std::string(value)); };
        aiHooks.unknownAbilityAssertion=[this](std::string_view) { Add("A"); };aiHooks.unknownAbilityLog=[this](std::string_view) { Add("L"); };
    }
    // 실제 호출을 누적하며 기대 순서를 미리 작성하지 않는다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 매 조회의 원본 키와 순서를 기록한다.
    std::string Get(std::string_view key) { Add("G,"+std::string(key));return config[std::string(key)]; }
    // 기본 글로벌과 실제 설정 처리기를 조립한다.
    RawMissionAiSettings SettingsRuntime() { ai.stuffRefreshRate=static_cast<std::int32_t>(numbers[2]);ai.startingSp=static_cast<std::int32_t>(numbers[3]);ai.checkingAsserts=numbers[4]!=0;return {OriginalEdition::Patch1078,ai,aiHooks}; }
    // 실제 공통 Start 결과를 직렬화한다. 현재 getter 재생은 fallback만 반대로 두고 같은 독립 입력을 사용한다.
    std::string Start(bool currentInputs=false) {
        MissionFrameGateState gate;gate.demo=static_cast<std::uint32_t>(numbers[3]);gate.started=0;gate.startSeconds=77.25;gate.demoEndSeconds=77.25;gate.tellAtSeconds=77.25;
        MissionStartState state;state.flags=0xffffffff;state.sequence=static_cast<std::uint32_t>(numbers[5]);state.tellAtIndex=static_cast<std::uint32_t>(numbers[6]);state.fieldC0=0xa5a5a5a5;state.fieldC4=0xa5a5a5a5;
        MissionStartGlobals globals;globals.localOwner=static_cast<std::uint32_t>(numbers[0]);globals.playerKnown=static_cast<std::uint32_t>(numbers[1]);globals.editor=static_cast<std::uint32_t>(numbers[2]);MissionStartHooks hooks;
        hooks.get=[this](std::string_view key) { return Get(key); };hooks.sectionNames=[this](std::string_view prefix) { Add("N,"+std::string(prefix));return config["@sections"]; };
        hooks.set=[this](std::string_view key,std::string_view value) { Add("W,"+std::string(key)+','+StartHex(value));config[std::string(key)]=value; };hooks.gameSeconds=[] { return 16777217.25; };
        hooks.setSp=[this](std::uint32_t owner,float value) { Add("S,"+std::to_string(owner)+','+std::to_string(std::bit_cast<std::uint32_t>(value))); };hooks.refreshSp=[this] { Add("R"); };
        hooks.clearTechnology=[this](std::uint32_t owner) { Add("C,"+std::to_string(owner)); };hooks.addTechnology=[this](std::uint32_t owner,std::string_view tech) { Add("T,"+std::to_string(owner)+','+StartHex(tech)); };
        hooks.hasPriest=[this](std::uint32_t owner) { Add("P,"+std::to_string(owner));return (static_cast<std::uint32_t>(numbers[4])&(1U<<owner))!=0; };hooks.createAi=[this](std::uint32_t owner) { Add("I,"+std::to_string(owner)); };
        hooks.refreshCommandGump=[this] { Add("Q"); };hooks.randomGeysers=[this] { Add("Y"); };hooks.resetCamera=[this] { Add("Z"); };
        const auto currentOwner=globals.localOwner,currentKnown=globals.playerKnown,currentEditor=globals.editor; // 기존 native의 실제 입력이며 getter 재생에서도 변경하지 않는다.
        const auto fallbackOwner=currentOwner==1 ? 8U : 1U,fallbackKnown=currentKnown==0 ? 1U : 0U,fallbackEditor=currentEditor==0 ? 1U : 0U; // 잘못된 fallback 사용을 드러낼 반대 입력이다.
        RawMissionStart runtime(OriginalEdition::Patch1078,gate,state,globals,hooks);
        if (currentInputs) {
            globals.localOwner=fallbackOwner;globals.playerKnown=fallbackKnown;globals.editor=fallbackEditor;
            runtime.BindCurrentInputs({[&] { return currentOwner; },[&] { return currentKnown; },[&] { return currentEditor; }});
        }
        runtime.Start();
        if (currentInputs) CHECK(globals.localOwner==fallbackOwner && globals.playerKnown==fallbackKnown && globals.editor==fallbackEditor);
        const std::array<std::uint32_t,10> values{{state.flags,state.sequence,state.tellAtIndex,state.fieldC0,state.fieldC4,gate.started,static_cast<std::uint32_t>(globals.denySalvage),static_cast<std::uint32_t>(globals.denyAscend),static_cast<std::uint32_t>(globals.allowAnyCapture),globals.ordinaryGeysers}};std::string result;
        // 전체 관찰 DWORD와 double 비트를 원본 fixture와 같은 순서로 쓴다.
        for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result+'|'+StartDouble(gate.startSeconds)+'|'+StartDouble(gate.demoEndSeconds)+'|'+StartDouble(gate.tellAtSeconds);
    }
    // 실제 Player 부착 몸체를 재사용하고 원본 생성자 요청만 외부 경계로 관찰한다.
    std::pair<std::string,std::uint32_t> Create() {
        const auto owner=static_cast<std::uint32_t>(numbers[0]);std::array<std::uint8_t,1620> players{};std::array<std::uint32_t,81> allies{};std::array<std::uint8_t,9> hidden{};PlayerParticipationState playerState;PlayerParticipationHooks playerHooks;
        Put(players,owner*180+0xb0,static_cast<std::uint32_t>(numbers[1]));Put(players,owner*180+0x78,127);Put(players,owner*180+0x50,static_cast<std::uint32_t>(numbers[5]));Put(players,owner*180+0x2c,static_cast<std::uint32_t>(numbers[6]));RawPlayerParticipation participation(OriginalEdition::Patch1078,players,allies,hidden,playerState,playerHooks);
        settings.fill(0);Put(settings,0x244,static_cast<std::uint32_t>(numbers[2]));Put(settings,0x250,static_cast<std::uint32_t>(numbers[3]));AiCreateHooks hooks;
        hooks.changeColor=[this](std::uint32_t target,std::uint32_t color) { Add("O,"+std::to_string(target)+','+std::to_string(color)); };hooks.firstRegion=[](std::uint32_t) { return 127U; };
        hooks.islandTemple=[this](std::uint32_t region) { Add("V,"+std::to_string(region));return static_cast<std::uint32_t>(numbers[4]); };hooks.destroyTemple=[this](std::uint32_t,std::uint32_t flags) { Add("D,"+std::to_string(flags)); };
        hooks.construct=[this](std::uint32_t target,std::span<const std::uint8_t>,std::size_t size) { Add("B,"+std::to_string(size));Add("K,"+std::to_string(target));return 0x12310000U; };hooks.initialUpdate=[this](std::uint32_t token,std::span<const std::uint8_t>) { Add("U,"+std::to_string(token)); };
        RawAiCreate runtime(OriginalEdition::Patch1078,participation,hooks);const auto token=runtime.Create(owner,settings);return {Hex(std::span<const std::uint8_t>(players).subspan(owner*180,180)),token};
    }
};
}
// 실제 기계어의 전체 설정600바이트/Player180바이트/모든 상태와 효과를 고유 입력204개에서 대조한다.
TEST_CASE(MissionStart_Native204) {
    std::size_t count=0;std::set<std::pair<std::string,std::string>> unique;
    // 같은 입력의 재생/강화를 새로운 관찰로 세지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_MISSION_START_FIXTURE).rows) {
        if (row[0]=="kind") continue;
        CHECK(row.size()==5);CHECK(unique.emplace(row[0],row[1]).second);StartScene scene(row[1]);std::string raw;std::string result="-";
        if (row[0]=="start") raw=scene.Start();else if (row[0]=="create") { const auto created=scene.Create();raw=created.first;result=std::to_string(created.second); }
        else { auto runtime=scene.SettingsRuntime();if (row[0]=="init") runtime.Initialize(scene.settings);else if (row[0]=="apply") runtime.Apply(scene.settings,static_cast<std::int32_t>(scene.numbers[1]));else if (row[0]=="prepare") runtime.Prepare(scene.settings,static_cast<std::int32_t>(scene.numbers[1]));else if (row[0]=="ability") runtime.SetAbility(scene.settings,scene.config["name"],scene.numbers[5]!=0);else CHECK(false);raw=Hex(scene.settings); }
        CHECK(raw==row[2]);CHECK(result==row[3]);CHECK((scene.trace.empty() ? "-" : scene.trace)==row[4]);++count;
    }
    CHECK(count==204);
}
// 기존Start48개의원본출력을현재getter/반대fallback으로그대로재생하며새native입력으로합산하지않는다.
TEST_CASE(MissionStart_CurrentInputsReplayNative48WithOppositeFallback) {
    std::size_t compared=0;
    // 원본48개에서기대상태/모든외부효과를다시읽는다.기대값을getter구현으로계산하지않는다.
    for (const auto& row:LoadFixture(NETSTORM_MISSION_START_FIXTURE).rows) {
        if (row[0]!="start") continue;StartScene scene(row[1]);CHECK(scene.Start(true)==row[2]);
        CHECK((scene.trace.empty() ? "-" : scene.trace)==row[4]);++compared;
    }
    CHECK(compared==48);
}
// SP/표시/기술/AI효과뒤현재owner재조회와myTech앞의owner보관을원문순서대로검사한다.
TEST_CASE(MissionStart_CurrentInputsReadAtOriginalEffectBoundaries) {
    MissionFrameGateState gate;MissionStartState state;MissionStartGlobals globals;globals.localOwner=8; // getter와다른값형fallback이다.
    std::uint32_t currentOwner=1,currentKnown=1,currentEditor=1;std::string trace; // 외부효과가바꾸는현재원문입력과관찰순서다.
    const auto add=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value; }; // 실제읽기/효과위치에서만관찰한다.
    MissionStartHooks hooks;
    hooks.get=[&](std::string_view key) {
        if (key=="myStartMoney") { add("M");return std::string("12"); }
        if (key=="myTech") { add("T");currentOwner=5; }
        return std::string{};
    };
    hooks.sectionNames=[](std::string_view) { return std::string{}; };hooks.set=[](std::string_view,std::string_view) {}; // 해당설정의명시빈입력이며현재getter순서관찰을방해하지않는다.
    hooks.setSp=[&](std::uint32_t owner,float money) { CHECK(owner==1 && money==194162.0F);add("S1");currentOwner=2; };
    hooks.refreshSp=[&] { add("R");currentOwner=3; };hooks.clearTechnology=[&](std::uint32_t owner) { CHECK(owner==3);add("C3");currentOwner=4; };
    hooks.addTechnology=[&](std::uint32_t owner,std::string_view) { CHECK(owner==4);add("A4");currentOwner=6; };
    hooks.hasPriest=[&](std::uint32_t owner) { add("P"+std::to_string(owner));return owner==2; };
    hooks.createAi=[&](std::uint32_t owner) { CHECK(owner==2);add("I2");currentOwner=7; };
    hooks.refreshCommandGump=[&] { add("Q"); };hooks.resetCamera=[&] { add("Z"); };hooks.gameSeconds=[&] { add("V");return 17.25; };
    RawMissionStart start(OriginalEdition::Patch1078,gate,state,globals,hooks);
    CHECK(Throws([&] { start.BindCurrentInputs({[&] { return currentOwner; },{},[&] { return currentEditor; }}); }));
    start.BindCurrentInputs({[&] { add("O"+std::to_string(currentOwner));return currentOwner; },[&] { add("K");return currentKnown; },[&] { add("E");return currentEditor; }});
    start.Start();CHECK(trace=="M;E;K;O1;S1;R;O3;C3;O4;T;A4;O6;P1;O6;P2;I2;O7;P3;O7;P4;O7;P5;O7;P6;O7;O7;P8;Q;Z;V");
    CHECK(globals.localOwner==8 && globals.playerKnown==0 && globals.editor==0 && gate.started==1 && gate.startSeconds==17.25);
}
// 미복원 생성자/초기갱신을 실제 성공으로 간주하지 않으며 기존 brain과 NoTemple은 생성자 없이도 정확히 반환한다.
TEST_CASE(MissionStart_MissingBrainBodiesAndLiveState) {
    std::array<std::uint8_t,1620> players{};std::array<std::uint32_t,81> allies{};std::array<std::uint8_t,9> hidden{};PlayerParticipationState playerState;RawPlayerParticipation participation(OriginalEdition::Patch1078,players,allies,hidden,playerState,{});std::array<std::uint8_t,600> settings{};
    RawAiCreate missing(OriginalEdition::Patch1078,participation,{});bool rejected=false;try { missing.Create(1,settings); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);CHECK(participation.Brain(1)==0 && players[180+0x50]==0);
    Put(players,180+0xb0,73);CHECK(missing.Create(1,settings)==73);Put(players,180+0xb0,0);
    MissionFrameGateState gate;MissionStartState state;MissionStartGlobals globals;MissionStartHooks hooks;hooks.get=[](std::string_view) { return std::string{}; };hooks.sectionNames=[](std::string_view) { return std::string{}; };hooks.set=[](std::string_view,std::string_view) {};hooks.refreshSp=[] {};hooks.clearTechnology=[&](std::uint32_t owner) { CHECK(owner==1);globals.localOwner=8; };
    hooks.addTechnology=[&](std::uint32_t owner,std::string_view) { CHECK(owner==8); };hooks.hasPriest=[&](std::uint32_t owner) { CHECK(owner!=globals.localOwner);return false; };hooks.refreshCommandGump=[] {};hooks.resetCamera=[] {};hooks.gameSeconds=[] { return 16777217.25; };
    RawMissionStart start(OriginalEdition::Patch1078,gate,state,globals,hooks);start.Start();CHECK(gate.started==1 && gate.startSeconds==16777217.25);
    gate.started=0;hooks.refreshSp={};RawMissionStart unavailable(OriginalEdition::Patch1078,gate,state,globals,hooks);rejected=false;try { unavailable.Start(); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && gate.started==0);
}
