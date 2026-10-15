#include "o/RawRainVortexDraw.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// little endian 값을 정렬되지 않은 원본 슬롯에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    if (offset+width>raw.size()) throw std::out_of_range("레인 신전 Draw raw 범위 오류");
    std::uint32_t value=0;
    // 지정한 폭만 읽어 CD의 frame 옆 extra를 섞지 않는다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[offset+i])<<(i*8);
    return value;
}
// PATCH DWORD와 CD BYTE 프레임을 원래 폭으로 조회한다.
std::int32_t Frame(OriginalEdition edition,std::span<const std::uint8_t> raw) {
    return edition==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(raw,36)) : static_cast<std::int32_t>(Read(raw,34,1));
}
}
// 같은 이름을 가진 다른 객체를 레인 신전 Draw로 처리하지 않는다.
bool IsRainVortex(OriginalEdition edition,std::span<const std::uint8_t> raw) {
    return Read(raw,0)==(edition==OriginalEdition::Patch1078 ? 0x5196f0U : 0x501d28U);
}
// 0번과 현재 그림을 항상 제출하고 WORD 상태가 있을 때 C구간 추가 그림/소리를 판정한다.
void DrawRainVortex(OriginalEdition edition,std::span<const std::uint8_t> raw,
    const RiftTypeFrames& frames,const RainVortexDrawHooks& hooks) {
    if (!IsRainVortex(edition,raw) || !hooks.frame || !hooks.ascendSound) throw std::invalid_argument("레인 신전 Draw 연결 누락");
    hooks.frame(0);hooks.frame(Frame(edition,raw));
    if (!Read(raw,12,2)) return;
    const auto current=Frame(edition,raw);const auto rise=frames.Run('C');
    if (current>=rise.count) return;
    if (current<0 || static_cast<std::size_t>(current)>=frames.Codes().size()) throw std::out_of_range("레인 신전 Draw 방향 프레임 범위 오류");
    const auto side=frames.Codes()[static_cast<std::size_t>(current)].side;
    if (current==frames.Run(side).first) hooks.ascendSound();
    hooks.frame(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(rise.first)+static_cast<std::uint32_t>(Frame(edition,raw))));
}
}
