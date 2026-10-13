// 감시 삭제의 재진입에서는 dead 비트를 공유하고 현재 상대 링크를 효과 뒤 다시 읽는다.
#include "o/DependFormHost.h"
#include <stdexcept>
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
    :pool_(pool),destroy_(destroy),notice_(std::move(notice)),fallback_(std::move(fallback)),forms_(pool,{},std::move(transmit)) {
    if (&destroy.Pool()!=&pool) throw std::invalid_argument("DependForm 삭제 SID 풀 불일치");
}
// 반납 뒤에도 타입 바이트는 보존되므로 free 필터는 호출자가 적용한다.
bool DependFormHost::IsDependForm(Sid form) const {
    return form.value>=5 && form.value<pool_.Capacity() && form.value!=pool_.Layout().predictableFirst
        && pool_.Slot(form)[kType]==kDepend;
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
    if (watcher==watched || pool_.Slot(watcher)[kType]==kDepend || pool_.Slot(watched)[kType]<kFirstAssetTypeNumber)
        throw std::logic_error("DependForm 감시 대상 타입 오류");
}
// watcher 쪽을 먼저 생성/부착한다. 원본 생성 인자 순서는 watched→watcher이며 Form 부모도 허용한다.
DependFormPair DependFormHost::Create(Sid watched,Sid watcher) const {
    // 생성은 제거 helper의 타입 제한을 추가하지 않는다. 같은 부모 안의 두 Form도 원본처럼 지원한다.
    for (const auto parent:{watcher,watched})
        if (parent.value<5 || parent.value>=pool_.Capacity() || parent.value==pool_.Layout().predictableFirst || (pool_.Slot(parent)[kState]&(kFree|kDead)))
            throw std::logic_error("DependForm 생성 부모 상태 오류");
    const bool client=watcher.value<pool_.Layout().serverFirst || watched.value<pool_.Layout().serverFirst;
    if (!client && !pool_.IsServer()) throw std::logic_error("DependForm 서버 쌍 생성 권한 필요");
    const auto first=forms_.Create(kDepend,client ? 2U : 0U),second=forms_.Create(kDepend,client ? 2U : 0U);
    Put(first,kWatched,second);Put(first,kWatcher,{});Put(second,kWatcher,first);Put(second,kWatched,{});
    forms_.Attach(first,watcher);forms_.Attach(second,watched);return {second,first};
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
        if (pool_.Slot(current)[kType]==kDepend && Watcher(current)==watcher) return true;
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
        if (pool_.Slot(current)[kType]==kDepend && Watcher(current)==watcher) Destroy(current,flags);
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
        if (IsDependForm(sid)) forms_.Unpop(sid);
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
