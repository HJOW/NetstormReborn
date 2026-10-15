// 원본 Ubergump.cpp의 메인 메뉴·Tell·미션 진입을 현재 표시 기반과 연결한다.
#pragma once
#include "client/Gump.h"
#include "client/RawButtonControls.h"
#include "client/ScriptCommand.h"
#include "client/HtmlTextParser.h"
#include "client/State.h"
#include "client/Mission.h"
#include "client/VFXDraw.h"
#include "o/Kernel.h"
#include "o/Squid.h"
#include <map>
#include <memory>

namespace netstorm::client {
class Client;
class GameWorld;
class UserInput;
class WarningMessages;
class DialogBodyLayout;
// 메뉴·브리핑·미션 페이지와 해당 UI 동작을 관리하는 처리기이다.
class UberGump {
public:
    // 초기화 뒤 원본 타이틀·구름·메뉴 자료를 읽고 기본 메뉴를 연다.
    explicit UberGump(Client& client);
    // 검사 장면과 미션 설정을 Renderer보다 먼저 해제한다.
    ~UberGump();
    // 원본 루프 시작의 State 자리에서 대기 명령을 실행하고 시간 제한을 갱신한다.
    void Tick();
    // 커널이 월드를 진행한 직후 변경된 실제 장면을 제출한다.
    void Frame();
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
    // --ui-script 전용 검사 경계다. 실제 월드의 가상 삭제를 호출하며 일반 게임 입력에는 노출하지 않는다.
    bool InspectDestroySurface(std::string_view type,float x,float y);
    // --ui-script의 커널 검사에서 일반 게임과 같은 Tell 경로를 실행한다. 일반 사용자 명령으로 노출하지 않는다.
    void InspectTell(std::string target);
private:
    // 원본 Tell의 파일.섹션/현재 tell 섹션을 읽어 대화상자를 연다.
    void Tell(std::string target, bool briefing = false);
    // 원본00482de0처럼 현재 미션/일반 섹션이 없으면 false로 반환하며 기존 페이지를 유지한다.
    bool TryTell(std::string target,bool briefing=false);
    // 원본 이름의 설정 객체를 보존해 캠페인 제목과 변수를 치환한다.
    o::Config& Script(std::string_view name);
    // @guideSpec/%userSpec 파일 목록을 현재 파일 설정으로 확장한다.
    void ExpandMenus(DialogPage& page);
    // 표시 상태를 합성한다. 페이지 교체 예약이 있을 때만 입력 영역과 실제 돌 버튼을 함께 교체한다.
    void Compose();
    // 새 페이지/팝업/해상도의 입력 영역 교체를 다음 합성에 예약한다. 단순 픽셀 갱신에는 쓰지 않는다.
    void RebuildControls();
    // 대기 중인 명령을 실행한다. 지원하지 않는 명령은 실제 동작으로 가장하지 않는다.
    void Execute(const DialogAction& action);
    // 미션 스크립트·요새를 먼저 검증하고 Loading→Briefing으로 넘긴다.
    void BeginMission(std::string name);
    // 이미 읽힌 원본 브리핑 섹션 이름을 순서대로 연다.
    void AdvanceBriefing();
    // 현재 미션 자원을 해제하고 기본 메뉴로 돌아간다.
    void MainMenu();
    // 현재 미션/요새의 설정 제목을 원본 선택 순서로 조회하여 실제 창에 적용한다.
    void RefreshWindowTitle();
    // 원본 Options/Help의 목록과 하위 메뉴를 구성한다.
    void OpenMenu(std::string name);
    // 원본 우클릭 메뉴의 연결점. 건설/수확 명령은 다음 단계에서 활성화한다.
    void OpenObjectMenu(o::SquidId id);
    // 커널이 소유한 월드와 입력 참조를 안전한 순서로 제거한다.
    void ClearWorld();
    // client_: 원본 전역 변수로 흩어져 있던 클라이언트 상태를 객체 하나로 묶는다. 한 프로세스에 하나만 만든다 (Client).
    Client& client_;
    // state_: 처리 중 조회·갱신하는 상태 (State).
    State state_;
    // input_: 처리할 입력 또는 입력 분배기.
    GumpControlInput input_;
    std::unique_ptr<RawButtonControls> rawButtons_; // 돌 버튼은 실제 공유 트리/수명/캡처를 사용한다. input_은 메뉴 행/판정 보고용이다.
    std::unique_ptr<WarningMessages> warnings_; // 원본의 모달이 아닌10초 글자 안내와 실제 StyleText/Gump 수명이다.
    ScriptStringRing scriptStrings_; // 입력 큐에 예약한 문자열의 원본80칸 수명이며 페이지가 바뀌어도 보존한다.
    std::unique_ptr<DialogBodyLayout> dialogBody_; // 현재페이지의실제StyleText입력/표시복제를소유하며Compose때효과를재실행하지않는다.
    HtmlTextParserGlobals htmlGlobals_; // 원본의페이지간공유P억제전역이다.
    std::uint32_t buttonTextureType_{},buttonTextureFrame_{}; // 실제 A00 배경 생성자에 전달할 타입/프레임 번호다.
    // scripts_: 스크립트.
    std::map<std::string, std::unique_ptr<o::Config>> scripts_;
    // decorations_: 장식.
    std::map<std::string, IndexedImage> decorations_;
    SquidFrameMetrics buttonTextureMetrics_{}; // 원본 A00의 signed short 폭/높이. 반복 주기는 각각 1을 뺀 값이다.
    ScreenPoint buttonTextureOffset_{}; // A00 VFX 픽셀 영역의 기준점 상대 위치.
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
    // world_: 미션 지도와 객체 상태를 갱신하고 월드 그림을 수집하는 프로세스 (GameWorld).
    GameWorld* world_{};
    // worldProcess_: 월드 프로세스.
    o::ProcessId worldProcess_{};
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
