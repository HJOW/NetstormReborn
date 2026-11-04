// 원문10.78의 저장된 flags/상태와 진단 뒤 현재 서버·깊이를 구별한다.
#include "o/RawSquidActivate.h"
#include <stdexcept>

namespace netstorm::o { // 호스트 포인터를 원본 주소로 호출하지 않고 같은 슬롯 번호를 사용한다.
namespace { // 선택한 callback만 복사해 자기 연결 교체 중에도 수명을 보존한다.
template<class F> F ActivateNeed(const F& function) { if (!function) throw std::logic_error("Activate의 선택된 실제 장치 미연결");return function; } // 미선택 장치는 요구하지 않는다.
constexpr std::string_view kActivateSource="\\Ns\\O\\Squid.cpp"; // 원문 진단의 동일 소스 경로다.
}
void RunSquidActivate(SidPool& pool,Sid sid,std::uint32_t flags,std::uint32_t& depth,const SquidActivateHooks& hooks) { //004ac560 전체 부모의 실제 순서를 따른다.
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("현재 Activate는10.78 전용");
    const auto diagnose=[&](std::string_view expression,std::uint32_t line) { ActivateNeed(hooks.assertion)(expression,kActivateSource,line); }; // 정상 반환 뒤 저장된 분기에서 계속한다.
    if (!sid.value || sid.value>=pool.Capacity()) diagnose("isValid()",0x35b);
    const auto slot=pool.Slot(sid);auto raw=std::span<std::uint8_t>{const_cast<std::uint8_t*>(slot.data()),slot.size()};
    if (!(raw[11]&4U)) diagnose("isVoid()",0x35c);
    const auto state=raw[11];raw[11]&=0xfb;if (state&0x10U) diagnose("!xmitJustCreatedFlag",0x35f);
    if (ActivateNeed(hooks.server)()) {
        if (sid.value>=5 && sid.value<15000) {
            if (flags&8U) diagnose("!(popFlags & pfSERVER_POP)",0x364);
            flags|=0x10;
        } else if (!(flags&0x10U)) flags|=8;
        if (!(flags&0x50U)) {
            if (flags&7U) raw[11]|=0x10;
            ActivateNeed(hooks.transmit)(sid,flags);
            raw[11]&=0xef; // 하위 정상 반환 뒤에만 전송중 비트를 지운다.
        }
    } else if (!(flags&8U)) flags|=0x10;
    const auto before=depth;++depth;
    ActivateNeed(hooks.postPop)(sid,flags);
    if (depth!=before) diagnose("curPostPop == basePostPop",0x392);
}
}
