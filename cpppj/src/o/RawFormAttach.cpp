// 선행 상태 검증이나 실패 rollback을 추가하지 않는 양판본 공통 부착 부모다.
#include "o/RawFormAttach.h"
#include <stdexcept>
namespace netstorm::o { // raw SID와 원문 가상 표 토큰을 호스트 함수 포인터로 역참조하지 않는다.
namespace { // 공통 원문 폭의 현재 메모리 접근과 선택 장치 요구를 공유한다.
template<class F> const F& NeedAttach(const F& value) { if (!value) throw std::logic_error("Form 부착의 선택된 실제 장치 미연결");return value; } // 앞선 링크 쓰기는 보존한다.
std::span<std::uint8_t> AttachRaw(SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // free/dead/예약 번호도 물리 범위 안이면 읽고 쓴다.
std::uint16_t AttachWord(std::span<const std::uint8_t> raw,std::size_t offset) { return static_cast<std::uint16_t>(raw[offset]|(std::uint16_t(raw[offset+1])<<8)); } // 원문 비정렬 WORD를 읽는다.
std::uint32_t AttachTable(std::span<const std::uint8_t> raw) { return AttachWord(raw,0)|(std::uint32_t(AttachWord(raw,2))<<16); } // 각 가상 호출 직전 현재 DWORD다.
void AttachPut(std::span<std::uint8_t> raw,std::size_t offset,std::uint16_t value) { raw[offset]=static_cast<std::uint8_t>(value);raw[offset+1]=static_cast<std::uint8_t>(value>>8); } // 주변 payload를 보존하는 WORD 저장이다.
}
void AttachCommonForm(SidPool& pool,Sid form,Sid parent,std::uint32_t flags,const FormAttachHooks& hooks) {
    const bool patch=pool.Edition()==OriginalEdition::Patch1078;
    const auto assertion=[&](std::string_view expression,std::uint32_t patchLine,std::uint32_t cdLine) { NeedAttach(hooks.assertion)(expression,patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp",patch ? patchLine : cdLine); }; // 진단 정상 복귀 뒤 다음 원문 명령을 계속한다.
    if (!parent.value) { if (patch) return;assertion("intoSid",0,0x3c2); }
    auto raw=AttachRaw(pool,form);
    if (NeedAttach(hooks.genus)(raw[10])&0x44200U) assertion("!(getGenus() & (gEMPLACEMENT|gVORTEX |gFACTORY))",0x3d0,0x3c3);
    NeedAttach(hooks.setParent)(form,AttachTable(raw),parent);
    auto parentRaw=AttachRaw(pool,parent);const auto head=AttachWord(parentRaw,6); // 원문은 부모 this와 머리 WORD를 함께 저장한다.
    if (head==form.value) NeedAttach(hooks.assertion)("n != getSid()",patch ? "c:\\ns\\o\\squid.h" : "\\ns\\o\\squid.h",patch ? 0x123U : 0x10cU);
    AttachPut(raw,4,head);
    if (head) { const Sid next{head};NeedAttach(hooks.setPrevious)(next,AttachTable(pool.Slot(next)),form); }
    AttachPut(parentRaw,6,form.value);
    NeedAttach(hooks.setPrevious)(form,AttachTable(raw),Sid{});
    raw[11]|=8;
    NeedAttach(hooks.pop)(form,flags|0x100U);
    const auto typeAfterPop=raw[10]; // 원문은 현재 TYPE BYTE를 읽고 현재 전체 content DWORD와 비교한다.
    if (typeAfterPop==NeedAttach(hooks.contentType)()) assertion("getType()!=contentFormType",0x3e0,0x3d2);
    if (raw[10]>0x45U) assertion("isForm()",0x3e3,0x3d5);
}
}
