// Damageable의 붕괴/폭발·해방 소리 접두와 공통 삭제 호출을 복원한다.
#pragma once
#include "o/RawCarrierPreDestroy.h"

namespace netstorm::o {
enum class DamageableSound { Collapse,Explosion,PriestFree };
struct DamageableSoundEvent {
    DamageableSound sound{};
    float x{},y{}; // Collapse/Explosion은 현재 raw 좌표다. PriestFree는 전역 소리의 0 인자다.
};
struct DamageablePreDestroyState {
    std::uint32_t priestType{158}; // 005412d0 / CD 0051cbbc: 종속 kind WORD와 비교하는 DWORD다.
};
struct DamageablePreDestroyHooks {
    std::function<void(Sid)> collapse; // 004605f0 / CD 00454800: 붕괴 효과의 하위 몸체다.
    std::function<void(Sid)> explosion; // 004605d0 / CD 004547d0: 폭발 효과의 하위 몸체다.
    std::function<void(const DamageableSoundEvent&)> sound; // 좌표 소리와 전역 priestFree 소리의 요청 경계다.
    std::function<void(Sid)> releaseContained; // 권한·flags 조건 뒤 생성/좌표/소유자/배치를 수행하는 미복원 구간이다.
    std::function<void(Sid,std::uint32_t)> validateBase; // 공통 장부/깊이/후속 공간 효과의 순수 사전 검사다.
    std::function<void(Sid,std::uint32_t)> base; // 004b0950 / CD 004add20: 같은 flags의 공통 preDestroy다.
};
class RawDamageablePreDestroy {
public:
    // 공간 자료는 256×256 바이트다. 풀·전역·공간·훅 대상은 더 오래 살아야 한다.
    RawDamageablePreDestroy(SidPool& pool,std::span<const std::uint8_t> spots,const ContainedFinderState& finderState,
        const DamageablePreDestroyState& state,DamageablePreDestroyHooks hooks);
    // 자산 상태/타입과 연결된 공통 경계를 효과 전에 검사한다.
    void Validate(Sid sid,std::uint32_t flags) const;
    // 0044b4b0 / CD 004615e0. extra&9는 첫 진입에서만 검사하고 공통 body는 항상 호출한다.
    void PreDestroy(Sid sid,std::uint32_t flags) const;
    // 같은 풀의 Carrier 연결에 사용한다.
    const SidPool& Pool() const;
private:
    // raw 현재 좌표에 단정도 0.9999를 더한 뒤 0 방향 절삭해 spot을 읽는다.
    std::uint8_t Spot(Sid sid) const;
    // 각 효과 뒤 현재 좌표 비트를 다시 읽어 위치 소리 요청을 만든다.
    void SoundAt(Sid sid,DamageableSound sound) const;
    SidPool& pool_;
    std::span<const std::uint8_t> spots_;
    const ContainedFinderState& finderState_;
    const DamageablePreDestroyState& state_;
    DamageablePreDestroyHooks hooks_;
};
// Carrier의 Damageable 경계만 같은 풀의 실제 접두/공통 호출로 교체한다.
CarrierPreDestroyHooks MakeDamageablePreDestroyHooks(const SidPool& pool,const RawDamageablePreDestroy& damageable,CarrierPreDestroyHooks hooks);
}
