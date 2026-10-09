// outpost의 배치 추가 목록/거리 후보 목록 등록과 삭제 준비 접두를 복원한다.
#pragma once
#include "o/SquidPostPop.h"

namespace netstorm::o {
class RawDamageablePreDestroy;
struct OutpostLifecycleHooks {
    // 지역 소유 투표/갱신(00455380 / CD 0047b3c0)이다. 패치의 세 번째 인자는 항상 0이다.
    std::function<void(float,float)> regionChanged;
    // 작업장 postPop(004538f0 / CD 0047a4b0)의 전체 몸체는 필수 외부 경계다.
    std::function<void(Sid,std::uint32_t)> validatePostPop,postPop;
    // Damageable preDestroy를 같은 raw 풀의 실제 모듈에 연결할 수 있다.
    std::function<void(Sid,std::uint32_t)> validatePreDestroy,preDestroy;
};
class RawOutpostLifecycle {
public:
    // 두 목록은 factories와 다른 전역이며 호출자/훅 대상은 이 객체보다 오래 살아야 한다.
    RawOutpostLifecycle(SidPool& pool,SquidPostPopList& additional,SquidPostPopList& nearest,OutpostLifecycleHooks hooks);
    // 004557c0 / CD 0047b680: 첫 Pop·일반 객체만 두 목록에 중복 없이 추가한다.
    void PostPop(Sid sid,std::uint32_t flags) const;
    // 00455840 / CD 0047b790: 일반 객체는 모든 중복을 제거하고 같은 flags로 base를 부른다.
    void PreDestroy(Sid sid,std::uint32_t flags) const;
    // Damageable 등의 훅 연결에서 실제 풀의 동일성을 검사한다.
    const SidPool& Pool() const;
private:
    // free/contained 또는 자산이 아닌 슬롯을 효과 전에 거부한다.
    std::span<const std::uint8_t> Object(Sid sid) const;
    SidPool& pool_;
    SquidPostPopList& additional_;
    SquidPostPopList& nearest_;
    OutpostLifecycleHooks hooks_;
};
// outpost의 부모 삭제 경계를 같은 풀의 실제 Damageable 접두/공통 호출로 교체한다.
OutpostLifecycleHooks MakeOutpostDamageablePreDestroyHooks(const SidPool& pool,const RawDamageablePreDestroy& damageable,OutpostLifecycleHooks hooks);
}
