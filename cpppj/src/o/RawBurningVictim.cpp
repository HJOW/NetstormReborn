//진단 callback 중의 현재 SID 변경과 통과 후 슬롯 고정을 구별한다.
#include "o/RawBurningVictim.h"
#include <stdexcept>

namespace netstorm::o { //원문 세 부모의 동일한 앞단만 공유하며 게임 동작은 실행하지 않는다.
namespace { //선택된 장치를 빈 성공으로 처리하지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("연소 피해 대상의 선택된 원문 장치 미연결");return function; } //checking0은 첫 진단 장치에 접근하지 않는다.
}
std::optional<Sid> RunBurningVictimParent(const Sid& current,BurningVictimEntry entry,RawCheckingFlag checking,const BurningVictimHooks& hooks) { //번호/TYPE/dead를 사전에 걸러 원문의 실제 조회를 생략하지 않는다.
    const auto robustLine=entry==BurningVictimEntry::Capture ? 0x6fU : entry==BurningVictimEntry::Sources ? 0x91U : 0x11fU; //같은 조건이라도 부모별 원문 줄 번호를 유지한다.
    if (checking && !Need(hooks.robust)(current,true)) Need(hooks.assertion)("isSidRobust(burningSid)",".\\Burning.cpp",robustLine);
    if (!Need(hooks.robust)(current,true)) return std::nullopt;
    const auto selected=current; //두 번째 조회가 바꾼 현재 번호를 여기서만 고정하며 이후 진단은 이 슬롯을 유지한다.
    if (entry==BurningVictimEntry::Capture && !Need(hooks.vtable)(selected)) Need(hooks.assertion)("*((long**)&s)!=0",".\\Burning.cpp",0x77);
    if (!Need(hooks.hasHitPoints)(selected)) Need(hooks.assertion)(entry==BurningVictimEntry::Sources ? "burningVictim.hasHitPoints()" : "s.hasHitPoints()",".\\Burning.cpp",entry==BurningVictimEntry::Capture ? 0x7fU : entry==BurningVictimEntry::Sources ? 0x98U : 0x126U);
    return selected;
}
}
