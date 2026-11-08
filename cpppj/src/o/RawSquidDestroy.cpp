// 종속/파생 효과 뒤 필드를 다시 읽고 기존 SquidUnpop·SidPool::Release를 실제로 호출한다.
#include "o/RawSquidDestroy.h"
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <algorithm>
#include "o/SquidFactory.h"
#include "o/SquidForms.h"

namespace netstorm::o {
namespace {
// 두 판본의 공통 raw 위치와 dead/free/void/contained 비트다.
constexpr std::size_t kType=10,kState=11,kNext=4,kHead=6;
constexpr std::uint8_t kFree=1,kDead=2,kVoid=4,kContained=8;
// 원본 서버/전파/종속 삭제 flags다. 서버의 client SID는 전파 억제 비트를 자동으로 추가한다.
constexpr std::uint32_t kServer=8,kClient=0x10,kDependent=0x40;
}
// 기존 공간/수명 상태를 공유하는 어댑터만 허용한다.
RawSquidDestroy::RawSquidDestroy(SidPool& pool,SquidUnpop& unpop,std::span<const RiftTypeRecord> types)
    :pool_(pool),unpop_(unpop),types_(types) {
    if (&unpop_.Pool()!=&pool_) throw std::invalid_argument("Destroy pool mismatch");
}
// 커서로 저장할 WORD는 콜백보다 먼저 읽는다.
std::uint16_t RawSquidDestroy::Word(Sid sid,std::size_t offset) const {
    const auto raw=pool_.Slot(sid);return static_cast<std::uint16_t>(raw[offset]|(static_cast<std::uint16_t>(raw[offset+1])<<8));
}
// 원본 카운터는 DWORD 감김을 허용한다. 이 완료 호출만으로 공통 훅 전체를 복원했다고 보지 않는다.
void RawSquidDestroy::CompletePreDestroy() { --preDepth_; }
// 공통 postDestroy가 정상적으로 끝났음을 상위 destroy에 돌려준다.
void RawSquidDestroy::CompletePostDestroy() { --postDepth_; }
// 카운터는 원본 주소/호스트 포인터와 독립적이다.
std::uint32_t RawSquidDestroy::PreDepth() const { return preDepth_; }
// 현재 postDestroy 카운터를 읽는다.
std::uint32_t RawSquidDestroy::PostDepth() const { return postDepth_; }
// 공통 삭제/훅 어댑터의 풀 소유자를 읽기 전용으로 제공한다.
const SidPool& RawSquidDestroy::Pool() const { return pool_; }
// 실제 공간 해제 어댑터가 사용하는 해시/spot에 대조한다.
void RawSquidDestroy::ValidateGraph(const RawGraph& graph) const { unpop_.ValidateGraph(graph); }
// 장치 연결은 상태를 변경하지 않으며 CD를 PATCH 몸체로 실행하는 연결만 거부한다.
void RawSquidDestroy::BindPatchDiagnostics(SquidDestroyDiagnostics diagnostics) {
    if (pool_.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("PATCH 공통 삭제 진단 판본 불일치");
    patchDiagnostics_=std::move(diagnostics);
}
// 원문의 client SID는 예약0..4를 제외한다. 현재 풀의 서버 경계를 호출 위치마다 읽는다.
bool RawSquidDestroy::IsClientSid(Sid sid) const { return sid.value>=5 && sid.value<pool_.Layout().serverFirst; }
// 원문 단언은 throw로 바꾸지 않으며, 정상 복귀하면 caller가 현재 상태를 다시 조회한다.
void RawSquidDestroy::AssertPatch(std::string_view expression,std::uint32_t line,std::string_view file) const {
    const auto assertion=patchDiagnostics_->assertion;
    if (!assertion) throw std::logic_error("PATCH 공통 삭제의 현재 진단 장치 미연결");
    assertion(expression,file,line);
}
// 원문 반납은 예약/free/void 조건을 효과 전 throw로 바꾸지 않고 두 단언 뒤 현재 TYPE를 기록한다.
void RawSquidDestroy::ReleasePatch(Sid sid) {
    if (!(pool_.Slot(sid)[kState]&kVoid)) AssertPatch("p->isVoid()",0x15c);
    if (pool_.Slot(sid)[kState]&kFree) AssertPatch("!p->isFree()",0x15d);
    pool_.Record(sid,pool_.Slot(sid)[kType]);
    const auto type=pool_.Slot(sid)[kType];
    const auto offset=pool_.Offset(sid);
    std::fill_n(pool_.bytes_.begin()+offset,pool_.layout_.stride,std::uint8_t{});
    pool_.bytes_[offset+kType]=type;
    if (!pool_.IsServer() && sid.value>=pool_.Layout().serverFirst) {
        pool_.bytes_[offset+kState]|=kFree;return;
    }
    if (sid.value>=5 && sid.value<pool_.Layout().predictableFirst) {
        const auto list=sid.value<pool_.Layout().serverFirst ? 0U : 1U;
        const Sid tail{pool_.tails_[list]};
        if (tail==sid) AssertPatch("n != getSid()",0x123,"c:\\ns\\o\\squid.h");
        pool_.WriteNext(pool_.Offset(tail),sid.value);
        pool_.tails_[list]=sid.value;
        pool_.WriteNext(pool_.Offset(sid),0);
        ++pool_.freeCount_;
    }
    pool_.bytes_[pool_.Offset(sid)+kState]|=kFree|kDead;
}
// 실제 PATCH 경로는 upfront typed 선검사 대신 원문이 선택한 위치에서 현재 필드와 장치를 읽는다.
void RawSquidDestroy::DestroyPatch(Sid sid,std::uint32_t flags,const SquidDestroyHooks& hooks) {
    const auto initial=pool_.Slot(sid)[kState];
    if (initial&kDead) return;
    pool_.bytes_[pool_.Offset(sid)+kState]=static_cast<std::uint8_t>(initial|kDead);
    if (initial&kFree) AssertPatch("!isFree()",0x24d);
    if (pool_.Slot(sid)[kState]&kFree) return;
    if (pool_.IsServer()) {
        if (IsClientSid(sid)) flags|=kClient;
    } else if (!(flags&kServer)) {
        if (!IsClientSid(sid)) AssertPatch("isClientSid()",0x269);
        if (!IsClientSid(sid)) return;
    }
    const auto pre=preDepth_++;
    const auto type=pool_.Slot(sid)[kType];
    if (type>=types_.size()) throw std::out_of_range("PATCH 공통 삭제의 현재 로그 TYPE 범위 오류");
    // 원문 TYPE의 char 배열처럼 첫 NUL까지 사용하며 자산 이름의 원래 바이트 인코딩을 보존한다.
    const auto name=std::string_view(types_[type].name).substr(0,types_[type].name.find('\0'));
    const auto text=std::string("destroying ")+std::string(name)+" "+std::to_string(sid.value)+"\n";
    const auto trace=patchDiagnostics_->trace;
    if (!trace) throw std::logic_error("PATCH 공통 삭제의 현재 로그 장치 미연결");
    trace(0x27,text);
    // 각 실제 가상 효과를 선택한 순간 콜백을 빌리므로 앞선 진단/로그 효과를 선검사로 없애지 않는다.
    const auto emit=[&hooks](const SquidDestroyEvent& event) {
        const auto current=hooks.emit;
        if (!current) throw std::logic_error("PATCH 공통 삭제의 현재 가상 효과 미연결");
        current(event);
    };
    emit({SquidDestroyEffect::PreDestroy,sid,{},flags});
    if (preDepth_!=pre) AssertPatch("baseDestroy == curDestroy",0x276);
    Sid dependent{Word(sid,kHead)};
    std::unordered_set<std::uint16_t> visited; // 원문에 없는 손상 순환은 이미 수행한 효과 뒤 호스트 경계에서만 중단한다.
    // 현재 head부터 순서대로 처리하며 전체 체인 선검사나 스냅샷을 추가하지 않는다.
    while (dependent.value) {
        if (!visited.insert(dependent.value).second) throw std::logic_error("PATCH 공통 삭제의 종속 순환은 호스트에서 지원하지 않습니다");
        if (pool_.Slot(dependent)[kState]&kFree) AssertPatch("!s.isFree()",0x27d);
        if (!(pool_.Slot(dependent)[kState]&kContained) && pool_.Slot(dependent)[kType]>=kFirstAssetTypeNumber)
            AssertPatch("s.isContained() || s.isForm()",0x27e);
        const bool free=bool(pool_.Slot(dependent)[kState]&kFree);
        const Sid next{Word(dependent,kNext)}; // 004af8fd..905는 free 조건을 읽고 next를 저장한 뒤 두 번째 free 단언을 실행한다.
        if (free) AssertPatch("!s.isFree()",0x280);
        emit({SquidDestroyEffect::ReleaseDependent,dependent,sid,0});
        if (!(pool_.Slot(dependent)[kState]&kFree)) emit({SquidDestroyEffect::DestroyDependent,dependent,sid,flags|kDependent});
        dependent=next;
    }
    if (pool_.Slot(sid)[kState]&kFree) AssertPatch("!isFree()",0x28f);
    if (pool_.Slot(sid)[kState]&kFree) return;
    if (!(flags&(kClient|kDependent)) && pool_.IsServer() && !IsClientSid(sid))
        emit({SquidDestroyEffect::Transmit,sid,{},flags|kServer});
    const auto selected=hooks.selected;const auto selectedRaw=hooks.selectedRaw;
    if (!selectedRaw && !selected) throw std::logic_error("PATCH 공통 삭제의 현재 선택 조회 미연결");
    if ((selectedRaw ? selectedRaw():std::uint32_t(selected().value))==sid.value) emit({SquidDestroyEffect::ClearSelection,sid,{},0}); //현재DWORD를조회한뒤비교하며고비트를선행절삭하거나별도물리예외로바꾸지않는다.
    if (!sid.value || sid.value>=pool_.Capacity()) AssertPatch("isValid()",0x2a2);
    const auto raw=pool_.Slot(sid);
    if (!(raw[kState]&kVoid)) {
        const auto table=std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);
        const SquidForms forms(pool_);
        bool form=false; // 현재 가상+48도 raw TYPE가 아니라 실제 네 Form VT로 선택한다.
        for (const auto formType:{1U,2U,3U,6U}) if (table==forms.Vtable(formType)) { form=true;break; } // 네 실제 클래스의 Unpop만 Form 호스트에 위임한다.
        if (form) {
            const auto unpop=hooks.unpopForm;
            if (!unpop) throw std::logic_error("PATCH 공통 삭제의 현재 Form Unpop 미연결");
            unpop(sid); // 원문 가상+48 뒤에 새 void 결과 단언을 추가하지 않는다.
        } else {
            if (!SquidFactory::SupportsConstructedVtable(pool_.Edition(),table)) throw std::logic_error("PATCH 공통 삭제의 현재 가상+48 미지 VT");
            if (raw[kType]>=types_.size()) throw std::out_of_range("PATCH 공통 삭제의 현재 Unpop TYPE 범위 오류");
            unpop_.Unpop(sid,types_[raw[kType]],0);
        }
    }
    const auto post=postDepth_++;
    emit({SquidDestroyEffect::PostDestroy,sid,{},flags});
    if (postDepth_!=post) AssertPatch("curPostDestroy == basePostDestroy",0x2a9);
    ReleasePatch(sid); // 실제 동일 Pool을 원문 단언 복귀 순서로 반납하며 독립 typed SidPool.Release는 변경하지 않는다.
}
// 원본은 dead를 먼저 켜므로 중첩된 같은 객체 삭제는 모든 외부 효과를 건너뛴다.
void RawSquidDestroy::Destroy(Sid sid,std::uint32_t flags,const SquidDestroyHooks& hooks) {
    if (patchDiagnostics_) { DestroyPatch(sid,flags,hooks);return; }
    if (sid.value<5 || sid.value==pool_.Layout().predictableFirst || sid.value>=pool_.Capacity())
        throw std::out_of_range("Destroy SID");
    auto raw=pool_.Slot(sid);
    if (raw[kState]&kDead) return;
    // 자산 루트는 contained가 아니어야 한다. form 루트는 부모 안에 들어 있으며 전용 Unpop 훅이 필요하다.
    const bool form=raw[kType]<kFirstAssetTypeNumber;
    if ((raw[kState]&kFree) || raw[kType]>=types_.size() || (form ? !hooks.unpopForm : (raw[kState]&kContained)!=0))
        throw std::logic_error("Destroy unsupported root");
    if (!hooks.emit || (!hooks.selected && !hooks.selectedRaw)) throw std::invalid_argument("Destroy hooks");
    const bool client=sid.value<pool_.Layout().serverFirst;
    // 패치 원본의 assert 앞 부분 변경은 새 코드에서 피한다. 서버 객체의 수신 삭제는 flag 8이 필요하다.
    if (!pool_.IsServer() && !client && !(flags&kServer)) throw std::logic_error("Destroy server authority");
    if (pool_.IsServer() && client) flags|=kClient;
    pool_.AllocatedBytes(sid)[kState]|=kDead;
    const auto pre=preDepth_++;
    hooks.emit({SquidDestroyEffect::PreDestroy,sid,{},flags});
    if (preDepth_!=pre) throw std::logic_error("Destroy pre completion");
    Sid dependent{Word(sid,kHead)};std::unordered_set<std::uint16_t> visited;
    // preDestroy가 바꾼 현재 head부터 처리하며 각 next는 종속 release 전에 저장한다.
    while (dependent.value) {
        if (dependent.value>=pool_.Capacity() || !visited.insert(dependent.value).second)
            throw std::logic_error("Destroy dependent chain");
        const auto child=pool_.Slot(dependent);
        if ((child[kState]&kFree) || (!(child[kState]&kContained) && child[kType]>=kFirstAssetTypeNumber))
            throw std::logic_error("Destroy dependent state");
        const Sid next{Word(dependent,kNext)};
        hooks.emit({SquidDestroyEffect::ReleaseDependent,dependent,sid,0});
        if (!(pool_.Slot(dependent)[kState]&kFree))
            hooks.emit({SquidDestroyEffect::DestroyDependent,dependent,sid,flags|kDependent});
        dependent=next;
    }
    if (pool_.Slot(sid)[kState]&kFree) throw std::logic_error("Destroy root released during pre");
    if (!(flags&(kClient|kDependent)) && pool_.IsServer() && !client)
        hooks.emit({SquidDestroyEffect::Transmit,sid,{},flags|kServer});
    if ((hooks.selectedRaw ? hooks.selectedRaw():std::uint32_t(hooks.selected().value))==sid.value) hooks.emit({SquidDestroyEffect::ClearSelection,sid,{},0}); //기존typed Sid와새Native DWORD의동일시점비교를구별한다.
    raw=pool_.Slot(sid);
    // 파생 효과 뒤 현재 타입/void를 사용한다. 실제 공간 해제는 flags 0으로 호출한다.
    if (!(raw[kState]&kVoid)) {
        if (raw[kType]>=types_.size()) throw std::out_of_range("Destroy live type");
        if (raw[kType]<kFirstAssetTypeNumber) {
            // form의 가상 Unpop은 void를 켜고 부모의 종속 체인에서 빠진다.
            if (!hooks.unpopForm) throw std::logic_error("Destroy form unpop hook");
            hooks.unpopForm(sid);
            if (!(pool_.Slot(sid)[kState]&kVoid)) throw std::logic_error("Destroy form unpop result");
        } else unpop_.Unpop(sid,types_[raw[kType]],0);
    }
    const auto post=postDepth_++;
    hooks.emit({SquidDestroyEffect::PostDestroy,sid,{},flags});
    if (postDepth_!=post) throw std::logic_error("Destroy post completion");
    pool_.Release(sid);
}
}
