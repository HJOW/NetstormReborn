// 왼쪽 버튼 이외의 누름은 돌 버튼을 실행하지 않는다. 호버는 목록 행에만 있다.
#include "client/Gump.h"

namespace netstorm::client {
// 이전 화면에서 누른 상태가 새 화면의 같은 번호를 실행하지 않도록 초기화한다.
void GumpInput::SetControls(std::vector<GumpControl> controls) { controls_ = std::move(controls); Cancel(); }
// 원본 부동 소수점 비교와 같은 경계 규칙을 정수 화면 좌표에서 사용한다.
bool GumpInput::Contains(ScreenRect rect, ScreenPoint point) { return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom; }
// 돌 버튼의 캡처는 커서가 밖으로 나가도 유지되며 눌린 그림만 사라진다.
std::optional<int> GumpInput::Event(InputEvent event) {
    if ((event.code & InputCode::kOutside) != 0) { Cancel(); return {}; }
    if ((event.code & 0x03ffffffu) != InputCode::kLeftButton) return {};
    Move({event.x, event.y});
    if ((event.code & InputCode::kRelease) != 0) {
        const auto activated = pressed_; capture_.reset(); pressed_.reset(); return activated;
    }
    // 가장 앞의 활성 판정 영역만 현재 사건을 받는다.
    for (const auto& control : controls_) if (Contains(control.rect, point_)) {
        if (!control.enabled) return {};
        if (control.menu || control.instant) return control.id;
        capture_ = control.id; pressed_ = control.id; return {};
    }
    return {};
}
// 활성 메뉴에는 호버가 있지만 돌 버튼은 누름 캡처 중에만 모습이 변한다.
bool GumpInput::Move(ScreenPoint point, bool active) {
    const auto oldPressed = pressed_, oldHovered = hovered_; point_ = point;
    pressed_.reset(); hovered_.reset();
    if (!active) { capture_.reset(); return oldPressed != pressed_ || oldHovered != hovered_; }
    // 새 좌표에서 눌림/강조를 계산한다.
    for (const auto& control : controls_) if (control.enabled && Contains(control.rect, point)) {
        if (capture_ == control.id) pressed_ = control.id;
        if (control.menu) hovered_ = control.id;
    }
    return oldPressed != pressed_ || oldHovered != hovered_;
}
// 실제 돌 버튼 그리기에서 사용하는 번호.
std::optional<int> GumpInput::Pressed() const { return pressed_; }
// 목록 강조 색을 적용할 번호.
std::optional<int> GumpInput::HoveredMenu() const { return hovered_; }
// 화면/포커스 변화 뒤 남은 캡처를 제거한다.
void GumpInput::Cancel() { capture_.reset(); pressed_.reset(); hovered_.reset(); }
// 메뉴 스모크도 사용자와 같은 좌표·입력 경로를 사용한다.
std::span<const GumpControl> GumpInput::Controls() const { return controls_; }
}
