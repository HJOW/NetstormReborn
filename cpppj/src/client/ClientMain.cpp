// 원본 소스: ClientMain.cpp (클라이언트 폴더) — 프로그램 진입점과 메인 루프. 원본처럼 Win32를 직접 부른다.
// 원본: FUN_00438dc0(WinMain) ↔ CD FUN_00485b20, FUN_00436550(창 프로시저), FUN_00436450(프레임) ↔ CD FUN_00486df0,
//       FUN_00436260(로딩 화면), FUN_00436020·FUN_00436190(키·글자 사건), FUN_00435220(설정 해석) ↔ CD FUN_00484ac0,
//       FUN_00435b30(메시지 처리), FUN_00452e00(입력 폴링).
// 범위: 창 만들기, 설정 읽기, 화면 장치, 메시지·입력 큐, 프레임 제한과 내보내기까지.
//       Renderer·원본 글꼴/커서·메뉴와 저장 미션의 기본 지형/객체·선택/이동을 연결했다. 건설/전투는 후속이다.
#include "client/ClientMain.h"
#include "client/UberGump.h"
#include "o/OriginalText.h"
#include "platform/Bitmap.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>

namespace netstorm::client {
namespace {
// 원본 창 클래스 스타일 0x23: CS_VREDRAW | CS_HREDRAW | CS_OWNDC.
constexpr UINT kWindowClassStyle = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
// 원본이 CreateWindowExA에 넘기는 처음 창 크기(0x1c4 × 0x1d7). 곧 화면 크기에 맞춰 바뀐다.
constexpr int kInitialWindowWidth = 0x1c4;
constexpr int kInitialWindowHeight = 0x1d7;
// 원본 실행 파일의 리소스 번호: 큰 아이콘, 작은 아이콘, 로딩 그림.
constexpr WORD kIconResource = 0x66;
constexpr WORD kSmallIconResource = 0x90;
constexpr WORD kLoadingBitmapResource = 0x93;
// 로딩 화면 글꼴: Arial 20포인트(원본 CreateFontA 인자).
constexpr int kLoadingFontPoints = 20;
// 로딩 화면의 글(원본 PTR_s_Loading_00531934, PTR_s_Please_Wait_00531938).
constexpr const char* kLoadingText = "Loading";
constexpr const char* kPleaseWaitText = "Please Wait";
// 로딩 글의 오른쪽·아래 여백(원본 -10, -0x14).
constexpr int kLoadingMarginX = 10;
constexpr int kLoadingMarginY = 0x14;
// 원본 메뉴 명령 번호: 끝내기.
constexpr WORD kMenuExit = 0x9c41;
// 설정 `highPriorityValue` 0·1·2에 대응하는 스레드 우선순위(원본 WinMain 루프).
constexpr std::array<int, 3> kThreadPriorities{THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_TIME_CRITICAL};
// 원본 실행 파일 버전(FUN_00441790에 넘기는 10, 0x4e). CD판은 10.72다.
constexpr int kMajorVersion = 10;
constexpr int kPatchMinorVersion = 78;
constexpr int kCdMinorVersion = 72;
// 원본 시작 표시 파일은 d/ 밖, 게임 폴더에 둔다(00436dd0·00436df0·00436e40).
constexpr const char* kFullScreenStateFile = "fullscreenStateFile.dat";
// 언어 이름 → 언어 번호(원본 FUN_004de420). 0·1은 영어다.
constexpr std::array<const char*, 7> kLanguages{"english", "english", "french", "german", "spanish", "japanese", "portuguese"};

// 창 프로시저가 찾는 단 하나의 클라이언트(원본은 전역 변수를 쓴다).
Client* g_client = nullptr;

// 정적 창 프로시저: 클라이언트가 있으면 넘기고, 없으면 기본 처리한다.
LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (g_client) return static_cast<LRESULT>(g_client->HandleMessage(window, message, wParam, lParam));
    return DefWindowProcA(window, message, wParam, lParam);
}

// 환경 변수를 읽는다(설정의 `{_이름}` 조회와 IDENTITY·USER).
std::optional<std::string> Environment(std::string_view name) {
    const char* value = std::getenv(std::string(name).c_str());
    if (!value) return std::nullopt;
    return std::string(value);
}

// 지금 눌린 보조 키를 사건 코드 비트로 바꾼다(원본 FUN_00436020의 GetKeyState 세 번).
std::uint32_t ModifierBits() {
    std::uint32_t bits = 0;
    if ((GetKeyState(VK_SHIFT) & 0x8000) != 0) bits |= InputCode::kShift;
    if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) bits |= InputCode::kControl;
    if ((GetKeyState(VK_MENU) & 0x8000) != 0) bits |= InputCode::kAlt;
    return bits;
}

// 원본 00436df0 → BaseFile 모드 1: r+b로 열고 없으면 w+b로 만든 뒤, 데이터를 쓰지 않고 닫는다.
void CreateFullScreenState(const std::filesystem::path& directory) {
    const auto name = (directory / kFullScreenStateFile).string();
    FILE* file = std::fopen(name.c_str(), "r+b");
    if (!file) file = std::fopen(name.c_str(), "w+b");
    if (!file) throw std::runtime_error("Unable to open fullscreenStateFile.dat");
    std::fclose(file);
}
}

// 원본: FUN_00435220 @ 00435220 (패치판 10.78) [신뢰도 A] ↔ CD판 FUN_00484ac0 @ 00484ac0 (string/strong)
//   DAT_005318d8 = 0x4b; FUN_00441270("maxFPS", &DAT_005318d8);
//   _DAT_005318e0 = 0.0; if (0 < DAT_005318d8) _DAT_005318e0 = 1.0 / (double)DAT_005318d8;
// 이 간격은 FUN_00436450 의 바쁜 대기(`벽시계 - 직전 그리기 시각 < 간격`)에서 쓰인다.
double FrameIntervalSeconds(int maxFps) {
    if (maxFps > 0) {
        return 1.0 / static_cast<double>(maxFps);
    }
    return 0.0;
}

// 새 계산 API: 원본 00436450의 대기를 1ms 눈금 기준으로 표시한다. 양수 나눗셈 올림을 사용한다.
std::uint32_t QuantizedFrameMilliseconds(int maxFps) {
    return maxFps > 0 ? (1000u + static_cast<std::uint32_t>(maxFps) - 1u) / static_cast<std::uint32_t>(maxFps) : 0u;
}

// 파일 시스템과 설정 객체만 만든다. 창과 화면은 Run에서 원본 순서대로 만든다.
Client::Client(ClientOptions options)
    : options_(std::move(options)), files_(options_.gameDirectory),
      configuration_([this](std::string_view path) { return files_.TryRead(path); }) {
    files_.RegisterArchive(options_.gameDirectory / "netstorm.tarc");
    configuration_.Registry().environment = Environment;
    if (g_client) throw std::logic_error("Only one client may exist");
    g_client = this;
}
// 화면 장치를 먼저 없앤 뒤 GDI 객체와 창을 정리한다.
Client::~Client() {
    menu_.reset();
    renderer_.reset(); fonts_.reset(); cursor_.reset();
    screen_.reset();
    if (loadingFont_) DeleteObject(static_cast<HFONT>(loadingFont_));
    if (loadingBitmap_) DeleteObject(static_cast<HBITMAP>(loadingBitmap_));
    if (window_ && IsWindow(static_cast<HWND>(window_))) DestroyWindow(static_cast<HWND>(window_));
    if (resources_) FreeLibrary(static_cast<HMODULE>(resources_));
    UnregisterClassA(kWindowClassName, static_cast<HINSTANCE>(instance_));
    g_client = nullptr;
}

// 원본 WinMain. 옮기지 않은 단계는 원본 순서의 자리에 주석으로 남긴다.
int Client::Run() {
    // [원본] CreateMutexA("TitanicNetStormMutex"): 먼저 뜬 실행인지 기록한다(네트워크용). 원본 게임과 같이 띄울 때
    //        서로 영향을 주지 않도록 아직 만들지 않는다.
    // [원본] 명령줄이 "window"면 전체화면 설정을 무시한다 → options_.forceWindow.
    // [원본] FUN_00435cd0: 명령줄이 있으면 그것을, 없으면 실행 파일 위치를 게임 폴더로 삼는다 → options_.gameDirectory.
    // [원본] CD-ROM에서 직접 실행하면 안내 창을 띄우고 끝낸다 — 옮기지 않았다.
    instance_ = GetModuleHandleA(nullptr);
    // 아이콘·커서·로딩 그림은 원본 실행 파일의 리소스다. 게임 폴더에 원본 실행 파일이 있으면 자료로 열어 쓴다.
    for (const char* name : {"Netstorm.exe", "NETSTORM.EXE"}) {
        const auto path = options_.gameDirectory / name;
        if (!resources_ && std::filesystem::exists(path))
            resources_ = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    }
    const HMODULE resources = static_cast<HMODULE>(resources_);

    WNDCLASSEXA windowClass{};
    windowClass.cbSize = sizeof windowClass;
    windowClass.style = kWindowClassStyle;
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = static_cast<HINSTANCE>(instance_);
    windowClass.hIcon = resources ? LoadIconA(resources, MAKEINTRESOURCEA(kIconResource)) : LoadIconA(nullptr, IDI_APPLICATION);
    windowClass.hIconSm = resources
        ? static_cast<HICON>(LoadImageA(resources, MAKEINTRESOURCEA(kSmallIconResource), IMAGE_ICON, 16, 16, 0)) : nullptr;
    windowClass.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;
    windowClass.lpszClassName = kWindowClassName;
    if (!RegisterClassExA(&windowClass)) return 0;

    // "init window": 바탕 화면이 게임 화면과 같은 크기면 테두리 없는 창, 아니면 일반 창.
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (GetSystemMetrics(SM_CXSCREEN) == screenWidth_ && GetSystemMetrics(SM_CYSCREEN) == screenHeight_) style = WS_POPUP;
    const HWND window = CreateWindowExA(0, kWindowClassName, "", style, CW_USEDEFAULT, CW_USEDEFAULT,
        kInitialWindowWidth, kInitialWindowHeight, nullptr, nullptr, static_cast<HINSTANCE>(instance_), nullptr);
    if (!window) return 0;
    window_ = window;
    RECT windowRect{}, clientRect{};
    GetWindowRect(window, &windowRect);
    GetClientRect(window, &clientRect);
    // 클라이언트 영역이 화면 크기가 되는 창 크기.
    windowWidth_ = ((clientRect.left - clientRect.right) - windowRect.left) + windowRect.right + screenWidth_;
    windowHeight_ = ((clientRect.top - clientRect.bottom) - windowRect.top) + windowRect.bottom + screenHeight_;
    MoveWindow(window, 0, 0, windowWidth_, windowWidth_, FALSE); // 원본은 높이 자리에도 폭을 넣는다.
    windowDc_ = GetDC(window);
    if (resources) loadingBitmap_ = LoadImageA(resources, MAKEINTRESOURCEA(kLoadingBitmapResource), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    loadingFont_ = CreateFontA(-MulDiv(kLoadingFontPoints, GetDeviceCaps(static_cast<HDC>(windowDc_), LOGPIXELSY), 72),
        0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, 0, 0, FF_SWISS, "Arial");
    // [원본] 레지스트리의 InstallMax·DiagResults 읽기, CD 찾기(FUN_00440c60) — 옮기지 않았다.

    // "init parse options"(FUN_004359f0): 설정을 시작 순서로 읽고 해석한다.
    if (!PumpMessages()) return 0;
    o::ConfigInterface::StartupOptions startup;
    startup.commandLine = options_.settings;
    startup.majorVersion = kMajorVersion;
    startup.minorVersion = options_.edition == o::OriginalEdition::Cd1072 ? kCdMinorVersion : kPatchMinorVersion;
    auto identity = Environment("IDENTITY");
    if (!identity || identity->empty()) identity = Environment("USER");
    startup.identity = identity.value_or(std::string());
    startup.installDir = std::filesystem::absolute(options_.gameDirectory).lexically_normal().string();
    configuration_.Startup(startup);
    SaveOptions(); // 원본 FUN_004359f0 → FUN_00441d10(0): 설정 해석 전에 한 번 저장한다.
    InterpretOptions();
    if (!PumpMessages()) return 0;
    // [원본] 가상 메모리가 부족하면 경고 창을 띄운다 — 옮기지 않았다.
    SetWindowTextA(window, Translate(kWindowTitle).c_str());

    // "init screen" → "init palette": fortPal 이름을 GamePalSpec에 넣은 경로의 팔레트.
    screen_ = std::make_unique<Screen>(window_, windowDc_, screenWidth_, screenHeight_);
    screen_->Init();
    const auto palettePath = configuration_.PathSpec("GamePalSpec", configuration_.PathSpec("fortPal"));
    screen_->LoadPalette(GamePalette(files_.Read(palettePath)));
    // [원본] 설치 검사(GameInstalledTest), 전체 시계·zacket·프로토콜 검사·플레이어 초기화 — 옮기지 않았다.
    clock_ = o::GameClock(timeGetTime()); // "init full time"(FUN_00461090)의 시작 시각.
    // "init dib section"
    screen_->InitDibSection();
    if (!PumpMessages()) return 0;
    // "init fonts"(004a3ce0): 슬롯 표를 만들고 원본의 0번 글꼴을 캐시/GDI로 준비한다.
    std::string fontFace;
    configuration_.ReadString("fontFaceName", fontFace);
    fonts_ = std::make_unique<FontStore>(files_, fontFace);
    fonts_->Get();
    // "init screen mode": startInFullScreen이면 workingFullScreenFlags, 아니면 windowScreenFlags.
    std::uint32_t flags = static_cast<std::uint32_t>(configuration_.GetInt("workingFullScreenFlags"));
    if (configuration_.GetInt("startInFullScreen") == 0 || options_.forceWindow) flags = 0;
    // 원본 00436dd0 → 004dd560(GetFileAttributesA): 남은 표시 파일이 있으면 전체화면 시도를 건너뛴다.
    // 디컴파일은 004395df를 별도 함수로 잘못 읽었다. 실제 분기(004395ad)는 flags=0인 창 모드 경로다.
    if (GetFileAttributesA((options_.gameDirectory / kFullScreenStateFile).string().c_str()) != INVALID_FILE_ATTRIBUTES)
        flags = 0;
    bool modeSet = false;
    // 원본 004395b5: 전체화면 시도 전에 표시 파일을 만든다. 실패하면 원본도 창 모드로 이어 간다.
    if (flags != 0) {
        CreateFullScreenState(options_.gameDirectory);
        modeSet = screen_->SetMode(flags, windowWidth_, windowHeight_, initWindowPos_);
    }
    if (!modeSet) {
        flags = static_cast<std::uint32_t>(configuration_.GetInt("windowScreenFlags"));
        if (!screen_->SetMode(flags, windowWidth_, windowHeight_, initWindowPos_))
            throw std::runtime_error("init screen mode completely failed"); // 원본 로그 + assert.
    }
    // 원본 004a4fe0의 끝: 정해진 모드를 설정에 적는다.
    const auto mode = screen_->Flags();
    configuration_.SetInt("global.ddFullScreen", (mode & ScreenMode::kFullScreen) != 0);
    configuration_.SetInt("global.ddBackBufferDib", (mode & ScreenMode::kBackBufferDib) != 0);
    configuration_.SetInt("global.ddFlipping", (mode & ScreenMode::kFlipping) != 0);
    configuration_.SetInt("global.ddParallax", (mode & ScreenMode::kParallax) != 0);
    configuration_.SetInt("global.ddSoftwareMouse", (mode & ScreenMode::kSoftwareMouse) != 0);
    configuration_.SetInt("global.ddFourPage", 0);
    // [원본] "init sound"(FUN_004aa600) — 옮기지 않았다.
    // "init kernel"(FUN_00471930): Kernel은 멤버로 이미 만들어져 있다.
    // "init renderer": 화면 장치와 같은 크기의 변경 표·프레임 캐시를 만든다.
    renderer_ = std::make_unique<Renderer>(screenWidth_, screenHeight_);
    cursor_ = std::make_unique<Cursor>(resources_, window_);
    if ((mode & ScreenMode::kSoftwareMouse) != 0) cursor_->BuildSoftware(screen_->Palette());
    screen_->ShowCursor(true);
    if (!PumpMessages()) return 0;
    // "init types"(FUN_0049ebb0): 타입·그림·타입 표.
    assets_ = std::make_unique<GameAssets>(files_, options_.edition, palettePath);
    if (!PumpMessages()) return 0;
    // [원본] 송수신기, squid 목록(120000), 좌표 해시, spot 배열, 영상, Guide, 그래프,
    //        전체 명령 표, 인트로 감지, 연락처, 지도 모드, HTTP 서버 — 옮기지 않았다.
    // 현재 UberGump는 단일 플레이 메뉴/브리핑과 저장 미션의 지형·선택·이동 월드를 연결한다.
    if (ready) ready(*this);
    else { menu_ = std::make_unique<UberGump>(*this); if (!options_.mission.empty()) menu_->StartMission(options_.mission); }

    // 메인 루프(docs/exe/main-loop.md 2절). 옮기지 않은 단계는 순서의 자리에 적는다.
    while (true) {
        // 0. 창이 활성이면 설정한 우선순위로, 아니면 보통으로.
        const int wanted = active_ ? std::clamp(highPriority_, 0, 2) : 0;
        const int current = GetThreadPriority(GetCurrentThread());
        const int level = current < 1 ? 0 : current < 3 ? 1 : current == THREAD_PRIORITY_TIME_CRITICAL ? 2 : 0;
        if (level != wanted) SetThreadPriority(GetCurrentThread(), kThreadPriorities[static_cast<std::size_t>(wanted)]);
        // 1. 시각 고정(FUN_00460e90).
        time_ = clock_.Capture(timeGetTime());
        if (menu_) menu_->Tick();
        // 3~6. [원본] 효과음 수 세기, 네트워크 폴링·내보내기 — 옮기지 않았다. 2는 위 UI 전환 부분이다.
        // 7. 쌓인 창 메시지를 모두 처리한다. WM_QUIT이면 끝낸다.
        MSG message;
        // 큐가 빌 때까지 꺼내 창 프로시저로 보낸다.
        while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE | PM_NOYIELD)) {
            if (message.message == WM_QUIT) return static_cast<int>(message.wParam);
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        // 8. [원본] 튜토리얼 안내(FUN_00463f70) — 옮기지 않았다.
        // 9. 입력·명령 처리(FUN_004d62b0). 0이 아니면 종료 절차다.
        if (beforeInput) beforeInput(*this);
        if ((input && input(*this)) || (!input && menu_ && menu_->Input())) {
            screen_->SetMode(ScreenMode::kFallbackWindowed, windowWidth_, windowHeight_, initWindowPos_);
            SendMessageA(window, WM_CLOSE, 0, 0);
            continue;
        }
        // 10. 갱신 목록. 지금은 프로세스 커널만 돈다(FUN_00471a30). 나머지는 옮기지 않았다.
        // 원본 00439ad3 → 00441de0: 설정 변경이 있으면 커널 갱신보다 먼저 저장한다.
        if (configuration_.Configuration().changed) SaveOptions();
        kernel_.RunFrame();
        if (menu_) menu_->Frame();
        // 11. 프레임 제한 + 그리기.
        Frame();
        if (afterFrame) afterFrame(*this);
        // 12. 커서 위치를 읽어 같은 자리에 다시 놓고(원본 그대로, 목적 미확인), 설정한 만큼 쉰다.
        POINT cursor{};
        GetCursorPos(&cursor);
        SetCursorPos(cursor.x, cursor.y);
        if (sleepPerLoop_ != 0) Sleep(static_cast<DWORD>(sleepPerLoop_));
        // 13. [원본] 플레이어별 알림(FUN_00490da0) — 옮기지 않았다.
    }
}

// 원본 00441d10·00441de0의 경로 인자는 빈 문자열(00540cec)·"d"(00540cf8)·"options.cfg"다.
// 원본은 게임 폴더를 작업 폴더로 바꾸고 d\options.cfg를 연다. cpppj는 같은 위치를 절대 경로로 연다.
void Client::SaveOptions() {
    configuration_.SaveFile("d\\options.cfg", [this](std::string_view, std::span<const std::uint8_t> bytes) {
        // 원본 0041b4c0 → 0041a5b0의 모드 2(w+b): 상위 폴더를 새로 만들지 않고 기존 내용을 잘라 쓴다.
        const auto name = std::filesystem::absolute(options_.gameDirectory / "d" / "options.cfg").string();
        FILE* file = std::fopen(name.c_str(), "w+b");
        if (!file) throw std::runtime_error("Unable to open d/options.cfg");
        const auto written = std::fwrite(bytes.data(), 1, bytes.size(), file);
        const int closed = std::fclose(file);
        if (written != bytes.size() || closed != 0) throw std::runtime_error("Unable to write d/options.cfg");
    });
}

// 원본 00436e40은 CRT remove를 부른다. UberGump 004cebf0의 버튼 사건에만 연결되는 함수다.
void Client::ClearFullScreenState() {
    std::remove((options_.gameDirectory / kFullScreenStateFile).string().c_str());
}

// 원본 00435220에서 지금 쓰는 설정만 읽는다. 나머지 키(소리·네트워크·치트·구름 등)는 해당 모듈을 옮길 때 더한다.
void Client::InterpretOptions() {
    std::string language;
    configuration_.ReadString("currentLanguage", language);
    // 언어 번호는 이름 표의 위치다. 1번(english)부터 찾는다.
    for (std::size_t i = 1; i < kLanguages.size(); ++i)
        if (o::AsciiLower(language) == kLanguages[i]) { languageNumber_ = static_cast<int>(i); break; }
    if (!language.empty()) {
        configuration_.LoadLanguage(language); // 원본 FUN_00441db0: 언어 용어표.
        if (const auto bytes = files_.TryRead("d/xlat." + language)) translations_.emplace(o::DecodeOriginalText(*bytes));
    }
    std::int32_t width = 0, height = 0;
    configuration_.ReadInt("SCREENW", width);
    configuration_.ReadInt("SCREENH", height);
    if (width != screenWidth_ || height != screenHeight_) {
        screenWidth_ = width;
        screenHeight_ = height;
        const HWND window = static_cast<HWND>(window_);
        RECT windowRect{}, clientRect{};
        GetWindowRect(window, &windowRect);
        GetClientRect(window, &clientRect);
        windowWidth_ = ((screenWidth_ - clientRect.right) - windowRect.left) + clientRect.left + windowRect.right;
        windowHeight_ = ((screenHeight_ - clientRect.bottom) - windowRect.top) + clientRect.top + windowRect.bottom;
        MoveWindow(window, 0, 0, windowWidth_, windowWidth_, TRUE); // 원본은 높이 자리에도 폭을 넣는다.
    }
    configuration_.ReadInt("highPriorityValue", highPriority_);
    configuration_.ReadInt("initWindowPos", initWindowPos_);
    configuration_.ReadInt("sleepPerLoop", sleepPerLoop_);
    maxFps_ = kDefaultMaxFps;
    configuration_.ReadInt("maxFPS", maxFps_);
    frameInterval_ = FrameIntervalSeconds(maxFps_);
}

// 원본 00436260.
void Client::PaintLoading(NativeHandle dcHandle) {
    const HDC dc = static_cast<HDC>(dcHandle);
    RECT all{0, 0, screenWidth_, screenHeight_};
    FillRect(dc, &all, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    BITMAP bitmap{};
    if (loadingBitmap_ && GetObjectA(static_cast<HBITMAP>(loadingBitmap_), sizeof bitmap, &bitmap) != 0) {
        // 그림을 화면 가운데에 놓는다.
        const HDC memory = CreateCompatibleDC(dc);
        const HGDIOBJ previous = SelectObject(memory, static_cast<HBITMAP>(loadingBitmap_));
        BitBlt(dc, (screenWidth_ - bitmap.bmWidth) / 2, (screenHeight_ - bitmap.bmHeight) / 2, bitmap.bmWidth, bitmap.bmHeight, memory, 0, 0, SRCCOPY);
        SelectObject(memory, previous);
        DeleteDC(memory);
    }
    if (!loadingFont_) return;
    const HGDIOBJ previousFont = SelectObject(dc, static_cast<HFONT>(loadingFont_));
    SetTextAlign(dc, TA_LEFT | TA_TOP);
    SetBkMode(dc, TRANSPARENT);
    SetBkColor(dc, 0);
    const auto loading = Translate(kLoadingText), wait = Translate(kPleaseWaitText);
    SIZE loadingSize{}, waitSize{};
    GetTextExtentPoint32A(dc, loading.c_str(), static_cast<int>(loading.size()), &loadingSize);
    GetTextExtentPoint32A(dc, wait.c_str(), static_cast<int>(wait.size()), &waitSize);
    // 두 줄의 왼쪽을 더 긴 줄에 맞추고 화면 오른쪽 아래에 붙인다.
    const int x = std::min(screenWidth_ - loadingSize.cx, screenWidth_ - waitSize.cx) - kLoadingMarginX;
    SetTextColor(dc, RGB(255, 255, 255));
    TextOutA(dc, x, (screenHeight_ - loadingSize.cy) - waitSize.cy - kLoadingMarginY, loading.c_str(), static_cast<int>(loading.size()));
    TextOutA(dc, x, (screenHeight_ - waitSize.cy) - kLoadingMarginY, wait.c_str(), static_cast<int>(wait.size()));
    SelectObject(dc, previousFont);
}

// 원본 00436450. 메뉴/검사 장면이 준비된 뒤 Renderer의 변경 영역만 그려 내보낸다.
void Client::Frame() {
    // [원본] 창이 비활성이고 전체화면이면 그리지 않는다(FUN_004977e0) — 전체화면을 옮기면 더한다.
    if (!clock_.IsPaused()) {
        // 바쁜 대기: 직전 그리기 뒤로 1 / maxFPS 초가 지날 때까지 벽시계를 다시 읽는다.
        while (clock_.WallSeconds(timeGetTime()) - lastDraw_ < frameInterval_) {}
    }
    lastDraw_ = clock_.WallSeconds(timeGetTime());
    if (sceneVisible_) {
        if ((screen_->Flags() & ScreenMode::kSoftwareMouse) != 0) {
            const auto position = Poll(0); const auto hotspot = cursor_->Hotspot();
            renderer_->SetSoftwareCursor(active_ && (position.code & InputCode::kOutside) == 0 ? cursor_->SoftwareImage() : nullptr,
                {position.x - hotspot.x, position.y - hotspot.y});
        }
        screen_->SetClip(0, 0, screenWidth_, screenHeight_);
        std::uint8_t* buffer = screen_->Lock();
        try {
            renderer_->Draw(std::span<std::uint8_t>(buffer, static_cast<std::size_t>(screen_->Pitch()) * static_cast<std::size_t>(screenHeight_)), screen_->Pitch());
        } catch (...) { screen_->Unlock(); throw; }
        screen_->Unlock();
        renderer_->Present([this](ScreenRect rect) { screen_->Update(rect); });
    }
    ++frames_;
    if (options_.frameLimit != 0 && frames_ == options_.frameLimit) {
        if (!options_.screenshot.empty()) SaveScreenshot();
        PostMessageA(static_cast<HWND>(window_), WM_CLOSE, 0, 0);
    }
}

// 현재 화면 버퍼를 팔레트로 색을 입혀 저장한다.
void Client::SaveScreenshot() {
    Capture(options_.screenshot);
}
// 프레임 관찰 도구도 종료 화면과 같은 팔레트 변환을 쓴다.
void Client::Capture(const std::filesystem::path& path) {
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(screenWidth_) * static_cast<std::size_t>(screenHeight_) * 4);
    const std::uint8_t* buffer = screen_->Lock();
    const auto palette = screen_->Palette();
    const auto pitch = static_cast<std::size_t>(screen_->Pitch());
    // 줄마다 팔레트 번호를 RGBA로 바꾼다.
    for (std::size_t y = 0; y < static_cast<std::size_t>(screenHeight_); ++y) {
        // 한 줄의 픽셀.
        for (std::size_t x = 0; x < static_cast<std::size_t>(screenWidth_); ++x) {
            const auto color = palette[buffer[y * pitch + x]];
            const auto out = (y * static_cast<std::size_t>(screenWidth_) + x) * 4;
            rgba[out] = color.red; rgba[out + 1] = color.green; rgba[out + 2] = color.blue; rgba[out + 3] = 255;
        }
    }
    screen_->Unlock();
    platform::WriteBitmap(path, static_cast<std::uint32_t>(screenWidth_), static_cast<std::uint32_t>(screenHeight_), rgba);
}

// UI는 정지 상태를 한 번만 적용한다. wall 시각은 계속 흐른다.
void Client::Pause(bool paused) {
    if (paused && !clock_.IsPaused()) clock_.Pause(timeGetTime());
    else if (!paused && clock_.IsPaused()) clock_.Resume(timeGetTime());
}
// 자동 검사는 표시 단계와 실제 시계 정지를 함께 검사한다.
bool Client::Paused() const { return clock_.IsPaused(); }
// 원본 창 제목을 ANSI 바이트로 적용한다(한국어는 후순위).
void Client::Title(std::string_view title) { SetWindowTextA(static_cast<HWND>(window_), std::string(title).c_str()); }
// 기본 UI만 제공한다. 검사 장면의 독립 실행은 기존 연결점을 보존한다.
UberGump* Client::Menu() { return menu_.get(); }
// 현재 단계의 창 모드 세 해상도만 지원한다. 전체화면 장치는 후속이다.
void Client::ChangeResolution(int width, int height) {
    if (!((width == 640 && height == 480) || (width == 800 && height == 600) || (width == 1024 && height == 768)))
        throw std::invalid_argument("Unsupported original resolution");
    if (screenWidth_ == width && screenHeight_ == height) return;
    std::array<ScreenColor, 256> palette{};
    std::copy(screen_->Palette().begin(), screen_->Palette().end(), palette.begin());
    sceneVisible_ = false;
    renderer_.reset(); screen_.reset();
    screenWidth_ = width; screenHeight_ = height;
    screen_ = std::make_unique<Screen>(window_, windowDc_, width, height);
    screen_->Init(); screen_->InitDibSection();
    screen_->SetPalette(0, 256, palette.data(), true);
    RECT rect{0, 0, width, height};
    AdjustWindowRectEx(&rect, static_cast<DWORD>(GetWindowLongPtrA(static_cast<HWND>(window_), GWL_STYLE)), FALSE, 0);
    windowWidth_ = rect.right - rect.left; windowHeight_ = rect.bottom - rect.top;
    if (!screen_->SetMode(ScreenMode::kFallbackWindowed, windowWidth_, windowHeight_, initWindowPos_))
        throw std::runtime_error("Unable to change window resolution");
    renderer_ = std::make_unique<Renderer>(width, height);
    sceneVisible_ = true;
    configuration_.SetInt("SCREENW", width); configuration_.SetInt("SCREENH", height);
}

// 원본 00435b30. 원본은 WM_QUIT을 받으면 곧바로 프로세스를 끝낸다(_exit).
bool Client::PumpMessages() {
    MSG message;
    // 쌓인 메시지를 모두 처리한다.
    while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE | PM_NOYIELD)) {
        if (message.message == WM_QUIT) return false;
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return true;
}

// 원본 00436020: 글자가 되는 키는 WM_CHAR에서 넣으므로 여기서는 건너뛴다.
void Client::KeyEvent(NativeHandle windowHandle, unsigned message, std::uintptr_t wParam, std::intptr_t lParam) {
    const HWND window = static_cast<HWND>(windowHandle);
    const std::uint32_t release = message == WM_KEYUP || message == WM_MBUTTONUP || message == WM_SYSKEYUP ? InputCode::kRelease : 0;
    const std::uint32_t modifiers = ModifierBits();
    POINT cursor{};
    RECT rect{};
    GetCursorPos(&cursor);
    GetWindowRect(window, &rect);
    const auto border = screen_ ? screen_->BorderOffset() : ScreenPoint{};
    const std::int32_t x = cursor.x - (border.x + rect.left), y = cursor.y - (border.y + rect.top);
    BYTE keyboard[256];
    GetKeyboardState(keyboard);
    WORD characters[2] = {0, 0};
    const UINT scan = (static_cast<UINT>(lParam >> 16) & 0xff) | (release != 0 ? 0x80000000u : 0u);
    const int count = ToAscii(static_cast<UINT>(wParam), scan, keyboard, characters, 0);
    // [원본] Esc가 글자가 되지 않는 자판(DAT_0054db58 == 3)에서는 글자 0x1b로 넣는다 — 자판 판별을 옮기지 않아 생략.
    if (!(wParam == VK_ESCAPE && count == 0) && count == 1) return;
    input_.Push((static_cast<std::uint32_t>(wParam) << InputCode::kVirtualKeyShift) | modifiers | release, x, y);
}
// 원본 00436190: 글자 코드를 아래 비트에 넣는다.
void Client::CharacterEvent(NativeHandle windowHandle, std::uintptr_t wParam) {
    POINT cursor{};
    RECT rect{};
    GetCursorPos(&cursor);
    GetWindowRect(static_cast<HWND>(windowHandle), &rect);
    const auto border = screen_ ? screen_->BorderOffset() : ScreenPoint{};
    input_.Push(ModifierBits() | static_cast<std::uint32_t>(wParam), cursor.x - (border.x + rect.left), cursor.y - (border.y + rect.top));
}

// 원본 00452e00.
InputEvent Client::Poll(std::uint32_t code) const {
    const std::uint32_t kept = code & 0x43ffffffu;
    const SHORT state = GetAsyncKeyState(static_cast<int>(kept) >> 16);
    // 지금 눌려 있지 않으면 뗌 표시, 지난 확인 뒤에 눌렸으면 0x40000000.
    std::uint32_t result = (~(static_cast<std::uint32_t>(static_cast<std::int32_t>(state)) << 16) & InputCode::kRelease)
        | ((static_cast<std::uint32_t>(state) & 1u) << 30) | kept;
    POINT cursor{};
    RECT rect{};
    GetCursorPos(&cursor);
    GetWindowRect(static_cast<HWND>(window_), &rect);
    const auto border = screen_ ? screen_->BorderOffset() : ScreenPoint{};
    if (cursor.x < border.x + rect.left || rect.right < cursor.x || cursor.y < border.y + rect.top || rect.bottom < cursor.y)
        result |= InputCode::kOutside;
    if (!active_) result |= InputCode::kOutside;
    // 좌표는 화면 안으로 넣는다(0 ~ 폭, 0 ~ 높이).
    const std::int32_t x = std::clamp<std::int32_t>(cursor.x - border.x - rect.left, 0, screenWidth_);
    const std::int32_t y = std::clamp<std::int32_t>(cursor.y - border.y - rect.top, 0, screenHeight_);
    return {result, x, y};
}

// 원본 00436550. 원본의 중첩된 범위 비교를 switch로 풀어 썼다. 메시지별 처리 내용은 원본과 같다.
std::intptr_t Client::HandleMessage(NativeHandle windowHandle, unsigned message, std::uintptr_t wParam, std::intptr_t lParam) {
    const HWND window = static_cast<HWND>(windowHandle);
    // [원본] 등록 메시지 "NETSTORMOOGABOOGA"(설정 도구와의 통신) — 옮기지 않았다.
    switch (message) {
    case WM_KEYDOWN:
    case WM_KEYUP:
        KeyEvent(window, message, wParam, lParam);
        break;
    case WM_DESTROY:
        // [원본] FUN_004b3490: 진행 중인 게임을 정리한다 — 옮기지 않았다.
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        clientWidth_ = LOWORD(lParam);
        clientHeight_ = HIWORD(lParam);
        break;
    case WM_ACTIVATE:
        // [원본] 전체화면에서 비활성이 되면 창을 최소화한다 — 전체화면을 옮기면 더한다.
        // 클릭으로 활성화됐으면 그 클릭은 게임 입력으로 넣지 않는다(제목 표시줄 클릭은 예외).
        activatedByClick_ = LOWORD(wParam) == WA_CLICKACTIVE && !nonClientClick_;
        nonClientClick_ = false;
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT paint;
            const HDC dc = BeginPaint(window, &paint);
            if (sceneVisible_ && renderer_) renderer_->InvalidateAll();
            else PaintLoading(dc);
            EndPaint(window, &paint);
        }
        break;
    case WM_CLOSE:
        closing_ = true;
        if (screen_) screen_->Clear({0, 0, screenWidth_, screenHeight_}, 0);
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_ACTIVATEAPP:
        active_ = wParam != 0;
        if (!active_) {
            Sleep(0);
            return DefWindowProcA(window, message, 0, lParam);
        }
        // [원본] FUN_004a1470: 전체화면이면 DirectDraw 표면을 되살린다 — 옮기지 않았다.
        break;
    case WM_SETCURSOR:
        if (cursor_ && LOWORD(lParam) == HTCLIENT) {
            if (screen_ && (screen_->Flags() & ScreenMode::kSoftwareMouse) != 0) SetCursor(nullptr);
            else cursor_->Apply();
            return TRUE;
        }
        break;
    case WM_GETMINMAXINFO:
        if (windowWidth_ != -1) {
            // 창을 화면 크기에 맞춘 한 가지 크기로만 둔다.
            auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMaxSize = {windowWidth_, windowHeight_};
            info->ptMaxTrackSize = {windowWidth_, windowHeight_};
            return 0;
        }
        break;
    case WM_NCLBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
        nonClientClick_ = GetActiveWindow() != window;
        break;
    case WM_SYSCOMMAND:
        if (wParam == SC_KEYMENU) return 0; // Alt가 메뉴를 열지 않게 한다.
        break;
    case WM_CHAR:
        CharacterEvent(window, wParam);
        break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        if ((lParam & 0x20000000) != 0) { // Alt와 함께 눌린 키.
            if (wParam != VK_MENU) KeyEvent(window, message, wParam, lParam);
            return 0;
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == kMenuExit) { DestroyWindow(window); return 0; }
        // [원본] 0x9c45: 소리 켜고 끄기 — 소리를 옮기면 더한다.
        break;
    case WM_QUERYNEWPALETTE:
    case WM_PALETTECHANGED:
        if (message == WM_PALETTECHANGED && reinterpret_cast<HWND>(wParam) == window) break;
        // [원본] 창 모드에서 논리 팔레트를 다시 고르고 창을 다시 그리게 한다. 256색 화면에서만 뜻이 있다.
        if (screen_) InvalidateRect(window, nullptr, TRUE);
        return 1;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        if (activatedByClick_) { // 창을 활성화한 클릭은 삼킨다.
            activatedByClick_ = false;
            nonClientClick_ = false;
            break;
        }
        [[fallthrough]];
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP: {
        const bool release = message == WM_LBUTTONUP || message == WM_RBUTTONUP || message == WM_MBUTTONUP;
        const std::uint32_t button = message == WM_LBUTTONUP || message == WM_LBUTTONDOWN ? InputCode::kLeftButton
            : message == WM_RBUTTONUP || message == WM_RBUTTONDOWN ? InputCode::kRightButton : InputCode::kMiddleButton;
        input_.Push(button | ModifierBits() | (release ? InputCode::kRelease : 0u), LOWORD(lParam), HIWORD(lParam));
        break;
    }
    default:
        break;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

// 화면 장치. Run이 만든 뒤에만 쓴다.
Screen& Client::GetScreen() { return *screen_; }
// 입력 큐.
InputQueue& Client::Input() { return input_; }
// 설정.
o::ConfigInterface& Client::Configuration() { return configuration_; }
// 파일 시스템.
const o::BaseFileSystem& Client::Files() const { return files_; }
// 타입·그림. Run이 "init types"에서 만든 뒤에만 쓴다.
const GameAssets& Client::Assets() const { return *assets_; }
// 프로세스 커널.
o::Kernel& Client::GetKernel() { return kernel_; }
// 원본 변경 영역 기반의 Renderer.
Renderer& Client::GetRenderer() { return *renderer_; }
// 원본 글꼴 슬롯.
FontStore& Client::Fonts() { return *fonts_; }
// 원본 커서 번호와 프레임.
Cursor& Client::GetCursor() { return *cursor_; }
// 통계 출력은 창을 초기화 도중 닫았어도 아직 없는 Renderer를 참조하지 않는다.
std::pair<std::uint64_t, std::uint64_t> Client::RenderCounts() const {
    return renderer_ ? std::make_pair(renderer_->DrawCount(), renderer_->PresentCount()) : std::make_pair(std::uint64_t{0}, std::uint64_t{0});
}
// 준비된 장면을 로딩 화면 대신 표시하고 남아 있는 창 영역도 검증한다.
void Client::ShowScene() { sceneVisible_ = true; renderer_->InvalidateAll(); InvalidateRect(static_cast<HWND>(window_), nullptr, FALSE); }
// 이번 프레임의 시각.
const o::FrameTime& Client::Time() const { return time_; }
// 창이 활성인가.
bool Client::Active() const { return active_; }
// 번역표가 없거나 영어면 원문 그대로다.
std::string Client::Translate(std::string_view text) const {
    return translations_ ? translations_->Translate(text, languageNumber_) : std::string(text);
}

}  // namespace netstorm::client
