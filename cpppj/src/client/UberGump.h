// 원본 Ubergump.cpp의 메인 메뉴·Tell·미션 진입을 현재 표시 기반과 연결한다.
#pragma once
#include "client/Gump.h"
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
private:
    // 원본 Tell의 파일.섹션/현재 tell 섹션을 읽어 대화상자를 연다.
    void Tell(std::string target, bool briefing = false);
    // 원본 이름의 설정 객체를 보존해 캠페인 제목과 변수를 치환한다.
    o::Config& Script(std::string_view name);
    // @guideSpec/%userSpec 파일 목록을 현재 파일 설정으로 확장한다.
    void ExpandMenus(DialogPage& page);
    // 표시 상태에 맞춰 배경·돌 장식·글자·판정 영역을 다시 만든다.
    void Compose(bool controls = true);
    // 대기 중인 명령을 실행한다. 지원하지 않는 명령은 실제 동작으로 가장하지 않는다.
    void Execute(const DialogAction& action);
    // 미션 스크립트·요새를 먼저 검증하고 Loading→Briefing으로 넘긴다.
    void BeginMission(std::string name);
    // 이미 읽힌 원본 브리핑 섹션 이름을 순서대로 연다.
    void AdvanceBriefing();
    // 현재 미션 자원을 해제하고 기본 메뉴로 돌아간다.
    void MainMenu();
    // 원본 Options/Help의 목록과 하위 메뉴를 구성한다.
    void OpenMenu(std::string name);
    // 원본 우클릭 메뉴의 연결점. 건설/수확 명령은 다음 단계에서 활성화한다.
    void OpenObjectMenu(o::SquidId id);
    // 커널이 소유한 월드와 입력 참조를 안전한 순서로 제거한다.
    void ClearWorld();
    Client& client_;
    State state_;
    GumpInput input_;
    std::map<std::string, std::unique_ptr<o::Config>> scripts_;
    std::map<std::string, IndexedImage> decorations_;
    SquidFrameMetrics buttonTextureMetrics_{}; // 원본 A00의 signed short 폭/높이. 반복 주기는 각각 1을 뺀 값이다.
    ScreenPoint buttonTextureOffset_{}; // A00 VFX 픽셀 영역의 기준점 상대 위치.
    IndexedImage title_, clouds_;
    DialogPage page_;
    std::vector<std::string> labels_;
    std::vector<DialogAction> actions_;
    std::unique_ptr<MissionScript> mission_;
    GameWorld* world_{};
    o::ProcessId worldProcess_{};
    std::unique_ptr<UserInput> userInput_;
    std::size_t fortObjects_{};
    std::vector<std::string> briefingSections_;
    std::size_t briefingIndex_{};
    std::string pageName_{"main"}, popup_;
    ScreenRect popupRect_{};
    double opened_{};
    bool quit_{}, rebuild_{true};
};
}
