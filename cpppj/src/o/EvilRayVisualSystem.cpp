// 원본 수치 몸체를 교체하지 않고 실제 등록/부착/통지/자기 삭제의 소유권만 연결한다.
#include "o/EvilRayVisualSystem.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o { // process10의 실제 공통 표시 수명이다.
namespace { constexpr std::uint32_t kLocalKill=0x10; } // 월드 종료/생성 실패의 실제 client Form 반납 권한이다.
EvilRayVisualProcess::EvilRayVisualProcess(EvilRayVisualSystem& system):system_(system) {} // 원문 파생 초기화는 공통 부착 뒤 수행한다.
EvilRayVisualProcess::~EvilRayVisualProcess() { system_.Destroy(*this); } // Kernel 소멸도 같은 실제 반납 순서를 사용한다.
void EvilRayVisualProcess::RunFrame() { system_.Run(*this); } // 실제 원문 전체 delta 프레임이다.
void EvilRayVisualProcess::OnFormDestroy(std::uint32_t) { system_.Removing(*this); } // 같은 삭제 영역을 실제 dirty에 전달한다.
void EvilRayVisualProcess::Notify(std::uint32_t event,std::uint32_t,std::uint32_t) { system_.Notify(*this,event); } // 나머지 두 인자는 원문처럼 무시한다.
void EvilRayVisualProcess::Draw() { system_.Draw(*this); } // 실제 타원 요청을 공통 kind3에 제출한다.
const EvilRayFields& EvilRayVisualProcess::Fields() const { return fields_; } // 단일 실제 필드를 읽는다.
EvilRayVisualSystem::EvilRayVisualSystem(RenderProcessSystem& rendering,EvilRayLifetimeState& lifetime,FieryTailFrameState& frame,FieryTailFramePalette& colors,EvilRaySystemHooks hooks)
    :rendering_(rendering),frame_(frame),colors_(colors),hooks_(std::move(hooks)),lifetime_(rendering.Pool(),rendering.Registry(),lifetime,hooks_.lifetime) { // 누락 장치는 실제 선택 위치까지 보류하되 소멸 관찰자는 미리 요구한다.
    if (!hooks_.release) throw std::invalid_argument("EvilRay 반납 관찰 장치 미연결");CheckScale();
}
EvilRayVisualSystem::~EvilRayVisualSystem() { // 조회 항목은 각 실제 소멸자가 지우므로 현재 첫 항목을 다시 읽는다.
    while (!processes_.empty()) { if (!unattached_.empty()) unattached_.pop_back();else rendering_.Host().Kill(*processes_.begin()->second,kLocalKill); } // 부모0의 등록도 입자/목록부터 반납한다.
}
void EvilRayVisualSystem::CheckScale() const { if (frame_.pixelScaleX!=rendering_.State().scaleX || frame_.pixelScaleY!=rendering_.State().scaleY) throw std::logic_error("EvilRay 공통 표시 배율 불일치"); } // 같은 원본 전역을 두 값으로 복제하지 않는다.
EvilRayVisualProcess* EvilRayVisualSystem::Add(Sid parent) { // Native parent0 등록도 다른 성공 효과로 바꾸지 않는다.
    if (creating_) throw std::logic_error("EvilRay 중첩 생성 오류");creating_=true;std::uint32_t token=0; // 생성 중인 실제 토큰이다.
    try {
        pending_=std::make_unique<EvilRayVisualProcess>(*this);auto* process=pending_.get();
        token=rendering_.Track(process->fields_.registration,[this](RenderProcessFields& fields,std::uint32_t type,Sid target,std::uint32_t flags) {
            if (!pending_ || &pending_->fields_.registration!=&fields) throw std::logic_error("EvilRay 부착 객체 불일치");auto* attached=rendering_.Host().Attach(std::move(pending_),type,target,flags);if (!attached) throw std::logic_error("EvilRay 실제 Host 부착 실패");fields.form=attached->Form().value;fields.parent=attached->Parent().value;
        },[process] { process->Draw(); });processes_.emplace(token,process);
        lifetime_.Initialize(process->fields_,token,parent);if (!processes_.contains(token)) throw std::logic_error("EvilRay 생성 중 객체 삭제");process->ready_=true;rendering_.Ready(token);
        if (pending_) unattached_.push_back(std::move(pending_));creating_=false;return process;
    }catch (...) { if (pending_) pending_.reset();else if (const auto found=processes_.find(token);found!=processes_.end()) rendering_.Host().Kill(*found->second,kLocalKill);creating_=false;throw; }
}
EvilRayFrameHooks EvilRayVisualSystem::FrameHooks(EvilRayVisualProcess& process) { auto hooks=hooks_.frame;hooks.kill=[this,&process](auto flags) { rendering_.Host().Kill(process,flags); };return hooks; } // 같은 실제 Form의 자기 삭제다.
void EvilRayVisualSystem::Run(EvilRayVisualProcess& process) { if (!process.ready_) return;CheckScale();RawEvilRayFrame body(rendering_.Pool(),frame_,colors_.readable,FrameHooks(process));body.Step(process.fields_); } // 삭제 뒤 process를 다시 읽지 않는다.
void EvilRayVisualSystem::Draw(EvilRayVisualProcess& process) { if (!process.ready_) return;CheckScale();RawEvilRayFrame body(rendering_.Pool(),frame_,colors_.readable,FrameHooks(process));body.Draw(process.fields_); } // 같은 현재 색/시계로 출력한다.
void EvilRayVisualSystem::Notify(EvilRayVisualProcess& process,std::uint32_t mode) { if (process.ready_) lifetime_.Stop(process.fields_,mode); } // 공통 부착 중에는 아직 파생 VT가 아니다.
void EvilRayVisualSystem::Removing(const EvilRayVisualProcess& process) const { // 공통 삭제 통지는 아직 파생 초기화 중이면 실행하지 않는다.
    if (!process.ready_) return;const auto bounds=RenderProcessRemovalBounds(process.fields_.registration,frame_.cameraX,frame_.cameraY);const auto invalidate=hooks_.frame.invalidate;if (!invalidate) throw std::logic_error("EvilRay 실제 삭제 dirty 장치 미연결");invalidate({bounds[0],bounds[1],bounds[2],bounds[3]});
}
void EvilRayVisualSystem::Destroy(EvilRayVisualProcess& process) { const auto token=process.fields_.registration.token;if (process.fields_.registration.slot>=0) lifetime_.Destroy(process.fields_,1,hooks_.release);processes_.erase(token);rendering_.Forget(token); } // 실제 반납 뒤 토큰 조회를 없앤다.
std::size_t EvilRayVisualSystem::Size() const { return processes_.size(); } // 공통 전체 수와 다른 현재10 소유 수다.
}
