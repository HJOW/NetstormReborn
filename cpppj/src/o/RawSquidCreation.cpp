// 파생 생성자/가상 함수의 실제 효과는 호출 장치에 남기고 부모의 선후 관계를 복원한다.
#include "o/RawSquidCreation.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 호스트 포인터를 원본32비트 주소로 역참조하지 않는다.
std::uint32_t CreateRawSquid(OriginalEdition edition,std::uint32_t type,std::uint32_t flags,const RawSquidCreationHooks& hooks) {
    const bool patch=edition==OriginalEdition::Patch1078;
    const auto diagnose=[&](std::string_view expression,std::uint32_t line) { // 선택된 진단만 장치를 요구하며 호출 중 재연결에도 수명을 유지한다.
        const auto selected=hooks.assertion;
        if (!selected) throw std::logic_error("Create의 선택된 원문 진단 장치 미연결");
        selected(expression,patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp",line);
    };
    if (patch) {
        if (!hooks.fakeSurfaceType) throw std::logic_error("Create의 현재 fakeSurface TYPE 미연결");
        if (type==hooks.fakeSurfaceType()) diagnose("type != fakeThreeByThreeSurfaceType",0x189);
    }
    if (!hooks.allocate) throw std::logic_error("Create의 실제 SID 할당 미연결");
    const auto sid=hooks.allocate(flags);
    const auto signedSid=std::bit_cast<std::int32_t>(sid);
    const std::int32_t predictable=patch ? 23001 : 14000;
    if (flags&1U) {
        if (signedSid<predictable) diagnose("sid >= FIRST_PREDICTABLE_SID",patch ? 0x191 : 0x188);
    } else if (signedSid>=predictable) diagnose("sid < FIRST_PREDICTABLE_SID",patch ? 0x194 : 0x18b);
    if (!hooks.constructor) throw std::logic_error("Create의 현재 TYPE 생성자 조회 미연결");
    const auto address=hooks.constructor(type);
    if (!hooks.construct) throw std::logic_error("Create의 실제 cdecl 생성자 미연결");
    const auto object=hooks.construct(address,sid);
    if (!hooks.vtable) throw std::logic_error("Create의 실제 객체 VT 조회 미연결");
    const auto table=hooks.vtable(object); // 양판본 ASM은 TYPE BYTE 쓰기 전에 VT DWORD만 저장한다.
    if (!hooks.storeType) throw std::logic_error("Create의 반환 객체 TYPE 쓰기 미연결");
    hooks.storeType(object,static_cast<std::uint8_t>(type));
    if (!hooks.postCreate) throw std::logic_error("Create의 실제 가상+28 미연결");
    hooks.postCreate(object,table);
    return object;
}
}
