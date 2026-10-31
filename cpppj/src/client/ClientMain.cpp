// 원본 소스: ClientMain.cpp (클라이언트 폴더) — 프로그램 진입점과 메인 루프. 원본처럼 Win32를 직접 부른다.
// 원본: FUN_00438dc0(WinMain) ↔ CD FUN_00485b20, FUN_00436550(창 프로시저), FUN_00436450(프레임) ↔ CD FUN_00486df0,
//       FUN_00436260(로딩 화면), FUN_00436020·FUN_00436190(키·글자 사건), FUN_00435220(설정 해석) ↔ CD FUN_00484ac0,
//       FUN_00435b30(메시지 처리), FUN_00452e00(입력 폴링).
// 범위: 창 만들기, 설정 읽기, 화면 장치, 메시지·입력 큐, 프레임 제한과 내보내기까지.
//       Renderer·원본 글꼴/커서·메뉴와 저장 미션의 기본 지형/객체·선택/이동을 연결했다. 건설/전투는 후속이다.
#include "client/ClientMain.h"
#include "client/RawCampaignSession.h" //Sleep 뒤의 실제 프레임말 Player 결과 소비에 장기 Session을 빌린다.
#include "client/ClientDiagnostics.h"
#include "client/ClientOptionsFile.h"
#include "client/CdDirectory.h"
#include "client/ClientBridgeDecay.h"
#include "client/GifScreenshot.h"
#include "o/RawColorSortPrimary.h"
#include "client/ClientAudio.h"
#include "client/PaletteFlash.h"
#include "client/ThunderProcess.h"
#include "client/RawProcessHostStorage.h"
#include "client/RawChatChannels.h"
#include "client/FortTypeConversionStorage.h"
#include "client/ClientRandomGlobals.h"
#include "client/ClientSpGlobals.h"
#include "client/ClientKencloudGlobals.h"
#include "client/ClientRendererFrame.h"
#include "client/ClientSubscriberGlobals.h"
#include "client/ClientColorSortInput.h"
#include "client/OwnerColorTableState.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientCdGlobals.h"
#include "client/ClientConnectionActivityGlobals.h"
#include "client/ClientConnectionGlobals.h"
#include "client/ClientFortGlobals.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientPlayerRuntime.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientNsInfoGlobals.h"
#include "client/ClientChatWindowGlobals.h"
#include "client/ClientHtmlWindowGlobals.h"
#include "client/ClientChatManagerGlobals.h"
#include "client/ClientChatRuntime.h"
#include "client/HudChatGeometry.h" //프레임말 입력줄 요청의 원문 소비 순서다.
#include "client/ClientStaticStrings.h"
#include "client/ClientDotCommandGlobals.h"
#include "client/RawSurfaceWorld.h"
#include "client/UberGump.h"
#include "o/OriginalText.h"
#include "o/RawFortBadgesGlobals.h"
#include "platform/Bitmap.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
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

// 원본과 다른 Windows10/11 호환 출력은 DPI 가상 좌표 대신 실제 모니터 픽셀과 입력을 사용한다.
void EnableModernScreenDpi() {
    using SetAwareness=BOOL(WINAPI*)(HANDLE); // Windows10의 DPI 컨텍스트 진입 API이며 구버전에서는 조회만 실패한다.
    const auto user=GetModuleHandleW(L"user32.dll");const auto set=reinterpret_cast<SetAwareness>(GetProcAddress(user,"SetProcessDpiAwarenessContext"));
    if (set && set(reinterpret_cast<HANDLE>(-4))) return; // PER_MONITOR_AWARE_V2: 모니터별 실제 픽셀이다.
    SetProcessDPIAware(); // 초기 Windows10/이미 설정된 프로세스의 시스템 DPI 호환 관문이다.
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

// 원본00558ed0의 정적100기록을 Client 수명으로 소유하며 읽기는 항상 현재 월드까지 내려간다.
struct Client::ChatState {
    std::array<std::uint8_t,RawChatChannels::kBytes> records{}; // 원본의0초기화된 단일4800바이트이며 미션 교체로 재초기화하지 않는다.
    std::optional<RawChatChannels> channels; // PATCH 몸체만 구성한다. 미확정 CD 동작을 PATCH로 대신하지 않는다.
    // 현재 subscriber를 복사하지 않고 Client의 실제 설치 경로를 매 Add 첫 비교 때 조회한다.
    ChatState(Client& client,o::OriginalEdition edition) {
        if (edition==o::OriginalEdition::Patch1078)
            channels.emplace(edition,records,[&client] { return client.CurrentSubscriber(); });
    }
};
// 파일 시스템·설정·전역 Chat 저장소만 만든다. 창과 화면은 Run에서 원본 순서대로 만든다.
Client::Client(ClientOptions options)
    : options_(std::move(options)), files_(options_.gameDirectory),
      configuration_([this](std::string_view path) { return files_.TryRead(path); }),
      fortBadges_(std::make_shared<o::RawFortBadgesGlobals>()),
      fortTypeConversion_(std::make_shared<FortTypeConversionStorage>()),
      subscriberGlobals_(std::make_shared<ClientSubscriberGlobals>()),
      randomGlobals_(std::make_shared<ClientRandomGlobals>()),
      spGlobals_(std::make_shared<ClientSpGlobals>(randomGlobals_,ClientCurrentUnixSeconds)),
      kencloudGlobals_(std::make_shared<ClientKencloudGlobals>()),
      colorSortInput_(std::make_shared<ClientColorSortInput>()),
      localStateGlobals_(std::make_shared<ClientLocalStateGlobals>()),
      cdGlobals_(std::make_shared<ClientCdGlobals>()),
      connectionActivityGlobals_(std::make_shared<ClientConnectionActivityGlobals>()),
      connectionGlobals_(std::make_shared<ClientConnectionGlobals>(localStateGlobals_)),
      fortGlobals_(std::make_shared<ClientFortGlobals>()),
      coreDataGlobals_(std::make_shared<ClientCoreDataGlobals>()),
      playerRecords_(std::make_shared<ClientPlayerRecords>()),
      gameplayGlobals_(std::make_shared<ClientGameplayGlobals>(subscriberGlobals_)),
      htmlWindowGlobals_(std::make_shared<ClientHtmlWindowGlobals>()),
      chatWindowGlobals_(std::make_shared<ClientChatWindowGlobals>()),
      nsInfoGlobals_(std::make_shared<ClientNsInfoGlobals>()),
      chatManagerGlobals_(std::make_shared<ClientChatManagerGlobals>()),
      staticStrings_(std::make_shared<ClientStaticStrings>()),
      dotCommandGlobals_(std::make_shared<ClientDotCommandGlobals>()),
      chat_(std::make_unique<ChatState>(*this,options_.edition)),trace_(std::make_unique<ClientTraceGlobals>()) {
    files_.RegisterArchives(); //원본441790처럼 기본 폴더의 모든*.tarc를 Win32 반환 순서로 등록한다.
    rendererFrame_=std::make_unique<ClientRendererFrame>(*this); //생성은 BSS0 owner/참조만 준비하며 아직 화면/파일/난수를 사용하지 않는다.
    playerRecords_->BindCoreData(coreDataGlobals_); // 같은 Native Player base가 바뀌면 실제 Core의 빌린 현재 Player 슬롯도 즉시 갱신한다.
    configuration_.Registry().environment = Environment;
    if (g_client) throw std::logic_error("Only one client may exist");
    // 원본00558dd0의 Set 직후 동기 시점에 현재 복원된 설정 전역을 갱신한다.
    configuration_.onChanged = [this] { configuration_.ReadInt("inputLineOff",chatWindowGlobals_->inputLineOff);InterpretFortFileOptions();configuration_.ReadInt("tracelevel",trace_->log.level);InterpretScriptMenuOptions();kencloudGlobals_->ReadOptions(configuration_);rendererFrame_->ReadOptions();InterpretMainMenuOptions();InterpretOwnerColorOptions(); }; //같은 설정 변경에서 입력줄요청/저장/진단/메뉴/구름·tellTips·배너 현재값을 동기 반영하며 새 장치를 만들지 않는다.
    g_client = this;
}
// 월드/소리 프로세스→음악·음향→화면→GDI/창 순서로 참조 대상보다 사용자를 먼저 정리한다.
Client::~Client() {
    configuration_.onChanged = {}; // 부분 반납 중인 Client를 콜백으로 다시 읽지 않게 먼저 해제한다.
    menu_.reset();
    rendererFrame_.reset(); //현재 Frame의 표시 작업 배열/Pool 대여를 화면·원문 물리 소유자보다 먼저 반납한다.
    ClearThunderFlashes();
    audio_.reset(); // 월드의 반복 소리를 멈춘 뒤 음악 스레드를 합류하고 장치를 닫는다.
    disabledAudio_.reset(); // noAudio 이름표도 같은 월드 프로세스 해제 이후에 없앤다.
    kencloudGlobals_->BindRenderer(nullptr);renderer_.reset(); fonts_.reset(); cursor_.reset(); // 실제 장치 반납 전에 현재 opaque32 연결도 해제한다.
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
    if (options_.edition==o::OriginalEdition::Patch1078) staticStrings_->Initialize(10240); // 보호PE00438fe0의 실제0x2800→004b92b0이며 첫 경로/설정 해석보다 앞선다.
    // [원본] CreateMutexA("TitanicNetStormMutex"): 먼저 뜬 실행인지 기록한다(네트워크용). 원본 게임과 같이 띄울 때
    //        서로 영향을 주지 않도록 아직 만들지 않는다.
    // [원본] 명령줄이 "window"면 전체화면 설정을 무시한다 → options_.forceWindow.
    // [원본] FUN_00435cd0: 명령줄이 있으면 그것을, 없으면 실행 파일 위치를 게임 폴더로 삼는다 → options_.gameDirectory.
    // [원본] CD-ROM에서 직접 실행하면 안내 창을 띄우고 끝낸다 — 옮기지 않았다.
    instance_ = GetModuleHandleA(nullptr);
    EnableModernScreenDpi(); // 원본에 없던 현대 Windows DPI 처리이며 첫 HWND 생성 전에만 실행한다.
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
    if (options_.edition==o::OriginalEdition::Patch1078) spGlobals_->Store().Initialize(); //004391ed의 실제 공통 SP 초기화이며 옵션/화면/구름/Session보다 먼저 같은 장기 SP를 구성한다.
    GetClientRect(window, &clientRect);
    // 클라이언트 영역이 화면 크기가 되는 창 크기.
    windowWidth_ = ((clientRect.left - clientRect.right) - windowRect.left) + windowRect.right + screenWidth_;
    windowHeight_ = ((clientRect.top - clientRect.bottom) - windowRect.top) + windowRect.bottom + screenHeight_;
    MoveWindow(window, 0, 0, windowWidth_, windowWidth_, FALSE); // 원본은 높이 자리에도 폭을 넣는다.
    windowDc_ = GetDC(window);
    if (resources) loadingBitmap_ = LoadImageA(resources, MAKEINTRESOURCEA(kLoadingBitmapResource), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    loadingFont_ = CreateFontA(-MulDiv(kLoadingFontPoints, GetDeviceCaps(static_cast<HDC>(windowDc_), LOGPIXELSY), 72),
        0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, 0, 0, FF_SWISS, "Arial");
    // [원본] 레지스트리의 InstallMax·DiagResults 읽기는 별도 잔여다. CDDir 재귀와 다른 시작 CD 결과만 원문 위치에 기록한다.
    if (options_.edition==o::OriginalEdition::Patch1078) cdGlobals_->InitializeStartup(MakeWindowsCdPresenceHooks()); //004392ed: 옵션/파일 해석 전에 같은 Client 캐시를 검사하고 별도0054db6c에 결과를 저장한다.

    // "init parse options"(FUN_004359f0): 설정을 시작 순서로 읽고 해석한다.
    if (!PumpMessages()) return 0;
    o::ConfigInterface::StartupOptions startup;
    startup.commandLine = options_.settings;
    startup.majorVersion = kMajorVersion;
    startup.minorVersion = options_.edition == o::OriginalEdition::Cd1072 ? kCdMinorVersion : kPatchMinorVersion;
    auto identity = Environment("IDENTITY");
    if (!identity || identity->empty()) identity = Environment("USER");
    startup.identity = identity.value_or(std::string());
    const auto installUtf8=std::filesystem::absolute(options_.gameDirectory).lexically_normal().u8string();startup.installDir.assign(installUtf8.begin(),installUtf8.end()); // 원본ANSI와 달리 Windows10/11의 한글 경로를 유효 UTF-8 설정 값으로 전달한다.
    startup.cdDir=FindWindowsCdDirectory(); // 원문00440d40의 CD-ROM/깊이2 탐색 결과다. 부재는 기존 InstallDir fallback이며 W API만 Windows10/11 호환 경계다.
    InitializeNativeSecondaryFiles(startup.cdDir); //설정 파일을 읽기 전에 원문41bc30의 보조 TAFF와41a4f0의 실제 root를 연결한다.
    configuration_.Startup(startup);
    SaveOptions(); // 원본 FUN_004359f0 → FUN_00441d10(0): 설정 해석 전에 한 번 저장한다.
    InterpretOptions();
    if (!PumpMessages()) return 0;
    // [원본] 가상 메모리가 부족하면 경고 창을 띄운다 — 옮기지 않았다.
    SetWindowTextA(window, Translate(kWindowTitle).c_str());

    // "init screen" → "init palette": fortPal 이름을 GamePalSpec에 넣은 경로의 팔레트.
    screen_ = std::make_unique<Screen>(window_, windowDc_, screenWidth_, screenHeight_,&paletteLoaded_);
    screen_->Init();
    const auto palettePath = configuration_.PathSpec("GamePalSpec", configuration_.PathSpec("fortPal"));
    screen_->LoadPalette(GamePalette(files_.Read(palettePath)),options_.edition);
    // [원본] 설치 검사(GameInstalledTest)·zacket·프로토콜 검사는 아직 연결하지 않았다.
    clock_ = o::GameClock(timeGetTime()); // "init full time"(FUN_00461090)의 시작 시각.
    if (options_.edition==o::OriginalEdition::Patch1078) InitializeClientPlayerRecords(*this); //0043952c의 실제 초기화이며 그 다음00439537 ColorSort 입력보다 앞선다.
    //00439537의초기ColorSort입력위치다.이캡처만으로미연결43b430검색/색띠전체를성공시킨것은아니다.
    colorSortInput_->CaptureWithPrimary(screen_->LogicalPalette());
    screen_->BindColorSortInput([owner=colorSortInput_] { return owner->Logical(); });
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
    // 원본과 다른 호환 DIB는 독점 DirectDraw 복구 표식을 만들지 않으며 남은 옛 표식 때문에 다음 실행을 창 모드로 강제하지 않는다.
    ClearFullScreenState();
    bool modeSet = false;
    if (flags != 0) {
        modeSet = screen_->SetMode(flags, windowWidth_, windowHeight_, initWindowPos_);
    }
    if (!modeSet) {
        flags = static_cast<std::uint32_t>(configuration_.GetInt("windowScreenFlags"));
        if (!screen_->SetMode(flags, windowWidth_, windowHeight_, initWindowPos_))
            throw std::runtime_error("init screen mode completely failed"); // 원본 로그 + assert.
    }
    // 원본 004a4fe0의 끝: 정해진 실제 모드를 설정에 적는다. --window는 시작 시에만 적용하고 저장된 사용자 선택을 바꾸지 않는다.
    const auto mode = screen_->Flags();
    PublishScreenMode();
    InitializeNativeAudio(); // 원본 init sound의 기존 위치에서 같은 본체를 실행한다.
    // "init kernel"(FUN_00471930): Kernel은 멤버로 이미 만들어져 있다.
    gameplayGlobals_->WorldCells()=256; //00438dc0/004395df의 실제 kernel 뒤 명시00531928 store다. Session 생성에서 다시 쓰지 않는다.
    // "init renderer": 화면 장치와 같은 크기의 변경 표·프레임 캐시를 만든다.
    renderer_ = std::make_unique<Renderer>(screenWidth_, screenHeight_,options_.edition);
    kencloudGlobals_->BindRenderer(renderer_.get()); // Native renderer 존재 관문은 실제 현재 장치와 같은 수명을 사용한다.
    InitializeNativeRendererView();
    cursor_ = std::make_unique<Cursor>(resources_, window_);
    if ((mode & ScreenMode::kSoftwareMouse) != 0) cursor_->BuildSoftware(screen_->Palette());
    screen_->ShowCursor(true);
    if (!PumpMessages()) return 0;
    // 초기 메뉴가 준비되기 전 현재 창에 원본147/Arial20 로딩을 확정 출력한다. 표시 여부가 WM_PAINT 예약만의 타이밍에 의존하지 않게 하며 별도 대기시간은 두지 않는다.
    InvalidateRect(window,nullptr,FALSE);UpdateWindow(window);GdiFlush();DwmFlush(); // 유효한 WM_PAINT 갱신을 실행한 뒤 Windows 합성 화면의 다음 표시까지 완료하여 빠른 메뉴 교체가 로딩을 덮지 않게 한다.
    // "init types"(FUN_0049ebb0): 타입·그림·타입 표.
    assets_ = std::make_unique<GameAssets>(files_, options_.edition, palettePath);
    if (options_.edition==o::OriginalEdition::Patch1078) colorSortInput_->OwnerTables()->Initialize(colorSortInput_->Primary(),assets_->TypeTable().Types().size(),static_cast<std::uint32_t>(assets_->Find("bridge").block+o::kFirstAssetTypeNumber),kencloudGlobals_->WorldRedraw()); // 원문43c4e0→43be30의 최초 등록이다. 호스트는 TYPE 크기가 확정된 직후, 첫 UI/World보다 먼저 수행한다.
    surfaceStorage_=MakeRawSurfaceStorage(*assets_,gameplayGlobals_->Authority(),gameplayGlobals_->GameAux()); // init squid/hash/spot의 실제 장기 물리를 메뉴보다 먼저 확보한다.
    surfaceStorage_->Processes().BindDevices(kernel_,time_); // 첫 메뉴/World보다 먼저 실제 같은 Kernel/현재 프레임을 장기 Host에 연결한다.
    const auto serverPath=configuration_.SubstituteLocal("{DataDir}/!rootservers.dat");const auto serverFile=files_.TryRead(serverPath); // 현재 보호 시작 목록을 읽으며 네트워크 갱신/저장 부모와 구별한다.
    if (serverFile) nsInfoGlobals_->LoadServerNames(*serverFile);else if (const auto original=files_.TryRead(configuration_.SubstituteLocal("{DataDir}/rootservers.dat"))) nsInfoGlobals_->LoadServerNames(*original);
    if (!PumpMessages()) return 0;
    // [원본] 송수신기, squid 목록(120000), 좌표 해시, spot 배열, 영상, Guide, 그래프,
    //        전체 명령 표, 인트로 감지, 연락처, 지도 모드, HTTP 서버 — 옮기지 않았다.
    // 원본 00438dc0 "loadContactList" 다음의 FUN_00469fc0: 전투 밖이므로 메뉴 곡 ser22.mus로 시작한다.
    if (options_.edition==o::OriginalEdition::Patch1078) InitializeClientIssueCommands(*this); //004397f5의 네 초기 등록은 뒤 map mode의 format/back/front보다 먼저 실행한다.
    if (audio_) audio_->StartScene(false);
    if (options_.edition==o::OriginalEdition::Patch1078) InitializeClientChatManager(*this); //00439840의 실제 map mode 시작이며 메뉴/Session 생성보다 먼저 같은 현재 manager/명령표를 구성한다.
    fortBadges_->ResetCounts(); // 원본0043984a는 Chat 시작 다음의 같은 장부 count reset이며 매 Session 생성 때 반복하지 않는다.
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
        *time_ = clock_.Capture(timeGetTime());
        if (menu_) menu_->Tick();
        // 3. 재생 중인 효과음 수를 다시 센다(FUN_004a8e60). 2는 위 실제 상태 전환 부분이다.
        if (audio_) audio_->CountPlaying();
        if (menu_) menu_->PollMissionPackets(); //4의 원문 위치에서 로컬 미션/Editor 큐를 진행한다. 외부 송수신기/5~6의 시간 동기화·주기 내보내기는 별도 MP 복원 범위다.
        // 7. 쌓인 창 메시지를 모두 처리한다. WM_QUIT이면 끝낸다.
        MSG message;
        // 큐가 빌 때까지 꺼내 창 프로시저로 보낸다.
        while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE | PM_NOYIELD)) {
            if (message.message == WM_QUIT) return static_cast<int>(message.wParam);
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        // 8. 원본 튜토리얼 안내(FUN_00463f70)의 현재 프레임을 입력보다 먼저 실행한다.
        if (menu_) menu_->MissionFrame();
        // 9. 입력·명령 처리(FUN_004d62b0). 0이 아니면 종료 절차다.
        if (beforeInput) beforeInput(*this);
        if ((input && input(*this)) || (!input && menu_ && menu_->Input())) {
            screen_->SetMode(ScreenMode::kFallbackWindowed, windowWidth_, windowHeight_, initWindowPos_);
            SendMessageA(window, WM_CLOSE, 0, 0);
            continue;
        }
        // 10. 원본439abe는 실제 Moviegump가 소유하는54dc58의 양수만 검사한다. 게임 시계 정지와 구별하며 영상 중에도 아래 idle은 실행한다.
        if (movieUpdatePauseDepth_<=0) {
            // 원본439ad3의 설정 저장은439b13의 가상 소멸/OnExit 예약보다 먼저다.
            if (configuration_.Configuration().changed) SaveOptions();
            RunClientBridgeDecay(*this); //00422bc0은 메뉴에서도 실행하며 위치 소리469f60보다 먼저 현재 다리 수명/파괴를 처리한다.
            if (audio_) audio_->SceneFrame(); //469f60은 미니맵/Gump/Kernel보다 앞이다.
            if (menu_) menu_->RefreshMinimap(); //481320은 게임 시계 정지와 무관하다.
            if (menu_) menu_->CleanupGumps(); //439b13→4662b0: 이 단계가 예약한 입력은 다음 Input에서만 소비한다.
            if (menu_) menu_->IdleGumps(); //439b18→464170: 정리 뒤·Kernel 앞의 같은 표 한 번이다.
            if (menu_) menu_->PrepareKernelFrame(); // 같은 실제 시각/연결/AI 입력을 준비하며 Kernel은 호출하지 않는다.
            kernel_->RunFrame();
            if (menu_) menu_->FinishKernelFrame(); // 모든 원문 프로세스가 같은 슬롯 순회를 끝낸 뒤 표시를 수집한다.
        } else if (menu_) menu_->IdleGumps(); //439b69: 영상 갱신 잠금 중에도 Idle만 계속 실행한다.
        if (menu_) menu_->Frame(); // 합성/그리기 경계는 양쪽 가지 모두 유지하며 State Tick과 합치지 않는다.
        // 11. 프레임 제한 + 그리기.
        Frame();
        ConsumeChatInputToggle(chatWindowGlobals_->inputLineOff,[this] { if (menu_) menu_->ToggleChatInput(); }); //원문439b96: 양쪽 갱신 가지의 그리기 뒤/커서·Sleep 앞에서 한 번 전환하고0을 쓴다.
        if (afterFrame) afterFrame(*this);
        // 12. 커서 위치를 읽어 같은 자리에 다시 놓고(원본 그대로, 목적 미확인), 설정한 만큼 쉰다.
        POINT cursor{};
        GetCursorPos(&cursor);
        SetCursorPos(cursor.x, cursor.y);
        if (sleepPerLoop_ != 0) Sleep(static_cast<DWORD>(sleepPerLoop_));
        // 13. 원문439c33의 Sleep 뒤 Player 결과 알림이다. known/battle이 아닌 메인 화면은 Session을 요구하지 않는다.
        if (gameplayGlobals_->player.playerKnown && surfaceStorage_->ownerMode.battle) gameplayGlobals_->RequireSession().NotifyPlayerResults();
    }
}

// 소리·음악 묶음이다. noAudio이거나 초기화 전이면 널이다.
ClientAudio* Client::Audio() { return audio_.get(); }
// 원문00435dc0의 같은 갱신 깊이를32비트로 가감하며 게임 시계와 합치지 않는다.
void Client::PauseMovieUpdates(bool paused) { movieUpdatePauseDepth_=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(movieUpdatePauseDepth_)+(paused ? 1U : 0xffffffffU)); }
// 실제 현재 갱신 깊이를 관측하며 상태를 합성하지 않는다.
std::int32_t Client::MovieUpdatePauseDepth() const { return movieUpdatePauseDepth_; }
// 원문의 종료 음질3 재초기화도 같은 장기 음향/창/예약 음량으로 수행한다.
void Client::RestoreMovieSound(std::uint32_t quality) { if (audio_) { auto values=ReadAudioOptions();values.quality=static_cast<int>(quality);audio_->Initialize(reinterpret_cast<std::uintptr_t>(window_),values); } }
// 두 검사 옵션은 실제 영상 PCM에도 그대로 적용한다.
bool Client::MovieAudioEnabled() const { return !options_.noAudio; }
bool Client::MovieAudioMuted() const { return options_.audioMute; }

// 원본 init sound 단계의 같은 Client 음향을 한 번만 만든다. noAudio는 창·장치 없이 같은 장기 owner를 준비한다.
void Client::InitializeNativeAudio() {
    if (audio_ || disabledAudio_) throw std::logic_error("실제 Client 음향의 중복 초기화");
    if (!options_.noAudio && (!screen_ || !window_)) throw std::logic_error("실제 음향 초기화 전 화면·창 필요");
    // "init sound"(FUN_004aa600 → FUN_004a9520("thunderCrack.wav") → FUN_004aadd0): 소리 장치·음악 스레드.
    // 장치가 없거나 열리지 않으면 소리 없이 계속 실행한다. noAudio도 실제 이름표/재생 계층은 유지한다.
    if (!options_.noAudio) {
        SceneMusicHooks scene;
        // 원본0043dad0의 현재 예산 serial 재부착이다. UI 생성 전에는 등록된 예산도 없다.
        scene.refresh=[this]() { if (menu_) menu_->RefreshStormPower(); };
        // 원본 00441470(key,1)은 값이 정확히 1일 때만 참이다. 음악/소리 옵션과는 별개다.
        scene.ascendancyPalette=[this]() { return configuration_.GetInt(kAscendancyPaletteOption)==1; };
        // 파일 이름에는 이미 .col이 포함돼 있으므로 GamePalSpec의 .COL을 추가하지 않는다.
        scene.loadPalette=[this](std::string_view name) { LoadScenePalette(name); };
        // 원본 00469c80은 천둥 효과음을 내기 전에 번개 프로세스를 생성한다. 소리 옵션과 별개로 진행한다.
        scene.thunderFlash=[this]() { StartThunderFlash(); };
        audio_ = std::make_unique<ClientAudio>(options_.gameDirectory, options_.edition, kLanguages[static_cast<std::size_t>(languageNumber_)],
            [this]() { return clock_.WallSeconds(timeGetTime()); },randomGlobals_->Game(),std::move(scene)); // 원문 전투 음악도 게임/SP/AI와 같은00532710을 소비한다.
        audio_->SetSceneTints(screen_->Colors().weather);
        audio_->SetView({0,0,0,0,screenWidth_,screenHeight_});
        audio_->Initialize(reinterpret_cast<std::uintptr_t>(window_), ReadAudioOptions());
        if (options_.audioMute) audio_->PushMute();
    } else {
        // 생성자/이름표 초기화는 OS 장치와 음악 스레드를 열지 않는다. 실제 Play가 미초기화 상태에서 0을 반환한다.
        disabledAudio_=std::make_unique<ClientAudio>(options_.gameDirectory,options_.edition,kLanguages[static_cast<std::size_t>(languageNumber_)],
            [this] { return clock_.WallSeconds(timeGetTime()); },randomGlobals_->Game()); // 장치 없는 검사도 같은 난수 소비 계약을 유지한다.
        disabledAudio_->Names().Initialize();disabledAudio_->SetView({0,0,0,0,screenWidth_,screenHeight_});
    }
    if (options_.edition==o::OriginalEdition::Patch1078) WorldAudio().BindCurrentView([this] { return CurrentSoundView(); }); // 원문497220/4c6b30은 소리 계산마다 현재 전역을 읽는다. 10.72의 기존 명시 어댑터는 이번 분석 범위에 넣지 않는다.
}

// 현재 위치 소리는 World의 마지막 통지가 아니라 Native snapshot과 같은 실제 Gump 영역을 사용한다.
// 근거: docs/exe/cpp-sound-current-view-20261101.md. 메뉴/State/Guide에서 별도 편의0쓰기를 추가하지 않는다.
SoundView Client::CurrentSoundView() const {
    const auto& pixels=gameplayGlobals_->Camera().Snapshot().pixels;
    const auto clip=menu_ ? menu_->CurrentMapView() : ScreenRect{0,0,screenWidth_,screenHeight_}; // Gump 생성 전에는 같은 초기 전체 화면 입력만 존재한다.
    return {pixels[0],pixels[1],clip.left,clip.top,clip.right,clip.bottom};
}

// 월드가 사용하는 같은 이름표와 실제 SoundPlayer는 소리 옵션/장치 유무와 분리해 유지한다.
ClientAudio& Client::WorldAudio() {
    if (audio_) return *audio_;
    if (disabledAudio_) return *disabledAudio_;
    throw std::logic_error("캠페인 음향 서비스 초기화 전 접근");
}

// 설정 키 이름은 원본 Interpret Options(00435220)가 FUN_00441270으로 읽는 문자열이다(sound·music·soundQuality·soundVolume·musicVolume·maxSimulSounds·swapLeftRightSpeakers).
// 원본 ReadInt는 키가 있고 변수의 현재 값과 다를 때만 변수를 바꾼다. 그래서 변수는 읽기 전에 기본값(setup.cfg·options.cfg의 기본값)을 가져야 하며,
// 0으로 시작해 읽으면 설정이 0인 경우(소리·음악 끄기)를 놓친다.
ClientAudioOptions Client::ReadAudioOptions() {
    ClientAudioOptions result;
    // 현재 값에서 시작해 설정이 있으면 그 값으로 바꿔 돌려준다(없으면 현재 값 그대로).
    const auto read = [this](const char* key, std::int32_t current) { configuration_.ReadInt(key, current); return current; };
    result.sound = read("sound", result.sound ? 1 : 0) != 0;
    result.music = read("music", result.music ? 1 : 0) != 0;
    result.quality = read("soundQuality", result.quality);
    result.soundVolumeStep = read("soundVolume", result.soundVolumeStep);
    result.musicVolumeStep = read("musicVolume", result.musicVolumeStep);
    result.maxSimulSounds = read("maxSimulSounds", result.maxSimulSounds);
    result.swapSpeakers = read("swapLeftRightSpeakers", result.swapSpeakers ? 1 : 0) != 0;
    return result;
}

// 옵션 메뉴가 소리 설정을 바꾼 뒤 부른다. 소리 묶음이 없으면(noAudio) 아무 일도 하지 않는다.
void Client::ApplyAudioOptions() {
    if (audio_) audio_->Interpret(reinterpret_cast<std::uintptr_t>(window_), ReadAudioOptions());
}

// 원본 00441d10·00441de0의 경로 인자는 빈 문자열(00540cec)·"d"(00540cf8)·"options.cfg"다.
// 원본은 게임 폴더를 작업 폴더로 바꾸고 d\options.cfg를 연다. cpppj는 같은 위치를 절대 경로로 연다.
void Client::SaveOptions() {
    SaveClientOptions(configuration_, [this](std::string_view, std::span<const std::uint8_t> bytes) {
        // 원본 0041b4c0 → 0041a5b0의 모드 2(w+b): 상위 폴더를 새로 만들지 않고 기존 내용을 잘라 쓴다.
        const auto path=std::filesystem::absolute(options_.gameDirectory / "d" / "options.cfg");const auto pathUtf8=path.u8string();const std::string name(pathUtf8.begin(),pathUtf8.end());
        FILE* file = ::_wfopen(path.c_str(), L"w+b"); // 원본ANSI와 다른 Windows10/11의 유니코드 경로 호환 처리이며 파일 모드는 같다.
        if (!file) {
            // 다른 CRT 접근 전에 Win32 값을 보관한다. fopen 내부에서 갱신하지 않은 과거 값일 수도 있다.
            const DWORD openLastError = ::GetLastError();
            // fopen의 현재 errno를 먼저 복사해 뒤의 오류 조회나 진단 출력과 분리한다.
            const int openErrno = errno;
            // _get_doserrno 조회 실패 때 이 초기값을 유효한 OS 오류로 해석하지 않는다.
            unsigned long openDosError = 0;
            // 반환 0일 때만 openDosError가 유효하며 Win32 값과 별도로 기록한다.
            const errno_t openDosErrorStatus = ::_get_doserrno(&openDosError);
            std::fprintf(stderr,
                "SaveOptions fopen failed: path=\"%s\" errno=%d doserrno_read_status=%d doserrno_valid=%d doserrno=%lu last_error=%lu last_error_may_be_stale=1\n",
                name.c_str(), openErrno, openDosErrorStatus, openDosErrorStatus == 0 ? 1 : 0,
                openDosError, static_cast<unsigned long>(openLastError));
            throw std::runtime_error("Unable to open d/options.cfg");
        }
        const auto written = std::fwrite(bytes.data(), 1, bytes.size(), file);
        const int closed = std::fclose(file);
        if (written != bytes.size() || closed != 0) throw std::runtime_error("Unable to write d/options.cfg");
    });
}

// 원본 00436e40은 CRT remove를 부른다. UberGump 004cebf0의 버튼 사건에만 연결되는 함수다.
void Client::ClearFullScreenState() {
    ::_wremove((options_.gameDirectory / kFullScreenStateFile).c_str()); // 원본ANSI remove와 달리 Windows10/11의 한글 설치 경로에서도 같은 표식을 제거한다.
}

// 현재 DWORD에서 시작하므로 누락 ReadInt는 이전 값을 보존하며 signed 전체 비트도 바꾸지 않는다.
void Client::InterpretFortFileOptions() {
    auto value = std::bit_cast<std::int32_t>(readOnlyForts_); // 원본 ReadInt가 받는 동일 DWORD의 signed 작업값이다.
    configuration_.ReadInt("readOnlyForts", value);
    readOnlyForts_ = std::bit_cast<std::uint32_t>(value);
    configuration_.ReadInt("curServer",nsInfoGlobals_->serverSelection); // 같은54db4c의 현재값에서 읽어 누락 시 보존한다.
}

// 원본4353ea..43541a의 세 전역 쓰기다. 누락 ReadInt는 현재값을 보존한 뒤 허용값 검사를 거친다.
void Client::InterpretScriptMenuOptions() {
    constexpr std::uint32_t magic=0x845fed; // 원본이 유일하게 허용하는 userCheat DWORD다.
    gameplayGlobals_->player.special=false;gameplayGlobals_->specialAuxiliary=0;
    auto value=std::bit_cast<std::int32_t>(gameplayGlobals_->energy.userCheat);configuration_.ReadInt("userCheat",value);
    gameplayGlobals_->energy.userCheat=std::bit_cast<std::uint32_t>(value)==magic ? magic : 0U;
}
// 가짜 UI 복사본 없이 현재 장기 소유자를 매번 읽는다. 특수 모드는 기존 Player의 bool 표현을 사용한다.
std::uint32_t Client::ScriptMenuGlobal(std::uint32_t address) const {
    switch (address) { case 0x5cab30:return gameplayGlobals_->energy.userCheat;case 0x55a480:return gameplayGlobals_->player.special ? 1U : 0U;default:throw std::out_of_range("HTML 메뉴 현재 전역 주소 미연결"); }
}

// 원본의 현재 DWORD를 signed ReadInt로 빌려 누락 시 보존하고 음수/상위 비트도 그대로 기록한다.
void Client::InterpretMainMenuOptions() {
    auto value=std::bit_cast<std::int32_t>(localStateGlobals_->TellTips());configuration_.ReadInt("tellTips",value);localStateGlobals_->TellTips()=std::bit_cast<std::uint32_t>(value);
    value=std::bit_cast<std::int32_t>(localStateGlobals_->TipNumber());configuration_.ReadInt("tipNumber",value);localStateGlobals_->TipNumber()=std::bit_cast<std::uint32_t>(value); //원문435220처럼 tellTips 바로 뒤에 현재 순번을 읽고 누락/상위 비트를 보존한다.
}
void Client::InterpretOwnerColorOptions() { // 준비 전의 원문 무시와 현재 DWORD·같은 전체 dirty 증가를 보존한다.
    colorSortInput_->OwnerTables()->SetEnabled(std::bit_cast<std::uint32_t>(configuration_.GetInt("useBanners")),kencloudGlobals_->WorldRedraw());
}
// 원본 00435220에서 지금 쓰는 설정만 읽는다. 나머지 키(네트워크 등)의 전체 부모 순서는 후속이다.
void Client::InterpretOptions() {
    configuration_.ReadInt("tracelevel",trace_->log.level); // 원문00542658의 같은 장기 필터를 읽으며 덤프 부모의 임시30과 구별한다.
    rendererFrame_->ReadOptions(); //같은 원문 showSoftAsserts의 현재값을 실제 Renderer 진단 관문에 적용한다.
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
    static_cast<void>(ReadAudioOptions());InterpretMainMenuOptions(); //원문의 음질/음량/maxSimulSounds ReadInt 뒤 tellTips를 읽는다. 이 부분 부모는 음향 장치 적용이나 전체435220 완료를 대신하지 않는다.
    configuration_.ReadInt("inputLineOff",chatWindowGlobals_->inputLineOff); //원문0054db18은 표시 여부 캐시가 아니라 프레임말 전환 요청이다. 설정 재해석마다 현재값을 읽는다.
    InterpretFortFileOptions(); // 원본처럼 sleepPerLoop/maxFPS보다 먼저 같은 저장 관문 DWORD를 읽는다.
    configuration_.ReadInt("sleepPerLoop", sleepPerLoop_);
    kencloudGlobals_->ReadOptions(configuration_); // 원문435220의 cloudiness/speed/fury/spacer/recenter/algorithm/kenCloud 위치다.
    maxFps_ = kDefaultMaxFps;
    configuration_.ReadInt("maxFPS", maxFps_);
    frameInterval_ = FrameIntervalSeconds(maxFps_);
    InterpretScriptMenuOptions(); // 현재 복원된 설정 순회에서 프레임 옵션 뒤의 원문 치트 관문을 연결한다.
}

// 원본 00436260.
void Client::PaintLoading(NativeHandle dcHandle) {
    if (!screen_ || !(screen_->Flags()&ScreenMode::kFullScreen)) { PaintLoadingLogical(dcHandle);return; }
    // 원본과 다른 전체화면에서도 로딩 그림/문자 위치는 같은 논리 크기에 그린 후 같은 레터박스 안으로 확대한다.
    const HDC dc=static_cast<HDC>(dcHandle);BITMAPINFO bitmap{};bitmap.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bitmap.bmiHeader.biWidth=screenWidth_;bitmap.bmiHeader.biHeight=-screenHeight_;bitmap.bmiHeader.biPlanes=1;bitmap.bmiHeader.biBitCount=32;bitmap.bmiHeader.biCompression=BI_RGB;
    struct LoadingSurface { HDC dc{}; /* dc: 임시 논리 출력이다. */ HBITMAP bitmap{}; /* bitmap: 실제32비트 픽셀이다. */ HGDIOBJ previous{}; /* previous: 종료 전 되돌릴 선택이다. */
        ~LoadingSurface() { if (previous) SelectObject(dc,previous);if (bitmap) DeleteObject(bitmap);if (dc) DeleteDC(dc); } // 예외 때도 GDI 임시 자원을 반납한다.
    } surface;
    surface.dc=CreateCompatibleDC(dc);void* pixels{};surface.bitmap=CreateDIBSection(dc,&bitmap,DIB_RGB_COLORS,&pixels,nullptr,0);
    if (!surface.dc || !surface.bitmap) throw std::runtime_error("전체화면 로딩 DIB 생성 실패");
    surface.previous=SelectObject(surface.dc,surface.bitmap);PaintLoadingLogical(surface.dc);GdiFlush();
    RECT all{};GetClientRect(static_cast<HWND>(window_),&all);FillRect(dc,&all,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    const auto output=screen_->PresentationRect();const int saved=SaveDC(dc);SetStretchBltMode(dc,COLORONCOLOR);
    StretchDIBits(dc,output.left,output.top,output.right-output.left,output.bottom-output.top,0,0,screenWidth_,screenHeight_,pixels,&bitmap,DIB_RGB_COLORS,SRCCOPY);
    if (saved) RestoreDC(dc,saved);
}
// 원본00436260의 배치/문자 출력은 창/전체화면 양쪽에 동일하게 사용한다.
void Client::PaintLoadingLogical(NativeHandle dcHandle) {
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
void Client::InitializeNativeRendererView() { gameplayGlobals_->Camera().InitializeNativeRenderer(screenWidth_,screenHeight_,gameplayGlobals_->WorldCells(),kencloudGlobals_->WorldRedraw(),gameplayGlobals_->MapRevision()); }

// 현재 장치 화면과 논리 Native Renderer의 수명을 혼동하지 않는다.
void Client::Frame() {
    // [원본] 창이 비활성이고 전체화면이면 그리지 않는다(FUN_004977e0) — 전체화면을 옮기면 더한다.
    if (!clock_.IsPaused()) {
        // 바쁜 대기: 직전 그리기 뒤로 1 / maxFPS 초가 지날 때까지 벽시계를 다시 읽는다.
        while (clock_.WallSeconds(timeGetTime()) - lastDraw_ < frameInterval_) {}
    }
    lastDraw_ = clock_.WallSeconds(timeGetTime());
    PresentScene();
    if (menu_) menu_->ApplyCameraFrame(); //499fe0 말미의 목표 제한/497960 적용은 화면 출력 뒤·마지막 snapshot 복사 앞이다.
    gameplayGlobals_->Camera().CopyRendererSnapshot(); //00436532: 프레임 말미 현재 논리 Renderer의16DWORD를 복사한다.
    ++frames_;
    if (options_.frameLimit != 0 && frames_ == options_.frameLimit) {
        if (!options_.screenshot.empty()) SaveScreenshot();
        // 새 검사 기능: 소리·음악 상태를 줄 단위 글로 남긴다(소리 장치가 실제로 재생 중인지 청취 없이 확인한다).
        if (!options_.audioReport.empty() && audio_) {
            std::ofstream report(options_.audioReport, std::ios::binary);
            report << audio_->Describe();
        }
        PostMessageA(static_cast<HWND>(window_), WM_CLOSE, 0, 0);
    }
}

// 현재 화면 버퍼를 팔레트로 색을 입혀 저장한다.
// 같은 실제 Draw/Lock/Present 경로를 장면 표시와 일반 프레임에서 공유한다. 프레임 대기는 호출자가 소유한다.
void Client::PresentScene() {
    if (sceneVisible_) {
        if ((screen_->Flags() & ScreenMode::kSoftwareMouse) != 0) {
            const auto position = Poll(0); const auto hotspot = cursor_->Hotspot();
            renderer_->SetSoftwareCursor(active_ && screen_->CursorVisible() && (position.code & InputCode::kOutside) == 0 ? cursor_->SoftwareImage() : nullptr,
                {position.x - hotspot.x, position.y - hotspot.y});
        } else renderer_->SetSoftwareCursor(nullptr,{}); // 호환 모드 복귀 때 이전 소프트웨어 커서 캐시를 지운다.
        const bool nativeFrame=rendererFrame_->Prepare();
        if (!nativeFrame && kencloudGlobals_->WorldRedraw()) renderer_->InvalidateAll(); //독립 표시/10.72는 기존 전체 갱신 경계를 유지한다.
        const auto drawing=nativeFrame ? std::optional<RendererWorldDrawing>{rendererFrame_->Drawing()} : std::nullopt;
        screen_->SetClip(0, 0, screenWidth_, screenHeight_);
        // Windows10/11 호환 DIB는 원본 DirectDraw 잠금 종류 대신 같은8비트 표면의 호스트 잠금을 쓴다. 구름 내부의 중첩 잠금/현재 clip은 원문 표시 부모가 유지한다.
        std::uint8_t* buffer = screen_->Lock();
        try {
            renderer_->Draw(std::span<std::uint8_t>(buffer, static_cast<std::size_t>(screen_->Pitch()) * static_cast<std::size_t>(screenHeight_)), screen_->Pitch(),0,drawing ? &*drawing : nullptr);
        } catch (...) { screen_->Unlock(); throw; }
        screen_->Unlock();
        if (nativeFrame) rendererFrame_->Finish(); //정상 Draw/잠금 반납 뒤·Present 앞의 같은 원문470ca0이다.
        if (screen_->Flags()&ScreenMode::kFullScreen) {
            bool changed=false;renderer_->Present([&changed](ScreenRect) { changed=true; }); // 원본 dirty 소비/캐시는 그대로 진행한다.
            if (changed) screen_->Update({0,0,screenWidth_,screenHeight_}); // Windows10/11의 비정수 확대는 clip별 GDI 샘플 위상 차이를 피하도록 프레임당 전체 DIB를 한 번 출력한다.
        } else renderer_->Present([this](ScreenRect rect) { screen_->Update(rect); });
        kencloudGlobals_->WorldRedraw()=0; //499fe0은 출력/dirty clear 뒤 같은 전역을0으로 쓰며 뒤 카메라 zoom이 다시1을 쓸 수 있다.
        if (afterPresentation) afterPresentation(*this); // 검사만 실제 표시한 버퍼를 읽는다. 원문의 Present→카메라 적용/다음 버퍼 준비 순서는 바꾸지 않는다.
    }
}
void Client::SaveScreenshot() {
    Capture(options_.screenshot);
}
void Client::SaveGifScreenshot() { // 원문이 저장하는 고정 primary 색과 현재8비트 화면을 같은 잠금 안에서 읽는다.
    const std::uint8_t* pixels{};ScreenshotSaveHooks hooks; // 잠금에서 빌린 현재 DIB 포인터와 파일 장치다.
    hooks.set=[this](auto key,auto value) { configuration_.Set(key,value); };
    hooks.read=[this](auto key,auto& value) { return configuration_.ReadString(key,value); };
    hooks.exists=[this](auto path) { return files_.TryRead(path).has_value(); }; //41ab10처럼 기본 디스크/아카이브/보조 디스크의 같은 존재 순서를 사용한다.
    hooks.lock=[this,&pixels] { pixels=screen_->Lock(); };
    hooks.gif=[this,&pixels](auto path) {
        const auto rgb=ScreenshotPrimaryPalette(colorSortInput_->Primary()->Bytes()); // 현재flash 팔레트가 아닌555d88의같은 기록이다.
        const auto bytes=EncodeScreenshotGif(static_cast<std::uint32_t>(screenWidth_),static_cast<std::uint32_t>(screenHeight_),static_cast<std::uint32_t>(screen_->Pitch()),{pixels,static_cast<std::size_t>(screen_->Pitch())*screenHeight_},rgb); // 패딩을 보존한 현재 논리 버퍼다.
        std::ofstream output(ResolveScreenshotDiskPath(options_.gameDirectory,path),std::ios::binary|std::ios::trunc); //41b4c0의 기본 디스크 경로다. 원문 fopen처럼 없는 상위 폴더는 생성하지 않는다.
        if (!output || !output.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size())) || !output.flush()) throw std::runtime_error("스크린샷 GIF 파일 저장 실패");
    };
    hooks.unlock=[this] { screen_->Unlock(); };
    hooks.log=[this](auto path) { WriteDiagnosticLog(29,"Screen shot written to "+std::string(path)+"\n"); }; // 원문29번 로그도 선택/덤프와 같은 FILE/필터/들여쓰기를 쓴다.
    static_cast<void>(SaveNumberedScreenshot(screenshotNumber_,hooks));
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

// 검사 전용 OS 클라이언트 캡처다. 논리 DIB를 재확대하지 않고 실제 출력된 픽셀/레터박스를 읽는다.
void Client::CapturePresentation(const std::filesystem::path& path) {
    RECT client{};GetClientRect(static_cast<HWND>(window_),&client);const int width=client.right,height=client.bottom;if (width<=0 || height<=0) throw std::runtime_error("실제 화면 캡처의 양수 클라이언트 필요");
    struct CaptureSurface { HDC dc{}; /* dc: 실제 창 출력의 복사 대상이다. */ HBITMAP bitmap{}; /* bitmap: 읽을32비트 DIB다. */ HGDIOBJ previous{}; /* previous: 반납 전의 GDI 선택이다. */
        ~CaptureSurface() { if (previous) SelectObject(dc,previous);if (bitmap) DeleteObject(bitmap);if (dc) DeleteDC(dc); } // 캡처 성공/실패 모두 임시 GDI를 반납한다.
    } surface;
    BITMAPINFO bitmap{};bitmap.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bitmap.bmiHeader.biWidth=width;bitmap.bmiHeader.biHeight=-height;bitmap.bmiHeader.biPlanes=1;bitmap.bmiHeader.biBitCount=32;bitmap.bmiHeader.biCompression=BI_RGB;
    void* pixels{};surface.dc=CreateCompatibleDC(static_cast<HDC>(windowDc_));surface.bitmap=CreateDIBSection(static_cast<HDC>(windowDc_),&bitmap,DIB_RGB_COLORS,&pixels,nullptr,0);
    if (!surface.dc || !surface.bitmap) throw std::runtime_error("실제 화면 캡처 DIB 생성 실패");surface.previous=SelectObject(surface.dc,surface.bitmap);
    GdiFlush();if (!BitBlt(surface.dc,0,0,width,height,static_cast<HDC>(windowDc_),0,0,SRCCOPY)) throw std::runtime_error("실제 화면 캡처 복사 실패");GdiFlush();
    const auto source=static_cast<const std::uint8_t*>(pixels);std::vector<std::uint8_t> rgba(static_cast<std::size_t>(width)*height*4);
    for (std::size_t i=0;i<rgba.size();i+=4) { rgba[i]=source[i+2];rgba[i+1]=source[i+1];rgba[i+2]=source[i];rgba[i+3]=255; } // GDI BGRA만 저장 도구의 RGBA로 바꾼다.
    platform::WriteBitmap(path,static_cast<std::uint32_t>(width),static_cast<std::uint32_t>(height),rgba);
}
// UI는 정지 상태를 한 번만 적용한다. wall 시각은 계속 흐른다.
void Client::Pause(bool paused) {
    if (paused==uiPaused_) return;
    if (paused) PushGamePause();else PopGamePause();
    uiPaused_=paused;
}
// 원본 미션 정지는 메뉴 정지 여부와 무관하게 실제 같은 시계 깊이를 한 번 증가시킨다.
void Client::PushGamePause() { clock_.Pause(timeGetTime()); }
// 원본 미션 재개는 실제 같은 시계의 마지막 깊이가 해제될 때만 시간을 진행한다.
void Client::PopGamePause() { clock_.Resume(timeGetTime());if (!clock_.IsPaused()) uiPaused_=false; } //실제 마지막 정지가 풀리면 호스트 UI의 보유 표식도 반납한다. Native State의 전체 Resume 뒤 다음 모달이 다시 정지할 수 있어야 한다.
// 자동 검사는 표시 단계와 실제 시계 정지를 함께 검사한다.
bool Client::Paused() const { return clock_.IsPaused(); }
// 실제 Client 시계의 기존 Reset만 호출하며 시각 조회/프레임 재캡처/중첩 정지 반납을 추가하지 않는다.
void Client::ResetGameClock() { clock_.Reset(); }
// Native State 부모의 정확한 위치에서 기존 설정 해석 본문을 재사용한다.
void Client::InterpretStateOptions() { InterpretOptions(); }
// 원본 창 제목을 ANSI 바이트로 적용한다(한국어는 후순위).
void Client::Title(std::string_view title) { SetWindowTextA(static_cast<HWND>(window_), std::string(title).c_str()); }
// 기본 UI만 제공한다. 검사 장면의 독립 실행은 기존 연결점을 보존한다.
UberGump* Client::Menu() { return menu_.get(); }
// 설치 중에는 menu/world가 아직 없을 수 있다. 별도 subscriber 사본이나 로컬 번호 상수를 쓰지 않는다.
const std::uint32_t& Client::CurrentSubscriber() const { return subscriberGlobals_->MySubscriber(); }
// 실제 PATCH 몸체를 반환하며 CD 호출은 미복원 경계로 명시 거부한다.
RawChatChannels& Client::ChatChannels() {
    if (!chat_->channels) throw std::logic_error("CD Chat 채널 몸체는 아직 복원되지 않음");
    return *chat_->channels;
}
// 처리기가 쓰는 물리 저장소 그대로를 읽기 전용으로 공개한다.
std::span<const std::uint8_t> Client::ChatRecords() const { return chat_->records; }
// 팔레트 적용 → 원소 색 표 갱신 순서다. SceneMusic은 이 함수 호출 전에 tint를 썼으므로 여기서 현재 tint를 덮어쓰지 않는다.
void Client::LoadScenePalette(std::string_view name) {
    const auto path=configuration_.PathSpec("DataDir")+"\\"+std::string(name);
    LoadPaletteFile(path,0);
    RefreshPaletteConsumers();
}
// 완성된실제PathSpec경로를읽어원문clear DWORD를전달한다.미리DataDir를붙이지않는다.
void Client::LoadPaletteFile(std::string_view path,std::uint32_t clear) {
    screen_->LoadPalette(GamePalette(files_.Read(path)),options_.edition,clear);
}
// 기존 전체 부모에 실제 파일/구름/Screen과 같은 고정 초기RGB의9표를 연결한다.
void Client::ChangeFortPalette(const std::span<const std::uint8_t>& name) {
    o::FortPaletteTransitionHooks devices;devices.log=[](auto level,auto text) { std::clog<<"Palette "<<level<<' '<<text; };
    devices.loadPalette=[this](auto path,auto clear) { const auto end=std::find(path.begin(),path.end(),std::uint8_t{});if (end==path.end()) throw std::out_of_range("전체 팔레트 현재 경로 NUL 누락");LoadPaletteFile({reinterpret_cast<const char*>(path.data()),static_cast<std::size_t>(end-path.begin())},clear); };
    devices.buildMaps=[this] { RebuildColorSortMaps();RefreshPaletteConsumers(); }; // 현재 Screen 명명색과 최초 고정RGB 표를 각 실제 소비자에 제공한다.
    kencloudGlobals_->Transition(name,files_,configuration_,randomGlobals_->Game(),std::move(devices));
}
// 기존소비자갱신은한군데에서재사용한다.구름캐시생성이나추가표부모자체의성공을대신하지않는다.
void Client::RebuildColorSortMaps() { static_cast<void>(colorSortInput_->Rebuild(screen_->Colors().basic[1])); }
//현재명명색과같은초기ColorSort입력은별개로소비자에전달한다.
void Client::RefreshPaletteConsumers() {
    if (audio_) audio_->SetSceneTints(screen_->Colors().weather);
    if (cursor_ && (screen_->Flags()&ScreenMode::kSoftwareMouse)!=0) cursor_->BuildSoftware(screen_->Palette());
    if (menu_) menu_->PaletteChanged();else if (renderer_) renderer_->InvalidateAll();
}
// 00470d40처럼 현재 Player의 기술 부모가 있을 때만 실제 TYPE55/flags0x10으로 부착한다.
void Client::RequestThunderFlash(std::int32_t brightness) { StartThunderFlash(brightness); } //원문 명령과 날씨가 같은 부모/효과 경계를 사용한다.
void Client::StartThunderFlash(std::int32_t brightness) {
    if (!gameplayGlobals_->player.playerKnown) return; // 원문595344가0이면 부모 조회조차 하지 않는다.
    if (!surfaceStorage_) throw std::logic_error("번개 기술 부모의 실제 표면 저장소 없음");
    if (surfaceStorage_->ownerMode.challenge) throw std::logic_error("번개 메타맵00479650의 실제 기록 조회 미복원"); // 멀티플레이 메타맵 기록을 일반 Player 행으로 대체하지 않는다.
    const auto players=coreDataGlobals_->Players();const auto at=std::size_t(gameplayGlobals_->player.battleOwner)*180+0x9c; //0048fc60과 같은 현재 Player base/owner다.
    if (at>players.size() || players.size()-at<4) throw std::out_of_range("번개 현재 Player 기술 DWORD 범위");
    std::uint32_t parent=0;
    for (std::size_t byte=0;byte<4;++byte) parent|=std::uint32_t(players[at+byte])<<(8*byte); // 실제 비정렬DWORD를 상위 비트까지 보존한다.
    if (!parent) return;
    if (parent>65535U || parent>=surfaceStorage_->pool.Capacity()) throw std::out_of_range("번개 기술 부모의 실제 SID 범위");
    auto process=std::make_unique<ThunderProcess>((screen_->Flags()&ScreenMode::kFullScreen)!=0,PaletteFlashHooks{
        [this] { return Time().wall; },[this] { return (screen_->Flags()&ScreenMode::kFullScreen)!=0; },
        [this](unsigned start,unsigned count,ScreenColor* destination) { std::copy_n(screen_->Palette().data()+start,count,destination); },
        [this](unsigned start,unsigned count,ScreenColor* colors) { screen_->SetPalette(start,count,colors,true); },
        [this] { screen_->SetPalette(0,256,nullptr,true); }
    },[this](o::ProcessId id) { std::erase(thunderProcesses_,id); },brightness);
    auto& host=surfaceStorage_->Processes().Host();auto* attached=host.Attach(std::move(process),kThunderProcessType,o::Sid{static_cast<std::uint16_t>(parent)},0x10);
    if (attached) thunderProcesses_.push_back(kernel_->Find(*attached)); // free/dead 부모의 원문 조용한 거부는 등록 목록에도 넣지 않는다.
}
// 종료 중 콜백이 번호를 지우므로 뒤 번호를 하나씩 읽어 제거한다. Screen이 사라지기 전에 호출한다.
void Client::ClearThunderFlashes() {
    // 등록된 프로세스를 즉시 파괴하여 전체화면 복구와 목록 정리가 끝나도록 한다.
    while (!thunderProcesses_.empty()) { // 매 삭제 콜백이 바꾼 현재 뒤 번호를 다시 읽는다.
        auto* process=dynamic_cast<ThunderProcess*>(kernel_->Get(thunderProcesses_.back()));
        if (!process) throw std::logic_error("번개 등록 목록과 실제 Kernel 불일치");
        surfaceStorage_->Processes().Host().Kill(*process,0x10); // Windows10/11 해상도/장치 교체 때도 원본과 달리 안전하게 먼저 효과와 Form을 함께 반납한다.
    }
}
// 개별 효과의 종료/취소 콜백으로 유지되는 현재 등록 수다.
std::size_t Client::ThunderFlashCount() const { return thunderProcesses_.size(); }
// 원본 세 논리 해상도를 바꾸되 Windows10/11 호환 전체화면 선택은 같은 모드로 유지한다.
void Client::ChangeResolution(int width, int height) {
    if (!((width == 640 && height == 480) || (width == 800 && height == 600) || (width == 1024 && height == 768)))
        throw std::invalid_argument("Unsupported original resolution");
    if (screenWidth_ == width && screenHeight_ == height) return;
    ClearThunderFlashes();
    const auto mode=screen_->Flags(); // 재생성 전 실제 모드를 보존한다.
    // 현재 RGBQUAD 256개를 새 장치의 저장 팔레트로도 옮긴다. SetPalette만 하면 SetMode의 기본 팔레트 재적용에서 검정으로 덮인다.
    std::array<std::uint8_t,1024> palette{};
    std::memcpy(palette.data(),screen_->Palette().data(),palette.size());
    sceneVisible_ = false;
    kencloudGlobals_->BindRenderer(nullptr);renderer_.reset(); screen_.reset(); // 장치 재생성은 구름 할당/이름/옵션을 지우지 않는다.
    screenWidth_ = width; screenHeight_ = height;
    screen_ = std::make_unique<Screen>(window_, windowDc_, width, height,&paletteLoaded_);
    screen_->Init(); screen_->InitDibSection();
    screen_->BindColorSortInput([owner=colorSortInput_] { return owner->Logical(); }); //해상도재생성은초기입력을다시캡처하지않는다.
    screen_->LoadPalette(GamePalette(palette),options_.edition);
    RECT rect{0, 0, width, height};
    AdjustWindowRectEx(&rect, static_cast<DWORD>(GetWindowLongPtrA(static_cast<HWND>(window_), GWL_STYLE)), FALSE, 0);
    windowWidth_ = rect.right - rect.left; windowHeight_ = rect.bottom - rect.top;
    if (!screen_->SetMode(mode, windowWidth_, windowHeight_, initWindowPos_))
        throw std::runtime_error("Unable to change window resolution");
    PublishScreenMode();if ((mode&ScreenMode::kSoftwareMouse) && cursor_) cursor_->BuildSoftware(screen_->Palette());
    renderer_ = std::make_unique<Renderer>(width, height,options_.edition);
    kencloudGlobals_->BindRenderer(renderer_.get()); // 새 장치를 같은 현재 renderer 전역에 공개한다.
    // 메뉴/독립 검사 화면도 새 화면 크기로 위치 효과음을 계산한다. 미션은 다음 Compose/Resize에서 실제 카메라를 공급한다.
    const auto previous=WorldAudio().View(); // noAudio도 같은 실제 카메라/화면 영역을 유지한다.
    WorldAudio().SetView({previous.cameraX,previous.cameraY,previous.left,previous.top,width,height});
    sceneVisible_ = true;
    configuration_.SetInt("SCREENW", width); configuration_.SetInt("SCREENH", height);
}

// 원본 모드 결과의 전역 이름은 유지하되 Windows10/11 호환 장치가 실제 지원하는 플래그를 공개한다.
void Client::PublishScreenMode() {
    const auto mode=screen_->Flags();
    configuration_.SetInt("global.ddFullScreen",(mode&ScreenMode::kFullScreen)!=0);
    configuration_.SetInt("global.ddBackBufferDib",(mode&ScreenMode::kBackBufferDib)!=0);
    configuration_.SetInt("global.ddFlipping",(mode&ScreenMode::kFlipping)!=0);
    configuration_.SetInt("global.ddClouds",(mode&ScreenMode::kClouds)!=0);
    configuration_.SetInt("global.ddAnimating",(mode&ScreenMode::kAnimating)!=0);
    configuration_.SetInt("global.ddParallax",(mode&ScreenMode::kParallax)!=0);
    configuration_.SetInt("global.ddSoftwareMouse",(mode&ScreenMode::kSoftwareMouse)!=0);
    configuration_.SetInt("global.ddFourPage",0);
}
// 원본 ScreenModeAbsChange 명령의 모드 값을 같은 장치에 적용하고 사용자 선택만 저장한다.
void Client::ChangeScreenMode(std::uint32_t flags,bool remember) {
    if (!screen_) throw std::logic_error("화면 모드 변경의 현재 장치 없음");
    if (!screen_->SetMode(flags,windowWidth_,windowHeight_,initWindowPos_)) throw std::runtime_error("지원하지 않는 화면 모드 조합");
    PublishScreenMode();const auto mode=screen_->Flags();
    if (remember) configuration_.SetInt("startInFullScreen",(mode&ScreenMode::kFullScreen)!=0); // 마지막 유효 workingFullScreenFlags는 원본처럼 별도 확인 명령에서만 바꾼다.
    ClearFullScreenState(); // 원본 DirectDraw 실패 표식을 새 호환 모드에 남기지 않는다.
    if (cursor_) { if (mode&ScreenMode::kSoftwareMouse) cursor_->BuildSoftware(screen_->Palette());else cursor_->Apply(); }
    if (renderer_) { renderer_->SetSoftwareCursor(nullptr,{});renderer_->InvalidateAll(); }
    InvalidateRect(static_cast<HWND>(window_),nullptr,FALSE);
}
// 원본 AcceptScreenModeChange의 workingFullScreenFlags 저장은 실제 미리보기를 사용자가 확인한 뒤에만 한다.
void Client::AcceptScreenMode() { if (screen_ && (screen_->Flags()&ScreenMode::kFullScreen)) configuration_.SetInt("workingFullScreenFlags",static_cast<std::int32_t>(screen_->Flags())); }
// 실제 UI 자동 검증이 논리 크기/모드와 현재 OS 창/출력/커서 제한을 구별해 기록한다.
std::string Client::ScreenDiagnostics() const {
    std::ostringstream out;if (!screen_) return {};RECT window{},client{},clip{};GetWindowRect(static_cast<HWND>(window_),&window);GetClientRect(static_cast<HWND>(window_),&client);GetClipCursor(&clip);const auto output=screen_->PresentationRect();
    out<<"screen_mode\t"<<screen_->Flags()<<'\t'<<screenWidth_<<'\t'<<screenHeight_<<'\t'<<screen_->Pitch()<<'\t'<<active_<<'\n';
    out<<"screen_window\t"<<window.left<<'\t'<<window.top<<'\t'<<window.right<<'\t'<<window.bottom<<'\t'<<client.right<<'\t'<<client.bottom<<'\t'<<GetWindowLongPtrA(static_cast<HWND>(window_),GWL_STYLE)<<'\n';
    out<<"screen_output\t"<<output.left<<'\t'<<output.top<<'\t'<<output.right<<'\t'<<output.bottom<<'\n';
    out<<"screen_cursor_clip\t"<<clip.left<<'\t'<<clip.top<<'\t'<<clip.right<<'\t'<<clip.bottom<<'\n';return out.str();
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
    GetCursorPos(&cursor);
    ScreenToClient(window,&cursor);const auto point=screen_ ? screen_->ToLogicalPoint({cursor.x,cursor.y}) : ScreenPoint{cursor.x,cursor.y}; // 원본1:1 입력과 호환 확대 입력의 같은 논리 좌표다.
    const std::int32_t x=point.x,y=point.y;
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
    GetCursorPos(&cursor);
    ScreenToClient(static_cast<HWND>(windowHandle),&cursor);const auto point=screen_ ? screen_->ToLogicalPoint({cursor.x,cursor.y}) : ScreenPoint{cursor.x,cursor.y};
    input_.Push(ModifierBits() | static_cast<std::uint32_t>(wParam),point.x,point.y);
}

// 원본 00452e00.
InputEvent Client::Poll(std::uint32_t code) const {
    // UI 스크립트는 사건 좌표와 다음 폴링 좌표를 함께 제공한다. 일반 실행은 비어 있어 원본 경로를 따른다.
    if (inspectPoll) if (const auto event=inspectPoll(code)) return *event;
    const std::uint32_t kept = code & 0x43ffffffu;
    const SHORT state = GetAsyncKeyState(static_cast<int>(kept) >> 16);
    // 지금 눌려 있지 않으면 뗌 표시, 지난 확인 뒤에 눌렸으면 0x40000000.
    std::uint32_t result = (~(static_cast<std::uint32_t>(static_cast<std::int32_t>(state)) << 16) & InputCode::kRelease)
        | ((static_cast<std::uint32_t>(state) & 1u) << 30) | kept;
    POINT cursor{};
    GetCursorPos(&cursor);
    ScreenToClient(static_cast<HWND>(window_),&cursor);const auto output=screen_ ? screen_->PresentationRect() : ScreenRect{0,0,screenWidth_,screenHeight_};
    if (cursor.x<output.left || output.right<cursor.x || cursor.y<output.top || output.bottom<cursor.y)
        result |= InputCode::kOutside;
    if (!active_) result |= InputCode::kOutside;
    // 좌표는 화면 안으로 넣는다(0 ~ 폭, 0 ~ 높이).
    const auto point=screen_ ? screen_->ToLogicalPoint({cursor.x,cursor.y}) : ScreenPoint{cursor.x,cursor.y};
    const std::int32_t x=std::clamp<std::int32_t>(point.x,0,screenWidth_),y=std::clamp<std::int32_t>(point.y,0,screenHeight_);
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
        if (screen_) screen_->SetActive(active_);
        if (renderer_) renderer_->InvalidateAll();
        break;
    case WM_DISPLAYCHANGE:
    case WM_DPICHANGED:
        // 원본 독점 모드 복구와 달리 현대 Windows 모니터/DPI 변경은 같은 DIB의 출력/입력 범위만 갱신한다.
        if (screen_) screen_->RelayoutFullscreen();
        if (renderer_) renderer_->InvalidateAll();
        break;
    case WM_ACTIVATE:
        // 원본과 다른 borderless는 강제 최소화 없이 다른 앱에 커서를 반납한다.
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
        if (screen_) screen_->SetActive(active_);
        if (active_ && renderer_) renderer_->InvalidateAll();
        if (!active_) {
            Sleep(0);
            return DefWindowProcA(window, message, 0, lParam);
        }
        // Windows10/11 DIB 호환 전체화면은 잃어버린 독점 DirectDraw 표면이 없어 같은 버퍼를 다시 출력한다.
        break;
    case WM_SETCURSOR:
        if (cursor_ && LOWORD(lParam) == HTCLIENT) {
            if (screen_ && (screen_->Flags() & ScreenMode::kSoftwareMouse) != 0) SetCursor(nullptr);
            else cursor_->Apply();
            return TRUE;
        }
        break;
    case WM_GETMINMAXINFO:
        if (windowWidth_ != -1 && (!screen_ || !(screen_->Flags()&ScreenMode::kFullScreen))) {
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
            // 원본 Alt-Enter의 모드 전환을 Windows10/11 호환 장치로 전달한다. 반복 keydown은 무시한다.
            if (wParam==VK_RETURN) { if (message==WM_SYSKEYDOWN && !(lParam&0x40000000) && screen_) ChangeScreenMode((screen_->Flags()&ScreenMode::kFullScreen) ? static_cast<std::uint32_t>(configuration_.GetInt("windowScreenFlags")) : static_cast<std::uint32_t>(configuration_.GetInt("workingFullScreenFlags")));return 0; }
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
        const ScreenPoint actual{static_cast<std::int16_t>(LOWORD(lParam)),static_cast<std::int16_t>(HIWORD(lParam))};const auto point=screen_ ? screen_->ToLogicalPoint(actual) : actual; // 캡처된 음수 좌표/확대 좌표를 signed로 유지한다.
        input_.Push(button | ModifierBits() | (release ? InputCode::kRelease : 0u),point.x,point.y);
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
// 같은 Client의 실제 한 DWORD 주소를 저장 소비자에 제공한다.
const std::uint32_t& Client::ReadOnlyForts() const { return readOnlyForts_; }
// 같은 Client가 소유한 실제 장부만 전달하며 새 Session용 복제본을 만들지 않는다.
const std::shared_ptr<o::RawFortBadgesGlobals>& Client::FortBadges() const { return fortBadges_; }
// 현재 변환표와 원래 할당을 같은 Client 수명으로 전달한다.
const std::shared_ptr<FortTypeConversionStorage>& Client::FortTypeConversion() const { return fortTypeConversion_; }
// 새 Session의 fallback을 만드는 대신 같은 Client의 실제 난수 소유자를 빌린다.
const std::shared_ptr<ClientColorSortInput>& Client::ColorSortInput() const { return colorSortInput_; } //같은Client소유슬롯이다.
// 같은 Client 두 DWORD owner를 수명째 빌리며 새 객체/캐시를 만들지 않는다.
const std::shared_ptr<ClientSubscriberGlobals>& Client::SubscriberGlobals() const { return subscriberGlobals_; }
const std::shared_ptr<ClientRandomGlobals>& Client::RandomGlobals() const { return randomGlobals_; }
const std::shared_ptr<ClientSpGlobals>& Client::SpGlobals() const { return spGlobals_; } // 같은 실제 저장소를 빌리며 초기화/할당/재시드는 하지 않는다.
const std::shared_ptr<ClientKencloudGlobals>& Client::KencloudGlobals() const { return kencloudGlobals_; } // 조회 자체에 생성/난수 소비/초기화 효과는 없다.
// 큰 메뉴/로컬 로그인 분배기가 같은 실제144BYTE 다섯 블록과 기한을 빌린다.
const std::shared_ptr<ClientLocalStateGlobals>& Client::LocalStateGlobals() const { return localStateGlobals_; }
const std::shared_ptr<ClientCdGlobals>& Client::CdGlobals() const { return cdGlobals_; } // 같은 현재 CD 캐시를 조회하며 확인 장치를 여기서 실행하지 않는다.
// 같은 활동 표/Zone owner를 조회하며 Session 교체나 조회 자체로 초기화하지 않는다.
const std::shared_ptr<ClientConnectionActivityGlobals>& Client::ConnectionActivityGlobals() const { return connectionActivityGlobals_; }
// 같은 현재 표의 소유권만 조회하며 연결 종료·등록·개수 쓰기를 실행하지 않는다.
const std::shared_ptr<ClientConnectionGlobals>& Client::ConnectionGlobals() const { return connectionGlobals_; }
const std::shared_ptr<ClientFortGlobals>& Client::FortGlobals() const { return fortGlobals_; } // 같은 실제 이름/목록의 주소를 유지한다.
//같은 Client의 현재 Core/캐시 소유자를 조회하며 외부 Player 수명을 변경하지 않는다.
const std::shared_ptr<ClientCoreDataGlobals>& Client::CoreDataGlobals() const { return coreDataGlobals_; }
// 실제 Player/factory의 단일 할당 소유자를 세션에 빌린다. 조회는 초기화를 실행하지 않는다.
const std::shared_ptr<ClientPlayerRecords>& Client::PlayerRecords() const { return playerRecords_; }
// 현재 전역의 주소를 빌리며 조회 자체는 게임/초기화를 수행하지 않는다.
const std::shared_ptr<ClientGameplayGlobals>& Client::GameplayGlobals() const { return gameplayGlobals_; }
// 실제 현재 NSInfo/서버 이름 소유자를 조회하며 목록을 다시 읽거나 초기화하지 않는다.
const std::shared_ptr<ClientHtmlWindowGlobals>& Client::HtmlWindowGlobals() const { return htmlWindowGlobals_; } // 같은 실제 장기 HTML 상태를 조회한다.
const std::shared_ptr<ClientChatWindowGlobals>& Client::ChatWindowGlobals() const { return chatWindowGlobals_; } // 같은 장기 소유자를 조회한다.
const std::shared_ptr<ClientNsInfoGlobals>& Client::NsInfoGlobals() const { return nsInfoGlobals_; }
//현재 manager와 실제 할당의 한 소유자를 반환하며 공통 초기화 시점을 앞당기지 않는다.
const std::shared_ptr<ClientChatManagerGlobals>& Client::ChatManagerGlobals() const { return chatManagerGlobals_; }
const std::shared_ptr<ClientStaticStrings>& Client::StaticStrings() const { return staticStrings_; } // 같은 실제 고리의 소유자를 조회한다.
//초기 say와 이후 등록을 보존한 같은 표를 반환하며 Session 해제 때 다시 초기화하지 않는다.
const std::shared_ptr<ClientDotCommandGlobals>& Client::DotCommandGlobals() const { return dotCommandGlobals_; }
// 파일 시스템.
const o::BaseFileSystem& Client::Files() const { return files_; }
// 원문은 CD를 찾았을 때만 별도 목록을 등록한다. Windows10/11 한글 CD 경로는 ANSI 대신 UTF-8/W 경로로 보존한다.
void Client::InitializeNativeSecondaryFiles(std::string_view directory) {
    files_.SetSecondaryDirectory(directory.empty()?std::filesystem::path{}:std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(directory.data()),directory.size())));
    if (!directory.empty()) files_.RegisterArchives(true);
}
// 타입·그림. Run이 "init types"에서 만든 뒤에만 쓴다.
const GameAssets& Client::Assets() const { return *assets_; }
// 프로세스 커널.
o::Kernel& Client::GetKernel() { return *kernel_; }
const std::shared_ptr<o::Kernel>& Client::ProcessKernelOwner() const { return kernel_; } // 실제 전역 Kernel 객체를 공유한다.
const std::shared_ptr<o::FrameTime>& Client::FrameClockOwner() const { return time_; } // 같은 현재 프레임의 수명을 공유한다.
// 자산 초기화 이전에 가짜 빈 월드로 성공하지 않는다.
const std::shared_ptr<RawSurfaceStorage>& Client::SurfaceStorage() const { if (!surfaceStorage_) throw std::logic_error("Client 실제 월드 물리 초기화 전 조회");return surfaceStorage_; }
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
// 별도 성공 상태를 만들지 않고 실제 Frame/구름 owner의 현재 관측만 반환한다.
std::string Client::RendererFrameDiagnostics() const { return rendererFrame_ ? rendererFrame_->Diagnostics() : std::string{}; }
// 준비된 장면을 로딩 화면 대신 표시하고 남아 있는 창 영역도 검증한다.
void Client::ShowScene() { sceneVisible_ = true; renderer_->InvalidateAll(); InvalidateRect(static_cast<HWND>(window_), nullptr, FALSE); }
// 이번 프레임의 시각.
const o::FrameTime& Client::Time() const { return *time_; }
// 정지와 무관한 실제 원본 벽 시계를 조회한다. 부착/idle의 여러 읽기도 각각 OS 눈금을 사용한다.
double Client::WallSeconds() const { return clock_.WallSeconds(timeGetTime()); }
// 창이 활성인가.
bool Client::Active() const { return active_; }
// 번역표가 없거나 영어면 원문 그대로다.
std::string Client::Translate(std::string_view text) const {
    return translations_ ? translations_->Translate(text, languageNumber_) : std::string(text);
}

}  // namespace netstorm::client
