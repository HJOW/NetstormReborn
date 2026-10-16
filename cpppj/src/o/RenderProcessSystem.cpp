// 표시 등록/수집의 기존 원본 몸체를 유지하며 효과별 콜백의 조회 수명만 연결한다.
#include "o/RenderProcessSystem.h"
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 CRT의 유한/정상 정수 범위만0 방향 절삭한다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("표시 삭제 영역 정수화 범위 오류");
    return static_cast<std::int32_t>(value);
}
}
// top만 카메라 감산 뒤 float에 저장하지 않는 실제 x87 명령 순서를 따른다.
std::array<std::int32_t,4> RenderProcessRemovalBounds(const RenderProcessFields& fields,float cameraX,float cameraY) {
    const auto left=Ftol(fields.left),top=Ftol(fields.top),right=Ftol(fields.right),bottom=Ftol(fields.bottom);
    const auto storedLeft=static_cast<float>(static_cast<double>(left)-cameraX);
    const auto storedRight=static_cast<float>(static_cast<double>(static_cast<float>(right))-cameraX);
    const auto storedBottom=static_cast<float>(static_cast<double>(static_cast<float>(bottom))-cameraY);
    return {Ftol(static_cast<double>(storedLeft)-1),Ftol((static_cast<double>(top)-cameraY)-1),
        Ftol(static_cast<double>(storedRight)+1),Ftol(static_cast<double>(storedBottom)+1)};
}
// 메모리 확보/반납은 기존 경계를 보존하고 실제 부착만 토큰별로 분배한다.
RenderProcessSystem::RenderProcessSystem(SidPool& pool,SquidProcessHost& host,ConstructionVisualState& state,RenderRegistryHooks hooks)
    :pool_(pool),host_(host),state_(state),registry_(pool,Hooks(std::move(hooks))) {
    if (&pool_!=&host_.Pool()) throw std::invalid_argument("공통 표시 Host/풀 불일치");
}
// 생성자 등록이 끝난 시점의 필드와 실제 pending 객체를 같은 토큰으로 연결한다.
RenderRegistryHooks RenderProcessSystem::Hooks(RenderRegistryHooks hooks) {
    hooks.attach=[this](RenderProcessFields& fields,std::uint32_t type,Sid parent,std::uint32_t flags) {
        const auto found=bindings_.find(fields.token);
        if (found==bindings_.end() || found->second.fields!=&fields) throw std::logic_error("공통 표시 부착 토큰 불일치");
        auto attach=found->second.attach;attach(fields,type,parent,flags);
    };return hooks;
}
// map 확보가 실패하면 필드/번호를 변경하지 않는다. 실제 슬롯 등록은 Raw 생성자가 수행한다.
std::uint32_t RenderProcessSystem::Track(RenderProcessFields& fields,
    std::function<void(RenderProcessFields&,std::uint32_t,Sid,std::uint32_t)> attach,std::function<void()> draw) {
    if (!attach || !draw || fields.slot>=0 || bindings_.contains(fields.token)) throw std::invalid_argument("공통 표시 조회/출력 경계 오류");
    if (!nextToken_) throw std::overflow_error("공통 표시 토큰 소진");
    const auto token=nextToken_;bindings_.emplace(token,Binding{&fields,std::move(attach),std::move(draw),false});
    fields.token=token;++nextToken_;return token;
}
// 원본 등록이 아직 끝나지 않은 항목을 활성화하는 호출 실수는 거부한다.
void RenderProcessSystem::Ready(std::uint32_t token) {
    const auto found=bindings_.find(token);
    if (found==bindings_.end() || found->second.fields->slot<0) throw std::logic_error("공통 표시 초기화 완료 토큰 오류");
    found->second.ready=true;
}
// 실제 버퍼/등록 반납은 효과 소멸자가 수행하므로 여기서는 조회만 없앤다.
void RenderProcessSystem::Forget(std::uint32_t token) { bindings_.erase(token); }
// 출력 중 신규 등록으로 vector가 이동할 수 있으므로 매 차례 현재 슬롯을 읽는다.
void RenderProcessSystem::Draw() const {
    // 종류별 분리 없이 하나의 원본 슬롯 순서다.
    for (std::size_t slot=0;slot<registry_.Slots().size();++slot) DrawToken(registry_.Slots()[slot]);
}
// 출력 콜백 자체도 복사해 자기 삭제로 조회 항목이 없어져도 호출 수명을 유지한다.
void RenderProcessSystem::DrawToken(std::uint32_t token) const {
    const auto found=bindings_.find(token);if (found==bindings_.end() || !found->second.ready) return;
    auto draw=found->second.draw;draw();
}
// 생성 중 파생 필드를 읽지 않는 lookup을 기존 collector의 확장 계약에 공급한다.
std::int32_t RenderProcessSystem::Collect(RenderWorldRect view,std::span<RenderCollectionEntry> output,std::int32_t limit) const {
    RawRenderCollection collection(pool_,state_,registry_,[this](std::uint32_t token)->const RenderProcessFields* {
        const auto found=bindings_.find(token);return found!=bindings_.end() && found->second.ready ? found->second.fields : nullptr;
    });return collection.Collect(view,output,limit);
}
// 생성/삭제는 이 registry만 사용한다.
RawRenderRegistry& RenderProcessSystem::Registry() { return registry_; }
// 현재 원본 등록 배열을 읽는다.
std::span<const std::uint32_t> RenderProcessSystem::Slots() const { return registry_.Slots(); }
// 생성 중 항목도 등록 수명 검사에 포함한다.
std::size_t RenderProcessSystem::Size() const { return bindings_.size(); }
// renderer가 같은 현재 필드에서 영역을 읽게 하며 생성 중인 버퍼는 노출하지 않는다.
const RenderProcessFields* RenderProcessSystem::Lookup(std::uint32_t token) const {
    const auto found=bindings_.find(token);return found!=bindings_.end() && found->second.ready ? found->second.fields : nullptr;
}
// 효과 모듈의 동일 월드 검사용 참조다.
SidPool& RenderProcessSystem::Pool() const { return pool_; }
// 같은 client Form/Kernel을 사용하는 호스트다.
SquidProcessHost& RenderProcessSystem::Host() const { return host_; }
// 공통 수집의 현재 좌표 배율 저장소다.
ConstructionVisualState& RenderProcessSystem::State() const { return state_; }
}
