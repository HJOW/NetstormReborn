// 순회 중 현재 풀/로그가 바뀌어도 보관된 후보 목록으로 대체하지 않는다.
#include "client/SpecialPointerTrace.h"
#include <stdexcept>
namespace netstorm::client { //일반 좌클릭의 편집 후처리보다 앞에 있는 원문 진단 부모다.
namespace { //도달한 경계만 지연 검사한다.
template<class F> const F& Need(const F& value) { if (!value) throw std::logic_error("Ctrl special 지점 진단의 실제 하위 미연결");return value; } //미선택 경계는 요구하지 않는다.
}
void TraceSpecialPointer(bool control,std::uint32_t special,const InputEvent& event,const SpecialPointerTraceHooks& hooks) { //조회 결과가0이면 후보/진단을 호출하지 않는다.
    if (!special || !control) return;
    const auto point=Need(hooks.project)(event.x,event.y);auto current=Need(hooks.begin)(point);
    while (current) { Need(hooks.trace)(current);current=Need(hooks.next)(); } //가상70 복귀 뒤 현재 finder를 진행하며 중복 후보도 임의 제거하지 않는다.
}
}
