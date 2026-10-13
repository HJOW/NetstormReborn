// ContentForm은 부모 머리 앞의 일반 Form과 달리 같은 목록 그룹의 순서 번호로 삽입한다.
#include "o/ContentFormHost.h"
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
    if (types_.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("ContentForm 타입 판본/크기 오류");
}
// 공통 가상 표 검사를 사용하여 오염된 타입6 슬롯의 필드 변경을 거부한다.
void ContentFormHost::Form(Sid form) const {
    if (pool_.Slot(form)[kType]!=kContent) throw std::logic_error("ContentForm 타입 오류");
    forms_.Parent(form);
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
    return Word(sid,pool_.Slot(sid)[kType]<kFirstAssetTypeNumber ? kPrevious : kParent);
}
// 가상 setPrev의 실제 폭만 기록한다.
void ContentFormHost::SetPrevious(Sid sid,std::uint16_t previous) const {
    Put(sid,pool_.Slot(sid)[kType]<kFirstAssetTypeNumber ? kPrevious : kParent,previous);
}
// 기존 체인을 한 번 훑고 첫 순서 공백에 삽입한다. 다른 그룹과 일반 Form/자산은 건너뛴다.
void ContentFormHost::Attach(Sid form,Sid parent,std::uint32_t flags) const {
    if (form.value<5 || parent.value<5 || parent.value>=pool_.Capacity() || parent==form) throw std::logic_error("ContentForm 부착 번호 오류");
    if ((pool_.Slot(parent)[kState]|pool_.Slot(form)[kState])&(kFree|kDead)) {
        if (pool_.Edition()==OriginalEdition::Cd1072) return;
        throw std::logic_error("ContentForm dead/free 부착 금지");
    }
    Form(form);forms_.ValidatePop(form,flags);
    const auto stored=Word(form,kContentType);if (stored>=types_.size()) throw std::logic_error("ContentForm 생산 타입 오류");
    if (state_ && state_->inBattle && (types_[stored].flags2&0x200000) &&
        (types_.at(pool_.Slot(parent)[kType]).flags2&0x30000) && !captured_)
        throw std::invalid_argument("ContentForm 전투 사제 포획 알림 경계 필요");
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
    pool_.AllocatedBytes(form)[kState]|=kContained;forms_.Pop(form,flags|0x100);notice_(parent,form,1);
    // 부모 알림/전송이 바꾼 현재 내용물 타입·부모 genus·전투 상태를 다시 읽어 사제 포획 알림 조건을 평가한다.
    if (state_ && state_->inBattle && (types_.at(Word(form,kContentType)).flags2&0x200000) &&
        (types_.at(pool_.Slot(parent)[kType]).flags2&0x30000)) {
        if (state_->checkingLinks && !Linked(form)) throw std::logic_error("ContentForm 사제 포획 알림 연결 오류");
        if (Linked(form)) {
            if (!captured_) throw std::invalid_argument("ContentForm 전투 사제 포획 알림 경계 필요");
            captured_(parent,pool_.Slot(form)[12]&0x7f);
        }
    }
}
// 부모의 dead/free 판정과 알림 시점은 공통 처리기에서 수행한다.
void ContentFormHost::Unpop(Sid form) const { Form(form);forms_.Unpop(form); }
// WORLD 플래그의 원본 assert 경로를 명시적으로 거부한다.
bool ContentFormHost::Robust(Sid form) const {
    Form(form);const auto flags=Word(form,kFlags);if (flags&kWorld) throw std::logic_error("WORLD ContentForm Robust 금지");
    return (flags&kRobust)!=0;
}
// 전체 체인을 검사해야 자기 번호 뒤의 무효 next가 있어도 false로 끝나는 원본 동작을 보존한다.
bool ContentFormHost::Linked(Sid form) const {
    Form(form);const Sid parent{Word(form,kParent)};
    if (!parent.value || parent.value>=pool_.Capacity()) throw std::logic_error("ContentForm 연결 검사 부모 번호 오류");
    Sid current{Word(parent,kHead)};bool found=false;std::unordered_set<std::uint16_t> visited;
    // 상태/타입 필터를 추가하지 않고 부모 체인의 실제 next만 따라간다.
    while (current.value) {
        if (current.value>=pool_.Capacity()) return false;
        if (!visited.insert(current.value).second) throw std::logic_error("ContentForm 연결 검사 체인 순환");
        if (current==form) found=true;
        current=Sid{Word(current,kNext)};
    }
    return found;
}
// 재부착은 현재 부모를 Unpop 전에 저장하며 제거 알림 뒤의 현재 플래그를 읽어 0x100만 끈다.
float ContentFormHost::Damage(Sid form,std::int32_t damage,float amount) const {
    Form(form);if (damage<=0) return amount;
    const auto parent=forms_.Parent(form);Unpop(form);Put(form,kFlags,static_cast<std::uint16_t>(Word(form,kFlags)&~kChanged));
    Attach(form,parent,0x10);return 0;
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
