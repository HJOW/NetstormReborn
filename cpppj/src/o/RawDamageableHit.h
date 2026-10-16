// 원본 일반 자산의 BaseHit·DamageableHit와 HP/사망/섬 경고 순서를 복원한다.
#pragma once
#include "o/RawPriestDamage.h"
#include "o/RawSquidHitPoints.h"

namespace netstorm::o {
// 실제 NoDamage/레벨 상태와 일반 피격의 서버·경고·사망 전역이다.
struct DamageableHitState {
    // levels: 사제와 같은 실제 Challenge rank/CD Player level/NoDamage 자료를 공유한다.
    PriestDamageState& levels;
    // server/checking: 원본 사망 권한과 패치 held 피격 진단 전역이다.
    bool server{true},checking{};
    // localOwner: 원본 현재 localPlayerId BYTE다.
    std::uint8_t localOwner{1};
    // primaryIslands: Player+0x78/CD+0x70의 현재 주 섬 번호다.
    std::array<std::int32_t,40> primaryIslands{};
    // warningTime/warningDelay: 원본 실제 시계 기준 경고 마감과 float30초 간격이다.
    double warningTime{};
    float warningDelay{30};
    // killedVictim/killerOwner: 원본 서버 사망 직전에 기록하는 두 DWORD 전역이다.
    Sid killedVictim{};
    std::uint32_t killerOwner{};
    // serverRead/localOwnerRead/primaryIslandRead: 세션의 현재 원본 저장소를 읽는다. 미지정 시 위 독립 입력을 사용한다.
    std::function<bool()> serverRead;
    std::function<std::uint8_t()> localOwnerRead;
    std::function<std::int32_t(std::uint8_t)> primaryIslandRead;
};
// 실제 가상 방어/삭제/소유자 통지/공간·경고 UI를 같은 월드에 연결한다.
struct DamageableHitHooks {
    // rejectDamage: 현재 표적 virtual+0x9c의 공격자 판정이며 true면 HP를 쓰지 않는다.
    std::function<bool(Sid,Sid)> rejectDamage;
    // destroy: 원본 virtual+0x10을 같은 실제 삭제기로 수행한다.
    std::function<void(Sid,std::uint32_t)> destroy;
    // ownerDamageNotice: HP쓰기 뒤 현재 owner에 표적/공격자 SID를 통지한다.
    std::function<void(std::uint8_t,Sid,Sid)> ownerDamageNotice;
    // refresh: 기존 실제 RawDamageableBurning.Refresh 몸체다.
    std::function<void(Sid)> refresh;
    // unrealize: 표적 rawextra bit4의 virtual+0x88 공간 재평가다.
    std::function<void(Sid)> unrealize;
    // islandAt: 현재 raw 좌표가 놓인 실제 지면 자산 SID를 조회한다.
    std::function<Sid(float,float)> islandAt;
    // realTime: gameTime과 구별하는 원본 wall clock의 현재 초를 반환한다.
    std::function<double()> realTime;
    // warningEvent: 패치만 event12의 원본 경고 표시 요청이다.
    std::function<void()> warningEvent;
    // warningSound: 현재 islandUnderAttack.wav 요청이다.
    std::function<void()> warningSound;
    // warningText: 패치만 format 뒤 islandUnderAttack 원본 알림을 제출한다.
    std::function<void()> warningText;
};
// 일반 피해 몸체의 raw 폭과 현재 전역/가상 호출 순서를 보존한다.
class RawDamageableHit {
public:
    // 같은 원본 타입/풀/공유 레벨·현재 경고 상태와 실제 하위 경계를 받는다.
    RawDamageableHit(SidPool& pool,std::span<const RiftTypeRecord> types,DamageableHitState& state,DamageableHitHooks hooks);
    // 004add10/CD004aeda0: 방어/레벨/HP/서버 사망 또는 client HP1/소유자 통지다.
    bool BaseHit(Sid victim,std::int32_t damage,std::uint32_t attackerOwner,Sid attacker) const;
    // 0044c8e0/CD00462300: 실제 HP보유 판정·local 섬 경고·unrealize·BaseHit·burning이다.
    bool Hit(Sid victim,std::int32_t damage,std::uint32_t attackerOwner,Sid attacker) const;
    // 동일 월드 adapter의 풀 혼용을 구성 시 거부한다.
    const SidPool& Pool() const;
private:
    // 현재 raw SID/타입 범위를 검사하며 free/dead는 실제 각 몸체가 판정한다.
    std::span<const std::uint8_t> Slot(Sid sid) const;
    // 현재 HP의 패치 signedDWORD/CD signedWORD를 직접 읽는다.
    std::int32_t Health(Sid victim) const;
    // +0x110 방향수와 구별하는 실제 dais genus의 extra bit0/TYPE_HAS_HP 판정이다.
    bool HasHitPoints(Sid victim) const;
    // 실제 지면 genus/번호·현재 local Player 섬과 wall clock 경고를 처리한다.
    void WarnOwner(Sid victim) const;
    // pool/types/state/hooks: 실제 저장소·현재 원본 타입·전역·엄격한 하위 경계다.
    SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    DamageableHitState& state_;
    DamageableHitHooks hooks_;
};
// 실제 현재 vtable와 virtual+0x98 진입주소의 대응이다. 타입 번호로 현재 vtable를 추측하지 않는다.
struct DamageableHitEntry {
    // vtable/address: raw의 현재32비트 가상 표와 원본 피격 진입주소다.
    std::uint32_t vtable{},address{};
};
// 현재 raw vtable의 실제+0x98 주소를 대조해 Base/Damageable을 분배한다. 표는 callback보다 오래 유지한다.
GunHitHooks MakeDamageableGunHitHooks(const RawDamageableHit& damage,std::span<const DamageableHitEntry> hitEntries,GunHitHooks hooks);
// 같은 풀의 실제 burning 숫자 몸체를 일반 피격 소비자에 연결한다.
DamageableHitHooks MakeDamageableHitBurningHooks(const SidPool& pool,const RawDamageableBurning& burning,DamageableHitHooks hooks);
}
