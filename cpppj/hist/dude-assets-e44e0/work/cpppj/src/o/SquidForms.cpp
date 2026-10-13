// Form 수신은 기존 바이트를 보존하고 공통 Unpop은 부모 삭제 중의 체인을 유지한다.
#include "o/SquidForms.h"
#include <array>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 네 내장 Form 타입과 실제 생성자가 기록하는 가상 표의 판본별 값이다.
constexpr std::array<std::uint32_t,4> kTypes{1,2,3,6};
constexpr std::array<std::uint32_t,4> kPatchVtables{0x512110,0x512098,0x512188,0x512208};
constexpr std::array<std::uint32_t,4> kCdVtables{0x505a08,0x505990,0x505a80,0x505b00};
// 두 판본의 Form 공통 필드와 contained 자산의 이전 필드다.
constexpr std::size_t kNext=4,kHead=6,kType=10,kState=11,kPrevious=14,kParent=16,kAssetPrevious=16;
// free/dead/void/contained 상태와 ProcessForm·ContentForm 타입 및 프로세스 타입 범위다.
constexpr std::uint8_t kFree=1,kDead=2,kVoid=4,kContained=8;
constexpr std::uint32_t kProcessForm=2,kContentForm=6,kProcessFirst=10,kPatchProcessEnd=63,kCdProcessEnd=62;
}
// 알림을 보관하되 다른 객체 수명이나 풀 초기화 효과는 숨기지 않는다.
SquidForms::SquidForms(SidPool& pool,FormRemovalNotice notice,FormPopTransmit transmit)
    :pool_(pool),notice_(std::move(notice)),transmit_(std::move(transmit)) {}
// 타입 검사는 Allocate/Take로 슬롯을 바꾸기 전에 끝낸다.
std::uint32_t SquidForms::Vtable(std::uint32_t type) const {
    // 고정된 네 타입 외의 생성자 fallback은 자산 factory의 책임이다.
    for (std::size_t index=0;index<kTypes.size();++index)
        if (kTypes[index]==type) return (pool_.Edition()==OriginalEdition::Patch1078 ? kPatchVtables : kCdVtables)[index];
    throw std::invalid_argument("미복원 내장 Form 생성자 타입");
}
// 번호 범위만 확인하여 원본 Take의 free 슬롯 및 예측 머리 쓰기를 허용한다.
std::span<std::uint8_t> SquidForms::Raw(Sid sid) const {
    return std::span(pool_.bytes_).subspan(pool_.Offset(sid),pool_.layout_.stride);
}
// 원본 WORD/DWORD 필드의 실제 바이트를 낮은 순서부터 읽는다.
std::uint32_t SquidForms::Read(Sid sid,std::size_t offset,std::size_t width) const {
    const auto raw=pool_.Slot(sid);std::uint32_t value=0;
    // 필드 폭 밖의 payload는 읽지 않는다.
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);
    return value;
}
// 생성자는 DWORD 하나만 쓰고 연결 해제는 WORD/상태 BYTE만 바꾼다.
void SquidForms::Put(Sid sid,std::size_t offset,std::uint32_t value,std::size_t width) const {
    auto raw=Raw(sid);
    // 지정한 쓰기 폭을 그대로 보존한다.
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 프로세스가 자신의 타입 번호로 바꾼 Form도 생성자 가상 표로 확인한다.
std::span<const std::uint8_t> SquidForms::Form(Sid sid) const {
    const auto raw=pool_.Slot(sid);const auto type=raw[kType];
    if (sid.value<5 || (raw[kState]&kFree)) throw std::logic_error("Form 슬롯 상태 오류");
    const auto end=pool_.Edition()==OriginalEdition::Patch1078 ? kPatchProcessEnd : kCdProcessEnd;
    const auto constructorType=type>=kProcessFirst && type<end ? kProcessForm : type;
    if (Read(sid,0,4)!=Vtable(constructorType)) throw std::logic_error("Form 가상 표 미복원");
    return raw;
}
// 원본 Form의 부모 getter는 WORD 전체를 반환한다.
Sid SquidForms::Parent(Sid form) const { Form(form);return Sid{static_cast<std::uint16_t>(Read(form,kParent,2))}; }
// 원본 Form의 이전 getter도 WORD 전체를 반환한다.
Sid SquidForms::Previous(Sid form) const { Form(form);return Sid{static_cast<std::uint16_t>(Read(form,kPrevious,2))}; }
// Allocate가 지운 슬롯에 생성자 가상 표→타입만 쓰며 자산 초기화는 호출하지 않는다.
Sid SquidForms::Create(std::uint32_t type,std::uint32_t flags) const {
    const auto vtable=Vtable(type);const auto sid=pool_.Allocate(flags);
    Put(sid,0,vtable,4);Raw(sid)[kType]=static_cast<std::uint8_t>(type);return sid;
}
// 사용 중인 같은 Form의 가상 Unpop 후 기존 payload에 생성자와 빈 postTake를 적용한다.
Sid SquidForms::Take(std::uint32_t type,Sid sid) const {
    const auto vtable=Vtable(type);
    if (sid.value<pool_.Layout().serverFirst) throw std::out_of_range("Form Take는 서버 영역 SID만 받습니다");
    const auto state=pool_.Slot(sid)[kState];
    if (!(state&kFree)) {
        if (pool_.Slot(sid)[kType]!=type) throw std::logic_error("Form Take 타입 불일치");
        Unpop(sid);
    }
    // 해제 알림이 바꾼 현재 슬롯을 다시 읽어 free/dead만 끄고 나머지 상태를 보존한다.
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (!patch) Raw(sid)[kState]&=static_cast<std::uint8_t>(~kFree);
    Raw(sid)[kState]&=static_cast<std::uint8_t>(~(kFree|kDead));
    Put(sid,0,vtable,4);
    // CD는 void→타입 순서이며 패치는 타입→void 순서다. 두 판본의 빈 postTake 뒤 결과는 같다.
    if (patch) Raw(sid)[kType]=static_cast<std::uint8_t>(type);
    Raw(sid)[kState]|=kVoid;
    if (!patch) Raw(sid)[kType]=static_cast<std::uint8_t>(type);
    return sid;
}
// client 번호/현재 권한에 따라 서버·로컬 플래그를 보완한다. 원본 assert 경로는 상태 변경 전에 거부한다.
std::uint32_t SquidForms::ValidatePop(Sid form,std::uint32_t flags) const {
    const auto state=Form(form)[kState];
    if (!(state&kVoid) || (state&(kDead|0x10))) throw std::logic_error("Form Pop 상태 오류");
    if (pool_.IsServer()) {
        if (form.value<pool_.Layout().serverFirst) {
            if (flags&8) throw std::logic_error("client Form 서버 Pop 금지");
            flags|=0x10;
        } else if (!(flags&0x10)) flags|=8;
        if (!(flags&0x50) && !transmit_) throw std::invalid_argument("Form 서버 Pop 전송 경계 필요");
    } else if (!(flags&8)) flags|=0x10;
    return flags;
}
// 004ac560/CD 004abfb0: void 해제→필요한 서버 전송→빈 Form postPop 순서를 유지한다.
void SquidForms::Pop(Sid form,std::uint32_t flags) const {
    flags=ValidatePop(form,flags);Raw(form)[kState]&=static_cast<std::uint8_t>(~kVoid);
    if (pool_.IsServer() && !(flags&0x50)) {
        if (flags&7) Raw(form)[kState]|=0x10;
        transmit_(form,flags);
        Raw(form)[kState]&=static_cast<std::uint8_t>(~0x10);
    }
}
// 기본 Form은 정렬 없이 부모의 첫 종속 항목으로 부착한다. 부모0의 패치 조기 반환을 보존한다.
void SquidForms::Attach(Sid form,Sid parent,std::uint32_t flags) const {
    if (!parent.value) {
        if (pool_.Edition()==OriginalEdition::Patch1078) return;
        throw std::logic_error("CD Form 부모0 부착 금지");
    }
    const auto raw=Form(form);
    if (raw[kType]==kContentForm) throw std::logic_error("ContentForm 정렬 부착기 필요");
    if (parent.value<5 || parent.value>=pool_.Capacity() || parent==form || (pool_.Slot(parent)[kState]&(kFree|kDead)))
        throw std::logic_error("Form 부착 부모 상태 오류");
    ValidatePop(form,flags);
    const auto head=static_cast<std::uint16_t>(Read(parent,kHead,2));
    if (head==form.value || head>=pool_.Capacity() || (head && (pool_.Slot(Sid{head})[kState]&kFree)))
        throw std::logic_error("Form 부착 종속 머리 오류");
    Put(form,kParent,parent.value,2);Put(form,kNext,head,2);
    if (head) SetPrevious(Sid{head},form.value);
    Put(parent,kHead,form.value,2);Put(form,kPrevious,0,2);Raw(form)[kState]|=kContained;
    Pop(form,flags|0x100);
}
// 가상 setPrev는 이웃의 현재 타입에 따라 Form/자산의 서로 다른 위치를 쓴다.
void SquidForms::SetPrevious(Sid sid,std::uint16_t previous) const {
    Put(sid,pool_.Slot(sid)[kType]<kFirstAssetTypeNumber ? kPrevious : kAssetPrevious,previous,2);
}
// 부모 free/dead 분기를 보존하고 live 부모에서 빠진 ContentForm만 부모에 통지한다.
void SquidForms::Unpop(Sid form) const {
    const auto raw=Form(form);if (raw[kState]&kVoid) throw std::logic_error("void Form Unpop 금지");
    const auto parent=Parent(form);
    if (!parent.value || parent.value>=pool_.Capacity()) throw std::logic_error("Form 부모 번호 오류");
    const auto parentState=pool_.Slot(parent)[kState];
    const bool notify=raw[kType]==kContentForm && !(parentState&(kFree|kDead));
    if (notify && !notice_) throw std::invalid_argument("ContentForm 부모 제거 알림 미연결");
    Raw(form)[kState]|=kVoid;
    if (parentState&kFree) return;
    if (!(Raw(form)[kState]&kContained)) throw std::logic_error("Form contained 누락");
    if (parentState&kDead) return;
    const auto next=static_cast<std::uint16_t>(Read(form,kNext,2)),previous=Previous(form).value;
    // 손상 번호는 배열 밖 쓰기 전에 거부한다. 원본 assert 뒤의 잘못된 접근은 호스트 계약 밖이다.
    for (const auto neighbor:{next,previous}) {
        if (neighbor && (neighbor<5 || neighbor>=pool_.Capacity() || neighbor==form.value || (pool_.Slot(Sid{neighbor})[kState]&kFree)))
            throw std::logic_error("Form 이웃 링크 오류");
    }
    if (previous && previous==next) throw std::logic_error("Form 종속 체인 순환");
    if (previous) Put(Sid{previous},kNext,next,2);
    if (next) SetPrevious(Sid{next},previous);
    if (Read(parent,kHead,2)==form.value) {
        if (previous) throw std::logic_error("Form 머리 이전 항목 오류");
        Put(parent,kHead,next,2);
    }
    Raw(form)[kState]&=static_cast<std::uint8_t>(~kContained);
    if (notify) notice_(parent,form);
}
// 처리기 연결 시 실제 월드를 확인할 수 있게 제공한다.
const SidPool& SquidForms::Pool() const { return pool_; }
}
