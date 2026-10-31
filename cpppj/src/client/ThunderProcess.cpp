// 번개도 다른 객체 프로세스와 같은 실제 종속 Form으로 생성하고 삭제한다.
#include "client/ThunderProcess.h"
#include <utility>

namespace netstorm::client { // 원본의 팔레트 계산과 공통 객체 수명을 결합한다.
ThunderProcess::ThunderProcess(bool fullScreen,PaletteFlashHooks hooks,std::function<void(o::ProcessId)> destroyed,std::int32_t brightness)
    :flash_(state_,std::move(hooks)),destroyed_(std::move(destroyed)) { state_.duration=ThunderDuration(fullScreen);state_.offset=ThunderOffset(brightness); } //모드별 기한과 요청 밝기는 생성 인자이며 시각/팔레트는 부착 때 읽는다.
ThunderProcess::~ThunderProcess() { flash_.Cancel();if (destroyed_) destroyed_(Pid()); } // 자연 종료는 이미 복구했으므로 추가 팔레트 변경이 없다.
void ThunderProcess::OnAttached() { flash_.Init(); } // 공통 Attach가 먼저 쓴 실제 Form/부모/PID를 유지한다.
void ThunderProcess::RunFrame() { if (!flash_.Frame()) Host()->Kill(*this,0); } // Kill 뒤에는 this가 삭제될 수 있으므로 다시 접근하지 않는다.
}
