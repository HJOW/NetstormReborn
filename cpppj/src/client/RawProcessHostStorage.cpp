// 실제 단일 Kernel.Clear와 호스트 최종 소멸을 구분하며 현재 World의 함수만 짧게 빌린다.
#include "client/RawProcessHostStorage.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client { // Client와 독립 Storage가 동일한 장기 Host 경로를 사용한다.
namespace { // 실제 Kernel.Clear의 가상 소멸 효과가 진행되는 한정 호출 수명이다.
struct ClearCall {
    std::shared_ptr<o::ProcessHostRuntimeBindings> runtime; // 같은 대여를 실제 Clear 정상/예외 복귀까지 소유한다.
    explicit ClearCall(std::shared_ptr<o::ProcessHostRuntimeBindings> value):runtime(std::move(value)) { ++runtime->activeCalls; } // 중첩 반납이 진행 중 호출을 소비하지 못하게 한다.
    ~ClearCall() { --runtime->activeCalls; } // 실제 Kernel.Clear 호출이 끝난 뒤에만 반납을 허용한다.
};
}
RawProcessHostStorage::RawProcessHostStorage(o::SidPool& pool,std::span<const o::RiftTypeRecord> types,std::shared_ptr<o::RawAuthorityState> authority,std::shared_ptr<std::uint32_t> pollType)
    :pool_(pool),types_(types),authority_(std::move(authority)),state_{.sharedAuthority=authority_.get()},pollType_(std::move(pollType)) { state_.sharedPollType=pollType_.get(); } // 권한의 원본 주소를 직접 빌린다.
// 게임 중 Native 효과는 이 경로를 사용하지 않는다. 마지막 물리 소유권만 실제 프로세스를 반납한다.
RawProcessHostStorage::~RawProcessHostStorage() {
    if (kernel_ && host_) {
        if (!external_) { do { kernel_->Clear(); } while (kernel_->Size()); } // 독립 Kernel의 최종 종료는 Host/Pool이 살아 있는 이 위치에서 수행한다.
        else { bool removed;do { removed=false;for (o::ProcessId id=1;id<=kernel_->Capacity();++id) { const auto* process=kernel_->Get(id);if (process && host_->Owns(*process)) { kernel_->Remove(id);removed=true; } } } while (removed); } // 다른 Storage/Client 소유 프로세스를 자기 최종 소멸로 지우지 않는다.
    }
    kernel_.reset();host_.reset(); // 실제 반납 뒤에만 Host를 소멸시킨다.
}
// 최초 명시 장치는 실제 같은 Client 객체를 pin하며 바인딩 이후 다른 장치로 교체하지 않는다.
void RawProcessHostStorage::BindDevices(std::shared_ptr<o::Kernel> kernel,std::shared_ptr<o::FrameTime> clock,std::shared_ptr<std::uint32_t> pollType) {
    if (!pollType) pollType=pollType_; // 명시 미지정은 기존 같은 장기 TYPE owner를 보존한다.
    if (external_ && kernel_==kernel && clock_==clock && pollType_==pollType) return; // 실제 같은 주소의 재확인은 대여/값을 변경하지 않는다.
    if (!kernel || !clock || !current_.expired() || (kernel_ && (kernel_!=kernel || clock_!=clock || pollType_!=pollType))) throw std::invalid_argument("ProcessHost의 같은 장기 Kernel/시각 장치 계약");
    kernel_=std::move(kernel);clock_=std::move(clock);pollType_=std::move(pollType);external_=true;state_.sharedNow=&clock_->game;state_.sharedPollType=pollType_ ? pollType_.get() : nullptr;
}
bool RawProcessHostStorage::UsesKernel(const o::Kernel* kernel) const { return kernel_ && kernel_.get()==kernel; } // 현재 실제 Kernel 주소만 비교한다.
bool RawProcessHostStorage::HasDevices() const { return external_; } // 대여를 만들거나 상태를 복사하지 않는다.
o::Kernel& RawProcessHostStorage::Kernel() { if (!kernel_) kernel_=std::make_shared<o::Kernel>(pool_.Edition()==o::OriginalEdition::Patch1078 ? o::kProcessCapacity : o::kCdProcessCapacity);return *kernel_; } // 독립 판본의 실제 capacity를 유지한다.
o::SquidProcessHost& RawProcessHostStorage::Host() {
    if (!host_) host_=std::make_unique<o::SquidProcessHost>(pool_,types_,Kernel(),state_,[this] { return current_.lock(); });return *host_;
} // 현재 World가 없어도 안정된 Host 자체를 소유할 수 있다.
o::SquidProcessState& RawProcessHostStorage::State() { return state_; } // 같은 scalar와 현재 외부 참조다.
std::unique_ptr<RawProcessHostStorage::Use> RawProcessHostStorage::Bind(std::shared_ptr<o::ProcessHostRuntimeBindings> runtime) {
    if (!runtime || !runtime->destroy || &runtime->destroy->Pool()!=&pool_ || !current_.expired()) throw std::invalid_argument("ProcessHost의 실제 현재 World 단일 대여 계약");
    static_cast<void>(Host());return std::unique_ptr<Use>(new Use(*this,std::move(runtime)));
} // 모든 거부는 기존 current를 변경하기 전에 수행한다.
RawProcessHostStorage::Use::Use(RawProcessHostStorage& owner,std::shared_ptr<o::ProcessHostRuntimeBindings> runtime):owner_(&owner),runtime_(std::move(runtime)) { owner.current_=runtime_; } // 실제 대여가 현재 처리기를 공개한다.
RawProcessHostStorage::Use::~Use() { if (owner_ && owner_->current_.lock()==runtime_) owner_->current_.reset(); } // 다른 바인딩의 현재 슬롯은 반납하지 않는다.
RawProcessHostStorage::ClearPass::ClearPass(RawProcessHostStorage* owner,std::weak_ptr<o::ProcessHostRuntimeBindings> runtime) noexcept:owner_(owner),runtime_(std::move(runtime)) {} // private 정상복귀 증빙 생성자다.
RawProcessHostStorage::Returned::Returned(std::shared_ptr<o::Kernel> kernel) noexcept:kernel_(std::move(kernel)) {} // 잔류가 소유된 같은 Kernel만 유지한다.
void RawProcessHostStorage::Use::DrainOwnedForShutdown() { if (owner_ && owner_->current_.lock()==runtime_) owner_->Host().DrainRuntimeBindings(runtime_); } // 생성 실패도 이전 Native 잔류의 다른 대여를 소멸시키지 않는다.
RawProcessHostStorage::ClearPass RawProcessHostStorage::Use::ClearForState() {
    if (!owner_ || owner_->current_.lock()!=runtime_) throw std::logic_error("현재 ProcessHost 대여 없는 Native Clear");
    ClearCall call(runtime_);owner_->Kernel().Clear();return ClearPass(owner_,runtime_);
} // 예외이면 증빙을 발급하지 않으며 이미 수행된 부분 효과는 되돌리지 않는다.
RawProcessHostStorage::Returned RawProcessHostStorage::Use::ReturnForState(ClearPass&& pass) {
    if (!owner_ || pass.owner_!=owner_ || pass.runtime_.lock()!=runtime_ || owner_->current_.lock()!=runtime_) throw std::invalid_argument("Native Clear의 같은 ProcessHost 대여 증빙 필요");
    if (runtime_->activeCalls) throw std::logic_error("진행 중 ProcessHost 효과의 현재 대여 반납 금지");
    if (!owner_->Host().HasOnlyRegularProcesses()) throw std::logic_error("Regular 이외 잔류의 현재 모듈 수명 이행 미완료");
    auto kernel=owner_->kernel_;owner_->current_.reset();owner_=nullptr;runtime_.reset();pass.owner_=nullptr;pass.runtime_.reset();return Returned(std::move(kernel));
} // 실제 현재 대여를 소비한 뒤에만 반납 증빙을 만든다. 잔류 PID에는 손대지 않는다.
}
