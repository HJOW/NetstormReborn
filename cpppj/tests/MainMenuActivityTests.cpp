// 창/미션 실행 없이 실제 원문 활동 관측과 옵션/부분 효과를 대조한다.
#include "TestSupport.h"
#include "client/MainMenuActivity.h"
#include "client/ClientMain.h"
#include "client/ClientLocalStateGlobals.h"
#include <bit>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace netstorm::client;
// 독립 CPU 기대값1944행의 double 비트·포인터·helper 호출 순서 전체를 재생한다.
TEST_CASE(MainMenuActivity_IndependentNative1944) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();std::ifstream input(root/"cpppj/tests/fixtures/main-menu-activity-x86.tsv");CHECK(input.good());std::string line;std::size_t count{};
    while (std::getline(input,line)) { //원문 도구가 저장한 각 명시 입력을 제품 계산 기대값 없이 재생한다.
        if (line.empty() || line.front()=='#') continue;std::istringstream row(line);std::uint32_t enabled{},html{},popup{},modal{};int x{},y{},outX{},outY{};std::uint64_t deadline{},first{},second{},out{};std::string expected;
        row>>enabled>>html>>popup>>x>>y>>modal>>deadline>>first>>second>>out>>outX>>outY>>expected;CHECK(!row.fail());MainMenuActivity state{std::bit_cast<double>(deadline),{0,0}};ScreenPoint pointer{x,y};const auto instances=std::bit_cast<std::int32_t>(html);std::string trace;std::size_t calls{};
        MainMenuActivityHooks hooks;hooks.popup=[&] { trace+='p';return popup!=0; };hooks.now=[&] { trace+='n';return std::bit_cast<double>(calls++ ? second : first); };hooks.queue=[&](auto command,auto a,auto b) { CHECK(command==0x020000c0U && a=="DemoStormRages" && b=="DemoStormRages");trace+='q'; };hooks.kill=[&] { trace+='k'; };
        IdleMainMenuActivity(state,enabled,instances,pointer,modal,1,hooks);CHECK(std::bit_cast<std::uint64_t>(state.deadline)==out && state.pointer.x==outX && state.pointer.y==outY && (trace.empty() ? "-" : trace)==expected);++count;
    }
    CHECK(count==1944);
}
// 상태 쓰기 뒤 helper 실패,현재 모달 재조회,동일 프레임 시계 도약과 활동 사건 release를 확인한다.
TEST_CASE(MainMenuActivity_LazyCurrentAndPartialEffects) {
    MainMenuActivity state{4,{7,8}};std::uint32_t enabled{},modal=1;std::int32_t html{};ScreenPoint pointer{9,10};IdleMainMenuActivity(state,enabled,html,pointer,modal,1,{});CHECK(state.deadline==4 && state.pointer.x==7);
    enabled=1;html=1;int calls{};MainMenuActivityHooks hooks;hooks.now=[&] { ++calls;modal=2;pointer={11,12};return 5.0; };IdleMainMenuActivity(state,enabled,html,pointer,modal,1,hooks);CHECK(calls==1 && state.deadline==50 && state.pointer.x==11 && state.pointer.y==12); //HTML 양수는 팝업을 생략하고 now가 바꾼 현재 모달/포인터를 뒤에서 읽는다.
    html=0;modal=1;state={0,{11,12}};std::string trace;hooks.popup=[&] { trace+='p';return false; };hooks.now=[&] { trace+='n';return 50.0; };hooks.queue=[&](auto,auto,auto) { trace+='q'; };hooks.kill={};bool failed{};
    try { IdleMainMenuActivity(state,enabled,html,pointer,modal,1,hooks); }catch (const std::invalid_argument&) { failed=true; }CHECK(failed && trace=="pnq" && state.deadline==0); //실제 queue가 정상 복귀한 선행 효과를 Kill 장치 누락이 되돌리지 않는다.
    CHECK(HandleMainMenuActivity(state,kMainMenuActivityEvent|InputCode::kRelease,[] { return 10.0; }) && state.deadline==55 && state.pointer.x==11);CHECK(!HandleMainMenuActivity(state,0,{}));
}
// 실제 동기 Config 변경과 장기 DWORD의 누락/음수 비트를 확인하고 창/파일 저장은 하지 않는다.
TEST_CASE(MainMenuActivity_ClientCurrentOptionOwner) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();ClientOptions options;options.gameDirectory=root/"originals";options.noAudio=true;Client client(options);auto& value=client.LocalStateGlobals()->AutoDemo();CHECK(value==0);value=0x12345678U;client.Configuration().Set("tellTips","1");CHECK(value==0x12345678U);client.Configuration().Set("autoDemo","-1");CHECK(value==0xffffffffU);client.Configuration().Set("autoDemo","0");CHECK(value==0);
}
