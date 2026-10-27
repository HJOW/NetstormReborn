// 원본 계산을 바꾸지 않고 실제 공통 등록·Kernel/Host·삭제/Draw 수명을 연결한다.
#include "o/SmokeVisualSystem.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 원본 process21의 실제 표시 수명이다.
namespace { /* 같은 client Form 반납 권한만 내부에 둔다. */ constexpr std::uint32_t kLocalKill=0x10; /* kLocalKill: client Form을 월드 종료 때 실제 반납하는 원본 권한이다. */ }
// 준비 완료 표시는 모든 원본 파생 초기화 뒤에 상위가 세운다.
SmokeVisualProcess::SmokeVisualProcess(SmokeVisualSystem& system):system_(system) {}
// Kernel 소멸에서 같은 입자/등록을 반납한다.
SmokeVisualProcess::~SmokeVisualProcess() { system_.Destroy(*this); }
// 같은 실제 현재 시각의 프레임 몸체로 전달한다.
void SmokeVisualProcess::RunFrame() { system_.Run(*this); }
// Form 소멸 직전 현재 표시 영역을 통지한다.
void SmokeVisualProcess::OnFormDestroy(std::uint32_t) { system_.Removing(*this); }
// 공통 kind3 출력의 실제 파생 Draw다.
void SmokeVisualProcess::Draw() { system_.Draw(*this); }
// 실제 단일 의미 필드만 조회한다.
const SmokeFields& SmokeVisualProcess::Fields() const { return fields_; }
// 같은 월드/장치와 실제 파생 생성 경계를 준비한다.
SmokeVisualSystem::SmokeVisualSystem(RenderProcessSystem& rendering,GameRandom& random,SmokeLifetimeState& lifetime,
    FieryTailFrameState& frame,FieryTailFramePalette& palette,SmokeSystemHooks hooks)
    :rendering_(rendering),random_(random),frame_(frame),palette_(palette),hooks_(std::move(hooks)),lifetime_(rendering.Pool(),rendering.Registry(),lifetime,hooks_.lifetime) {
    if (!hooks_.frame.invalidate || !hooks_.frame.line || !hooks_.release) throw std::invalid_argument("냉각 연기 실제 장치/반납 미연결");CheckScale();
}
// 삭제가 조회 항목을 지우므로 현재 첫 항목을 매번 다시 읽는다.
SmokeVisualSystem::~SmokeVisualSystem() {
    // 상위는 같은 Host가 살아 있을 때 이 시스템을 해제한다.
    while (!processes_.empty()) rendering_.Host().Kill(*processes_.begin()->second,kLocalKill);
}
// 동일 원본 전역이 서로 다른 배율로 복제되지 않게 한다.
void SmokeVisualSystem::CheckScale() const {
    if (frame_.pixelScaleX!=rendering_.State().scaleX || frame_.pixelScaleY!=rendering_.State().scaleY) throw std::logic_error("냉각 연기 공통 표시 배율 불일치");
}
// 공통 등록/실제 부착 뒤200입자/공유 표와 냉각 wrapper의 두 값을 초기화한다.
SmokeVisualProcess* SmokeVisualSystem::AddCooling(Sid parent,std::array<std::int32_t,2> hotspot) {
    if (!parent.value || creating_) throw std::invalid_argument("냉각 연기 실제 부모/중첩 생성 오류");std::uint32_t token=0;creating_=true;
    try {
        pending_=std::make_unique<SmokeVisualProcess>(*this);auto* process=pending_.get();
        token=rendering_.Track(process->fields_.registration,[this](RenderProcessFields& fields,std::uint32_t type,Sid target,std::uint32_t flags) {
            if (!pending_ || &pending_->fields_.registration!=&fields) throw std::logic_error("냉각 연기 부착 객체 불일치");
            auto* attached=rendering_.Host().Attach(std::move(pending_),type,target,flags);if (!attached) throw std::logic_error("냉각 연기 실제 Host 부착 실패");fields.form=attached->Form().value;fields.parent=attached->Parent().value;
        },[process] { process->Draw(); });processes_.emplace(token,process);
        lifetime_.InitializeCooling(process->fields_,token,parent,hotspot);if (!processes_.contains(token)) throw std::logic_error("냉각 연기 생성 중 삭제");process->ready_=true;rendering_.Ready(token);creating_=false;return process;
    } catch (...) {
        if (pending_) pending_.reset();else if (const auto found=processes_.find(token);found!=processes_.end()) rendering_.Host().Kill(*found->second,kLocalKill);creating_=false;throw;
    }
}
// 실제 Frame이 종료 요청을 낼 때 현재 객체를 같은 Host로 삭제한다.
SmokeFrameHooks SmokeVisualSystem::FrameHooks(SmokeVisualProcess& process) {
    auto hooks=hooks_.frame;hooks.kill=[this,&process](std::uint32_t flags) { rendering_.Host().Kill(process,flags); };return hooks;
}
// 자기 삭제 뒤 process 필드를 다시 읽지 않는다.
void SmokeVisualSystem::Run(SmokeVisualProcess& process) {
    if (!process.ready_) return;CheckScale();RawSmokeFrame frame(rendering_.Pool(),random_,frame_,palette_,FrameHooks(process));frame.Step(process.fields_);
}
// 현재 카메라/색 메모리의 실제 출력 요청이다.
void SmokeVisualSystem::Draw(SmokeVisualProcess& process) {
    if (!process.ready_) return;CheckScale();RawSmokeFrame frame(rendering_.Pool(),random_,frame_,palette_,FrameHooks(process));frame.Draw(process.fields_);
}
// 파생 생성 중에는 공통 삭제 통지가 아직 완성되지 않은 필드를 읽지 않는다.
void SmokeVisualSystem::Removing(const SmokeVisualProcess& process) const {
    if (!process.ready_) return;const auto rect=RenderProcessRemovalBounds(process.fields_.registration,frame_.cameraX,frame_.cameraY);hooks_.frame.invalidate({rect[0],rect[1],rect[2],rect[3]});
}
// 실제 입자 반납→공통 목록/base→조회 토큰 제거 순서다.
void SmokeVisualSystem::Destroy(SmokeVisualProcess& process) {
    const auto token=process.fields_.registration.token;if (process.fields_.registration.slot>=0) lifetime_.Destroy(process.fields_,1,hooks_.release);processes_.erase(token);rendering_.Forget(token);
}
// 공통 목록 전체와 다른 연기 소유 수다.
std::size_t SmokeVisualSystem::Size() const { return processes_.size(); }
}
