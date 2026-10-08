// 사제 preDestroy의 목록 제거·보호막 삭제 요청·Carrier 호출 순서를 복원한다.
#pragma once
#include "o/RawPriestPostPop.h"

namespace netstorm::o {
struct PriestPreDestroyHooks {
    std::function<Sid(Sid)> findForcefield; // 동일 소유자/위치의 보호막 조회다. MakePriestForcefieldHooks로 실제 finder에 연결한다.
    std::function<void(Sid,std::uint32_t)> destroyForcefield; // 반환 SID가 풀 범위 안이면 가상 destroy(flags=0)를 요청한다.
    std::function<void(Sid,std::uint32_t)> validateCarrier; // 순수 사전 검사다. raw/목록을 바꾸지 않고 미연결 하위 효과를 거부한다.
    std::function<void(Sid,std::uint32_t)> carrierPre; // Carrier→Damageable의 전체 삭제 준비는 명시적인 외부 경계다.
};
class RawPriestPreDestroy {
public:
    // postPop과 같은 사제 목록을 받는다. 상태/훅의 참조 대상은 이 객체보다 오래 살아야 한다.
    RawPriestPreDestroy(SidPool& pool,PriestPostPopState& state,PriestPreDestroyHooks hooks);
    // 004919b0 / CD 0040c330. dead/void여도 실행하며 extra&9이면 Carrier만 호출한다.
    // 깊이 감소는 연결한 Carrier의 책임이다. 이 몸체는 일반 사제 Pop/공간 해제를 허용하지 않는다.
    void PreDestroy(Sid sid,std::uint32_t flags) const;
    // 가상 분배기가 같은 풀인지 확인하고 실제 사제 표만 판별한다.
    const SidPool& Pool() const;
    bool Handles(Sid sid) const;
private:
    SidPool& pool_;
    PriestPostPopState& state_;
    PriestPreDestroyHooks hooks_;
};
// 사제 PreDestroy만 새 몸체로 분배하고 다른 삭제 사건/선택/form 훅은 fallback으로 넘긴다.
// ProcessHost의 자산 훅으로 연결하면 회복 form의 실제 Kernel 제거를 그대로 사용할 수 있다.
SquidDestroyHooks MakePriestPreDestroyHooks(const SidPool& pool,const RawPriestPreDestroy& priest,SquidDestroyHooks fallback);
}
