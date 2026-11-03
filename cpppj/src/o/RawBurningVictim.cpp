//진단 callback 중의 현재 SID 변경과 통과 후 슬롯 고정을 구별한다.
#include "o/RawBurningVictim.h"
#include "o/RawSidRobust.h"
#include <stdexcept>
#include <string>

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
BurningVictimHooks MakeBurningVictimHooks(const SidPool& pool,std::span<const RiftTypeRecord> types,const BurningVictimState& state) { //원문의 비가상 HP/가상 표 조회와 현재 진단을 동일한 풀에 연결한다.
    BurningVictimHooks result;
    result.robust=[&state](Sid sid,bool live) {
        if (state.robust) return state.robust(sid,live);
        RawSidRobustDiagnostics diagnostics;SidRobustHooks hooks;hooks.checking=[&state] { return state.checking.Read(); }; //독립 checking0은 원문 번호 판정만 사용한다. 비영이면 미연결 상세 장치를 선택 지점에서 거부한다.
        return RawIsSidRobust(static_cast<std::int32_t>(sid.value),live ? 1 : 0,diagnostics,hooks);
    };
    result.vtable=[&pool](Sid sid) { const auto raw=pool.Slot(sid);return (raw[0]|raw[1]|raw[2]|raw[3])!=0; }; //현재 DWORD의 비영 여부만 같은 네 바이트에서 읽는다.
    result.hasHitPoints=[&pool,types](Sid sid) { const auto raw=pool.Slot(sid);const auto type=raw[10];if (type>=types.size()) throw std::out_of_range("연소 피해 대상의 현재 TYPE 범위");const auto& record=types[type];return record.flags2&0x400000U ? (raw[pool.Edition()==OriginalEdition::Patch1078 ? 40U : 35U]&1U)!=0 : (record.flags1&0x10U)!=0; }; //Dais는 extra bit1, 일반 자산은 현재 flags1 bit10이다.
    result.assertion=[&state](auto condition,auto source,auto line) { if (state.assertion) state.assertion(condition,source,line);else throw std::logic_error("원문 연소 진단 "+std::string(source)+":"+std::to_string(line)+": "+std::string(condition)); }; //독립 장치도 선택 진단을 빈 성공으로 처리하지 않는다.
    return result;
}
}
