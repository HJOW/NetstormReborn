// 기술 부모·타입 개수를 callback 뒤 다시 읽으며 원본 CheatAllTechnology의 필터와 하위 호출만 수행한다.
#include "o/RawThemeTechnology.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 누락된 실제 경계를 빈 성공으로 대체하지 않는다.
RawThemeTechnology::RawThemeTechnology(ThemeTechnologyHooks hooks):hooks_(std::move(hooks)) {
    if (!hooks_.typeCount || !hooks_.typeAt || !hooks_.currentTechnology || !hooks_.findContained || !hooks_.createContained)
        throw std::invalid_argument("테마 기술은 현재 타입/현재 부모/실제 contained 경계가 모두 필요합니다");
}
// 타입0도 원본 필터에 참여하며 Find와 Create 사이에는 현재 기술 부모를 반드시 재조회한다.
void RawThemeTechnology::Apply(std::int32_t theme) const {
    // count를 캐시하지 않아 typeAt/Find/Create callback의 변경이 다음 반복 경계에 반영된다.
    for (std::int32_t type=0;type<hooks_.typeCount();++type) {
        const auto& record=hooks_.typeAt(type);
        if (record.group==10 || record.theme==5 || record.theme!=theme) continue;
        const auto parent=hooks_.currentTechnology();
        if (!hooks_.findContained(parent,4,static_cast<std::uint32_t>(type),0).value)
            hooks_.createContained(hooks_.currentTechnology(),static_cast<std::uint32_t>(type),0,4,0x10,0);
    }
}
}
