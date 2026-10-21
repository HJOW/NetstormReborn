// 외부 효과가 현재 후보의 next를 바꿔도 이미 저장한 다음 SID를 사용한다.
#include "o/RawContainedFinder.h"
#include <bit>
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
    if (parent.value>=pool_.Capacity()) throw std::out_of_range("contained finder 부모 SID 오류");
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
// kind는 DWORD 그대로 비교하므로65536을0으로 축소하지 않는다.후보 mask는 전체 WORD다.
std::int32_t CountContained(const SidPool& pool,const ContainedFinderState& state,Sid parent,std::uint32_t mask,std::uint32_t kind) {
    std::uint32_t count{};RawContainedFinder finder(pool,state);
    // 원본은 종류 판정 뒤에도 mask 조건을 평가하고 전체 현재 종속 체인을 순서대로 센다.
    for (auto sid=finder.Begin(parent);sid.value;sid=finder.Next()) { const auto raw=pool.Slot(sid);const auto candidateKind=static_cast<std::uint32_t>(raw[18])|(static_cast<std::uint32_t>(raw[19])<<8);const bool matches=!kind || kind==candidateKind;const auto candidateMask=static_cast<std::uint32_t>(raw[20])|(static_cast<std::uint32_t>(raw[21])<<8);if ((!mask || (mask&candidateMask)) && matches) ++count; }
    return std::bit_cast<std::int32_t>(count);
}
}
