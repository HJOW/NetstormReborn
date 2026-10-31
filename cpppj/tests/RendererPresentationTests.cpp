// 원문 출력 요청과 실제 dirty 그리기를 분리해 화면 이동 뒤 불필요한 전체 재합성을 검출한다.
#include "TestSupport.h"
#include "client/Renderer.h"
#include "client/RawCameraCommit.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <string>

namespace { // 독립 원문 출력에서 화면 업데이트 호출만 읽는다.
// 구분된 원문 호출 중 U의 네 인자를 Renderer 출력과 같은 문자열로 묶는다.
std::string Updates(std::string text) {
    std::istringstream input(text);std::string event,result;
    while (std::getline(input,event,';')) { // 화면 잠금/진단 장치는 이 메모리 표면 검사에서 실행하지 않는다.
        if (event.starts_with("U:")) { if (!result.empty()) result+=';';result+=event; }
    }
    return result;
}
// 동일 화면 사각형의 실제 출력 호출을 직렬화한다.
std::string Update(netstorm::client::ScreenRect rect) {
    return "U:"+std::to_string(rect.left)+":"+std::to_string(rect.top)+":"+std::to_string(rect.right)+":"+std::to_string(rect.bottom);
}
}

// PE의 요청/첫 flags/빈 표/세 화면비 288입력과 현재 제품의 그리기·출력 소비를 대조한다.
TEST_CASE(RendererPresentation_NativeDirtyRequestsAndFirstFlags) {
    using namespace netstorm::client;
    std::ifstream input(NETSTORM_RENDERER_PRESENT_DIRTY_FIXTURE);CHECK(input.good());std::string line;std::size_t rows{};
    while (std::getline(input,line)) { // 기대값은 제품 계산으로 만들지 않고 보호 PE의 고정 출력만 읽는다.
        if (line.empty() || line.front()=='#') continue;
        std::istringstream fields(line);std::array<std::uint32_t,17> words{};std::string events;
        for (auto& word:words) fields>>word; // 앞8입력과 최종9DWORD를 분리하지 않고 정확히 읽는다.
        fields>>events;CHECK(!fields.fail());++rows;
        Renderer renderer(static_cast<int>(words[0]),static_cast<int>(words[1]));std::vector<std::uint8_t> pixels(static_cast<std::size_t>(words[0])*words[1]);
        renderer.Draw(pixels,static_cast<int>(words[0]));renderer.Present([](ScreenRect) {});
        renderer.CameraDirtyEntries()={{{2,3,7,8},words[6]}};renderer.ClearDirty(); // count0도 첫 flags 물리 바이트를 지우지 않는 원문 입력이다.
        if (words[5]) renderer.CameraDirtyEntries().push_back({{2,3,7,8},words[6]});
        if (words[5]==2) renderer.CameraDirtyEntries().push_back({{20,30,40,50},words[7]});
        renderer.CameraRedraw()=words[2];renderer.BackgroundRedraw()=words[3];
        const auto drawn=renderer.Draw(pixels,static_cast<int>(words[0]));
        const auto expectedDraws=(words[5]>0 && !(words[6]&1U) ? 1U:0U)+(words[5]>1 && !(words[7]&1U) ? 1U:0U);
        CHECK(drawn.size()==expectedDraws); // 출력 전체화가 월드의 재수집/재합성을 늘리지 않아야 한다.
        CHECK(renderer.CameraRedraw()==words[2] && renderer.BackgroundRedraw()==words[3]);
        std::string actual;
        renderer.Present([&](ScreenRect rect) { // 원문은 두 요청을 지운 뒤 같은 표를 내보낸다.
            CHECK(renderer.CameraRedraw()==0 && renderer.BackgroundRedraw()==0);
            CHECK(renderer.CameraDirtyEntries().size()==((words[2] || words[3]) ? 1U:words[5]));
            if (!actual.empty()) actual+=';';actual+=Update(rect);
        });
        CHECK(actual==Updates(events));CHECK(renderer.CameraRedraw()==words[8] && renderer.BackgroundRedraw()==words[9]);
        CHECK(renderer.CameraDirtyEntries().empty());CHECK(renderer.Draw(pixels,static_cast<int>(words[0])).empty());renderer.Present([](ScreenRect) {});
    }
    CHECK(rows==288);
}

// 이동 후 기존 내부 픽셀은 보존하고 노출 띠만 그리면서 전체 화면을 출력한다.
TEST_CASE(RendererPresentation_ScrollKeepsInteriorAndPresentsFull) {
    using namespace netstorm::client;Renderer renderer(20,10);std::vector<std::uint8_t> pixels(200,7);std::vector<ScreenRect> collected,updated;
    renderer.Draw(pixels,20);renderer.Present([](ScreenRect) {});std::fill(pixels.begin(),pixels.end(),std::uint8_t{7});
    const RendererWorldDrawing world{[&](ScreenRect rect,std::uint32_t) { collected.push_back(rect);return 0U; },[](ScreenRect,std::uint32_t,std::uint32_t) {},
        [&](std::span<std::uint8_t> surface,int pitch,int,ScreenRect rect) {
            for (int y=rect.top;y<rect.bottom;++y) { // 실제 노출 띠 바깥의 스크롤 결과는 그대로 둔다.
                std::fill(surface.begin()+y*pitch+rect.left,surface.begin()+y*pitch+rect.right,std::uint8_t{9});
            }
        }};
    renderer.Invalidate({18,0,20,10});renderer.CameraRedraw()=1;renderer.Draw(pixels,20,0,&world);
    CHECK(collected.size()==1 && collected[0].left==18 && collected[0].right==20);CHECK(pixels[10]==7 && pixels[19]==9);
    renderer.Present([&](ScreenRect rect) { updated.push_back(rect); });
    CHECK(updated.size()==1 && Update(updated[0])=="U:0:0:20:10");CHECK(renderer.Draw(pixels,20,0,&world).empty());renderer.Present([](ScreenRect) {});
}

// 그리기에서 빈 영역으로 제한된 유효 항목도 원문의 출력 표에서 임의로 제거하지 않는다.
TEST_CASE(RendererPresentation_ClipsActualTableBeforePresent) {
    using namespace netstorm::client;Renderer renderer(20,10);std::vector<std::uint8_t> pixels(200);std::vector<ScreenRect> updates;
    renderer.Draw(pixels,20);renderer.Present([](ScreenRect) {});
    renderer.CameraDirtyEntries()={{{-2,-3,7,8},0},{{21,11,30,20},0}};
    const auto drawn=renderer.Draw(pixels,20);CHECK(drawn.size()==1 && Update(drawn[0])=="U:0:0:7:8");
    renderer.Present([&](ScreenRect rect) { updates.push_back(rect); });
    CHECK(updates.size()==2);if (updates.size()==2) CHECK(Update(updates[1])=="U:20:10:20:10");
}

// 실제497960이 같은 Renderer 표에 만든 노출 띠와 +5c를 다음 Draw/Present가 소비한다.
TEST_CASE(RendererPresentation_ActualCameraCommitSharesDirtyAndRequest) {
    using namespace netstorm::client;Renderer renderer(20,10);std::vector<std::uint8_t> pixels(200);ClientCameraGlobals camera;
    std::uint32_t worldRedraw{},revision{},depth{},scrolls{},roots{}; // 원문 현재 전역과 명시 커서/화면 장치 호출 관측이다.
    camera.InitializeNativeRenderer(20,10,256,worldRedraw,revision);worldRedraw=0;
    renderer.Draw(pixels,20);renderer.Present([](ScreenRect) {});
    for (int y=0;y<10;++y) { // 이동 전 각 열을 구별해 내부 픽셀의 실제 이동을 확인한다.
        for (int x=0;x<20;++x) pixels[static_cast<std::size_t>(y)*20+x]=static_cast<std::uint8_t>(x);
    }
    camera.Target().pixels={2,0};const std::uint32_t flags=2;const std::int32_t denominator=128,width=20,height=10;
    const RawCameraCommitViewport viewport{0,0,20,10};const ScreenRect map{0,0,20,10};const ScreenPoint mouse{};
    RawCameraCommitHooks hooks;
    hooks.lockCursor=[&](auto skip) { CHECK(skip==0);++depth; };hooks.unlockCursor=[&] { --depth; }; // 실제 OS 커서 대신 잠금 경계만 관측한다.
    hooks.redrawRoots=[&] { ++roots; };hooks.updateMouse=[](ScreenPoint) {}; // Gump/마우스 효과는 별도 장치 경계다.
    hooks.addDirty=[&](ScreenRect rect,std::uint32_t value) { renderer.Invalidate(rect,value); }; // 노출 영역 계산은 실제 원문 적용 본체가 수행한다.
    hooks.scroll=[&](auto x,auto y,auto w,auto h,auto dx,auto dy) {
        CHECK(x==0 && y==0 && w==20 && h==10 && dx==2 && dy==0);++scrolls;
        for (int row=0;row<h;++row) { // OS 출력 없이 실제 전달된 양수 X 이동을 같은 메모리 표면에 수행한다.
            auto begin=pixels.begin()+row*w;std::copy(begin+dx,begin+w,begin);
        }
    };
    CommitCameraTarget({camera.RendererView(),camera.Snapshot(),camera.Target(),renderer.CameraRedraw(),worldRedraw,renderer.CameraDirtyEntries()},
        {flags,denominator,width,height,viewport,map,mouse},hooks);
    CHECK(scrolls==1 && roots==2 && depth==0 && renderer.CameraRedraw()==1 && worldRedraw==0); //원문은 이동 시작과 끝에서 각각 Gump 갱신을 호출한다.
    CHECK(camera.RendererView().pixels[0]==2 && camera.Snapshot().pixels[0]==2 && pixels[10]==12);
    std::vector<ScreenRect> collected,updated;
    const RendererWorldDrawing drawing{[&](ScreenRect rect,std::uint32_t) { collected.push_back(rect);return 0U; },[](ScreenRect,std::uint32_t,std::uint32_t) {},
        [&](auto surface,int pitch,int,ScreenRect rect) {
            for (int row=rect.top;row<rect.bottom;++row) { // 실제 본체가 등록한 노출 띠만 다시 합성한다.
                std::fill(surface.begin()+row*pitch+rect.left,surface.begin()+row*pitch+rect.right,std::uint8_t{99});
            }
        }};
    renderer.Draw(pixels,20,0,&drawing);CHECK(collected.size()==1 && Update(collected[0])=="U:18:0:20:10");CHECK(pixels[10]==12 && pixels[19]==99);
    renderer.Present([&](ScreenRect rect) { updated.push_back(rect); });
    CHECK(updated.size()==1 && Update(updated[0])=="U:0:0:20:10" && renderer.CameraRedraw()==0);
}
