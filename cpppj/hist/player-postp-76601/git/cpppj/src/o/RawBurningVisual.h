// Burning 모드0/1 생성자의 표시 등록·150입자 초기화와 실제 입자 추가를 복원한다.
#pragma once
#include "o/RawConstructionLifetime.h"

namespace netstorm::o {
// 원본24바이트 입자는 실제 위치/속도/수명과 고정 source 표식을 보관한다.
struct BurningParticle {
    float x{},y{}; // +0/+4: 화면 또는 피해 프레임 내부 좌표다.
    float velocityX{},velocityY{}; // +8/+c: 현재 입자의 두 이동속도다.
    float life{}; // +10: 현재 수명이며0 이하인 입자는 재사용할 수 있다.
    bool source{}; // +14: this+74 포인터와 같은 고정 연소점인지 표시한다.
};
// 실제 생성자가 쓰는 의미 필드다. 미초기화 delta40/44와 DWORD74의 값을 만들지 않는다.
struct BurningVisualFields {
    RenderProcessFields registration; // 기존 공통 표시 목록/부모/form/영역/깊이 필드다.
    std::uint32_t capacity{},total{}; // +24/+28: 실제 입자 용량150과 현재 입자 수다.
    std::vector<BurningParticle> particles; // +2c: 용량만큼 확보한 현재24바이트 입자다.
    double lastTime{},elapsed{}; // +30/+38: 공통 시각 helper가 이후 사용하는 초기0이다.
    std::uint32_t palette{},sound{}; // +48/+50: 생성자 초기0과 후속 소리 핸들이다.
    std::uint32_t stage{}; // +54: 첫 시각/캡처/좌표 동기화를 제어하는 현재 단계다.
    ConstructionVisualImage image; // +58..+68: 후속 현재프레임 캡처/크기/hotspot이며 생성 때 비어 있다.
    Sid victim{}; // +6c: 부착 경계가 바꾼 parent와 별도로 생성 진입 피해 SID를 보존한다.
    std::uint32_t mode{}; // +70: DudeNotice0 또는 일반 저HP효과1의 실제 인수다.
    std::vector<std::uint8_t> rows; // +78: 후속 고정 연소점 선택표이며 생성 때 비어 있다.
    std::int32_t selected{}; // +7c: 현재 선택된 연소점 개수다.
    bool off{}; // +80: 후속 visible/시각 갱신의 실제 표식이며 생성 때false다.
};
// 원본이 공유하는 Burning 색 표는 생성한 객체 수명과 별개로 월드가 소유한다.
struct BurningVisualState {
    std::uint32_t processType{14}; // 양판본 burningSystemProcessType 번호다.
    std::uint32_t palette{}; // 패치005453d0/CD00538d34의 현재 공유 색 표다.
};
// 첫 색 표의 실제 생성/채우기와 메모리 확보 경계다. 캐시0에서 누락을 거부한다.
struct BurningVisualHooks {
    std::function<bool(std::uint32_t)> reserve; // 생성자150*24 또는 공유색표8바이트 확보다.
    std::function<std::uint32_t()> createPalette; // 실제8바이트 색표 생성자가 반환하는 월드 토큰이다.
    std::function<void(std::array<double,3>,std::array<double,3>,std::uint32_t,double)> gradient; // 원본 주황색 띠의 시작/끝 RGB,색수8,보정2.4다.
    std::function<void(std::uint32_t)> buildPalette; // 실제 색띠를 같은 공유표에 넣는다.
};
// 생성자 상위 몸체만 담당한다. cold색표/입자프레임/표시/삭제는 후속 실제 구현 경계다.
class RawBurningVisual {
public:
    // registry/pool은 같은월드여야 하며 현재state/hooks는 이 처리기보다 오래 살아야 한다.
    RawBurningVisual(const SidPool& pool,RawRenderRegistry& registry,BurningVisualState& state,BurningVisualHooks hooks);
    // 실제 이미확보한 객체의 ctor: 공통 표시등록→의미필드 초기화→150입자 zero→필요한 첫 색표다.
    void Initialize(BurningVisualFields& fields,std::uint32_t token,Sid victim,std::uint32_t mode) const;
    // 00423e80/CD004a60a0: 여유칸 또는 첫 만료입자에 실제6필드를 쓰고 성공 여부를 반환한다.
    bool Append(BurningVisualFields& fields,BurningParticle particle) const;
    // 같은 피해/주변 통지/Host 소비자에서 실제 풀을 확인한다.
    const SidPool& Pool() const;
private:
    const SidPool& pool_; // 현재부모/표시등록과 공유하는 원본 SID 풀이다.
    RawRenderRegistry& registry_; // 생성자가 재사용하는 기존 실제 표시 목록 몸체다.
    BurningVisualState& state_; // 현재type14/공유팔레트 저장소다.
    BurningVisualHooks hooks_; // 실제메모리/첫색표 하위 경계다.
};
}
