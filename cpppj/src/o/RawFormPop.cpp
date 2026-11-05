// 공통 Form Pop은 void/전송 비트의 완료 쓰기를 실패 시 되돌리지 않는다.
#include "o/RawFormPop.h"
#include <stdexcept>
namespace netstorm::o { // 실제 PE 토큰은 가상 분배 장치에 전달하고 호스트 포인터로 역참조하지 않는다.
namespace { // 같은 원문 폭 메모리와 선택 장치 요구를 공유한다.
template<class F> const F& NeedFormPop(const F& value) { if (!value) throw std::logic_error("공통 Form Pop의 선택된 실제 장치 미연결");return value; } // 앞선 상태/깊이 쓰기는 유지한다.
std::span<std::uint8_t> FormPopRaw(SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 진단 후 free/dead/예약 슬롯도 물리 범위 안에서 접근한다.
std::uint32_t FormPopTable(std::span<const std::uint8_t> raw) { return std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24); } // 깊이 증가 뒤 현재 VT DWORD다.
}
void PopCommonForm(SidPool& pool,Sid form,std::uint32_t flags,const FormPopHooks& hooks) {
    const bool patch=pool.Edition()==OriginalEdition::Patch1078;
    const auto assertion=[&](std::string_view expression,std::uint32_t patchLine,std::uint32_t cdLine) { NeedFormPop(hooks.assertion)(expression,patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp",patch ? patchLine : cdLine); }; // 정상 복귀 뒤 원문 현재값을 다시 읽는다.
    if (!form.value || form.value>=pool.Capacity()) assertion("isValid()",0x35b,0x352);
    auto raw=FormPopRaw(pool,form);
    if (!(raw[11]&4U)) assertion("isVoid()",0x35c,0x353);
    const auto state=raw[11];raw[11]=state&static_cast<std::uint8_t>(~4U);
    if (state&0x10U) assertion("!xmitJustCreatedFlag",0x35f,0x356);
    if (!NeedFormPop(hooks.server)()) { if (!(flags&8U)) flags|=0x10U; }
    else {
        const auto end=patch ? 15000U : 6000U; // 예약0~4는 원문 client 범위에 포함하지 않는다.
        if (form.value<5U || form.value>=end) { if (!(flags&0x10U)) flags|=8U; }
        else { if (flags&8U) assertion("!(popFlags & pfSERVER_POP)",0x364,0x35b);flags|=0x10U; }
        if (!(flags&0x50U)) {
            if (flags&7U) raw[11]|=0x10U;
            NeedFormPop(hooks.transmit)(form,0xffffffffU,0xffffffffU,0U,flags);
            raw[11]&=static_cast<std::uint8_t>(~0x10U);
        }
    }
    const auto base=NeedFormPop(hooks.depth)();NeedFormPop(hooks.setDepth)(base+1U);
    NeedFormPop(hooks.postPop)(form,FormPopTable(raw),flags);
    if (base!=NeedFormPop(hooks.depth)()) assertion("curPostPop == basePostPop",0x392,0x389);
}
}
