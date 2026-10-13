// type23 teleportEffect와 type54 사제의 실제 Kernel 프레임/도착/수명 분배를 복원한다.
#pragma once
#include "o/RawPathRuntime.h"
#include "o/RawPathAnimation.h"
#include "o/RawSquidFinder.h"
#include "o/SquidDisplay.h"
#include "o/SquidProcess.h"

namespace netstorm::o {
// 현재 원본 전역을 읽는 경로 상태다. clock은 같은 ProcessHost가 사용하는 상태여야 한다.
struct RawPathProcessState {
    SquidProcessState& clock;
    RawPathObservation observation;
    std::uint32_t teleportProcess{23},priestProcess{54},priestType{158}; // 원본 mutable 프로세스/자산 번호다.
    std::array<std::uint32_t,5> groundTypes{158,137,156,135,187}; // 00427da0의 허용 타입이며 CD는 마지막 empty를 제외한다.
    std::uint32_t blockingGenus{0x10444200}; // 005325b4/CD0053fc28: 도착 칸의 원본 겹침 제거 부류다.
};
// 실제 가상 효과 및 아직 복원되지 않은 큰 하위 몸체의 경계다. 누락을 실행 가능한 빈 효과로 바꾸지 않는다.
struct RawPathProcessHooks {
    std::function<void(Sid,std::uint32_t)> unpop,repop; // 같은 풀/hash/spot의 가상 공간 호출이다.
    std::function<void(Sid,RawPathPoint,std::uint32_t)> pop; // 원본 +0x90의 x/y/flags다.
    std::function<std::uint32_t(Sid)> halt; // 부모 +0x38: 10 계속/11 종료/12 이미 삭제다.
    std::function<std::uint32_t(Sid,std::uint32_t,std::uint32_t)> arrival; // 부모 +0x3c: 1000/1001/1002이며 자신을 지울 수 있다.
    std::function<void(Sid,float,float,std::uint32_t)> stepSound; // 004a9c30/CD00438ae0: 현재 raw 부모 위치·타입 stepSound 요청이다.
    std::function<std::uint32_t(Sid)> soundId; // 현재 타입 +0x6c를 같은 원본 타입 자산에서 읽는다.
    std::function<void(RawPathRuntime&,Sid,RawPathPoint,RawPathPoint,std::uint32_t)> search; // 00488af0→00488370 실제 BFS; 미복원 시 InitializePath에서 진단한다.
    std::function<void(RawPathRuntime&,Sid)> retarget; // 00489ff0 transport 대상 사망 후 실제 운반/재탐색 몸체다.
    std::function<void(Sid,RawPathPoint,std::uint32_t,std::uint32_t)> clearBlocked; // 00443800/CD004d2450의 실제 겹침 정리다.
    std::function<void(RawPathRuntime&,Sid,std::uint32_t)> cleanup; // 0048aa00의 예약·결정·사제 의식·대상 삭제 하위 몸체다.
    std::function<std::uint64_t()> observationStamp; // CD의 진단 시계 읽기이며 게임 now와 별개다.
};
// same pool/hash/spot/Kernel 위에서 원본 이동 프로세스를 실행한다. 별도 좌표/공간 복사본을 만들지 않는다.
class RawPathProcess final:public SquidProcess {
public:
    // 타입 표·프레임·실제 SHP·지도·상태는 프로세스보다 오래 살아야 한다. 생성만으로 부착하지 않는다.
    RawPathProcess(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const RiftTypeFrames> frames,std::span<const SquidDisplayShape> shapes,std::span<const std::uint8_t> spots,
        RawPathProcessState& state,RawPathProcessHooks hooks,bool priest);
    // 원본 destructor는 deadFlag를 켠다. shared watch는 부모 callback이 this를 지운 뒤의 역참조를 막는다.
    ~RawPathProcess() override;
    // 실제 Init +0x20은 void 시작 Pop→실제 경로 탐색→current=0 순서이며 자동 직선 경로는 없다.
    void InitializePath(Sid parent,RawPathPoint destination,RawPathPoint start);
    // +0x30의 현재 speed/turningSpeed 입력을 받는다. override=0이면 현재 타입 속도를 사용한다.
    void Prepare(float speedOverride,float speed,float turningSpeed);
    // type23 0048a950/CD0047efb0 및 type54 0048bb40/CD004805d0의 실제 실행 분기다.
    void RunFrame() override;
    // 도착의 사제 정지 프레임 접두→final Pop0→Halt→가상 도착→1000/1001/1002 종단이다.
    void Arrive();
    // 실제 Form 삭제 통지의 공간 접두와 명시된 정리 하위 몸체를 실행한다.
    void OnFormDestroy(std::uint32_t flags) override;
    // 실제 Host.Attach 완료 뒤 base +4/+8을 경로 이미지에 동기화한다.
    void OnAttached() override;
    // 실제 탐색/건설 시각 조정과 네트워크 어댑터가 같은 경로 저장소를 사용한다.
    RawPathRuntime& Runtime();
    const RawPathRuntime& Runtime() const;
    // 원본 priest Attach는 부모 타입을 현재 priestType 전역과 검증한 뒤 실제 Host에 등록한다.
    static RawPathProcess* Attach(SquidProcessHost& host,std::unique_ptr<RawPathProcess> process,Sid parent,std::uint32_t flags=0,Sid form={});
private:
    // 현재 raw 부모와 프레임을 읽으며 다른 런타임 좌표 캐시를 만들지 않는다.
    RawPathPoint Position(Sid sid) const;
    std::int32_t Frame(Sid sid) const;
    void SetFrame(Sid sid,std::int32_t frame);
    // 부모 가상 Halt의 self-delete/extra/운반 대상 재설정 분기다.
    bool Halt();
    // 원본 +0x28 다음에 조회하는 길 밖 지면/주변 부류 검사다.
    bool GroundStop(RawPathPoint point);
    // spot8의 실제 일반 finder로 막힌 부류를 확인한다.
    bool Blocked(RawPathPoint point) const;
    // callback 뒤에도 살아 있는 같은 Kernel/form/부모인지 확인한다.
    bool Robust() const;
    // 원본 권한 또는 로컬 form 번호 범위다.
    bool MayMutate() const;
    SidPool& pool_;
    const SquidHash& hash_;
    std::span<const RiftTypeRecord> types_;
    std::span<const RiftTypeFrames> frames_;
    std::span<const SquidDisplayShape> shapes_;
    std::span<const std::uint8_t> spots_;
    RawPathProcessState& state_;
    RawPathProcessHooks hooks_;
    bool priest_{};
    RawPathRuntime runtime_;
    std::shared_ptr<bool> alive_=std::make_shared<bool>(true); // 원본 destructor의 stack deadFlag 대체이며 게임 상태를 대체하지 않는다.
};
}
