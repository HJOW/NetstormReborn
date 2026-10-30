// 정렬되지 않은 WORD/DWORD와 원문 조기 반환·조회 순서를 보존한다.
#include "o/RawAuxiliaryDraw.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 실제 화면과 픽셀 선택이 공유하는 추가 Draw 몸체다.
namespace { // 필요한 필드만 읽으며 도달하지 않은 프레임/SHP는 요구하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t bytes=4) { // 원문 little endian 필드의 하위값이다.
    if (offset>raw.size() || bytes>raw.size()-offset) throw std::out_of_range("추가 Draw raw 필드 범위 오류");std::uint32_t result{};
    for (std::size_t index=0;index<bytes;++index) result|=std::uint32_t(raw[offset+index])<<(index*8); // WORD 또는 DWORD만 원래 순서로 조합한다.
    return result;
}
void Require(const AuxiliaryDrawHooks& hooks) { if (!hooks.shape || !hooks.draw) throw std::invalid_argument("추가 Draw SHP/출력 경계 필요"); } // 도달한 실제 출력 분기의 필수 장치다.
}
void DrawScaledMetadisplay(std::span<const std::uint8_t> raw,std::int32_t x,std::int32_t y,const AuxiliaryDrawHooks& hooks) { // raw/TYPE 저장소는 이 동기 출력 동안 같은 주소로 유지한다.
    const auto flags=Read(raw,26);if (flags&0x4000U) return;Require(hooks);
    auto scale=((flags&0x3fffU)<<16)+Read(raw,8,2);if (!scale) scale=0x10000U; // 원문 정수 배율과 분수 WORD를 합산하며0만1배로 바꾼다.
    const auto frame=Read(raw,36);const auto shape=hooks.shape(raw[10]); // 원문은 프레임을 준비한 뒤 현재 TYPE의 SHP를 조회한다.
    hooks.draw({shape,frame,x,y,false,std::bit_cast<std::int32_t>(scale)});
}
void DrawAbstractAdditional(std::span<const std::uint8_t> raw,std::int32_t x,std::int32_t y,const AuxiliaryDrawHooks& hooks) { // 별도 visible/dead/owner 필터를 추가하지 않는다.
    Require(hooks);const auto frame=Read(raw,36);const auto shape=hooks.shape(raw[10]);hooks.draw({shape,frame,x,y,true,0});
}
}
