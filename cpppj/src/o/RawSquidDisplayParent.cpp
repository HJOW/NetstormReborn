// 원문 두 frameCheck의 현재 조회와 진단/로그 뒤 표시 인수 재조회를 같은 순서로 실행한다.
#include "o/RawSquidDisplayParent.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 독립 타입/로그 장치를 받아도 부모의 판단 계산은 여기서 수행한다.
namespace { // 원문 BYTE/DWORD 폭과 signed 감김만 공통으로 다룬다.
template<class Callback> const Callback& Need(const Callback& callback) { // 선택한 장치만 요구하며 미연결을 빈 성공으로 숨기지 않는다.
    if (!callback) throw std::logic_error("Squid 표시 부모의 선택된 실제 장치 미연결");return callback;
}
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) { // 정확한4BYTE를 little endian DWORD로 읽는다.
    std::uint32_t result{}; // 현재 필드의 전체 비트다.
    for (std::size_t index=0;index<4;++index) result|=std::uint32_t(raw[offset+index])<<(index*8);return result; // 비정렬 호스트 포인터를 역참조하지 않는다.
}
bool FrameRange(RawSquidDisplayTypeView type,std::int32_t frame) { // 원문의 두 배 count는 low DWORD 감김 뒤 signed 비교다.
    const auto count=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(type.frameCount)*((type.flags1&0x440000U) ? 2U : 1U)); // TYPE 필드는 호출 순간의 현재 값이다.
    return frame>=0 && frame<count;
}
bool ShapePresent(RawSquidDisplayTypeView type) { // null SHP와 두 해제 표식은 유효 프레임 범위와 별개로 검사한다.
    return type.loaded && type.header!=-0x22222223 && type.header!=-0x32323233;
}
void Report(std::span<const std::uint8_t> raw,RawSquidDisplayTypeView type,std::int32_t frame,const RawSquidDisplayParentHooks& hooks) { // TYPE 번호 조회 전에 abstract를 고정한 뒤 실제 세인자 상세 writer를 호출한다.
    const auto abstract=raw[40]&1U;const auto number=Need(hooks.typeNumber)(type);Need(hooks.frameError)(number,frame,abstract); // 두 장치 사이에 checking을 임의로 다시 검사하지 않는다.
}
}
void RunRawSquidDisplayParent(std::span<const std::uint8_t> raw,RawCheckingFlag checking,const RawSquidDisplayParentHooks& hooks) {
    if (raw.size()!=50) throw std::invalid_argument("Squid 표시 부모의 물리50BYTE 슬롯 필요");
    const auto frame=std::bit_cast<std::int32_t>(Read(raw,36)); // 진단이 raw frame을 바꿔도 원문 지역값은 유지한다.
    if (checking) {
        const auto type=Need(hooks.type)(raw[10]);const bool range=FrameRange(type,frame); // 첫 조회가 checking/raw를 바꿔도 선택된 TYPE 뷰를 유지한다.
        if (!range && checking) Report(raw,type,frame,hooks);
        if (!range || !ShapePresent(type)) Need(hooks.assertion)("getTypeStruct()->frameCheck( isAbstract(), theFrame, true )","\\Ns\\O\\Squid.cpp",0x846);
    }
    const auto type=Need(hooks.type)(raw[10]); // 진단 복귀 뒤 현재 raw TYPE 번호로 새 뷰를 조회한다.
    if (FrameRange(type,frame)) {
        if (ShapePresent(type)) Need(hooks.mark)(raw[10],frame,Read(raw,14),Read(raw,18),((raw[40]>>2)&1U)<<4);
    } else if (checking) Report(raw,type,frame,hooks);
}
}
