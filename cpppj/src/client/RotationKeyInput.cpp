// 원문004d96b6의 정지 뒤004d97a0..004d97ce의 반전/Config 순서를 보존한다.
#include "client/RotationKeyInput.h"
#include <stdexcept>
#include <string>

namespace netstorm::client { // 현재 전체 DWORD와 실제 설정 callback만 빌린다.
bool DispatchRotationKeyInput(InputEvent event,std::uint32_t& rotationMode,const RotationKeyInputHooks& hooks) {
    const auto key=event.code&0x03ffffffU; // 원문과 같은 수정키 제거이며 명령 비트는 그대로 남긴다.
    if ((event.code&InputCode::kRelease) || (key!='C' && key!='c')) return false;
    if (!hooks.paused) throw std::logic_error("회전 방향 키의 실제 정지 판정 미연결");
    if (hooks.paused()) return true;
    const auto selected=rotationMode==0 ? 1U : 0U;rotationMode=selected; // Config가 현재값을 바꿔도 서식 인자는 이 앞서 저장한 반전값이다.
    if (!hooks.configure) throw std::logic_error("회전 방향 키의 실제 Config 쓰기 미연결");
    hooks.configure("rotmode="+std::to_string(selected));return true;
}
}
