// 두 판본 ASM의 상태 BYTE/VT 저장 시점과 선택된 반환 객체를 유지한다.
#include "o/RawSquidTake.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 가상/생성자 자식의 효과와 부모 순서를 구별한다.
namespace { // 선택한 콜백은 자기 연결 교체에도 호출 끝까지 유지한다.
template<class F> F NeedTake(const F& function) { if (!function) throw std::logic_error("Take의 선택된 실제 장치 미연결");return function; } // 미선택 장치는 선행 요구하지 않는다.
}
std::uint32_t TakeRawSquid(OriginalEdition edition,std::uint32_t type,std::uint32_t sid,const RawSquidTakeHooks& hooks) { // TYPE는 마지막 저장에서만BYTE로 좁힌다.
    const bool patch=edition==OriginalEdition::Patch1078;
    const auto diagnose=[&](std::string_view expression,std::uint32_t line) { NeedTake(hooks.assertion)(expression,patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp",line); }; // 실제 진단의 정상 복귀만 허용한다.
    const auto number=std::bit_cast<std::int32_t>(sid);
    if (number<(patch ? 15000 : 6000) || number>=std::bit_cast<std::int32_t>(NeedTake(hooks.capacity)())) diagnose("(FIRST_SERVER_SID <= sid && sid < numSquids)",patch ? 0x1a7 : 0x19e);
    const auto original=NeedTake(hooks.slot)(sid);
    if (!(NeedTake(hooks.state)(original)&1U)) {
        const auto previous=NeedTake(hooks.type)(original);
        if (previous!=type) { NeedTake(hooks.detail)(type,previous);diagnose("0",patch ? 0x1ba : 0x1b1); }
        const auto table=NeedTake(hooks.vtable)(original);NeedTake(hooks.unpop)(original,table);
    }
    const auto state=NeedTake(hooks.state)(original);
    if (!patch) NeedTake(hooks.storeState)(original,static_cast<std::uint8_t>(state&0xfeU)); // CD의 두 실제 BYTE 쓰기를 합치지 않는다.
    NeedTake(hooks.storeState)(original,static_cast<std::uint8_t>(state&0xfcU));
    const auto address=NeedTake(hooks.constructor)(type);const auto object=NeedTake(hooks.construct)(address,sid);
    if (patch) {
        const auto current=NeedTake(hooks.state)(object);const auto table=NeedTake(hooks.vtable)(object);
        NeedTake(hooks.storeType)(object,static_cast<std::uint8_t>(type));NeedTake(hooks.storeState)(object,static_cast<std::uint8_t>(current|4U));NeedTake(hooks.postTake)(object,table);
    } else {
        const auto table=NeedTake(hooks.vtable)(object);const auto current=NeedTake(hooks.state)(object);
        NeedTake(hooks.storeState)(object,static_cast<std::uint8_t>(current|4U));NeedTake(hooks.storeType)(object,static_cast<std::uint8_t>(type));NeedTake(hooks.postTake)(object,table);
    }
    return object;
}
}
