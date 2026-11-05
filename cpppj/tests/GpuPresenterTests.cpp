// 7차 안티앨리어싱/FSR 1.0: Direct3D 11 출력기(GpuPresenter)가 숨은 창에서 장치 생성, 셰이더 컴파일, 후처리 패스, 읽기를 끝까지 수행하는지 확인한다.
// 하드웨어 장치가 없는 환경에서는 소프트웨어(WARP)로 대신하므로 장치 생성 자체가 실패하는 환경에서만 건너뛴다.
#include "TestSupport.h"
#include "client/GpuPresenter.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

using namespace netstorm::client;

namespace { // 숨은 검사 창과 합성 프레임 보조다.
// 클라이언트 영역이 width×height인 숨은 팝업 창을 만든다.
HWND MakeWindow(int width, int height) {
    RECT rect{0, 0, width, height};
    AdjustWindowRectEx(&rect, WS_POPUP, FALSE, 0);
    return CreateWindowExA(0, "STATIC", "gpu-test", WS_POPUP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
}
// 색 표: 0은 검정, 1은 빨강, 2는 초록, 3은 파랑, 4는 흰색이며 나머지는 회색 계단이다(BGRX 바이트 순서).
std::array<std::uint32_t, 256> Palette() {
    std::array<std::uint32_t, 256> palette{};
    for (int index = 0; index < 256; ++index) palette[index] = static_cast<std::uint32_t>(index) * 0x010101u;
    palette[1] = 0x00ff0000u; // B,G,R,X 바이트 순서에서 R이 세 번째라 0x00RR0000 위치의 값이 빨강이다.
    palette[2] = 0x0000ff00u;
    palette[3] = 0x000000ffu;
    palette[4] = 0x00ffffffu;
    return palette;
}
// 읽은 BGRA 그림에서 한 점의 색을 {R,G,B}로 돌려준다.
std::array<int, 3> At(const std::vector<std::uint8_t>& bgra, int width, int x, int y) {
    const auto* p = bgra.data() + (static_cast<std::size_t>(y) * width + x) * 4;
    return {p[2], p[1], p[0]};
}
}

// 장치 확인과 FSR/안티앨리어싱 끝-끝 경로: 같은 색 영역은 업스케일 뒤에도 같은 색이고 레터박스 바깥은 검정이며 읽은 크기가 클라이언트 크기와 같다.
TEST_CASE(GpuPresenter_FsrAndAntiAliasingPathsRunAndPreserveFlatColors) {
    std::string adapter;
    if (!GpuPresenter::Probe(&adapter)) { std::printf("Direct3D 11 장치를 만들 수 없는 환경이라 건너뜀\n"); return; }
    CHECK(!adapter.empty());
    const int clientWidth = 640, clientHeight = 480;
    HWND window = MakeWindow(clientWidth, clientHeight);
    CHECK(window != nullptr);
    std::string error;
    auto presenter = GpuPresenter::Create(window, error);
    CHECK(presenter != nullptr);
    if (!presenter) { std::printf("출력기 생성 실패: %s\n", error.c_str()); DestroyWindow(window); return; }
    CHECK(error.empty() && !presenter->AdapterName().empty());
    // 64×36 논리 프레임: 왼쪽 절반 빨강, 오른쪽 절반 파랑이다(경계를 제외한 안쪽은 균일색).
    const int width = 64, height = 36, pitch = 68;
    std::vector<std::uint8_t> frame(static_cast<std::size_t>(pitch) * height, 0);
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) frame[static_cast<std::size_t>(y) * pitch + x] = x < width / 2 ? 1 : 3;
    const auto palette = Palette();
    std::vector<std::uint8_t> bgra;
    int readWidth = 0, readHeight = 0;
    CHECK(!presenter->ReadBack(bgra, readWidth, readHeight)); // 표시 전에는 읽을 그림이 없다.
    // 출력 사각형: 640×360 16:9가 480 높이 안에 레터박스로 놓인다(위아래 60칸 검정).
    const GpuOutputRect output{0, 60, 640, 420};
    for (const bool fsr : {false, true}) for (const bool antiAliasing : {false, true}) { // 네 조합을 모두 돌린다.
        GpuPresentOptions options;
        options.fsr = fsr;
        options.antiAliasing = antiAliasing;
        const bool ok = presenter->Present(frame.data(), pitch, width, height, palette.data(), output, options);
        if (!ok) std::printf("Present 실패(fsr=%d aa=%d): %s\n", fsr, antiAliasing, presenter->LastError().c_str());
        CHECK(ok);
        if (!ok) continue;
        CHECK(presenter->ReadBack(bgra, readWidth, readHeight) && readWidth == clientWidth && readHeight == clientHeight);
        if (bgra.size() != static_cast<std::size_t>(clientWidth) * clientHeight * 4) continue;
        CHECK(At(bgra, clientWidth, 100, 20) == (std::array<int, 3>{0, 0, 0}) && At(bgra, clientWidth, 100, 450) == (std::array<int, 3>{0, 0, 0})); // 레터박스 위/아래는 검정이다.
        const auto left = At(bgra, clientWidth, 100, 240), right = At(bgra, clientWidth, 540, 240); // 경계에서 멀리 떨어진 안쪽 점이다.
        CHECK(left[0] > 240 && left[1] < 15 && left[2] < 15); // 빨강이 유지된다.
        CHECK(right[2] > 240 && right[0] < 15 && right[1] < 15); // 파랑이 유지된다.
    }
    presenter.reset();
    DestroyWindow(window);
}

// 클라이언트 크기가 바뀌면 스왑 체인이 새 크기로 다시 만들어지고 이후 읽기 크기도 따라간다.
TEST_CASE(GpuPresenter_ResizesWithTheClientAreaAndRejectsBadArguments) {
    if (!GpuPresenter::Probe()) return;
    HWND window = MakeWindow(320, 200);
    std::string error;
    auto presenter = GpuPresenter::Create(window, error);
    CHECK(presenter != nullptr);
    if (!presenter) { DestroyWindow(window); return; }
    const std::vector<std::uint8_t> frame(16 * 9, 4);
    const auto palette = Palette();
    GpuPresentOptions options;
    CHECK(presenter->Present(frame.data(), 16, 16, 9, palette.data(), {0, 0, 320, 200}, options));
    std::vector<std::uint8_t> bgra;
    int width = 0, height = 0;
    CHECK(presenter->ReadBack(bgra, width, height) && width == 320 && height == 200);
    SetWindowPos(window, nullptr, 0, 0, 480 + 0, 300 + 0, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    RECT client{};
    GetClientRect(window, &client);
    CHECK(presenter->Present(frame.data(), 16, 16, 9, palette.data(), {0, 0, client.right, client.bottom}, options));
    CHECK(presenter->ReadBack(bgra, width, height) && width == client.right && height == client.bottom);
    CHECK(At(bgra, width, width / 2, height / 2) == (std::array<int, 3>{255, 255, 255})); // 흰색 프레임이 가운데에 유지된다.
    CHECK(!presenter->Present(nullptr, 16, 16, 9, palette.data(), {0, 0, 320, 200}, options)); // 널 프레임은 거부한다.
    CHECK(!presenter->Present(frame.data(), 8, 16, 9, palette.data(), {0, 0, 320, 200}, options)); // 줄 간격이 폭보다 작으면 거부한다.
    CHECK(!presenter->Present(frame.data(), 16, 16, 9, palette.data(), {10, 10, 10, 10}, options)); // 빈 출력 사각형은 거부한다.
    CHECK(!presenter->LastError().empty());
    CHECK(presenter->Present(frame.data(), 16, 16, 9, palette.data(), {0, 0, client.right, client.bottom}, options) && presenter->LastError().empty()); // 다음 성공이 오류를 지운다.
    presenter.reset();
    DestroyWindow(window);
}
