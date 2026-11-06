// 저장 flags/부모·가상 토큰과 현재 재조회를 같은 raw 물리에 적용한다.
#include "o/RawContentAttach.h"
#include <stdexcept>
#include <unordered_set>
namespace netstorm::o { // 사제/전투 꼬리와 구별되는 정렬·부착 공통 접두다.
namespace { // 장치 교체 중에도 선택한 callable을 복귀까지 보관한다.
template<class F> F NeedContent(const F& value) { if (!value) throw std::logic_error("정렬 Content 부착의 선택된 장치 미연결");return value; }
std::span<std::uint8_t> ContentRaw(SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 물리 범위 안이면 TYPE/free 선행 검사 없이 읽고 쓴다.
std::uint16_t ContentWord(std::span<const std::uint8_t> raw,std::size_t at) { return static_cast<std::uint16_t>(raw[at]|(std::uint16_t(raw[at+1])<<8)); } // 비정렬 WORD 폭이다.
std::uint32_t ContentTable(std::span<const std::uint8_t> raw) { return std::uint32_t(ContentWord(raw,0))|(std::uint32_t(ContentWord(raw,2))<<16); } // 소비 위치의 가상 표 토큰이다.
void ContentPut(std::span<std::uint8_t> raw,std::size_t at,std::uint16_t value) { raw[at]=static_cast<std::uint8_t>(value);raw[at+1]=static_cast<std::uint8_t>(value>>8); } // 다른 payload는 보존한다.
void ContentNext(SidPool& pool,Sid sid,Sid next,const ContentAttachHooks& hooks) { auto raw=ContentRaw(pool,sid);if (next==sid) NeedContent(hooks.assertion)("n != getSid()","c:\\ns\\o\\squid.h",0x123);ContentPut(raw,4,next.value); } //004aba70의 진단 복귀 뒤 저장한 this에 쓴다.
}
bool AttachSortedContent(SidPool& pool,Sid form,Sid parent,std::uint32_t flags,const ContentAttachHooks& hooks) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("현재 정렬 Content 부모의10.78 계약");
    auto parentRaw=ContentRaw(pool,parent);auto raw=ContentRaw(pool,form); // 부모 this를 첫 진단 앞에서 저장한다.
    if (parentRaw[11]&3U) NeedContent(hooks.assertion)("!into.isDead() && !into.isFree()","\\Ns\\O\\Squid.cpp",0xbe6);
    if (raw[11]&3U) NeedContent(hooks.assertion)("!isDead() && !isFree()","\\Ns\\O\\Squid.cpp",0xbe7);
    if ((parentRaw[11]&3U) || (raw[11]&3U)) return false;
    const auto group=ContentWord(raw,20);std::uint32_t order{};Sid current{ContentWord(parentRaw,6)},previous{};bool inserted=false;std::unordered_set<std::uint16_t> visited;
    while (current.value) { // 다음 WORD/현재 content DWORD를 원문 위치에서 읽는다.
        if (!visited.insert(current.value).second) throw std::logic_error("정렬 Content 부착의 비종료 체인 순환");
        const auto candidate=pool.Slot(current);const auto content=NeedContent(hooks.contentType)();
        if (candidate[10]==content && ((ContentWord(candidate,20)^group)&0xfcffU)==0) {
            if (order<candidate[23]) {
                ContentNext(pool,form,current,hooks);
                const auto ownTable=ContentTable(raw); // 이웃 getter보다 먼저 저장한 자기 VT다.
                const auto before=NeedContent(hooks.previous)(current,ContentTable(candidate));
                NeedContent(hooks.setPrevious)(form,ownTable,before);
                NeedContent(hooks.setPrevious)(current,ContentTable(candidate),form);
                if (!NeedContent(hooks.previous)(form,ContentTable(raw)).value) ContentPut(parentRaw,6,form.value);
                else { const auto selected=NeedContent(hooks.previous)(form,ContentTable(raw));ContentNext(pool,selected,form,hooks); } // 두 번째 getter 반환을 다시 사용한다.
                inserted=true;break;
            }
            ++order;
        }
        previous=current;current=Sid{ContentWord(candidate,4)};
    }
    if (!inserted) {
        if (previous.value) { ContentNext(pool,previous,form,hooks);NeedContent(hooks.setPrevious)(form,ContentTable(raw),previous);ContentNext(pool,form,Sid{},hooks); }
        else { ContentPut(parentRaw,6,form.value);ContentNext(pool,form,Sid{},hooks);NeedContent(hooks.setPrevious)(form,ContentTable(raw),Sid{}); }
    }
    raw[23]=static_cast<std::uint8_t>(order);NeedContent(hooks.setParent)(form,ContentTable(raw),parent);
    raw[11]|=8;NeedContent(hooks.pop)(form,flags|0x100U);NeedContent(hooks.notice)(parent,form,1);return true;
}
}
