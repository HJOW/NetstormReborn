// 원본 소스: ClientMain.cpp (클라이언트 폴더) — 프로그램 진입점(WinMain), 창 프로시저, 메인 루프.
//
// 원본 WinMain(FUN_00438dc0)의 순서를 따라 창·설정·화면 장치를 만들고 루프를 돈다.
// 아직 옮기지 않은 초기화 단계와 루프 단계는 ClientMain.cpp에 원본 순서대로 적어 두었다.
// 분석 문서: docs/exe/main-loop.md, docs/exe/cpp-screen-reconstruction.md
#pragma once
#include "client/GameAssets.h"
#include "client/InputEvent.h"
#include "client/Screen.h"
#include "client/Renderer.h"
#include "client/Cursor.h"
#include "o/BaseFile.h"
#include "o/ConfigInterface.h"
#include "o/GameClock.h"
#include "o/Kernel.h"
#include "o/RiftType.h"
#include "o/Xlat.h"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace netstorm::client {
class UberGump;
class ClientAudio;
struct ClientAudioOptions;

// 설정 키 "maxFPS" 의 코드 기본값. 화면 루프를 초당 몇 번까지 돌릴지 정한다.
// 원본: FUN_00435220 @ 00435220 의 `DAT_005318d8 = 0x4b`
inline constexpr int kDefaultMaxFps = 75;
// 화면 크기의 코드 기본값(원본 DAT_00531860·DAT_00531864의 초기값). 설정 SCREENW·SCREENH가 덮어쓴다.
inline constexpr int kDefaultScreenWidth = 640;
inline constexpr int kDefaultScreenHeight = 480;
// 원본 창 클래스 이름(PTR_s_NetstormClient_0053192c)과 창 제목(번역 전 원문).
inline constexpr const char* kWindowClassName = "NetstormClient";
inline constexpr const char* kWindowTitle = "Activision and Titanic Entertainment Present: NetStorm";

// 프레임 제한 간격(초)을 구한다. maxFps 가 0 이하면 0(제한 없음)이다.
double FrameIntervalSeconds(int maxFps);

// 1ms 눈금에서 원본 대기가 끝나는 최소 간격. 75fps이면 ceil(1000/75) = 14ms다.
// 원본 자체는 실수 간격의 바쁜 대기다. 여기서는 그 눈금의 결과만 계산한다.
std::uint32_t QuantizedFrameMilliseconds(int maxFps);

// 원본 WinMain의 인자와, 원본에 없는 검사용 옵션.
struct ClientOptions {
    std::filesystem::path gameDirectory;   // 원본은 명령줄 또는 실행 파일 위치에서 정한다(FUN_00435cd0).
    o::OriginalEdition edition{o::OriginalEdition::Patch1078};
    bool forceWindow{};                    // 원본 명령줄 "window": 전체화면 설정을 무시한다.
    std::string settings;                  // 원본 명령줄의 설정 글(`키=값;키=값`).
    std::string mission;                   // 복원 검사용: 메뉴 목록이 없는 사용자 미션도 같은 브리핑/월드 경로로 시작한다.
    std::uint64_t frameLimit{};            // 새 옵션(검사용): 0이 아니면 그만큼 그린 뒤 창을 닫는다.
    std::filesystem::path screenshot;      // 새 옵션(검사용): 닫기 직전의 화면을 BMP로 저장한다.
    bool noAudio{};                        // 새 옵션(검사용): 참이면 소리 장치와 음악 스레드를 만들지 않는다(자동 검사가 소리를 내지 않게 한다).
    bool audioMute{};                      // 새 옵션(검사용): 참이면 장치를 열고 음악을 실제로 재생하되 소리는 내지 않는다(음소거 깊이 1로 시작).
    std::filesystem::path audioReport;     // 새 옵션(검사용): 닫기 직전의 소리·음악 상태를 이 파일에 `이름	값` 줄로 저장한다.
};

// 원본 전역 변수로 흩어져 있던 클라이언트 상태를 객체 하나로 묶는다. 한 프로세스에 하나만 만든다.
class Client {
public:
    explicit Client(ClientOptions options);
    ~Client();
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    // 원본 WinMain(FUN_00438dc0): 초기화하고 WM_QUIT까지 루프를 돈다. 반환값은 종료 코드다.
    int Run();

    // 원본 00436e40 ↔ CD 00487720: UberGump의 버튼 사건에서 전체화면 시작 표시 파일을 지운다.
    // 기본 메뉴의 활성 버튼 사건에 연결한다. 일반 종료에서는 부르지 않는다.
    void ClearFullScreenState();
    // 메뉴/브리핑의 게임 시간 정지. 같은 상태를 반복 적용해도 중첩하지 않는다.
    void Pause(bool paused);
    // 원본 게임 시계의 실제 정지 여부를 관찰한다.
    bool Paused() const;
    // 원본 세 해상도의 창 모드 장치와 Renderer를 다시 만든다.
    void ChangeResolution(int width, int height);
    // 원본 창 제목을 현재 미션/메뉴 상태에 맞춘다.
    void Title(std::string_view title);
    // 기본 실행의 메뉴 상태와 입력 경로. --view 검사에서는 널이다.
    UberGump* Menu();
    // 검사 전용으로 이미 그려진 화면을 지정 파일에 저장한다.
    void Capture(const std::filesystem::path& path);

    // 독립 검사 화면의 입력 연결점. 기본 실행은 부분 UserInput/UberGump를 사용한다. 참이면 종료한다.
    std::function<bool(Client&)> input;
    // 초기화가 끝나고 루프에 들어가기 직전에 한 번 부른다(새 연결점).
    std::function<void(Client&)> ready;
    // 검사 전용 프레임 입력. 일반 실행에서는 비어 있다.
    std::function<void(Client&)> beforeInput;
    // 프레임 그리기 뒤의 상태/화면 관찰. 일반 실행에서는 비어 있다.
    std::function<void(Client&)> afterFrame;

    // 원본 FUN_00452e00: 키·버튼의 현재 상태와 커서 위치를 읽는다. code는 가상 키를 16비트 민 값이다.
    InputEvent Poll(std::uint32_t code) const;

    Screen& GetScreen();
    InputQueue& Input();
    o::ConfigInterface& Configuration();
    const o::BaseFileSystem& Files() const;
    const GameAssets& Assets() const;
    o::Kernel& GetKernel();
    // 복원한 그리기·글꼴·커서 기반. 초기화 뒤에만 쓴다.
    Renderer& GetRenderer();
    FontStore& Fonts();
    Cursor& GetCursor();
    // 그린 프레임·출력 사각형 횟수. 초기화 도중 창을 닫았으면 두 값 모두 0이다.
    std::pair<std::uint64_t, std::uint64_t> RenderCounts() const;
    // 로딩 화면에서 실제 Renderer 장면으로 전환한다. 후속 메인 메뉴도 이 경로를 사용한다.
    void ShowScene();
    // 소리·음악 묶음. 소리를 끈 실행(noAudio)이거나 초기화 전이면 널이다.
    ClientAudio* Audio();
    // 원본 Interpret Options(00435220)의 소리 부분: 설정을 다시 읽어 장치·음량·현재 곡을 맞춘다. 옵션 메뉴에서 소리 설정을 바꾼 뒤 부른다.
    void ApplyAudioOptions();
    // 월드에 붙은 번개 프로세스를 화면/월드 해제 전에 제거한다. 부모 Squid 수명을 대신하는 현재 어댑터의 경계다.
    void ClearThunderFlashes();
    // 같은 프레임의 검사 보고에 쓰는 현재 등록된 번개 프로세스 수다.
    std::size_t ThunderFlashCount() const;
    // 이번 프레임에 고정된 시각(원본 FUN_00460e90).
    const o::FrameTime& Time() const;
    // 창이 활성인가(원본 DAT_0054dc50).
    bool Active() const;
    // 번역표로 글을 바꾼다(원본 FUN_004de9a0).
    std::string Translate(std::string_view text) const;
    // 창 프로시저의 본체(원본 FUN_00436550). 정적 창 프로시저가 부른다.
    std::intptr_t HandleMessage(NativeHandle window, unsigned message, std::uintptr_t wParam, std::intptr_t lParam);
private:
    // 날씨의 완전한 파일 이름을 DataDir 아래에서 읽어 화면에 적용하고 색 표·커서·월드/UI를 갱신한다.
    void LoadScenePalette(std::string_view name);
    // 천둥 곡의 화면 효과를 현재 월드에 붙인다. 월드가 없으면 원본의 부모 조회 실패처럼 생성하지 않는다.
    void StartThunderFlash();
    // 원본 00441d10·00441de0: 게임 폴더의 d/options.cfg에 저장한다. 변경 검사 여부는 호출 위치가 정한다.
    void SaveOptions();
    // 원본 FUN_00435220("Interpret Options")의 일부: 화면 크기·창 위치·프레임 제한 등 설정을 읽는다.
    void InterpretOptions();
    // 원본 FUN_00436260: 로딩 화면(검은 바탕, 가운데 그림, 오른쪽 아래의 두 줄 글).
    void PaintLoading(NativeHandle dc);
    // 원본 FUN_00436450: 프레임 제한 뒤 그리고 창으로 내보낸다.
    void Frame();
    // 원본 FUN_00436020 / FUN_00436190: 키·글자 사건을 큐에 넣는다.
    void KeyEvent(NativeHandle window, unsigned message, std::uintptr_t wParam, std::intptr_t lParam);
    void CharacterEvent(NativeHandle window, std::uintptr_t wParam);
    // 원본 FUN_00435b30: 초기화 도중 쌓인 창 메시지를 처리한다. WM_QUIT이면 거짓.
    bool PumpMessages();
    // 화면을 BMP로 저장한다(새 검사 기능).
    void SaveScreenshot();
    // 설정 객체에서 소리·음악 설정(sound·music·soundQuality·soundVolume·musicVolume·maxSimulSounds·swapLeftRightSpeakers)을 읽는다.
    // 읽지 못한 키는 원본 setup.cfg·options.cfg의 기본값을 유지한다(원본 FUN_00441270의 ReadInt와 같다).
    ClientAudioOptions ReadAudioOptions();

    ClientOptions options_;
    o::BaseFileSystem files_;
    o::ConfigInterface configuration_;
    std::optional<o::XlatTable> translations_;
    int languageNumber_{1};
    NativeHandle instance_{};          // DAT_0054dbf8
    NativeHandle resources_{};         // 원본 실행 파일을 자료로 연 모듈(아이콘·커서·로딩 그림). 없으면 널.
    NativeHandle window_{};            // DAT_0054dbf4
    NativeHandle windowDc_{};          // DAT_0054d960
    NativeHandle loadingBitmap_{};     // DAT_0054de70
    NativeHandle loadingFont_{};       // DAT_0054de74
    std::unique_ptr<Screen> screen_;
    std::unique_ptr<GameAssets> assets_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<FontStore> fonts_;
    std::unique_ptr<Cursor> cursor_;
    std::unique_ptr<UberGump> menu_;
    std::unique_ptr<ClientAudio> audio_; // 원본 "init sound"가 만드는 소리·음악 묶음. 화면보다 먼저 닫는다.
    bool sceneVisible_{};
    InputQueue input_;
    o::GameClock clock_;
    o::FrameTime time_{};
    o::Kernel kernel_;
    std::vector<o::ProcessId> thunderProcesses_; // 커널이 소유한 번개 번호. 파괴 콜백에서 제거하므로 재사용된 슬롯을 지우지 않는다.
    int screenWidth_{kDefaultScreenWidth};   // DAT_00531860
    int screenHeight_{kDefaultScreenHeight}; // DAT_00531864
    int windowWidth_{-1};              // DAT_00542390
    int windowHeight_{-1};             // DAT_00542394
    int clientWidth_{};                // DAT_005c78fc
    int clientHeight_{};               // DAT_005c7900
    std::int32_t initWindowPos_{};     // DAT_0054db40
    std::int32_t maxFps_{kDefaultMaxFps}; // DAT_005318d8
    double frameInterval_{};           // DAT_005318e0
    std::int32_t sleepPerLoop_{};      // DAT_0054db0c
    std::int32_t highPriority_{};      // DAT_0054db2c
    double lastDraw_{};                // DAT_0054de80
    bool active_{};                    // DAT_0054dc50
    bool activatedByClick_{};          // DAT_0054dee4
    bool nonClientClick_{};            // DAT_0054dee0
    bool closing_{};                   // DAT_0054dee8
    std::uint64_t frames_{};
};

}  // namespace netstorm::client
