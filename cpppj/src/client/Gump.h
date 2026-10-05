// 원본 Gump·Buttongump·Menugump의 입력 기반. 전체 원본 객체/깊이 계층은 후속이다.
#pragma once
#include "client/InputEvent.h"
#include "client/Screen.h"
#include <optional>
#include <span>

namespace netstorm::client {
struct GumpControl { int id{}; ScreenRect rect{}; bool menu{}, enabled{true}, instant{}; };
class GumpInput {
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
    std::vector<GumpControl> controls_;
    std::optional<int> capture_, pressed_, hovered_;
    ScreenPoint point_{};
};
}
