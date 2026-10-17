// 공유 빔의 실제 수명/프레임을 같은 Host/Kernel과 공통 표시 목록에 연결한다.
#pragma once
#include "o/RawLightningLifetime.h"
#include "o/RenderProcessSystem.h"

namespace netstorm::o {
class LightningVisualSystem;
// 원본 process19다. 소유권은 pending에서 실제 Kernel로 한 번만 이동한다.
class LightningVisualProcess final : public SquidProcess {
public:
    // system은 이 객체보다 오래 살아야 한다. Add만 실제 생성/부착을 수행한다.
    explicit LightningVisualProcess(LightningVisualSystem& system);
    // 경로→입자→등록/base→객체의 실제 소멸 몸체를 실행한다.
    ~LightningVisualProcess() override;
    // 같은 Kernel의 현재 시각/게임 난수로 최초/후속 프레임을 실행한다.
    void RunFrame() override;
    // Form 삭제 직전 현재 표시 영역을 실제 공통 방식으로 무효화한다.
    void OnFormDestroy(std::uint32_t flags) override;
    // 같은 공통 목록이 실제 이전 점/굵기 선을 요청한다.
    void Draw();
    // 삭제 전까지 유효한 단일 의미 상태다. 입자를 복사해 진행하지 않는다.
    const LightningFrameFields& Fields() const;
private:
    friend class LightningVisualSystem;
    LightningVisualSystem& system_; // 몸체/현재 전역/출력 경계의 소유자다.
    LightningFrameFields fields_; // 공통 등록·실제 경로/입자·시계의 단일 저장소다.
    bool ready_{}; // 파생 생성 완료 전 프레임/Draw/삭제 통지 재진입을 차단한다.
};
// 메모리/출력과 현재 raw 중심만 외부 경계다. 계산/게임 수명은 실제 모듈을 사용한다.
struct LightningSystemHooks {
    LightningLifetimeHooks lifetime; // 입자/공유 색 객체/경로의 실제 확보 경계다.
    LightningFrameHooks frame; // dirty/선/파일명 음향 소비자다. removeForm은 시스템이 실제 Host로 연결한다.
    std::function<std::array<float,2>(Sid)> center; // 현재 같은 월드의004adea0 중심 조회다.
    std::function<void(LightningRelease)> release; // 반납 순서를 관찰하며 예외/수명 변경을 일으키지 않는다.
};
// 같은 공통 목록·Host·Kernel과 게임 난수/시간을 사용하는 빔 표시 관리자다.
class LightningVisualSystem {
public:
    // 모든 참조/출력 소비자는 시스템보다 오래 살아야 한다.
    LightningVisualSystem(RenderProcessSystem& rendering,GameRandom& random,LightningLifetimeState& lifetime,
        LightningFrameState& frame,LightningSystemHooks hooks);
    // Host가 살아 있을 때 남은 client Form을 실제 삭제한다.
    ~LightningVisualSystem();
    LightningVisualSystem(const LightningVisualSystem&)=delete; // 콜백이 고정한 시스템 주소를 유지한다.
    LightningVisualSystem& operator=(const LightningVisualSystem&)=delete; // 다른 월드로 복사하지 않는다.
    // 등록→실제 부착→입자/공유 색/경로 초기화 순서다. 실패하면 등록/Kernel도 정리한다.
    LightningVisualProcess* Add(Sid parent,Sid target,std::optional<std::string> sound);
    // 실제104확보뒤화면복원정수양끝을float로변환해직접좌표생성자를실행한다.
    LightningVisualProcess* AddAtScreen(Sid parent,std::array<std::int32_t,2> from,std::array<std::int32_t,2> to,float life,std::optional<std::string> sound);
    // 공통 목록 전체와 구별한 실제 process19 수다.
    std::size_t Size() const;
private:
    friend class LightningVisualProcess;
    // 객체확보→좌표준비→공통등록/부착→선택한실제생성자→ready의공유수명이다.
    LightningVisualProcess* AddImpl(Sid parent,const std::function<void(LightningFrameFields&,std::uint32_t)>& initialize,const std::function<void()>& prepare={});
    // 같은 전역인 프레임/수집 배율을 검사하고 경로 생성 배율에 반영한다.
    void CheckScale();
    // 현재 객체의 소진 요청을 실제 Host.Kill(flags0)에 연결한다.
    LightningFrameHooks FrameHooks(LightningVisualProcess* process);
    // ready 이후 실제 프레임 본체만 실행한다.
    void Run(LightningVisualProcess& process);
    // ready 이후 실제 선 출력 본체만 실행한다.
    void Draw(LightningVisualProcess& process);
    // 공통 Form 삭제 통지의 두 단계 정수화를 사용한다.
    void Removing(const LightningVisualProcess& process) const;
    // 실제 수명 본체와 토큰 조회를 함께 정리한다.
    void Destroy(LightningVisualProcess& process);
    RenderProcessSystem& rendering_; // 모든 효과가 공유하는 현재 표시 목록/Host다.
    GameRandom& random_; // 경로/프레임/게임 계산이 공유하는 난수다.
    LightningFrameState& frame_; // 같은 게임 시각/배율/카메라 전역이다.
    LightningSystemHooks hooks_; // 메모리/실제 출력/중심 경계다.
    LightningPathState pathState_; // 같은 배율과 원본 보정 타입 전역이다.
    RawLightningPath path_; // 실제 snapshot/재귀/진행 몸체다.
    RawLightningLifetime lifetime_; // 원본 생성·공유 색·경로/입자 반납 몸체다.
    std::unordered_map<std::uint32_t,LightningVisualProcess*> processes_; // 소유권은 pending 또는 Kernel에 있다.
    std::unique_ptr<LightningVisualProcess> pending_; // 실제 부착 전 임시 소유권이다.
    bool creating_{}; // 생성 콜백의 중첩 Add를 거부한다.
};
}
