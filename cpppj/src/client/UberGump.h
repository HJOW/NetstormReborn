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
class DialogBodyLayout;
struct GameMenuBarState; // 현재Session의기존검산참조와메뉴serial만빌린다.
class CampaignObjectMenuClient; // 같은 실제 원본 객체 메뉴와 현재 명령 큐를 연결한다.
struct MissionFortLoadRequest; // Init부터 실제 Session 설치까지 같은 요청과 Zone 이미지를 소유한다.
class CampaignTemplateStorage; // 원본 명시 Init/State 정리까지 같은9개 manager의 수명을 소유한다.
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
    void RefreshMinimap(); // 원본00481320의커널앞위치에서실제같은HUD큐를소비한다.
    // 날씨 곡 전환의 화면 갱신 요청이다(0043dad0 경계). 같은 프레임의 Frame에서 UI를 다시 합성한다.
    void Refresh();
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
    std::string Report() const;
    // 버튼/목록의 실제 위치를 라벨로 찾는다. 없으면 빈 값.
    std::optional<ScreenPoint> ControlPoint(std::string_view label) const;
    bool QuitRequested() const { return quit_; } // 실제 QuitApp/사용자 종료 요청만 읽으며 검사 프레임 제한 종료와 구별한다.
    void CleanupDialogs(); // 원본처럼 설정 저장/미니맵 갱신 뒤 Kernel 전에 반납한 부모의 OnExit를 예약한다.
    void CleanupGumps(); // 원본4662b0 위치에서 raw 공통 큐·별도 Warning·논리 HTML을 정리하는 현재 어댑터다.
    void IdleGumps(); // 원본464170 위치에서 현재 raw/Warning idle을 각각 한 번 실행한다. 최종 단일 표 통합은 후속이다.
    // --ui-script 전용 검사 경계다. 실제 월드의 가상 삭제를 호출하며 일반 게임 입력에는 노출하지 않는다.
    bool InspectDestroySurface(std::string_view type,float x,float y);
    // --ui-script의 커널 검사에서 일반 게임과 같은 Tell 경로를 실행한다. 일반 사용자 명령으로 노출하지 않는다.
    void InspectTell(std::string target);
    void InspectBridgePatterns(std::string_view patterns); // 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. UI 검사에서 지정 순서를 교체/해제한다.
private:
    // 원본 Tell의 파일.섹션/현재 tell 섹션을 읽어 대화상자를 연다.
    void Tell(std::string target, bool briefing = false);
    // 원본00482de0처럼 현재 미션/일반 섹션이 없으면 false로 반환하며 기존 페이지를 유지한다.
    bool TryTell(std::string target,bool briefing=false,bool* windowCreated=nullptr);
    // 원본 이름의 설정 객체를 보존해 캠페인 제목과 변수를 치환한다.
    o::Config& Script(std::string_view name);
    // @guideSpec/%userSpec 파일 목록을 현재 파일 설정으로 확장한다.
    void ExpandMenus(DialogPage& page);
    DialogMetadataHooks MapMetadata(); // 실제 메타 추출 시점의 메뉴 열과 현재 파일 목록을 페이지 표시 자료에 연결한다.
    // 표시 상태를 합성한다. 페이지 교체 예약이 있을 때만 입력 영역과 실제 돌 버튼을 함께 교체한다.
    void Compose();
    // 새 페이지/팝업/해상도의 입력 영역 교체를 다음 합성에 예약한다. 단순 픽셀 갱신에는 쓰지 않는다.
    void RebuildControls();
    // 대기 중인 명령을 실행한다. 지원하지 않는 명령은 실제 동작으로 가장하지 않는다.
    void Execute(const DialogAction& action);
    // 기존 미션을 원본 Begin 순서로 종료/해제하고 새 빈 미션을 준비한다.
    void BeginMission(std::string name,std::uint32_t demo=0,std::uint32_t mode=0);
    // 이전 현재가 없는 Guide 공장 위치에서만 Script/빈 Runtime을 만든다.
    void CreateMission(std::string name,std::uint32_t demo=0,std::uint32_t mode=0);
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
    void OpenGameMenu(); //004cc210의같은controlled가로막대와실제하위메뉴를현재Session에작성한다.
    void ToggleGameMenu(); // ESC의현재serial조회→열기또는Kill/serial0/팝업닫기를실행한다.
    bool GameMenuOpen() const; // 현재serial을공통등록표에서조회하며과거열림bool을저장하지않는다.
    bool GameMenuCommand(const InputEvent& event); // 메뉴부모가예약한현재원본명령을각실제소유자로분배한다.
    void ClearPage(); // 실제 페이지 교체/반납에서 부모의 OnExit를 지연 소멸 표로 옮기고 표시 소유 내용을 비운다.
    void ActivatePageAction(DialogAction action); // 실제 HTML 버튼/메뉴/timeout은 명령을 즉시 예약한 뒤 부모를 Kill한다. 호스트 임시 UI는 기존 전환을 사용한다.
    void DisposeRetiredPages(); // 설정 저장 뒤 갱신에서 지연 반납한 부모들의 OnExit를 같은 큐에 예약한다.

    // 원본 우클릭 메뉴의 연결점. 건설/수확 명령은 다음 단계에서 활성화한다.
    void OpenObjectMenu(o::SquidId id,const InputEvent& event);
    // 메뉴가 소유한 월드와 입력 참조를 안전한 순서로 제거한다.
    void ClearWorld();
    // client_: 원본 전역 변수로 흩어져 있던 클라이언트 상태를 객체 하나로 묶는다. 한 프로세스에 하나만 만든다 (Client).
    Client& client_;
    // state_: 처리 중 조회·갱신하는 상태 (State).
    State state_;
    std::shared_ptr<o::FortSerializationGlobals> serializationGlobals_; // 주Template·맵목록임시manager·실제Session이같은판본DWORD를빌리는Client수명이다.
    std::shared_ptr<CampaignTemplateStorage> templateStorage_; // 이전 Session이 사라져도 같은 Template9를 명시 정리 전까지 보유한다.
    std::shared_ptr<MissionFortLoadRequest> missionFortRequest_; // 현재 미션의 지연 요청이다. 미션 교체 시 이전 큐와 함께 반납한다.
    std::shared_ptr<std::array<std::uint8_t,87>> missionOptions_{std::make_shared<std::array<std::uint8_t,87>>()}; // 월드보다 오래 사는 실제 Init/로그인 옵션이다.
    std::shared_ptr<std::array<std::uint32_t,256>> missionRestrictions_{std::make_shared<std::array<std::uint32_t,256>>()}; // 설치 전/후가 공유하는 기술 허용 표다.
    std::uint32_t missionNumber_{},tutorialSeen_{}; // 실제5ca8e0/f0의 현재 전역이다.
    std::uint32_t& initLoginSubscriber_; //호환 이름이며 실제540c74의 같은 Client DWORD다.
    const std::uint32_t& initialSubscriber_; //실제54265c의 현재 const참조이며 생성 때1을 다시 쓰지 않는다.
    std::uint32_t nextMissionSaveToken_{}; // 64비트 포인터를 자르지 않는 실제 Runtime별 비영 수명 토큰이며 미션 교체에서 되돌리지 않는다.
    // input_: 처리할 입력 또는 입력 분배기.
    GumpControlInput input_;
    GumpPointerBoundaryState pointerBoundary_; // 원본 정적 안팎/버튼 기록은 같은 Client UI 수명 동안 유지하고 페이지·월드 교체에서 지우지 않는다.
    std::unique_ptr<RawButtonControls> rawButtons_; // 돌 버튼은 실제 공유 트리/수명/캡처를 사용한다. input_은 메뉴 행/판정 보고용이다.
    std::unique_ptr<WarningMessages> warnings_; // 원본의 모달이 아닌10초 글자 안내와 실제 StyleText/Gump 수명이다.
    ScriptStringRing scriptStrings_; // 입력 큐에 예약한 문자열의 원본80칸 수명이며 페이지가 바뀌어도 보존한다.
    SoloMapFileSpec mapFileSpec_; // 페이지 교체 뒤에도 같은9BYTE FileSpec 키를 보존한다.
    std::unique_ptr<SoloMapCatalog> mapCatalog_; // 현재 파일 캐시/dirty/이름을 Client 수명에 소유한다.
    std::string mapUserName_,pendingFortName_; // 원본 초기 빈 사용자 이름과 별도 pending 이름이다. current Fort 이름과 합치지 않는다.
    std::unique_ptr<DialogBodyLayout> dialogBody_; // 현재페이지의실제StyleText입력/표시복제를소유하며Compose때효과를재실행하지않는다.
    HtmlTextParserGlobals htmlGlobals_; // 원본의페이지간공유P억제전역이다.
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
    bool pageAlive_{}; // Kill 전 현재 HTML 부모다. Kill 뒤에도 retiredPageExits_는 Cleanup까지 원본 창 개수를 유지한다.
    DialogExitCommandState pageExit_; // 현재 표시 부모의 원문 +21e0/+21e4이며 메타 해석 시 고정한 명령/고리 토큰이다.
    std::vector<DialogExitCommandState> retiredPageExits_; // 현재 입력 처리 중 Kill된 페이지의 지연 소멸 값이며 표시 재합성과 구별한다.
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
    std::uint32_t hideBuildings_{}; // 원본0059a8ac의현재표시모드이며메뉴체크와실제Draw가공유한다.
    std::uint32_t worldRedraw_{}; // 원본0059a8b0의 메뉴 명령 갱신 요청이며 실제 Renderer 무효화에 전달한다.
    std::uint32_t menuConfigRevision_{}; // 원본00542678의 설정 명령 증가를 같은 Client 수명에 보관한다.
    std::uint32_t commandGump_{}; // 원본00548740의 초기0이다. 현재 Client는 이 전용 Gump를 아직 생성하지 않는다.
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
