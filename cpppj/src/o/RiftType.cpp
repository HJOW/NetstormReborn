// 원본 RiftType.cpp. 자동 대응 0049a9e0 ↔ CD 00444350은 다른 오버로드이므로 사용하지 않는다.
// 00444410을 역어셈블·ret 16·두 판본 기계어 에뮬레이션으로 확인했다.
// 범위: 순차 검색 3개. 원본 assert 후 -1은 오류 보고 계층 없이 -1만 반환한다.
#include "o/RiftType.h"
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// movsx로 읽는 원본 signed char를 플랫폼 기본 char 부호와 무관하게 재현한다.
std::int32_t SignedFlag(std::uint8_t value) { return std::bit_cast<std::int8_t>(value); }
}
// 원본 반환 인덱스는 signed 32비트이므로 그 범위를 넘는 배열은 거부한다.
RiftTypeFrames::RiftTypeFrames(std::vector<FrameCode> frames) : frames_(std::move(frames)) {
    if (frames_.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::runtime_error("Too many frame codes");
}
// 원본 0049a9a0(B), CD 00444350: 방향·변형·번호의 첫 일치.
int RiftTypeFrames::FindNumber(std::uint8_t side, std::uint8_t variant, std::uint8_t number) const {
    // 첫 일치를 반환하므로 중복 프레임의 원본 순서가 유지된다.
    for (std::size_t i = 0; i < frames_.size(); ++i) {
        const auto& f = frames_[i];
        if (f.side == side && f.variant == variant && f.number == number) return static_cast<int>(i);
    }
    return -1;
}
// 원본 0049a9e0(B), CD 00444410: 플래그를 signed 8비트에서 32비트로 부호 확장한다.
int RiftTypeFrames::FindMasked(std::uint8_t side, std::uint8_t variant, std::uint8_t number, std::uint32_t mask) const {
    // 원본처럼 방향·변형·번호를 확인한 뒤 마스크를 검사한다.
    for (std::size_t i = 0; i < frames_.size(); ++i) {
        const auto& f = frames_[i];
        if (f.side == side && f.variant == variant && f.number == number && (static_cast<std::uint32_t>(SignedFlag(f.flags)) & mask) != 0)
            return static_cast<int>(i);
    }
    return -1;
}
// 원본 0049aa30(A) ↔ CD 00444460(A). 번호는 이 검색의 조건이 아니다.
int RiftTypeFrames::FindFlags(std::uint8_t side, std::uint8_t variant, std::int32_t flags) const {
    // 정확한 플래그 값까지 일치하는 첫 코드를 찾는다.
    for (std::size_t i = 0; i < frames_.size(); ++i) {
        const auto& f = frames_[i];
        if (f.side == side && f.variant == variant && SignedFlag(f.flags) == flags) return static_cast<int>(i);
    }
    return -1;
}
// 원본 코드의 저장 순서를 보존한 배열을 반환한다.
std::span<const FrameCode> RiftTypeFrames::Codes() const { return frames_; }
}
