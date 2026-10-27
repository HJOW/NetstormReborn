// 같은00467330을 사용하는 썬/썬더 캐논·반더 타워의 실제 파생 발사와 현재 월드 연결이다.
#pragma once
#include "o/RawGunAnimation.h"
#include "o/RawPriestPlacementGeometry.h"

namespace netstorm::o {
// 실제 가상 단계표/생성자와 투사체 getter를 선택하는 복원 종류다.
enum class CannonFireKind { Sun,Thunder,Vander };
class GameRandom; // 즉시 빔의 회피 판정도 같은 게임 난수 저장소를 사용한다.
// 일반 Missile과 구별한 실제 즉시 빔/피해/회피 경계다.
struct CannonBeamHooks {
    std::function<void(Sid,Sid)> create; // 현재 source/target으로 shortZap 빔을 실제 생성한다.
    std::function<void(Sid)> refresh; // 초기 source의 실제004d5000 시각 갱신이다.
    std::function<void(Sid,Sid)> hit; // 초기 source→현재 target의 실제004d4f00 피해다.
    GameRandom* random{}; // 원본 구간 난수를 소비하는 같은 월드의 저장소다.
};
// 현재 시각과 원본 가상 getter/효과음 단계 전역을 호출 시점에 읽는다.
struct CannonFireState {
    double now{}; // 같은 Kernel 게임 시각이다.
    CannonFireKind kind{CannonFireKind::Thunder}; // 실제 파생 가상 표를 선택한다.
    std::uint32_t type{131},displayType{132},effectType{}; // 부모·thunderCannonMissile·효과의 현재 TYPE다.
    float speed{20},delay{0.5f},launchScale{0.25f}; // 실제 가상 속도/지연과 발사 지연 배율이다.
    std::uint32_t scaledType{131}; // 00541264와 일치하는 부모에서만 발사 지연에 배율을 곱한다.
    std::int32_t growlOffset{5},roarOffset{1}; // 마지막 프레임에서 소리를 내는 실제 두 전역 차이다.
    std::uint32_t vanderType{148},floaterType{146}; // 공통 launch의 즉시 빔과 Cloud Floater 비교 전역이다.
    std::int32_t avoidanceCutoff{800}; // 실제 signed DWORD와 RandomFloat(0,1000)를 비교한다.
};
// 원본 Sun/Thunder Fire의64BYTE 몸체이며 Ice의80BYTE 파생과 구별한다.
class RawCannonFire {
public:
    // 타입별 실제 코드/문자 구간과 같은 풀·전역·효과의 수명을 공유한다.
    RawCannonFire(SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const PriestPlainCanonType> frames,CannonFireState& state,GunAnimationHooks hooks,CannonBeamHooks beam={});
    // 공통64BYTE ctor와 파생 가상 표만 초기화한다. Host 부착은 호출자가 수행한다.
    void Initialize(GunAnimationFields& fields,Sid gunForm,Sid target) const;
    // 실제00467330: due 앞 추적·가상 단계·현재 부모 재조회·발사/완료/지연이다.
    void Frame(GunAnimationFields& fields) const;
    // 같은00467330의 가상 Step만80BYTE 아이스 파생에 맡기며 추적/발사/완료 순서는 공유한다.
    void FrameWithStep(GunAnimationFields& fields,const std::function<std::uint32_t(std::int32_t)>& step) const;
    //004684b0/00468520/00468130의 단계표/효과음과 반더 동시 비트를 계산한다.
    std::uint32_t Step(GunAnimationFields& fields,std::int32_t frame) const;
    //00466f50의 일반 투사체/즉시 빔을 구별하고 callback 뒤의 현재 인수를 재조회한다.
    bool Launch(GunAnimationFields& fields) const;
    // 같은 풀/TYPE의 실제 factory 계약을 확인하는 읽기 전용 조회다.
    const SidPool& Pool() const;
    std::uint32_t Type() const;
private:
    // 현재 부모 번호와 복원한 TYPE를 검사한다.
    Sid Parent(const GunAnimationFields& fields) const;
    // 현재 부모 snapshot의 공간 해제 뒤 DWORD 프레임/재등록을 수행한다.
    void SetFrame(GunAnimationFields& fields,std::uint32_t frame) const;
    // 완료 GunProcess 통지 뒤 실제 현재 animation을 해제한다.
    void Finish(GunAnimationFields& fields) const;
    SidPool& pool_; // 실제 자산·Form의 단일 풀이다.
    std::span<const RiftTypeRecord> types_; // 표적 HP 플래그의 실제 표다.
    std::span<const PriestPlainCanonType> frames_; // TYPE+124/+130/+170의 실제 코드와 문자 구간이다.
    CannonFireState& state_; // 효과 호출 뒤에도 다시 읽는 현재 전역이다.
    GunAnimationHooks hooks_; // 공간·중심·소리·Missile·완료의 실제 소비자다.
    CannonBeamHooks beam_; // 현재 빔 생성과 초기 source/target snapshot의 실제 소비자다.
};
// 같은 Kernel/Host에서 실제 파생 발사를 실행하는 TYPE35 객체다.
class CannonFire final : public SquidProcess {
public:
    // 실제 원본 이미지와 장기 몸체만 소유/참조한다.
    CannonFire(const RawCannonFire& logic,GunAnimationFields fields);
    // Kernel 틱과 실제 Form/Parent 부착을 원본 필드에 반영한다.
    void RunFrame() override;
    void OnAttached() override;
    // 현재 raw 필드는 후속 관찰/전송에도 같은 객체다.
    GunAnimationFields& Fields();
private:
    const RawCannonFire& logic_; // 객체보다 오래 사는 실제 몸체다.
    GunAnimationFields fields_; // padding을 포함한64BYTE 객체 이미지다.
};
// 같은 GunProcess 통지와 실제 CannonFire 클래스의 kill을 연결한다.
GunAnimationHooks MakeCannonFireHostHooks(SquidProcessHost& host,GunAnimationHooks hooks);
// 같은 GunForm 부모에 실제35/flags0x10 발사를 부착한다.
CannonFire* CreateCannonFire(SquidProcessHost& host,const RawCannonFire& logic,Sid gunForm,Sid target);
}
