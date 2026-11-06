// 타입 진단→필드 쓰기→부모 상태→Kernel→Form→가상 통지 순서와 부분 실패를 유지한다.
#include "o/RawBaseProcessAttach.h"
#include <bit>
#include <stdexcept>
namespace netstorm::o { // C++ 소유권의 최종 반납은 Host/Kernel에 남긴다.
namespace { // 원문 폭의 물리 슬롯 접근과 선택 장치 복사를 공유한다.
template<class F> F NeedBaseAttach(const F& value) { if (!value) throw std::logic_error("BaseProcess 부착의 선택된 실제 장치 미연결");return value; }
std::span<std::uint8_t> BaseAttachRaw(SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // free/예약도 실제 소비 시점에만 물리 범위를 확인한다.
std::uint32_t BaseAttachPid(std::span<const std::uint8_t> raw) { return std::uint32_t(raw[18])|(std::uint32_t(raw[19])<<8)|(std::uint32_t(raw[20])<<16)|(std::uint32_t(raw[21])<<24); } // 비정렬 PID DWORD 전체다.
void BaseAttachPutPid(std::span<std::uint8_t> raw,std::uint32_t pid) { for (std::size_t index=0;index<4;++index) raw[18+index]=static_cast<std::uint8_t>(pid>>(index*8)); } // PID 밖 payload는 보존한다.
}
bool AttachBaseProcess(SidPool& pool,BaseProcessAttachFields fields,std::uint32_t processType,Sid parent,Sid form,std::uint32_t flags,const BaseProcessAttachHooks& hooks) {
    const bool patch=pool.Edition()==OriginalEdition::Patch1078;
    const auto diagnose=[&](std::string_view expression,std::uint32_t patchLine,std::uint32_t cdLine) { NeedBaseAttach(hooks.assertion)(expression,patch ? "\\Ns\\O\\Baseprocess.cpp" : "\\ns\\o\\BaseProcess.cpp",patch ? patchLine : cdLine); }; // 필드 쓰기 전 진단과 이후 현재 상태를 구별한다.
    const auto signedType=std::bit_cast<std::int32_t>(processType);
    if (signedType<10 || std::bit_cast<std::int32_t>(NeedBaseAttach(hooks.typeEnd)())<=signedType) diagnose("isProcessType(_processType)",0x1c,0x1c);
    fields.parent=parent;fields.form=form;
    const auto parentRaw=pool.Slot(parent); // CD는 두 진단 중에도 같은 부모 this를 유지한다.
    if (patch) { if (parentRaw[11]&3U) return false; }
    else {
        if (parentRaw[11]&1U) diagnose("!base.isFree()",0,0x38);
        if (parentRaw[11]&2U) diagnose("!base.isDead()",0,0x39);
    }
    const auto pid=NeedBaseAttach(hooks.registerProcess)();
    if (!fields.form.value) {
        const auto created=NeedBaseAttach(hooks.create)((flags&0x10U) ? 2U : 0U);fields.form=created;
        auto raw=BaseAttachRaw(pool,created);
        const auto typeDiagnostic=[&](std::string_view expression,std::uint32_t patchLine,std::uint32_t cdLine) { NeedBaseAttach(hooks.assertion)(expression,patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp",patch ? patchLine : cdLine); }; // 실제 setProcessType의 두 진단도 정상 복귀한다.
        if (signedType<0 || signedType>0xfe) typeDiagnostic("0 <= pType && pType < MAX_TYPES",0xab7,0xa6e);
        if (raw[10]!=NeedBaseAttach(hooks.processFormType)()) typeDiagnostic("typeNum == processFormType",0xab8,0xa6f);
        raw[10]=static_cast<std::uint8_t>(processType);BaseAttachPutPid(raw,pid);
        NeedBaseAttach(hooks.attach)(created,fields.parent,flags|0x80U);NeedBaseAttach(hooks.onAttached)();return true;
    }
    const auto selected=fields.form;auto raw=BaseAttachRaw(pool,selected); // Take 전에 저장한 지정 객체는 이후 필드 변경에도 유지한다.
    if (raw[11]&1U) {
        NeedBaseAttach(hooks.take)(selected);raw[10]=static_cast<std::uint8_t>(processType);BaseAttachPutPid(raw,pid);
        NeedBaseAttach(hooks.attach)(selected,fields.parent,flags|0x80U);
    }
    const auto currentParent=NeedBaseAttach(hooks.parent)(selected); // getter가 복귀한 뒤 현재 프로세스 부모 필드를 읽는다.
    if (currentParent!=fields.parent) diagnose("s.getParentSid() == attachToSid",0x5e,0x59);
    if (BaseAttachPid(raw)!=pid) diagnose("s.getPid() == pid",0x5f,0x5a);
    NeedBaseAttach(hooks.onAttached)();return true;
}
}
