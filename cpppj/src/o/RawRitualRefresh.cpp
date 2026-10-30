// 큰 의식/포획 처리를 재현하지 않고 같은 로컬 완료 분기의 세 실제 자식만 실행한다.
#include "o/RawRitualRefresh.h"
#include <stdexcept>

namespace netstorm::o { // 원문 순서와 현재 owner 재조회를 보존한다.
namespace { // 선택한 장치의 누락은 그 호출 위치에서만 거부한다.
template<class Function> const Function& Need(const Function& function) { // 앞선 정상 효과는 되돌리거나 성공 기본값으로 바꾸지 않는다.
    if (!function) throw std::logic_error("의식 뒤 로컬 갱신의 실제 선택 장치 미연결");return function;
}
}
// 원문은 마지막 cdecl snapshot 앞에서도 사용하지 않는 owner/ECX를 다시 계산한다.
void RefreshLocalRitualData(OriginalEdition edition,const RitualRefreshHooks& hooks) {
    if (edition!=OriginalEdition::Patch1078) throw std::invalid_argument("의식 뒤 로컬 갱신은 실제10.78 경계가 필요합니다");
    Need(hooks.core)();const auto owner=Need(hooks.templateOwner)();Need(hooks.coreData)(owner);
    static_cast<void>(Need(hooks.templateOwner)());Need(hooks.snapshot)();
}
}
