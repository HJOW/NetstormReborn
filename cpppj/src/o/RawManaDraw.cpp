// 소유자/dead/추상 관문을 추가하지 않고 원문 마나 추가 Draw의 조회 순서를 유지한다.
#include "o/RawManaDraw.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 클라이언트의 화면/선택 출력이 공유하는 원문 몸체다.
namespace { // 정렬되지 않은 raw와 signed 좌표 감김의 작은 읽기 경계다.
std::uint32_t Frame(std::span<const std::uint8_t> raw) { // 원문+24 DWORD를 각 호출 때 다시 읽는다.
    std::uint32_t value{};
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[36+index])<<(index*8); // little endian 네 BYTE다.
    return value;
}
std::int32_t ShadowCoordinate(std::int32_t value) { return std::bit_cast<std::int32_t>(std::uint32_t(value)+10U); } // 원문의 LEA+10 하위 DWORD 감김이다.
}
void DrawManaAdditional(std::span<const std::uint8_t> raw,std::int32_t x,std::int32_t y,const ManaAdditionalHooks& hooks) { // raw/TYPE 저장소는 두 출력 동안 같은 주소로 살아 있어야 한다.
    if (raw.size()<40 || !hooks.shape || !hooks.draw) throw std::invalid_argument("마나 추가 Draw raw/SHP/출력 경계 필요");
    const auto type=raw[10]; // 첫4ac550의 TYPE 선택을 유지하며 그림자 뒤 바뀐 raw TYPE로 다시 선택하지 않는다.
    if ((std::uint32_t(raw[12])|(std::uint32_t(raw[13])<<8))!=0) {
        const auto frame=Frame(raw);const auto shape=hooks.shape(type); // 원문은 프레임을 먼저 읽고 선택한 TYPE의 현재 SHP를 읽는다.
        hooks.draw({shape,frame,ShadowCoordinate(x),ShadowCoordinate(y),true});
    }
    const auto frame=Frame(raw);const auto shape=hooks.shape(type); // 그림자 소비자가 바꾼 프레임/SHP를 본체에 반영한다.
    hooks.draw({shape,frame,x,y,false});
}
}
