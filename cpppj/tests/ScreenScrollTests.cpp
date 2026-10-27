// DIB의 겹친 화면 이동과 실제 Renderer의 후처리 dirty/다음 Draw 소비를 검사한다.
#include "TestSupport.h"
#include "client/ScreenPresentation.h"
#include "client/RawCameraCommit.h"
#include <array>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>

using namespace netstorm::client; // 실제 같은 인덱스 버퍼와 Renderer를 사용한다.
// 보호4a1ad0이 실제 고른 행 순서로 만든 전체144BYTE와 padding을 그대로 대조한다.
TEST_CASE(ScreenScroll_MatchesNativeDibPixels) {
    std::ifstream file(NETSTORM_SCREEN_SCROLL_FIXTURE);CHECK(file.good());std::string line;std::size_t count{};
    while (std::getline(file,line)) { // 각 실제 원문 관측에 같은 독립 초기 버퍼를 공급한다.
        if (line.empty() || line.front()=='#') continue;std::istringstream row(line);int dx{},dy{};std::string expected,events;row>>dx>>dy>>expected>>events;CHECK(!row.fail() && expected.size()==288);if (row.fail() || expected.size()!=288) continue;
        std::array<std::uint8_t,144> pixels{};for (std::size_t i=0;i<pixels.size();++i) pixels[i]=static_cast<std::uint8_t>(i); // 원문 입력만 만들며 기대 이동은 계산하지 않는다.
        ScrollScreenPixels(pixels,16,13,9,2,1,9,7,dx,dy);
        for (std::size_t i=0;i<pixels.size();++i) CHECK(pixels[i]==static_cast<std::uint8_t>(std::stoul(expected.substr(i*2,2),nullptr,16))); // 원문 전체 결과의 각 바이트를 비교한다.
        ++count;
    }
    CHECK(count==357);
}
// 목적지 각 픽셀을 변경 전 버퍼의 독립 좌표로 대조하여 행 순서/겹침/부분 영역/padding을 확인한다.
TEST_CASE(ScreenScroll_AllDirectionsPreserveSourceAndPadding) {
    constexpr int pitch=16,width=13,height=9,x=2,y=1,w=9,h=7; // 홀수 논리 폭과 실제 DIB padding을 포함한 입력이다.
    std::vector<std::uint8_t> original(pitch*height);
    for (std::size_t i=0;i<original.size();++i) original[i]=static_cast<std::uint8_t>(i); // 각 원본 픽셀을 서로 다른 번호로 구별한다.
    for (int dy=-8;dy<=8;++dy) { // 위/아래·범위 밖 이동과0을 함께 검사한다.
        for (int dx=-10;dx<=10;++dx) { // 좌/우·범위 밖과 대각선의 겹침을 교차한다.
            auto actual=original;ScrollScreenPixels(actual,pitch,width,height,x,y,w,h,dx,dy);
            for (int row=0;row<height;++row) { // 이동 범위 밖과 padding도 변경되지 않아야 한다.
                for (int column=0;column<pitch;++column) { // 각 기대 픽셀은 변경 전 원본에서만 읽는다.
                    const bool copied=std::abs(dx)<w && std::abs(dy)<h && column>=x && column<x+w && row>=y && row<y+h && column+dx>=x && column+dx<x+w && row+dy>=y && row+dy<y+h;
                    CHECK(actual[row*pitch+column]==original[copied ? (row+dy)*pitch+column+dx : row*pitch+column]);
                }
            }
        }
    }
}
// 선택된 복사의 화면 밖 메모리는 쓰기 전에 거부하고0/겹침 없는 경우는 원문처럼 반환한다.
TEST_CASE(ScreenScroll_SelectedSurfaceBounds) {
    std::array<std::uint8_t,64> pixels{};bool rejected{};try { ScrollScreenPixels(pixels,8,8,8,-1,0,5,5,1,1); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);
    ScrollScreenPixels({},8,8,8,-1,0,5,5,0,0);ScrollScreenPixels({},8,8,8,0,0,5,5,5,0);
}
// 현재 카메라 적용이 같은 Renderer 표를 수정하고 다음 Draw만 전체 재그리기 DWORD를 소비한다.
TEST_CASE(ScreenScroll_CameraUsesActualRendererDirtyAndNextDraw) {
    Renderer renderer(16,12);ClientCameraGlobals camera;InitializeCameraView(camera.RendererView());InitializeCameraView(camera.Snapshot());camera.Target().zoom=65536;camera.Target().pixels={2,3};
    renderer.CameraDirtyEntries().push_back({{4,5,8,9},2});std::uint32_t flags=2,world=0;std::int32_t width=16,height=12,denominator=15;RawCameraCommitViewport viewport{0,0,16,12};ScreenRect clip{0,0,16,12};ScreenPoint mouse{};std::vector<std::uint8_t> pixels(16*12);
    RawCameraCommitHooks hooks;hooks.lockCursor=[](auto) {};hooks.unlockCursor=[] {};hooks.redrawRoots=[] {};
    hooks.scroll=[&](auto x,auto y,auto w,auto h,auto dx,auto dy) { ScrollScreenPixels(pixels,16,16,12,x,y,w,h,dx,dy); };hooks.updateMouse=[](auto) {};
    hooks.addDirty=[&](auto rect,auto value) { renderer.Invalidate(rect,value); };
    CommitCameraTarget({camera.RendererView(),camera.Snapshot(),camera.Target(),renderer.CameraRedraw(),world,renderer.CameraDirtyEntries()},{flags,denominator,width,height,viewport,clip,mouse},hooks);
    CHECK(renderer.CameraRedraw()==1 && !renderer.CameraDirtyEntries().empty());const auto drawn=renderer.Draw(pixels,16);CHECK(drawn.size()==1 && drawn[0].left==0 && drawn[0].right==16 && renderer.CameraRedraw()==0);
    renderer.Present([](auto) {});CHECK(renderer.CameraDirtyEntries().empty());
}
