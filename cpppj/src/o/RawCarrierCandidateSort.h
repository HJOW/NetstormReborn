// 골렘 후보 평가기가 사용하는 원본 비교기와 CRT 정렬의 동률 순서를 보존한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
//428020은 요소의 SID 값이 아니라 qsort가 전달한 요소 주소를 SID처럼 계산한다.
class RawCarrierCandidateSort {
public:
    // 기본 주소는 호스트 포인터의 하위32비트다. relocate는 독립 관측의 원본 주소 재배치에만 쓴다.
    explicit RawCarrierCandidateSort(const SidPool& pool,std::function<std::uint32_t(const void*)> relocate={});
    //428020 전체: 두 인자는 역참조하지 않고 감김 산술 후 현재 raw+8의 unsigned WORD를 비교한다.
    std::int32_t Compare(std::uint32_t firstAddress,std::uint32_t secondAddress) const;
    //4e6010/4e5fa0의 폭4 정렬이다. 같은 키의 교환·pivot 이동·작은 구간 처리도 원본 순서다.
    void Sort(std::span<std::uint32_t> candidates) const;
private:
    std::uint32_t Address(const void* pointer) const; // 호스트 주소 또는 관측용 재배치 주소의32비트 표현이다.
    const SidPool& pool_; // 매 비교마다 현재 물리 raw/용량을 읽는 동일 월드다.
    std::function<std::uint32_t(const void*)> relocate_; // 게임 SID 값으로 바꾸는 함수가 아닌 주소 재배치 경계다.
};
}
