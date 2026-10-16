// 실제 플라이어 삭제의 현재+8 WORD 대상 정리와 Damageable 하위 순서를 복원한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
// 같은 월드의 실제 가상 삭제와 Damageable 전처리 경계다.
struct FlyerPreDestroyHooks {
    std::function<void(Sid,std::uint32_t)> destroy; // 현재+8 WORD가 가리키는 대상의 가상+10(0)이다.
    std::function<void(Sid,std::uint32_t)> damageable; // 대상 삭제 뒤 같은 this/flags의0044b4b0이다.
};
// 0045e7e0의 직접 몸체다. 실제 VT 지원 범위는 상위 분배기가 확인한다.
class RawFlyerPreDestroy {
public:
    // 같은10.78 풀과 현재 월드의 하위를 연결하며 모든 참조는 객체보다 오래 살아야 한다.
    RawFlyerPreDestroy(const SidPool& pool,FlyerPreDestroyHooks hooks);
    // 현재 WORD가 비영이면 먼저 삭제하고 callback 뒤 현재 같은 raw 자산을 Damageable로 처리한다.
    void PreDestroy(Sid sid,std::uint32_t flags) const;
private:
    const SidPool& pool_; // 원본+8 WORD를 읽는 고정 SID 풀이다.
    FlyerPreDestroyHooks hooks_; // 실제 대상 삭제/하위 전처리다.
};
}
