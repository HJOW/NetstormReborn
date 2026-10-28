// 원본 Ubergump.cpp의 메인 메뉴·Tell·미션 진입을 현재 표시 기반과 연결한다.
#pragma once
#include "client/DialogExitCommand.h"
#include "client/GumpPointerBoundary.h"
#include "client/Gump.h"
#include "client/RawButtonControls.h"
#include "client/ScriptCommand.h"
#include "client/HtmlTextParser.h"
#include "client/State.h"
#include "client/Mission.h"
#include "client/VFXDraw.h"
#include "o/Kernel.h"
#include "o/Squid.h"
#include "o/RawMissionGuide.h"
#include "o/RawFortMissionSave.h"
#include "o/RawLocalMissionState.h" //실제 Editor 부모 반환과 다섯 State 블록의 공통 확정 계약이다.
#include "o/RawStartupStateCheck.h" //State1/3의 같은 실제 저장상태 reader와 재접속 질문 장치를 공유한다.
#include "client/GuideMissionTransitions.h"
#include "client/TutorialInit.h"
#include "client/SoloMapCatalog.h"
#include <map>
#include <memory>

namespace netstorm::client {
class Client;
class GameWorld;
class UserInput;
class WarningMessages;
struct GameMenuBarState; // 현재Session의기존검산참조와메뉴serial만빌린다.
struct FortSelectionHooks; //편집 선택과 Test Battle 복귀가 같은 실제 파일/State/섬/Notify 장치를 사용한다.
class CampaignObjectMenuClient; // 같은 실제 원본 객체 메뉴와 현재 명령 큐를 연결한다.
struct MissionFortLoadRequest; // Init부터 실제 Session 설치까지 같은 요청과 Zone 이미지를 소유한다.
class CampaignTemplateStorage; // 원본 명시 Init/State 정리까지 같은9개 manager의 수명을 소유한다.
struct RawHtmlWindowContext; // 실제 전체 HTML에 현재 게임 소유자의 장치를 빌린다.
class TellFileMenus; // 실제 파일 목록의 등록 Config를 부모/미션보다 오래 유지한다.
class TextClipboard; //같은 Client UI의 모든 문자 입력이 공유하는 실제 CF_TEXT 부모다.
class EditorCatalogWorkspace; //목록/직접 Edit/이름 변경이 공유하는 장기 단일 namespace와 이관 장부다.
// 메뉴·브리핑·미션 페이지와 해당 UI 동작을 관리하는 처리기이다.
class UberGump {
public:
    // 초기화 뒤 원본 타이틀·구름·메뉴 자료를 읽고 기본 메뉴를 연다.
    explicit UberGump(Client& client);
    // 검사 장면과 미션 설정을 Renderer보다 먼저 해제한다.
    ~UberGump();
    // 원본 루프 시작의 State 자리에서 대기 명령을 실행하고 시간 제한을 갱신한다.
    void Tick();
    // 원본 루프8번의 Guide 자리다. 현재0..6 단계와 실제 공통 프레임을 입력보다 먼저 실행한다.
    void MissionFrame();
    void PrepareKernelFrame(); // 같은 실제 GameWorld가 전역 Kernel 앞에서 프레임 입력만 준비한다.
    void FinishKernelFrame(); // 전역 Kernel 뒤 같은 월드의 표시 자료만 수집한다.
    // 커널이 월드를 진행한 직후 변경된 실제 장면을 제출한다.
    void Frame();
    void ApplyCameraFrame(); //Present 말미의 실제 목표 제한/적용을 같은 Client·현재 월드·Gump/Screen 장치에서 수행한다.
    void RefreshMinimap(); // 원본00481320의커널앞위치에서실제같은HUD큐를소비한다.
    // 호스트 장면 합성을 예약한다. 원문0043dad0의 예산 재부착은 RefreshStormPower로 구별한다.
    void Refresh();
    void RefreshStormPower(); //0043dad0의 실제 현재 예산 serial Push/Attach/깊이 복원이다.
    void KillGumpsForState(); //004666d0의 같은 공통 등록표 필터로 Kill만 예약한다.
    void CleanupGumpsForState(); //004662b0 실제 공통 정리와 호스트 페이지 캐시 반납이며 State 확정은 하지 않는다.
    void ApplyHudModeForState(std::uint32_t mode); //00466280의 현재 명령/원문 모드를 자식 우선·숨김 포함으로 분배하며 State/loaded는 쓰지 않는다.
    void ReplaceHudForState(); //004c77c0의 전체 네 자식 공장을 World 생성 전에 구성하며 mode/Player/loaded를 쓰지 않는다.
    void CleanupNativeStateWorld(std::uint32_t reason); //004b2df0 전체 부모를 같은 실제 Client/UI와 Core0 뒤 Runtime 반납에 실행한다. State1 prefix/확정은 별도다.
    // 새 화면 팔레트를 월드 어댑터에도 전달하고 UI/표시를 다시 그린다.
    void PaletteChanged();
    // 현재 월드가 번개 효과의 부모로 살아 있는지 반환한다. 브리핑 정지 중에도 월드는 존재한다.
    bool HasWorld() const;
    // 현재 설치된 GameWorld의 실제 Session subscriber를 읽는다. 월드 생성/해제 중 미설치 상태는 0이다.
    std::uint32_t CurrentSubscriber() const;
    // 명령줄 검사용 미션도 State→Loading→Briefing의 같은 경로로 예약한다.
    void StartMission(std::string name);
    // 입력 큐·커서 폴링으로 돌 버튼/목록/ESC를 처리한다. 종료 요구면 참.
    bool Input();
    // 검사도 실제 메뉴가 사용하는 같은 사건 처리기로 입력한다.
    void Event(InputEvent event);
    // 자동 검사에서 화면과 활성 상태를 읽는다.
    std::string Report(bool cameraOnly=false) const; //카메라 검사 전용 간결 관측은 큰 월드 덤프를 생략하며 게임 상태를 바꾸지 않는다.
    // 버튼/목록의 실제 위치를 라벨로 찾는다. 없으면 빈 값.
    std::optional<ScreenPoint> ControlPoint(std::string_view label) const;
    bool QuitRequested() const { return quit_; } // 실제 QuitApp/사용자 종료 요청만 읽으며 검사 프레임 제한 종료와 구별한다.
    void CleanupGumps(); // 원본4662b0 위치에서 실제 전체 HTML/Warning을 포함한 같은 공통 큐를 정리한다.
    void IdleGumps(); // 원본464170 위치에서 Warning을 포함한 현재 공통 idle 표를 한 번 실행한다.
    // --ui-script 전용 검사 경계다. 실제 월드의 가상 삭제를 호출하며 일반 게임 입력에는 노출하지 않는다.
    bool InspectDestroySurface(std::string_view type,float x,float y);
    // --ui-script의 커널 검사에서 일반 게임과 같은 Tell 경로를 실행한다. 일반 사용자 명령으로 노출하지 않는다.
    void InspectTell(std::string target);
    void InspectBridgePatterns(std::string_view patterns); // 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. UI 검사에서 지정 순서를 교체/해제한다.
private:
    // 원본 Tell의 파일.섹션/현재 tell 섹션을 읽어 대화상자를 연다.
    void Tell(std::string target, bool briefing = false);
    // 원본00482de0처럼 현재 미션/일반 섹션이 없으면 false로 반환하며 기존 페이지를 유지한다.
    bool TryTell(std::string target,bool briefing=false,bool* windowCreated=nullptr,std::uint32_t suppress=0); // 원문 mode1은 첫 HTML 처리만 실행하고 창을 만들지 않는다.
    // 원본 이름의 설정 객체를 보존해 캠페인 제목과 변수를 치환한다.
    o::Config& Script(std::string_view name);
    RawHtmlWindowContext LoadingHtmlContext(ScreenRect mapView); // 같은 Client/Config/큐와 매번 현재 뷰를 공급한다.
    // 표시 상태를 합성한다. 페이지 교체 예약이 있을 때만 입력 영역과 실제 돌 버튼을 함께 교체한다.
    void Compose();
    // 새 페이지/팝업/해상도의 입력 영역 교체를 다음 합성에 예약한다. 단순 픽셀 갱신에는 쓰지 않는다.
    void RebuildControls();
    // 대기 중인 명령을 실행한다. 지원하지 않는 명령은 실제 동작으로 가장하지 않는다.
    void Execute(const DialogAction& action);
    bool FortSelectionEvent(const InputEvent& event); // 실제 AskEditFort/ReadFort를 같은 pending 이름·목록·숫자 nextState에 분배한다.
    void SelectCurrentFortName(std::string_view name); // 원문 MissionFortRequest.selectFort 위치에서 전체 현재 이름 부모를 실제 Client/Template에 연결한다.
    FortSelectionHooks FortEditEffects(); //실제 선택된 Battle14의 파일/State 보존 경계도 같은 현재 World와 작업 파일에 연결한다.
    void EnsureHudFactories(); //현재 Client 장기 전역·물리로 네 공장만 준비하며 새 월드/Session을 생성하지 않는다.
    void RetireWorldForState(); //실제 State 자식 정리 뒤 같은 Runtime의 호스트 참조만 반납한다. 게임 전역·기한·정지를 다시 쓰지 않는다.
    void InstallHud(); // 현재 실제 월드와 같은 공통 전체 Uber/자식 공장·도킹을 연결한다.
    void PollEditorState(); //메인 Edit의 next1과 SaveGoMain의 next3 및 뒤따르는 next4를 같은 Native State 분배기로 소비한다.
    void OpenSaveFortNameDialog(std::uint32_t fortMode); //현재 Editor의 실제 파일/헤더/이름 부모를 원문 fortMode 인자의 TextEntry 모달에 연결한다.
    void OpenSendMoneyDialog(std::uint32_t target); // 원본 숫자 창의 대상·현재 입력·확정이 같은 월드를 빌린다.
    o::LocalMissionParentResult InstallExistingEditorState(std::uint32_t selected); //State1의 정리/prefix/현재 파일/UI tail을 실행하며 공통 확정은 분배기에 맡긴다.
    void PrepareNativeFortState(bool editor); //State1/3의 같은 실제 cleanup/Chat/listener/Fort/Core/Renderer/Chunk/options 준비를 실행한다. HUD는 각 원문 위치의 별도 호출이다.
    o::LocalMissionParentResult InstallMainMenuNativeState(std::uint32_t selected); //State3의 실제 전체 부모를 실행하며 호스트 메뉴 표시는 Native 공통 확정 뒤에만 공개한다.
    o::StartupStateCheckHooks EditorStartupStateHooks(); //State1/3이 같은 실제 saved-state reader/벽시각을 사용한다. 기본 옵션을 강제로 바꾸지 않는다.
    void PublishNativeMainMenuState(); //실제 State3 공통 확정/다음4/world 반납을 확인한 뒤 Native 전역 재작성 없이 호스트 메뉴 표시만 공개한다.
    // 기존 미션을 원본 Begin 순서로 종료/해제하고 새 빈 미션을 준비한다.
    void BeginMission(std::string name,std::uint32_t demo=0,std::uint32_t mode=0);
    // 이전 현재가 없는 Guide 공장 위치에서만 Script/빈 Runtime을 만든다.
    void CreateMission(std::string name,std::uint32_t demo=0,std::uint32_t mode=0,o::ConfigInterface::FileReader source={}); //편집 테스트는 같은 작업 source를 미션에 넘길 수 있으며 일반 진입은 기존 자산 reader를 사용한다.
    // 교체마다 같은 실제 현재/빈 종료/해제/공장/설정/필드 소비자를 묶는다.
    GuideMissionTransitionHooks MissionTransitionEffects();
    // 실제 Guide 단계1의 Header/월드/로그인/커서를 설치하고 다음 단계만 요청한다.
    void InitializeMission(); // 실제 Tutorial Init은 번호/설정/전역을 쓰고 다음 루프의 로드 요청과 Header를 수행한다.
    void LoadMissionWorld(std::optional<std::string> requestedFort={}); // Init이 캡처한 요새 이름을 사용한다. 기존 Battle은 현재 설정을 읽는다.
    o::FortMissionSaveHooks FortMissionSaveEffects(); // 현재 Runtime의 이름과 같은 실제 sequence/flags를 동기 저장에 빌린다.
    // 실제 Guide 단계3에서 같은 설치된 Session의 Start를 실행한다.
    void StartMissionWorld();
    // 이미 읽힌 원본 브리핑 섹션 이름을 순서대로 연다.
    void AdvanceBriefing();
    // 현재 미션 자원을 해제하고 기본 메뉴로 돌아간다.
    void MainMenu();
    // 현재 미션/요새의 설정 제목을 원본 선택 순서로 조회하여 실제 창에 적용한다.
    void RefreshWindowTitle();
    // 원본 Options/Help의 목록과 하위 메뉴를 구성한다.
    void OpenMenu(std::string name);
    void OpenGameMenu(bool playerList=false); // 기본값은004cc210 가로막대, true는004cbf10 중앙 Player List를 같은 Session/controlled 생성기로 작성한다.
    void ToggleGameMenu(); // ESC의현재serial조회→열기또는Kill/serial0/팝업닫기를실행한다.
    bool GameMenuOpen() const; // 현재serial을공통등록표에서조회하며과거열림bool을저장하지않는다.
    bool GameMenuCommand(const InputEvent& event); // 메뉴부모가예약한현재원본명령을각실제소유자로분배한다.
    void ClearPage(); // 실제 전체 HTML 부모를 Kill만 예약하고 호스트 팝업 자료를 비운다.
    void ActivatePageAction(DialogAction action); // 메인/호스트 팝업 선택만 기존 전환 큐로 보낸다. 실제 HTML은 공통 부모가 원시 큐를 사용한다.
    void SyncTellPresentation(); // 현재 실제 전체 부모의 Kill을 읽어 표시 phase/호스트 정지만 반납하며 소멸을 앞당기지 않는다.

    // 원본 우클릭 메뉴의 연결점. 건설/수확 명령은 다음 단계에서 활성화한다.
    void OpenObjectMenu(o::SquidId id,const InputEvent& event);
    // 메뉴가 소유한 월드와 입력 참조를 안전한 순서로 제거한다.
    void ClearWorld();
    void DetachMissionWorldBindings(); //Session을 빌리는 Tutorial/Start/입력만 반납하고 실제 Guide 해제까지 미션 gate/state를 보존한다.
    void ReleaseCurrentGuideMission(); //선택된 Guide 소멸 시점에만 자기 정지/허용표/번호/단계를 반납한 뒤 스크립트와 Native mission을 해제한다.
    // client_: 원본 전역 변수로 흩어져 있던 클라이언트 상태를 객체 하나로 묶는다. 한 프로세스에 하나만 만든다 (Client).
    Client& client_;
    // state_: 처리 중 조회·갱신하는 상태 (State).
    State state_;
    std::shared_ptr<o::FortSerializationGlobals> serializationGlobals_; // 주Template·맵목록임시manager·실제Session이같은판본DWORD를빌리는Client수명이다.
    std::shared_ptr<CampaignTemplateStorage> templateStorage_; // 이전 Session이 사라져도 같은 Template9를 명시 정리 전까지 보유한다.
    std::shared_ptr<MissionFortLoadRequest> missionFortRequest_; //현재 Guide의 지연 요청이다. Guide/전체 종료에서 반납하며 이전 World의 State 반납은 보존한다.
    std::shared_ptr<std::array<std::uint8_t,87>> missionOptions_{std::make_shared<std::array<std::uint8_t,87>>()}; // 월드보다 오래 사는 실제 Init/로그인 옵션이다.
    std::shared_ptr<std::array<std::uint32_t,256>> missionRestrictions_{std::make_shared<std::array<std::uint32_t,256>>()}; // 설치 전/후가 공유하는 기술 허용 표다.
    std::uint32_t missionNumber_{},tutorialSeen_{}; // 실제5ca8e0/f0의 현재 전역이다.
    std::uint32_t& initLoginSubscriber_; //호환 이름이며 실제540c74의 같은 Client DWORD다.
    const std::uint32_t& initialSubscriber_; //실제54265c의 현재 const참조이며 생성 때1을 다시 쓰지 않는다.
    std::uint32_t nextMissionSaveToken_{}; // 64비트 포인터를 자르지 않는 실제 Runtime별 비영 수명 토큰이며 미션 교체에서 되돌리지 않는다.
    // input_: 처리할 입력 또는 입력 분배기.
    GumpControlInput input_;
    GumpPointerBoundaryState pointerBoundary_; // 원본 정적 안팎/버튼 기록은 같은 Client UI 수명 동안 유지하고 페이지·월드 교체에서 지우지 않는다.
    double edgeScrollSince_{}; //005cafc8의 장기 최초 시각이며 World/페이지 교체로 지우지 않는다.
    std::int32_t edgeScrollEnabled_{1},edgeScrollSpeed_{20}; //보호 PE의005318f8/005318f0 초기값이다. 누락 설정은 현재값을 유지한다.
    void PollEdgeScroll(const InputEvent& mouse); //Guide 통지 뒤·Overlay/일반 큐 앞에서 원문 가장자리 입력 본체를 수행한다.
    struct HudRuntime;std::unique_ptr<HudRuntime> hud_; // 실제 화면/지도 전역의 주소를 Client UI 수명에 고정한다.
    struct EditorRuntime;std::unique_ptr<EditorRuntime> editor_; //실제 파일/전송/State 장치는 World보다 먼저 만들고 Session 반납 뒤 해제한다.
    std::shared_ptr<const TextClipboard> textClipboard_; //최초 Chat과 뒤 Save as의 두 callback이 같은 OS 장치 수명을 공유한다.
    std::unique_ptr<RawButtonControls> rawButtons_; // 돌 버튼은 실제 공유 트리/수명/캡처를 사용한다. input_은 메뉴 행/판정 보고용이다.
    WarningMessages* warnings_{}; // rawButtons가 소유하는 같은 실제 Gump/StyleText의10초 안내를 빌린다.
    ScriptStringRing scriptStrings_; // 입력 큐에 예약한 문자열의 원본80칸 수명이며 페이지가 바뀌어도 보존한다.
    SoloMapFileSpec& mapFileSpec_; // 같은 Client의9BYTE FileSpec 키를 페이지/Session 교체 뒤에도 직접 빌린다.
    std::unique_ptr<SoloMapCatalog> mapCatalog_; // 현재 파일 캐시/dirty/이름을 Client 수명에 소유한다.
    std::shared_ptr<EditorCatalogWorkspace> editorCatalog_; //World/listener 반납 뒤에도 같은 작업 폴더와 durable 삭제 상태를 유지한다.
    std::shared_ptr<TellFileMenus> fileMenus_; // 기존 Tell과 실제 전체 HTML 메타가 같은 @/# Config를 공유한다.
    std::string mapUserName_; // 원본 초기 빈 사용자 이름이며 두 Fort 이름 버퍼와 다르다.
    std::uint32_t& fortSelectionMode_; // 같은 Client005cae84이며 Ask의2와 진행State1을 구별한다.
    HtmlTextParserGlobals& htmlGlobals_; //Client 장기565e70 배경과565ec0 P 억제를 Tell/Help/로그인 상태가 같은 주소로 사용한다.
    ScreenRect htmlMapView_{};std::string htmlCurrentFortName_; // 각 조회에서 현재 뷰와 반환 문자열의 수명을 보존한다.
    std::uint32_t buttonTextureType_{},buttonTextureFrame_{}; // 실제 버튼 배경 생성자에 전달할 fortGump/A01 타입·프레임 번호다.
    // scripts_: 스크립트.
    std::map<std::string, std::unique_ptr<o::Config>> scripts_;
    // decorations_: 장식.
    std::map<std::string, IndexedImage> decorations_;
    SquidFrameMetrics buttonTextureMetrics_{}; // 원본 A01의 signed short 폭/높이. 반복 주기는 각각 1을 뺀 값이다.
    ScreenPoint buttonTextureOffset_{}; // A01 VFX 픽셀 영역의 기준점 상대 위치.
    // title_: 화면에 표시할 제목 또는 제목 Gump의 토큰.
    // clouds_: 팔레트 인덱스 픽셀로 구성한 그림 (IndexedImage).
    IndexedImage title_, clouds_;
    // page_: 표시할 페이지 또는 스크롤 페이지 크기.
    DialogPage page_;
    // labels_: 라벨.
    std::vector<std::string> labels_;
    // actions_: 동작.
    std::vector<DialogAction> actions_;
    // mission_: 미션 이름에 맞는 스크립트를 읽고 섹션을 선택하는 처리기 (std::unique_ptr<MissionScript>).
    std::unique_ptr<MissionScript> mission_;
    std::unique_ptr<GameMenuBarState> gameMenuState_; // Session해제전에같은검산참조를반납한다.
    std::unique_ptr<CampaignObjectMenuClient> objectMenus_; // Client 수명이며 Session 해제 전 실제 팝업/Form을 먼저 닫는다.
    struct MissionRuntime; // 같은 미션의 Start/프레임 게이트 상태와 실제 연결을 고정 주소로 소유한다.
    std::unique_ptr<MissionRuntime> missionRuntime_; // 월드와 스크립트보다 먼저 해제한다.
    std::unique_ptr<o::RawMissionGuide> guide_; // 현재 미션 해제 중에도 반복 호출 스택이 살아 있도록 UberGump 수명에 둔다.
    std::uint32_t& hideBuildings_; // 원본0059a8ac의 같은 Client 장기 DWORD를 메뉴·Draw·정리가 공유한다.
    std::uint32_t& worldRedraw_; // 원본0059a8b0의 같은 Client 장기 DWORD이며 전체 팔레트/메뉴/resize가 직접 공유한다.
    std::uint32_t& menuConfigRevision_; // 이름 변경/설정/목록이 같은00542678 dirty를 직접 사용한다.
    std::uint32_t& commandGump_; // 실제00548740의 같은 Client Chat 현재 포인터이며 전체 공장이 이 슬롯을 갱신한다.
    std::unique_ptr<GameWorld> world_; // 호스트 월드를 Kernel 밖에서 소유하여 전역 Clear 중 중복 소멸/중첩 프레임을 막는다.
    // userInput_: 플레이 입력을 선택·카메라·명령 동작으로 전달하는 처리기 (std::unique_ptr<UserInput>).
    std::unique_ptr<UserInput> userInput_;
    // fortObjects_: Fort 객체.
    std::size_t fortObjects_{};
    // briefingSections_: 브리핑 섹션.
    std::vector<std::string> briefingSections_;
    // briefingIndex_: 브리핑 인덱스.
    std::size_t briefingIndex_{};
    // pageName_: 페이지 이름.
    // popup_: 팝업.
    std::string pageName_{"main"}, popup_;
    // popupRect_: 화면 좌표의 사각형. right·bottom은 포함하지 않는다(원본 004a1800의 인자) (ScreenRect).
    ScreenRect popupRect_{};
    // opened_: 열린.
    double opened_{};
    // quit_: 종료 여부.
    // rebuild_: 재구성 여부.
    bool quit_{}, rebuild_{true};
    // controlsChanged_: 새 페이지의 라벨·명령·영역과 raw 버튼 트리를 같은 합성에서 교체해야 하는지 나타낸다.
    bool controlsChanged_{true};
};
}
