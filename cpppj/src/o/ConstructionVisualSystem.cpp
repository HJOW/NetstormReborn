// 생성·실행·출력·소멸은 기존 복원 몸체를 사용하며 이 파일은 호스트 소유권만 연결한다.
#include "o/ConstructionVisualSystem.h"
#include "o/RawConstructionProcess.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 실패 정리/시스템 종료에서 client form 삭제 권한을 명시하는 원본 flag다.
constexpr std::uint32_t kClientKill=0x10;
// 프레임이 바꾼 표시 영역/깊이를 같은 RenderProcess의 등록 필드에 반영한다.
void SynchronizeRegistration(RenderProcessFields& registration,const ConstructionVisualFields& visual) {
    registration.left=visual.left;registration.top=visual.top;registration.right=visual.right;registration.bottom=visual.bottom;
    registration.zOrder=visual.zOrder;
}
}
// 부착 전 필드는 기본값이며 생성 완료 표시는 system.Add가 설정한다.
ConstructionVisualProcess::ConstructionVisualProcess(ConstructionVisualSystem& system):system_(system) {}
// Kernel의 가상 소멸자가 이 경로를 호출한다. 공유 팔레트는 소유하지 않는다.
ConstructionVisualProcess::~ConstructionVisualProcess() { system_.Destroy(*this); }
// Run이 자기 자신을 삭제할 수 있으므로 호출 뒤에는 아무 작업도 하지 않는다.
void ConstructionVisualProcess::RunFrame() { system_.Run(*this); }
// 출력 경계도 객체 수명을 바꿀 수 있으므로 바로 반환한다.
void ConstructionVisualProcess::Draw() const { system_.DrawProcess(*this); }
// 생성/실행 결과를 읽으며 소유 버퍼를 복사하지 않는다.
const ConstructionFrameFields& ConstructionVisualProcess::Fields() const { return fields_; }
// 같은 객체의 표시 슬롯과 현재 영역을 읽는다.
const RenderProcessFields& ConstructionVisualProcess::Registration() const { return registration_; }
// 경계 유효성은 객체를 생성하기 전에 확인한다. Raw 몸체의 외부 attach/kill만 내부 연결로 대체한다.
ConstructionVisualSystem::ConstructionVisualSystem(SidPool& pool,SquidProcessHost& host,ConstructionVisualState& visualState,
    ConstructionFrameState& frameState,const ConstructionFramePalette& palette,GameRandom& random,
    RenderRegistryHooks registryHooks,ConstructionVisualHooks visualHooks,ConstructionFrameHooks frameHooks,
    std::function<void(ConstructionRelease)> release)
    :pool_(pool),host_(host),visualState_(visualState),frameState_(frameState),palette_(palette),random_(random),
     visualHooks_(std::move(visualHooks)),frameHooks_(std::move(frameHooks)),release_(std::move(release)),
     registry_(pool,RegistryHooks(std::move(registryHooks))) {
    if (&pool_!=&host_.Pool() || !release_) throw std::invalid_argument("건설 표시 시스템 연결 오류");
    auto visualCheck=visualHooks_;visualCheck.attach=[](ConstructionVisualFields&,std::uint32_t) {};
    const RawConstructionVisual visual(pool_,visualState_,random_,std::move(visualCheck));
    auto frameCheck=frameHooks_;frameCheck.kill=[](std::uint32_t) {};
    const RawConstructionFrame frame(pool_,visualState_,frameState_,palette_,random_,std::move(frameCheck));
}
// host는 system보다 오래 살아 있으므로 남은 Kernel/form을 공통 삭제로 반납할 수 있다.
ConstructionVisualSystem::~ConstructionVisualSystem() {
    // 각 Kill이 소멸자에서 조회 항목을 지우므로 매번 현재 첫 항목을 다시 읽는다.
    while (!processes_.empty()) host_.Kill(*processes_.begin()->second,kClientKill);
}
// 표시 목록이 먼저 채워진 뒤 해당 객체의 소유권을 Kernel로 옮긴다.
RenderRegistryHooks ConstructionVisualSystem::RegistryHooks(RenderRegistryHooks hooks) {
    hooks.attach=[this](RenderProcessFields& fields,std::uint32_t type,Sid parent,std::uint32_t flags) {
        if (!pending_ || &pending_->registration_!=&fields) throw std::logic_error("건설 표시 부착 객체 불일치");
        auto* attached=host_.Attach(std::move(pending_),type,parent,flags);
        if (!attached) throw std::logic_error("건설 표시 부모 부착 실패");
        fields.form=attached->Form().value;fields.parent=attached->Parent().value;
    };return hooks;
}
// 원본 마지막 Kill이 process를 없애더라도 로컬 RawConstructionFrame의 콜백 수명은 유지된다.
ConstructionFrameHooks ConstructionVisualSystem::FrameHooks(ConstructionVisualProcess& process) const {
    auto hooks=frameHooks_;hooks.kill=[host=&host_,self=&process](std::uint32_t flags) { host->Kill(*self,flags); };return hooks;
}
// Raw Create가 끝나기 전의 Kernel 재진입은 초기화되지 않은 버퍼를 실행하지 않는다.
ConstructionVisualProcess* ConstructionVisualSystem::Add(Sid parent,bool instant) {
    if (creating_) throw std::logic_error("건설 표시 중첩 생성은 지원하지 않습니다");
    if (!nextToken_) throw std::overflow_error("건설 표시 토큰 소진");
    const auto token=nextToken_++;creating_=true;
    try {
        pending_=std::make_unique<ConstructionVisualProcess>(*this);auto* process=pending_.get();
        process->registration_.token=token;processes_.emplace(token,process);
        RawConstructionVisual visual(pool_,visualState_,random_,MakeConstructionRegistryHooks(registry_,process->registration_,token,visualHooks_));
        auto fields=visual.Create(parent,instant);
        if (!processes_.contains(token)) throw std::logic_error("건설 표시 생성 중 객체 삭제");
        process->fields_.visual=std::move(fields);SynchronizeRegistration(process->registration_,process->fields_.visual);
        process->ready_=true;creating_=false;return process;
    } catch (...) {
        if (pending_) pending_.reset();
        else if (const auto found=processes_.find(token);found!=processes_.end()) host_.Kill(*found->second,kClientKill);
        creating_=false;throw;
    }
}
// 프레임 종료 이후에는 토큰 조회로 생존을 확인해 삭제된 객체에 사각형을 쓰지 않는다.
void ConstructionVisualSystem::Run(ConstructionVisualProcess& process) {
    if (!process.ready_) return;const auto token=process.registration_.token;
    RawConstructionFrame frame(pool_,visualState_,frameState_,palette_,random_,FrameHooks(process));frame.Run(process.fields_);
    if (const auto found=processes_.find(token);found!=processes_.end() && found->second==&process)
        SynchronizeRegistration(found->second->registration_,found->second->fields_.visual);
}
// Draw의 kill 경계는 사용되지 않지만 필수 계약을 충족한다. 제출 뒤 객체를 다시 읽지 않는다.
void ConstructionVisualSystem::DrawProcess(const ConstructionVisualProcess& process) const {
    if (!process.ready_) return;auto hooks=frameHooks_;hooks.kill=[](std::uint32_t) {};
    RawConstructionFrame frame(pool_,visualState_,frameState_,palette_,random_,std::move(hooks));frame.Draw(process.fields_);
}
// 슬롯 span은 콜백 중 Add가 확장할 수 있으므로 각 차례에 다시 얻는다.
void ConstructionVisualSystem::Draw() const {
    // 현재 살아 있는 초기화 완료 객체만 원래 등록 슬롯 순서로 제출한다.
    for (std::size_t slot=0;slot<registry_.Slots().size();++slot) {
        const auto token=registry_.Slots()[slot];const auto found=processes_.find(token);
        if (found!=processes_.end()) found->second->Draw();
    }
}
// 원본 collector는 등록 필드/부모 raw를 읽는다. 호스트 생성 중 객체만 lookup에서 제외한다.
std::int32_t ConstructionVisualSystem::Collect(RenderWorldRect view,std::span<RenderCollectionEntry> output,std::int32_t limit) const {
    RawRenderCollection collection(pool_,visualState_,registry_,[this](std::uint32_t token)->const RenderProcessFields* {
        const auto found=processes_.find(token);
        return found!=processes_.end() && found->second->ready_ ? &found->second->registration_ : nullptr;
    });
    return collection.Collect(view,output,limit);
}
// 원본 kind 3 분배를 현재 토큰 조회로 연결한다. 삭제된 포인터를 호출하는 원본 오류 경로는 재현하지 않는다.
void ConstructionVisualSystem::DrawCollected(std::span<const RenderCollectionEntry> entries) const {
    // 목록은 수집된 시점의 항목이다. 출력 중 새 객체를 만들어도 이번 목록에 추가하지 않는다.
    for (const auto& entry:entries) {
        if (entry.kind!=3) throw std::invalid_argument("건설 표시 수집 항목 종류 오류");
        DrawToken(entry.token);
    }
}
// 상위 공통 목록의 kind 3은 키/종류 대신 표시 토큰만 전달한다.
void ConstructionVisualSystem::DrawToken(std::uint32_t token) const {
    const auto found=processes_.find(token);if (found!=processes_.end()) found->second->Draw();
}
// 소멸 경계는 예외를 던지지 않는 계약이다. 부착 전 확보 실패라면 아직 등록할 칸이 없다.
void ConstructionVisualSystem::Destroy(ConstructionVisualProcess& process) {
    const auto token=process.registration_.token;
    if (process.registration_.slot>=0) DestroyConstructionVisual(process.fields_.visual,process.registration_,registry_,1,release_);
    processes_.erase(token);
}
// 원본 슬롯의 구멍을 그대로 제공한다.
std::span<const std::uint32_t> ConstructionVisualSystem::Slots() const { return registry_.Slots(); }
// 생성 중 객체를 포함한 현재 토큰 조회 수다.
std::size_t ConstructionVisualSystem::Size() const { return processes_.size(); }
// 시작 어댑터가 같은 월드인지 확인하는 읽기 전용 참조다.
const SidPool& ConstructionVisualSystem::Pool() const { return pool_; }
// Start가 표시를 만든 후 시각을 읽는 순서도 기존 Start 몸체 그대로 유지한다.
ConstructionProcessHooks MakeConstructionVisualSystemHooks(const SidPool& pool,ConstructionVisualSystem& system,ConstructionProcessHooks hooks) {
    if (&pool!=&system.Pool()) throw std::invalid_argument("건설 표시 시작의 풀 연결 오류");
    hooks.effect=[&system](Sid parent,bool instant) { static_cast<void>(system.Add(parent,instant)); };return hooks;
}
}
