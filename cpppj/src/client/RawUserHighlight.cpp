// 원본 강조의 모드 쓰기/마우스 TYPE 초기화/현재 선택 스냅샷/외부 효과 순서를 보존한다.
#include "client/RawUserHighlight.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 선택된 필수 외부 몸체가 없으면 앞선 원본 쓰기를 유지한 채 그 위치에서 거부한다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("실제 사용자 강조 경계 미연결");return function;
}
}
// 현재 전역과 외부 소비자의 고정 수명만 빌리며 초기화 효과를 앞당기지 않는다.
RawUserHighlight::RawUserHighlight(UserHighlightState state,UserHighlightHooks hooks):state_(state),hooks_(std::move(hooks)) {}
// 실제4d5480은 이 DWORD 한 번 쓰기 이외의 효과가 없다.
void RawUserHighlight::ResetMouse() const { state_.mouseType=0; }
// mode5는 역투영 전 선택 SID를 보관하므로 역투영 callback의 선택 변경을 이동 인자에 섞지 않는다.
void RawUserHighlight::Highlight(std::uint32_t mode) const {
    if (mode==state_.mode) return;state_.mode=mode;ResetMouse();
    switch (state_.mode) {
    case 1:case 2:case 3:Need(hooks_.selectCursor)(3);return;
    case 4:Need(hooks_.selectCursor)(2);return;
    case 5: {
        const auto selected=state_.selectedSid; // PE의 ESI는004971e0 호출 이전 선택 SID를 보존한다.
        const auto point=Need(hooks_.project)(state_.mouseX,state_.mouseY); // 실제 두 float 반환 비트를 그대로 보관한다.
        Need(hooks_.move)(selected,point[0],point[1],0);return;
    }
    default:Need(hooks_.selectCursor)(1);return;
    }
}
}
