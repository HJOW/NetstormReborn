// 외부 효과가 현재 후보의 next를 바꿔도 이미 저장한 다음 SID를 사용한다.
#include "o/RawContainedFinder.h"
#include <stdexcept>

namespace netstorm::o {
namespace {
// 공통 raw 종속 next/head·타입·상태와 dead 비트다.
constexpr std::size_t kNext=4,kHead=6,kType=10,kState=11;
constexpr std::uint8_t kDead=2;
// 정렬되지 않은 종속 WORD를 읽는다.
Sid Link(std::span<const std::uint8_t> raw,std::size_t offset) {
    return {static_cast<std::uint16_t>(raw[offset]|(static_cast<std::uint16_t>(raw[offset+1])<<8))};
}
}
// 독립 커서를 구성하며 원본 가상 필터의 기본 true를 사용한다.
RawContainedFinder::RawContainedFinder(const SidPool& pool,const ContainedFinderState& state):pool_(pool),state_(state) {}
// Begin의 허용 표식은 패치 assert만 우회하며 후보의 타입 조건은 바꾸지 않는다.
Sid RawContainedFinder::Begin(Sid parent,bool allowDead) {
    if (!parent.value || parent.value>=pool_.Capacity()) throw std::out_of_range("contained finder 부모 SID 오류");
    const auto raw=pool_.Slot(parent);
    if (pool_.Edition()==OriginalEdition::Patch1078 && state_.checkingDead && (raw[kState]&kDead) && !state_.boss && !allowDead)
        throw std::logic_error("권한 없는 dead 부모 종속 조회 assert 경로");
    next_=Link(raw,kHead);visited_.clear();return Next();
}
// 반환할 후보의 다음 링크를 먼저 저장해 호출자의 중첩 효과에 대비한다.
Sid RawContainedFinder::Next() {
    // 타입 전역은 매 후보마다 읽으며 순환/범위 오류는 방문 시점에 거부한다.
    while (next_.value) {
        const Sid current=next_;
        if (current.value>=pool_.Capacity() || !visited_.insert(current.value).second)
            throw std::logic_error("contained finder 종속 체인 손상");
        const auto raw=pool_.Slot(current);next_=Link(raw,kNext);
        if (raw[kType]==state_.containedType) return current;
    }
    return {};
}
}
