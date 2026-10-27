// 보호 PE의 실제 가장자리/카메라 목표 결과와 같은 입력을 직접 대조한다.
#include "TestSupport.h"
#include "client/RawEdgeScroll.h"
#include <bit>
#include <fstream>
#include <sstream>
#include <string>
using namespace netstorm::client;
// 입력21열과 실제 방향/시간/목표/전체 전역 및 조회 순서를 대조하며 C++ 결과로 기대값을 만들지 않는다.
TEST_CASE(EdgeScroll_MatchesNativeInputAndCameraTarget) {
    std::ifstream file(NETSTORM_EDGE_SCROLL_FIXTURE);CHECK(file.good());std::string line;std::size_t count=0;
    while (std::getline(file,line)) { // 원문 관찰 행마다 독립 현재 상태와 실제 외부 장치를 준비한다.
        if (line.empty() || line.front()=='#') continue;
        std::istringstream row(line);std::uint32_t flags{},enabled{},dialog{},left{},shift{},menu{},popup{},revision{},expectedFlags{},expectedRevision{};std::uint64_t sinceBits{},wallBits{},expectedSince{};std::int32_t borderX{},borderY{},x{},y{},width{},height{},speed{},cameraX{},cameraY{},targetX{},targetY{};std::string directions,targets,expectedEvents;
        row>>flags>>sinceBits>>enabled>>borderX>>borderY>>dialog>>x>>y>>width>>height>>left>>shift>>menu>>popup>>wallBits>>speed>>cameraX>>cameraY>>targetX>>targetY>>revision>>expectedFlags>>expectedSince>>directions>>targets>>expectedRevision>>expectedEvents;CHECK(!row.fail());if (row.fail()) continue;
        double since=std::bit_cast<double>(sinceBits);RawCameraViewState view;view.pixels={cameraX,cameraY};RawCameraTargetState target;target.pixels={targetX,targetY};std::string events;
        const auto record=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; // 실제 선택된 호출만 순서대로 기록한다.
        RawEdgeScrollHooks hooks;
        hooks.poll=[&](std::uint32_t code) { record("P:"+std::to_string(code));return InputEvent{code|((code==InputCode::kLeftButton ? left : shift) ? InputCode::kRelease : 0U),x,y}; };
        hooks.menu=[&](std::uint32_t serial) { record("M:"+std::to_string(serial));return menu; };
        hooks.blockingPopup=[&] { record("B");return popup!=0; };hooks.closePopups=[&] { record("C"); };
        hooks.request=[&](std::int32_t dx,std::int32_t dy) { record("D:"+std::to_string(static_cast<std::uint32_t>(dx))+":"+std::to_string(static_cast<std::uint32_t>(dy)));RequestCameraDelta(view,target,height,dx,dy,revision); };
        const auto result=ProcessEdgeScroll({flags,since},{enabled,dialog,731,borderX,borderY,width,height,x,y,speed,std::bit_cast<double>(wallBits)},hooks);
        CHECK(flags==expectedFlags && std::bit_cast<std::uint64_t>(since)==expectedSince && revision==expectedRevision);
        CHECK(std::to_string(std::bit_cast<std::uint32_t>(result[1]))+":"+std::to_string(std::bit_cast<std::uint32_t>(result[0]))==directions);
        CHECK(std::to_string(static_cast<std::uint32_t>(target.pixels[0]))+":"+std::to_string(static_cast<std::uint32_t>(target.pixels[1]))==targets);
        CHECK((events.empty() ? "-" : events)==expectedEvents);++count;
    }
    CHECK(count==135);
}
// 목표 요청은 현재 view를 보존하고 이전 목표 대신 같은 현재 원점에 델타를 더한다.
TEST_CASE(EdgeScroll_RequestOnlyChangesTargetAndWrapsCounter) {
    RawCameraViewState view;view.pixels={100,200};RawCameraTargetState target;target.pixels={1000,2000};std::uint32_t revision=0xffffffffU;
    RequestCameraDelta(view,target,768,2,-3,revision);CHECK((view.pixels==std::array<std::int32_t,2>{100,200}));CHECK((target.pixels==std::array<std::int32_t,2>{102,197}));CHECK(revision==0);
}
// 여러 프레임이 같은 flags/최초 시각을 공유하고 중앙 복귀 때만 가속이 해제되는지 확인한다.
TEST_CASE(EdgeScroll_ContinuousHoverKeepsTimeAndRestartsAfterCenter) {
    std::uint32_t flags=0x20;double since=0;RawEdgeScrollInput input{1,0,731,0,0,1024,768,1023,384,30,10.0};std::int32_t delta{};
    RawEdgeScrollHooks hooks;hooks.poll=[](auto code) { return InputEvent{code|InputCode::kRelease,0,0}; };hooks.menu=[](auto) { return 0U; };hooks.blockingPopup=[] { return false; };hooks.closePopups=[] {};hooks.request=[&](auto x,auto) { delta=x; };
    ProcessEdgeScroll({flags,since},input,hooks);CHECK(delta==2 && since==10.0 && flags==0x24);
    input.wall=10.25;ProcessEdgeScroll({flags,since},input,hooks);CHECK(delta==9 && since==10.0);
    input.x=512;ProcessEdgeScroll({flags,since},input,hooks);CHECK(since==0.0);
    input.x=1023;input.wall=11.0;ProcessEdgeScroll({flags,since},input,hooks);CHECK(delta==2 && since==11.0 && flags==0x24);
}
