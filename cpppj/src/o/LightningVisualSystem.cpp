// 실제 생성/소진 삭제와 공통 표시 수명을 같은 Host/Kernel에 부착한다.
#include "o/LightningVisualSystem.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 시스템 소멸/실패 정리에서 사용하는 실제 client Form 삭제 권한이다.
constexpr std::uint32_t kClientKill=0x10;
}
// 파생 준비 완료 표식은 상위 Add가 마지막에 세운다.
LightningVisualProcess::LightningVisualProcess(LightningVisualSystem& system):system_(system) {}
// Kernel 소멸자가 경로/입자/등록/객체를 원본 순서로 정리한다.
LightningVisualProcess::~LightningVisualProcess() { system_.Destroy(*this); }
// 별도 프레임 목록/시계를 만들지 않는다.
void LightningVisualProcess::RunFrame() { system_.Run(*this); }
// 입자 반납 전에 현재 표시 영역을 통지한다.
void LightningVisualProcess::OnFormDestroy(std::uint32_t) { system_.Removing(*this); }
// 공통 kind3의 실제 출력 분배다.
void LightningVisualProcess::Draw() { system_.Draw(*this); }
// 별도 입자 사본 대신 현재 단일 저장소를 읽는다.
const LightningFrameFields& LightningVisualProcess::Fields() const { return fields_; }
// 원본 생성/경로/프레임 참조를 같은 월드에 연결하고 출력 누락을 생성 전에 거부한다.
LightningVisualSystem::LightningVisualSystem(RenderProcessSystem& rendering,GameRandom& random,LightningLifetimeState& lifetime,
    LightningFrameState& frame,LightningSystemHooks hooks):rendering_(rendering),random_(random),frame_(frame),hooks_(std::move(hooks)),
    path_(rendering.Pool(),random_,pathState_,hooks_.center),lifetime_(rendering.Pool(),rendering.Registry(),path_,lifetime,hooks_.lifetime) {
    if (!hooks_.release) throw std::invalid_argument("빔 실제 반납 경계 누락");CheckScale();
    const RawLightningFrame checked(rendering_.Pool(),random_,path_,frame_,FrameHooks(nullptr));
}
// 각 실제 삭제가 조회를 지우므로 매번 현재 첫 항목을 다시 읽는다.
LightningVisualSystem::~LightningVisualSystem() {
    // 상위 월드는 Host보다 먼저 이 시스템을 파괴해야 한다.
    while (!processes_.empty()) rendering_.Host().Kill(*processes_.begin()->second,kClientKill);
}
// 원본의 같은 배율을 다른 값으로 조용히 실행하지 않는다.
void LightningVisualSystem::CheckScale() {
    if (frame_.pixelScaleX!=rendering_.State().scaleX || frame_.pixelScaleY!=rendering_.State().scaleY) throw std::logic_error("빔 공통 표시 배율 불일치");
    pathState_.scaleX=frame_.pixelScaleX;pathState_.scaleY=frame_.pixelScaleY;
}
// 소진 순간의 Form 삭제는 실제 Host 권한 검사/종속 해제/Kernel 소멸로 이어진다.
LightningFrameHooks LightningVisualSystem::FrameHooks(LightningVisualProcess* process) {
    auto hooks=hooks_.frame;hooks.removeForm=[this,process] {
        if (!process) throw std::logic_error("빔 프레임 객체 미지정");rendering_.Host().Kill(*process,0);
    };return hooks;
}
// ready 전 재진입은 파생 필드를 읽지 않는다. 원본 공통 부착 순서를 바꾸지 않는다.
LightningVisualProcess* LightningVisualSystem::Add(Sid parent,Sid target,std::optional<std::string> sound) {
    if (!parent.value || creating_) throw std::invalid_argument("빔 Host 부모/중첩 생성 오류");CheckScale();std::uint32_t token=0;creating_=true;
    try {
        pending_=std::make_unique<LightningVisualProcess>(*this);auto* process=pending_.get();
        token=rendering_.Track(process->fields_.registration,[this](RenderProcessFields& fields,std::uint32_t type,Sid source,std::uint32_t flags) {
            if (!pending_ || &pending_->fields_.registration!=&fields) throw std::logic_error("빔 부착 객체 불일치");
            auto* attached=rendering_.Host().Attach(std::move(pending_),type,source,flags);if (!attached) throw std::logic_error("빔 실제 부모 부착 실패");
            fields.form=attached->Form().value;fields.parent=attached->Parent().value;
        },[process] { process->Draw(); });processes_.emplace(token,process);
        lifetime_.Initialize(process->fields_,token,parent,target,std::move(sound));
        if (!processes_.contains(token)) throw std::logic_error("빔 생성 중 객체 삭제");
        process->ready_=true;rendering_.Ready(token);creating_=false;return process;
    } catch (...) {
        if (pending_) pending_.reset();
        else if (const auto found=processes_.find(token);found!=processes_.end()) rendering_.Host().Kill(*found->second,kClientKill);
        creating_=false;throw;
    }
}
// Step가 자기 Form을 삭제할 수 있으므로 호출 뒤 process를 다시 읽지 않는다.
void LightningVisualSystem::Run(LightningVisualProcess& process) {
    if (!process.ready_) return;CheckScale();RawLightningFrame frame(rendering_.Pool(),random_,path_,frame_,FrameHooks(&process));frame.Step(process.fields_);
}
// 별도 끝점 조회가 필요 없는 실제 이전 점 연결 Draw다.
void LightningVisualSystem::Draw(LightningVisualProcess& process) {
    if (!process.ready_) return;CheckScale();RawLightningFrame frame(rendering_.Pool(),random_,path_,frame_,FrameHooks(&process));frame.Draw(process.fields_);
}
// 같은 공통 표시 삭제 통지의 실제 정수화 순서를 유지한다.
void LightningVisualSystem::Removing(const LightningVisualProcess& process) const {
    if (!process.ready_) return;const auto rect=RenderProcessRemovalBounds(process.fields_.registration,frame_.cameraX,frame_.cameraY);hooks_.frame.invalidate({rect[0],rect[1],rect[2],rect[3]});
}
// 생성 중 실패도 등록 여부에 따라 실제 소멸/조회 정리를 수행한다.
void LightningVisualSystem::Destroy(LightningVisualProcess& process) {
    const auto token=process.fields_.registration.token;if (process.fields_.registration.slot>=0) lifetime_.Destroy(process.fields_,1,hooks_.release);
    processes_.erase(token);rendering_.Forget(token);
}
// 다른 효과를 포함한 공통 목록 크기와 구별한다.
std::size_t LightningVisualSystem::Size() const { return processes_.size(); }
}
