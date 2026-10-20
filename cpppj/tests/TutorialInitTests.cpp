// Init 최초 전체 제어/실제 보호11개 Script·Header와 다음 루프 요청을 검사한다. 실행은 후속 통합 단계로 이월한다.
#include "DudeSpawnSupport.h"
#include "client/TutorialInit.h"
#include "client/Mission.h"
#include "client/MissionTechnology.h"
#include "client/State.h"
#include "o/RawMissionBattleOptions.h"
#include "RawSceneSupport.h"
#include <sstream>
using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// ASCII JSON 배열의 원문 바이트를 읽는다. 관측의 \u00xx는 한 원본 BYTE로 복원한다.
std::vector<std::string> InitInput(std::string_view input) {
    std::vector<std::string> values;std::size_t at=1;
    // 고정된 평면 입력만 해석하며 함수 결과를 생성하지 않는다.
    while (at<input.size() && input[at]!=']') {
        if (input[at]==',') { ++at;continue; }std::string value;
        if (input[at]=='"') { ++at;
            // 인용 문자열의 Latin1 escape를 원래 바이트로 복원한다.
            while (at<input.size() && input[at]!='"') { if (input[at]=='\\') { ++at;if (input.substr(at,3)!="u00") throw std::logic_error("Init 입력 escape 오류");value+=static_cast<char>(std::stoul(std::string(input.substr(at+3,2)),nullptr,16));at+=5; }else value+=input[at++]; }++at;
        } else { const auto end=input.find_first_of(",]",at);value=std::string(input.substr(at,end-at));at=end; }values.push_back(std::move(value));
    }return values;
}
// 현재 C문자열의 바이트만 원문 trace용16진 문자열로 옮긴다.
std::string InitHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 명시 큰 경계의 같은 입력/변이와 원본이 남긴 현재 필드를 보유한다.
struct InitScene {
    std::vector<std::string> input;TutorialInitState state;std::uint32_t flags{},demo{},tutorial{0x22222222},mission{0x11111111},seen{},subscriber{},source{}; // 원문 현재 필드/전역이다.
    std::array<std::uint8_t,87> options{};std::string trace; // 실제 원본 초기 옵션과 큰 효과 순서다.
    // 첫 관측의 명시 입력으로 각 행의 지역 문자열과 필드·전역을 새로 구성한다.
    explicit InitScene(std::string_view text):input(InitInput(text)) { if (input.size()!=11) throw std::logic_error("Init 입력 열 수");state.name=input[0];state.scratch.fill(static_cast<char>(0xa5));demo=Number(input[5]);flags=Number(input[6]);seen=Number(input[7]);subscriber=Number(input[8]);source=Number(input[9]);
        // 독립 관측이 사용한 실제87BYTE 전 입력을 공급한다.
        for (std::size_t index=0;index<options.size();++index) options[index]=static_cast<std::uint8_t>(index*11+17);
    }
    // 큰 효과 호출의 관측 구분자를 그대로 유지한다.
    void Add(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 동일 Init 몸체와 실제 옵션 초기화에 입력을 연결하고 전체 출력을 직렬화한다.
    std::array<std::string,5> Run() {
        const auto variant=Number(input[10]);TutorialInitHooks h;h.clearTemplates=[&] { Add("C");if (variant==1) state.name="AFTER_CLEAR"; };
        h.get=[&](std::string_view key,std::optional<std::string_view> fallback) { const auto value=input[key=="tutorialNumber" ? 1 : key=="missionNumber" ? 2 : key=="demo" ? 3 : 4];const auto result=value.empty() && fallback ? std::string(*fallback) : value;Add("G,"+std::string(key)+','+InitHex(fallback.value_or(""))+','+InitHex(result));return result; };
        h.resetOptions=[&] { Add("O");if (variant==2) { state.name="AFTER_OPTIONS";source=7; }ResetMissionBattleOptions(OriginalEdition::Patch1078,options); };
        h.set=[&](auto key,auto value) { Add("S,"+InitHex(key)+','+InitHex(value));if (variant==3) { state.name="AFTER_SET";source=0x76543210; } };
        h.requestFort=[&](auto value) { Add("L,"+InitHex(value));if (variant==4) flags=0xfffffffe; };h.header=[&] { Add("H");if (variant==4) flags^=0x102U; };
        InitializeTutorialMission(state,flags,demo,{tutorial,mission,seen,subscriber,source},h);std::ostringstream fields;fields<<flags<<','<<demo<<','<<state.namePrefix<<','<<tutorial<<','<<mission<<','<<seen<<','<<subscriber;
        return {fields.str(),InitHex({state.scratch.data(),state.scratch.size()}),InitHex(state.name),Hex(options),trace};
    }
};
}
// 152개 첫 관측의 전체 필드/지역80BYTE/원래 이름/옵션/큰 경계 순서를 전수 비교한다.
TEST_CASE(TutorialInit_Native152) {
    const auto rows=LoadFixture(NETSTORM_TUTORIAL_INIT_FIXTURE).rows;CHECK(rows.size()==152);
    // 각 입력은 새 지역/전역을 사용하며 원문 기대값을 다시 계산하지 않는다.
    for (const auto& row:rows) { CHECK(row.size()==6);if (row.size()!=6) return;InitScene scene(row[0]);const auto result=scene.Run();
        // 모든 별도 출력 영역은 같은 원문 열과 비교한다.
        for (std::size_t index=0;index<result.size();++index) CHECK(result[index]==row[index+1]);
    }
}
// 실제 보호11개 설정은 설치 전 Header를 쓰고 같은 State 큐의 다음 루프까지 로드하지 않는다.
TEST_CASE(TutorialInit_ProtectedElevenBeforeQueuedLoad) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ConfigInterface config([&](auto path) { return assets.files.TryRead(path); });config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\n[END]\n");
    constexpr std::array<std::string_view,11> names{"Tutorial1","Tutorial2","Tutorial3","Tutorial4","Tutorial5","Tutorial6","TheWarBegins","MasterOfWhirligigs","DemoStormRages","GuardMyBack","DissolvedAlliance"}; // 지정 전체11개 실제 이름이다.
    constexpr std::array<std::string_view,11> normalized{"Tutorial1","Tutorial2","Tutorial3","Tutorial4","Tutorial5","Tutorial6","Thewarbegins","Masterofwhirligigs","Demostormrages","Guardmyback","Dissolvedalliance"}; // 원본 두 정규화 몸체의 명시 결과다.
    std::array<std::uint32_t,256> allowed{};allowed.back()=0x7391; // 같은 전역 허용 표의 보존 칸이다.
    // 미션 해제의 AllowAll1과 같은 경계를 거친 뒤 새 Init을 호출한다.
    for (std::size_t index=0;index<names.size();++index) {
        SetTotalMadeAllowAll(allowed,1);MissionScript script(config,names[index]);CHECK(script.Loaded());TutorialInitState state;state.name=names[index];State pending;std::array<std::uint8_t,87> options{};std::uint32_t flags{},demo=index>=8 ? 1U : 0U,tutorial{},mission{},seen{},subscriber{},source{1};bool loaded{}; // 현재 Script/전역/다음 루프 요청이다.
        TutorialInitHooks h;h.clearTemplates=[] {};h.get=[&](auto key,std::optional<std::string_view> fallback) { const auto value=script.Get(key).value_or("");return value.empty() && fallback ? std::string(*fallback) : value; };h.set=[&](auto key,auto value) { script.Set(key,value); };h.resetOptions=[&] { ResetMissionBattleOptions(OriginalEdition::Patch1078,options); };
        h.requestFort=[&](auto value) { CHECK(!loaded);pending.Post({"LoadMissionFort",std::string(value)}); };h.header=[&] { CHECK(pending.Pending() && !loaded);ApplyMissionTechnology(script,assets.game.TypeTable(),std::span(allowed)); };
        InitializeTutorialMission(state,flags,demo,{tutorial,mission,seen,subscriber,source},h);CHECK(flags==1 && !loaded && pending.Pending() && subscriber==1 && allowed.back()==0x7391);CHECK(script.Get("fileName")==normalized[index]);CHECK(options[1]==2 && options[2]==2 && options[4]==3);
        if (index==0) CHECK(allowed[assets.game.TypeTable().Find("sunArcher")]==1);if (index==1) CHECK(allowed[assets.game.TypeTable().Find("sunArcher")]==0);
        const auto request=pending.Take();CHECK(request && request->command=="LoadMissionFort" && !request->argument.empty() && !pending.Pending());loaded=true;CHECK(loaded); // 실제 로드 소비는 다음 상태 자리의 책임이다.
    }
}
// 설치 전 미션 교체는 그 소유자의 요청만 반납하고 새 요청을 그대로 보존한다.
TEST_CASE(TutorialInit_PendingRequestReleasedWithOwner) {
    State pending;pending.Post({"LoadMissionFort","oldFort"});CHECK(!pending.Cancel("Tell") && pending.Pending());CHECK(pending.Cancel("LoadMissionFort") && !pending.Pending());CHECK(!pending.Cancel("LoadMissionFort"));
    pending.Post({"LoadMissionFort","newFort"});const auto request=pending.Take();CHECK(request && request->argument=="newFort" && !pending.Pending());pending.Post({"Tell","ABORT"});CHECK(!pending.Cancel("LoadMissionFort") && pending.Pending());
}
