// Unit.cpp의 압축 총 피해 시각과 현재 표적의 실제 가상 피해 전달을 복원한다.
#pragma once
#include "o/RawMissile.h"

namespace netstorm::o {
// 총 타입의 DPS와 패치 전용 flyer/balloon DPS 재정의다.
struct GunHitType {
    // damagePerSecond: 실제 타입 +0x48 float DPS다.
    float damagePerSecond{};
    // specialFlyerDamage/flyerDamagePerSecond: 패치 +0x1b0/+0x1b4의 별도 DPS 선택이다.
    bool specialFlyerDamage{};
    float flyerDamagePerSecond{};
};
// 원본 가상 피해와 공간 통지를 같은 월드에 연결한다.
struct GunHitHooks {
    // damage: 현재 표적 +0x98 피해 처리기로 정수 피해/소유자/공격자 SID를 전달한다.
    std::function<void(Sid,std::int32_t,std::uint8_t,Sid)> damage;
    // notifySurface: 시각 WORD 쓰기 직후 원본004214a0/CD00448c10 통지다.
    std::function<void(Sid)> notifySurface;
};
// 총의 Q16 WORD를8분의1초 시각으로 사용하며 실제 현재 표적 피해를 계산한다.
class RawGunHit {
public:
    // 동일 판본의 타입/genus와 DPS를 공유하고 현재 게임 시각은 호출할 때 전달한다.
    RawGunHit(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const GunHitType> hitTypes,GunHitHooks hooks);
    // 004d4da0/CD00443160: raw+12 WORD의 bit0을 제외한 압축 시각을 읽는다.
    double Time(Sid firing) const;
    // 004d4e10/CD00443200: 현재 시각을 1/8초로 절삭해 WORD에 기록하고bit0을 보존한다.
    void SetTime(Sid firing,double now) const;
    // 004d4f00/CD004432f0: 양자화 시각차×DPS가1 이상일 때 원본 정수 피해를 전달한다.
    // 두 판본 모두30초 초과 간격에서3초분 DPS를 쓰며 CD는 곱을float로 저장한 뒤 절삭한다.
    void Hit(Sid firing,Sid target,double now) const;
    // 004d4eb0/CD004432a0: live 표적의 현재 가상 피해 처리기로 전달한다.
    void Apply(Sid firing,Sid target,std::int32_t damage) const;
    // 동일 월드 연결 여부를 검사한다.
    const SidPool& Pool() const;
private:
    // 원본 group0/1 또는 flyer만 허용하는 시간 필드 계약이다.
    std::span<const std::uint8_t> Firing(Sid firing) const;
    // pool_: 피해 시각/소유자/표적 상태를 읽고 쓰는 실제 SID 저장소다.
    SidPool& pool_;
    // types_: group/genus에 사용하는 같은 실제 타입 표다.
    std::span<const RiftTypeRecord> types_;
    // hitTypes_: 현재 타입의 실제 DPS 보충 자료다.
    std::span<const GunHitType> hitTypes_;
    // hooks_: 실제 피해/공간 통지 경계다.
    GunHitHooks hooks_;
};
// 같은 Missile 소비자에 실제 Gun hit/refresh 시각 몸체를 연결한다.
MissileHooks MakeMissileGunHitHooks(const RawGunHit& hit,const MissileState& state,MissileHooks hooks);
}
