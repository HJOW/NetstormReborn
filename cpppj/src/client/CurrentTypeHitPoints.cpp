// 번호 선택과 같은 물리 TYPE의 현재 getNum/진단 복귀를 공통 getter에 연결한다.
#include "client/CurrentTypeHitPoints.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client { // Reader 사본은 같은 전역 주소와 호출 장치를 유지한다.
std::function<std::int32_t(std::uint32_t)> MakeCurrentTypeHitPoints(o::OriginalEdition edition,std::function<std::span<const o::RiftTypeRecord>()> currentTypes,o::RiftHitPointGlobals globals,std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion) { // 현재 표는 생성 시 캐시하지 않고 실제 읽기 위치에서만 요구한다.
    return [edition,currentTypes=std::move(currentTypes),globals,assertion=std::move(assertion)](std::uint32_t number) {
        if (!currentTypes) throw std::logic_error("현재 TYPE HP의 실제 표 조회 미연결");
        const auto selectedBase=currentTypes();if (number>=selectedBase.size()) throw std::out_of_range("현재 TYPE HP 번호 범위");const auto& selected=selectedBase[number];
        o::RiftHitPointHooks hooks;hooks.number=[&] { return o::CurrentRiftTypeNumber(selected,selectedBase,currentTypes()); };hooks.assertion=assertion;
        return o::ReadRiftHitPoints(edition,selected,globals,hooks);
    };
}
}
