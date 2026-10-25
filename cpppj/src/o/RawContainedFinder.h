// contained finder의 현재/다음 SID와 기본 true 필터를 보존한다.
#pragma once
#include "o/SidPool.h"
#include <unordered_set>

namespace netstorm::o {
// 내용물 검색기 처리 중 조회·갱신하는 상태 묶음이다.
struct ContainedFinderState {
    bool boss{true}; // 00540bc4 / CD 00540a2c: 현재 권한 전역이다.
    std::uint32_t containedType{6}; // 00541088 / CD 0051c978: 후보 BYTE와 비교하는 DWORD 타입이다.
    bool checkingDead{}; // 005e4794: 패치판 dead 부모 조회 assert를 활성화한다.
    const bool* sharedBoss{}; // Carrier·Damageable·내용물 검색이 같은 실제 Client boss를 읽는 주소다.
    bool Boss() const { return sharedBoss ? *sharedBoss : boss; } // 외부 효과 뒤 또는 dead 진단의 원문 관문에서 현재값을 조회한다.
};
// contained finder의 현재/다음 SID와 기본 true 필터를 보존한다.
class RawContainedFinder {
public:
    // 풀/전역 상태는 탐색기보다 오래 살아야 한다. 후보의 free/dead/void/contained는 제외하지 않는다.
    RawContainedFinder(const SidPool& pool,const ContainedFinderState& state);
    // 004b2270 / CD 004eb8d0: 부모 head WORD에서 시작한다. Damageable은 allowDead=true다.
    Sid Begin(Sid parent,bool allowDead=false);
    // 004b1b20 / CD 004eb900: 이전 반환 전에 저장한 next부터 현재 타입 전역으로 진행한다.
    Sid Next();
private:
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;
    // state_: 처리 중 조회·갱신하는 상태 (const ContainedFinderState).
    const ContainedFinderState& state_;
    // next_: 다음 항목의 번호 또는 다음 처리 위치.
    Sid next_{};
    // visited_: 방문.
    std::unordered_set<std::uint16_t> visited_;
};
//004acc60의 기본 finder로 kind DWORD/WORD mask 조건에 맞는 모든 내용물을 센다.중복/free/dead 후보를 임의 제외하지 않는다.
std::int32_t CountContained(const SidPool& pool,const ContainedFinderState& state,Sid parent,std::uint32_t mask,std::uint32_t kind);
}
