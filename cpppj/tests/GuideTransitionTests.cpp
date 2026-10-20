// 실제 Guide 교체/명령 분배와 보호 캠페인의 다음 미션 버튼을 같은 문자열 고리·큐에 연결해 대조한다.
#include "DudeSpawnSupport.h"
#include "client/GuideMissionTransitions.h"
#include "client/MissionLifecycle.h"
#include "client/Mission.h"
#include "client/DialogScript.h"

using namespace netstorm::client;
namespace {
// 원문 문자열 hex를 바이트 그대로 복원한다.
std::string TransitionBytes(std::string_view encoded) {
    std::string text;
    // 원본 각 바이트를 읽으며 UTF-8 치환으로 기대값을 바꾸지 않는다.
    for (std::size_t index=0;index<encoded.size();index+=2) text+=static_cast<char>(std::stoul(std::string(encoded.substr(index,2)),nullptr,16));
    return text;
}
// 독립 원문 입력과 같은 현재/재진입/이름/필드 및 명시 수명 경계다.
struct TransitionScene {
    std::array<std::uint32_t,9> input{}; // 연산/현재/모드/데모/재진입/인자/이름/플래그/x의 입력이다.
    std::uint32_t active{},reentrant{},mode{},demo{}; // 현재 식별자와 새 객체의 두 필드다. 앞선 객체와 별도로 관측한다.
    std::string guideName{"TheWarBegins"},beginName,target,trace; // 원본 Guide 이름과 별도 원문/해제되는 설정 버퍼다.
    // 관련 필드를 독립 입력으로만 준비하며 교체 결과는 계산하지 않는다.
    explicit TransitionScene(std::string_view values) {
        const auto fields=Split(values,',');CHECK(fields.size()==input.size());
        // 각 DWORD의 높은 비트와 signed 인자를 유지한다.
        for (std::size_t index=0;index<input.size();++index) input[index]=Number(fields[index]);
        active=input[1];reentrant=input[4];mode=input[2];demo=input[3];beginName=input[6] ? "MasterOfWhirligigs" : "Tutorial2";target=beginName;
    }
    // 실제 효과 한 번의 순서를 기록한다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 원본 명시 경계와 같은 해제 재진입/설정 무효화/새 공장 필드를 공급한다.
    GuideMissionTransitionHooks Hooks() {
        GuideMissionTransitionHooks hooks;hooks.current=[this] { return active!=0; };hooks.finish=[this] { Add("F,"+std::to_string(active)); };
        hooks.release=[this] { Add("L,"+std::to_string(active));active=0;guideName.clear();target="invalidated";if (reentrant) { active=2;reentrant=0; } };
        hooks.create=[this](std::string_view name) { Add("C,"+Hex({reinterpret_cast<const std::uint8_t*>(name.data()),name.size()}));guideName=name;active=3;mode=demo=0; };
        hooks.get=[this](std::string_view key) { Add("G,"+Hex({reinterpret_cast<const std::uint8_t*>(key.data()),key.size()}));return target; };
        hooks.mode=[this](std::uint32_t value) { CHECK(active==3);mode=value; };hooks.demo=[this](std::uint32_t value) { CHECK(active==3);demo=value; };return hooks;
    }
    // 실제 제품 분배/교체 함수를 호출하고 원문의 최종 이름/현재/필드/순서를 직렬화한다.
    std::string Run() {
        const auto hooks=Hooks();std::string result="void";
        const auto restart=[&] { const auto request=CaptureMissionReload(guideName,input[2],input[3]);RestartGuideMission(request,hooks); }; // 원본 첫 해제 전에 같은 입력 필드를 캡처한다.
        if (input[0]==0) BeginGuideMission(beginName,hooks);
        else if (input[0]==1) ForkGuideMission(input[5],hooks);
        else if (input[0]==2) restart();
        else {
            const std::array<std::uint32_t,4> codes{kMissionBeginCommand,kMissionForkCommand,kMissionReloadCommand,kMissionEndCommand}; // 독립 원문 연산의 실제 사건 번호다.
            GuideMissionCommandHooks commands;commands.begin=[&](std::uint32_t parameter) { CHECK(parameter==input[5]);BeginGuideMission(beginName,hooks); };
            commands.fork=[&](std::uint32_t parameter) { ForkGuideMission(parameter,hooks); };commands.restart=restart;commands.end=[&] { EndGuideMission(hooks); };commands.invalidBegin=[&](std::uint32_t parameter) { CHECK(parameter==input[5]);Add("A"); };
            result=DispatchGuideMissionCommand({codes[input[0]-3]|input[7],std::bit_cast<std::int32_t>(input[8]),std::bit_cast<std::int32_t>(input[5])},commands) ? "1" : "0";
        }
        return result+'|'+std::to_string(active)+'|'+Hex({reinterpret_cast<const std::uint8_t*>(guideName.data()),guideName.size()})+'|'+std::to_string(mode)+','+std::to_string(demo)+'|'+(trace.empty() ? "-" : trace);
    }
};
}

// 실제 교체256입력과 이름/인자/고리30입력의 최초 독립 원문을 직접 비교한다.
TEST_CASE(GuideTransitions_Native286) {
    const auto rows=LoadFixture(NETSTORM_GUIDE_TRANSITIONS_FIXTURE).rows;CHECK(rows.size()==286);
    // 원문 기대값을 제품의 분기로 다시 계산하지 않는다.
    for (const auto& row:rows) {
        if (row[0]=="T") { CHECK(row.size()==3);TransitionScene scene(row[1]);CHECK(scene.Run()==row[2]); }
        else {
            CHECK(row[0]=="S" && row.size()==4);const auto raw=TransitionBytes(row[1]);const auto comma=raw.find(',');InputQueue queue;ScriptStringRing strings;
            CHECK(QueueMissionControlAction(raw.substr(0,comma),raw.substr(comma+1),queue,strings));const auto event=queue.Pop(false);const auto expected=Split(row[2],',');
            CHECK(expected.size()==2 && event.code==Number(expected[0]) && event.x==event.y && queue.Empty() && strings.Index()==Number(row[3]));
            if (expected[1].front()=='s') { CHECK(event.x>100);CHECK(strings.Resolve(std::bit_cast<std::uint32_t>(event.x))==TransitionBytes(expected[1].substr(1))); }
            else CHECK(std::bit_cast<std::uint32_t>(event.x)==Number(expected[1].substr(1)));
        }
    }
}

// 보호 성공 본문의 실제 Config/Next Mission을 HTML parser→고리/큐→Guide 공장에 전달한다. 월드 승리를 주입하지 않는다.
TEST_CASE(GuideTransitions_AuthoredNextMissionQueueAndOwnedString) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });
    config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\n[END]\n");
    auto mission=std::make_unique<MissionScript>(config,"TheWarBegins");const auto body=mission->Section("Succeeded");CHECK(body);InputQueue queue;ScriptStringRing strings;
    HtmlTextParserState html;HtmlTextParserGlobals globals;HtmlTextParserHooks htmlHooks;htmlHooks.command=[&](HtmlTextParserState&,std::string_view command) {
        QueueScriptCommand(command,queue,strings,{[](std::string_view name) { return name=="Config" ? kScriptConfig : MissionControlScriptCode(name); },{}});
    };
    const auto page=ParseDialogPage(config.SubstituteLocal(*body),html,globals,std::move(htmlHooks));
    const auto item=std::find_if(page.items.begin(),page.items.end(),[](const DialogItem& value) { return value.label=="Next Mission"; });CHECK(item!=page.items.end());
    CHECK(QueueMissionControlAction(item->action.command,item->action.argument,queue,strings));std::size_t finished{},released{},created{};
    GuideMissionTransitionHooks effects;effects.current=[&] { return mission!=nullptr; };effects.finish=[&] { MissionStartState state;RunTutorialMissionStage(state,6,{});++finished; };
    effects.release=[&] { mission.reset();++released; };effects.create=[&](std::string_view name) { CHECK(!mission);mission=std::make_unique<MissionScript>(config,name);++created; };
    // 먼저 예약된 실제 완료 Config를 처리한 다음 같은 큐의 문자열 Begin을 처리한다.
    while (!queue.Empty()) {
        const auto event=queue.Pop(false);
        if (event.code==kScriptConfig) ApplyScriptConfig(strings.Resolve(std::bit_cast<std::uint32_t>(event.x)),config);
        else {
            CHECK(event.y>100);GuideMissionCommandHooks commands;commands.begin=[&](std::uint32_t token) { const std::string name(strings.Resolve(token));BeginGuideMission(name,effects); };
            commands.invalidBegin=[](std::uint32_t) { throw std::logic_error("유효 문자열 고리의 Begin 주소 관문 실패"); };CHECK(DispatchGuideMissionCommand(event,commands));
        }
    }
    CHECK(mission && mission->Loaded() && mission->Name()=="MasterOfWhirligigs" && finished==1 && released==1 && created==1 && config.GetInt("DoneTheWarBegins")==1);
    const auto stale=strings.Store("old");
    // 앞선 문자열 수명이80칸 고리를 벗어나면 새 양수 토큰 도메인에서도 원래처럼 거부한다.
    for (int index=0;index<80;++index) { const auto token=strings.Store("next");CHECK(token>100 && token<=0x7fffffffU); }
    CHECK(Throws([&] { strings.Resolve(stale); }));
}
