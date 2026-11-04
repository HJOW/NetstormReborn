// Evilray.cpp의 생성·중단 통지와 공통 Particle 소멸 순서를 복원한다.
#pragma once
#include "o/RawFieryTailFrame.h"
#include "o/ParticleGradient.h"

namespace netstorm::o { // 실제 process10의 원문 저장소와 선택 장치만 제공한다.
// 공통 SystemProcess와 EvilRay 파생 필드의 의미 저장소다. 호스트 메모리 배치를 원본 객체 주소로 사용하지 않는다.
struct EvilRayFields {
    RenderProcessFields registration; // 공통 +4..+20의 표시 등록과 부모/Form이다.
    std::vector<FieryTailParticle> particles; // +24/+2c: 원문50개·24BYTE 입자이며 공통 필드를 재사용한다.
    std::int32_t total{}; // +28: 사용한 입자 수이며 생성 시0이다.
    double lastTime{},elapsed{}; // +30/+38: 공통 시계의 마지막 시각과 누적 경과다.
    float delta{},scaledDelta{}; // +40/+44: 생성자가 쓰지 않는 현재 시간 조각이다.
    bool first{}; // +48: 공통 최초 시계 준비 여부다.
    float nextEmission{}; // +50: 다음 방출 기한이며 생성 시0이다.
    std::uint32_t source{},stopping{}; // +54/+58: 현재 전역에서 읽은 source DWORD와 중단 DWORD다.
};
// 같은 Client 수명의 현재 전역을 빌릴 수 있는 독립 생성 입력이다.
struct EvilRayLifetimeState {
    std::uint32_t palette{}; // 0055a6f0의 공유 색 객체 토큰이며 개별 소멸 때 반납하지 않는다.
    std::uint32_t processType{10},source{}; // 독립 기본00541098/00595128이며 실제 Session 연결은 별도 단계다.
    const std::uint32_t* sharedProcessType{}; // 등록/부착 전에 읽을 장기 TYPE 주소다.
    const std::uint32_t* sharedSource{}; // 공통 등록/부착 뒤에 읽을 장기 source 주소다.
    const std::uint32_t& ProcessType() const { return sharedProcessType ? *sharedProcessType : processType; } // 현재 DWORD를 원문 인자 위치에서 읽는다.
    const std::uint32_t& Source() const { return sharedSource ? *sharedSource : source; } // SID 축소/범위 검사 없이 현재 전체 DWORD를 읽는다.
};
// 실제 확보와 첫 공유 색 계산의 경계다. 도달하지 않는 장치는 요구하지 않는다.
struct EvilRayLifetimeHooks {
    std::function<bool(std::uint32_t)> reserve; // 입자1200BYTE와 첫 공유 객체8BYTE의 확보 요청이다.
    std::function<std::uint32_t()> createPalette; // 원문00487cb0의 빈 공유 색 객체 토큰을 반환한다.
    std::function<void(std::uint32_t)> buildPalette; // 같은 토큰에 원문 단일3색 띠를 채운다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // Stop의 원문 진단이 정상 복귀할 수 있다.
};
// 원문 공통 소멸자의 실제 입자와 객체 반납을 구분한다.
enum class EvilRayRelease { Particles,Object };
// PATCH004530b0의 실제 RGB 상수로 단일3색 띠를 만든다. 1을 넘는 끝색 성분도 원문대로 유지한다.
std::vector<std::uint32_t> BuildEvilRayPalette(std::span<const std::uint32_t,kParticlePaletteColors> palette);
// 원문 생성/Stop/소멸 몸체다. 실제 Kernel 부착/Frame/Draw 연결과 구별한다.
class RawEvilRayLifetime {
public:
    RawEvilRayLifetime(const SidPool& pool,RawRenderRegistry& registry,EvilRayLifetimeState& state,EvilRayLifetimeHooks hooks); // 같은 PATCH 풀/등록 owner를 요구한다.
    void Initialize(EvilRayFields& fields,std::uint32_t token,Sid parent) const; // TYPE 인자→공통 등록→source→50입자→첫 공유3색 순서다.
    void Stop(EvilRayFields& fields,std::uint32_t mode) const; //00453240의 mode3만 중단하며 다른 mode는 진단 뒤 복귀한다.
    void Destroy(EvilRayFields& fields,std::uint32_t flags,const std::function<void(EvilRayRelease)>& release) const; // 입자→등록/base→bit0 객체 순서다.
private:
    RawRenderRegistry& registry_; // 다른 표시와 공유하는 실제 등록 목록이다.
    EvilRayLifetimeState& state_; // 첫 공유 색과 현재 두 전역을 소유하거나 빌리는 저장소다.
    EvilRayLifetimeHooks hooks_; // 도달 위치에서 선택하는 확보/색/진단 장치다.
};
}
