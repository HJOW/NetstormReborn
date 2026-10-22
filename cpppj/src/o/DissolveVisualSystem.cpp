// parent0 등록과 뒤의 수동 부착을 합치지 않고 실제 공통 수명에 연결한다.
#include "o/DissolveVisualSystem.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// client Form의 실패/종료 정리 권한이다.
constexpr std::uint32_t kClientKill=16;
}
// 파생 준비는 Add의 마지막에만 완료한다.
DissolveVisualProcess::DissolveVisualProcess(DissolveVisualSystem& system):system_(system) {}
// 원본 메모리/등록 순서의 실제 소멸 몸체다.
DissolveVisualProcess::~DissolveVisualProcess() { system_.Destroy(*this); }
// 별도 프레임 목록/시계를 두지 않는다.
void DissolveVisualProcess::RunFrame() { system_.Run(*this); }
// 현재 영역을 메모리 반납보다 먼저 통지한다.
void DissolveVisualProcess::OnFormDestroy(std::uint32_t) { system_.Removing(*this); }
// 공통 kind3이 현재 픽셀을 출력한다.
void DissolveVisualProcess::Draw() { system_.Draw(*this); }
// 입자를 복사하지 않는 읽기 조회다.
const DissolveFrameFields& DissolveVisualProcess::Fields() const { return fields_; }
// 세계 경계와 실제 Frame 출력의 누락을 첫 등록 전에 확인한다.
DissolveVisualSystem::DissolveVisualSystem(RenderProcessSystem& rendering,GameRandom& game,ShrapnelFrameState& frame,DissolveSystemHooks hooks)
    :rendering_(rendering),game_(game),frame_(frame),hooks_(std::move(hooks)) {
    if (!hooks_.lifetime.reserve || !hooks_.lifetime.create || !hooks_.lifetime.pop || !hooks_.lifetime.depend || !hooks_.lifetime.capture || !hooks_.release || !hooks_.destroyUnpaired) throw std::invalid_argument("분해 실제 세계/반납 경계 누락");CheckScale();
    const RawDissolveFrame checked(rendering_.Pool(),game_,frame_,FrameHooks(nullptr));
}
// 삭제가 현재 조회를 지우므로 매번 첫 객체를 다시 찾는다.
DissolveVisualSystem::~DissolveVisualSystem() {
    // 같은 실제 Host가 이 시스템보다 오래 살아야 한다.
    while (!processes_.empty()) rendering_.Host().Kill(*processes_.begin()->second,kClientKill);
}
// 서로 다른 좌표 전역으로 조용히 출력하지 않는다.
void DissolveVisualSystem::CheckScale() const {
    if (frame_.pixelScaleX!=rendering_.State().scaleX || frame_.pixelScaleY!=rendering_.State().scaleY) throw std::logic_error("분해 공통 표시 배율 불일치");
}
// 같은 pending 객체를 실제 Host에 넘기고 원본 prepare를 연결한다.
DissolveLifetimeHooks DissolveVisualSystem::LifetimeHooks(DissolveVisualProcess& process) {
    auto hooks=hooks_.lifetime;hooks.attach=[this,&process](RenderProcessFields& fields,std::uint32_t type,Sid parent,std::uint32_t flags) {
        if (!pending_ || pending_.get()!=&process || &fields!=&process.fields_.common.registration) throw std::logic_error("분해 실제 부착 객체 불일치");
        auto* attached=rendering_.Host().Attach(std::move(pending_),type,parent,flags);if (!attached) throw std::logic_error("분해 실제 dummy 부착 실패");fields.form=attached->Form().value;fields.parent=attached->Parent().value;
    };
    hooks.depend=[this,&process](Sid form,Sid dummy) { hooks_.lifetime.depend(form,dummy);process.paired_=true; };
    hooks.prepare=[this,&process](DissolveFrameFields& fields,Sid first,Sid second,std::int32_t xOriented) { RawDissolveFrame logic(rendering_.Pool(),game_,frame_,FrameHooks(&process));logic.Initialize(fields,first,second,xOriented); };return hooks;
}
// 실제 Frame의 소진은 같은 Form/Kernel/의존 쌍 삭제로 이어진다.
ShrapnelFrameHooks DissolveVisualSystem::FrameHooks(DissolveVisualProcess* process) {
    auto hooks=hooks_.frame;hooks.removeForm=[this,process] { if (!process) throw std::logic_error("분해 소진 객체 미지정");rendering_.Host().Kill(*process,0); };return hooks;
}
// 공통 목록에 먼저 보인 뒤 실제 dummy/부착/캡처/준비를 수행하고 ready를 마지막에 세운다.
DissolveVisualProcess* DissolveVisualSystem::Add(Sid source,Sid first,Sid second,std::int32_t xOriented) {
    if (!source.value || creating_) throw std::invalid_argument("분해 source/중첩 생성 오류");CheckScale();if (!hooks_.lifetime.reserve(120)) return nullptr;
    creating_=true;std::uint32_t token=0;
    try {
        pending_=std::make_unique<DissolveVisualProcess>(*this);auto* process=pending_.get();auto lifetimeHooks=LifetimeHooks(*process);
        token=rendering_.Track(process->fields_.common.registration,lifetimeHooks.attach,[process] { process->Draw(); });processes_.emplace(token,process);
        RawDissolveLifetime lifetime(rendering_.Pool(),rendering_.Registry(),std::move(lifetimeHooks));lifetime.Initialize(process->fields_,token,source,first,second,xOriented,process->dummy_);
        if (!processes_.contains(token)) throw std::logic_error("분해 생성 중 현재 객체 삭제");process->ready_=true;rendering_.Ready(token);creating_=false;return process;
    } catch (...) {
        if (pending_) pending_.reset();else if (const auto found=processes_.find(token);found!=processes_.end()) rendering_.Host().Kill(*found->second,kClientKill);
        creating_=false;throw;
    }
}
// 자기 삭제 뒤 같은 객체를 다시 읽지 않는다.
void DissolveVisualSystem::Run(DissolveVisualProcess& process) {
    if (!process.ready_) return;CheckScale();RawDissolveFrame logic(rendering_.Pool(),game_,frame_,FrameHooks(&process));logic.Step(process.fields_);
}
// 현재 같은 캡처 bitmap/실제 표면에 출력한다.
void DissolveVisualSystem::Draw(DissolveVisualProcess& process) {
    if (!process.ready_) return;CheckScale();RawDissolveFrame logic(rendering_.Pool(),game_,frame_,FrameHooks(&process));logic.Draw(process.fields_);
}
// 프레임 dirty와 다른 공통 삭제 영역 계산을 유지한다.
void DissolveVisualSystem::Removing(const DissolveVisualProcess& process) const {
    if (!process.ready_) return;const auto rect=RenderProcessRemovalBounds(process.fields_.common.registration,frame_.cameraX,frame_.cameraY);hooks_.frame.invalidate({rect[0],rect[1],rect[2],rect[3]});
}
// 의존 쌍 완료 뒤의 dummy 삭제는 실제 Form 삭제 사슬에 맡겨 원본 순서를 보존한다.
void DissolveVisualSystem::Destroy(DissolveVisualProcess& process) {
    const auto token=process.fields_.common.registration.token;
    if (process.fields_.common.registration.slot>=0) { RawDissolveLifetime lifetime(rendering_.Pool(),rendering_.Registry(),LifetimeHooks(process));lifetime.Destroy(process.fields_,1,hooks_.release); }
    processes_.erase(token);rendering_.Forget(token);
    if (process.dummy_.value && !process.paired_ && !(rendering_.Pool().Slot(process.dummy_)[11]&3)) hooks_.destroyUnpaired(process.dummy_);
}
// 다른 종류를 합산하지 않은 실제 분해 프로세스 수다.
std::size_t DissolveVisualSystem::Size() const { return processes_.size(); }
}
