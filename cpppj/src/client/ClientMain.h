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
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace netstorm::o { class RawFortBadgesGlobals; } // Client 수명의 원본 Badges 장부를 고정 주소로 소유한다.
namespace netstorm::client {
class UberGump;
class ClientAudio;
class RawChatChannels;
class FortTypeConversionStorage; // 원본 현재 변환표의 실제 할당을 Client 수명에 소유한다.
class ClientColorSortInput; //최초논리RGB와43c010의같은표입력을Client수명으로소유한다.
class ClientRandomGlobals; // 같은 Client의 CRT holdrand와 Subscriber ID 최초시드 전역을 소유한다.
class ClientSpGlobals; // 공통 시작의 SP/추적 offset을 사제와 Session보다 오래 같은 주소에 소유한다.
class ClientKencloudGlobals; // 같은 구름/팔레트 전역과 현재 메모리를 모든 메뉴/장치 교체에서 유지한다.
class ClientSubscriberGlobals; // 같은 Client의 실제 my/login 두 DWORD만 장기 소유한다.
class ClientLocalStateGlobals; // 실제 메뉴/Editor/로그인이 같은 State 블록과 벽시각 기한을 빌린다.
class ClientConnectionActivityGlobals; // 실제 두22DWORD 활동 표와 Zone 간극을 Client 수명으로 소유한다.
class ClientFortGlobals; // 현재/pending 이름·FileSpec·목록 dirty와 선택 모드의 실제 Client 소유자다.
class ClientCoreDataGlobals; //현재Core/active/캐시와 빌린 Player 슬롯을 Client 수명에 소유한다.
class ClientPlayerRecords; // Player/factory 전체 할당과 로그인 카운터를 Session보다 오래 소유한다.
class ClientChatManagerGlobals; //현재 Chatmanager와 실제 미초기화 할당들을 Client 수명에 소유한다.
class ClientStaticStrings; // 원문 공유 임시 고리의 같은 네 DWORD와 실제 할당 수명을 소유한다.
class ClientDotCommandGlobals; //최초 say와 같은 인접 명령 표를 Session보다 오래 보유한다.
class RawSurfaceStorage; // 실제 Pool/Hash/TYPE/spot을 모든 Session과 초기 HUD보다 오래 보유한다.
class ClientGameplayGlobals; // 실제 현재 Player/커서/미션 전역과 짧은 Runtime 연결을 소유한다.
struct ClientChatWindowGlobals; // 실제 Chat 현재 전역과 공유 로그의 장기 소유자다.
struct ClientHtmlWindowGlobals; // 같은 Client의 HTML 수/serial/종료 시각 소유자다.
struct ClientNsInfoGlobals; // 실제 NSInfo/zone/선택 서버와 이름 버퍼의 장기 소유자다.
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
    // edition: 판본별 필드 배치와 동작을 선택하는 원본 버전.
    o::OriginalEdition edition{o::OriginalEdition::Patch1078};
    bool forceWindow{};                    // 원본 명령줄 "window": 전체화면 설정을 무시한다.
    std::string settings;                  // 원본 명령줄의 설정 글(`키=값;키=값`).
    std::string mission;                   // 복원 검사용: 메뉴 목록이 없는 사용자 미션도 같은 브리핑/월드 경로로 시작한다.
    std::uint64_t frameLimit{};            // 새 옵션(검사용): 0이 아니면 그만큼 그린 뒤 창을 닫는다.
    std::filesystem::path screenshot;      // 새 옵션(검사용): 닫기 직전의 화면을 BMP로 저장한다.
    bool noAudio{};                        // 새 옵션(검사용): 참이면 소리 장치와 음악 스레드를 만들지 않는다(자동 검사가 소리를 내지 않게 한다).
    bool audioMute{};                      // 새 옵션(검사용): 참이면 장치를 열고 음악을 실제로 재생하되 소리는 내지 않는다(음소거 깊이 1로 시작).
    std::filesystem::path audioReport;     // 새 옵션(검사용): 닫기 직전의 소리·음악 상태를 이 파일에 `이름	값` 줄로 저장한다.
    std::string testBridgePatterns; // 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. 명시 CLI에서만 지정하고 설정 파일에 저장하지 않는다.
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
    // 메뉴/브리핑 UI가 보유한 게임 시간 정지만 변경한다. 같은 요청은 중첩하지 않고 다른 소유자의 정지를 해제하지 않는다.
    void Pause(bool paused);
    // 원본00460df0의 중첩 정지 한 깊이를 보유한다. 미션의 scriptPaused가 중복 요청을 막아야 한다.
    void PushGamePause();
    // 원본00460e30의 정지 한 깊이를 반납한다. 호출자가 자신이 보유한 깊이와 짝을 맞춘다.
    void PopGamePause();
    // 원본 게임 시계의 실제 정지 여부를 관찰한다.
    bool Paused() const;
    // 00460e70의 같은 시계 누적 정지 시간만0으로 쓴다. Native cleanup 호출 연결은 별도이며 현재 프레임/원점/정지 소유권은 보존한다.
    void ResetGameClock();
    // 원본 세 해상도의 창 모드 장치와 Renderer를 다시 만든다.
    void ChangeResolution(int width, int height);
    // 원본 창 제목을 현재 미션/메뉴 상태에 맞춘다.
    void Title(std::string_view title);
    // 기본 실행의 메뉴 상태와 입력 경로. --view 검사에서는 널이다.
    UberGump* Menu();
    // 메뉴/월드 유무와 관계없이 같은 Client의 실제00540c74 const참조다.
    const std::uint32_t& CurrentSubscriber() const;
    // Client 전체 수명의 유일한 PATCH Chat 기록 처리기다. 미션 교체 때 새로 만들거나 지우지 않는다.
    RawChatChannels& ChatChannels();
    // 같은 실제 100기록을 읽기 전용으로 빌린다. UI/검사는 별도 채널 사본을 만들지 않는다.
    std::span<const std::uint8_t> ChatRecords() const;
    // 번역 표와 동일한 현재 원본 언어 번호를 지식 창의 언어3 설명 생략 분기에 제공한다.
    int LanguageNumber() const { return languageNumber_; }
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
    // 검사 전용 폴링 공급자다. 값이 있으면 해당 키/버튼에 적용하고 빈 값이면 실제 Win32 상태를 읽는다.
    std::function<std::optional<InputEvent>(std::uint32_t)> inspectPoll;

    // 원본 FUN_00452e00: 키·버튼의 현재 상태와 커서 위치를 읽는다. code는 가상 키를 16비트 민 값이다.
    InputEvent Poll(std::uint32_t code) const;

    Screen& GetScreen();
    void LoadPaletteFile(std::string_view path,std::uint32_t clear); //GamePalSpec완성경로를DataDir재결합없이같은Screen에전달한다.
    void RebuildColorSortMaps(); //43c010의고정초기RGB/현재빨강9표만생성하며소비자갱신과구별한다.
    void RefreshPaletteConsumers(); //기존AudioTints/커서/월드/UI소비자를이후maps경계에서갱신한다.
    InputQueue& Input();
    o::ConfigInterface& Configuration();
    // 원본0054db30의 Client 수명 DWORD를 빌린다. 저장할 때 설정을 다시 조회하지 않는다.
    const std::uint32_t& ReadOnlyForts() const;
    std::uint32_t ScriptMenuGlobal(std::uint32_t address) const; // HTML Checked가 같은5cab30/55a480의 현재 허용 조건을 읽는다. 다른 주소는 거부한다.
    // 미션 교체와 무관한 같은 실제 장부의 공유 수명을 Session에 전달한다.
    const std::shared_ptr<o::RawFortBadgesGlobals>& FortBadges() const;
    const std::shared_ptr<FortTypeConversionStorage>& FortTypeConversion() const; // 로더들이 같은 current DWORD와 실제255BYTE 할당을 빌린다.
    const std::shared_ptr<ClientColorSortInput>& ColorSortInput() const; //현재ScreenRGB로재캡처하지않는같은초기입력소유자를빌린다.
    const std::shared_ptr<ClientSubscriberGlobals>& SubscriberGlobals() const; //Init/Session이 같은 실제 두 DWORD를 빌리는 owner다.
    const std::shared_ptr<ClientRandomGlobals>& RandomGlobals() const; // 미션마다 복제하지 않는 실제 난수/ID초기화 소유자를 전달한다.
    const std::shared_ptr<ClientSpGlobals>& SpGlobals() const; // 생성자/getter에서 초기화하지 않는 같은 SP/추적 DWORD 소유자다.
    const std::shared_ptr<ClientKencloudGlobals>& KencloudGlobals() const; // 같은 실제 구름/팔레트/현재 Renderer 전역을 순수 조회한다.
    void ChangeFortPalette(const std::span<const std::uint8_t>& name); //004b6100의 전체 부모를 같은 구름/Screen clear1/추가9표/dirty에 실행한다.
    const std::shared_ptr<ClientLocalStateGlobals>& LocalStateGlobals() const; // 실제 State144BYTE와 활동기한의 Client 수명을 Session에 전달한다.
    const std::shared_ptr<ClientConnectionActivityGlobals>& ConnectionActivityGlobals() const; // 같은 실제 활동 표/Zone DWORD의 고정 주소를 Session에 빌린다.
    const std::shared_ptr<ClientFortGlobals>& FortGlobals() const; // 같은 이름/키/목록 물리를 메뉴/Native 이름 부모에 빌린다.
    const std::shared_ptr<ClientCoreDataGlobals>& CoreDataGlobals() const; //같은 현재 Core 슬롯/캐시를 빌리며 Player 자동 선택이나 해제를 실행하지 않는다.
    const std::shared_ptr<ClientPlayerRecords>& PlayerRecords() const; // 실제 세션이 같은 할당 소유자와 대여 관문을 빌린다.
    const std::shared_ptr<ClientGameplayGlobals>& GameplayGlobals() const; // 초기 HUD와 Session에 같은 현재 전역을 전달한다.
    const std::shared_ptr<ClientHtmlWindowGlobals>& HtmlWindowGlobals() const; // 같은 장기 HTML 전역을 빌린다.
    const std::shared_ptr<ClientChatWindowGlobals>& ChatWindowGlobals() const; // 같은 실제 Chat 현재/공유 로그 상태를 빌린다.
    const std::shared_ptr<ClientNsInfoGlobals>& NsInfoGlobals() const; // 현재 NSInfo와 서버 이름을 같은 고정 주소에서 빌린다.
    const std::shared_ptr<ClientChatManagerGlobals>& ChatManagerGlobals() const; //실제 manager 소유자를 빌리며 조회로 공통 startup을 실행하지 않는다.
    const std::shared_ptr<ClientStaticStrings>& StaticStrings() const; // 같은 고리의 실제 수명을 빌리며 조회 자체는 예약/할당하지 않는다.
    const std::shared_ptr<ClientDotCommandGlobals>& DotCommandGlobals() const; //최초 say를 보존하는 같은 장기 등록 표를 빌린다.
    const o::BaseFileSystem& Files() const;
    const GameAssets& Assets() const;
    o::Kernel& GetKernel();
    const std::shared_ptr<o::Kernel>& ProcessKernelOwner() const; // 같은 실제 전역 Kernel 소유권만 전달한다.
    const std::shared_ptr<o::FrameTime>& FrameClockOwner() const; // 같은 Time().game 주소가 외부 Storage보다 먼저 사라지지 않게 한다.
    const std::shared_ptr<RawSurfaceStorage>& SurfaceStorage() const; // 자산 초기화 뒤 모든 Session과 초기 HUD가 같은 물리를 빌린다.
    const std::shared_ptr<RawSurfaceStorage>& SurfaceStorageIfInitialized() const { return surfaceStorage_; } // 초기 정리 연결은 미초기화 NULL도 순수 조회한다. 기존 SurfaceStorage의 필수 초기화 검사는 유지한다.
    // 복원한 그리기·글꼴·커서 기반. 초기화 뒤에만 쓴다.
    void InitializeNativeRendererView(); // 실제 Native Renderer 초기화 위치에서만64BYTE 이미지/목표/갱신을 구성한다.
    Renderer& GetRenderer();
    std::string_view TestBridgePatterns() const { return options_.testBridgePatterns; } // 테스트 전용 지정 순서다. 비어 있으면 원본 동작이며 원본 분석 결과가 아니다.
    FontStore& Fonts();
    Cursor& GetCursor();
    // 그린 프레임·출력 사각형 횟수. 초기화 도중 창을 닫았으면 두 값 모두 0이다.
    std::pair<std::uint64_t, std::uint64_t> RenderCounts() const;
    // 로딩 화면에서 실제 Renderer 장면으로 전환한다. 후속 메인 메뉴도 이 경로를 사용한다.
    void ShowScene();
    void PresentScene(); // 현재 장면만 즉시 출력한다. 미션 파일 적재 전 안내를 표시하며 루프/시계/검사 프레임은 진행하지 않는다.
    // 소리·음악 묶음. 소리를 끈 실행(noAudio)이거나 초기화 전이면 널이다.
    ClientAudio* Audio();
    // 캠페인의 유일한 이름표/재생 계층이다. noAudio에서는 장치·스레드 없는 비활성 묶음을 빌린다.
    // init sound 이전에는 예외다. Audio()의 noAudio/null 계약은 그대로 유지한다.
    ClientAudio& WorldAudio();
    // 원본 init sound 위치에서 실제 음향을 한 번 만든다. noAudio 검사는 창·장치 없이 같은 비활성 owner를 준비한다.
    void InitializeNativeAudio();
    // 원본 Interpret Options(00435220)의 소리 부분: 설정을 다시 읽어 장치·음량·현재 곡을 맞춘다. 옵션 메뉴에서 소리 설정을 바꾼 뒤 부른다.
    void ApplyAudioOptions();
    // 월드에 붙은 번개 프로세스를 화면/월드 해제 전에 제거한다. 부모 Squid 수명을 대신하는 현재 어댑터의 경계다.
    void ClearThunderFlashes();
    // 같은 프레임의 검사 보고에 쓰는 현재 등록된 번개 프로세스 수다.
    std::size_t ThunderFlashCount() const;
    // 이번 프레임에 고정된 시각(원본 FUN_00460e90).
    const o::FrameTime& Time() const;
    // 원본 00460d70처럼 호출 때마다 timeGetTime을 읽는다. 버튼 예약의 별도 조회를 프레임 값으로 합치지 않는다.
    double WallSeconds() const;
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
    // 원본00435860의 현재 ReadInt만 재해석한다. 설정 변경 callback과 초기 해석이 공유한다.
    void InterpretFortFileOptions();
    void InterpretScriptMenuOptions(); // 원본4353ea의 개발 모드0/현재 userCheat ReadInt·허용값 검사를 같은 장기 전역에 적용한다.
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

    // options_: 원본 WinMain의 인자와, 원본에 없는 검사용 옵션 (ClientOptions).
    ClientOptions options_;
    // files_: 디스크·보조 디렉터리·아카이브 순서로 게임 파일을 찾는 파일 시스템 (o::BaseFileSystem).
    o::BaseFileSystem files_;
    // configuration_: 현재 언어와 실행 설정을 조회하는 인터페이스.
    o::ConfigInterface configuration_;
    // 원본0054db30의 최초 BSS0이다. 미션 교체·누락 키로 초기화하지 않는 실제 한 DWORD다.
    std::uint32_t readOnlyForts_{};
    std::shared_ptr<o::RawFortBadgesGlobals> fortBadges_; // CRT 한 번과 시작 count reset을 구별하는 단일6160BYTE 소유자다.
    std::shared_ptr<FortTypeConversionStorage> fortTypeConversion_; // Session마다 새 현재 포인터나 할당 사본을 만들지 않는 단일 소유자다.
    std::shared_ptr<ClientSubscriberGlobals> subscriberGlobals_; //Chat/메뉴/Session보다 먼저 만들고 그 소비자보다 오래 유지한다.
    std::shared_ptr<ClientRandomGlobals> randomGlobals_; // 모든 실제 Session과 Subscriber 생성기가 같은 holdrand/초기화DWORD를 빌린다.
    std::shared_ptr<ClientSpGlobals> spGlobals_; // 같은 난수 뒤에 만들고 모든 Session/사제 반납 뒤에 소멸하는 실제 SP/추적 전역이다.
    std::shared_ptr<ClientKencloudGlobals> kencloudGlobals_; // 구름 현재 주소/이름/옵션과 Native redraw를 같은 Client 수명에 보관한다.
    std::shared_ptr<ClientColorSortInput> colorSortInput_; //메뉴/월드/두Screen생성경로보다오래유지하는같은초기RGB다.
    std::shared_ptr<ClientLocalStateGlobals> localStateGlobals_; // Session 소멸/Editor 메뉴 재진입에서 State/기한을 초기화하지 않는 한 소유자다.
    std::shared_ptr<ClientConnectionActivityGlobals> connectionActivityGlobals_; // 메뉴/Session보다 먼저 구성하고 소비자 반납 뒤까지 두 표와 Zone 값을 보존한다.
    std::shared_ptr<ClientFortGlobals> fortGlobals_; // 모든 메뉴/Session보다 오래 사는 같은 두 이름/목록/dirty 소유자다.
    std::shared_ptr<ClientCoreDataGlobals> coreDataGlobals_; //menu/Session보다 먼저 선언해 소비자 반납 뒤에도 현재/캐시가 살아 있게 한다.
    std::shared_ptr<ClientPlayerRecords> playerRecords_; // 모든 Session 고정뷰가 소멸한 뒤에 실제 Player/factory를 반납한다.
    std::shared_ptr<ClientGameplayGlobals> gameplayGlobals_; // Subscriber 뒤·UI/Session 앞에 구성한 실제 현재 전역이다.
    std::shared_ptr<ClientHtmlWindowGlobals> htmlWindowGlobals_; // 표시 부모 교체보다 오래 유지하는 실제 HTML 전역이다.
    std::shared_ptr<ClientChatWindowGlobals> chatWindowGlobals_; // 표시 부모 교체보다 오래 유지하는 공유 상태다.
    std::shared_ptr<ClientNsInfoGlobals> nsInfoGlobals_; // 모든 실제 NSInfo 자식/문자 소비자보다 오래 유지한다.
    std::shared_ptr<ClientChatManagerGlobals> chatManagerGlobals_; //같은 Client의 현재 manager/할당을 보유하며 Session마다 재구성하지 않는다.
    std::shared_ptr<ClientStaticStrings> staticStrings_; // 메뉴/Session보다 먼저 만들고 그 소비자 뒤에 반납하는 원문 임시 고리다.
    std::shared_ptr<ClientDotCommandGlobals> dotCommandGlobals_; //menu/Session보다 먼저 구성한 실제 인접 표이며 현재 count를 별도로 복제하지 않는다.
    struct ChatState; // 실제 전역 Chat 기록과 현재 subscriber 처리기를 구현 파일의 고정 주소에 소유한다.
    std::unique_ptr<ChatState> chat_; // 메뉴/월드보다 오래 사는 단일100×48바이트이며 미션 수명과 분리한다.
    // translations_: 용어 키를 언어별 표시 문자열로 바꾸는 번역표.
    std::optional<o::XlatTable> translations_;
    // languageNumber_: 현재 언어를 언어 표에서 찾는 번호.
    int languageNumber_{1};
    // instance_: 현재 Win32 프로그램 인스턴스 핸들.
    NativeHandle instance_{};          // DAT_0054dbf8
    NativeHandle resources_{};         // 원본 실행 파일을 자료로 연 모듈(아이콘·커서·로딩 그림). 없으면 널.
    // window_: 게임 출력이나 장치 협조 수준에 사용하는 Win32 창 핸들.
    NativeHandle window_{};            // DAT_0054dbf4
    // windowDc_: Win32 창으로 그림을 전송할 장치 컨텍스트.
    NativeHandle windowDc_{};          // DAT_0054d960
    // loadingBitmap_: 초기 로딩 화면에 그릴 GDI 비트맵 핸들.
    NativeHandle loadingBitmap_{};     // DAT_0054de70
    // loadingFont_: 초기 로딩 화면의 문자를 그릴 GDI 글꼴 핸들.
    NativeHandle loadingFont_{};       // DAT_0054de74
    // screen_: 원본 Screen.cpp의 전역 변수 묶음을 객체 하나로 둔다. 창과 창의 장치 문맥은 ClientMain이 만든다 (std::unique_ptr<Screen>).
    std::uint32_t paletteLoaded_{}; //00555e08실제DWORD를Screen보다오래소유하여해상도재생성으로0복제하지않는다.
    std::unique_ptr<Screen> screen_;
    // assets_: 타입·셰이프·팔레트 자산 또는 해당 자산의 처리 경계.
    std::unique_ptr<GameAssets> assets_;
    std::shared_ptr<RawSurfaceStorage> surfaceStorage_; // 자산 뒤·UI/Session 앞에 확보해 모든 물리 참조보다 오래 유지한다.
    // renderer_: 수집한 그림·텍스트를 화면에 그리는 렌더러.
    std::unique_ptr<Renderer> renderer_;
    // fonts_: 이름·크기로 찾아 재사용하는 글꼴 저장소.
    std::unique_ptr<FontStore> fonts_;
    // cursor_: 원본 커서 그림과 핫스폿을 Win32 커서로 만들어 선택하는 관리자 (std::unique_ptr<Cursor>).
    std::unique_ptr<Cursor> cursor_;
    // menu_: 메뉴·브리핑·미션 페이지와 해당 UI 동작을 관리하는 처리기 (std::unique_ptr<UberGump>).
    std::unique_ptr<UberGump> menu_;
    std::unique_ptr<ClientAudio> audio_; // 원본 "init sound"의 실제 장치/음악 묶음. 월드 뒤·화면 전에 닫는다.
    std::unique_ptr<ClientAudio> disabledAudio_; // noAudio 전용 실제 이름표/비활성 재생 묶음. 장치·스레드는 열지 않는다.
    // sceneVisible_: 장면 표시 여부.
    bool sceneVisible_{};
    // input_: 처리할 입력 또는 입력 분배기.
    InputQueue input_;
    // clock_: 프레임 판정에 현재 게임 시각을 공급하는 시계.
    o::GameClock clock_;
    bool uiPaused_{}; // 메뉴/브리핑의 단일 정지 소유 표식이다. 원본 미션 scriptPaused와 별도로 유지한다.
    std::int32_t movieUpdatePauseDepth_{}; // 원본0054dc58의 BSS0 정상 경계다. Moviegump483680/4837f0의435dc0 소유 연결은 미복원이며 GameClock 깊이로 합성하지 않는다.
    // time_: 프레임의 게임 시각과 벽 시각 및 시간 차 (o::FrameTime).
    std::shared_ptr<o::FrameTime> time_{std::make_shared<o::FrameTime>()}; // 같은 실제 프레임 객체를 장기 Host도 pin하며 별도 시각 사본을 만들지 않는다.
    // kernel_: 프로세스 등록과 프레임 실행을 담당하는 커널.
    std::shared_ptr<o::Kernel> kernel_{std::make_shared<o::Kernel>()}; // Client와 장기 Host가 동일 Kernel의 수명을 공유한다.
    std::vector<o::ProcessId> thunderProcesses_; // 커널이 소유한 번개 번호. 파괴 콜백에서 제거하므로 재사용된 슬롯을 지우지 않는다.
    // screenWidth_: 게임 화면의 픽셀 너비.
    int screenWidth_{kDefaultScreenWidth};   // DAT_00531860
    // screenHeight_: 게임 화면의 픽셀 높이.
    int screenHeight_{kDefaultScreenHeight}; // DAT_00531864
    // windowWidth_: Win32 창의 너비.
    int windowWidth_{-1};              // DAT_00542390
    // windowHeight_: Win32 창의 높이.
    int windowHeight_{-1};             // DAT_00542394
    // clientWidth_: Win32 클라이언트 영역의 너비.
    int clientWidth_{};                // DAT_005c78fc
    // clientHeight_: Win32 클라이언트 영역의 높이.
    int clientHeight_{};               // DAT_005c7900
    // initWindowPos_: 초기화 창 위치.
    std::int32_t initWindowPos_{};     // DAT_0054db40
    // maxFps_: 화면 갱신의 초당 최대 프레임 수.
    std::int32_t maxFps_{kDefaultMaxFps}; // DAT_005318d8
    // frameInterval_: 화면 프레임 갱신의 최소 간격(초).
    double frameInterval_{};           // DAT_005318e0
    // sleepPerLoop_: 프레임 루프가 반복할 때 적용하는 대기 시간.
    std::int32_t sleepPerLoop_{};      // DAT_0054db0c
    // highPriority_: 실행 우선순위를 높이는 설정을 적용할지.
    std::int32_t highPriority_{};      // DAT_0054db2c
    // lastDraw_: 직전 화면 그리기의 벽 시계 시각(초).
    double lastDraw_{};                // DAT_0054de80
    // active_: 현재 입력·실행 대상이 활성 상태인지.
    bool active_{};                    // DAT_0054dc50
    // activatedByClick_: 활성화 조회기준 클릭 여부.
    bool activatedByClick_{};          // DAT_0054dee4
    // nonClientClick_: 비 클라이언트 클릭 여부.
    bool nonClientClick_{};            // DAT_0054dee0
    // closing_: 창 또는 프로세스 종료 처리를 진행 중인지.
    bool closing_{};                   // DAT_0054dee8
    // frames_: 타입별 프레임 자료.
    std::uint64_t frames_{};
};

}  // namespace netstorm::client
