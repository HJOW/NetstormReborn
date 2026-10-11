// 복원한 건설 표시 몸체를 실제 ProcessForm·Kernel·표시 목록의 소유권에 연결한다.
#pragma once
#include "o/RawConstructionFrame.h"
#include "o/RawConstructionLifetime.h"
#include "o/SquidProcess.h"
#include <unordered_map>

namespace netstorm::o {
class ConstructionVisualSystem;
// Kernel이 소유하는 건설 표시 프로세스다. 생성 중에는 실행/출력을 건너뛴다.
class ConstructionVisualProcess final : public SquidProcess {
public:
    // system은 이 객체보다 오래 살아야 한다. 생성과 부착은 system.Add를 통해 수행한다.
    explicit ConstructionVisualProcess(ConstructionVisualSystem& system);
    // 캡처/출력/입자와 표시 등록을 원본 순서로 반납한다. form 반납은 host가 담당한다.
    ~ConstructionVisualProcess() override;
    // 실제 Kernel 호출을 복원한 프레임 몸체에 전달한다. 자기 삭제 뒤 멤버를 읽지 않는다.
    void RunFrame() override;
    // 클라이언트 출력 단계에서 현재 출력 버퍼를 제출한다.
    void Draw() const;
    // 관찰용 프레임/등록 필드다. 삭제 뒤 반환 참조를 사용해서는 안 된다.
    const ConstructionFrameFields& Fields() const;
    const RenderProcessFields& Registration() const;
private:
    friend class ConstructionVisualSystem;
    ConstructionVisualSystem& system_; // 몸체와 표시 목록의 수명 관리자다.
    ConstructionFrameFields fields_; // 이 프로세스가 소유하는 픽셀/입자/시각이다.
    RenderProcessFields registration_; // 원본 RenderProcess 등록 필드다.
    bool ready_{}; // 생성 몸체가 끝난 뒤에만 Run/Draw를 허용한다.
};
// 원본 복원 몸체를 묶는 호스트 어댑터다. 장치 출력/클라이언트 깊이 정렬은 외부 단계다.
class ConstructionVisualSystem {
public:
    // host/풀/전역/색 표/난수는 system보다 오래 살아야 한다. 빈 attach/kill 경계는 내부 연결로 대체한다.
    // release는 관찰만 하고 예외나 수명 변경을 일으키지 않아야 한다. 나머지 경계도 반환 span을 보관하지 않는다.
    ConstructionVisualSystem(SidPool& pool,SquidProcessHost& host,ConstructionVisualState& visualState,
        ConstructionFrameState& frameState,const ConstructionFramePalette& palette,GameRandom& random,
        RenderRegistryHooks registryHooks,ConstructionVisualHooks visualHooks,ConstructionFrameHooks frameHooks,
        std::function<void(ConstructionRelease)> release);
    // 남은 표시 form을 host를 통해 지워 Kernel에 system 참조가 남지 않게 한다.
    ~ConstructionVisualSystem();
    // 저장한 콜백과 프로세스의 참조가 주소를 사용하므로 system을 복사/이동하지 않는다.
    ConstructionVisualSystem(const ConstructionVisualSystem&)=delete;
    ConstructionVisualSystem& operator=(const ConstructionVisualSystem&)=delete;
    // 생성 중 목록 등록→실제 client form 부착→캡처/입자 초기화 순서다. 경계에서 중첩 Add는 거부한다.
    // 실패하면 생성 중인 form/Kernel/표시 등록을 정리한다. 반환 포인터는 자기 종료/부모 삭제 때 무효다.
    ConstructionVisualProcess* Add(Sid parent,bool instant);
    // 현재 표시 슬롯의 순서로 Draw를 제출한다. 원본 클라이언트의 깊이 정렬/가시 영역 선별 복원은 후속이다.
    void Draw() const;
    // 목록의 구멍과 현재 소유 프로세스 수를 읽는다. Slots의 span은 Add의 확장 때 무효다.
    std::span<const std::uint32_t> Slots() const;
    std::size_t Size() const;
    // 같은 월드의 건설 시작에 연결할 때 풀을 확인한다.
    const SidPool& Pool() const;
private:
    friend class ConstructionVisualProcess;
    // registry 생성자의 attach를 실제 host 부착으로 대체한다. 목록 등록은 이미 끝난 상태다.
    RenderRegistryHooks RegistryHooks(RenderRegistryHooks hooks);
    // 한 프레임만 살아 있는 kill 콜백을 만든다. 프로세스 자체에 자기 삭제 콜백을 저장하지 않는다.
    ConstructionFrameHooks FrameHooks(ConstructionVisualProcess& process) const;
    // Run 뒤 살아 있는 프로세스에만 현재 사각형/깊이를 등록 필드로 돌려 쓴다.
    void Run(ConstructionVisualProcess& process);
    // 한 객체의 Draw를 수행한다. draw 경계가 객체를 지워도 이후 객체를 읽지 않는다.
    void DrawProcess(const ConstructionVisualProcess& process) const;
    // 실제 객체 소멸 시 소유 버퍼/등록과 토큰 조회를 함께 정리한다.
    void Destroy(ConstructionVisualProcess& process);
    SidPool& pool_; // host와 공유하는 raw 풀이다.
    SquidProcessHost& host_; // client form 부착/공통 삭제를 수행한다.
    ConstructionVisualState& visualState_; // 타입/공유 팔레트/좌표 배율이다.
    ConstructionFrameState& frameState_; // 매 프레임 현재 시각/카메라다.
    const ConstructionFramePalette& palette_; // 출력 색 표다.
    GameRandom& random_; // 생성과 프레임이 공유하는 난수다.
    ConstructionVisualHooks visualHooks_; // 캡처/팔레트/확보 경계다.
    ConstructionFrameHooks frameHooks_; // 최대 HP/dirty/장치 경계다.
    std::function<void(ConstructionRelease)> release_; // 원본 반납 순서의 관찰자다.
    RawRenderRegistry registry_; // 표시 토큰의 원본 슬롯 목록이다.
    std::unordered_map<std::uint32_t,ConstructionVisualProcess*> processes_; // 소유권은 pending 또는 Kernel에 있다.
    std::unique_ptr<ConstructionVisualProcess> pending_; // 상위 부착 전 생성 중인 객체의 소유권이다.
    std::uint32_t nextToken_{1}; // 호스트 포인터 대신 부여하는 비영 식별자다. 감김은 거부한다.
    bool creating_{}; // 콜백 중 중첩 생성을 방지한다.
};
// 건설 Start의 표시 효과를 실제 표시 프로세스 생성에 연결한다. system은 hooks보다 오래 살아야 한다.
ConstructionProcessHooks MakeConstructionVisualSystemHooks(const SidPool& pool,ConstructionVisualSystem& system,
    ConstructionProcessHooks hooks);
}
