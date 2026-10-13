// 실제 충돌의 anim160 생성·공간 Pop·Regular 예약 및 프레임/삭제 몸체를 연결한다.
#pragma once
#include "o/RawMissile.h"
#include "o/RawAssetRegular.h"

namespace netstorm::o {
// 원본 시각 효과 생성기가 읽는 두 억제 전역과 실제 anim 타입 번호다.
struct GunImpactState {
    double suppressedTime{}; // 패치005c85f0/CD00564db8의 현재 double 억제 값이다.
    std::uint32_t suppressed{}; // 패치005c89b8/CD005178d4의 추가 억제 DWORD다.
    std::uint32_t animationType{160}; // 양판본 실제 anim 자산 번호이며 임의 대체 타입을 받지 않는다.
};
// 생성·공간·주기·삭제 및 아직 별도 복원할 Dude 반응을 같은 월드에 연결한다.
struct GunImpactHooks {
    std::function<std::size_t()> processCount; // 실제 Kernel.Size의 현재 등록 개수다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 실제 SquidFactory.Create(anim160,2)다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 원본 가상+0x90의 현재 좌표/flags Pop이다.
    std::function<void(Sid,std::uint32_t,float)> regular; // 원본 new40/Regular 생성과 flags0x50 실제 부착이다.
    std::function<void(Sid,std::uint32_t)> destroy; // anim 프레임 종료의 가상 Destroy(0)다.
    std::function<void(float,float)> notifyDudes; // 패치00451470/CD004e8e70의 서로 다른 실제 주변 Dude 반응 몸체다.
};
// 포구 효과와 구별하는 실제 충돌/지면 효과 생성 및 anim Handle 처리기다.
class RawGunImpact {
public:
    // 실제 타입/프레임 표와 같은 풀의 지정기·공유 난수·현재 억제 전역을 참조한다.
    RawGunImpact(SidPool& pool,std::span<const RiftTypeFrames> frames,const SquidFrame& setter,
        GameRandom& random,GunImpactState& state,GunImpactHooks hooks);
    // 0041d0d0/CD0043f950: 원본 비교 순서대로 각 좌표를0.5..255에 제한한다.
    static std::array<float,2> Clamp(float x,float y);
    // 0045a750/CD0047c8a0: 억제/Kernel 한도→Clamp→anim 생성→첫 frame→Pop→Regular를 수행한다.
    void Visual(float x,float y,std::uint32_t variant,float interval) const;
    // 00470ac0/CD00455b40: Clamp→현재 Dude 반응→공유 난수100 소비→D 효과0.02다.
    void Ground(float x,float y) const;
    // 0045a6e0/CD0047c810: 유효 count의 실제 SquidFrame.Set 또는 Destroy0/반환-1이다.
    float Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const;
    // 현재 raw 가상 표가 실제 anim인지 확인하며 타입 번호만으로 추측하지 않는다.
    bool Handles(Sid sid) const;
    // 현재 anim만 분배하고 다른 부모의 기존 처리기를 보존한다.
    RegularHandler Handler(RegularHandler fallback={}) const;
    // 어댑터 구성 때 실제 풀 혼용을 거부하기 위한 읽기 참조다.
    const SidPool& Pool() const;
private:
    // 실제 A..P의 첫 frame/개수 표를 현재 자산 타입에서 읽는다.
    RiftTypeFrames::LetterRun Run(std::uint32_t type,std::uint32_t event) const;
    SidPool& pool_; // 실제 anim 슬롯과 원본 판본별 frame 필드 저장소다.
    std::span<const RiftTypeFrames> frames_; // 실제 물리 프레임 글자별 구간 자료다.
    const SquidFrame& setter_; // 표시/해시 단계/Unpop/Repop의 실제 프레임 지정기다.
    GameRandom& random_; // 같은 게임 상태의 공유 난수100 소비를 보존한다.
    GunImpactState& state_; // 다음 호출에서 읽는 원본 효과 억제 전역이다.
    GunImpactHooks hooks_; // 위 명시된 실제 하위 몸체와 관찰 경계다.
};
// 같은 실제 Factory/Kernel/Host를 효과 생성 경계에 연결하며 공간·Dude·삭제는 보존한다.
GunImpactHooks MakeGunImpactRuntimeHooks(const SidPool& pool,SquidFactory& factory,Kernel& kernel,
    SquidProcessHost& host,GunImpactHooks hooks);
// 원본 Missile의 실제 barrier/표적 충돌 소비자를 이 효과 생성기에 연결한다.
MissileHooks MakeMissileGunImpactHooks(const RawGunImpact& impact,MissileHooks hooks);
// 기존 공통 Damageable의 실제 지면 피해 사건을 같은 생성기에 연결한다.
AssetRegularHooks MakeAssetGunImpactHooks(const RawGunImpact& impact,AssetRegularHooks hooks);
}
