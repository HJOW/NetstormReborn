// 진단 전 저장 값과 진단/해방/Unpop 뒤 현재 값의 순서를 실제 원문대로 분리한다.
#include "o/RawAltarReleaseParent.h"
#include <stdexcept>
namespace netstorm::o { // 선택되지 않은 장치를 성공값으로 대체하지 않는다.
namespace { // 선택한 실제 경계의 누락 검사만 내부에서 공유한다.
template<class Function> const Function& NeedAltarRelease(const Function& function) { if (!function) throw std::logic_error("제단 해방의 선택된 실제 경계 미연결");return function; } // 앞선 효과는 누락/중단 뒤에도 보존한다.
}
void RunAltarReleaseParent(Sid altar,RawCheckingFlag checking,const AltarReleaseParentHooks& hooks) { // 체크중 checking이0이 돼도 이미 진입한 방향 판정을 다시 시작하지 않는다.
    const auto dais=NeedAltarRelease(hooks.findDais)(altar);
    if (checking && (NeedAltarRelease(hooks.direction)(altar)==12 || NeedAltarRelease(hooks.direction)(altar)==11))
        NeedAltarRelease(hooks.assertion)("!isInvisibleFrame() && !isSacrifice()",".\\Dais.cpp",0x578);
    if (NeedAltarRelease(hooks.valid)(dais)) NeedAltarRelease(hooks.release)(dais);
    NeedAltarRelease(hooks.unpop)(altar,0);NeedAltarRelease(hooks.plainP)(altar);NeedAltarRelease(hooks.repop)(altar,0);
}
}
