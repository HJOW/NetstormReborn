// 원본 Gump·Buttongump·Menugump의 입력 기반. 전체 원본 객체/깊이 계층은 후속이다.
#pragma once
#include "client/InputEvent.h"
#include "client/Screen.h"
#include <optional>
#include <span>
#include <vector>

namespace netstorm::client {
// Gump 입력 대상의 식별자·사각형·동작 상태이다.
struct GumpControl { /* id: 대상을 구별하는 식별 번호. */ int id{}; /* rect: 출력·적중·배치 검사에 사용할 사각형. */ ScreenRect rect{}; /* menu: 메뉴 여부. */ /* enabled: 해당 동작을 입력·실행할 수 있는지. */ /* instant: 버튼 해제 대기 없이 누름 즉시 동작할지. */ bool menu{}, enabled{true}, instant{}; };
// 현재 메뉴의 사각형 입력 어댑터다. 복원된 GumpInput 트리 처리기와 서로 다른 타입/저장소를 사용한다.
class GumpControlInput {
public:
    // 현재 화면의 판정 영역을 바꾸며 이전 화면의 누름 캡처를 해제한다.
    void SetControls(std::vector<GumpControl> controls);
    // 004248e0: 오른쪽/아래 끝은 제외한다. 호출자가 버튼에 세로 1픽셀을 더한다.
    static bool Contains(ScreenRect rect, ScreenPoint point);
    // 004249a0·00476820: 돌 버튼은 같은 곳에서 뗄 때, 목록/즉시 버튼은 누르는 순간 실행한다.
    std::optional<int> Event(InputEvent event);
    // 누른 버튼에서 나갔다 돌아오는 모양과 목록 행 강조를 폴링 위치에 맞춘다.
    bool Move(ScreenPoint point, bool active = true);
    // 현재 눌린 모양의 돌 버튼 번호.
    std::optional<int> Pressed() const;
    // 현재 강조된 목록 행 번호.
    std::optional<int> HoveredMenu() const;
    // 포커스 상실/전환 때 캡처와 강조를 지운다.
    void Cancel();
    // 자동 검사에서 실제 화면의 판정 영역을 읽는다.
    std::span<const GumpControl> Controls() const;
private:
    // controls_: 입력 대상인 UI 컨트롤의 목록.
    std::vector<GumpControl> controls_;
    // capture_: 버튼 해제까지 입력을 받을 캡처 대상.
    // pressed_: 버튼의 누름 상태.
    // hovered_: 현재 마우스 위치 아래의 컨트롤.
    std::optional<int> capture_, pressed_, hovered_;
    // point_: 원본 004a4fe0의 창 위치 계산(설정 `initWindowPos`): 1 왼쪽 위, 2 오른쪽 위, 3 오른쪽 아래, 4 왼쪽 아래, 5 가운데 (ScreenPoint).
    ScreenPoint point_{};
};
}
