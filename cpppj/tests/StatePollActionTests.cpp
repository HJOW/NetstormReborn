// Native Poll과 호스트 지연 큐의 같은 Tick 경계를 검사한다. 게임·패킷·승패 장치는 실행하지 않는다.
#include "TestSupport.h"
#include "client/State.h"
#include "o/RawLocalMissionState.h"
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::o;
namespace { // 실제 RawLocalMissionState가 참조할 독립 원문 입력이다.
struct HostPollNativeScene {
    std::array<LocalMissionStateBlock,5> blocks{}; // 같은 old/phase/immediate/current/next 전체144BYTE를 빌린다.
    bool server{true},boss{true},battle{},post{true},saved{},mission{true},simul{},meta{};PlayerWorldState player; // 단독 미션 이외 상태는 선택하지 않는다.
    std::uint32_t zone{},ticks{},source{},cd{},refresh{},rebuild{};std::uint8_t title{},battleByte{};double connect{},activity{},game{},wall{}; // Native 부모의 서로 다른 현재 전역이다.
    LocalMissionStateStorage Storage() { return {blocks[0],blocks[1],blocks[2],blocks[3],blocks[4]}; } // 값 사본을 만들지 않는 동일 저장소 뷰다.
    LocalMissionStateGlobals Globals() { return {server,boss,battle,post,saved,mission,simul,meta,player,zone,ticks,source,cd,blocks[2][33],refresh,rebuild,title,battleByte,connect,activity,game,wall}; } // 모든 참조는 Scene보다 짧은 실제 분배기에 빌린다.
};
}
// 원문 State1 조기 ret가 쓴7과 새 호스트 설치는 두 Poll 사이에서 같은 phase를 유지한다.
TEST_CASE(StatePollAction_ActualNativeEarlyReturnDefersNewMissionBody) {
    State pending;HostPollNativeScene scene;scene.blocks[1].fill(99);scene.blocks[4][0]=1;const auto previous=scene.blocks[1];std::uint32_t parents{},loads{},polls{};
    LocalMissionStateHooks hooks;hooks.logState=[](auto selected) { CHECK(selected==1 || selected==7); };hooks.parentState=[&](auto selected) {
        CHECK(selected==1 && loads==0);++parents;scene.blocks[4][0]=7;pending.Post({"LoadMissionWorld","GuideFort"});return LocalMissionParentResult::ReturnWithoutCommit;
    };
    RawLocalMissionState native(OriginalEdition::Patch1078,scene.Storage(),scene.Globals(),std::move(hooks));
    auto action=pending.TakeAfterPoll([&] { ++polls;native.Poll(); });
    CHECK(!action && pending.Pending() && parents==1 && loads==0 && polls==1 && scene.blocks[1]==previous && scene.blocks[3][0]==1 && scene.blocks[4][0]==7);
    action=pending.TakeAfterPoll([&] { ++polls;CHECK(scene.blocks[4][0]==7 && scene.blocks[1]==previous); }); // 실제 호스트 큰 부모 분배기는7을 소비하지 않고 기존 요청만 호스트 설치에 인계한다.
    CHECK(action && action->command=="LoadMissionWorld" && action->argument=="GuideFort" && !pending.Pending() && polls==2);
    native.PollStarting([&] { ++loads;CHECK(parents==1 && scene.blocks[3][0]==7 && scene.blocks[1]==previous); }); // 설치 장치는 명시 외부 입력이며 RawState의 복사/로그/공통 확정은 실제 본체다.
    CHECK(loads==1 && parents==1 && scene.blocks[1][0]==7 && scene.blocks[0]==previous && scene.blocks[3][0]==0 && scene.blocks[4][0]==0);
}
// 이미 대기하던 같은 요청은 Poll 복귀 뒤 원래 인자로 한 번만 소비하며 꽉 찬 큐의 Post는 무시한다.
TEST_CASE(StatePollAction_PreexistingRequestKeepsOriginalSingleSlotPolicy) {
    State pending;pending.Post({"MissionRestart","old"});std::uint32_t polls{};
    const auto action=pending.TakeAfterPoll([&] { ++polls;pending.Post({"Tell","ignored"}); });
    CHECK(action && action->command=="MissionRestart" && action->argument=="old" && polls==1 && !pending.Pending());
    CHECK(!pending.TakeAfterPoll([&] { ++polls; }) && polls==2);
}
// Guide 해제가 기존 요청을 취소하고 다른 요청을 넣어도 같은 주소/명령 이름을 항목 동일성으로 쓰지 않는다.
TEST_CASE(StatePollAction_CancelAndReplaceWaitsUntilNextPollEvenWithSameCommand) {
    State pending;pending.Post({"LoadMissionFort","former"});
    CHECK(!pending.TakeAfterPoll([&] { CHECK(pending.Cancel("LoadMissionFort"));pending.Post({"LoadMissionFort","replacement"}); }));
    CHECK(pending.Pending());const auto action=pending.TakeAfterPoll([] {});
    CHECK(action && action->command=="LoadMissionFort" && action->argument=="replacement" && !pending.Pending());
    pending.Post({"Tell","keep"});const auto retained=pending.TakeAfterPoll([&] { CHECK(!pending.Cancel("LoadMissionFort")); });CHECK(retained && retained->argument=="keep");
}
// Native 부분 실패는 이미 예약한 호스트 요청을 꺼내거나 되돌리지 않고 호출자에게 그대로 전파한다.
TEST_CASE(StatePollAction_PollFailureAndMissingDevicePreserveQueue) {
    State pending;bool failed{};try { pending.TakeAfterPoll([&] { pending.Post({"LoadMissionWorld","partial"});throw std::runtime_error("선택한 Poll 중단"); }); }catch (const std::runtime_error& error) { failed=std::string_view(error.what())=="선택한 Poll 중단"; }
    CHECK(failed && pending.Pending());bool missing{};try { pending.TakeAfterPoll({}); }catch (const std::invalid_argument&) { missing=true; }CHECK(missing && pending.Pending());
    const auto action=pending.TakeAfterPoll([] {});CHECK(action && action->argument=="partial" && !pending.Pending());
}
