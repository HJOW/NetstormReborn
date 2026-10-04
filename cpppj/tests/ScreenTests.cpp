// 화면 장치(Screen)의 순수 계산과 입력 사건 큐의 단위 검사. 창을 만들지 않는다.
// 창·DIB·팔레트가 실제로 동작하는지는 tools/cpp_window_smoke.py가 실행 파일을 띄워 확인한다.
#include "TestSupport.h"
#include "client/InputEvent.h"
#include "client/Screen.h"

using namespace netstorm::client;

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
