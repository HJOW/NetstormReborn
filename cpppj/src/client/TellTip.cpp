// 이전/다음/OK의 서로 다른 원문 분기와 DWORD 감김 뒤 signed0하한을 보존한다.
#include "client/TellTip.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 원문 입력과 현재 tipNumber의 같은 DWORD를 빌린다.
namespace { // 실제 선택한 경계에서만 누락을 진단한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("TellTip의 선택된 실제 장치 미연결");return function; } // 앞서 쓴 번호를 실패 때 되돌리지 않는다.
}
// 원문의 첫 정수 인자는 사용하지 않고 HTML 명령이 준 두 번째 정수 인자 y만 읽는다.
bool DispatchTellTip(const InputEvent& event,std::uint32_t& tipNumber,const TellTipHooks& hooks) {
    if ((event.code&0x03ffffffU)!=kTellTipCommand) return false;
    if (event.code&InputCode::kRelease) return true;
    const auto parameter=event.y; //원문 ESI는 Config callback 뒤에도 같은 최초 인자를 보존한다.
    if (parameter==1) { const auto value=tipNumber-1U;tipNumber=std::bit_cast<std::int32_t>(value)<=0 ? 0U : value; }
    else if (parameter>=2) { const auto value=tipNumber+1U;tipNumber=std::bit_cast<std::int32_t>(value)<=0 ? 0U : value; }
    const auto text=Need(hooks.format)(std::bit_cast<std::int32_t>(tipNumber));Need(hooks.setConfig)("tipNumber",text);
    if (parameter!=3) Need(hooks.tell)("Tips");return true;
}
}
