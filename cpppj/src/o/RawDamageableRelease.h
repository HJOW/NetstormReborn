// Damageable 삭제 준비에서 권한 확인 뒤 수행하는 종속 해방 구간을 복원한다.
#pragma once
#include "o/RawDamageablePreDestroy.h"
#include "o/RiftType.h"
#include <array>

namespace netstorm::o {
struct DamageableReleaseState {
    bool battle{}; // 00594fbc / CD 00540a20: 전투에서는 허용 genus 검사를 건너뛴다.
    std::uint32_t allowedGenus{0x40480002}; // 00542640 / CD 005178c4의 초기 마스크다.
    std::uint32_t bridgeType{82}; // 005411a0 / CD 0051ca8c: WORD 쓰기 후 표면 알림 대상이다.
};
struct DamageableReleaseHooks {
    // 0044b2e0 / CD 00461430: MakePriestSpawnHooks로 나선 생성 몸체를 연결한다. WORD 전체를 전달한다.
    std::function<void(float,float,std::uint32_t,std::uint16_t)> spawnPriest;
    // 004af530 / CD 004ab390: 일반 자산 생성(type,0). 초기화된 할당 슬롯을 반환한다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create;
    // 가상 +0x74 소유자 지정(new,0)과 +0x90 Pop(new,x,y,0)의 하위 몸체다.
    std::function<void(Sid,std::uint32_t)> setOwner;
    std::function<void(Sid,float,float,std::uint32_t)> pop;
    // 004ac220 / CD 004abb00의 실제 WORD 쓰기 뒤 다리 타입에만 부르는 표면 검사다.
    std::function<void(Sid)> notifySurface;
};
class RawDamageableRelease {
public:
    // 풀·타입 표·공간·현재 전역은 객체보다 오래 살아야 한다. 타입 표를 현재 값으로 읽는다.
    RawDamageableRelease(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> spots,
        const ContainedFinderState& finderState,const DamageableReleaseState& state,DamageableReleaseHooks hooks);
    // 0044b626~0044b80c / CD 0046174e~0046195a. 상위에서 확인한 boss/extra/flags를 다시 평가하지 않는다.
    void Release(Sid parent) const;
    // 같은 풀의 Damageable 콜백 연결을 검사할 때 쓴다.
    const SidPool& Pool() const;
private:
    // 현재 타입의 발자국 중심을 단정도 저장까지 원본 순서로 계산한다.
    std::array<float,2> Center(Sid parent) const;
    // 현재 raw 좌표 두 개를 비트 그대로 읽는다.
    std::array<float,2> Position(Sid parent) const;
    // 발자국·genus를 조회하며 타입 표 범위를 검사한다.
    const RiftTypeRecord& Type(std::uint32_t number) const;
    // 고정 보정 좌표의 공간 바이트다. 원본의 +0.9999 절삭을 쓴다.
    std::uint8_t Spot(const std::array<float,2>& point) const;
    SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    std::span<const std::uint8_t> spots_;
    const ContainedFinderState& finderState_;
    const DamageableReleaseState& state_;
    DamageableReleaseHooks hooks_;
};
// 동일한 풀의 실제 종속 해방만 연결하고 다른 Damageable 효과는 유지한다.
DamageablePreDestroyHooks MakeDamageableReleaseHooks(const SidPool& pool,const RawDamageableRelease& release,DamageablePreDestroyHooks hooks);
}
