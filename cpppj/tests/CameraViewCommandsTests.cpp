// 공개 CenterOn의 현재 TYPE 재조회·Carrier preview와 Reset의 실제 로딩 깊이 공유를 검증한다.
#include "RawSceneSupport.h"
#include "client/CameraHome.h"
#include "client/Screen.h"
#include "o/RawSquidSelection.h"
#include <stdexcept>
#include <vector>

using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 같은 실제 SID 슬롯에 최소 자산/좌표만 설치한다. 기대 좌표는 아래 원문 계약의 고정 관측이다.
struct CameraViewScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false};std::vector<RiftTypeRecord> types{188}; // 실제10.78 풀/TYPE의 장기 저장소다.
    Sid target;CameraHomeState state;std::uint32_t depth{},regulars{}; // 실제 대상·공유005c89b8·관측한 Regular 수다.
    std::vector<std::string> trace; // 화면 적용과 preview의 실제 호출 순서다.
    // 사제+Carrier genus와 전체 footX/footY를 가진 같은 raw 자산을 준비한다.
    CameraViewScene() {
        target=pool.Allocate(2);auto raw=pool.AllocatedBytes(target);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=70;raw[34]=1;Put(raw,14,std::bit_cast<std::uint32_t>(10.0F));Put(raw,18,std::bit_cast<std::uint32_t>(20.0F));types[70].footX=3;types[70].footY=2;types[70].flags2=0x210000;types[71].footX=9;types[71].footY=9;
    }
    // 실제 StartSelectionContentPreview를 같은 원문 상태/소유 저장소에 연결한다.
    CameraHomeHooks Hooks() {
        CameraHomeHooks hooks;hooks.suppressionDepth=[this] { return depth; };hooks.setSuppressionDepth=[this](auto value) { depth=value;trace.push_back("depth"+std::to_string(value)); };
        hooks.contentPreview=[this](Sid sid) { trace.push_back("preview");StartSelectionContentPreview(pool,sid,depth,[this](Sid parent,std::uint32_t event,float payload) { CHECK(parent==target && event==0x491 && std::bit_cast<std::uint32_t>(payload)==0x3e4ccccd);++regulars;trace.push_back("regular"); },[] { throw std::logic_error("정상 Carrier의 예상 밖 Form 진단"); }); };return hooks;
    }
};
}
// 좌표 적용 후 flags2를 다시 읽으며 flags1과 이전 타입 사본으로 preview를 고르지 않는다.
TEST_CASE(CameraViewCommands_CurrentTypeAfterApplyAndActualPreview) {
    CameraViewScene scene;std::vector<ScreenPoint> applied; // 실제 장치의 최종 원점만 관측한다.
    CameraHome logic(scene.pool,scene.types,scene.state,[&](auto x,auto y) { applied.push_back({x,y});scene.trace.push_back("apply"); },scene.Hooks());
    logic.CenterOn(scene.target);CHECK(applied.size()==1 && applied[0].x==-208 && applied[0].y==-42);CHECK(scene.trace==std::vector<std::string>({"apply","preview","regular"}));CHECK(scene.regulars==1 && scene.state.requests==1 && scene.state.revision==1);
    scene.trace.clear();scene.types[70].flags2=0;scene.types[70].flags1=0x30000;logic.CenterOn(scene.target);CHECK(scene.trace==std::vector<std::string>({"apply"}) && scene.regulars==1); // flags1의 같은 마스크는 preview 조건이 아니다.
    scene.trace.clear();scene.types[70].flags2=0x30000;
    CameraHome changed(scene.pool,scene.types,scene.state,[&](auto,auto) { scene.trace.push_back("apply");scene.pool.AllocatedBytes(scene.target)[10]=71; },scene.Hooks());changed.CenterOn(scene.target);CHECK(scene.trace==std::vector<std::string>({"apply"}) && scene.regulars==1); // 적용 뒤 타입71은 Carrier가 아니다.
    scene.trace.clear();scene.types[71].flags2=0x20000;changed.CenterOn(scene.target);CHECK(scene.trace==std::vector<std::string>({"apply","preview","regular"}) && scene.regulars==2);
}
// 명시 SID0/free/dead는 다른 대상 재선택이나 preview 없이 돌아오며 void만으로는 제외하지 않는다.
TEST_CASE(CameraViewCommands_NullFreeDeadAndPreviewGuards) {
    CameraViewScene scene;std::uint32_t applied{};CameraHome logic(scene.pool,scene.types,scene.state,[&](auto,auto) { ++applied; },scene.Hooks());logic.CenterOn({});CHECK(applied==0 && scene.regulars==0);
    auto raw=scene.pool.AllocatedBytes(scene.target);raw[11]=1;logic.CenterOn(scene.target);raw[11]=2;logic.CenterOn(scene.target);CHECK(applied==0 && scene.regulars==0);
    raw[11]=4;raw[40]=8;logic.CenterOn(scene.target);CHECK(applied==1 && scene.regulars==0);raw[40]=0;scene.depth=3;logic.CenterOn(scene.target);CHECK(applied==2 && scene.regulars==0);scene.depth=0;logic.CenterOn(scene.target);CHECK(applied==3 && scene.regulars==1);
}
// Reset의 실제 증가/감소를 같은 로딩 전역에 적용하여 Carrier Regular를 억제하고 현재 값으로 감소한다.
TEST_CASE(CameraViewCommands_ResetSharesCurrentLoadingDepth) {
    CameraViewScene scene;scene.depth=4;scene.state.suppressionDepth=99;CameraHome logic(scene.pool,scene.types,scene.state,[&](auto,auto) { CHECK(scene.depth==5 && scene.state.suppressionDepth==5);scene.trace.push_back("apply"); },scene.Hooks());
    logic.Reset();CHECK(scene.depth==4 && scene.state.suppressionDepth==4 && scene.regulars==0);CHECK(scene.trace==std::vector<std::string>({"depth5","apply","preview","depth4"}));
    scene.trace.clear();CameraHome changed(scene.pool,scene.types,scene.state,[&](auto,auto) { scene.depth=9;scene.trace.push_back("apply"); },scene.Hooks());changed.Reset();CHECK(scene.depth==8 && scene.state.suppressionDepth==8 && scene.regulars==0); // 원문 --는 적용 전 깊이 사본이 아니라 현재 전역을 감소한다.
}
// 화면 적용 예외 후에도 실제 로딩 소유자와 UI state 모두 증가 전 깊이로 복구한다.
TEST_CASE(CameraViewCommands_ResetFailureRestoresActualDepth) {
    CameraViewScene scene;scene.depth=3;CameraHome logic(scene.pool,scene.types,scene.state,[](auto,auto) { throw std::runtime_error("명시 카메라 장치 오류"); },scene.Hooks());bool failed{};
    try { logic.Reset(); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && scene.depth==3 && scene.state.suppressionDepth==3 && scene.regulars==0);
}
