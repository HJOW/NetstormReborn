// 공통 종속 분리는 부분 쓰기를 취소하지 않으며 Form 전용 Unpop을 자산에 호출하지 않는다.
#include "o/RawSquidContainedUnpop.h"
#include <array>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace netstorm::o { // 물리 주소 대신 같은 SID 슬롯과 현재 VT를 사용한다.
namespace { // 두 PE에서 확인한 생성자 허용목록과 비정렬 WORD 접근을 공유한다.
constexpr std::string_view kSquidSource="\\Ns\\O\\Squid.cpp"; // 공통 분리의 원문 진단 파일이다.
constexpr std::string_view kFinderSource="\\Ns\\O\\Squidfinder.cpp"; // UI Form 검색의 원문 진단 파일이다.
constexpr std::uint8_t kFree=1,kDead=2,kContained=8; // 상태 BYTE의 진단/분리 관문이며 void는 여기서 바꾸지 않는다.
constexpr std::uint8_t kLastForm=0x45; // Form finder는 내용물6 전용 검색과 달리 이 범위 전체를 반환한다.
struct ContentNoticeEntry { // 기존 생성자 목록의 VT만 공통 getter 허용목록으로 재사용한다.
    std::uint32_t table; // 실제 생성자가 쓰는 VT 토큰이다.
    std::uint32_t entry; // 기존 목록의+24 기록이며 이 모듈은 부모 통지를 실제 hook에 맡긴다.
};
#include "ContentNoticeVtables.inc"
constexpr std::array<std::uint32_t,4> kPatchForms{0x512110U,0x512098U,0x512188U,0x512208U}; // 같은 실제 Form getter/setter 네 생성자다.
constexpr std::array<std::uint32_t,4> kCdForms{0x505a08U,0x505990U,0x505a80U,0x505b00U}; // CD의 같은 네 내장 Form 생성자다.
template<class Function> const Function& NeedContained(const Function& function) { if (!function) throw std::logic_error("공통 contained 해제의 선택된 실제 장치 미연결");return function; } // 앞선 부분 효과는 되돌리지 않는다.
std::uint16_t LinkWord(std::span<const std::uint8_t> raw,std::size_t offset) { return static_cast<std::uint16_t>(raw[offset]|(std::uint16_t(raw[offset+1])<<8)); } // 원문 WORD 전체만 읽는다.
std::span<std::uint8_t> LinkRaw(SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 진단 정상 복귀 뒤 free 슬롯에도 물리 범위 안 쓰기를 유지한다.
void PutLink(SidPool& pool,Sid sid,std::size_t offset,std::uint16_t value) { auto raw=LinkRaw(pool,sid);raw[offset]=static_cast<std::uint8_t>(value);raw[offset+1]=static_cast<std::uint8_t>(value>>8); } // 인접 payload를 바꾸지 않는 원문 WORD 쓰기다.
bool FormLinkLayoutAtTable(OriginalEdition edition,std::uint32_t table) { // 선택한 실제 가상 표의 getter/setter 배치다.
    const auto& forms=edition==OriginalEdition::Patch1078 ? kPatchForms : kCdForms;
    for (const auto form:forms) if (form==table) return true; // PATCH4ad010/4abac0/4abae0,CD4b04a0/4b04b0/4b04d0의+0/+4/+c다.
    const auto entries=edition==OriginalEdition::Patch1078 ? std::span<const ContentNoticeEntry>(kPatchContentNotices) : std::span<const ContentNoticeEntry>(kCdContentNotices);
    for (const auto& entry:entries) if (entry.table==table) return false; // PE전수검사:PATCH83개4ad000/4ad010/4ad040,CD68개4acc80/4acc90/4accc0다.
    throw std::logic_error("공통 contained 링크의 미복원 현재 가상 표");
}
bool FormLinkLayout(const SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);return FormLinkLayoutAtTable(pool.Edition(),std::uint32_t(LinkWord(raw,0))|(std::uint32_t(LinkWord(raw,2))<<16)); } // 일반 경로는 호출 직전 현재 VT를 선택한다.
}
Sid CommonContainedParent(const SidPool& pool,Sid sid) { const bool form=FormLinkLayout(pool,sid);return Sid{LinkWord(pool.Slot(sid),form ? 16U : 14U)}; } // 같은 현재 VT의 부모 WORD getter다.
Sid CommonContainedPrevious(const SidPool& pool,Sid sid) { const bool form=FormLinkLayout(pool,sid);return Sid{LinkWord(pool.Slot(sid),form ? 14U : 16U)}; } // 같은 현재 VT의 이전 WORD getter다.
Sid CommonContainedPreviousAtTable(const SidPool& pool,Sid sid,std::uint32_t table) { const bool form=FormLinkLayoutAtTable(pool.Edition(),table);return Sid{LinkWord(pool.Slot(sid),form ? 14U : 16U)}; } // 저장한 VT의 getter WORD다.
void CommonContainedSetParent(SidPool& pool,Sid sid,Sid parent) { const bool form=FormLinkLayout(pool,sid);PutLink(pool,sid,form ? 16U : 14U,parent.value); } // 확인한 실제 VT+8의 하위 WORD만 바꾼다.
std::uint32_t RawSquidContainedUnpop::ContentFormType() const { const auto pin=runtimePin_;const auto runtimeOwner=pin ? pin():std::shared_ptr<void>{}; return state_.contentFormType; } // 약한 Session 대여가 살아 있는 동안 현재 DWORD를 읽는다.
void CommonContainedSetPrevious(SidPool& pool,Sid sid,Sid previous) { const bool form=FormLinkLayout(pool,sid);PutLink(pool,sid,form ? 14U : 16U,previous.value); } // 공통 자산의+0xe DWORD 상위WORD 쓰기와 동일한 비트 결과다.
void CommonContainedSetPreviousAtTable(SidPool& pool,Sid sid,std::uint32_t table,Sid previous) { const bool form=FormLinkLayoutAtTable(pool.Edition(),table);PutLink(pool,sid,form ? 14U : 16U,previous.value); } // 선택한 VT+c를 실행한다.
RawSquidContainedUnpop::RawSquidContainedUnpop(SidPool& pool,SquidContainedUnpopState state,SquidContainedUnpopHooks hooks)
    :pool_(pool),state_(std::move(state)),hooks_(std::move(hooks)) {} // 생성 중에는 선택하지 않은 장치를 요구하거나 풀을 수정하지 않는다.
void RawSquidContainedUnpop::Assert(std::string_view expression,std::uint32_t patchLine,std::uint32_t cdLine) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;NeedContained(hooks_.assertion)(expression,patch ? kSquidSource : "\\ns\\o\\Squid.cpp",patch ? patchLine : cdLine); // CD 원문 문자열의 소문자 디렉터리도 보존한다.
}
void RawSquidContainedUnpop::Detach(Sid sid) const { const auto pin=runtimePin_;const auto runtimeOwner=pin ? pin():std::shared_ptr<void>{}; // getter/통지가 바꾼 현재 슬롯을 각 원문 위치에서 다시 읽는다.
    if (!(pool_.Slot(sid)[11]&kContained)) Assert("containedFlag",0x3a3,0x39a);
    const auto parent=NeedContained(hooks_.parent)(sid);
    if (!parent.value) Assert("parentSid",0x3a5,0x39c);
    if (pool_.Slot(parent)[11]&kFree) Assert("!p.isFree()",0x3a8,0x39f);
    if (pool_.Slot(parent)[11]&kDead) return;
    if (!(pool_.Slot(sid)[11]&kContained)) Assert("containedFlag",0x3b1,0x3a8);
    const Sid next{LinkWord(pool_.Slot(sid),4)}; // previous getter보다 먼저 읽은 next는 이후 진단/통지에도 보존한다.
    const auto previous=NeedContained(hooks_.previous)(sid);
    if (previous.value) {
        if (previous==next) NeedContained(hooks_.assertion)("n != getSid()",pool_.Edition()==OriginalEdition::Patch1078 ? "c:\\ns\\o\\squid.h" : "\\ns\\o\\squid.h",pool_.Edition()==OriginalEdition::Patch1078 ? 0x123U : 0x10cU);
        PutLink(pool_,previous,4,next.value);
    }
    if (next.value) NeedContained(hooks_.setPrevious)(next,previous);
    if (LinkWord(pool_.Slot(parent),6)==sid.value) {
        if (previous.value) Assert("prev == 0",0x3c0,0x3b7);
        PutLink(pool_,parent,6,next.value);
    }
    LinkRaw(pool_,sid)[11]&=static_cast<std::uint8_t>(~kContained);
    if (pool_.Slot(sid)[10]>kLastForm || pool_.Slot(sid)[10]==state_.contentFormType) NeedContained(hooks_.parentNotice)(parent,sid,2);
}
void RawSquidContainedUnpop::NotifyUi(Sid sid) const { const auto pin=runtimePin_;const auto runtimeOwner=pin ? pin():std::shared_ptr<void>{}; // PATCH004ad230/CD004acf40은 이 객체 자신의 Form 종속을 조회한다.
    if (pool_.Edition()==OriginalEdition::Patch1078 && state_.checkingDead && !NeedContained(state_.inMetamapMode)() &&
        (pool_.Slot(sid)[11]&kDead) && !NeedContained(state_.boss)())
        NeedContained(hooks_.assertion)("inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy",kFinderSource,0x158);
    Sid next{LinkWord(pool_.Slot(sid),6)};std::unordered_set<std::uint16_t> seen; // 실제 Form finder처럼 반환 전 next를 저장한다.
    while (next.value) { // 원문의 순환/물리 범위 밖 접근만 소비 시점에서 호스트 오류로 중단한다.
        const auto current=next;if (!seen.insert(current.value).second) throw std::logic_error("공통 contained UI검색의 종속 순환");
        const auto raw=pool_.Slot(current);next=Sid{LinkWord(raw,4)};
        if (raw[10]>kLastForm) continue;
        if (pool_.Slot(current)[10]==state_.gumpFormType) { NeedContained(hooks_.command)(state_.uiEvent,sid.value,0);return; }
    }
}
void RawSquidContainedUnpop::Unpop(Sid sid) const { const auto pin=runtimePin_;const auto runtimeOwner=pin ? pin():std::shared_ptr<void>{}; Detach(sid);NotifyUi(sid); } // 두 실제 하위 종단을 원래 호출 순서로 실행한다.
void RawSquidContainedUnpop::BindRuntimePin(std::function<std::shared_ptr<void>()> pin) { runtimePin_=std::move(pin); } //실제선택호출수명만결합/반납하며종속링크나상태는쓰지않는다.
const SidPool& RawSquidContainedUnpop::Pool() const { return pool_; } // 연결 시 다른 World/Session의 풀 혼용을 거부할 수 있게 한다.
}
