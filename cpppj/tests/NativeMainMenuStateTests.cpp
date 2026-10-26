// State3 선택 부모와 실제 공통144BYTE 확정/기존 tail의 다음 Poll4를 창 없이 대조한다.
#include "TestSupport.h"
#include "client/NativeMainMenuState.h"
#include "client/ExistingEditorAfterCheckpoint.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

namespace { //선택한 OS/UI/파일 장치만 관측하며 Native 분기와 상태 복사는 실제 부모가 수행한다.
using namespace netstorm::client;
using namespace netstorm::o;
struct MainMenuStop {}; //선택된 외부 Tell 중단과 Native 정상 commit을 구별한다.
// 다섯 실제 Native 블록과 서로 다른 각 전역의 명시 입력을 보유한다.
struct MainMenuScene {
    ClientLocalStateGlobals local;PlayerWorldState player;InputQueue input; //기존 실제 상태/현재Player/원시40칸 큐다.
    std::uint32_t source{99},outcome{},first{1},editor{1},updated{},tips{1},intro{1},welcome{1},startupResult{}; //각 DWORD를 합치지 않는다.
    bool battle{},server{},boss{},post{},saved{},mission{},simul{},meta{}; //RawState의 현재 외부 모드다.
    std::uint32_t zone{},ticks{},seed{},refresh{},rebuild{};std::uint8_t battleByte{};double end{},seconds{},wall{}; //각 별도 Native 전역의 실제 참조 입력이다.
    std::array<char,80> background{};std::array<std::uint8_t,256> current{},pending{}; //이름의 NUL 뒤 흔적도 함께 보존한다.
    std::vector<std::string> trace;std::uint32_t startups{},islands{};bool stopTips{}; //외부 도달/부분효과의 관측이다.
    MainMenuScene() {
        auto state=local.State();state.phase.fill(0x10203040);state.phase[0]=4;state.current.fill(0x50607080);state.current[0]=3;
        background.fill('#');current.fill(0xa5);pending.fill(0x5a);const std::string old="Old",next="Next";
        std::copy(old.begin(),old.end(),current.begin());current[old.size()]=0;std::copy(next.begin(),next.end(),pending.begin());pending[next.size()]=0;
    }
    // 부모가 사용하는 이름과 flags는 위의 같은 물리이며 값형 전역 사본을 만들지 않는다.
    NativeMainMenuStateGlobals Globals() { return {local.State(),source,outcome,first,battle,editor,background,current,pending,[this] { trace.emplace_back("updated");return updated; },[this] { trace.emplace_back("tips");return tips; }}; }
    // 공통 tail은 기존 실제 부모를 호출한다. State3에서 섬 수/추가 명령에 도달하면 실패해야 한다.
    void Tail() {
        ExistingEditorAfterCheckpointEffects effects;effects.hud=[this](auto) { trace.emplace_back("hud2"); };effects.wallSeconds=[this] { trace.emplace_back("wall");return 100.5; };
        effects.requestMusic=[this](auto name) { trace.emplace_back("music:"+std::string(name)); };effects.fortPalette=[this] { trace.emplace_back("palette"); };
        effects.islandCount=[this] { ++islands;return 0U; };ApplyExistingEditorAfterCheckpoint(local,input,effects);
    }
    // 미선택 장치는 비워 두어 실제 잘못된 분기가 성공으로 넘어가지 못하게 한다.
    NativeMainMenuStateHooks Hooks() {
        NativeMainMenuStateHooks hooks;
        hooks.configInt=[this](auto key) { trace.emplace_back(std::string(key));if (key=="intro") return static_cast<std::int32_t>(intro);CHECK(key=="WelcomeGiven");return static_cast<std::int32_t>(welcome); };
        hooks.prefix=[this] { CHECK(local.State().phase[0]==4 && local.State().current[0]==3 && std::string_view(background.data())=="titleMenu");trace.emplace_back("prefix");editor=0; };
        hooks.hudPrefix=[this] { trace.emplace_back("hud1"); };hooks.title=[this] { CHECK(!current[0] && !pending[0]);trace.emplace_back("title"); };hooks.closeGuide=[this] { trace.emplace_back("close"); };
        hooks.tell=[this](auto name) { trace.emplace_back("tell:"+std::string(name));if (stopTips && name=="Tips") throw MainMenuStop{}; };
        hooks.startup=[this] { trace.emplace_back("startup");++startups;return startupResult; };hooks.tail=[this] { Tail(); };return hooks;
    }
    // 기존 RawLocalMissionState의 same-block 공통 ret와 다음 Poll을 직접 실행한다.
    void Run() {
        LocalMissionStateHooks hooks;hooks.logState=[](auto) {};hooks.parentState=[this](auto selected) { CHECK(selected==3);return InstallNativeMainMenuState(Globals(),Hooks()); };
        RawLocalMissionState state(OriginalEdition::Patch1078,local.State(),{server,boss,battle,post,saved,mission,simul,meta,player,zone,ticks,source,seed,local.State().immediate[33],refresh,rebuild,reinterpret_cast<std::uint8_t&>(background[0]),battleByte,end,local.ActivityEnd(),seconds,wall},std::move(hooks));
        state.Run();CHECK(local.State().phase[0]==3 && !local.State().current[0] && local.State().next[0]==4);state.Poll();CHECK(local.State().phase[0]==4 && local.State().old[0]==3 && !local.State().next[0]);
    }
};
}

// 초기 안내 순서·같은 두 이름의 첫BYTE만 쓰기·전체 공통확정·다음 Poll4를 함께 검증한다.
TEST_CASE(NativeMainMenuState_DefaultPreludeTellStartupTailAndActualCommitPollFour) {
    MainMenuScene scene;const auto previous=scene.local.State().phase;scene.Run();
    const std::vector<std::string> expected{"intro","prefix","hud1","title","close","WelcomeGiven","tell:Blank","startup","updated","tell:UserNotUpdated","tips","tell:Tips","hud2","wall","music:ser22.mus","palette"};
    CHECK(scene.trace==expected && !scene.source && !scene.first && !scene.islands && scene.input.Empty() && scene.startups==1 && scene.local.ActivityEnd()==110.5);
    CHECK(!scene.current[0] && scene.current[1]=='l' && scene.current[4]==0xa5 && !scene.pending[0] && scene.pending[1]=='e' && scene.pending[5]==0x5a);
    CHECK(scene.local.State().old[1]==0x50607080 && !scene.local.State().old[33] && scene.local.State().phase[1]==0x50607080 && !scene.local.State().phase[33] && previous[1]==0x10203040);
}

// WelcomeGiven0은 Welcome만 선택하며 첫안내 flag를 소모하거나 startup reader를 부르지 않는다.
TEST_CASE(NativeMainMenuState_FirstWelcomeDoesNotConsumeFirstMenuOrReadStartup) {
    MainMenuScene scene;scene.welcome=0;scene.Run();CHECK(scene.first==1 && !scene.startups && !scene.islands && scene.input.Empty());
    CHECK(std::find(scene.trace.begin(),scene.trace.end(),"tell:Welcome")!=scene.trace.end() && std::find(scene.trace.begin(),scene.trace.end(),"tell:Blank")==scene.trace.end() && std::find(scene.trace.begin(),scene.trace.end(),"updated")==scene.trace.end());
}

// 실제 재접속 질문이 나오면 두 초기 안내를 생략하며 Tell 중단은 앞선 부분효과와 이전 phase를 보존한다.
TEST_CASE(NativeMainMenuState_StartupQuestionSuppressesTipsAndTellFailureDoesNotCommit) {
    MainMenuScene question;question.startupResult=1;question.Run();CHECK(!question.first && question.startups==1 && std::find(question.trace.begin(),question.trace.end(),"updated")==question.trace.end());
    MainMenuScene stopped;stopped.stopTips=true;bool rejected{};try { stopped.Run(); }catch (const MainMenuStop&) { rejected=true; }
    CHECK(rejected && stopped.first==1 && stopped.local.State().phase[0]==4 && stopped.local.State().current[0]==3 && !stopped.local.State().next[0] && !stopped.current[0] && !stopped.pending[0] && !stopped.editor);
    CHECK(std::find(stopped.trace.begin(),stopped.trace.end(),"hud2")==stopped.trace.end());
}

// intro0은 세 menuWindow를 지운 뒤 immediate39/introPending1만 예약하고 일반 메뉴 prefix를 실행하지 않는다.
TEST_CASE(NativeMainMenuState_IntroSelectionUsesActualImmediateRequestWithoutMenuSuccess) {
    MainMenuScene scene;scene.intro=0;auto hooks=scene.Hooks();hooks.request=[&](auto selected) { CHECK(selected==39);scene.local.State().immediate[0]=selected; };
    CHECK(InstallNativeMainMenuState(scene.Globals(),hooks)==LocalMissionParentResult::Commit);
    CHECK(scene.local.State().immediate[35]==1 && scene.local.State().immediate[0]==39 && !scene.local.State().immediate[33] && !scene.local.State().current[33] && !scene.local.State().next[33] && scene.local.State().phase[0]==4 && scene.local.State().current[0]==3 && scene.current[0]=='O' && scene.trace==std::vector<std::string>{"intro"});
}

// 앞선 Tell callback이 바꾼 현재 tellTips를 뒤에서 다시 읽어야 하며 진입 시 복사한 옵션을 사용하면 안 된다.
TEST_CASE(NativeMainMenuState_UserNoticeRechecksCurrentTipsAfterTellCallback) {
    MainMenuScene scene;auto hooks=scene.Hooks();const auto original=hooks.tell;
    hooks.tell=[&](auto name) { original(name);if (name=="UserNotUpdated") scene.tips=0; };
    CHECK(InstallNativeMainMenuState(scene.Globals(),hooks)==LocalMissionParentResult::Commit);
    CHECK(!scene.tips && !scene.first && scene.local.State().next[0]==4 && std::find(scene.trace.begin(),scene.trace.end(),"tips")!=scene.trace.end() && std::find(scene.trace.begin(),scene.trace.end(),"tell:Tips")==scene.trace.end());
}
