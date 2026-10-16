// 크로스보우의 실제 내림 발사와 공통 회전의 동일 Host 연결을 제공한다.
#pragma once
#include "o/RawArcherAnimation.h"

namespace netstorm::o {
// 10.78 크로스보우 가상 getter의 현재 게임 시각과 원본 전역이다.
struct CrossbowAnimationState {
    double now{}; // 같은 Kernel이 공급하는 현재 게임 시각이다.
    std::uint32_t type{140},displayType{81}; // 실제 windArcher와 arrow의 원본 TYPE다.
    float speed{35},delay{0.2f}; // 0050b168/00508d98에서 읽은 발사 속도와 지연이다.
};
// 004674c0/00468050의 64BYTE 발사 객체와00466f50의 일반 투사체 분기다.
class RawCrossbowAnimation {
public:
    // 현재 풀·타입·방향별 물리 프레임과 월드 효과의 수명을 공유한다.
    RawCrossbowAnimation(SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const ArcherTypeData> animationTypes,CrossbowAnimationState& state,GunAnimationHooks hooks);
    // 공통 ctor와 실제 파생 vtable/미초기화 padding을 보존한다. Host 부착은 별도다.
    void Initialize(GunAnimationFields& fields,Sid gunForm,Sid target) const;
    // due 검사 뒤 추적하고 방향별 마지막 프레임에서 발사 후 완료한다.
    void Frame(GunAnimationFields& fields) const;
    // 실제 HP/free/dead 검사와 arrow81/속도35의 Missile 생성 인자를 전달한다.
    bool Launch(GunAnimationFields& fields) const;
    // 같은 풀과 복원 TYPE만 factory의 계약 검사에 제공한다.
    const SidPool& Pool() const;
    std::uint32_t Type() const;
private:
    // 현재 부모의 실제 TYPE와 번호가 이 몸체의 입력인지 확인한다.
    Sid Parent(const GunAnimationFields& fields) const;
    // 현재 부모를 다시 조회한 뒤 공간 해제·프레임 쓰기·재등록을 수행한다.
    void SetFrame(GunAnimationFields& fields,std::int32_t frame) const;
    // GunProcess 완료 통지 뒤 같은 Host에서 현재 animation을 해제한다.
    void Finish(GunAnimationFields& fields) const;
    SidPool& pool_; // 실제 자산과 Form이 공유하는 풀이다.
    std::span<const RiftTypeRecord> types_; // 현재 표적 HP 플래그를 읽는 타입 표다.
    std::span<const ArcherTypeData> animationTypes_; // 현재 방향수/물리 프레임 수다.
    CrossbowAnimationState& state_; // 현재 시각과 mutable 가상 getter 입력이다.
    GunAnimationHooks hooks_; // 실제 공간·중심·투사체·완료 효과다.
};
// 같은 Kernel에서 실제 크로스보우 발사 몸체를 실행하는35 프로세스다.
class CrossbowAnimation final : public SquidProcess {
public:
    // 실제 원본 필드 이미지와 장기 처리기만 소유/참조한다.
    CrossbowAnimation(const RawCrossbowAnimation& logic,GunAnimationFields fields);
    // 실제 Kernel 틱과 Form/Parent 부착을 원본 필드에 연결한다.
    void RunFrame() override;
    void OnAttached() override;
    // 검사·후속 전송기가 같은 raw 객체를 조회한다.
    GunAnimationFields& Fields();
private:
    const RawCrossbowAnimation& logic_; // 객체보다 오래 사는 실제 발사 처리기다.
    GunAnimationFields fields_; // padding을 포함한64BYTE 원본 이미지다.
};
// 완료/kill만 현재 실제 C++ 프로세스 종류에 맞게 조립한다.
GunAnimationHooks MakeCrossbowAnimationHostHooks(SquidProcessHost& host,GunAnimationHooks hooks);
// 실제 GunForm 부모에 flags0x10으로 TYPE35 발사를 부착한다.
CrossbowAnimation* CreateCrossbowAnimation(SquidProcessHost& host,const RawCrossbowAnimation& logic,Sid gunForm,Sid target);
}
