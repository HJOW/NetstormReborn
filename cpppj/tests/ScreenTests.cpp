// 화면 장치(Screen)의 순수 계산과 입력 사건 큐의 단위 검사. 창을 만들지 않는다.
// 창·DIB·팔레트가 실제로 동작하는지는 tools/cpp_window_smoke.py가 실행 파일을 띄워 확인한다.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "TestSupport.h"
#include "client/InputEvent.h"
#include "client/Screen.h"
#include <fstream>
#include <sstream>
#include <string>

using namespace netstorm::client;

// 실제 GDI에서 100번 팔레트를 바꾸고 끝낼 때 선택된 객체가 누수되지 않는지 검사한다. 창과 소리 장치는 만들지 않는다.
TEST_CASE(ScreenPalette_ReplacementsAndDestructionReleaseGdiObjects) {
    // 검사 참조 DC만 소유하는 도우미다. Screen이 먼저 사라지고 마지막에 참조 DC를 해제한다.
    struct ReferenceDc {
        HDC handle{};
        // 화면과 호환되는 메모리 DC를 만든다. 바탕 화면이나 창의 선택 객체를 건드리지 않는다.
        ReferenceDc():handle(CreateCompatibleDC(nullptr)) {}
        // 생성에 성공한 참조 DC만 해제한다.
        ~ReferenceDc() { if (handle) DeleteDC(handle); }
    } reference;
    CHECK(reference.handle!=nullptr);if (!reference.handle) return;
    const auto before=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
    {
        Screen screen(nullptr,reference.handle,640,480);screen.Init();screen.InitDibSection();
        std::array<std::uint8_t,776> bytes{};
        // 검사 COL의 256색을 회색 단계로 채운다. 파일 읽기 없이 실제 팔레트 적용만 반복한다.
        for (std::size_t i=8;i<bytes.size();++i) bytes[i]=static_cast<std::uint8_t>((i-8)/3);
        const GamePalette palette(bytes);const auto live=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
        // 각 교체 직후 객체 수가 같아야 한다. 이전 팔레트가 DC에 선택된 채 남으면 DeleteObject가 실패하여 증가한다.
        for (int i=0;i<100;++i) {
            screen.LoadPalette(palette);CHECK(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)==live);
        }
    }
    CHECK(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)==before);
}

// 파일 로더/화면을 거치지 않고 실제 원본 검색 몸체의 전체 32비트 입력·두 x87 결과와 대조한다.
TEST_CASE(ScreenPalette_NearestColorMatchesThreeNativeBodies) {
    std::ifstream input(NETSTORM_PALETTECOLOR_FIXTURE);CHECK(input.good());
    std::string line;std::size_t cases=0;
    // 주석을 건너뛰고 관찰 행마다 논리 팔레트의 little-endian DWORD 256개를 복원한다.
    while (std::getline(input,line)) {
        if (line.empty() || line.front()=='#') continue;
        std::istringstream row(line);std::string edition,hex;std::int32_t red{},green{},blue{};std::uint32_t expected{};
        row>>edition>>red>>green>>blue>>hex>>expected;
        CHECK(!row.fail() && hex.size()==2048);if (row.fail() || hex.size()!=2048) continue;
        std::array<std::uint32_t,256> palette{};
        // DWORD마다 네 바이트를 원본 순서로 합친다. 최상위 플래그도 유지한다.
        for (std::size_t i=0;i<palette.size();++i)
            // RGB와 플래그가 서로 바뀌지 않도록 각 바이트를 정해진 자리로 옮긴다.
            for (std::size_t byte=0;byte<4;++byte)
                palette[i]|=static_cast<std::uint32_t>(std::stoul(hex.substr(i*8+byte*2,2),nullptr,16))<<(byte*8);
        CHECK(FindPaletteColor(palette,red,green,blue)==expected);++cases;
    }
    CHECK(cases==786);
}

// 논리 팔레트는 apply=false 준비에서도 바뀐다. 검색이 표시 팔레트 BGR이나 예약 바이트를 읽지 않는지 본다.
TEST_CASE(ScreenPalette_PreparedLogicalColorsAndWeatherTable) {
    Screen screen(nullptr,nullptr,640,480);
    std::array<ScreenColor,256> colors{};
    colors[17]={22,255,255,193};colors[38]={255,22,22,147};
    colors[69]={22,22,255,91};colors[127]={111,140,181,255};
    screen.SetPalette(0,256,colors.data(),false);
    const auto tints=screen.WeatherTints();
    CHECK((tints==std::array<std::uint32_t,4>{17,38,69,127}));
    CHECK(screen.FindColor(0,0,0)==0 && screen.FindColor(255,255,255)==255);
    // 표시 버퍼는 적용하지 않았으므로 여전히 검정이다.
    CHECK(screen.Palette()[17].red==0 && screen.Palette()[17].green==0);
}

// 원본 004a13d0의 다섯 조건. setup.cfg의 기본값(창 10, 전체화면 11)이 허용되는지도 본다.
TEST_CASE(ScreenMode_LegalCombinations_FollowOriginalRules) {
    // setup.cfg의 windowScreenFlags = 10(DIB + 구름).
    CHECK(ScreenModeIsLegal(ScreenMode::kFallbackWindowed, false));
    CHECK(ScreenMode::kFallbackWindowed == 10);
    // 전체화면은 DirectDraw가 있어야 한다.
    CHECK(!ScreenModeIsLegal(ScreenMode::kFullScreen, false));
    CHECK(ScreenModeIsLegal(ScreenMode::kFullScreen, true));
    CHECK(ScreenModeIsLegal(ScreenMode::kFullScreen | ScreenMode::kFlipping, true));
    // setup.cfg의 workingFullScreenFlags = 11(전체화면 + DIB + 구름).
    CHECK(ScreenModeIsLegal(11, true) && !ScreenModeIsLegal(11, false));
    // 플리핑은 전체화면에서만.
    CHECK(!ScreenModeIsLegal(ScreenMode::kBackBufferDib | ScreenMode::kFlipping, true));
    // 시차·움직임은 구름이 있어야 한다.
    CHECK(!ScreenModeIsLegal(ScreenMode::kBackBufferDib | ScreenMode::kParallax, true));
    CHECK(!ScreenModeIsLegal(ScreenMode::kBackBufferDib | ScreenMode::kAnimating, true));
    CHECK(ScreenModeIsLegal(ScreenMode::kBackBufferDib | ScreenMode::kClouds | ScreenMode::kParallax | ScreenMode::kAnimating, false));
    // 전체화면도 DIB도 아니면 그릴 곳이 없다.
    CHECK(!ScreenModeIsLegal(0, true));
    CHECK(!ScreenModeIsLegal(ScreenMode::kClouds | ScreenMode::kModeSet, true));
}

// 원본 00445290: 네 좌표를 각각 경계 안으로 넣는다. 뒤집힌 사각형을 바로잡지는 않는다.
TEST_CASE(ScreenRect_Clip_ClampsEachCoordinate) {
    const ScreenRect bounds{0, 0, 640, 480};
    const auto inside = ClipScreenRect({10, 20, 30, 40}, bounds);
    CHECK(inside.left == 10 && inside.top == 20 && inside.right == 30 && inside.bottom == 40);
    const auto crossing = ClipScreenRect({-5, -7, 700, 500}, bounds);
    CHECK(crossing.left == 0 && crossing.top == 0 && crossing.right == 640 && crossing.bottom == 480);
    // 완전히 밖이면 폭·높이가 0인 사각형이 된다.
    const auto outside = ClipScreenRect({650, 490, 700, 500}, bounds);
    CHECK(outside.left == 640 && outside.right == 640 && outside.top == 480 && outside.bottom == 480);
    const auto reversed = ClipScreenRect({30, 40, 10, 20}, bounds);
    CHECK(reversed.left == 30 && reversed.right == 10);
}

// 원본 004a4fe0의 switch(initWindowPos): 1~5는 바탕 화면의 모서리·가운데, 그 밖은 지금 위치.
TEST_CASE(Screen_InitialWindowPosition_FollowsSetting) {
    const ScreenPoint current{33, 44};
    // 바탕 화면 1920 × 1080, 창 1040 × 807.
    const auto at = [&](int setting) { return InitialWindowPosition(setting, 1920, 1080, 1040, 807, current); };
    CHECK(at(1).x == 0 && at(1).y == 0);
    CHECK(at(2).x == 880 && at(2).y == 0);
    CHECK(at(3).x == 880 && at(3).y == 273);
    CHECK(at(4).x == 0 && at(4).y == 273);
    CHECK(at(5).x == 440 && at(5).y == 136);
    CHECK(at(0).x == 33 && at(0).y == 44);
    CHECK(at(6).x == 33 && at(6).y == 44);
    // 창이 바탕 화면보다 크면 음수가 된다(원본은 고치지 않는다).
    CHECK(InitialWindowPosition(5, 800, 600, 1040, 807, current).x == -120);
}

// 원본 00452f90·00452f00: 넣은 순서로 나오고, 빈 큐는 0으로 채운 사건을 준다.
TEST_CASE(InputQueue_PushAndPop_KeepOrder) {
    InputQueue queue;
    CHECK(queue.Empty());
    const auto none = queue.Pop(false);
    CHECK(none.code == 0 && none.x == 0 && none.y == 0);
    queue.Push(InputCode::kLeftButton, 10, 20);
    queue.Push(InputCode::kRightButton | InputCode::kShift, 30, 40);
    CHECK(!queue.Empty());
    const auto first = queue.Pop(false);
    CHECK(first.code == InputCode::kLeftButton && first.x == 10 && first.y == 20);
    const auto second = queue.Pop(false);
    CHECK(second.code == (InputCode::kRightButton | InputCode::kShift) && second.x == 30 && second.y == 40);
    CHECK(queue.Empty());
}

// 40칸 고리에는 39개까지 담긴다. 가득 차면 새로 넣은 것을 버린다.
TEST_CASE(InputQueue_Overflow_DropsNewest) {
    InputQueue queue;
    // 칸 수보다 많이 넣는다.
    for (std::uint32_t i = 1; i <= 50; ++i) queue.Push(i, static_cast<std::int32_t>(i), 0);
    // 앞의 39개가 순서대로 남는다.
    for (std::uint32_t i = 1; i <= InputQueue::kCapacity - 1; ++i) CHECK(queue.Pop(false).code == i);
    CHECK(queue.Empty());
    // 고리를 한 바퀴 돈 뒤에도 순서가 유지된다.
    queue.Push(100, 0, 0);
    queue.Push(101, 0, 0);
    CHECK(queue.Pop(false).code == 100 && queue.Pop(false).code == 101 && queue.Empty());
}

// 뗌 사건 거르기, DEL → Ctrl+Backspace, 명령 사건의 코드 다듬기.
TEST_CASE(InputQueue_ReleaseFilterDeleteAndCommand_MatchOriginal) {
    InputQueue queue;
    // 걸러진 뗌 사건도 큐에서는 빠지고, 돌려주는 값은 0이다.
    queue.Push(InputCode::kLeftButton | InputCode::kRelease, 1, 2);
    queue.Push(InputCode::kLeftButton, 3, 4);
    const auto skipped = queue.Pop(true);
    CHECK(skipped.code == 0 && skipped.x == 0);
    const auto pressed = queue.Pop(true);
    CHECK(pressed.code == InputCode::kLeftButton && pressed.x == 3);
    // 거르지 않으면 뗌 사건이 그대로 나온다.
    queue.Push(InputCode::kRightButton | InputCode::kRelease, 5, 6);
    CHECK(queue.Pop(false).code == (InputCode::kRightButton | InputCode::kRelease));
    // 글자 0x7f(DEL)는 Ctrl+Backspace(8)가 된다. 보조 키 가운데 Ctrl 자리만 새로 켜진다.
    queue.Push(0x7f, 0, 0);
    CHECK(queue.Pop(false).code == (InputCode::kControl | 8u));
    queue.Push(InputCode::kRelease | 0x7f, 0, 0);
    CHECK(queue.Pop(false).code == (InputCode::kRelease | InputCode::kControl | 8u));
    // 가상 키 0x7f(F16)는 글자가 아니므로 바뀌지 않는다.
    queue.Push(0x7fu << InputCode::kVirtualKeyShift, 0, 0);
    CHECK(queue.Pop(false).code == (0x7fu << InputCode::kVirtualKeyShift));
    // 명령 사건: 0x0100ffff만 남기고 명령 표시를 붙인다.
    queue.PushCommand(0xffffffffu, 7, 8);
    const auto command = queue.Pop(false);
    CHECK(command.code == (0x0100ffffu | InputCode::kCommand) && command.x == 7 && command.y == 8);
}
