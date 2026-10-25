// 같은 물리의 ProcessState/Host/Kernel과 짧은 현재 World 처리기 대여를 분리한다.
#pragma once
#include "o/GameClock.h"
#include "o/SquidProcess.h"
#include <memory>

namespace netstorm::client { // 실행 모듈을 복제하지 않고 실제 공통 Host의 수명만 소유한다.
class RawProcessHostStorage {
public:
    class ClearPass;class Returned; // 실제 단일 Clear 정상 반환과 대여 반납을 구별하는 이동 전용 증빙이다.
    class Use { // 현재 World 처리기만 빌리며 실제 반납 증빙의 발급 위치를 소유한다.
    public:
        ~Use(); // 현재 자기 처리기만 반납하며 Kernel을 실행하거나 비우지 않는다.
        Use(const Use&)=delete;Use& operator=(const Use&)=delete; // 단일 현재 바인딩을 복제하지 않는다.
        void DrainOwnedForShutdown(); // 현재 World의 생성 실패/일반 종료에서 자기 대여가 만든 객체만 반납한다. 이전 Native 잔류는 건드리지 않는다.
        ClearPass ClearForState(); // 실제 같은 Kernel.Clear 한 번이 정상 복귀했을 때만 증빙을 만든다.
        Returned ReturnForState(ClearPass&& pass); // 같은 대여의 증빙과 현재 잔류 종류를 확인하고 실제 바인딩을 반납한다.
    private:
        friend class RawProcessHostStorage;
        Use(RawProcessHostStorage& owner,std::shared_ptr<o::ProcessHostRuntimeBindings> runtime); // 검증한 현재 World만 등록한다.
        RawProcessHostStorage* owner_; // 반납 시nullptr로 소비하는 실제 대여 소유권이다.
        std::shared_ptr<o::ProcessHostRuntimeBindings> runtime_; // old World 캡처는 이 짧은 대여에만 존재한다.
    };
    class ClearPass { // 같은 실제 Kernel.Clear 정상 복귀의 이동 전용 증빙이다.
    public:
        ClearPass(ClearPass&&) noexcept=default;ClearPass& operator=(ClearPass&&) noexcept=default; // 정상 복귀 증빙의 소유권만 이동한다.
        ClearPass(const ClearPass&)=delete;ClearPass& operator=(const ClearPass&)=delete; // 같은 단계를 중복 반납하지 않는다.
    private:
        friend class Use;
        ClearPass(RawProcessHostStorage* owner,std::weak_ptr<o::ProcessHostRuntimeBindings> runtime) noexcept; // Use의 실제 Clear 정상 복귀에서만 발급한다.
        RawProcessHostStorage* owner_;std::weak_ptr<o::ProcessHostRuntimeBindings> runtime_; // 같은 물리/대여 정체성을 검사한다.
    };
    class Returned { // 현재 처리기 반납 뒤 Kernel을 pin하는 이동 전용 결과다.
    public:
        Returned(Returned&&) noexcept=default;Returned& operator=(Returned&&) noexcept=default; // 소비한 실제 반납 증빙만 이동한다.
        Returned(const Returned&)=delete;Returned& operator=(const Returned&)=delete; // cleanupDone 같은 공개 성공 setter를 만들지 않는다.
    private:
        friend class Use;
        explicit Returned(std::shared_ptr<o::Kernel> kernel) noexcept; // 실제 대여가 끝난 뒤 같은 Kernel 소유권을 유지한다.
        std::shared_ptr<o::Kernel> kernel_; // 잔류가 old World보다 오래 살아도 Kernel 자체가 먼저 사라지지 않는다.
    };
    RawProcessHostStorage(o::SidPool& pool,std::span<const o::RiftTypeRecord> types,std::shared_ptr<o::RawAuthorityState> authority,std::shared_ptr<std::uint32_t> pollType={}); // Pool/TYPE는 이 owner보다 오래 살아야 한다.
    ~RawProcessHostStorage(); // 최종 물리 소멸에서만 잔류를 반납하며 Host/Pool은 그때까지 유지한다.
    RawProcessHostStorage(const RawProcessHostStorage&)=delete;RawProcessHostStorage& operator=(const RawProcessHostStorage&)=delete; // Host가 캡처한 주소를 고정한다.
    RawProcessHostStorage(RawProcessHostStorage&&)=delete;RawProcessHostStorage& operator=(RawProcessHostStorage&&)=delete; // 실제 Host가 캡처한 owner 주소를 이동하지 않는다.
    void BindDevices(std::shared_ptr<o::Kernel> kernel,std::shared_ptr<o::FrameTime> clock,std::shared_ptr<std::uint32_t> pollType={}); // 같은 실제 Client 장치를 pin하며 Session 지역 주소는 받지 않는다.
    bool UsesKernel(const o::Kernel* kernel) const; // raw legacy 입력이 이미 pin한 같은 실제 장치인지 조회한다.
    bool HasDevices() const; // 명시 장기 외부 장치가 연결됐는지 읽는다.
    o::Kernel& Kernel(); // 외부 장치가 없으면 같은 Storage의 독립 Kernel을 한 번만 만든다.
    o::SquidProcessHost& Host(); // 첫 사용에 안정된 Host를 만들고 같은 주소를 유지한다.
    o::SquidProcessState& State(); // 전체 World/Regular가 같은 실제 스칼라 저장소를 빌린다.
    std::unique_ptr<Use> Bind(std::shared_ptr<o::ProcessHostRuntimeBindings> runtime); // 중복/다른 Pool은 현재 바인딩을 바꾸기 전에 거부한다.
private:
    o::SidPool& pool_;std::span<const o::RiftTypeRecord> types_; // 실제 물리 배열은 외부 Storage가 소유한다.
    std::shared_ptr<o::RawAuthorityState> authority_; // 현재 두 권한 bool과 같은 긴 수명이다.
    o::SquidProcessState state_; // now fallback/typeRotation/pollType와 실제 외부 참조를 한 번 소유한다.
    std::weak_ptr<o::ProcessHostRuntimeBindings> current_; // 현재 짧은 대여만 조회하며 World를 숨겨서 붙잡지 않는다.
    std::unique_ptr<o::SquidProcessHost> host_; // Kernel의 최종 소멸보다 오래 살아 있어야 한다.
    std::shared_ptr<o::FrameTime> clock_;std::shared_ptr<std::uint32_t> pollType_; // 스칼라 포인터의 실제 소유자를 pin한다.
    std::shared_ptr<o::Kernel> kernel_; // Host보다 먼저 반납하며 독립 Kernel도 잔류보다 먼저 사라지지 않는다.
    bool external_{}; // 같은 명시 외부 장치와 독립 최종 종료 정책만 구별한다.
};
}
