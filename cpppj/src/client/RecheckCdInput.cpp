// 원문4d8b8d..4d8b9e와 4c8d90의 조회→Kill→0쓰기 순서를 보존한다.
#include "client/RecheckCdInput.h"
#include <stdexcept>
namespace netstorm::client { // Guide/Gump가 소비하지 않은 일반 입력만 전달받는다.
namespace { // 선택하지 않은 실제 장치를 선행 요구하지 않는다.
// 필수 장치가 없으면 조용히 성공하지 않고 명시 오류로 멈춘다. 호출자는 Client가 만든 실제 장치를 넘겨야 한다. 단위696에서 추가했다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("RecheckCD 일반 입력의 실제 선택 장치 미연결");return hook; }
}
// 원문 분기는 뗌(최상위 비트)이면 명령 체인 전체를 건너뛴다. 뗌이 아니면 435060(1)을 호출하나 이 함수는 상수 1을 돌려줄 뿐
// 호출자가 결과를 쓰지 않으므로 효과가 없다. 이어 4c8d90이 serial을 조회해 있으면 Kill하고 항상 0을 쓴다. 단위696에서 추가했다.
bool DispatchRecheckCdInput(InputEvent event,std::uint32_t& serial,const RecheckCdHooks& hooks) {
    if ((event.code&0x03ffffffU)!=kRecheckCdCommand) return false;
    if (event.code&InputCode::kRelease) return true; // 뗌은 serial을 읽지도 쓰지도 않는다(원문 관측의 serialwrites 0).
    if (const auto token=Need(hooks.find)(serial)) Need(hooks.kill)(token); // 0도 조회하며 찾은 경우에만 Kill한다.
    serial=0; // Kill 예외가 나면 이 줄에 닿지 않는다. 원문도 Kill 반환 뒤에만 0을 쓴다.
    return true;
}
}
