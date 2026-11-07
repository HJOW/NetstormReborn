// 원문 등록 메시지의 분류/현재 표식과 실제 효과 요청만 소유한다.
#include "client/RegisteredMessages.h"
#include <stdexcept>

namespace netstorm::client { // 설정 도구와 LAN 시작 통지의 공통 메시지 계층이다.
namespace { // 선택된 장치의 누락을 조용한 성공으로 바꾸지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("등록 메시지 실제 장치 미연결");return function; } // 선택된 원문 효과에 실제 callback을 요구한다.
}
void DispatchRegisteredMessage(RegisteredMessageState& state,std::uint32_t message,std::uint32_t parameter,const RegisteredMessageHooks& hooks) { //00436550의 앞부분만 처리하며 반환값으로 일반 메시지를 삼키지 않는다.
    if (message!=state.message) return;
    switch (parameter&0xff00U) {
    case 0x100U: Need(hooks.reload)();Need(hooks.interpret)();Need(hooks.subscriber)();break;
    case 0x200U: state.toolActive=1;break;
    case 0x300U: state.toolActive=0;break;
    case 0x500U: if (Need(hooks.fullscreen)()) Need(hooks.restore)();break;
    default: break;
    }
}
}
