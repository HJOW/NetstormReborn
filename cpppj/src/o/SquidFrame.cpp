// Squid 프레임 지정. 세 실제 PE의 제한 x86 기대값(setframe-x86.tsv)으로 검사한다.
#include "o/SquidFrame.h"
#include "o/SquidHash.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 공통 raw 필드: 타입(+10), 위치 float x(+14)·y(+18).
constexpr std::size_t kType = 10, kX = 14, kY = 18;
// little endian raw DWORD를 호스트 정렬과 무관하게 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw, std::size_t offset) {
    std::uint32_t value = 0;
    // 낮은 바이트부터 합친다.
    for (std::size_t i = 0; i < 4; ++i) value |= static_cast<std::uint32_t>(raw[offset + i]) << (8 * i);
    return value;
}
}
// 외부 효과는 빠짐없이 연결되어야 한다.
SquidFrame::SquidFrame(SidPool& pool, std::span<const RiftTypeRecord> types, SquidFrameHooks hooks)
    : pool_(pool), types_(types.begin(), types.end()), hooks_(std::move(hooks)) {
    if (!hooks_.frameSize || !hooks_.update || !hooks_.unpop || !hooks_.pop)
        throw std::invalid_argument("프레임 지정 훅이 모두 연결되어야 한다");
}
// 패치는 +0x24 DWORD(부호 있는 번호), CD는 +0x22 바이트다.
std::int32_t SquidFrame::Frame(Sid sid) const {
    const auto raw = pool_.Slot(sid);
    if (pool_.Edition() != OriginalEdition::Patch1078) return raw[0x22];
    return std::bit_cast<std::int32_t>(Read(raw, 0x24));
}
int SquidFrame::StoreLevel(Sid sid) const {
    const std::size_t number = pool_.Slot(sid)[kType];
    if (number >= types_.size()) throw std::out_of_range("프레임 지정 타입 범위 오류");
    const std::uint32_t genus = types_[number].flags2;
    int level = 0;
    // island·bridge는 프레임을 읽지 않는다. 그 밖은 현재 프레임의 크기로 정한다.
    if ((genus & (TypeFlag2::kIsland | TypeFlag2::kBridge)) == 0) {
        const auto size = hooks_.frameSize(sid, Frame(sid));
        level = SquidHash::ObjectLevel(genus, size[0], size[1]);
    }
    pool_.AllocatedBytes(sid)[pool_.Edition() == OriginalEdition::Patch1078 ? 0x21 : 0x1f] = static_cast<std::uint8_t>(level);
    return level;
}
void SquidFrame::Set(Sid sid, std::int32_t frame, std::uint32_t flags) const {
    if (flags != 0 && flags != kFrameUpdateAlternate) throw std::invalid_argument("프레임 지정 flags(원본 assert)");
    const bool patch = pool_.Edition() == OriginalEdition::Patch1078;
    // 단계 바이트는 부호 있는 바이트로 읽는다. 단계 계산이 그 자리를 덮어쓰기 전에 읽어 둔다.
    const int stored = static_cast<std::int8_t>(pool_.Slot(sid)[patch ? 0x21 : 0x1f]);
    const std::int32_t current = Frame(sid);
    const int level = StoreLevel(sid);
    // 패치는 DWORD 전체, CD는 하위 바이트만 쓴다(원본 mov byte).
    const auto write = [&] {
        auto raw = pool_.AllocatedBytes(sid);
        if (!patch) { raw[0x22] = static_cast<std::uint8_t>(frame); return; }
        // 낮은 바이트부터 쓴다.
        for (std::size_t i = 0; i < 4; ++i) raw[0x24 + i] = static_cast<std::uint8_t>(std::bit_cast<std::uint32_t>(frame) >> (8 * i));
    };
    if (level == stored) {
        // 같은 단계: 옛 그림 영역과 새 그림 영역을 한 번씩 갱신한다. 다시 등록하지 않는다.
        hooks_.update(sid, flags);
        write();
        hooks_.update(sid, flags);
        return;
    }
    // 단계가 맞지 않는 객체: 프레임이 그대로면 단계 바이트만 고친 채 끝난다. CD는 인자 DWORD와 현재 바이트를 비교한다.
    if (frame == current) return;
    hooks_.unpop(sid, flags);
    write();
    // +0x4c의 넘김 함수는 Unpop 뒤의 위치를 읽어 가상 Pop에 넘긴다.
    const auto raw = pool_.Slot(sid);
    hooks_.pop(sid, std::bit_cast<float>(Read(raw, kX)), std::bit_cast<float>(Read(raw, kY)), flags | kFrameRepopFlags);
}
}
