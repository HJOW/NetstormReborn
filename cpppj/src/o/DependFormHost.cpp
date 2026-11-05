// 감시 삭제의 재진입에서는 dead 비트를 공유하고 현재 상대 링크를 효과 뒤 다시 읽는다.
#include "o/DependFormHost.h"
#include "o/RawDependCreate.h" // 두 Create/링크/가상 부착의 실제 순서 부모다.
#include "o/SquidFactory.h" // 독립 입력도 원문 TYPE 생성자를 사용하는 같은 공장으로 처리한다.
#include <stdexcept>
#include <optional>
#include <unordered_set>
#include <utility>

namespace netstorm::o {
namespace {
// 공통 종속 필드와 타입1 감시 쌍의 상대 번호 필드다.
constexpr std::size_t kNext=4,kHead=6,kType=10,kState=11,kWatcher=18,kWatched=20;
// 원본 free/dead 상태와 내장 감시 타입이다.
constexpr std::uint8_t kFree=1,kDead=2,kDepend=1;
}
// 다른 풀의 삭제기가 우연히 같은 번호를 처리하지 못하도록 생성 시 확인한다.
DependFormHost::DependFormHost(SidPool& pool,RawSquidDestroy& destroy,DependFormNotice notice,
    SquidDestroyHooks fallback,FormPopTransmit transmit)
    :pool_(pool),destroy_(destroy),notice_(std::move(notice)),fallback_(std::move(fallback)),transmit_(transmit),forms_(pool,{},std::move(transmit)) {
    if (&destroy.Pool()!=&pool) throw std::invalid_argument("DependForm 삭제 SID 풀 불일치");
}
void DependFormHost::BindUnpop(std::function<void(Sid)> unpop) { unpop_=std::move(unpop); } // 공통 삭제에서도 현재 Factory의 진단/분리 순서를 사용한다.
void DependFormHost::BindCreation(SquidFactory& factory,const std::uint32_t& type,std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion) {
    if (&factory.Pool()!=&pool_) throw std::invalid_argument("Depend 생성 공장의 실제 SID 풀 불일치");factory_=&factory;dependType_=&type;creationAssertion_=std::move(assertion); // 반납 후 다른 World 공장을 장기 저장소에 남기지 않는다.
}
std::uint32_t DependFormHost::DependType() const { return dependType_ ? *dependType_ : kDepend; } // 각 호출 위치에서 BYTE로 줄이지 않은 현재 번호다.
// 반납 뒤에도 타입 바이트는 보존되므로 free 필터는 호출자가 적용한다.
bool DependFormHost::IsDependForm(Sid form) const {
    if (form.value<5 || form.value>=pool_.Capacity() || form.value==pool_.Layout().predictableFirst) return false;
    const auto raw=pool_.Slot(form);const auto table=std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);
    return table==forms_.Vtable(kDepend); // 실제 감시 클래스의 가상 진입은 이전/current TYPE BYTE 차이로 거부하지 않는다.
}
// 죽는 중의 Form은 재진입 처리에 필요하며 free 슬롯의 가상 필드는 읽지 않는다.
void DependFormHost::Form(Sid form) const {
    if (!IsDependForm(form) || (pool_.Slot(form)[kState]&kFree)) throw std::logic_error("DependForm 슬롯 상태 오류");
    forms_.Parent(form);
}
// WORD 전체를 읽어 상위 바이트를 잃지 않는다.
Sid DependFormHost::Word(Sid sid,std::size_t offset) const {
    const auto raw=pool_.Slot(sid);return Sid{static_cast<std::uint16_t>(raw[offset]|(std::uint16_t(raw[offset+1])<<8))};
}
// WORD 두 바이트만 써서 기존 상대 payload를 그대로 둔다.
void DependFormHost::Put(Sid sid,std::size_t offset,Sid value) const {
    auto raw=pool_.AllocatedBytes(sid);raw[offset]=static_cast<std::uint8_t>(value.value);raw[offset+1]=static_cast<std::uint8_t>(value.value>>8);
}
// watcher는 ProcessForm도 가능하지만 DependForm은 금지이며 watched는 자산이어야 한다.
void DependFormHost::Parents(Sid watcher,Sid watched) const {
    // 두 부모 번호는 SID 예약 영역과 free 슬롯을 제외한다.
    for (const auto parent:{watcher,watched})
        if (parent.value<5 || parent.value>=pool_.Capacity() || parent.value==pool_.Layout().predictableFirst || (pool_.Slot(parent)[kState]&kFree))
            throw std::logic_error("DependForm 부모 상태 오류");
    if (watcher==watched || pool_.Slot(watcher)[kType]==DependType() || pool_.Slot(watched)[kType]<kFirstAssetTypeNumber)
        throw std::logic_error("DependForm 감시 대상 타입 오류");
}
// watcher 쪽을 먼저 생성/부착한다. 원본 생성 인자 순서는 watched→watcher이며 Form 부모도 허용한다.
DependFormPair DependFormHost::Create(Sid watched,Sid watcher) const {
    const auto object=[this](std::uint32_t number) { if (number>=pool_.Capacity()) throw std::out_of_range("Depend 생성의 물리 객체 범위");return Sid{static_cast<std::uint16_t>(number)}; }; // 링크/부착이 실제 소비하는 위치에서만 물리 범위를 확인한다.
    std::optional<SquidFactory> independent;
    if (!factory_) { independent.emplace(pool_,destroy_.Types());independent->BindDiagnostics({destroy_.Types(),creationAssertion_}); } // 독립 입력에도 고정 VT 할당 경로를 추가하지 않는다.
    auto& factory=factory_ ? *factory_ : *independent;DependCreateHooks hooks;hooks.server=[this] { return pool_.IsServer(); };hooks.formType=[this] { return DependType(); };hooks.assertion=creationAssertion_;
    hooks.create=[&factory](auto type,auto flags) { return std::uint32_t(factory.Create(type,flags).value); }; // 각 현재 번호/생성자는 공통 Create가 다시 읽는다.
    hooks.sid=[](auto token) { return token; }; // 실제 풀 어댑터의 객체 토큰은 같은 물리 SID다.
    hooks.word=[this,object](auto token,auto offset,auto value) { const auto slot=pool_.Slot(object(token));auto* raw=const_cast<std::uint8_t*>(slot.data());raw[offset]=static_cast<std::uint8_t>(value);raw[offset+1]=static_cast<std::uint8_t>(value>>8); }; // 원문 WORD 저장에 할당/free 선행 검사를 추가하지 않는다.
    hooks.vtable=[this,object](auto token) { const auto raw=pool_.Slot(object(token));return std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24); }; // Create/첫 부착 이후의 현재 VT를 조회한다.
    hooks.attach=[this,object,&factory](auto token,auto table,auto parent,auto flags) { // 1/3/6은 실제 공통 thunk이며 Process2는 별도 직렬화 접두다.
        bool common{};for (const auto type:{1U,3U,6U}) if (table==forms_.Vtable(type)) common=true; // Content도 공통 몸체를 실행한 뒤 현재 번호 진단을 받는다.
        if (!common) throw std::logic_error("Depend 생성의 선택된 실제 VT+74 미연결");factory.AttachForm(object(token),object(parent),flags,transmit_);
    };
    const auto pair=CreateDependForms(pool_.Edition(),watched.value,watcher.value,hooks);return {object(pair.watched),object(pair.watcher)};
}
// 상대는 Form 부모 getter를 사용하므로 ProcessForm의 원래 타입도 공통 검사를 통과한다.
Sid DependFormHost::Watcher(Sid form) const {
    Form(form);const auto watcher=Word(form,kWatcher);return watcher.value ? forms_.Parent(watcher) : Sid{};
}
// 원본은 free/dead/void 필터를 추가하지 않고 현재 Form 타입과 watcher 번호만 비교한다.
bool DependFormHost::Has(Sid watched,Sid watcher) const {
    if (!watched.value || watched.value>=pool_.Capacity()) throw std::out_of_range("DependForm 조회 부모 번호 오류");
    std::unordered_set<std::uint16_t> visited;Sid current=Word(watched,kHead);
    // 후보를 반환하기 전에 next를 저장하는 FormFinder 규칙을 유지한다.
    while (current.value) {
        if (current.value>=pool_.Capacity() || !visited.insert(current.value).second) throw std::logic_error("DependForm 조회 체인 손상");
        const auto next=Word(current,kNext);
        if (pool_.Slot(current)[kType]==DependType() && Watcher(current)==watcher) return true;
        current=next;
    }
    return false;
}
// watched가 이미 dead이면 타입/같은 객체 검사보다 먼저 반환한다.
void DependFormHost::Remove(Sid watcher,Sid watched,std::uint32_t flags) const {
    if (!watched.value || watched.value>=pool_.Capacity()) throw std::out_of_range("DependForm 제거 부모 번호 오류");
    if (pool_.Slot(watched)[kState]&kDead) return;
    Parents(watcher,watched);std::unordered_set<std::uint16_t> visited;Sid current=Word(watched,kHead);
    // 삭제 콜백이 현재 슬롯을 반납하더라도 선저장한 next로 이어간다.
    while (current.value) {
        if (current.value>=pool_.Capacity() || !visited.insert(current.value).second) throw std::logic_error("DependForm 제거 체인 손상");
        const auto next=Word(current,kNext);
        if (pool_.Slot(current)[kType]==DependType() && Watcher(current)==watcher) Destroy(current,flags);
        current=next;
    }
}
// 알림은 상대 Form의 client 번호/서버 권한으로 결정하며 콜백 뒤 현재 watchedFid를 다시 검사한다.
void DependFormHost::Notify(Sid form,Sid watched) const {
    Form(form);const auto watcher=Word(form,kWatcher);if (!watcher.value) return;
    const auto parent=forms_.Parent(watcher);
    if (watcher.value<pool_.Layout().serverFirst || pool_.IsServer()) {
        if (!notice_) throw std::invalid_argument("DependForm watcher 통지 경계 필요");
        notice_(parent,watched,form);
    }
    if (Word(form,kWatched).value) throw std::logic_error("DependForm 통지 뒤 watchedFid 오류");
}
// 상대 삭제도 같은 hooks를 사용하므로 두 Form이 서로를 재삭제해도 dead에서 즉시 반환한다.
void DependFormHost::Destroy(Sid form,std::uint32_t flags) const {
    if (!IsDependForm(form)) throw std::logic_error("DependForm 삭제 타입 오류");
    destroy_.Destroy(form,flags,Hooks());
}
// 실제 월드가 다른 파생 Form/자산 분배를 겹쳐 연결할 수 있도록 미처리 효과를 전달한다.
SquidDestroyHooks DependFormHost::Hooks() const {
    SquidDestroyHooks hooks;hooks.selected=fallback_.selected ? fallback_.selected : std::function<Sid()>([] { return Sid{}; });
    // Form 공통 Unpop은 부모 dead의 링크를 보존한다.
    hooks.unpopForm=[this](Sid sid) {
        if (IsDependForm(sid)) { if (unpop_) unpop_(sid);else forms_.Unpop(sid); }
        else { if (!fallback_.unpopForm) throw std::invalid_argument("DependForm 외 Unpop 경계 필요");fallback_.unpopForm(sid); }
    };
    // preDestroy는 watched→현재 watcher 순서로 재조회하여 삭제하고 마지막에 공통 깊이를 반납한다.
    hooks.emit=[this](const SquidDestroyEvent& event) {
        if (IsDependForm(event.sid)) {
            switch (event.effect) {
            case SquidDestroyEffect::PreDestroy: {
                const auto watched=Word(event.sid,kWatched);if (watched.value) Destroy(watched,event.flags);
                const auto watcher=Word(event.sid,kWatcher);if (watcher.value) Destroy(watcher,event.flags);
                destroy_.CompletePreDestroy();return;
            }
            case SquidDestroyEffect::PostDestroy:destroy_.CompletePostDestroy();return;
            case SquidDestroyEffect::ReleaseDependent:Notify(event.sid,event.parent);return;
            case SquidDestroyEffect::DestroyDependent:Destroy(event.sid,event.flags);return;
            default:break;
            }
        }
        if (!fallback_.emit) throw std::invalid_argument("DependForm 외 삭제 효과 경계 필요");
        fallback_.emit(event);
    };return hooks;
}
// 연결기의 동일 풀 검증에 사용한다.
const SidPool& DependFormHost::Pool() const { return pool_; }
}
