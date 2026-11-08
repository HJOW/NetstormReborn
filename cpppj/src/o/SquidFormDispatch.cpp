// 공통 효과를 파생 fallback으로 다시 분배하지 않아 같은 타입의 순환 호출을 방지한다.
#include "o/SquidFormDispatch.h"
#include <stdexcept>
#include <utility>
#include "o/SquidFactory.h"
#include "o/SquidForms.h"

namespace netstorm::o {
namespace {
// 네 내장 Form 클래스의 등록 번호이며 변경 가능한 raw TYPE 범위와 구별한다.
constexpr std::uint32_t kDepend=1,kProcess=2,kGump=3,kContent=6;
// 현재 VT DWORD를 물리 슬롯에서 읽으며 TYPE 번호를 가상 선택의 별도 조건으로 사용하지 않는다.
std::uint32_t CurrentDestroyVtable(const SidPool& pool,Sid sid) {
    const auto raw=pool.Slot(sid);
    return std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);
}
// 네 내장 Form은 같은 공통+10을 쓰며 각 실제 클래스의 Pre/Post/Unpop 호스트는 별도로 선택한다.
bool CurrentFormVtable(SidPool& pool,std::uint32_t table) {
    const SquidForms forms(pool); // 표 주소만 읽으며 풀/목록은 수정하지 않는다.
    for (const auto type:{kDepend,kProcess,kGump,kContent}) if (forms.Vtable(type)==table) return true; // 네 실제 원문 클래스만 순서대로 비교한다.
    return false;
}
}
// 같은 풀/깊이로 들어가는 공통 몸체와 최상위 자산 가상 삭제를 묶는다.
SquidFormDispatch::SquidFormDispatch(SidPool& pool,RawSquidDestroy& destroy,SquidDestroyHooks assets,SquidAssetDestroy assetDestroy)
    :pool_(pool),destroy_(destroy),assets_(std::move(assets)),assetDestroy_(std::move(assetDestroy)) {
    if (&destroy.Pool()!=&pool) throw std::invalid_argument("Form 분배 공통 삭제 SID 풀 불일치");
    if (!assets_.emit || (!assets_.selected && !assets_.selectedRaw)) throw std::invalid_argument("Form 분배 자산 공통 경계 필요");
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
// 실제 Process VT의 가상 몸체를 하나의 호스트에 등록하며 PID 유효성은 각 원문 소비 위치에서 확인한다.
void SquidFormDispatch::Bind(SquidProcessHost& processes) {
    if (&processes.Pool()!=&pool_) throw std::invalid_argument("Form 분배 프로세스 SID 풀 불일치");
    Register(processes.Pool(),kProcess,processes.Hooks());
}
// 전체 Form 경로가 합성한 현재 자산 훅/가상 삭제 경계를 공유하게 한다.
void SquidFormDispatch::SetAssets(SquidDestroyHooks assets,SquidAssetDestroy assetDestroy) {
    if (!assets.emit || (!assets.selected && !assets.selectedRaw) || !assetDestroy) throw std::invalid_argument("Form 분배 현재 자산 경계 필요");
    assets_=std::move(assets);
    if (!destroy_.HasPatchDiagnostics()) assetDestroy_=std::move(assetDestroy); // 실제 PATCH83VT의+10은 최초 같은 World의 common/Bridge 부모를 유지한다. 후설치의 TYPE별 분류는 현재 Pre/Post hooks에만 반영한다.
}
// 콜백 뒤 현재 VT의 실제 Form 클래스로 다시 선택하며 TYPE 별칭을 다른 파생 효과로 보내지 않는다.
const SquidDestroyHooks& SquidFormDispatch::Route(Sid sid) const {
    const auto raw=pool_.Slot(sid);const auto table=std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const std::array<std::pair<std::uint32_t,std::uint32_t>,4> classes{{{patch ? 0x512110U:0x505a08U,kDepend},{patch ? 0x512098U:0x505990U,kProcess},{patch ? 0x512188U:0x505a80U,kGump},{patch ? 0x512208U:0x505b00U,kContent}}}; // 네 실제 생성자 VT가 고른 클래스 호스트만 사용한다.
    for (const auto& [vtable,number]:classes) { // 선택한현재클래스에필요한호스트만요구한다.
        if (table==vtable) { if (!bound_[number]) throw std::logic_error("선택된 현재 Form 클래스 처리기 미등록");return routes_[number]; }
    }
    throw std::logic_error("선택된 현재 Form 가상 처리기 미등록");
}
// 루트 자산 재정의와 Form 공통 몸체를 현재 타입에서 구분한다.
bool SquidFormDispatch::Destroy(Sid sid,std::uint32_t flags) const {
    if (destroy_.HasPatchDiagnostics()) {
        const auto table=CurrentDestroyVtable(pool_,sid);
        if (SquidFactory::SupportsConstructedVtable(pool_.Edition(),table)) {
            if (!assetDestroy_) throw std::logic_error("PATCH 현재 자산 가상+10의 실제 World 처리기 미연결");
            return assetDestroy_(sid,flags,Hooks());
        }
        if (!CurrentFormVtable(pool_,table)) throw std::logic_error("PATCH 현재 가상+10의 미지 VT는 호스트에서 실행할 수 없습니다");
        destroy_.Destroy(sid,flags,Hooks());return true;
    }
    if (pool_.Slot(sid)[10]>=kFirstAssetTypeNumber && assetDestroy_) return assetDestroy_(sid,flags,Hooks());
    destroy_.Destroy(sid,flags,Hooks());return true;
}
// 공유 분배기는 파생 호스트가 전달한 공통 효과도 같은 최상위 자산 경계로 바로 보낸다.
SquidDestroyHooks SquidFormDispatch::Hooks() const {
    SquidDestroyHooks hooks;if (assets_.selected) hooks.selected=[this] { return assets_.selected(); };
    if (assets_.selectedRaw) hooks.selectedRaw=[this] { const auto selected=assets_.selectedRaw;if (selected) return selected();if (!assets_.selected) throw std::logic_error("현재Form분배의실제선택조회미연결");return std::uint32_t(assets_.selected().value); }; //현재부모교체뒤에도Native전체DWORD를유지하고typed는기존Sid를사용한다.
    // 파생 Unpop도 현재 타입별 실제 호스트가 처리한다.
    hooks.unpopForm=[this](Sid sid) { Route(sid).unpopForm(sid); };
    // 종속 삭제는 루트 재진입으로 처리해 가상 자산 재정의를 보존하며 각 pre/post/release는 해당 호스트에 전달한다.
    hooks.emit=[this](const SquidDestroyEvent& event) {
        if (event.effect==SquidDestroyEffect::Transmit || event.effect==SquidDestroyEffect::ClearSelection) { assets_.emit(event);return; }
        if (event.effect==SquidDestroyEffect::DestroyDependent) { Destroy(event.sid,event.flags);return; }
        if (destroy_.HasPatchDiagnostics()) {
            const auto table=CurrentDestroyVtable(pool_,event.sid);
            if (CurrentFormVtable(pool_,table)) { Route(event.sid).emit(event);return; }
            if (!SquidFactory::SupportsConstructedVtable(pool_.Edition(),table)) throw std::logic_error("PATCH 현재 삭제 가상 효과의 미지 VT");
        } else if (pool_.Slot(event.sid)[10]<kFirstAssetTypeNumber) { Route(event.sid).emit(event);return; }
        assets_.emit(event);
    };return hooks;
}
// 월드/클라이언트가 같은 분배기를 공유하는지 확인할 때 사용한다.
const SidPool& SquidFormDispatch::Pool() const { return pool_; }
}
