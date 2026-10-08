// Carrier 삭제 준비와 contained finder의 타입·WORD 조건 조회를 복원한다.
#pragma once
#include "o/RawPriestPreDestroy.h"
#include "o/RawContainedFinder.h"

namespace netstorm::o {
// Carrier와 Damageable이 같은 권한/contained 타입 전역을 공유한다.
using CarrierPreDestroyState=ContainedFinderState;
struct CarrierPreDestroyHooks {
    std::function<void(Sid,std::uint32_t)> validateDamageable; // 효과 전 하위 경계의 순수 사전 검사다.
    std::function<void(Sid,std::uint32_t)> damageablePre; // 0044b4b0 / CD 004615e0의 하위 효과 경계다.
    std::function<void()> dependentFound; // 00485e40 / CD 004151d0: 인자 없는 전역 후처리 경계다.
};
class RawCarrierPreDestroy {
public:
    // 가상 지원 범위를 넓히지 않는 직접 호출용 몸체다. 풀·상태·훅 대상은 더 오래 살아야 한다.
    RawCarrierPreDestroy(SidPool& pool,const CarrierPreDestroyState& state,CarrierPreDestroyHooks hooks);
    // 사제 목록 변경 전에 하위 몸체와 raw 자산의 지원 상태를 검사한다.
    void Validate(Sid sid,std::uint32_t flags) const;
    // 00426890 / CD 004e43b0: Damageable→현재 boss→FindContained(1,0,0)→전역 후처리다.
    void PreDestroy(Sid sid,std::uint32_t flags) const;
    // 004ac9f0 / CD 004ac560: 타입 일치 종속의 +0x14 WORD & mask, +0x12 WORD == kind를 검사한다.
    // 0인 조건은 생략하고 index개의 일치를 건너뛴다. 후보 상태/extra는 추가로 거르지 않는다.
    Sid FindContained(Sid parent,std::uint32_t mask,std::uint32_t kind,std::uint32_t index) const;
    // 연결 어댑터가 같은 풀인지 확인한다.
    const SidPool& Pool() const;
private:
    SidPool& pool_;
    const CarrierPreDestroyState& state_;
    CarrierPreDestroyHooks hooks_;
};
// 사제의 Carrier 경계를 같은 풀의 실제 몸체에 연결하고 보호막 훅을 보존한다.
PriestPreDestroyHooks MakeCarrierPreDestroyHooks(const SidPool& pool,const RawCarrierPreDestroy& carrier,PriestPreDestroyHooks hooks);
}
