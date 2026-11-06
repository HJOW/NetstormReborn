// 원문 종료 부모의 부분 효과와 메시지/정리 뒤 현재 전역 재조회를 보존한다.
#include "o/RawAllocationFailure.h"
#include <stdexcept>

namespace netstorm::o { // 할당 실패를 정상 게임 성공이나 새 프로세스 생성으로 바꾸지 않는다.
namespace { // 선택한 함수 포인터만 확인하여 미도달 장치를 먼저 요구하지 않는다.
template<class Function> Function AllocationNeed(Function function) { // 누락 장치는 해당 효과 위치에서 명시적으로 거부한다.
    if (!function) throw std::logic_error("메모리 실패의 선택된 원문 장치 미연결");return function;
}
}
bool CleanupRawGameForExit(AllocationFailureState state,const AllocationFailureHooks& hooks) {
    if (!state.initialized || (!state.fort && !state.battle && !state.metamap) || state.exiting) return false;
    state.exiting=1;AllocationNeed(hooks.cleanup)(hooks.context,0);return true; // 원문4437f0은 단일RET이므로 별도 효과를 만들지 않는다.
}
void HandleRawAllocationFailure(AllocationFailureState state,const AllocationFailureHooks& hooks) {
    AllocationNeed(hooks.message)(hooks.context,"Out of memory!","Unrecoverable Error!",0x10);
    static_cast<void>(CleanupRawGameForExit(state,hooks));
    const auto event=AllocationNeed(hooks.currentEvent)(hooks.context); // 메시지/정리 중 교체된 현재 포인터를 이 위치에서 읽는다.
    if (event) AllocationNeed(hooks.signal)(hooks.context,event);
    AllocationNeed(hooks.stopMusic)(hooks.context); // 앞선435060은 return1의 빈 몸체이므로 장치 호출을 추가하지 않는다.
    AllocationNeed(hooks.terminate)(hooks.context,0);
}
}
