// Lightning.cpp의 빔 생성/경로 소유와 공통 표시 반납 순서를 복원한다.
#pragma once
#include "o/RawLightningFrame.h"

namespace netstorm::o {
// 원본0055a758의 공유 색 객체다. 다른 효과의0055a750과 구별한다.
struct LightningLifetimeState {
    std::uint32_t palette{}; // 첫 생성 때 채우며 개별 빔의 소멸로 반납하지 않는다.
};
// 원본 외부 메모리 확보와 공유 색 구성의 실제 경계다.
struct LightningLifetimeHooks {
    std::function<bool(std::uint32_t)> reserve; // 입자2880·공유 색 객체8·경로64바이트의 요청이다.
    std::function<std::uint32_t()> createPalette; // 빈 공유 색 객체를 만들고 비영 토큰을 반환한다.
    std::function<void(std::uint32_t)> buildPalette; // 실제8+3색을 같은 공유 객체에 구성한다.
};
// 소멸자가 구별해 반납하는 세 대상이다. 공유 팔레트는 포함하지 않는다.
enum class LightningRelease { Path,Particles,Object };
class LightningVisualProcess; // 실제 빔 객체의 소유권은 Host/Kernel이 유지한다.
// 원본 wrapper의 raw 주소 반환/NULL/생성 성공을 호스트 포인터 혼용 없이 구별한다.
enum class LightningCreateStatus { TargetUnavailable,AllocationFailed,Created };
// raw 주소는 해당 SID 토큰으로 보존하며 생성 성공 때만 process가 유효하다.
struct LightningCreateResult {
    LightningCreateStatus status{LightningCreateStatus::AllocationFailed}; // 실제 wrapper 반환의 종류다.
    Sid skippedTarget{}; // dead/void일 때 원본이 반환한 raw 주소에 해당하는 SID다.
    LightningVisualProcess* process{}; // 성공한104BYTE 원본 객체의 호스트 표현이다.
};
// 00472700/00472690의 생존 검사 뒤에만 실제104BYTE 생성 경계를 호출한다.
// create는 객체 확보 실패를 nullptr로 반환한다. named wrapper는 sound="shortZap.wav"를 전달한다.
LightningCreateResult CreateLightning(const SidPool& pool,Sid parent,Sid target,std::optional<std::string> sound,
    const std::function<LightningVisualProcess*(Sid,Sid,std::optional<std::string>)>& create);
//00472770은현재카메라를signedDWORD감김으로더한뒤104바이트확보/float변환/생성을요청한다.
// create는확보뒤에만전달된정수좌표를float로변환해야한다.실패는nullptr이며음향파일명은shortZap.wav다.
LightningVisualProcess* CreateLightningAtScreen(Sid parent,std::array<std::int32_t,2> from,std::array<std::int32_t,2> to,float life,std::int32_t cameraX,std::int32_t cameraY,
    const std::function<LightningVisualProcess*(Sid,std::array<std::int32_t,2>,std::array<std::int32_t,2>,float,std::string)>& create);
// 실제 생성/삭제 순서와 필드 저장을 실행한다. Host/Kernel 소유권은 호출자가 관리한다.
class RawLightningLifetime {
public:
    // pool/registry/path는 같은10.78 월드여야 하며 모든 참조는 이 객체보다 오래 살아야 한다.
    RawLightningLifetime(const SidPool& pool,RawRenderRegistry& registry,const RawLightningPath& path,
        LightningLifetimeState& state,LightningLifetimeHooks hooks);
    // 등록/부착→120입자→공유 색→깊이−25→현재 부모의 경로→거리×float0.025→파일명 순서다.
    // delta/scaledDelta와 경로 생성자가 쓰지 않는 바이트를 보존한다. 실패한 등록 정리는 호출자 책임이다.
    void Initialize(LightningFrameFields& fields,std::uint32_t token,Sid parent,Sid target,std::optional<std::string> sound) const;
    //00471e70: 실제직접좌표경로/선택수명과파일명이며life가0일때만기본2를보존한다.
    void InitializePoints(LightningFrameFields& fields,std::uint32_t token,Sid parent,std::array<float,2> from,std::array<float,2> to,float life,std::optional<std::string> sound) const;
    // 경로→입자→공통 등록/base→flags bit0 객체 반납 순서다. release는 예외/수명 변경을 일으키지 않아야 한다.
    void Destroy(LightningFrameFields& fields,std::uint32_t flags,const std::function<void(LightningRelease)>& release) const;
private:
    // 두생성자가공유하는실제등록/부착/입자/팔레트/경로확보순서다.
    void InitializeBase(LightningFrameFields& fields,std::uint32_t token,Sid parent) const;
    const SidPool& pool_; // 부착 콜백 뒤의 현재 부모 head를 읽는 같은 풀이다.
    RawRenderRegistry& registry_; // 다른 효과와 공유하는 실제 원본 표시 목록이다.
    const RawLightningPath& path_; // 현재 부모/표적의 실제 생성 시 snapshot이다.
    LightningLifetimeState& state_; // 최초/재사용을 제어하는 월드 공유 상태다.
    LightningLifetimeHooks hooks_; // 메모리/실제 RGB 색 띠 구성의 외부 경계다.
};
}
