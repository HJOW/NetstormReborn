// 원본 등록/입자/시간/난수/색 계산을 바꾸지 않고 실제 프로세스 소유권만 연결한다.
#include "o/FieryTailVisualSystem.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// client Form의 실제 로컬 삭제 권한이다.
constexpr std::uint32_t kClientKill=0x10;
}
// 파생 필드 초기화 완료 표시는 상위 Add가 마지막에 세운다.
FieryTailVisualProcess::FieryTailVisualProcess(FieryTailVisualSystem& system):system_(system) {}
// Kernel 소멸자가 입자/표시 등록을 원본 순서대로 정리한다.
FieryTailVisualProcess::~FieryTailVisualProcess() { system_.Destroy(*this); }
// 동일 Kernel의 등록 순서와 현재 시각을 그대로 사용한다.
void FieryTailVisualProcess::RunFrame() { system_.Run(*this); }
// raw Form의 가상0xc는 입자 반납보다 먼저 현재 표시 영역을 무효화한다.
void FieryTailVisualProcess::OnFormDestroy(std::uint32_t) { system_.Removing(*this); }
// 같은 공통 목록의 kind3 출력 분배다.
void FieryTailVisualProcess::Draw() { system_.Draw(*this); }
// 실제 프로세스의 현재 단일 필드를 읽는다.
const FieryTailFrameFields& FieryTailVisualProcess::Fields() const { return fields_; }
// 출력 경계와 실제 같은 풀/판본은 객체를 생성하기 전에 검사한다.
FieryTailVisualSystem::FieryTailVisualSystem(RenderProcessSystem& rendering,GameRandom& random,FieryTailLifetimeState& lifetime,
    FieryTailFrameState& frame,FieryTailFramePalette& palette,FieryTailSystemHooks hooks,FieryTailVariant variant)
    :rendering_(rendering),random_(random),frame_(frame),palette_(palette),hooks_(std::move(hooks)),
     lifetime_(rendering.Pool(),rendering.Registry(),lifetime,hooks_.lifetime,variant) {
    if (!hooks_.endpoint || !hooks_.release) throw std::invalid_argument("Fiery Tail 실제 끝점/반납 경계 누락");
    CheckScale();const RawFieryTailFrame checked(rendering_.Pool(),random_,frame_,palette_,hooks_.frame);
}
// 각 삭제가 조회를 지우므로 매번 현재 첫 항목을 다시 읽는다.
FieryTailVisualSystem::~FieryTailVisualSystem() {
    // 공통 Host는 시스템보다 오래 살아 있으며 client Form을 즉시 해제한다.
    while (!processes_.empty()) rendering_.Host().Kill(*processes_.begin()->second,kClientKill);
}
// 두 값은 원본의 같은 전역이므로 다른 배율로 조용히 실행하지 않는다.
void FieryTailVisualSystem::CheckScale() const {
    if (frame_.pixelScaleX!=rendering_.State().scaleX || frame_.pixelScaleY!=rendering_.State().scaleY)
        throw std::logic_error("Fiery Tail 공통 표시 배율 불일치");
}
// 생성 중 프레임/Draw는 ready가 막는다. 실제 Host 부착 뒤에 파생 초기화를 마친다.
FieryTailVisualProcess* FieryTailVisualSystem::Add(Sid parent) {
    if (!parent.value || creating_) throw std::invalid_argument("Fiery Tail Host 부모/중첩 생성 오류");
    std::uint32_t token=0;creating_=true;
    try {
        pending_=std::make_unique<FieryTailVisualProcess>(*this);auto* process=pending_.get();
        token=rendering_.Track(process->fields_.registration,[this](RenderProcessFields& fields,std::uint32_t type,Sid target,std::uint32_t flags) {
            if (!pending_ || &pending_->fields_.registration!=&fields) throw std::logic_error("Fiery Tail 부착 객체 불일치");
            auto* attached=rendering_.Host().Attach(std::move(pending_),type,target,flags);
            if (!attached) throw std::logic_error("Fiery Tail 실제 부모 부착 실패");
            fields.form=attached->Form().value;fields.parent=attached->Parent().value;
        },[process] { process->Draw(); });processes_.emplace(token,process);
        lifetime_.Initialize(process->fields_,token,parent);
        if (!processes_.contains(token)) throw std::logic_error("Fiery Tail 생성 중 객체 삭제");
        process->ready_=true;rendering_.Ready(token);creating_=false;return process;
    } catch (...) {
        if (pending_) pending_.reset();
        else if (const auto found=processes_.find(token);found!=processes_.end()) rendering_.Host().Kill(*found->second,kClientKill);
        creating_=false;throw;
    }
}
// 최초10입자와 후속 실제 시간 조각을 같은 난수/시계로 실행한다.
void FieryTailVisualSystem::Run(FieryTailVisualProcess& process) {
    if (!process.ready_) return;CheckScale();RawFieryTailFrame frame(rendering_.Pool(),random_,frame_,palette_,hooks_.frame);frame.Step(process.fields_);
}
// 끝점은 호출 시점의 실제 부모로 읽으며 픽셀 배율/카메라를 여기서 다시 더하지 않는다.
void FieryTailVisualSystem::Draw(FieryTailVisualProcess& process) {
    if (!process.ready_) return;CheckScale();const auto endpoint=hooks_.endpoint(Sid{static_cast<std::uint16_t>(process.fields_.registration.parent)});
    RawFieryTailFrame frame(rendering_.Pool(),random_,frame_,palette_,hooks_.frame);frame.Draw(process.fields_,endpoint[0],endpoint[1]);
}
// 일반 프레임의 영역 계산으로 대체하지 않고 실제 삭제 통지의 두 단계 정수화를 사용한다.
void FieryTailVisualSystem::Removing(const FieryTailVisualProcess& process) const {
    if (!process.ready_) return;const auto rect=RenderProcessRemovalBounds(process.fields_.registration,frame_.cameraX,frame_.cameraY);
    hooks_.frame.invalidate({rect[0],rect[1],rect[2],rect[3]});
}
// 소멸자 호출이 끝난 뒤에는 부모/form 대신 원본 -12345 표식만 남긴다.
void FieryTailVisualSystem::Destroy(FieryTailVisualProcess& process) {
    const auto token=process.fields_.registration.token;
    if (process.fields_.registration.slot>=0) lifetime_.Destroy(process.fields_,1,hooks_.release);
    processes_.erase(token);rendering_.Forget(token);
}
// 공통 목록 전체 수와 다른 이 시스템의 소유 항목 수다.
std::size_t FieryTailVisualSystem::Size() const { return processes_.size(); }
}
