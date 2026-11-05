// 내장 Form을 생성하는 독립 입력에도 보호 PE의 실제 생성자 메타데이터를 명시한다.
#pragma once
#include "o/SquidFactory.h"
#include <stdexcept>

namespace netstorm::test { // 기대값은 만들지 않고 TYPE 입력의 네 생성자 주소만 제공한다.
inline std::vector<o::RiftTypeRecord> MakeFormTypeInputs(std::size_t count) { // 기존 판본별188/171 입력 표를 만들며 다른 크기는 호출 오류다.
    if (count!=188 && count!=171) throw std::invalid_argument("Form 입력 TYPE 표의 판본 크기");
    const auto edition=count==188 ? o::OriginalEdition::Patch1078 : o::OriginalEdition::Cd1072;std::vector<o::RiftTypeRecord> result(count);
    for (const auto type:{1U,2U,3U,6U}) result[type].constructorAddress=o::TypeConstructorAddress(edition,type); // 원문 표의 동일 주소이며 출력 바이트/기대값을 계산하지 않는다.
    return result;
}
}
