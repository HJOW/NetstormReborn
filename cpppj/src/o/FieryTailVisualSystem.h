// 복원한 Fiery Tail 몸체를 실제 Host/Kernel과 공통 표시 목록에 연결한다.
#pragma once
#include "o/RawFieryTailLifetime.h"
#include "o/RenderProcessSystem.h"

namespace netstorm::o {
class FieryTailVisualSystem;
// 원본 process11이다. 소유권은 pending에서 실제 Kernel로 한 번만 이동한다.
class FieryTailVisualProcess final : public SquidProcess {
public:
    // system은 이 객체보다 오래 살아야 한다. 생성/부착은 system.Add를 통해 수행한다.
    explicit FieryTailVisualProcess(FieryTailVisualSystem& system);
    // 입자→표시 등록→객체 반납을 실제 원본 몸체로 처리한다.
    ~FieryTailVisualProcess() override;
    // 실제 같은 Kernel이 최초/후속 시간 조각 프레임을 호출한다.
    void RunFrame() override;
    // ProcessForm 삭제 직전의 실제00487960 dirty 통지다. 생성 중이면 파생 필드를 읽지 않는다.
    void OnFormDestroy(std::uint32_t flags) override;
    // 공통 renderer가 현재 부모 화면 끝점으로 실제 선 요청을 제출한다.
    void Draw();
    // 삭제 전까지만 유효한 실제 필드다. 별도 입자 사본을 만들지 않는다.
    const FieryTailFrameFields& Fields() const;
private:
    friend class FieryTailVisualSystem;
    FieryTailVisualSystem& system_; // 실제 몸체/현재 전역/출력 경계의 소유자다.
    FieryTailFrameFields fields_; // 원본 입자·시계·공통 등록의 단일 저장소다.
    bool ready_{}; // 파생 생성 완료 전 Run/Draw/삭제 표시 통지의 재진입을 차단한다.
};
// 게임 계산을 대체하지 않는 실제 출력/확보/부모 좌표 경계다.
struct FieryTailSystemHooks {
    FieryTailLifetimeHooks lifetime; // 실제120입자/첫 공유 색 표 확보 경계다.
    FieryTailFrameHooks frame; // 같은 dirty 큐/선 래스터의 실제 소비자다.
    std::function<std::array<std::int32_t,2>(Sid)> endpoint; // 원본00497220의 현재 부모 화면 좌표다.
    std::function<void(FieryTailRelease)> release; // 실제 반납 순서를 관찰하며 예외/수명 변경을 일으키지 않는다.
};
// endpoint/line은 실제 좌표 조회·래스터 경계이므로 호출 중 효과를 삭제하거나 게임 상태를 진행하지 않는다.
// 같은 공통 registry/Host/Kernel·난수와 실제 Fiery Tail 수명을 묶는다.
class FieryTailVisualSystem {
public:
    // 모든 참조와 hooks가 반환하는 색 span은 이 시스템보다 오래 살아야 한다.
    FieryTailVisualSystem(RenderProcessSystem& rendering,GameRandom& random,FieryTailLifetimeState& lifetime,
        FieryTailFrameState& frame,FieryTailFramePalette& palette,FieryTailSystemHooks hooks);
    // 상위 월드가 아직 Host를 유지할 때 남은 effect Form을 실제 삭제한다.
    ~FieryTailVisualSystem();
    FieryTailVisualSystem(const FieryTailVisualSystem&)=delete; // 콜백의 고정 주소를 유지한다.
    FieryTailVisualSystem& operator=(const FieryTailVisualSystem&)=delete; // 다른 월드로 복사하지 않는다.
    // 표시 등록→실제 client Form 부착→120입자/공유 색 초기화 순서다. 실패하면 등록/Kernel도 정리한다.
    FieryTailVisualProcess* Add(Sid parent);
    // 현재 소유한 process11 수다. 공통 목록의 다른 효과 수와 구별한다.
    std::size_t Size() const;
private:
    friend class FieryTailVisualProcess;
    // 같은 원본 전역인 프레임/수집 배율이 어긋나는 연결을 거부한다.
    void CheckScale() const;
    // 초기화 완료 뒤 실제 프레임 몸체만 호출한다.
    void Run(FieryTailVisualProcess& process);
    // 같은 부모의 현재 화면 끝점을 읽어 실제 Draw에 전달한다.
    void Draw(FieryTailVisualProcess& process);
    // Form 삭제 전 실제 카메라/영역 통지를 보낸다.
    void Removing(const FieryTailVisualProcess& process) const;
    // 원본 소멸 몸체와 두 토큰 조회를 함께 정리한다.
    void Destroy(FieryTailVisualProcess& process);
    RenderProcessSystem& rendering_; // 건설 등 다른 표시와 공유하는 원본 목록이다.
    GameRandom& random_; // 실제 게임 전체와 같은 난수 저장소다.
    FieryTailFrameState& frame_; // 같은 현재 게임 시각/카메라/배율이다.
    FieryTailFramePalette& palette_; // 공유 색 표와 근거 있는 인접 메모리 span이다.
    FieryTailSystemHooks hooks_; // 실제 확보/표시/끝점 경계다.
    RawFieryTailLifetime lifetime_; // 앞 단위에서 원본 대조한 생성/삭제 몸체다.
    std::unordered_map<std::uint32_t,FieryTailVisualProcess*> processes_; // 실제 소유권은 pending 또는 Kernel에 있다.
    std::unique_ptr<FieryTailVisualProcess> pending_; // 공통 등록의 Host 부착 전 소유권이다.
    bool creating_{}; // 생성 콜백의 중첩 Add를 거부한다.
};
}
