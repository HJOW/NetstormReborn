// 원본과 다른 Windows10/11 호환 확대/입력의 계약을 원본 화면 모드 검사와 구별한다.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "TestSupport.h"
#include "client/Screen.h"
#include "client/ScreenPresentation.h"
#include "client/GameMenuCommands.h"
#include <array>
#include <stdexcept>
using namespace netstorm::client;

// 원본4:3과 와이드 논리 화면이4:3·16:9·16:10 출력에서 늘어나지 않는지 확인한다.
TEST_CASE(ScreenPresentation_AspectFitAndLetterbox) {
    const auto classic=FitScreenPresentation(1024,768,1920,1080);CHECK(classic.left==240 && classic.top==0 && classic.right==1680 && classic.bottom==1080);
    const auto wide=FitScreenPresentation(1280,720,1600,1200);CHECK(wide.left==0 && wide.top==150 && wide.right==1600 && wide.bottom==1050);
    const auto ten=FitScreenPresentation(1280,800,1920,1080);CHECK(ten.left==96 && ten.top==0 && ten.right==1824 && ten.bottom==1080);
    const auto small=FitScreenPresentation(1024,768,640,480);CHECK(small.left==0 && small.top==0 && small.right==640 && small.bottom==480);
    const auto odd=FitScreenPresentation(1366,768,1920,1080);CHECK(odd.left==0 && odd.top==0 && odd.right==1920 && odd.bottom==1079);
    bool rejected=false;try { static_cast<void>(FitScreenPresentation(0,768,1920,1080)); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
// 모니터 좌표의 음수/레터박스/끝점과 확대된 실제 중앙 클릭을 논리 입력으로 되돌린다.
TEST_CASE(ScreenPresentation_InversePointAndDirtyPhase) {
    const ScreenRect output{240,0,1680,1080};const auto center=MapScreenPresentationPoint({960,540},output,1024,768);CHECK(center.x==512 && center.y==384);
    const auto first=MapScreenPresentationPoint({-200,-40},output,1024,768);CHECK(first.x==0 && first.y==0);
    const auto last=MapScreenPresentationPoint({1920,1080},output,1024,768);CHECK(last.x==1024 && last.y==768);
    const auto dirty=MapScreenPresentationRect({1,1,2,2},output,1024,768);CHECK(dirty.left==241 && dirty.top==1 && dirty.right==243 && dirty.bottom==3);
    const auto all=MapScreenPresentationRect({-10,-10,2000,1000},output,1024,768);CHECK(all.left==240 && all.top==0 && all.right==1680 && all.bottom==1080);
    const auto tiny=FitScreenPresentation(1024,768,1,1);CHECK(tiny.right==1 && tiny.bottom==1);
}
// 호환 모드는 실제 DIB/비flip을 공개하며 원본 합법성 함수의 DirectDraw 판정은 바꾸지 않는다.
TEST_CASE(ScreenPresentation_CompatibilityFlagsAndNativeCommands) {
    CHECK(ModernScreenMode(111)==235U);CHECK(ModernScreenMode(139)==139U);CHECK(ModernScreenMode(0)==130U);
    CHECK(ModernScreenMode(94)==138U);CHECK(!ScreenModeIsLegal(111,false));CHECK(ScreenModeIsLegal(ModernScreenMode(111),true));
    CHECK(GameMenuScriptCode("ScreenModeAbsChange")==0x02000030U);CHECK(GameMenuScriptCode("ScreenModeChange")==0x02000002U);CHECK(GameMenuScriptCode("AcceptScreenModeChange")==0x020000e4U);
    CHECK(IsGameMenuCommand(0x02000030U) && IsGameMenuCommand(0x02000002U) && IsGameMenuCommand(0x020000e4U));
}
//1366폭의 실제 Win32 DIB는 행마다1368BYTE이므로 끝 행 쓰기가 옆 행과 섞이지 않아야 한다.
TEST_CASE(ScreenPresentation_WideDibUsesActualAlignedPitch) {
    const HDC reference=CreateCompatibleDC(nullptr);CHECK(reference!=nullptr);if (!reference) return;
    {
        ScreenSurface surface(ScreenSurface::Kind::Dib,1366,768,nullptr,reference);CHECK(surface.Pitch()==1368);
        auto* bytes=surface.Lock();CHECK(bytes!=nullptr);if (bytes) { bytes[1365]=17;bytes[1368]=31;bytes[1368*767+1365]=47;CHECK(bytes[1365]==17 && bytes[1368]==31 && bytes[1368*767+1365]==47); }surface.Unlock();
    }
    DeleteDC(reference);
}
// 확대/축소의 중앙 표본·실제 행 간격·padding 보존을 작은 독립 입력으로 확인한다.
TEST_CASE(ScreenPresentation_IntegerNearestPixelsAndBounds) {
    const std::array<std::uint8_t,8> source{1,2,77,77,3,4,77,77};std::array<std::uint8_t,12> output;output.fill(99);
    ScaleScreenPresentation(source,4,2,2,output,4,3,3);
    CHECK((output==std::array<std::uint8_t,12>{1,2,2,99,3,4,4,99,3,4,4,99}));
    const std::array<std::uint8_t,15> large{0,1,2,3,77,4,5,6,7,77,8,9,10,11,77};std::array<std::uint8_t,8> small;small.fill(99);
    ScaleScreenPresentation(large,5,4,3,small,4,3,2);CHECK((small==std::array<std::uint8_t,8>{0,2,3,99,8,10,11,99}));
    bool rejected=false;try { ScaleScreenPresentation(std::span<const std::uint8_t>(source).first(5),4,2,2,output,4,3,3); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);
}
