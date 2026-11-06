// 원문 파생 접두의 진단을 선행 예외나 전체 장치 검사로 대체하지 않는다.
#include "o/RawProcessAttach.h"
#include <stdexcept>
namespace netstorm::o { // 실제 프로세스/직렬화 저장소는 외부 장치가 소유한다.
namespace { // 콜백 자체 교체에도 현재 호출이 복귀할 때까지 선택한 callable을 보관한다.
template<class F> F NeedProcessAttach(const F& value) { if (!value) throw std::logic_error("Process 부착의 선택된 실제 장치 미연결");return value; }
}
void AttachProcessForm(SidPool& pool,Sid form,Sid parent,std::uint32_t flags,const ProcessAttachHooks& hooks) {
    const bool patch=pool.Edition()==OriginalEdition::Patch1078;
    const auto raw=pool.Slot(form);const bool changed=(std::uint16_t(raw[16])|(std::uint16_t(raw[17])<<8))!=parent.value; // 첫 진단 전에 읽은 부모 변경 판정을 유지한다.
    const auto assertion=[&](std::string_view expression,std::uint32_t patchLine,std::uint32_t cdLine) { NeedProcessAttach(hooks.assertion)(expression,patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp",patch ? patchLine : cdLine); }; // 진단 장치가 정상 복귀하면 현재 서버 판정부터 계속한다.
    if (changed && !(flags&0x80U)) assertion("popFlags & pfPROCESS_DEBUG",0xa77,0xa2e);
    if (NeedProcessAttach(hooks.server)()) {
        NeedProcessAttach(hooks.lookup)(form);
        if (!(flags&0x50U)) {
            const auto size=NeedProcessAttach(hooks.writeData)(changed);
            if (!size) assertion("transmitSize!=0",0xa7f,0xa36);
            if (size>0) {
                if (size>0x7ff) assertion("transmitSize < TRANSMIT_BUFFER_SIZE",0xa81,0xa38);
                NeedProcessAttach(hooks.transmit)(size);
            }
        }
        flags|=0x40U;
    }
    NeedProcessAttach(hooks.attach)(form,parent,flags);
}
}
