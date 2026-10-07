// 다리 이벤트 처리기와 끝 칸 변환. 세 실제 PE의 제한 x86 기대값(bridgeevent-x86.tsv)으로 검사한다.
#include "o/RawBridgeEvents.h"
#include "o/Bridge.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 공통 raw 필드: 부모 단어(+8), 타입(+10), 수명 단어(+12), 위치 float x(+14)·y(+18).
constexpr std::size_t kParent = 8, kType = 10, kWord = 12, kX = 14, kY = 18;
// 프레임 코드 플래그 0x40: 단단한 프레임. 이 프레임의 다리 칸은 끝 칸 객체로 교체하지 않는다.
constexpr std::uint8_t kHardFrameFlag = 0x40;
// 소유자·플래그 필드가 가진 비트: 새 객체로 옮기는 플래그 비트(0x10).
constexpr std::uint8_t kCopiedFlagBit = 0x10;
// 원본 assert("!isForm()")가 보는 경계: 이 번호 미만의 타입은 form이다.
constexpr std::uint32_t kFirstObjectType = 0x46;
// 끝 칸 변환의 방향 글자: 현재 프레임 J(0x4a)·K(0x4b), 바꿀 끝 프레임 L·M·N·O.
constexpr std::uint8_t kSideJ = 'J', kSideK = 'K', kEndL = 'L', kEndM = 'M', kEndN = 'N', kEndO = 'O';
// little endian raw 단어를 호스트 정렬과 무관하게 읽는다.
std::uint16_t Word(std::span<const std::uint8_t> raw, std::size_t offset) {
    return static_cast<std::uint16_t>(raw[offset] | (static_cast<std::uint16_t>(raw[offset + 1]) << 8));
}
// raw 단어를 쓴다.
void PutWord(std::span<std::uint8_t> raw, std::size_t offset, std::uint16_t value) {
    raw[offset] = static_cast<std::uint8_t>(value);
    raw[offset + 1] = static_cast<std::uint8_t>(value >> 8);
}
// raw 좌표 float의 비트를 그대로 읽는다.
float Coordinate(std::span<const std::uint8_t> raw, std::size_t offset) {
    std::uint32_t bits = 0;
    // 낮은 바이트부터 합친다.
    for (std::size_t i = 0; i < 4; ++i) bits |= static_cast<std::uint32_t>(raw[offset + i]) << (8 * i);
    return std::bit_cast<float>(bits);
}
// 원본 CRT _ftol은 64비트 절삭값의 하위 DWORD를 쓴다. 비유한/64비트 밖은 정수 불확정값이라 하위 0이다.
std::uint32_t FtolLow(float value) {
    const double wide = value;
    if (!(wide > -9223372036854775808.0 && wide < 9223372036854775808.0)) return 0;
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(wide)));
}
}
RawBridgeEvents::RawBridgeEvents(SidPool& pool, const BridgeEventState& state, BridgeEventHooks hooks)
    : pool_(pool), state_(state), hooks_(std::move(hooks)) {
    if (!hooks_.frames || !hooks_.firstNeighbor || !hooks_.notifySurface || !hooks_.create || !hooks_.destroy ||
        !hooks_.setOwner || !hooks_.pop) throw std::invalid_argument("다리 이벤트 훅이 모두 연결되어야 한다");
}
// 패치는 +0x24 DWORD(부호 있는 인덱스), CD는 +0x22 바이트다.
std::int32_t RawBridgeEvents::Frame(Sid sid) const {
    const auto raw = pool_.Slot(sid);
    if (pool_.Edition() != OriginalEdition::Patch1078) return raw[0x22];
    std::uint32_t value = 0;
    // 낮은 바이트부터 합친다.
    for (std::size_t i = 0; i < 4; ++i) value |= static_cast<std::uint32_t>(raw[0x24 + i]) << (8 * i);
    return static_cast<std::int32_t>(value);
}
// CD는 하위 바이트만 쓴다(원본 mov byte). 패치는 DWORD 전체를 쓴다.
void RawBridgeEvents::SetFrame(Sid sid, std::uint32_t frame) const {
    auto raw = pool_.AllocatedBytes(sid);
    if (pool_.Edition() != OriginalEdition::Patch1078) { raw[0x22] = static_cast<std::uint8_t>(frame); return; }
    // 낮은 바이트부터 쓴다.
    for (std::size_t i = 0; i < 4; ++i) raw[0x24 + i] = static_cast<std::uint8_t>(frame >> (8 * i));
}
// 004ac220 ↔ CD 004abb00: 단어를 쓴 뒤 객체 타입이 다리 타입이면 표면 알림을 부른다.
void RawBridgeEvents::SetWord(Sid sid, std::uint16_t word) const {
    auto raw = pool_.AllocatedBytes(sid);
    PutWord(raw, kWord, word);
    if (static_cast<std::uint32_t>(raw[kType]) == state_.bridgeType) hooks_.notifySurface(sid);
}
void RawBridgeEvents::ConvertEnd(Sid bridge, float x, float y) const {
    // 004215d0의 첫 조건: 디버그 유지가 꺼져 있고 권한이 있어야 한다.
    if (state_.debugKeep || !state_.authority) return;
    const bool patch = pool_.Edition() == OriginalEdition::Patch1078;
    // 소유자·플래그 필드 위치는 판본마다 다르다.
    const std::size_t ownerOffset = patch ? 0x22 : 0x20, flagOffset = patch ? 0x28 : 0x23;
    const std::uint32_t type = pool_.Slot(bridge)[kType];
    if (type < kFirstObjectType) throw std::logic_error("다리 타입이 form이다(004ad680 assert)");
    const RiftTypeFrames& frames = hooks_.frames(type);
    const auto codes = frames.Codes();
    const std::int32_t oldFrame = Frame(bridge);
    if (oldFrame < 0 || static_cast<std::size_t>(oldFrame) >= codes.size()) throw std::out_of_range("다리 현재 프레임 범위 오류");
    const auto raw = pool_.Slot(bridge);
    const float objectX = Coordinate(raw, kX), objectY = Coordinate(raw, kY);
    // 004ad680: 현재 프레임 코드의 방향 글자를 signed char로 읽는다. J는 y, K는 x를 비교한다.
    // 비교가 거짓이거나 NaN이면(`!(a<b)`) 각각 L, O 쪽이다(x87 fcomp의 C0/C2 판정과 같다).
    const auto side = static_cast<std::int8_t>(codes[static_cast<std::size_t>(oldFrame)].side);
    std::uint8_t letter = 0;
    if (side == static_cast<std::int8_t>(kSideJ)) letter = (objectY >= y) ? kEndL : kEndN;
    else if (side == static_cast<std::int8_t>(kSideK)) letter = (objectX < x) ? kEndM : kEndO;
    else return;
    // 타입의 글자별 첫 프레임 표(+0x2c + 글자*4). 끝 프레임이 없는 타입은 원본도 -1을 프레임으로 써 정의되지 않는 동작이다.
    const int first = frames.Run(letter).first;
    if (first < 0) throw std::logic_error("다리 타입에 끝 프레임 글자가 없다");
    const auto newFrame = static_cast<std::uint32_t>(first);
    // 임시로 새 프레임을 써서 flag 8 이웃 탐색기를 만들고 곧바로 원래 프레임을 되돌린다.
    SetFrame(bridge, newFrame);
    Sid neighbor;
    try { neighbor = hooks_.firstNeighbor(bridge); }
    catch (...) { SetFrame(bridge, static_cast<std::uint32_t>(oldFrame)); throw; }
    SetFrame(bridge, static_cast<std::uint32_t>(oldFrame));
    // 이웃이 없으면 이 칸은 끝 칸이 될 수 없으므로 destroy한다.
    if (neighbor.value == 0) { hooks_.destroy(bridge, 0); return; }
    // 수명 비트(+0xc의 3~6비트)를 (금 간 수명 - 1)로 쓰고 표면 알림을 부른다.
    const auto lifeBits = static_cast<std::uint16_t>((kBridgeCrackLife * 8 - 8) & kBridgeLifeMask);
    const auto word = static_cast<std::uint16_t>((Word(pool_.Slot(bridge), kWord) & ~kBridgeLifeMask) | lifeBits);
    PutWord(pool_.AllocatedBytes(bridge), kWord, word);
    hooks_.notifySurface(bridge);
    // 단단한 프레임 검사는 되돌려 놓은 옛 프레임의 플래그에 대해 한다. 켜져 있으면 객체를 교체하지 않는다.
    const std::int32_t current = Frame(bridge);
    if (current < 0 || static_cast<std::size_t>(current) >= codes.size()) throw std::out_of_range("다리 프레임 범위 오류");
    if (codes[static_cast<std::size_t>(current)].flags & kHardFrameFlag) return;
    // 같은 타입의 새 객체를 만들어 끝 프레임·소유자·수명 단어·부모 단어·플래그 비트를 옮긴다.
    const Sid born = hooks_.create(type);
    SetFrame(born, newFrame);
    hooks_.setOwner(born, pool_.Slot(bridge)[ownerOffset]);
    SetWord(born, Word(pool_.Slot(bridge), kWord));
    PutWord(pool_.AllocatedBytes(born), kParent, Word(pool_.Slot(bridge), kParent));
    auto newRaw = pool_.AllocatedBytes(born);
    newRaw[flagOffset] = static_cast<std::uint8_t>((newRaw[flagOffset] & ~kCopiedFlagBit) | (pool_.Slot(bridge)[flagOffset] & kCopiedFlagBit));
    // 옛 칸의 위치를 읽고 옛 칸을 지운 다음 새 객체를 같은 위치에 놓는다.
    const float atX = Coordinate(pool_.Slot(bridge), kX), atY = Coordinate(pool_.Slot(bridge), kY);
    hooks_.destroy(bridge, 0);
    hooks_.pop(born, atX, atY, 0);
}
float RawBridgeEvents::Handle(Sid bridge, std::uint32_t event, std::uint32_t, float payload) const {
    if (event == kBridgeDestroyEvent) {
        // 권한이 없으면 아무것도 하지 않고 예약을 끝낸다(반환 0).
        if (!state_.authority) return kBridgeEventEnd;
        hooks_.destroy(bridge, 0);
        return kBridgeEventKeep;
    }
    if (event == kBridgeFallEvent) {
        if (!state_.authority) return kBridgeEventEnd;
        // payload는 (x & 255) | (y << 8)을 float로 바꾼 값이다. _ftol 하위 DWORD에서 x는 하위 8비트, y는 부호 있는 오른쪽 8비트 이동이다.
        const std::uint32_t low = FtolLow(payload);
        ConvertEnd(bridge, static_cast<float>(low & 0xffU), static_cast<float>(static_cast<std::int32_t>(low) >> 8));
        return kBridgeEventKeep;
    }
    // 처리하지 않는 이벤트는 payload를 그대로 돌려준다(base 이벤트 처리기와 같다).
    return payload;
}
}
