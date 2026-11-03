// 삭제된 this를 참조하지 않는 현재 진단 복귀와 원본 분기 순서다.
#include "o/RawPathHaltParent.h"
#include <stdexcept>
namespace netstorm::o { // 실제 하위 경계가 선택된 때에만 공급을 요구한다.
namespace { // 공통 미연결 검사만 내부에서 공유한다.
template<class Function> const Function& NeedPathHalt(const Function& function) { if (!function) throw std::logic_error("공통 Halt의 선택된 실제 경계 미연결");return function; } // 이미 발생한 효과를 되돌리지 않는다.
}
bool RunPathHaltParent(RawCheckingFlag checking,const PathHaltParentHooks& hooks) { // 반환값은 최초 가상 결과를 보존하고 강제 정지10만11로 바꾼다.
    const auto parent=NeedPathHalt(hooks.parent)();if (hooks.armWatch) hooks.armWatch();auto result=NeedPathHalt(hooks.halt)(parent);
    if (!NeedPathHalt(hooks.alive)()) {
        if (checking && result!=12) NeedPathHalt(hooks.assertion)("result == haltALREADY_DEAD","\\Ns\\O\\Pathprocess.cpp",0x2d5);
        return true;
    }
    if (hooks.clearWatch) hooks.clearWatch();
    if (result==10 && NeedPathHalt(hooks.forcedStop)(parent)) result=11;
    if (result==11) { if (NeedPathHalt(hooks.mayKill)()) NeedPathHalt(hooks.kill)();return true; }
    if (NeedPathHalt(hooks.boss)() && NeedPathHalt(hooks.carrier)(parent) && NeedPathHalt(hooks.hasTargets)()) {
        if (hooks.armWatch) hooks.armWatch();NeedPathHalt(hooks.retarget)(parent);if (!NeedPathHalt(hooks.alive)()) return true;if (hooks.clearWatch) hooks.clearWatch();
    }
    return result!=10;
}
}
