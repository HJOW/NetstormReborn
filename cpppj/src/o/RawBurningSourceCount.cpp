// 원문4241a2/CD4a5d81의 곱셈/덧셈 감김과 나눗셈 실패를 그대로 구별한다.
#include "o/RawBurningSourceCount.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::o { // C++ signed 오버플로/0 나눗셈을 직접 실행하지 않는다.
std::int32_t BurningSourceCount(OriginalEdition edition,std::int32_t hitPoints,std::int32_t maximum,std::int32_t width,std::int32_t height) { // 실제 원문 계산이 끝난 뒤에만 목표 행을1 이상으로 제한한다.
    const auto twice=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(maximum)*2U); // x86 LEA의 하위 DWORD를 저장한 뒤 signed /4를 수행한다.
    const auto half=twice/4;
    const bool patch=edition==OriginalEdition::Patch1078;
    const auto divisor=std::bit_cast<std::int32_t>(patch ? std::bit_cast<std::uint32_t>(maximum)-std::bit_cast<std::uint32_t>(half) : std::bit_cast<std::uint32_t>(half)-std::bit_cast<std::uint32_t>(maximum));
    const auto numerator=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(hitPoints)*5U);
    if (!divisor) throw std::domain_error("연소점 원문 IDIV의0 나눗셈");
    if (numerator==std::numeric_limits<std::int32_t>::min() && divisor==-1) throw std::overflow_error("연소점 원문 IDIV의signed 몫 범위 초과");
    const auto quotient=numerator/divisor;
    const auto wanted=std::bit_cast<std::int32_t>(patch ? 6U-std::bit_cast<std::uint32_t>(quotient) : std::bit_cast<std::uint32_t>(quotient)+6U);
    const auto rows=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(width)+std::bit_cast<std::uint32_t>(height))/5;
    return std::max(1,std::min(wanted,rows));
}
}
