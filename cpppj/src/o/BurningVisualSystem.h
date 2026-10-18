// 원본 Burning14를 같은 Host/Kernel/표시 등록과 캡처·난수·출력에 연결한다.
#pragma once
#include "o/RawBurningLifetimeDraw.h"
#include "o/RenderProcessSystem.h"

namespace netstorm::o {
class BurningVisualSystem; // 같은 장기 상태와 단일 소유권을 유지하는 실제 시스템이다.
class BurningVisualProcess final:public SquidProcess {
public:
    explicit BurningVisualProcess(BurningVisualSystem& system); // 파생 초기화는 Add가 실제 부착 후 완료한다.
    ~BurningVisualProcess() override; // 이미지/행/소리/입자/등록 순서로 반납한다.
    void RunFrame() override; // 원본+18 gate를 거쳐 실제+24 Frame을 수행한다.
    void OnFormDestroy(std::uint32_t flags) override; // 원본487960의 현재 dirty 통지다.
    void Notify(std::uint32_t,std::uint32_t,std::uint32_t) override; // 실제+1c=480260 ret12의 빈 통지다.
    const BurningVisualFields& Fields() const; // 현재 단일 실제 필드를 복사 없이 관찰한다.
private:
    friend class BurningVisualSystem;
    BurningVisualSystem& system_; // 이 프로세스보다 오래 사는 현재 몸체/장치 경계다.
    BurningVisualFields fields_; // 실제 등록/150입자/캡처의 단일 소유 저장소다.
    BurningFrameFields frame_; // 생성자가 미정으로 남긴 두float는 실제 timer 호출 뒤에만 소비한다.
    bool ready_{}; // 생성 중 Kernel/Draw/삭제 통지는 미완료 파생 필드를 읽지 않는다.
};
struct BurningSystemHooks {
    BurningVisualHooks visual; // 실제 확보/첫 팔레트 경계다.
    BurningFrameHooks frame; // 같은 원본 SHP 캡처/소리 정지다.
    std::function<void(SquidDisplayRect)> invalidate; // 현재 장치 또는 보존 dirty 큐다.
    std::function<SquidDisplayRect()> clip; // 실제 Draw 동안의 VFX dirty clip이다.
    std::function<void(std::int32_t,std::int32_t,std::uint8_t)> pixel; // 실제8비트 표면 한 픽셀이다.
    std::function<void(BurningRelease)> release; // 반납 관찰이며 저장소 자체는 vector/Kernel이 소유한다.
};
class BurningVisualSystem {
public:
    BurningVisualSystem(RenderProcessSystem& rendering,GameRandom& random,std::span<const RiftTypeRecord> types,
        BurningVisualState& visualState,BurningSourcesState& sourcesState,BurningFrameState& frameState,
        BurningDrawPalette& palette,const std::uint32_t& frameNumber,BurningSystemHooks hooks); // 같은 현재 월드/시각과 필수 장치를 확인한다.
    ~BurningVisualSystem(); // Host가 살아 있을 때 남은 실제 client Form을 정리한다.
    BurningVisualSystem(const BurningVisualSystem&)=delete; // 등록 callback의 고정 주소를 유지한다.
    BurningVisualSystem& operator=(const BurningVisualSystem&)=delete; // 다른 월드에 상태를 복제하지 않는다.
    BurningVisualProcess* Add(Sid victim,std::uint32_t mode); // 실제 모드0/1 wrapper가 같은 생성자를 부른다.
    std::size_t Size() const; // 다른 표시 종류와 구별한 실제14 수다.
private:
    friend class BurningVisualProcess;
    struct Sink final:SquidDisplaySink {
        const BurningSystemHooks& hooks; // 시스템이 소유한 같은 현재 dirty 소비자다.
        explicit Sink(const BurningSystemHooks& supplied):hooks(supplied) {} // 외부 실제 출력 경계를 빌린다.
        bool Suppressed() const override { return false; } // Burning 자체의 두 SetRect는 sink의 억제 전역을 조회하지 않는다.
        void Invalidate(SquidDisplayRect rect,std::uint32_t) override { hooks.invalidate(rect); } // 실제 프레임이 요청한0 flags의 영역이다.
    };
    void Run(BurningVisualProcess& process); // 현재 slot/프레임gate 뒤 같은 원본 Step이다.
    void Draw(BurningVisualProcess& process); // 현재 dirty clip으로 원본5픽셀을 제출한다.
    void Removing(const BurningVisualProcess& process) const; // 실제 삭제 통지의 두 단계 정수화다.
    void Destroy(BurningVisualProcess& process); // 버퍼/등록/단일 조회를 같은 순서로 없앤다.
    RenderProcessSystem& rendering_; // 다른 입자와 공유하는 원본 목록/Host다.
    GameRandom& random_; // 게임 전체가 공유하는 원본 난수다.
    std::span<const RiftTypeRecord> types_; // 현재 HP와 캡처가 읽는 같은 타입 번호다.
    BurningFrameState& frameState_; // 현재 시간/카메라/배율/바람이다.
    BurningDrawPalette& palette_; // 같은 월드의 현재8색 및 인접 DWORD snapshot이다.
    const std::uint32_t& frameNumber_; // 원본55b4a8의 현재DWORD 프레임이다.
    BurningSystemHooks hooks_; // 같은 현재 실제 외부 소비자다.
    Sink sink_; // 실제 Frame의 두 dirty 요청을 전달한다.
    RawBurningVisual visual_; // 원본 생성/Append다.
    RawBurningSources sources_; // 원본 현재HP/캡처 연소점 선택이다.
    std::unordered_map<std::uint32_t,BurningVisualProcess*> processes_; // 소유권은 pending 또는 Kernel에 있다.
    std::unique_ptr<BurningVisualProcess> pending_; // 실제 Host 부착 전 유일한 소유자다.
    bool creating_{}; // 생성 중 같은 시스템 재진입을 명시 거부한다.
};
}
