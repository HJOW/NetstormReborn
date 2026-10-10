// 공유 사제 목록에서 현재 소유자의 첫 살아 있는 항목을 찾는다.
#pragma once
#include "o/RawPriestPostPop.h"

namespace netstorm::o {
// 004921f0 / CD 0040d940: 목록 순서와 DWORD 소유자 인자를 그대로 보존하는 조회다.
class RawPriestLookup {
public:
    // postPop/preDestroy와 같은 상태를 받는다. 풀과 상태는 이 객체보다 오래 살아야 한다.
    RawPriestLookup(const SidPool& pool,const PriestPostPopState& state);
    // 현재 owner BYTE와 전체 player DWORD를 비교한다. dead bit 2만 제외하고 없으면 0이다.
    // 손상된 양수 개수나 범위 밖 DWORD SID는 접근 시 거부하며 WORD로 먼저 자르지 않는다.
    std::uint32_t Find(std::uint32_t player) const;
    // 건설 통지 어댑터가 같은 월드인지 검사하도록 풀을 반환한다.
    const SidPool& Pool() const;
private:
    const SidPool& pool_; // 현재 소유자와 사망 상태를 매 조회마다 읽는 raw 풀이다.
    const PriestPostPopState& state_; // 등록·제거가 즉시 보이는 공유 목록이다.
};
}
