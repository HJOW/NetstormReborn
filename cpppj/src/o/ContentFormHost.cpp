// ContentForm은 부모 머리 앞의 일반 Form과 달리 같은 목록 그룹의 순서 번호로 삽입한다.
#include "o/ContentFormHost.h"
#include "o/RawContentAttach.h"
#include "o/RawFormLinked.h" // 같은 공통 부모의 전체 체인 검사를 재사용한다.
#include "o/RawSquidContainedUnpop.h"
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace netstorm::o {
namespace {
// 두 판본의 공통 링크·타입·상태·생산 타입·목록 플래그 위치다.
constexpr std::size_t kNext=4,kHead=6,kType=10,kState=11,kPrevious=14,kParent=16,kContentType=18,kFlags=20;
// 타입6, free/dead/contained와 비교할 때 제외하는 플래그다.
constexpr std::uint8_t kContent=6,kFree=1,kDead=2,kContained=8;
constexpr std::uint16_t kGroupMask=0xfcff,kWorld=0x80,kRobust=0x34,kChanged=0x100;
}
// 부모 제거 통지도 같은 알림 경계에 연결하여 Take/Unpop과 부착을 한 계약으로 맞춘다.
ContentFormHost::ContentFormHost(SidPool& pool,std::span<const RiftTypeRecord> types,ContentFormNotice notice,FormPopTransmit transmit,
    const ContentFormState* state,ContentPriestNotice captured)
    :pool_(pool),types_(types.begin(),types.end()),notice_(std::move(notice)),
     forms_(pool,[this](Sid parent,Sid form) { notice_(parent,form,2); },std::move(transmit)),state_(state),captured_(std::move(captured)) {
    if (!notice_) throw std::invalid_argument("ContentForm 부모 알림 경계 필요");
    if (state_) { FormUnpopHooks hooks;hooks.assertion=state_->assertion;forms_.BindUnpop(std::move(hooks)); } // 독립 입력도 제공된 원문 진단의 정상 복귀를 허용한다.
    if (types_.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("ContentForm 타입 판본/크기 오류");
}
// 선택한 원문 진단의 복귀 뒤 현재 전역과 슬롯 판정을 계속한다.
void ContentFormHost::Assert(std::string_view expression,std::uint32_t line) const {
    if (!state_ || !state_->assertion) throw std::logic_error("ContentForm 실제 원문 진단 장치 미연결");
    state_->assertion(expression,"\\Ns\\O\\Squid.cpp",line);
}
// 정렬되지 않은 WORD를 little endian으로 읽는다.
std::uint16_t ContentFormHost::Word(Sid sid,std::size_t offset) const {
    const auto raw=pool_.Slot(sid);return static_cast<std::uint16_t>(raw[offset]|(std::uint16_t(raw[offset+1])<<8));
}
// 부모·링크·플래그의 기존 이웃 payload를 보존한다.
void ContentFormHost::Put(Sid sid,std::size_t offset,std::uint16_t value) const {
    auto raw=pool_.AllocatedBytes(sid);raw[offset]=static_cast<std::uint8_t>(value);raw[offset+1]=static_cast<std::uint8_t>(value>>8);
}
// Form과 자산은 부모/previous WORD 위치가 서로 반대다.
std::uint16_t ContentFormHost::Previous(Sid sid) const {
    return CommonContainedPrevious(pool_,sid).value;
}
// 가상 setPrev의 실제 폭만 기록한다.
void ContentFormHost::SetPrevious(Sid sid,std::uint16_t previous) const {
    CommonContainedSetPrevious(pool_,sid,Sid{previous});
}
// 기존 체인을 한 번 훑고 첫 순서 공백에 삽입한다. 다른 그룹과 일반 Form/자산은 건너뛴다.
void ContentFormHost::Attach(Sid form,Sid parent,std::uint32_t flags) const {
    if (pool_.Edition()==OriginalEdition::Patch1078) {
        ContentAttachHooks hooks;hooks.contentType=[this] { const auto getter=state_ ? state_->contentType : std::function<std::uint32_t()>{};return getter ? getter() : 6U; };
        hooks.previous=[this](Sid sid,std::uint32_t table) { return CommonContainedPreviousAtTable(pool_,sid,table); };
        hooks.setPrevious=[this](Sid sid,std::uint32_t table,Sid value) { CommonContainedSetPreviousAtTable(pool_,sid,table,value); };
        hooks.setParent=[this](Sid sid,std::uint32_t,Sid value) { CommonContainedSetParent(pool_,sid,value); };
        hooks.pop=[this](Sid sid,std::uint32_t value) { if (pop_) { const auto selected=pop_;selected(sid,value); }else forms_.Pop(sid,value); };
        hooks.notice=notice_;hooks.assertion=state_ ? state_->assertion : std::function<void(std::string_view,std::string_view,std::uint32_t)>{};
        if (!AttachSortedContent(pool_,form,parent,flags,hooks)) return;
    }
    else { // CD의 기존 정렬 한정 계약은 유지한다.
    if (form.value<5 || parent.value<5 || parent.value>=pool_.Capacity() || parent==form) throw std::logic_error("ContentForm 부착 번호 오류");
    if ((pool_.Slot(parent)[kState]|pool_.Slot(form)[kState])&(kFree|kDead)) return;
    forms_.Parent(form);forms_.ValidatePop(form,flags);
    const auto stored=Word(form,kContentType);if (stored>=types_.size()) throw std::logic_error("ContentForm 생산 타입 오류");
    const auto listFlags=Word(form,kFlags);const auto orderOffset=pool_.Edition()==OriginalEdition::Patch1078 ? 23U : 22U;
    std::uint32_t order=0;Sid previous{},current{Word(parent,kHead)};bool inserted=false;std::unordered_set<std::uint16_t> visited;
    // 일치하는 그룹만 순서 카운터를 증가시키며 모든 항목은 tail 후보로 기억한다.
    while (current.value) {
        if (current.value>=pool_.Capacity() || current==form || !visited.insert(current.value).second || (pool_.Slot(current)[kState]&kFree))
            throw std::logic_error("ContentForm 부착 체인 손상");
        const auto raw=pool_.Slot(current);
        if (raw[kType]==kContent && ((Word(current,kFlags)^listFlags)&kGroupMask)==0) {
            if (order<raw[orderOffset]) {
                Put(form,kNext,current.value);SetPrevious(form,Previous(current));SetPrevious(current,form.value);
                const Sid before{Previous(form)};
                if (before.value) Put(before,kNext,form.value);else Put(parent,kHead,form.value);
                inserted=true;break;
            }
            ++order;
        }
        previous=current;current=Sid{Word(current,kNext)};
    }
    if (!inserted) {
        if (previous.value) Put(previous,kNext,form.value);else Put(parent,kHead,form.value);
        Put(form,kNext,0);SetPrevious(form,previous.value);
    }
    pool_.AllocatedBytes(form)[orderOffset]=static_cast<std::uint8_t>(order);Put(form,kParent,parent.value);
    pool_.AllocatedBytes(form)[kState]|=kContained;if (pop_) pop_(form,flags|0x100);else forms_.Pop(form,flags|0x100);notice_(parent,form,1);
    }
    // 부모 알림/전송이 바꾼 현재 내용물 타입·부모 genus·전투 상태를 다시 읽어 사제 포획 알림 조건을 평가한다.
    if ((TypeGenus(Word(form,kContentType))&0x200000) &&
        (TypeGenus(pool_.Slot(parent)[kType])&0x30000) && state_ && state_->inBattle) {
        if (state_->checkingLinks) {
            if (form.value<1 || form.value>=pool_.Capacity()) Assert("isValid()",0xc2b);
            if (state_->checkingLinks && !Linked(form)) Assert("isRobust()",0xc2c);
        }
        if (form.value>=1 && form.value<pool_.Capacity() && Linked(form)) {
            if (!captured_) throw std::invalid_argument("ContentForm 전투 사제 포획 알림 경계 필요");
            captured_(parent,pool_.Slot(form)[12]&0x7f);
        }
    }
}
// 부모의 dead/free 판정과 알림 시점은 공통 처리기에서 수행한다.
void ContentFormHost::Unpop(Sid form) const { if (unpop_) unpop_(form);else forms_.Unpop(form); } // 실제 가상 진입 앞에 고정 TYPE/free 검사를 추가하지 않는다.
void ContentFormHost::BindPop(std::function<void(Sid,std::uint32_t)> pop) { pop_=std::move(pop); } // 같은 Session이 살아 있는 동안 현재 실제 Pop만 사용한다.
void ContentFormHost::BindUnpop(std::function<void(Sid)> unpop) { unpop_=std::move(unpop); } // 현재 World와 같은 수명의 공통 해제를 연결한다.
void ContentFormHost::BindReattach(std::function<void(Sid,Sid,std::uint32_t)> reattach) { reattach_=std::move(reattach); } // Regular의 Unpop 뒤 현재 VT+50을 선택할 실제 장치를 연결한다.
// 실제 현재 TYPE 대여는 통지 뒤에도 같은 표를 다시 읽는다.
void ContentFormHost::BindCurrentTypes(std::span<const RiftTypeRecord> types) { if (!types.empty() && types.size()!=types_.size()) throw std::invalid_argument("Content 현재 TYPE 표의 판본/길이 계약");currentTypes_=types; }
std::uint32_t ContentFormHost::TypeGenus(std::uint32_t type) const { const auto current=currentTypes_.empty() ? std::span<const RiftTypeRecord>(types_) : currentTypes_;if (type>=current.size()) throw std::out_of_range("Content 통지 뒤 현재 TYPE 물리 범위");return current[type].flags2; } // 실제 소비 위치의 오류만 보고한다.
// WORLD 진단이 정상 복귀하면 현재 flags를 다시 읽으며 TYPE/상태 필터를 합성하지 않는다.
bool ContentFormHost::Robust(Sid form) const {
    if (Word(form,kFlags)&kWorld) {
        const auto assertion=state_ ? state_->assertion : std::function<void(std::string_view,std::string_view,std::uint32_t)>{}; // 장치가 자기 연결을 반납해도 선택한 callable은 복귀까지 유지한다.
        if (!assertion) throw std::logic_error("ContentForm Robust의 선택된 원문 진단 장치 미연결");const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
        assertion("!(getListFlags() & CFF_WORLD)",patch ? "\\Ns\\O\\Squid.cpp":"\\ns\\o\\Squid.cpp",patch ? 0xbb1U:0xb68U);
    }
    return (Word(form,kFlags)&kRobust)!=0;
}
// 현재 실제 부모 getter를 한 번 읽고 자기 뒤의 범위 실패까지 기존 공통 부모에서 검사한다.
bool ContentFormHost::Linked(Sid form) const {
    return CheckRawFormLinked(pool_,form,[this](Sid current) { return CommonContainedParent(pool_,current); }); // 예약 부모0도 실제 물리 head만 읽으며 TYPE/free/dead를 별도로 거르지 않는다.
}
// 원본 Regular+5c는 양수 count에서만 현재 getter→Unpop→flags 쓰기→현재 가상 재부착을 실행한다.
float ContentFormHost::Damage(Sid form,std::int32_t damage,float amount) const {
    if (damage<=0) return amount;
    const auto parent=CommonContainedParent(pool_,form);Unpop(form);const auto current=pool_.Slot(form);auto* bytes=const_cast<std::uint8_t*>(current.data());
    if (pool_.Edition()==OriginalEdition::Patch1078) bytes[kFlags+1]&=0xfeU; // PATCH는 Unpop 뒤 현재 high BYTE의 bit0만 끈다.
    else { const auto flags=Word(form,kFlags)&~kChanged;bytes[kFlags]=static_cast<std::uint8_t>(flags);bytes[kFlags+1]=static_cast<std::uint8_t>(flags>>8); } // CD는 같은 효과를 WORD store로 남긴다.
    const auto reattach=reattach_; // Unpop이 장치를 바꿀 수 있으므로 실제 소비 위치에서 선택하고 수명을 유지한다.
    if (reattach) reattach(form,parent,0x10);
    else { const auto raw=pool_.Slot(form);const auto table=std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);
        if (table!=forms_.Vtable(6)) throw std::logic_error("ContentForm 현재 가상 재부착 장치 미연결");Attach(form,parent,0x10); // 독립 기존 Content 호출만 보존하고 다른 클래스의 +50을 임의 정렬 부착으로 대체하지 않는다.
    }
    return 0;
}
// RawContainedCreate의 원래 생성·필드 쓰기 순서를 유지하면서 마지막 부착만 실제 처리기로 바꾼다.
ContainedCreateHooks ContentFormHost::BindCreate(SquidFactory& factory,ContainedCreateHooks hooks) const {
    hooks=MakeContainedCreateHooks(pool_,factory,std::move(hooks));
    // 표면 별도 알림은 호출자의 기존 경계를 유지하고 ContentForm 부착만 같은 저장소를 사용한다.
    hooks.attach=[this](Sid form,Sid parent,std::uint32_t flags) { Attach(form,parent,flags); };return hooks;
}
// 외부 분배기의 풀 소유권 대조에 사용한다.
const SidPool& ContentFormHost::Pool() const { return pool_; }
}
