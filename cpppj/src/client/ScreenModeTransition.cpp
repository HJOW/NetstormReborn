// 실패 진단 뒤 이전 모드를 복구하고 이후의 현재 모드/갱신값을 다시 읽는 원문 부모다.
#include "client/ScreenModeTransition.h"
#include <stdexcept>

namespace netstorm::client { // 원문 성공 경로와 예외 복구 경로를 같은 장치에 연결한다.
namespace { // 선택된 장치의 누락은 화면 변경 실패로 삼키지 않는다.
template<class Function> const Function& Need(const Function& function) { // 실제 의존성 누락을 명시한다.
    if (!function) throw std::logic_error("화면 모드 전환의 실제 장치 미연결");return function;
}
}
void TransitionScreenMode(std::uint32_t requested,std::uint32_t& refresh,const ScreenModeTransitionHooks& hooks) { //00435ba0/435c29의 현재 값 소비 순서다.
    const auto previous=Need(hooks.current)(); // 예외 복구용 값은 원문처럼 비트 정리/장치 변경 전에 보관한다.
    if (!(requested&1U)) requested=(requested&0xffffffabU)|2U;
    if (requested!=hooks.current()) {
        const auto& change=Need(hooks.change); // 누락된 callback은 원문의 장치 예외가 아니다.
        try { change(requested); }
        catch (...) { // 원문 C++ catch 경계는 첫 장치 변경만 감싸며 복구/설정 실패는 재포획하지 않는다.
            Need(hooks.diagnostic)(0x1d,"screen mode change failed.\n");
            if (Need(hooks.initialized)()) change(previous);
        }
    }
    Need(hooks.remember)("startInFullScreen",hooks.current()&1U); // 복구 뒤 현재 장치 결과를 저장하며 최초 요청으로 성공을 합성하지 않는다.
    ++refresh; //005c85d8은 설정 callback 뒤의 현재 DWORD를 읽으며 UINT32 감김을 유지한다.
}
}
