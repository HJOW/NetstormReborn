// Form 수신은 기존 바이트를 보존하고 공통 Unpop은 부모 삭제 중의 체인을 유지한다.
#include "o/SquidForms.h"
#include "o/RawSquidContainedUnpop.h" // Form도 같은 현재 가상 링크와 공통 종속 분리를 사용한다.
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
// free/dead/void/contained 상태와 실제 ContentForm 생성자 번호다.
constexpr std::uint8_t kFree=1,kDead=2,kVoid=4,kContained=8;
constexpr std::uint32_t kContentForm=6;
}
// 알림을 보관하되 다른 객체 수명이나 풀 초기화 효과는 숨기지 않는다.
SquidForms::SquidForms(SidPool& pool,FormRemovalNotice notice,FormPopTransmit transmit,FormUnpopHooks unpop)
    :pool_(pool),notice_(std::move(notice)),transmit_(std::move(transmit)),unpop_(std::move(unpop)) {}
void SquidForms::BindUnpop(FormUnpopHooks hooks) { unpop_=std::move(hooks); } // 이후 호출 위치에서 현재 대여를 선택한다.
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
// 현재 생성자가 남긴 네 Form VT를 확인하며 변경 가능한 TYPE 번호에서 VT를 추정하지 않는다.
std::span<const std::uint8_t> SquidForms::Form(Sid sid) const {
    const auto raw=pool_.Slot(sid);
    if (sid.value<5 || (raw[kState]&kFree)) throw std::logic_error("Form 슬롯 상태 오류");
    const auto table=Read(sid,0,4);for (const auto type:kTypes) if (table==Vtable(type)) return raw; // 실제 생성자 클래스만 지원하며 알 수 없는 VT는 거부한다.
    throw std::logic_error("Form 가상 표 미복원");
}
// 원본 Form의 부모 getter는 WORD 전체를 반환한다.
Sid SquidForms::Parent(Sid form) const { Form(form);return Sid{static_cast<std::uint16_t>(Read(form,kParent,2))}; }
// 원본 Form의 이전 getter도 WORD 전체를 반환한다.
Sid SquidForms::Previous(Sid form) const { Form(form);return Sid{static_cast<std::uint16_t>(Read(form,kPrevious,2))}; }
void SquidForms::ConstructVtableAt(std::uint32_t type,Sid sid) const { Put(sid,0,Vtable(type),4); } // 원문 cdecl1/2/3/6의 단일DWORD 저장만 실행한다.
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
// 정렬 Content 등 기존 호출자가 쓰는 명시 선행 검증이다. 실제 공통 Pop의 진단/부분 쓰기와 구별한다.
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
// 현재 권한/진단 복귀와 같은 공유 깊이를 사용하는 실제 공통 Pop 부모다.
void SquidForms::Pop(Sid form,std::uint32_t flags) const {
    auto hooks=pop_;
    if (!hooks.server) hooks.server=[this] { return pool_.IsServer(); }; // 호출 위치의 현재 장기 권한이다.
    if (!hooks.depth) hooks.depth=[this] { return localPostPopDepth_; }; // 독립 미연결 입력의 명시 지역 깊이다.
    if (!hooks.setDepth) hooks.setDepth=[this](std::uint32_t value) { localPostPopDepth_=value; }; // 원문 DWORD wrap을 보존한다.
    if (!hooks.assertion) hooks.assertion=unpop_.assertion; // Content 독립 입력의 같은 원문 진단도 재사용한다.
    if (!hooks.transmit) hooks.transmit=[this](Sid sid,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t value) { if (!transmit_) throw std::logic_error("현재 공통 Form Pop 전송 미연결");transmit_(sid,value); }; // 원문 고정 -1/-1/0을 쓰는 기존 Form 전송 경계다.
    if (!hooks.postPop) hooks.postPop=[this,&hooks](Sid,std::uint32_t table,std::uint32_t) { // 현재 실제 VT가 네 Form 빈 후처리인지 선택한다.
        for (const auto type:kTypes) if (table==Vtable(type)) { hooks.setDepth(hooks.depth()-1U);return; } // PE004ac1d0/CD004abab0의 같은 공유 DWORD 감소다.
        throw std::logic_error("공통 Form Pop의 선택된 현재 VT+20 미연결");
    };
    PopCommonForm(pool_,form,flags,hooks);
}
void SquidForms::BindPop(FormPopHooks hooks) { pop_=std::move(hooks); } // 다음 호출부터 현재 World의 깊이/진단을 사용한다.
// 실제 공통 +74 몸체는 클래스/상태 선행 guard 없이 현재 가상 setter와 직접 Pop을 호출한다.
void SquidForms::Attach(Sid form,Sid parent,std::uint32_t flags) const {
    auto hooks=attach_;
    if (!hooks.genus) hooks.genus=[](std::uint8_t) { return 0U; }; // 독립 편의 입력에는 TYPE 표가 없으며 실제 공장은 현재 getter를 제공한다.
    if (!hooks.contentType) hooks.contentType=[] { return 6U; }; // 독립 입력의 명시 초기 번호다.
    if (!hooks.setParent) hooks.setParent=[this](Sid sid,std::uint32_t,Sid value) { CommonContainedSetParent(pool_,sid,value); }; // 실제 현재 VT+8을 선택한다.
    if (!hooks.setPrevious) hooks.setPrevious=[this](Sid sid,std::uint32_t,Sid value) { CommonContainedSetPrevious(pool_,sid,value); }; // raw TYPE 비교로 필드 배치를 추정하지 않는다.
    if (!hooks.pop) hooks.pop=[this](Sid sid,std::uint32_t value) { Pop(sid,value); }; // 기존 공통 Pop의 guard는 이 자식 위치에서만 적용한다.
    AttachCommonForm(pool_,form,parent,flags,hooks);
}
void SquidForms::BindAttach(FormAttachHooks hooks) { attach_=std::move(hooks); } // 다음 부착부터 현재 getter/진단을 사용한다.
// 가상 setPrev는 이웃의 현재 타입에 따라 Form/자산의 서로 다른 위치를 쓴다.
void SquidForms::SetPrevious(Sid sid,std::uint16_t previous) const {
    Put(sid,pool_.Slot(sid)[kType]<kFirstAssetTypeNumber ? kPrevious : kAssetPrevious,previous,2);
}
// 두 판본의 진단 복귀 뒤 현재 TYPE/VT를 사용하고 void 쓰기 뒤 선택한 실제 종속 분리를 실행한다.
void SquidForms::Unpop(Sid form) const {
    const auto hooks=unpop_;const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto assertion=[&](std::string_view expression,std::uint32_t patchLine,std::uint32_t cdLine) { // 선택한 진단만 요구하고 정상 복귀 뒤 현재 바이트를 다시 읽는다.
        if (!hooks.assertion) throw std::logic_error("Form Unpop 원문 진단 장치 미연결");hooks.assertion(expression,patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp",patch ? patchLine : cdLine);
    };
    if (Raw(form)[kType]>0x45U) assertion("isForm()",0xa59,0xa10);
    if (!form.value || form.value>=pool_.Capacity()) assertion("isValid()",0xa5a,0xa11);
    if (Raw(form)[kState]&kVoid) assertion("!isVoid()",0xa5b,0xa12);
    auto table=patch ? 0U : Read(form,0,4); // CD는 OR 앞에서 VT를 저장하고 PATCH는 OR 뒤에서 읽는다.
    Raw(form)[kState]|=kVoid;
    if (patch) table=Read(form,0,4);
    const auto parent=hooks.parent ? hooks.parent(form,table) : CommonContainedParent(pool_,form);
    if (pool_.Slot(parent)[kState]&kFree) return;
    if (hooks.detach) { hooks.detach(form);return; }
    const std::uint32_t contentType=6,gumpType=3,event=0; // 독립 입력의 원문 초기 번호이며 실제 World는 현재 전역의 종단을 대여한다.
    SquidContainedUnpopHooks detach;
    detach.parent=[this](Sid sid) { return CommonContainedParent(pool_,sid); }; // 분리 몸체는 현재 getter를 다시 호출한다.
    detach.previous=[this](Sid sid) { return CommonContainedPrevious(pool_,sid); }; // 다음 번호 저장 뒤 현재 이전 getter를 읽는다.
    detach.setPrevious=[this](Sid sid,Sid previous) { CommonContainedSetPrevious(pool_,sid,previous); }; // 다음 객체의 실제 VT가 정한 WORD만 쓴다.
    detach.parentNotice=[this](Sid parent,Sid sid,std::uint32_t) { if (!notice_) throw std::invalid_argument("ContentForm 부모 제거 알림 미연결");notice_(parent,sid); }; // 선택된 알림 부재는 앞선 링크 쓰기를 취소하지 않는다.
    detach.assertion=hooks.assertion;
    RawSquidContainedUnpop(pool_,{contentType,gumpType,event,{}, {}, {}},std::move(detach)).Detach(form);
}
// 처리기 연결 시 실제 월드를 확인할 수 있게 제공한다.
const SidPool& SquidForms::Pool() const { return pool_; }
}
