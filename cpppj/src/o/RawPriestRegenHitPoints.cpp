// 원문4945ee/CD40cd15의 Getter/현재HP/번호 읽기를 문장별로 고정한다.
#include "o/RawPriestRegenHitPoints.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 부호 있는 오버플로를 직접 실행하지 않고 x86 DWORD 비트를 유지한다.
namespace { // 자기 연결 교체 중에도 선택된 실제 장치의 수명을 보존한다.
template<class F> F RegenNeed(const F& function) { if (!function) throw std::logic_error("회복 HP 부모의 선택된 실제 장치 미연결");return function; } // 미선택 TYPE 조회는 요구하지 않는다.
std::int32_t RegenAdd(std::int32_t value,std::int32_t increment) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(value)+std::bit_cast<std::uint32_t>(increment)); } // 원문 ADD의 하위32비트다.
}
std::optional<std::int32_t> ReadPriestRegenHitPoints(OriginalEdition edition,const PriestRegenHitPointHooks& hooks) { // PATCH는 HP를 타입 getter 전, CD는 그 뒤에 저장한다.
    const auto initial=RegenNeed(hooks.current)();const auto maximum=RegenNeed(hooks.maximum)();
    if (initial>=maximum) return std::nullopt;
    const auto increment=[&] { // 두 분기 모두 번호를 재조회하며 번호 자체는 HP 읽기 전에 고정한다.
        const auto number=RegenNeed(hooks.priestType)();
        if (edition==OriginalEdition::Patch1078) {
            const auto current=RegenNeed(hooks.current)();const auto typeHp=RegenNeed(hooks.typeMaximum)(number);
            return RegenAdd(current,typeHp/6);
        }
        const auto typeHp=RegenNeed(hooks.typeMaximum)(number);const auto current=RegenNeed(hooks.current)();
        return RegenAdd(current,typeHp/6);
    };
    const auto candidate=increment();const auto currentMaximum=RegenNeed(hooks.maximum)();
    if (candidate>currentMaximum) return RegenNeed(hooks.maximum)();
    return increment();
}
}
