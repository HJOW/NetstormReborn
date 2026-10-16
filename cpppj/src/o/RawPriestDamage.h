// 실제 사제 피격의 회복 취소·HP1 제한·소유자 통지·HP1 감시 해제를 복원한다.
#pragma once
#include "o/RawGunHit.h"
#include "o/DependFormHost.h"
#include "o/RawPathAnimation.h"
#include "o/RawPriestOwner.h"
#include "o/RawPriestState.h"
#include "o/RawDamageableBurning.h"

namespace netstorm::o {
// 원본 플레이어 레벨 보정과 피격 권한/전역을 같은 월드에서 공유한다.
struct PriestDamageState {
    // invulnerable: 원본NoDamage 전역이며 true면 피격량을0으로 만든다.
    bool invulnerable{};
    // boss: HP1 사제의 현재 의존 감시를 끊는 원본 권한이다.
    bool boss{true};
    // levelCorrectionDisabled: 원본00594fa4/CD0052e170이0아니면 레벨 피해 보정을 생략한다.
    bool levelCorrectionDisabled{};
    // levels: 패치Challenge+0x4f의rank BYTE와 CD Player+0x68 level BYTE를소유자번호로공급한다.
    std::array<std::int32_t,40> levels{};
    // levelCorrectionDisabledRead: 같은 미션 전역을 호출 시 읽는다. 독립 입력은 위 bool을 사용한다.
    std::function<bool()> levelCorrectionDisabledRead;
};
// 사제 피격의 현재 가상 프레임/HP/소유자/효과를 실제 월드에 연결한다.
struct PriestDamageHooks {
    // hasRegenEvent: 원본현재Regular0x25d를 찾는다. nullptr은미복원이다.
    std::function<bool(Sid)> hasRegenEvent;
    // cancelEvent: 원본0x25c/0x25d의 모든현재Regular를같은Host에서 없앤다.
    std::function<void(Sid,std::uint32_t)> cancelEvent;
    // defaultFrame: 현재 타입+0x118의실제defaultFrame을읽는다.
    std::function<std::int32_t(Sid)> defaultFrame;
    // setFrame: 기존 실제SquidFrame.Set(victim,frame,0)을 실행한다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame;
    // setHitPoints: 같은 월드의RawPriestState.SetHitPoints(공간 경계 전환 포함)이다.
    std::function<void(Sid,std::int32_t)> setHitPoints;
    // ownerDamageNotice: 현재소유자에피격 사제/공격자SID를통지한다.
    std::function<void(std::uint8_t,Sid,Sid)> ownerDamageNotice;
    // damageableRefresh: 실제0044c060/CD00462480의burning 판정을 수행한다.
    std::function<void(Sid)> damageableRefresh;
};
// 기본 피해 레벨 보정과 실제Priest+0x98 피격 몸체를 같은 raw 자산에 적용한다.
class RawPriestDamage {
public:
    // 같은 타입/프레임/감시Host를받는다. HP setter와burning은이미 복원한 월드몸체에연결한다.
    RawPriestDamage(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
        PriestDamageState& state,PriestDamageHooks hooks,DependFormHost* depend=nullptr);
    // 0044bfe0/CD00462280: 패치 레벨차+20/CD BYTE 레벨차+8을 기준으로 정수 피해를 보정한다.
    static std::int32_t Adjust(std::int32_t damage,std::uint32_t attackerOwner,std::uint32_t victimOwner,
        const PriestDamageState& state,OriginalEdition edition=OriginalEdition::Patch1078);
    // 00494380/CD0040cb10: live몸체의NoDamage·회복취소·HP1 clamp·소유자/burning·감시해제다.
    bool Hit(Sid victim,std::int32_t damage,std::uint32_t attackerOwner,Sid attacker) const;
    // 같은 풀의GunHit 소비자만 연결한다.
    const SidPool& Pool() const;
private:
    // 현재 사제의 raw HP를 원본 패치DWORD/CDsignedWORD로 읽는다.
    std::int32_t Health(Sid victim) const;
    // 현재 watched쪽DependForm을저장된next순서로 순회하고통지/감시제거를 수행한다.
    void DropDependents(Sid victim) const;
    // pool_: 현재사제/프로세스/감시쌍을공유하는SID저장소다.
    SidPool& pool_;
    // types_: 현재원본 타입계약/프레임표 인덱스다.
    std::span<const RiftTypeRecord> types_;
    // frames_: 현재사제의held방향10..14를 판정하는프레임표다.
    std::span<const RiftTypeFrames> frames_;
    // state_: 현재NoDamage/권한/플레이어레벨전역이다.
    PriestDamageState& state_;
    // hooks_: 실제기존HP/프레임/회복/소유자/burning몸체다.
    PriestDamageHooks hooks_;
    // depend_: 실제양방향감시쌍/통지/제거를처리하는Host이며HP1감시가있을때필수다.
    DependFormHost* depend_{};
};
// 실제사제표적의가상피해를GunHit callback에연결하며다른표적은기존분배기를보존한다.
GunHitHooks MakePriestGunHitHooks(const SidPool& pool,const RawPriestDamage& priest,GunHitHooks hooks,
    std::function<void(Sid,std::int32_t,std::uint8_t,Sid)> fallback={});
// 동일Host의실제Regular검색/반복Kill과기존HPsetter/burning 숫자몸체를사제피격에연결한다.
PriestDamageHooks MakePriestDamageWorldHooks(const SidPool& pool,SquidProcessHost& host,const RawPriestState& hp,
    const PriestHitPointMode& mode,PriestHitPointHooks space,const RawDamageableBurning& burning,PriestDamageHooks hooks);
}
