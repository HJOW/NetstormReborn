// 공통 효과를 파생 fallback으로 다시 분배하지 않아 같은 타입의 순환 호출을 방지한다.
#include "o/SquidFormDispatch.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 두 PE의 내장 Form 타입과 실제 Kernel 프로세스 타입의 범위다.
constexpr std::uint32_t kDepend=1,kGump=3,kContent=6,kProcessFirst=10,kPatchProcessEnd=63,kCdProcessEnd=62;
}
// 같은 풀/깊이로 들어가는 공통 몸체와 최상위 자산 가상 삭제를 묶는다.
SquidFormDispatch::SquidFormDispatch(SidPool& pool,RawSquidDestroy& destroy,SquidDestroyHooks assets,SquidAssetDestroy assetDestroy)
    :pool_(pool),destroy_(destroy),assets_(std::move(assets)),assetDestroy_(std::move(assetDestroy)) {
    if (&destroy.Pool()!=&pool) throw std::invalid_argument("Form 분배 공통 삭제 SID 풀 불일치");
    if (!assets_.emit || !assets_.selected) throw std::invalid_argument("Form 분배 자산 공통 경계 필요");
}
// 가상 몸체의 emit/Unpop 두 경계가 함께 있어야 완전한 Form 등록으로 인정한다.
void SquidFormDispatch::Register(const SidPool& pool,std::uint32_t type,SquidDestroyHooks hooks) {
    if (&pool!=&pool_ || type>=routes_.size()) throw std::invalid_argument("Form 분배 등록 풀/타입 오류");
    if (bound_[type]) throw std::logic_error("Form 분배 타입 중복 등록");
    if (!hooks.emit || !hooks.unpopForm) throw std::invalid_argument("Form 분배 가상 경계 누락");
    routes_[type]=std::move(hooks);bound_[type]=true;
}
// 감시 쌍의 상대 삭제/알림은 같은 분배기의 다른 Form/자산에도 도달한다.
void SquidFormDispatch::Bind(DependFormHost& forms) { Register(forms.Pool(),kDepend,forms.Hooks()); }
// 소유 UI 알림이 중첩 Form/프로세스를 삭제하는 경로를 등록한다.
void SquidFormDispatch::Bind(GumpFormHost& forms) { Register(forms.Pool(),kGump,forms.Hooks()); }
// 내용물의 경제 효과와 부모 제거 알림을 등록한다.
void SquidFormDispatch::Bind(ContentFormLifetime& forms) { Register(forms.Pool(),kContent,forms.Hooks()); }
// 실제 프로세스 타입 BYTE만 등록하며 아직 PID를 쓰지 않은 기본 ProcessForm(2)은 지원 경로에 추가하지 않는다.
void SquidFormDispatch::Bind(SquidProcessHost& processes) {
    if (&processes.Pool()!=&pool_) throw std::invalid_argument("Form 분배 프로세스 SID 풀 불일치");
    const auto end=pool_.Edition()==OriginalEdition::Patch1078 ? kPatchProcessEnd : kCdProcessEnd;
    // 전체 등록을 바꾸기 전에 모든 타입이 비어 있는지 검사한다.
    for (std::uint32_t type=kProcessFirst;type<end;++type) if (bound_[type]) throw std::logic_error("Form 분배 프로세스 중복 등록");
    const auto hooks=processes.Hooks();
    // 실제 Kernel의 순환 타입 번호 모두에 같은 ProcessForm 가상 몸체를 연결한다.
    for (std::uint32_t type=kProcessFirst;type<end;++type) Register(processes.Pool(),type,hooks);
}
// 콜백 뒤 다음 효과는 현재 raw 타입으로 다시 선택한다.
const SquidDestroyHooks& SquidFormDispatch::Route(Sid sid) const {
    const auto type=pool_.Slot(sid)[10];
    if (type>=routes_.size() || !bound_[type]) throw std::logic_error("Form 분배 미등록 타입");return routes_[type];
}
// 루트 자산 재정의와 Form 공통 몸체를 현재 타입에서 구분한다.
bool SquidFormDispatch::Destroy(Sid sid,std::uint32_t flags) const {
    if (pool_.Slot(sid)[10]>=kFirstAssetTypeNumber && assetDestroy_) return assetDestroy_(sid,flags,Hooks());
    destroy_.Destroy(sid,flags,Hooks());return true;
}
// 공유 분배기는 파생 호스트가 전달한 공통 효과도 같은 최상위 자산 경계로 바로 보낸다.
SquidDestroyHooks SquidFormDispatch::Hooks() const {
    SquidDestroyHooks hooks;hooks.selected=assets_.selected;
    // 파생 Unpop도 현재 타입별 실제 호스트가 처리한다.
    hooks.unpopForm=[this](Sid sid) { Route(sid).unpopForm(sid); };
    // 종속 삭제는 루트 재진입으로 처리해 가상 자산 재정의를 보존하며 각 pre/post/release는 해당 호스트에 전달한다.
    hooks.emit=[this](const SquidDestroyEvent& event) {
        if (event.effect==SquidDestroyEffect::Transmit || event.effect==SquidDestroyEffect::ClearSelection) { assets_.emit(event);return; }
        if (event.effect==SquidDestroyEffect::DestroyDependent) { Destroy(event.sid,event.flags);return; }
        if (pool_.Slot(event.sid)[10]<kFirstAssetTypeNumber) { Route(event.sid).emit(event);return; }
        assets_.emit(event);
    };return hooks;
}
// 월드/클라이언트가 같은 분배기를 공유하는지 확인할 때 사용한다.
const SidPool& SquidFormDispatch::Pool() const { return pool_; }
}
