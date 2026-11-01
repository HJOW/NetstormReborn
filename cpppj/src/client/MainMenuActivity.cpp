// 원본 활동 갱신과 만료는 실제 현재 장치를 필요한 분기에서만 호출한다.
#include "client/MainMenuActivity.h"
#include <stdexcept>

namespace netstorm::client { // UI 생성/소멸과 같은 현재 저장소를 직접 수정한다.
// 원문은 HTML 양수일 때 팝업을 조회하지 않으며 재설정 뒤에도 같은 프레임에서 만료를 검사한다.
void IdleMainMenuActivity(MainMenuActivity& state,const std::uint32_t& enabled,const std::int32_t& htmlInstances,const ScreenPoint& pointer,const std::uint32_t& currentDialog,std::uint32_t self,const MainMenuActivityHooks& hooks) {
    if (!enabled) return;
    const auto wall=[&] { if (!hooks.now) throw std::invalid_argument("메인 활동 벽시각 없음");return hooks.now(); }; //필요한 실제 조회에서만 현재 장치를 요구해 선행 효과를 보존한다.
    if (htmlInstances>0 || (hooks.popup ? hooks.popup() : throw std::invalid_argument("메인 현재 팝업 장치 없음")) || pointer.x!=state.pointer.x || pointer.y!=state.pointer.y) { state.deadline=wall()+45.0;state.pointer=pointer; }
    if (currentDialog==self) { const auto now=wall();if (state.deadline<=now) { if (!hooks.queue) throw std::invalid_argument("메인 데모 큐 장치 없음");hooks.queue(kMainMenuDemoCommand,"DemoStormRages","DemoStormRages");if (!hooks.kill) throw std::invalid_argument("메인 부모 Kill 장치 없음");hooks.kill(); } }
}
// 활동 사건은 코드 마스크만 적용하며 release/autoDemo 켜짐 조건을 추가하지 않는다.
bool HandleMainMenuActivity(MainMenuActivity& state,std::uint32_t event,const std::function<double()>& now) {
    if ((event&0x03ffffffU)!=kMainMenuActivityEvent) return false;if (!now) throw std::invalid_argument("메인 활동 벽시각 없음");state.deadline=now()+45.0;return true;
}
}
