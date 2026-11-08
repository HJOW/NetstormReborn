// outpost의 배치 추가 목록/거리 후보 목록 등록과 삭제 준비 접두를 복원한다.
#pragma once
#include "o/SquidPostPop.h"

namespace netstorm::o {
class RawDamageablePreDestroy;
// 아웃포스트 수명 외부 동작과 저장소 조회를 연결하는 콜백 묶음이다.
struct OutpostLifecycleHooks {
    // 지역 소유 투표/갱신(00455380 / CD 0047b3c0)이다. 패치의 세 번째 인자는 항상 0이다.
    std::function<void(float,float)> regionChanged;
    // 작업장 postPop(004538f0 / CD 0047a4b0)의 전체 몸체는 필수 외부 경계다.
    std::function<void(Sid,std::uint32_t)> validatePostPop,postPop;
    // Damageable preDestroy를 같은 raw 풀의 실제 모듈에 연결할 수 있다.
    std::function<void(Sid,std::uint32_t)> validatePreDestroy,preDestroy;
};
// 실제 PATCH 삭제에만 필요한 두 후행 장치다. 사용하지 않는 PostPop/선검사 callback을 요구하지 않는다.
struct OutpostPreDestroyHooks {
    std::function<void(float,float)> regionChanged; // 두 목록 압축 뒤 현재XY로 같은455380 mode0을 호출한다.
    std::function<void(Sid,std::uint32_t)> damageablePre; // 지역 효과 정상 복귀 뒤 같은flags로 현재Damageable 부모를 호출한다.
};
// outpost의 배치 추가 목록/거리 후보 목록 등록과 삭제 준비 접두를 복원한다.
class RawOutpostLifecycle {
public:
    // 두 목록은 factories와 다른 전역이며 호출자/훅 대상은 이 객체보다 오래 살아야 한다.
    RawOutpostLifecycle(SidPool& pool,SquidPostPopList& additional,SquidPostPopList& nearest,OutpostLifecycleHooks hooks);
    // 004557c0 / CD 0047b680: 첫 Pop·일반 객체만 두 목록에 중복 없이 추가한다.
    void PostPop(Sid sid,std::uint32_t flags) const;
    // 00455840 / CD 0047b790: 일반 객체는 모든 중복을 제거하고 같은 flags로 base를 부른다.
    void PreDestroy(Sid sid,std::uint32_t flags) const;
    static void PreDestroyNative(SidPool& pool,SquidPostPopList& additional,SquidPostPopList& nearest,Sid sid,std::uint32_t flags,const OutpostPreDestroyHooks& hooks); //455840은extra9→두현재목록→현재XY/지역→부모순서이며typed자산/부모선검사를추가하지않는다.
    // Damageable 등의 훅 연결에서 실제 풀의 동일성을 검사한다.
    const SidPool& Pool() const;
private:
    // free/contained 또는 자산이 아닌 슬롯을 효과 전에 거부한다.
    std::span<const std::uint8_t> Object(Sid sid) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // additional_: 추가.
    SquidPostPopList& additional_;
    // nearest_: 가까운 대상을 찾는 검색 목록.
    SquidPostPopList& nearest_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (OutpostLifecycleHooks).
    OutpostLifecycleHooks hooks_;
};
// outpost의 부모 삭제 경계를 같은 풀의 실제 Damageable 접두/공통 호출로 교체한다.
OutpostLifecycleHooks MakeOutpostDamageablePreDestroyHooks(const SidPool& pool,const RawDamageablePreDestroy& damageable,OutpostLifecycleHooks hooks);
}
