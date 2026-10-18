// 고정 본체를 전용 애니메이션으로 대체하지 않고 현재 raw 조회 순서를 유지한다.
#include "o/RawTempleDraw.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 비정렬 슬롯의 지정 폭만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at,std::size_t width=4) {
    if (at+width>raw.size()) throw std::out_of_range("템플 Draw raw 범위 오류");std::uint32_t value{};
    // 원본 little endian 바이트 순서다.
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[at+index])<<(index*8);return value;
}
// PATCH 프레임은 signed DWORD이며 WORD 상태와 폭을 섞지 않는다.
std::int32_t Current(std::span<const std::uint8_t> raw) { return std::bit_cast<std::int32_t>(Read(raw,36)); }
}
// 이름이나 vortex 플래그가 같은 다른 가상표를 허용하지 않는다.
bool IsPatchTemple(std::span<const std::uint8_t> raw) { const auto table=Read(raw,0);return table==0x519860U || table==0x5196f0U || table==0x519918U; }
// 실제 helper는 current 애니메이션/가상88 대신 TYPE의 baseFrame 한 필드만 읽는다.
std::int32_t PatchTempleMainFrame(std::span<const std::uint8_t> raw,std::span<const std::int32_t> baseFrames) {
    if (!IsPatchTemple(raw) || raw[10]>=baseFrames.size()) throw std::invalid_argument("템플 기본 프레임 자료 불일치");return baseFrames[raw[10]];
}
// 윈드는 고정13보다 앞의 회오리만, 썬더는 모든 현재 프레임을 추가Draw로 제출한다.
void DrawWindThunderTemple(std::span<const std::uint8_t> raw,const RiftTypeFrames& frames,std::int32_t baseFrame,const std::function<void(std::int32_t)>& frame) {
    const auto table=Read(raw,0);if ((table!=0x519860U && table!=0x519918U) || !frame) throw std::invalid_argument("윈드/썬더 전용 Draw 연결 누락");
    if (table==0x519918U || Current(raw)<baseFrame) frame(Current(raw));
    if (!Read(raw,12,2)) return;const auto rise=frames.Run('C');if (Current(raw)>=rise.count) return;
    frame(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(rise.first)+std::bit_cast<std::uint32_t>(Current(raw))));
}
}
