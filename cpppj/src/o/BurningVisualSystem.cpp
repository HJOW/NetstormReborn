// 생성 중/후 단일 소유권과 실제 원본 표시 등록·가상 수명 순서를 보존한다.
#include "o/BurningVisualSystem.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
BurningVisualProcess::BurningVisualProcess(BurningVisualSystem& system):system_(system) {} // 실제 초기화는 부착 후 수행한다.
BurningVisualProcess::~BurningVisualProcess() { system_.Destroy(*this); } // Kernel이 소유한 객체의 실제 반납이다.
void BurningVisualProcess::RunFrame() { system_.Run(*this); } // +18→선택된+24의 실제 호출이다.
void BurningVisualProcess::OnFormDestroy(std::uint32_t) { system_.Removing(*this); } // 소멸자 앞의 현재 영역 통지다.
void BurningVisualProcess::Notify(std::uint32_t,std::uint32_t,std::uint32_t) {} // 보호VT503730+1c의480260은 빈ret12다.
const BurningVisualFields& BurningVisualProcess::Fields() const { return fields_; } // 현재 실제 필드다.
// Frame 입력과 공통 수집의 실제16/11 배율이 일치해야 한다.
BurningVisualSystem::BurningVisualSystem(RenderProcessSystem& rendering,GameRandom& random,std::span<const RiftTypeRecord> types,
    BurningVisualState& visualState,BurningSourcesState& sourcesState,BurningFrameState& frameState,
    BurningDrawPalette& palette,const std::uint32_t& frameNumber,BurningSystemHooks hooks)
    :rendering_(rendering),random_(random),types_(types),frameState_(frameState),palette_(palette),frameNumber_(frameNumber),hooks_(std::move(hooks)),sink_(hooks_),
     visual_(rendering.Pool(),rendering.Registry(),visualState,hooks_.visual),sources_(rendering.Pool(),types,visual_,random,sourcesState) {
    if (!hooks_.invalidate || !hooks_.clip || !hooks_.pixel || !hooks_.release || !hooks_.frame.stopSound || rendering.Pool().Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("Burning 실제 시스템/장치/판본 누락");
    const RawBurningFrame checked(rendering.Pool(),types,visual_,sources_,random,frameState,sink_,hooks_.frame);
}
// 각 삭제가 현재 map을 바꾸므로 첫 항목을 매번 다시 읽는다.
BurningVisualSystem::~BurningVisualSystem() { while (!processes_.empty()) rendering_.Host().Kill(*processes_.begin()->second,0x10); } // 같은 실제 client Form의 즉시 해제다.
// Raw 생성자의 목록 등록→Host 부착→150입자/첫 색표 순서를 바꾸지 않는다.
BurningVisualProcess* BurningVisualSystem::Add(Sid victim,std::uint32_t mode) {
    if (!victim.value || creating_) throw std::invalid_argument("Burning 실제 피해 부모/중첩 생성 오류");std::uint32_t token{};creating_=true;
    try {
        pending_=std::make_unique<BurningVisualProcess>(*this);auto* process=pending_.get();
        token=rendering_.Track(process->fields_.registration,[this](RenderProcessFields& fields,std::uint32_t type,Sid target,std::uint32_t flags) {
            if (!pending_ || &pending_->fields_.registration!=&fields) throw std::logic_error("Burning 실제 부착 필드 불일치");auto* attached=rendering_.Host().Attach(std::move(pending_),type,target,flags);if (!attached) throw std::logic_error("Burning 실제 부모 부착 거부");fields.form=attached->Form().value;fields.parent=attached->Parent().value;
        },[this,process] { Draw(*process); });processes_.emplace(token,process);
        visual_.Initialize(process->fields_,token,victim,mode);process->ready_=true;rendering_.Ready(token);creating_=false;return process;
    } catch (...) { if (pending_) pending_.reset();else if (const auto found=processes_.find(token);found!=processes_.end()) rendering_.Host().Kill(*found->second,0x10);creating_=false;throw; }
}
// 공통 Kernel은+18을 부르며 뒤쪽 표시slot은 원본처럼4프레임 중 한 번만 실행한다.
void BurningVisualSystem::Run(BurningVisualProcess& process) {
    if (!process.ready_ || !RenderProcessFrameDue(process.fields_.registration.slot,frameNumber_)) return;
    if (frameState_.pixelScaleX!=rendering_.State().scaleX || frameState_.pixelScaleY!=rendering_.State().scaleY) throw std::logic_error("Burning 현재 표시/프레임 배율 불일치");
    RawBurningFrame frame(rendering_.Pool(),types_,visual_,sources_,random_,frameState_,sink_,hooks_.frame);frame.Step(process.fields_,process.frame_);
}
// 현재 renderer clip이 설정된 뒤에만 실제 출력한다.
void BurningVisualSystem::Draw(BurningVisualProcess& process) { if (!process.ready_) return;auto state=frameState_;state.viewport=hooks_.clip();DrawBurningVisual(process.fields_,state,palette_,hooks_.pixel); }
// 생성 미완료 객체에는 파생 사각형을 읽지 않는다.
void BurningVisualSystem::Removing(const BurningVisualProcess& process) const { if (!process.ready_) return;const auto rect=RenderProcessRemovalBounds(process.fields_.registration,frameState_.cameraX,frameState_.cameraY);hooks_.invalidate({rect[0],rect[1],rect[2],rect[3]}); }
// 실제 소멸자는 공유 색 표를 건드리지 않고 단일 현재 조회만 제거한다.
void BurningVisualSystem::Destroy(BurningVisualProcess& process) {
    const auto token=process.fields_.registration.token;if (process.fields_.registration.slot>=0) DestroyBurningVisual(process.fields_,rendering_.Registry(),1,hooks_.release,hooks_.frame.stopSound);processes_.erase(token);rendering_.Forget(token);
}
std::size_t BurningVisualSystem::Size() const { return processes_.size(); } // 실제 살아 있는14의 수다.
}
