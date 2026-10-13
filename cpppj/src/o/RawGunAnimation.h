// Gunanim.cpp의 썬 캐논 회전·발사·원위치와 GunProcess 완료 통지를 같은 SID 풀에 연결한다.
#pragma once
#include "o/RawGunProcess.h"
#include <array>

namespace netstorm::o {
// 캠페인 1-1의 썬 캐논이 사용하는 원본 파생 애니메이션 종류다.
enum class GunAnimationKind { SunRotate,SunFire,SunRest };
// 원본 0x38/0x40 객체의 필드와 미초기화 padding을 보존한다.
struct GunAnimationFields {
    // bytes: 직접 필드 이미지이며 조회/직렬화는 원본 쓰기 폭을 사용한다.
    std::array<std::uint8_t,0x40> bytes{};
    // 실제 Baseprocess Form/Parent와 내장 gunForm/target/시각/원위치 방향을 읽는다.
    Sid Form() const;
    Sid Parent() const;
    Sid GunForm() const;
    Sid Target() const;
    double Time() const;
    std::int32_t RestDirection() const;
};
// 실제 Missile 생성자가 받는 12개 인자다. descriptor 복원과 투사체 월드 생성을 구분한다.
struct GunProjectileRequest {
    // firing/target: 실제 발사 자산과 피해 표적의 원본 SID다.
    Sid firing{},target{};
    // speed: 애니메이션 가상 함수가 반환하는 타일/초 속도다.
    float speed{20.0f};
    // displayType/effectType: 비행 표시 자산과 충돌 효과의 원본 타입 번호다.
    std::uint32_t displayType{80},effectType{11};
    // targetPosition/targetCenter: 추적한 raw 위치와004571b0의 교차 레인 배리어 코너다.
    // 일반 발사에서 targetCenter는 invalid(-10000,-10000)이며 표적 발자국 중심과 구별한다.
    std::array<float,2> targetPosition{},targetCenter{};
    // ricochet: 최초 발사는 0이며 이후 Missile 반사 생성이 사용하는 번호다.
    std::uint32_t ricochet{};
    // firingPosition: 포구 기본 선택을 요청하는 원본 invalid 좌표다.
    std::array<float,2> firingPosition{};
};
// 애니메이션 몸체가 읽는 현재 게임 시각과 원본 타입 번호다.
struct GunAnimationState {
    // now: 정지/재개를 포함한 현재 원본 게임 시각이다.
    double now{};
    // sunCannonType/displayType/effectType: 판본의 실제 타입 표에서 공급한다.
    std::uint32_t sunCannonType{128},displayType{80},effectType{11};
};
// 실제 월드 효과 경계다. 필요한 callback이 없으면 성공이나 완료로 우회하지 않는다.
struct GunAnimationHooks {
    // center: 004adea0/CD004aef30의 현재 발자국 중심이다.
    std::function<std::array<float,2>(Sid)> center;
    // targetPoint: 004571b0/CD004793d0의 교차 레인 배리어 코너 선택이다.
    std::function<std::array<float,2>(Sid,Sid)> targetPoint;
    // unpop/repop: 부모 프레임 변경 앞뒤의 실제 공간 해제/재등록이다.
    std::function<void(Sid,std::uint32_t)> unpop,repop;
    // randomInteger: 회전 소리 파일 번호용 원본 구간 난수다.
    std::function<std::int32_t(std::int32_t,std::int32_t)> randomInteger;
    // sound: 현재 부모 좌표에서 지정 WAV를 재생한다.
    std::function<void(Sid,const std::string&)> sound;
    // createProjectile: 실제 Missile 생성·자산 Pop·Kernel 부착을 수행해야 하는 경계다.
    std::function<void(const GunProjectileRequest&)> createProjectile;
    // notify: 완료 event1/info3·6·9/value4·7·10을 현재 GunProcess에 보낸다.
    std::function<void(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> notify;
    // kill: 완료 통지 후 현재 animation Form을 flags0x10으로 없앤다.
    std::function<void(Sid,std::uint32_t)> kill;
};
// 원본 썬 캐논 애니메이션의 전체 프레임 몸체와 launch descriptor를 복원한다.
class RawGunAnimation {
public:
    // 타입 표/상태/효과는 같은 풀의 현재 월드 자료를 사용한다.
    RawGunAnimation(SidPool& pool,std::span<const RiftTypeRecord> types,GunAnimationState& state,GunAnimationHooks hooks);
    // 원본 파생 ctor의 초기화와 gunForm/target setter 및 현재 시각을 준비한다.
    void Initialize(GunAnimationFields& fields,GunAnimationKind kind,Sid gunForm,Sid target) const;
    // 00467130/00467330/00467240 및 CD 대응의 실제 프레임 몸체다.
    void Frame(GunAnimationFields& fields,GunAnimationKind kind) const;
    // 00466f50/CD004a12f0의 표적 검증과 실제 Missile 생성 인자를 만든다.
    // createProjectile가 실제 효과를 구현하지 않았다면 이 메서드 완료는 투사체 복원 완료가 아니다.
    bool Launch(GunAnimationFields& fields) const;
    // 원본 공통 dirty/WORD 표적/시각 packet을 작성하고 dirty를 소비한다.
    static std::uint16_t WritePacket(GunAnimationFields& fields,std::span<std::uint8_t> packet,bool includeParent,bool force=false,std::uint8_t protocol=1);
    // 현재 풀과 부모 타입을 어댑터의 같은 월드 계약에 제공한다.
    const SidPool& Pool() const;
    std::uint32_t SunCannonType() const;
private:
    // 원본 DWORD/BYTE 프레임과 부모 raw를 읽고 유효 계약을 검사한다.
    std::int32_t CurrentFrame(Sid parent) const;
    // 원본 Unpop→프레임 쓰기→Repop(0x40)의 순서를 유지한다.
    void SetFrame(Sid parent,std::uint32_t frame) const;
    // 현재 살아 있는 표적의 위치/중심을 매 프레임 다시 기록한다.
    bool TrackTarget(GunAnimationFields& fields) const;
    // 실제 GunProcess 통지 이후 animation 삭제를 실행한다.
    void Finish(GunAnimationFields& fields,std::uint32_t info,std::uint32_t value) const;
    // pool_: 현재 자산과 애니메이션 Form이 공유하는 SID 풀이다.
    SidPool& pool_;
    // types_: HP 플래그와 부모 group을 읽는 불변 타입 표다.
    std::vector<RiftTypeRecord> types_;
    // state_: 매 프레임 갱신되는 실제 게임 시각과 타입 번호다.
    GunAnimationState& state_;
    // hooks_: 실제 월드의 공간/소리/투사체/완료 효과다.
    GunAnimationHooks hooks_;
};
// 같은 Kernel과 ProcessForm 수명에서 프레임 몸체를 소비하는 애니메이션 객체다.
class GunAnimation final : public SquidProcess {
public:
    // 초기화된 원본 필드와 선택한 파생 종류를 소유한다.
    GunAnimation(const RawGunAnimation& logic,GunAnimationFields fields,GunAnimationKind kind);
    // 실제 Kernel 가상 frame과 부착 통지를 처리한다.
    void RunFrame() override;
    void OnAttached() override;
    // 월드 전송기가 같은 raw 상태로 packet을 작성한다.
    std::uint16_t WritePacket(std::span<std::uint8_t> packet,bool parentChanged,bool force=false);
    // 검사/현재 월드 사건이 같은 필드를 조회한다.
    GunAnimationFields& Fields();
    const GunAnimationFields& Fields() const;
private:
    // logic_: 객체보다 오래 사는 실제 프레임 처리기다.
    const RawGunAnimation& logic_;
    // fields_: 현재 raw 객체 필드와 padding이다.
    GunAnimationFields fields_;
    // kind_: 생성자에서 선택한 원본 파생 종류다.
    GunAnimationKind kind_;
};
// flags0x10 부착·notify·삭제를 실제 호스트와 연결한다. 다른 미복원 effect는 그대로 보존한다.
GunAnimationHooks MakeGunAnimationHostHooks(SquidProcessHost& host,GunAnimationHooks hooks);
// 현재 GunProcess Form의 부모가 썬 캐논인 경우 stage2/5/8의 실제 애니메이션을 생성한다.
GunAnimation* CreateGunAnimation(SquidProcessHost& host,const RawGunAnimation& logic,std::uint32_t stage,Sid gunForm,Sid target);
// RawGunProcess의 startAnimation callback을 위 실제 factory에 연결한다.
GunProcessHooks MakeGunAnimationProcessHooks(SquidProcessHost& host,const RawGunAnimation& logic,GunProcessHooks hooks);
}
