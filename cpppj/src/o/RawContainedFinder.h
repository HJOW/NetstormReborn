// contained finder의 현재/다음 SID와 기본 true 필터를 보존한다.
#pragma once
#include "o/SidPool.h"
#include <unordered_set>

namespace netstorm::o {
struct ContainedFinderState {
    bool boss{true}; // 00540bc4 / CD 00540a2c: 현재 권한 전역이다.
    std::uint32_t containedType{6}; // 00541088 / CD 0051c978: 후보 BYTE와 비교하는 DWORD 타입이다.
    bool checkingDead{}; // 005e4794: 패치판 dead 부모 조회 assert를 활성화한다.
};
class RawContainedFinder {
public:
    // 풀/전역 상태는 탐색기보다 오래 살아야 한다. 후보의 free/dead/void/contained는 제외하지 않는다.
    RawContainedFinder(const SidPool& pool,const ContainedFinderState& state);
    // 004b2270 / CD 004eb8d0: 부모 head WORD에서 시작한다. Damageable은 allowDead=true다.
    Sid Begin(Sid parent,bool allowDead=false);
    // 004b1b20 / CD 004eb900: 이전 반환 전에 저장한 next부터 현재 타입 전역으로 진행한다.
    Sid Next();
private:
    const SidPool& pool_;
    const ContainedFinderState& state_;
    Sid next_{};
    std::unordered_set<std::uint16_t> visited_;
};
}
