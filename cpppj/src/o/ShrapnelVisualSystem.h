// 실제 process16/더미 상호 의존과 픽셀 프레임을 같은 Host/Kernel에 연결한다.
#pragma once
#include "o/RawShrapnelLifetime.h"
#include "o/RenderProcessSystem.h"

namespace netstorm::o {
class ShrapnelVisualSystem;
// 파편 소유권은 생성 중 pending에서 실제 Kernel로 한 번만 이동한다.
class ShrapnelVisualProcess final : public SquidProcess {
public:
    // system이 이 객체보다 오래 살아야 한다.
    explicit ShrapnelVisualProcess(ShrapnelVisualSystem& system);
    // 실제 bitmap/입자/표시 등록을 정리한다.
    ~ShrapnelVisualProcess() override;
    // 같은 Kernel 시각의 실제 중력/수명 Frame이다.
    void RunFrame() override;
    // 공통 RenderProcess의 현재 삭제 영역을 먼저 통지한다.
    void OnFormDestroy(std::uint32_t flags) override;
    // 실제 캡처 색의 한 픽셀 출력이다.
    void Draw();
    // 삭제 전까지 유효한 단일 상태를 읽는다.
    const ShrapnelFrameFields& Fields() const;
private:
    friend class ShrapnelVisualSystem;
    ShrapnelVisualSystem& system_; // 같은 실제 계산/표시 경계의 소유자다.
    ShrapnelFrameFields fields_; // 표시 등록/bitmap/입자/시각의 단일 저장소다.
    Sid dummy_; // 부착 전 예외에도 소유한 dummy를 찾는다.
    bool paired_{},ready_{}; // 실제 의존 쌍/파생 초기화가 완료됐는지 구별한다.
};
// 수명/메모리/픽셀은 실제 소비자이며 생성/Frame의 계산을 대체하지 않는다.
struct ShrapnelSystemHooks {
    ShrapnelLifetimeHooks lifetime; // create/pop/capture/depend/reserve이며 attach/prepare는 시스템이 실제 연결한다.
    ShrapnelFrameHooks frame; // reserve/release/dirty/pixel이며 소진은 실제 Host에 연결한다.
    std::function<void(ShrapnelRelease)> release; // 반납 관찰 경계다. 저장소는 실제 수명 몸체가 반납한다.
    std::function<void(Sid)> destroyUnpaired; // 실제 의존 쌍 부착 전 실패한 dummy의 가상 삭제다.
};
// 기존 모든 효과와 같은 표시 registry/Host/게임 난수/CRT 스레드 난수를 사용한다.
class ShrapnelVisualSystem {
public:
    // 참조/실제 출력 경계는 시스템보다 오래 살아야 한다.
    ShrapnelVisualSystem(RenderProcessSystem& rendering,GameRandom& game,CrtRandom& crt,ShrapnelFrameState& frame,ShrapnelSystemHooks hooks);
    // Host가 살아 있을 때 잔여 client Form을 정리한다.
    ~ShrapnelVisualSystem();
    ShrapnelVisualSystem(const ShrapnelVisualSystem&)=delete; // callback의 고정 주소를 유지한다.
    ShrapnelVisualSystem& operator=(const ShrapnelVisualSystem&)=delete; // 다른 월드로 복사하지 않는다.
    // 실제104바이트 실패는nullptr이며 성공은 dummy와 같은 Kernel의 process16이다.
    ShrapnelVisualProcess* Add(Sid source,ShrapnelKind kind);
    // 공통 표시 전체와 구별한 현재 파편 수다.
    std::size_t Size() const;
private:
    friend class ShrapnelVisualProcess;
    // 동일한 전역 배율 계약을 검사한다.
    void CheckScale() const;
    // 같은 현재 객체의 실제 부착/계산 경계를 만든다.
    ShrapnelLifetimeHooks LifetimeHooks(ShrapnelVisualProcess& process);
    // 자기 소진을 실제 Host.Kill(flags0)에 연결한다.
    ShrapnelFrameHooks FrameHooks(ShrapnelVisualProcess* process);
    // 파생 초기화 완료 뒤 실제 Frame/Draw/삭제 영역을 실행한다.
    void Run(ShrapnelVisualProcess& process);
    void Draw(ShrapnelVisualProcess& process); // 현재 bitmap 색/현재 화면 경계를 사용한다.
    void Removing(const ShrapnelVisualProcess& process) const; // 공통 표시의 정확한 정수화다.
    void Destroy(ShrapnelVisualProcess& process); // 실제 반납/목록/조회와 부착 전 실패 dummy 정리다.
    RenderProcessSystem& rendering_; // 단일 원본 목록과 실제 Host다.
    GameRandom& game_; // 게임 전체가 공유하는 난수다.
    CrtRandom& crt_; // 원본 TLS와 같은 별도 스트림이다.
    ShrapnelFrameState& frame_; // 현재 시각/카메라/16·11 배율이다.
    ShrapnelSystemHooks hooks_; // 실제 세계/bitmap/메모리/표시 경계다.
    std::unordered_map<std::uint32_t,ShrapnelVisualProcess*> processes_; // 실제 소유권은 pending/Kernel에 있다.
    std::unique_ptr<ShrapnelVisualProcess> pending_; // 실제 부착 전 임시 소유권이다.
    bool creating_{}; // 중첩 Add가 같은 pending을 바꾸지 못하게 한다.
};
}
