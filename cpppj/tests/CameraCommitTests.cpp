// 원문 적용의 두64BYTE·dirty 표·전역·호출 순서를 독립 Native 행에 대조한다.
#include "TestSupport.h"
#include "client/RawCameraCommit.h"
#include <bit>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>
#include <stdexcept>

using namespace netstorm::client; // 실제 Client 자료형과 같은 카메라 적용 본체를 실행한다.
namespace { // 원문 입력과 외부 경계만 준비하며 적용 기대값을 계산하지 않는다.
// Native 입력28열은 signed 입력과 전체DWORD를 함께 읽기 위해 넓은 정수로 보관한다.
using CommitInput=std::array<std::int64_t,28>;
// 실제 초기 입력은 Python Native 관찰의 기본 장면과 같은 독립 값이다.
CommitInput DefaultInput() { return {100,200,100,200,65536,65536,65536,16,11,32,22,0,15,128,0,896,700,1024,768,99,88,17,19,1,0,317,419,32768}; }
// 현재 DWORD 배열을 원문과 같은 손실 없는 콜론 구분자로 출력한다.
template<class Range> std::string Words(const Range& values) {
    std::string result;for (const auto value:values) { if (!result.empty()) result+=':';result+=std::to_string(static_cast<std::uint32_t>(value)); } // signed 입력도 하위32비트를 유지한다.
    return result;
}
// 입력·현재 상태와 명시 효과 경계를 같은 실제 자료형으로 준비한다.
struct CommitScene {
    ClientCameraGlobals camera; // Renderer/snapshot/target의 별도 현재 저장소다.
    std::vector<DirtyRegion> dirty; // Native 표의 같은 초기 입력과 장치가 등록한 후속 표다.
    std::uint32_t flags{},redraw{},worldRedraw{},mutation{}; // 현재 네 DWORD의 소유자다.
    std::int32_t denominator{},width{},height{}; // 시차 분모/논리 화면의 현재 signed 입력이다.
    RawCameraCommitViewport viewport; // 네 실제 viewport 전역이다.
    ScreenRect fullViewport{3,4,997,701}; // 다른 전역과 합치지 않는 전체 사각형 입력이다.
    ScreenPoint mouse; // 실제 커서 장치에 전달할 현재 정수 좌표다.
    std::array<std::uint32_t,3> after{}; // callback의 명시 새 목표/배율이다.
    std::array<bool,7> mutated{}; // 같은 명시 외부 입력을 첫 선택 위치에서 한 번만 수행한다.
    std::string events; // 실제 선택된 외부 호출의 인자/순서다.
    // Native 초기값을 넣을 뿐 줌/이동/확장 결과는 만들지 않는다.
    explicit CommitScene(const CommitInput& input) {
        const auto number=[&](std::size_t i) { return static_cast<std::uint32_t>(input[i]); }; // 원문 DWORD 입력 비트다.
        const auto integer=[&](std::size_t i) { return std::bit_cast<std::int32_t>(number(i)); }; // 같은 입력의 signed 해석이다.
        const auto bits=[](float value) { return std::bit_cast<std::uint32_t>(value); }; // 명시 초기 float만 직렬화한다.
        camera.RendererView()=std::bit_cast<RawCameraViewState>(std::array<std::uint32_t,16>{bits(1.25F),bits(2.5F),16,11,number(7),number(8),number(4),number(0),number(1),bits(16),bits(11),bits(16),bits(11),bits(1),bits(100),bits(200)});
        camera.Snapshot()=std::bit_cast<RawCameraViewState>(std::array<std::uint32_t,16>{bits(9),bits(8),32,22,number(9),number(10),number(6),75,85,bits(32),bits(22),bits(16),bits(11),bits(1),bits(75),bits(85)});
        camera.Target().limits={480,490};camera.Target().pixels={integer(2),integer(3)};camera.Target().zoom=number(5);flags=number(11);denominator=integer(12);
        viewport={integer(13),integer(14),integer(15),integer(16)};width=integer(17);height=integer(18);mouse={integer(19),integer(20)};
        redraw=number(21);worldRedraw=number(22);mutation=number(24);after={number(25),number(26),number(27)};
        const auto profile=number(23);if (!profile) return;
        for (std::int32_t i=0;i<(profile==4 ? 100 : 4);++i) { // 원문 입력 표만 공급하고 이동 결과는 계산하지 않는다.
            const ScreenRect rect=profile==3 ? ScreenRect{-2147483647-1+i,2147483647-i,17+i,-33-i} : profile==4 ? ScreenRect{i,i+1,i+10,i+20} : ScreenRect{10+i,20+i,30+i,40+i};
            dirty.push_back({rect,static_cast<std::uint32_t>(profile==4 ? i%8 : i+(profile==2 ? 4 : 0))});
        }
    }
    // 원문 외부 호출이 관측한 하위DWORD 인자를 기록한다.
    template<class... T> void Event(char code,T... values) { if (!events.empty()) events+=';';events+=code;if constexpr (sizeof...(T)>0) events+=':'+Words(std::array<std::uint32_t,sizeof...(T)>{static_cast<std::uint32_t>(values)...}); }
    // 계산 없이 Native와 같은 명시 callback 입력만 현재 자료에 쓴다.
    void Mutate(std::uint32_t phase) {
        if (mutation!=phase || mutated[phase]) return;mutated[phase]=true;
        if (phase<=3) { camera.Target().pixels={std::bit_cast<std::int32_t>(after[0]),std::bit_cast<std::int32_t>(after[1])};camera.Target().zoom=after[2]; }
        else if (phase==4) dirty.push_back({{100,110,120,130},2});
        else if (phase==5) camera.Snapshot().baseCellPixels={63,31};
        else if (phase==6) { viewport={7,9,333,444};flags=18; }
    }
    // 실제 적용 함수가 같은 현재 자료와 같은 표를 빌리게 한다.
    RawCameraCommitState State() { return {camera.RendererView(),camera.Snapshot(),camera.Target(),redraw,worldRedraw,dirty}; }
    // 복사하지 않은 현재 전역 주소를 외부 효과 뒤에도 읽게 한다.
    RawCameraCommitGlobals Globals() const { return {flags,denominator,width,height,viewport,fullViewport,mouse}; }
    // 기존 장치와 dirty 추가의 호출 경계만 관찰한다. 원문 본체의 투영/표 이동을 대체하지 않는다.
    RawCameraCommitHooks Hooks() {
        RawCameraCommitHooks hooks;hooks.lockCursor=[this](auto skip) { Event('L',skip);Mutate(1); };hooks.unlockCursor=[this] { Event('U'); };
        hooks.redrawRoots=[this] { const auto& v=camera.RendererView();const auto& s=camera.Snapshot();Event('R',v.zoom,v.pixels[0],v.pixels[1],s.zoom,s.pixels[0],s.pixels[1],redraw,worldRedraw);Mutate(2);Mutate(5); };
        hooks.scroll=[this](auto x,auto y,auto w,auto h,auto dx,auto dy) { Event('S',x,y,w,h,dx,dy);Mutate(3);Mutate(6); };
        hooks.scrollBackground=[this](auto x,auto y,auto w,auto h,auto dx,auto dy) { Event('P',x,y,w,h,dx,dy); };
        hooks.addDirty=[this](ScreenRect r,auto f) { Event('D',r.left,r.top,r.right,r.bottom,f); };hooks.addBackgroundDirty=[this](ScreenRect r) { Event('B',r.left,r.top,r.right,r.bottom); };
        hooks.updateMouse=[this](ScreenPoint point) { Event('M',point.x,point.y);Mutate(4); };hooks.assertion=[this](auto,auto line) { Event('A',line); };return hooks;
    }
    // 전체 현재 변경 표를 원문5DWORD 행으로 직렬화한다.
    std::string DirtyWords() const {
        std::string result;for (const auto& entry:dirty) { if (!result.empty()) result+='|';result+=Words(std::array{static_cast<std::uint32_t>(entry.rect.left),static_cast<std::uint32_t>(entry.rect.top),static_cast<std::uint32_t>(entry.rect.right),static_cast<std::uint32_t>(entry.rect.bottom),entry.flags}); } // 보존 항목과 미사용 고비트도 비교한다.
        return result.empty() ? "-" : result;
    }
};
}
// 원문106행의 두64BYTE·목표/전체dirty·현재 표/전역·외부 호출을 전부 대조한다.
TEST_CASE(CameraCommit_MatchesNativeViewsDirtyAndEffects) {
    std::ifstream file(NETSTORM_CAMERA_COMMIT_FIXTURE);CHECK(file.good());std::string line;std::size_t count{};
    while (std::getline(file,line)) { // 원문 관측마다 독립 초기 상태를 공급한다.
        if (line.empty() || line.front()=='#') continue;std::istringstream row(line);CommitInput input{};
        for (auto& value:input) row>>value; // 결과와 섞지 않고28개 입력만 읽는다.
        std::string renderer,snapshot,target,dirty,globals,events;row>>renderer>>snapshot>>target>>dirty>>globals>>events;CHECK(!row.fail());if (row.fail()) continue;
        CommitScene scene(input);CommitCameraTarget(scene.State(),scene.Globals(),scene.Hooks());const auto& goal=scene.camera.Target();
        CHECK(Words(std::bit_cast<std::array<std::uint32_t,16>>(scene.camera.RendererView()))==renderer);CHECK(Words(std::bit_cast<std::array<std::uint32_t,16>>(scene.camera.Snapshot()))==snapshot);
        CHECK(Words(std::array{static_cast<std::uint32_t>(goal.limits[0]),static_cast<std::uint32_t>(goal.limits[1]),static_cast<std::uint32_t>(goal.pixels[0]),static_cast<std::uint32_t>(goal.pixels[1]),goal.zoom,scene.redraw})==target);
        CHECK(scene.DirtyWords()==dirty);CHECK(Words(std::array{scene.flags,static_cast<std::uint32_t>(scene.denominator),static_cast<std::uint32_t>(scene.viewport.x),static_cast<std::uint32_t>(scene.viewport.y),static_cast<std::uint32_t>(scene.viewport.width),static_cast<std::uint32_t>(scene.viewport.height),static_cast<std::uint32_t>(scene.width),static_cast<std::uint32_t>(scene.height),static_cast<std::uint32_t>(scene.mouse.x),static_cast<std::uint32_t>(scene.mouse.y),scene.worldRedraw})==globals);
        if (scene.events!=events) std::printf("카메라 적용 행 %zu 기대=%s 실제=%s\n",count,events.c_str(),scene.events.c_str());CHECK(scene.events==events);++count;
    }
    CHECK(count==106);
}
// 화면 이동 효과가 뒤 목표를 바꾸어도 이동량은 앞에서 보존하고 두 현재 setter는 새 목표를 읽는다.
TEST_CASE(CameraCommit_CurrentTargetAfterScreenEffect) {
    auto input=DefaultInput();input[2]=317;input[3]=419;input[24]=3;input[25]=701;input[26]=905;CommitScene scene(input);
    CommitCameraTarget(scene.State(),scene.Globals(),scene.Hooks());CHECK(scene.events.find("S:128:0:896:700:217:219")!=std::string::npos);
    CHECK((scene.camera.RendererView().pixels==std::array<std::int32_t,2>{701,905}));CHECK(scene.camera.Snapshot().pixels==scene.camera.RendererView().pixels);
    CHECK(scene.camera.RendererView().zoom==65536 && scene.camera.Target().zoom==32768 && scene.redraw==1 && scene.worldRedraw==19);
}
// 같은 위치 경로는 화면 이동만 요구하고 배율 변경 경로는 두 Gump 호출만 요구한다.
TEST_CASE(CameraCommit_SelectedHooksAndSeparateZoomBranch) {
    CommitScene same(DefaultInput());std::size_t scrolls{};RawCameraCommitHooks hooks;hooks.scroll=[&](auto,auto,auto,auto,auto x,auto y) { CHECK(x==0 && y==0);++scrolls; };
    CommitCameraTarget(same.State(),same.Globals(),hooks);CHECK(scrolls==3 && same.redraw==17 && same.worldRedraw==19 && same.camera.Snapshot().pixels==same.camera.RendererView().pixels);
    auto input=DefaultInput();input[5]=32768;CommitScene zoom(input);std::size_t redraws{};RawCameraCommitHooks onlyZoom;onlyZoom.redrawRoots=[&] { ++redraws; };
    CommitCameraTarget(zoom.State(),zoom.Globals(),onlyZoom);CHECK(redraws==2 && zoom.redraw==17 && zoom.worldRedraw==1);
    CHECK((zoom.camera.RendererView().cellPixels==std::array<std::int32_t,2>{8,5}));CHECK((zoom.camera.Snapshot().cellPixels==std::array<std::int32_t,2>{16,11}));
}
// IDIV 오류는 실제 시차 분기에서만 거부하고 DIB/일반 경로는 같은 분모0을 읽지 않는다.
TEST_CASE(CameraCommit_OnlySelectedParallaxDivisionCanFail) {
    auto input=DefaultInput();input[12]=0;CommitScene normal(input);CommitCameraTarget(normal.State(),normal.Globals(),normal.Hooks());
    input[11]=18;CommitScene dib(input);CommitCameraTarget(dib.State(),dib.Globals(),dib.Hooks());
    input[11]=16;CommitScene parallax(input);bool failed{};try { CommitCameraTarget(parallax.State(),parallax.Globals(),parallax.Hooks()); }catch (const std::domain_error&) { failed=true; }
    CHECK(failed && parallax.events.find("P:")==std::string::npos && parallax.camera.Snapshot().pixels[0]==75);
}

//433aa0 부모부터 같은497960까지 이어진 독립 원문 관측을 확인한다.
#include "CameraImmediateTests.inc"
