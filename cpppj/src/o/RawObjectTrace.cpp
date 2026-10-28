// 원본 재귀 호출의 앞선 효과와 현재 전역 조회를 예외 뒤에도 임의로 되돌리지 않는다.
#include "o/RawObjectTrace.h"
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace netstorm::o { // CD의 다른 Form 기준/가상 표는 이 PATCH 복원과 구별한다.
namespace { // 원문 물리 접근과 호스트의 잘못된 입력 거부만 공통화한다.
// 선택된 실제 장치가 없으면 앞선 출력/깊이 변경을 유지한 채 거부한다.
template<class T> const T& Need(const T& value) { if (!value) throw std::logic_error("객체 trace 실제 장치 미연결");return value; }
// 원문의 DWORD 번호를 WORD로 좁히기 전에 물리 슬롯 범위를 확인한다.
std::span<const std::uint8_t> Raw(const SidPool& pool,std::uint32_t sid) {
    if (sid>=pool.Capacity()) throw std::out_of_range("객체 trace 물리 SID 범위");return pool.Slot(Sid{static_cast<std::uint16_t>(sid)});
}
// 정렬되지 않은 head/next WORD를 현재 물리 바이트에서 읽는다.
std::uint32_t Word(std::span<const std::uint8_t> raw,std::size_t offset) { return raw[offset]|(static_cast<std::uint32_t>(raw[offset+1])<<8); }
// 원문은 재귀/종속 고리에서 끝없이 실행한다. 호스트만 현재 경로의 고리를 명시 오류로 거부한다.
// 전역 방문 집합으로 다른 부모가 다시 참조한 정상 후보를 생략하지 않는다.
void Trace(const SidPool& pool,const ObjectTraceGlobals& globals,const ObjectTraceHooks& hooks,std::uint32_t sid,std::unordered_set<std::uint32_t>& active) {
    if (!sid) return;
    if (Raw(pool,sid)[11]&1U) { Need(hooks.log)(29,"sid "+std::to_string(sid)+" is free\n");return; }
    if (!active.insert(sid).second) throw std::logic_error("객체 trace 재귀 종속 고리");
    Need(hooks.trace)(Sid{static_cast<std::uint16_t>(sid)});
    globals.depth+=3U;Need(hooks.indent)(globals.depth);
    std::uint32_t children{}; // free 자식도 방문 횟수에 포함하며 END 출력 조건으로만 사용한다.
    if (globals.checkingDead && !globals.metamap && (Raw(pool,sid)[11]&2U) && !globals.boss) Need(hooks.deadParent)();
    auto next=Word(Raw(pool,sid),6);std::unordered_set<std::uint32_t> chain; // finder는 부모 head부터 시작하고 각 후보 next를 먼저 저장한다.
    while (next) { // 현재 모든 Form 탐색기는 TYPE0..69를 통과시키고 나머지 TYPE만 건너뛴다.
        const auto current=next;const auto raw=Raw(pool,current);
        if (!chain.insert(current).second) throw std::logic_error("객체 trace 형제 종속 고리");
        next=Word(raw,4);if (raw[10]>69U) continue;
        if (current>=globals.count) break;
        ++children;Trace(pool,globals,hooks,current,active);
    }
    globals.depth-=3U;Need(hooks.indent)(globals.depth);
    if (children) { auto number=std::to_string(sid);if (number.size()<4) number.insert(0,4-number.size(),'0');Need(hooks.log)(29,"~1~yEND "+number+"\n"); }
    active.erase(sid);
}
}
// 최초 SID0은 판본/장치 검사 없이 반환하고 PATCH의 실제 함수 경계만 지원한다.
void TraceRawObject(const SidPool& pool,const ObjectTraceGlobals& globals,const ObjectTraceHooks& hooks,std::uint32_t sid) {
    if (!sid) return;
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("객체 trace PATCH 풀 필요");
    std::unordered_set<std::uint32_t> active;Trace(pool,globals,hooks,sid,active); // 같은 경로의 호스트 재귀 고리만 구별한다.
}
}
