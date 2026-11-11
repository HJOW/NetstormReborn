// 원본 Ubergump.cpp의 메인 메뉴·Tell·미션 진입을 현재 표시 기반과 연결한다.
#pragma once
#include "client/DialogExitCommand.h"
#include "client/SkirmishSetup.h" // 12차: 스커미시 설정 구조체(StartSkirmish 등의 인자)다.
#include "client/ClientPacketSources.h" //현재 실제 Native Source와 next7의 weak 인계값이다.
#include "client/GumpPointerBoundary.h"
#include "client/Gump.h"
#include "client/RawButtonControls.h"
#include "client/ScriptCommand.h"
#include "client/RawGameMenus.h" // 메인과 ESC가 같은 현재 Options 작성기를 사용한다.
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
struct SkirmishWorldPlan; // 12차: 스커미시 시작 월드 계획이다(정의는 SkirmishSetup.h).
class Client;
struct RawSurfaceGumpBindings; // 실제 현재 UI 등록표와 저장소 pin을 Storage에 약하게 빌린다.
class GameWorld;
class RawCampaignSession; // State1의 현재 Guide Subscriber 요청을 같은 Session에 연결한다.
class RawCampaignFactoryDisplay; //메타도같은Pool/현재가상표를사용하는공통SpriteDraw출력기를소유한다.
class UserInput;
class WarningMessages;
struct GameMenuBarState; // 현재Session의기존검산참조와메뉴serial만빌린다.
struct FortSelectionHooks; //편집 선택과 Test Battle 복귀가 같은 실제 파일/State/섬/Notify 장치를 사용한다.
class CampaignObjectMenuClient; // 같은 실제 원본 객체 메뉴와 현재 명령 큐를 연결한다.
struct MissionFortLoadRequest; // Init부터 실제 Session 설치까지 같은 요청과 Zone 이미지를 소유한다.
struct GeneralBattleStartRequest;class MetaConnectionRuntime; //일반 State7의 실제 요청과 인계할 전체 메타 전송 owner다.
class CampaignTemplateStorage; // 원본 명시 Init/State 정리까지 같은9개 manager의 수명을 소유한다.
struct CampaignGuideRuntime; // 같은 실제 Guide 상태와 소멸을 창 없는 World 검사에도 공유한다.
struct RawHtmlWindowContext; // 실제 전체 HTML에 현재 게임 소유자의 장치를 빌린다.
struct RawChatWindowContext; // 실제 Chat UI/시간 알림이 같은 제출 장치를 구성한다.
struct RawNsInfoContext; // 실제 NSInfo 입력도 같은 Client와 현재 메타 전송/Session 장치를 빌린다.
struct RawMovieContext; // 실제 MovieGump의 화면/두 정지/음향 장치를 빌린다.
class TellFileMenus; // 실제 파일 목록의 등록 Config를 부모/미션보다 오래 유지한다.
class TextClipboard; //같은 Client UI의 모든 문자 입력이 공유하는 실제 CF_TEXT 부모다.
class EditorFortFileDevices; // 최초 캠페인 편집 요청에서도 같은 지연 작업 파일 장치를 사용한다.
class EditorCatalogWorkspace; //목록/직접 Edit/이름 변경이 공유하는 장기 단일 namespace와 이관 장부다.
class EditorScenarioFiles;class EditorScenarioController; // 실제 Editor 문서/사용자 파일과 별도 시나리오 UI owner다.
struct LanLobbyBattle; // 대기실의 실제 명단/ACK 뒤 외부 State7으로 인계하는 요청이다.
// 메뉴·브리핑·미션 페이지와 해당 UI 동작을 관리하는 처리기이다.
class UberGump {
    friend class ClientRendererFrame; //같은 실제 Gump의 override/지도 clip만 현재 Frame에 빌린다.
public:
    // 초기화 뒤 원본 타이틀·구름·메뉴 자료를 읽고 기본 메뉴를 연다.
    explicit UberGump(Client& client);
    // 검사 장면과 미션 설정을 Renderer보다 먼저 해제한다.
    ~UberGump();
    // 원본 루프 시작의 State 자리에서 대기 명령을 실행하고 시간 제한을 갱신한다.
    void Tick();
    void RunNativePacketState(); //장기 수신 F가 선택한 현재 State를 기존 실제 동기 Run 부모로 전달한다.
    void RecreateNativePacketHud(); //67h의 실제 현재 HUD 재생성만 같은 UI에 요청한다.
    void RefreshNativePacketInfo(); //47cab0의 실제 현재 NSInfo/메뉴 Push·Attach를 같은 UI에 요청한다.
    void ReceiveNativePacketChat(std::span<std::uint8_t> packet); //장기41h가 선택한 현재 UI에서 기존42f0e0/42ed30 수신 부모를 그대로 실행한다.
    void ShowNativePacketTell(std::string_view section); //같은 현재 Config/미션 HTML Tell을 사용하고 상태나 성공값을 만들지 않는다.
    NativeBattleSource CurrentNativeBattleSource() const; //현재 실제 Composite/Meta/Editor device만 읽으며 phase 추정/새Source 생성을 하지 않는다.
    void DispatchNativeBattleStart(NativeBattleSource source,bool direct); //1a는 기존next7에weak로 기억하고1b는같은current7을즉시실행한다.
    void PollMissionPackets(); // 정상 루프의 메시지 처리 전에 실제 로컬 큐를 폴링하고 같은 미션의 접속 완료만 UI에 공개한다. 외부 TCP/IP 폴링을 대신하지 않는다.
    // 원본 루프8번의 Guide 자리다. 현재0..6 단계와 실제 공통 프레임을 입력보다 먼저 실행한다.
    void MissionFrame();
    void PrepareKernelFrame(); // 같은 실제 GameWorld가 전역 Kernel 앞에서 프레임 입력만 준비한다.
    void FinishKernelFrame(); // 전역 Kernel 뒤 같은 월드의 표시 자료만 수집한다.
    // 커널이 월드를 진행한 직후 변경된 실제 장면을 제출한다.
    void Frame();
    void ApplyCameraFrame(); //Present 말미의 실제 목표 제한/적용을 같은 Client·현재 월드·Gump/Screen 장치에서 수행한다.
    void CommitCameraNow(); //497960의 실제 장치/dirty/Gump 적용만 수행한다. Home/초기시야는flip/Present 제한 없이 같은 본체를 호출한다.
    void CenterViewSpot(std::string_view text); //대본 ViewSpot의433ad0: 인자를 읽어433aa0 즉시 이동을 실행한다. 월드가 없어도 같은 Client 카메라에 쓴다(단위676).
    void CenterViewPoint(float x,float y); //대본과 Tab의 같은433aa0 즉시 중심 이동을 두 단정도 인자로 실행한다.
    void RefreshMinimap(); // 원본00481320의커널앞위치에서실제같은HUD큐를소비한다.
    // 호스트 장면 합성을 예약한다. 원문0043dad0의 예산 재부착은 RefreshStormPower로 구별한다.
    void Refresh();
    void RefreshProduction(); //장기영역/신전도현재실제버튼트리에원문02000003사건을보낸다.
    std::uint32_t NativeMissionDemo() const; //00463b50의 현재 미션 포인터/+68만 읽으며 미션이 없으면0이다.
    void RefreshStormPower(); //0043dad0의 실제 현재 예산 serial Push/Attach/깊이 복원이다.
    void ForwardDiagnosticChat(std::uint32_t enabled,std::string_view format); //434f40의 실제 로그를 같은 현재 Chat UI/팔레트에 전달한다.
    void ShowCaptureNotice(std::string_view body,std::uint32_t width,std::uint32_t height); //4cf550 직접 본문을 기존 공통 HTML 공장/모달 수명에 전달한다.
    void ShowLoginProgress(o::LocalMissionProgress kind,std::int32_t remaining); // 원문 State/로그인의 각 번역 안내를 같은 현재 loading serial로 표시하며 상태는 쓰지 않는다.
    std::uint32_t ShowLoginWaiting(std::string_view body); // 이미 번역/서식화한 일반 대기를 cf570/현재 HTML에 표시하고 실제 반환 serial을 전달한다.
    void RequestFortEditState(); //004b3560의 실제 현재 World/Fort/State 보존과 next1 요청을 실행한다.
    std::uint32_t BattleGate() const; //Fort 선택·편집 요청이 읽는 00594fbc의 현재 값이다. Native는 Session 없이도 장기 Authority를 읽는다(M6-n, 1272).
    void SynchronizeRegisteredSubscriber(std::uint32_t id); //4b3410의 NULL 이름 변경을 같은 현재 Template/실제 Session에 전달한다.
    void SelectReconnectFortForState(); // 실제 복구State7의 pending 이름을00460980/기존 Template snapshot·Notify·제목·dirty 부모로 선택한다.
    void SelectBuildReplayObject(std::uint32_t sid); // 현재 World의 실제Select(SID,0,1)이며 저장소에SID만 쓰지 않는다.
    void ToggleChatInput(); //그리기 뒤 프레임말의0042f210을 같은 현재 실제 Console에 전달한다.
    void KillGumpsForState(); //004666d0의 같은 공통 등록표 필터로 Kill만 예약한다.
    void CleanupGumpsForState(); //004662b0 실제 공통 정리와 호스트 페이지 캐시 반납이며 State 확정은 하지 않는다.
    void ApplyHudModeForState(std::uint32_t mode); //00466280의 현재 명령/원문 모드를 자식 우선·숨김 포함으로 분배하며 State/loaded는 쓰지 않는다.
    void ReplaceHudForState(); //004c77c0의 전체 네 자식 공장을 World 생성 전에 구성하며 mode/Player/loaded를 쓰지 않는다.
    void CleanupNativeStateWorld(std::uint32_t reason,std::shared_ptr<MetaConnectionRuntime> handoff={}); //004b2df0 전체 부모다. 명시한 같은 메타 owner만 수신 중 인계할 수 있고 다른 World/메타 콜백의 정리 금지는 유지한다.
    // 새 화면 팔레트를 월드 어댑터에도 전달하고 UI/표시를 다시 그린다.
    void PaletteChanged();
    // 현재 월드가 번개 효과의 부모로 살아 있는지 반환한다. 브리핑 정지 중에도 월드는 존재한다.
    bool HasWorld() const;
    ScreenRect CurrentMapView() const; // 위치 음향도 World 유무와 무관하게 같은 실제 GumpTree 지도LTRB를 호출 시점에 읽는다.
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
    // 검사 전용: 현재 Session/World 상태를 판번호 TSV로 읽는다(client/SessionStateDump). 미션 월드가 없으면 world 0 덤프다. 게임 상태를 바꾸지 않는다.
    // 2026-11-09: webpj/dotnetpj L2·L3 대조용 ui-script session-dump 단계에서 쓰려고 추가했다. Session 선택은 InputSession()과 같다.
    std::string SessionStateDump() const;
    // 버튼/목록의 실제 위치를 라벨로 찾는다. 없으면 빈 값.
    std::optional<ScreenPoint> ControlPoint(std::string_view label) const;
    bool QuitRequested() const { return quit_; } // 실제 QuitApp/사용자 종료 요청만 읽으며 검사 프레임 제한 종료와 구별한다.
    void CleanupGumps(); // 원본4662b0 위치에서 실제 전체 HTML/Warning을 포함한 같은 공통 큐를 정리한다.
    void RemoveNativePacketMessage(std::int32_t kind,std::int32_t player,std::int32_t parameter); //같은 실제 WarningMessages.Remove와 UI 다시 그리기를 Native 전역/기존 Session이 공유한다.
    void ShowGameTimeMessage(); //00439b5f의kind16/player1/표시1을기존메시지20칸에전달한다.
    void PollConnectionHealth(); //00439ad8의 실제 활동/지연/자동 서버 이전과 뒤439ae3 로그인 타이머를 실행한다.
    void PollBattleCompletion(); //439b3e의44ac30을주기저장뒤·시간안내앞한번실행한다.
    void PollBattleResources(); //00439b34의 실제 자원 프레임이며 월드 없는 메뉴도 같은 전역의 자연 관문을 실행한다.
    void ShowSidRecoveryNotice(); //CD SID 회수 뒤 BridgesKilled를 실제 공통 Tell 부모로 연다.
    void IdleGumps(); // 원본464170 위치에서 Warning을 포함한 현재 공통 idle 표를 한 번 실행한다.
    // --ui-script 전용 검사 경계다. 실제 월드의 가상 삭제를 호출하며 일반 게임 입력에는 노출하지 않는다.
    bool InspectDestroySurface(std::string_view type,float x,float y);
    // --ui-script의 커널 검사에서 일반 게임과 같은 Tell 경로를 실행한다. 일반 사용자 명령으로 노출하지 않는다.
    void InspectTell(std::string target);
    void QueueHtmlScriptCommand(std::string_view text); // 현재 HTML과 검사 입력이 같은 원문 이름표/문자열 고리/실제 입력 큐로 명령을 예약한다. 상태나 결과를 직접 바꾸지 않는다.
    void InspectBridgePatterns(std::string_view patterns); // 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. UI 검사에서 지정 순서를 교체/해제한다.
    std::string_view CurrentHandle() const; // 기존 World/메뉴 이름과 같은 현재00540ca8을 읽는다.
    void SubmitMonitorChat(std::string_view format,std::string_view line); // 현재 Chat manager의 임시서식/일반 제출/정상복원을 사용한다.
    std::uint32_t CurrentDialog() const; //565dd4의 현재 실제 모달 토큰이며 serial/인스턴스 수와 구별한다.
    std::uint32_t CurrentDialogSerial() const; // 테스트 전용 읽기다. 현재 모달 Gump의 실제 serial이며 없으면 0이다. RecheckCD 검사가 005c85ec에 넣을 값을 읽을 때만 쓴다(단위696).
private:
    RawCampaignSession* InputSession() const; //현재 실제 GameWorld/준비된 MetaMap의 Session을 빌리며 미설치일 때만 nullptr다.
    // 원본 Tell의 파일.섹션/현재 tell 섹션을 읽어 대화상자를 연다.
    void Tell(std::string target, bool briefing = false);
    // 원본00482de0처럼 현재 미션/일반 섹션이 없으면 false로 반환하며 기존 페이지를 유지한다.
    bool TryTell(std::string target,bool briefing=false,bool* windowCreated=nullptr,std::uint32_t suppress=0); // 원문 mode1은 첫 HTML 처리만 실행하고 창을 만들지 않는다.
    // 원본 이름의 설정 객체를 보존해 캠페인 제목과 변수를 치환한다.
    o::Config& Script(std::string_view name);
    RawHtmlWindowContext LoadingHtmlContext(ScreenRect mapView); // 같은 Client/Config/큐와 매번 현재 뷰를 공급한다.
    RawMovieContext MovieDevices(); // 영상 객체가 같은 실제 Client의 장기 Screen/시계/음향을 조회하도록 연결한다.
    // 표시 상태를 합성한다. 페이지 교체 예약이 있을 때만 입력 영역과 실제 돌 버튼을 함께 교체한다.
    void Compose();
    // 새 페이지/팝업/해상도의 입력 영역 교체를 다음 합성에 예약한다. 단순 픽셀 갱신에는 쓰지 않는다.
    void RebuildControls();
    // 대기 중인 명령을 실행한다. 지원하지 않는 명령은 실제 동작으로 가장하지 않는다.
    void Execute(const DialogAction& action);
    bool ScrollMapList(const InputEvent& event); // 5차: 맵 목록의 이전/다음 이동 행을 처리한다. 표시 창 첫 번호를 바꾸고 같은 대화창을 캐시 재사용으로 다시 연다.
    bool FortSelectionEvent(const InputEvent& event); // 실제 AskEditFort/ReadFort를 같은 pending 이름·목록·숫자 nextState에 분배한다.
    void SnapshotNativeCurrentMap(bool onlyIfPresent); // 현재 장기 Template과 파일 장치로 조건부 또는 독립 두 저장을 수행한다.
    void SnapshotNativeFortSlot(std::uint32_t slot); // 지정 슬롯의 실제 Fort 전체 저장을 수행한다.
    void NotifyNativeFortSlot(std::uint32_t slot); // 지정 슬롯의 실제 State 변경 통지만 수행한다.
    void SelectCurrentFortName(std::string_view name); // 원문 MissionFortRequest.selectFort 위치에서 전체 현재 이름 부모를 실제 Client/Template에 연결한다.
    EditorFortFileDevices& EnsureEditorFortFiles(); // World/State 설치 전에 실제 현재 파일 경계가 필요할 때만 장기 작업본 장치를 준비한다.
    FortSelectionHooks FortEditEffects(); //실제 선택된 Battle14의 파일/State 보존 경계도 같은 현재 World와 작업 파일에 연결한다.
    RawChatWindowContext MakeChatContext(); // 창을 만들지 않고 현재 이름/Identity/제출/패킷 장치를 조립해 알림과 UI가 공유한다.
    void BindMetaNsInfoInput(RawNsInfoContext& context); // 같은 현재 Session 카메라·실제 음향/메타 송신 장치를 두 NSInfo 생성자에 연결한다.
    void EnsureHudFactories(); //현재 Client 장기 전역·물리로 네 공장만 준비하며 새 월드/Session을 생성하지 않는다.
    std::uint32_t ReadChatGlobal(std::uint32_t address); // 현재 Client/실제 월드·팔레트에서 채팅이 읽는 전역만 조회한다.
    void ReceiveChatMessage(std::span<std::uint8_t> packet); // 같은 실제 Chat 로그/설정·음향·표시 사건에 수신 부모를 연결한다.
    void RetireWorldForState(); //실제 State 자식 정리 뒤 같은 Runtime의 호스트 참조만 반납한다. 게임 전역·기한·정지를 다시 쓰지 않는다.
    void InstallHud(); // 현재 실제 월드와 같은 공통 전체 Uber/자식 공장·도킹을 연결한다.
    void PollEditorState(); //메인/Edit/인트로39~41과 후속4를 같은 Native State 분배기로 소비한다.
    o::LocalMissionParentResult InstallIntroNativeState(std::uint32_t selected); // 실제 Client 준비/영상 경계와 Native39~41 부모를 연결한다.
    void OpenSaveFortNameDialog(std::uint32_t fortMode); //현재 Editor의 실제 파일/헤더/이름 부모를 원문 fortMode 인자의 TextEntry 모달에 연결한다.
    bool ScenarioEvent(const InputEvent& event); // Editor 전용 새 명령/ESC만 소비하고 원문 입력은 그대로 넘긴다.
    void OpenScenarioPage(std::uint32_t page); // 현재 문서의 실효값/원문 필드 페이지를 실제 Gump에 작성한다.
    void OpenScenarioField(std::uint32_t field); // 같은 TextEntry/IME/MCP 일반 입력 모달로 한 필드를 편집한다.
    void CloseScenarioUi(bool discard); // 패널/편집 모달의 실제 토큰만 Kill하고 취소 시 보류 문서를 반납한다.
    void PrepareScenarioTest(); // Fort snapshot 정상 복귀 뒤 같은 사용자 sidecar를 저장/보장한다.
    void OpenSendMoneyDialog(std::uint32_t target); // 원본 숫자 창의 대상·현재 입력·확정이 같은 월드를 빌린다.
    o::LocalMissionParentResult InstallExistingEditorState(std::uint32_t selected); //State1의 정리/prefix/현재 파일/UI tail을 실행하며 공통 확정은 분배기에 맡긴다.
    o::LocalMissionParentResult InstallPreparedFortState(bool prefixHud,std::function<void()> battleOver={}); // 같은 Native 앞단 뒤 파일/월드/꼬리를 연결한다. false는 State3의 HUD 중복을 피하고 현재6은 실제 후반 옵션/결과 장치를 요구한다.
    o::LocalMissionParentResult InstallBattleReturnNativeState(std::uint32_t selected); // 실제 State6의 보관 결과/출처와 일반 Fort 재설치를 기존 큰 부모에 연결한다.
    void OpenMultiplayerOptions(); // 실제 현재 Fort/이름/로컬 서버 설정과 원문 일반 옵션 창을 조립한다.
    void OpenMultiplayerInput(bool server); // 이름/로컬 서버 주소를 기존 HTML Input 공장으로 편집하고 실제 저장소에 반영한다.
    bool MultiplayerOptionsEvent(const InputEvent& event); // 원문 SetName b9/Connect c4 예약을 실제 일반 입력 위치에서 처리한다.
    void PrepareNativeFortState(bool editor); //State1/3의 같은 실제 cleanup/Chat/listener/Fort/Core/Renderer/Chunk/options 준비를 실행한다. HUD는 각 원문 위치의 별도 호출이다.
    o::LocalMissionParentResult InstallMainMenuNativeState(std::uint32_t selected); //State3의 실제 전체 부모를 실행하며 호스트 메뉴 표시는 Native 공통 확정 뒤에만 공개한다.
    o::LocalMissionParentResult InstallMetaNativeState(std::uint32_t selected); //State19의 실제 정리/Fort/Core/CS/뉴스/채널 부모를 실행한다. 즉시 다음 상태는 공통 분배기에 남긴다.
    void PrepareMetaWorldRuntime(); //State19가 읽은 Fort/Core와 같은 Pool·Player·Kernel을 빌린 메타 Session/GumpForm을 HUD 앞에서 구성한다. 게임 Load/로그인·loaded 공개는 실행하지 않는다.
    void OpenMetaBattleDescription(std::uint32_t text); //표식된 현재 CS 주소·실제 HTML 고리 문자열·NULL 기본 문구를 구별하고 확정은 현재 메타 전송에 맡긴다.
    std::uint32_t ShowMetaTell(std::string_view section); //메타의 같은 Config/현재 미션 HTML을 열고 실제 반환 serial을 전달하며 호스트 phase를 바꾸지 않는다.
    RawCampaignFactoryDisplay* MetaSpriteDisplay() const; //Renderer가현재메타의실제출력기만빌린다.게임출력기/가짜장면을대신반환하지않는다.
    std::shared_ptr<MetaConnectionRuntime> EnsureMetaConnectionSource(); //같은 실제 Meta/TCP/UDP만 확보/공개하며 World 정리나 State 실행/접속을 하지 않는다.
    o::LocalMissionParentResult InstallMetaConnectionState(std::uint32_t selected); //실제 메타20~27/검색34~38과TCP10/11·UDP12/현재HTML·Run을같은장기owner에접합한다.37의새서버실행은후속이다.
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
    void LoadMissionWorldBody(std::optional<std::string> requestedFort); // 부모 공통부의 최초 복사/로그 뒤 실제 정리/새 World/로그인 후반만 수행한다.
    void FinishMissionWorldLoad(); // 실제 phase14/현재 미션 수명 확인 뒤 커서/Start/안내를 설치한다. 상태8 준비 단계에서 호출하지 않는다.
    void RequestCurrentGuideFort(RawCampaignSession& session); //State1의 파일 설치 뒤 Champion/현재 handle로 State7을 요청하고 동일 Zone 이미지를 보유한다.
    void RequestMissionFort(std::string_view name); // Tutorial/일반 미션이 같은 현재 Fort 선택과86BYTE Zone/144BYTE State7 요청을 작성한다. 호스트 전환 큐는 호출자가 예약한다.
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
    GameMenusHooks CurrentGameMenuHooks(); // 월드 유무와 관계없이 같은 Client의 현재 설정·화면·모드로 원문 메뉴를 작성한다.
    void ConnectPostProcessMenu(GameMenusHooks& hooks); // 7차: 후처리(안티앨리어싱/FSR 1.0) 장치가 있으면 FSR 해상도와 두 토글을 옵션 메뉴 훅에 연결한다.
    void OpenMainMenuPopup(bool options); // 메인 버튼의 실제 기하와 원본 Help/Options 작성기를 공통 팝업 수명에 연결한다.
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
    void ReleaseCurrentGuideMission(); //선택된 Guide의 허용표·제한·번호·AI·Config만 반납한다. World·입력·시계는 해당 부모가 보유한다.
    // client_: 원본 전역 변수로 흩어져 있던 클라이언트 상태를 객체 하나로 묶는다. 한 프로세스에 하나만 만든다 (Client).
    Client& client_;
    // state_: 처리 중 조회·갱신하는 상태 (State).
    State state_;
    std::shared_ptr<o::FortSerializationGlobals> serializationGlobals_; // 주Template·맵목록임시manager·실제Session이같은판본DWORD를빌리는Client수명이다.
    std::shared_ptr<CampaignTemplateStorage> templateStorage_; // 이전 Session이 사라져도 같은 Template9를 명시 정리 전까지 보유한다.
    std::shared_ptr<MissionFortLoadRequest> missionFortRequest_; //현재 Guide의 지연 요청이다. Guide/전체 종료에서 반납하며 이전 World의 State 반납은 보존한다.
    std::shared_ptr<GeneralBattleStartRequest> nativeBattleStart_; //대기실 ACK와 별개인 실제 메타 State7의 현재 Zone/설치 요청이다.
    std::optional<WeakNativeBattleSource> pendingNativeBattleSource_; //1ah의같은다음7만실제전체Source에연결한다. 전송 owner/새 게임 요청을 소유하거나 만들지 않는다.
    std::shared_ptr<std::array<std::uint8_t,87>> missionOptions_; // 월드보다 오래 사는 실제 Init/로그인 옵션이다.
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
    std::int32_t scrollWhileMouseOut_{1}; //005318d0의 PE 초기1이며 설정 누락은 현재값을 유지한다.
    bool PollCameraPan(const InputEvent& pointer); //현재 커서 갱신 뒤 원문 Alt/가운데 이동과 전용 큐 경로를 선택한다.
    struct HudRuntime;std::unique_ptr<HudRuntime> hud_; // 실제 화면/지도 전역의 주소를 Client UI 수명에 고정한다.
    struct EditorRuntime;std::shared_ptr<EditorRuntime> editor_; //작업 파일/양끝/State와실제Meta Session/표시/입력을함께소유하며배송중전체owner를보유할수있다.
    struct LanRuntime;std::unique_ptr<LanRuntime> lan_; // 대기실 제어 수명과 정리 전 일반 시작 요청을 소유한다.
    void OpenLanChoice(); // 원문 HTML 부품으로 로컬 host/join/주소 선택을 제공한다.
    void ResumeLanChoiceAfterInput(); // 주소 입력이 대기실 선택에서 열렸을 때만 같은 선택 창으로 복귀한다.
    void StartLanLobby(bool host,bool startOnly=false); // 현재 실제 Fort ID/이름으로 열거나 이미 열린 host에만 명시 Start를 요청한다.
    void ShowLanLobby(); // 실제 가입자/시작 대기/실패를 같은 원문 HTML 창에 표시한다.
    void PollLanLobby(); // TCP callback 밖에서 완료 요청을 다음 호스트 Tick에 예약한다.
    void CloseLanLobby(); // 자기 대기실/창만 닫고 이미 인계한 State7 요청은 보존한다.
    void RequestLanBattle(LanLobbyBattle battle); // 실제 명단을 Native next7과 같은 요청 owner에 인계한다.
    void RequestBattleReconnect(bool reloadFile); //004b8cb0의 실제 reader/서버 선택과 동기15·다음7/3을 일반 입력에서 수행한다.
    void QueueReconnectBattleWorld(); // 원문 next7/문자열40BYTE/복구 표식을 보존해 다음 Tick의 같은 일반 World 설치에 인계한다.
    void LoadLanBattleWorld(); // Client 수명의 State7 공통 부모가 이전/새 World를 끝까지 소유한다.
    void LoadLanBattleWorldBody(); // 실제 cleanup/전투 prefix와 일반 Session 후반을 연결한다.
    void LoadGeneralBattleWorldBody(const std::shared_ptr<GeneralBattleStartRequest>& request,NativeBattleSource source,std::optional<bool> lanHost); //기존 LAN과 메타가 같은 일반 World 본체를 사용하되 메타는 캡처 Zone 유무로 역할을 고른다.
    void LoadNativeBattleWorldBody(NativeBattleSource source); //현재7의 실제Source를 받아 같은 Zone/내부·TCP·UDP로 일반 World를 설치한다.
    std::uint32_t pendingNativeBattleZone_{}; //실제 next+80 소유 token으로 예약 출처를 대조하며 phase/성공flag를 만들지 않는다.
    bool NativeBattleStartPending() const; //현재next7과실제Zone포인터token의동일성을확인하여오래된weak예약을다른요청에사용하지않는다.
    void EnsureNativeStateDispatcher(); //같은 장기 State 분배기만 준비하며NativeState/접속/성공값은쓰지않는다.
    void RunMetaRequestedState(); //현재7은 같은 분배기의 RunStarting으로 동기 처리하고 다른 요청은 기존 Run으로 전달한다.
    void RunNativeBattleState(NativeBattleSource source); //1bh는실제Source만복귀까지보유하고이전World는원래정리에서반납하여같은현재7을동기실행한다.
    void FinishLanBattleWorld(); // 실제 phase14/Player 참여 뒤에만 사용자 입력·커서·표시를 연다.
    std::uint32_t multiplayerOptionsToken_{}; // 현재 일반 옵션의 실제 Gump token이며 닫힌 창의 serial을 새 창으로 대신하지 않는다.
    std::shared_ptr<const TextClipboard> textClipboard_; //최초 Chat과 뒤 Save as의 두 callback이 같은 OS 장치 수명을 공유한다.
    std::shared_ptr<RawSurfaceGumpBindings> gumpUiRuntime_; // 실제 UI 등록표의 대여는 그 저장소 정리가 끝날 때까지 유지한다.
    std::shared_ptr<RawButtonControls> rawButtons_; // 실제 트리/등록표/지연 삭제의 저장소이며 선택된 Gump 통지 동안만 추가 pin한다.
    WarningMessages* warnings_{}; // rawButtons가 소유하는 같은 실제 Gump/StyleText의10초 안내를 빌린다.
    ScriptStringRing scriptStrings_; // 입력 큐에 예약한 문자열의 원본80칸 수명이며 페이지가 바뀌어도 보존한다.
    SoloMapFileSpec& mapFileSpec_; // 같은 Client의9BYTE FileSpec 키를 페이지/Session 교체 뒤에도 직접 빌린다.
    std::unique_ptr<SoloMapCatalog> mapCatalog_; // 현재 파일 캐시/dirty/이름을 Client 수명에 소유한다.
    std::shared_ptr<EditorCatalogWorkspace> editorCatalog_; //World/listener 반납 뒤에도 같은 작업 폴더와 durable 삭제 상태를 유지한다.
    std::shared_ptr<EditorScenarioFiles> scenarioFiles_; // Script/World 반납 뒤 TestBattle reader도 같은 작업 namespace를 유지한다.
    std::unique_ptr<EditorScenarioController> scenario_; // 현재 맵의 보류 시나리오만 소유하며 월드 전환 때 반납한다.
    std::uint32_t scenarioPage_{},scenarioToken_{}; // 실제 현재 페이지/모달 토큰이며 명령 ID나 성공 상태를 합성하지 않는다.
    std::shared_ptr<TellFileMenus> fileMenus_; // 기존 Tell과 실제 전체 HTML 메타가 같은 @/# Config를 공유한다.
    std::string mapUserName_; // 원본 초기 빈 사용자 이름이며 두 Fort 이름 버퍼와 다르다.
    std::uint32_t& fortSelectionMode_; // 같은 Client005cae84이며 Ask의2와 진행State1을 구별한다.
    HtmlTextParserGlobals& htmlGlobals_; //Client 장기565e70 배경과565ec0 P 억제를 Tell/Help/로그인 상태가 같은 주소로 사용한다.
    bool StartSkirmish(const SkirmishSettings& settings); // 12차 스커미시: 설정으로 대본/fort를 만들어 미션을 시작한다.
    SkirmishSettings CurrentSkirmishSettings(); // 12차: 설정 객체에서 스커미시 설정을 읽는다.
    void StoreSkirmishSettings(const SkirmishSettings& settings); // 12차: 스커미시 설정을 설정 객체에 쓴다.
    std::string SkirmishOrProductBody(std::string_view target,std::string_view body); // 12차: 스커미시 화면 본문 또는 제품 전용 Tell 본문이다.
    std::shared_ptr<SkirmishWorldPlan> skirmishWorld_; // 12차: 이번 스커미시의 월드 계획이다. 세션 서비스가 미션 시작 때 읽고 일반 미션에서는 null이다.
    o::ConfigInterface::FileReader skirmishSource_; // 12차: 생성한 스커미시 대본/fort를 읽는 가상 파일 reader다(시작할 때 채운다).
    std::string lastTellTarget_; // 5차: 마지막으로 실제 창을 만든 Tell 섹션 이름이다. 맵 목록 스크롤이 같은 대화창을 다시 열 때 쓴다.
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
    std::unique_ptr<CampaignGuideRuntime> missionRuntime_; // 같은 미션의 Start·프레임 상태·연결을 고정 주소로 소유하고 Script보다 먼저 해제한다.
    std::uint32_t worldMissionToken_{}; // 접속 중 미션 교체가 이전 World의 완료를 새 Script에 공개하지 않도록 현재 설치 수명을 식별한다.
    std::unique_ptr<o::RawMissionGuide> guide_; // 현재 미션 해제 중에도 반복 호출 스택이 살아 있도록 UberGump 수명에 둔다.
    std::uint32_t& hideBuildings_; // 원본0059a8ac의 같은 Client 장기 DWORD를 메뉴·Draw·정리가 공유한다.
    std::uint32_t& worldRedraw_; // 원본0059a8b0의 같은 Client 장기 DWORD이며 전체 팔레트/메뉴/resize가 직접 공유한다.
    std::uint32_t& menuConfigRevision_; // 이름 변경/설정/목록이 같은00542678 dirty를 직접 사용한다.
    std::uint32_t& commandGump_; // 실제00548740의 같은 Client Chat 현재 포인터이며 전체 공장이 이 슬롯을 갱신한다.
    std::shared_ptr<GameWorld> world_; // Kernel 밖의 실제 월드 전체를 소유하며 실행 중 패킷 콜백도 같은 표시/Session 수명을 유지한다.
    // userInput_: 플레이 입력을 선택·카메라·명령 동작으로 전달하는 처리기 (std::unique_ptr<UserInput>).
    std::unique_ptr<UserInput> userInput_;
    // 실제 미션 설치 완료 시점의 월드 객체 수다. 선행 Fort 파싱을 만들지 않고 진단 mission 행에 사용한다.
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
