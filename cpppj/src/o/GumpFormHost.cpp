// 원본의 부착/해제 쓰기 순서와 부모 삭제 중 재진입을 유지하며 UI 포인터는 소유 등록 ID로만 연결한다.
#include "o/GumpFormHost.h"
#include "o/SquidForms.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 두 판본의 공통 raw 필드다. Form의 부모/이전은 contained 자산과 위치가 반대다.
constexpr std::size_t kNext=4,kHead=6,kType=10,kState=11,kPrevious=14,kParent=16,kOwner=18,kAssetPrevious=16;
// 공통 free/dead/void/contained/생성 전파 중 상태 비트다.
constexpr std::uint8_t kFree=1,kDead=2,kVoid=4,kContained=8,kTransmitting=0x10;
// Pop의 서버/client/종속 전송 억제 비트와 Form 부착을 막는 emplacement·vortex·factory genus다.
constexpr std::uint32_t kServerPop=8,kClientPop=0x10,kDependentPop=0x40,kNoContainerGenus=0x44200;
}
// 삭제/생성과 같은 풀을 사용하는지 확인하며 타입 메타데이터는 호출자 수명과 분리한다.
GumpFormHost::GumpFormHost(SidPool& pool,SquidFactory& factory,std::span<const RiftTypeRecord> types,
    RawSquidDestroy& destroy,GumpFormNotice notice,SquidDestroyHooks fallback)
    :pool_(pool),factory_(factory),types_(types.begin(),types.end()),destroy_(destroy),
     notice_(std::move(notice)),fallback_(std::move(fallback)) {
    if (&factory.Pool()!=&pool || &destroy.Pool()!=&pool) throw std::invalid_argument("GumpForm 생성/삭제 SID 풀이 다릅니다");
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U))
        throw std::invalid_argument("GumpForm 타입 표의 판본/크기 오류");
}
// 별도 풀을 만들지 않고 기존 Form 생성 계약을 사용한다.
Sid GumpFormHost::Create(std::uint32_t flags) const { return factory_.CreateGumpForm(flags); }
// 같은 raw 풀의 지정 Take를 사용하여 부착된 Form의 링크 정리도 실제로 수행한다.
Sid GumpFormHost::Take(Sid form) const { return SquidForms(pool_).Take(kGumpFormType,form); }
// 메뉴/수명 연결기가 실제 판본과 풀을 확인할 수 있게 제공한다.
const SidPool& GumpFormHost::Pool() const { return pool_; }
// 타입 바이트는 반납 뒤에도 보존된다. dead 검사/중첩 삭제를 위해 free 여부는 여기서 제외한다.
bool GumpFormHost::IsGumpForm(Sid form) const {
    return form.value>=5 && form.value!=pool_.Layout().predictableFirst && form.value<pool_.Capacity()
        && pool_.Slot(form)[kType]==kGumpFormType;
}
// 삭제 중 dead Form도 알림/Unpop이 필요하지만 반납된 슬롯의 필드 접근은 거부한다.
std::span<const std::uint8_t> GumpFormHost::Form(Sid form) const {
    if (!IsGumpForm(form) || (pool_.Slot(form)[kState]&kFree)) throw std::logic_error("GumpForm 슬롯 상태 오류");
    return pool_.Slot(form);
}
// 부모/종속 링크는 두 바이트를 조합하여 호스트 정렬과 무관하게 읽는다.
std::uint16_t GumpFormHost::Word(Sid sid,std::size_t offset) const {
    const auto raw=pool_.Slot(sid);return static_cast<std::uint16_t>(raw[offset]|(std::uint16_t(raw[offset+1])<<8));
}
// 원본 WORD/DWORD store를 같은 폭의 little endian 쓰기로 바꾼다.
void GumpFormHost::Put(Sid sid,std::size_t offset,std::uint32_t value,std::size_t width) const {
    auto raw=pool_.AllocatedBytes(sid);
    // 지정한 필드 폭만 쓰므로 옆의 정렬되지 않은 소유 ID/부모 필드를 보존한다.
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// Gump 등록 ID는 정렬되지 않은 DWORD 전체다. 알림의 조건도 이 전체 값으로 판정한다.
std::uint32_t GumpFormHost::Owner(Sid form) const {
    const auto raw=Form(form);std::uint32_t owner=0;
    // 높은 두 바이트를 버리지 않고 원본 소유 ID를 조립한다.
    for (std::size_t index=0;index<4;++index) owner|=std::uint32_t(raw[kOwner+index])<<(index*8);
    return owner;
}
// 원본 Form getParent(+0x0)의 WORD를 반환한다.
Sid GumpFormHost::Parent(Sid form) const { Form(form);return Sid{Word(form,kParent)}; }
// 생성 후 소유 ID 쓰기는 아직 부모에 붙기 전에도 허용한다.
void GumpFormHost::SetOwner(Sid form,std::uint32_t owner) const { Form(form);Put(form,kOwner,owner,4); }
// 종속 체인에는 Form과 contained 자산이 섞일 수 있으며 각 가상 setPrev의 위치가 다르다.
void GumpFormHost::SetPrevious(Sid sid,std::uint16_t previous) const {
    Put(sid,pool_.Slot(sid)[kType]<kFirstAssetTypeNumber ? kPrevious : kAssetPrevious,previous,2);
}
// 메뉴는 client SID여서 기본 flags 0에서도 전송하지 않는다. 서버 Form 전송 경로는 별도 복원이 필요하다.
void GumpFormHost::ValidatePop(Sid form,std::uint32_t flags) const {
    const auto state=Form(form)[kState];
    if (!(state&kVoid) || (state&(kDead|kTransmitting))) throw std::logic_error("GumpForm Pop 상태 오류");
    if (!pool_.IsServer()) return;
    if (form.value<pool_.Layout().serverFirst) {
        if (flags&kServerPop) throw std::logic_error("client GumpForm의 서버 Pop 금지");
        flags|=kClientPop;
    } else if (!(flags&kClientPop)) flags|=kServerPop;
    if (!(flags&(kClientPop|kDependentPop))) throw std::logic_error("서버 GumpForm의 Pop 전송은 미복원입니다");
}
// 004af160은 004ac800 thunk이고 CD 004b04e0도 같은 공통 부착을 호출한다. 부모 변경 debug 조건은 없다.
void GumpFormHost::Attach(Sid form,Sid parent,std::uint32_t flags) const {
    if (!parent.value) {
        if (pool_.Edition()==OriginalEdition::Patch1078) return;
        throw std::logic_error("CD GumpForm의 부모 0 부착은 지원하지 않습니다");
    }
    const auto raw=Form(form);
    if (types_.at(raw[kType]).flags2&kNoContainerGenus) throw std::logic_error("GumpForm contained genus 오류");
    if (parent.value<5 || parent.value==pool_.Layout().predictableFirst || parent.value>=pool_.Capacity()
        || (pool_.Slot(parent)[kState]&(kFree|kDead)) || parent==form)
        throw std::logic_error("GumpForm 부착 부모 상태 오류");
    ValidatePop(form,flags);
    const auto head=Word(parent,kHead);
    if (head==form.value || head>=pool_.Capacity() || (head && (head<5 || head==pool_.Layout().predictableFirst
        || (pool_.Slot(Sid{head})[kState]&kFree)))) throw std::logic_error("GumpForm 부모 종속 머리 오류");
    // 부모 기록→이전 머리를 next로 기록→이전 머리의 prev→부모 head→자신 prev→contained→Pop 순서다.
    Put(form,kParent,parent.value,2);Put(form,kNext,head,2);
    if (head) SetPrevious(Sid{head},form.value);
    Put(parent,kHead,form.value,2);Put(form,kPrevious,0,2);
    pool_.AllocatedBytes(form)[kState]|=kContained;
    pool_.AllocatedBytes(form)[kState]&=static_cast<std::uint8_t>(~kVoid);
    // GumpForm postPop은 증가시킨 공통 깊이를 즉시 감소시키는 빈 몸체이므로 외부 효과가 없다.
}
// 살아 있는 부모에서는 링크를 고치지만 부모 삭제 중에는 저장한 next로 계속 순회하게 체인을 유지한다.
void GumpFormHost::Unpop(Sid form) const {
    Form(form);SquidForms(pool_).Unpop(form);
}
// 부모 삭제 인자는 사용하지 않는다. UI 콜백 전 현재 소유 ID/부모를 모두 읽고 콜백 뒤 Form을 읽지 않는다.
void GumpFormHost::Notify(Sid form) const {
    const auto owner=Owner(form);if (!owner) return;
    const auto parent=Parent(form);
    if (!notice_) throw std::invalid_argument("GumpForm 소유 UI 알림 경계 필요");
    notice_(owner,parent);
}
// dead인 Form은 공통 몸체가 바로 반환한다. pre/post는 UI 삭제를 호출하지 않으며 반납은 기존 풀에 맡긴다.
void GumpFormHost::Destroy(Sid form,std::uint32_t flags) const {
    if (!IsGumpForm(form)) throw std::logic_error("GumpForm 삭제 타입 오류");
    destroy_.Destroy(form,flags,Hooks());
}
// ProcessForm/자산의 기존 분배를 바깥에서 감싸야 해당 부모의 종속 UI 알림에도 이 처리기가 도달한다.
SquidDestroyHooks GumpFormHost::Hooks() const {
    SquidDestroyHooks hooks;
    // 기존 월드의 선택 번호를 유지하며 연결이 없으면 선택 없음으로 답한다.
    hooks.selected=fallback_.selected ? fallback_.selected : std::function<Sid()>([] { return Sid{}; });
    // 공통 destroy가 실제 Unpop을 요청하면 GumpForm만 여기서 처리한다.
    hooks.unpopForm=[this](Sid sid) {
        if (IsGumpForm(sid)) Unpop(sid);
        else { if (!fallback_.unpopForm) throw std::invalid_argument("GumpForm 외 Unpop 경계 필요");fallback_.unpopForm(sid); }
    };
    // 종속 release는 부모 알림이며 종속 destroy는 같은 가상 분배로 재진입한다.
    hooks.emit=[this](const SquidDestroyEvent& event) {
        if (IsGumpForm(event.sid)) {
            switch (event.effect) {
            case SquidDestroyEffect::PreDestroy:destroy_.CompletePreDestroy();return;
            case SquidDestroyEffect::PostDestroy:destroy_.CompletePostDestroy();return;
            case SquidDestroyEffect::ReleaseDependent:Notify(event.sid);return;
            case SquidDestroyEffect::DestroyDependent:Destroy(event.sid,event.flags);return;
            default:break;
            }
        }
        if (!fallback_.emit) throw std::invalid_argument("GumpForm 외 삭제 효과 경계 필요");
        fallback_.emit(event);
    };
    return hooks;
}
}
