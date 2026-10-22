// 보호 PE에서+78=4acfe0/+a0=41c0f0/+a4=41c100을 각각 확인한 표다.
#include "o/RawPlainUnitCapture.h"
#include <array>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,13> kUnits{{
    {70,0x508cc8},{73,0x50a1e8},{111,0x5040c0},{133,0x50e568},{135,0x503e50},
    {137,0x503d80},{138,0x50a050},{141,0x5040c0},{144,0x50a388},{145,0x50e738},
    {146,0x504198},{156,0x503f20},{158,0x50f210}
}}; //운반/비행/일반 유닛과 같은 빈 추가 그림인 애시드145의 실제 표다.
}
// 타입 번호만 맞는 다른 가상 파생은 이 빈 추가 그림으로 대신하지 않는다.
bool IsPlainUnitCapture(std::span<const std::uint8_t> raw) {
    if (raw.size()<50) return false;std::uint32_t table{};
    for (std::size_t index=0;index<4;++index) table|=std::uint32_t(raw[index])<<(index*8); //비정렬VT의 DWORD를 읽는다.
    for (const auto& [type,value]:kUnits) if (raw[10]==type && table==value) return true; //정확한 두 필드만 비교한다.
    return false;
}
}
