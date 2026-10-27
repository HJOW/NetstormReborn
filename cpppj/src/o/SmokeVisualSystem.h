// 냉각 연기의 실제 몸체를 같은 Host/Kernel과 공통 표시 목록에 연결한다.
#pragma once
#include "o/RawSmokeEffect.h"
#include "o/RenderProcessSystem.h"

namespace netstorm::o { // 원본 process21의 표시/소유 수명을 관리한다.
class SmokeVisualSystem; // 각 프로세스보다 오래 사는 실제 처리기/표시 경계다.
// 실제 client Form에 부착되고 Kernel이 소유하는 원본 냉각 연기 프로세스다.
class SmokeVisualProcess final : public SquidProcess {
public:
    // 실제 생성/부착/초기화는 system.AddCooling이 순서대로 수행한다.
    explicit SmokeVisualProcess(SmokeVisualSystem& system);
    ~SmokeVisualProcess() override; // 입자/등록/조회 토큰을 같은 순서로 반납한다.
    void RunFrame() override; // 같은 Kernel 시각으로 실제 Frame을 실행한다.
    void OnFormDestroy(std::uint32_t flags) override; // 삭제 전 현재 영역을 실제 dirty에 통지한다.
    void Draw(); // 현재 공통 renderer 표면에 실제2픽셀 선을 제출한다.
    const SmokeFields& Fields() const; // 삭제 전까지 유효한 단일 실제 의미 필드다.
private:
    friend class SmokeVisualSystem;
    SmokeVisualSystem& system_; // 현재 월드의 실제 수명/출력 경계다.
    SmokeFields fields_; // 복사 프레임 없이 같은 저장소를 계속 갱신한다.
    bool ready_{}; // 생성 중 Frame/Draw/삭제 통지가 미초기화 필드를 읽지 않게 한다.
};
// 동일 allocator/공유 색과 dirty/선 출력의 실제 장치 경계다.
struct SmokeSystemHooks {
    SmokeLifetimeHooks lifetime; //200입자/첫 공유 색의 실제 확보다.
    SmokeFrameHooks frame; // dirty/선 출력이다. kill은 실제 프로세스별 Host로 연결한다.
    std::function<void(SmokeRelease)> release; // 실제 vector/Kernel 반납 순서를 관찰한다.
};
// 같은 원본 registry/Host/Kernel·난수와 연기 생성/프레임/소멸을 묶는다.
class SmokeVisualSystem {
public:
    // 모든 참조와 출력/공유 색 메모리는 이 시스템보다 오래 살아야 한다.
    SmokeVisualSystem(RenderProcessSystem& rendering,GameRandom& random,SmokeLifetimeState& lifetime,
        FieryTailFrameState& frame,FieryTailFramePalette& palette,SmokeSystemHooks hooks);
    ~SmokeVisualSystem(); // 실제 Host가 살아 있을 때 남은 effect Form을 정리한다.
    SmokeVisualSystem(const SmokeVisualSystem&)=delete; // 콜백이 가리키는 고정 주소를 유지한다.
    SmokeVisualSystem& operator=(const SmokeVisualSystem&)=delete; // 다른 월드로 소유권을 복사하지 않는다.
    //004a8770의 현재 SHP 포인트를 받아 실제21·200입자·10회/3간격을 생성한다.
    SmokeVisualProcess* AddCooling(Sid parent,std::array<std::int32_t,2> hotspot);
    std::size_t Size() const; // 현재 소유한 process21 수다.
private:
    friend class SmokeVisualProcess;
    void CheckScale() const; // 공통 수집과 입자가 같은 원본 픽셀 배율을 쓰는지 확인한다.
    SmokeFrameHooks FrameHooks(SmokeVisualProcess& process); // 현재 프로세스의 실제 Host 자기 삭제를 공급한다.
    void Run(SmokeVisualProcess& process); // 준비 완료된 실제 Frame만 실행한다.
    void Draw(SmokeVisualProcess& process); // 준비 완료된 실제 Draw만 실행한다.
    void Removing(const SmokeVisualProcess& process) const; // Form 반납 전 현재 영역을 무효화한다.
    void Destroy(SmokeVisualProcess& process); // 입자/등록/base/조회 수명을 끝낸다.
    RenderProcessSystem& rendering_; // 다른 표시와 공유하는 원본 목록/Host다.
    GameRandom& random_; // 실제 게임 전역 난수다.
    FieryTailFrameState& frame_; // 현재 게임 시각/픽셀 배율/카메라다.
    FieryTailFramePalette& palette_; // 실제12색과 읽을 수 있는 메모리다.
    SmokeSystemHooks hooks_; // 같은 확보/출력/반납 장치다.
    RawSmokeLifetime lifetime_; // 독립 원문과 대조한 실제 생성/소멸 몸체다.
    std::unordered_map<std::uint32_t,SmokeVisualProcess*> processes_; // 소유권은 pending 또는 Kernel에 있다.
    std::unique_ptr<SmokeVisualProcess> pending_; // Host 부착 전의 유일한 소유자다.
    bool creating_{}; // 생성 중 중첩 Add를 거부한다.
};
}
