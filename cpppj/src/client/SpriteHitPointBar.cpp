// 원문49898c/CD456789의 HP 막대 부모를 실제 출력 장치와 분리한다.
#include "client/SpriteHitPointBar.h"
#include <stdexcept>

namespace netstorm::client { // 색 배열은 진단 복귀 뒤 현재값을 읽도록 사본을 만들지 않는다.
namespace { // 자기 연결 교체 중에도 선택된 읽기 장치의 수명을 유지한다.
template<class F> F BarNeed(const F& function) { if (!function) throw std::logic_error("선택 HP 막대의 실제 조회 장치 미연결");return function; } // 미선택 읽기에는 장치를 요구하지 않는다.
}
SpriteHitPointBar ReadSpriteHitPointBar(o::OriginalEdition edition,const std::array<std::uint32_t,3>& colors,const SpriteHitPointBarHooks& hooks) { // 각 원문 호출 사이에 현재 HP를 캐시하지 않는다.
    SpriteHitPointBar result;
    if (edition==o::OriginalEdition::Patch1078) {
        result.color=colors[0];
        auto current=BarNeed(hooks.current)();auto maximum=BarNeed(hooks.maximum)();
        if (current<=maximum/2) result.color=colors[1];
        current=BarNeed(hooks.current)();maximum=BarNeed(hooks.maximum)();
        if (current<=maximum/4) result.color=colors[2];
        result.current=BarNeed(hooks.current)();result.maximum=BarNeed(hooks.maximum)();
        return result;
    }
    result.maximum=1;
    if (BarNeed(hooks.maximum)()>=1) result.maximum=BarNeed(hooks.maximum)();
    result.current=BarNeed(hooks.current)();
    const auto lesser=result.current>result.maximum ? result.maximum : result.current; // CD는 std::clamp의 양수 범위 가정을 추가하지 않는다.
    if (lesser<0) result.current=0;
    else if (result.current>result.maximum) result.current=result.maximum;
    result.color=colors[0];
    if (result.current<=result.maximum/2) result.color=colors[1];
    if (result.current<=result.maximum/4) result.color=colors[2];
    return result;
}
}
