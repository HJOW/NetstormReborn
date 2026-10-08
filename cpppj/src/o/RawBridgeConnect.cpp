// 다리/섬 연결·연결 객체 생성·소유자 전파. 세 실제 PE의 제한 x86 기대값(bridgeconnect-x86.tsv)으로 검사한다.
#include "o/RawBridgeConnect.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 공통 raw 필드: 두 번째 참조(+8), 타입(+10), 첫 참조(+12), 위치 float x(+14)·y(+18), 화면 좌표(+22·+24).
constexpr std::size_t kSecondReference = 8, kType = 10, kFirstReference = 12, kX = 14, kY = 18, kScreenX = 22, kScreenY = 24;
// extra의 abstract(1)·buried(8) 마스크.
constexpr std::uint8_t kAbstractOrBuried = 9;
// 00502310 ↔ CD 005017a0의 실제 float 비트. 중심과 사각형의 오른쪽/아래를 거의 올림하는 값이며 spot 조회의 0.9999와 다르다.
constexpr float kRoundUpBias = std::bit_cast<float>(0x3f7fff58u);
// 0052f87c·0052f89c ↔ CD 0051c370·0051c390: 방향(0 북부터 시계 방향 8개)별 한 칸 이동량이다.
constexpr std::array<float, 8> kStepX{0, 1, 1, 1, 0, -1, -1, -1}, kStepY{-1, -1, 0, 1, 1, 1, 0, -1};
// 화면 좌표의 칸 크기와 반올림 값(00503358·0050334c·00503350 ↔ CD 005019cc·005019d8·005019d0).
constexpr double kScreenScaleX = 16.0, kScreenScaleY = 11.0, kScreenBias = 0.5;
// 한 칸 조회가 0단계 해시부터 찾는 타입의 genus: island(2)·bridge(4).
constexpr std::uint32_t kSurfaceLevelGenus = TypeFlag2::kIsland | TypeFlag2::kBridge;
// little endian raw 필드를 호스트 정렬과 무관하게 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw, std::size_t offset, std::size_t width) {
    std::uint32_t value = 0;
    // 낮은 바이트부터 합친다.
    for (std::size_t i = 0; i < width; ++i) value |= static_cast<std::uint32_t>(raw[offset + i]) << (8 * i);
    return value;
}
// 폭 밖의 바이트를 보존하면서 raw 필드를 쓴다.
void Write(std::span<std::uint8_t> raw, std::size_t offset, std::uint32_t value, std::size_t width) {
    // 낮은 바이트부터 쓴다.
    for (std::size_t i = 0; i < width; ++i) raw[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));
}
// raw 좌표 float의 비트를 그대로 읽는다.
float Coordinate(std::span<const std::uint8_t> raw, std::size_t offset) { return std::bit_cast<float>(Read(raw, offset, 4)); }
// 원본 CRT _ftol: 64비트 절삭값의 하위 DWORD다. 비유한/64비트 밖은 정수 불확정값이라 하위 0이다.
std::int32_t Ftol(double value) {
    if (!(value > -9223372036854775808.0 && value < 9223372036854775808.0)) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 0041d7a0 ↔ CD 00440a80: 0.99999를 더한 x87 값을 자른 정수다. 중간값을 단정도로 좁히지 않는다.
std::int32_t RoundUp(float value) { return Ftol(static_cast<double>(value) + static_cast<double>(kRoundUpBias)); }
// 판본별 소유자·extra 필드 위치.
std::size_t OwnerOffset(const SidPool& pool) { return pool.Edition() == OriginalEdition::Patch1078 ? 0x22 : 0x20; }
std::size_t ExtraOffset(const SidPool& pool) { return pool.Edition() == OriginalEdition::Patch1078 ? 0x28 : 0x23; }
// 패치는 +0x24 DWORD(부호 있는 번호), CD는 +0x22 바이트다.
std::int32_t ReadFrame(const SidPool& pool, Sid sid) {
    const auto raw = pool.Slot(sid);
    if (pool.Edition() != OriginalEdition::Patch1078) return raw[0x22];
    return std::bit_cast<std::int32_t>(Read(raw, 0x24, 4));
}
// CD는 하위 바이트만 쓴다(원본 mov byte). 패치는 DWORD 전체를 쓴다.
void WriteFrame(SidPool& pool, Sid sid, std::int32_t frame) {
    auto raw = pool.AllocatedBytes(sid);
    if (pool.Edition() != OriginalEdition::Patch1078) raw[0x22] = static_cast<std::uint8_t>(frame);
    else Write(raw, 0x24, std::bit_cast<std::uint32_t>(frame), 4);
}
}
std::int32_t OwnerColorFrame(const BridgeConnectState& state, std::int32_t owner) {
    if (!state.battle || owner <= 0) return kNeutralOwnerFrame;
    if (static_cast<std::size_t>(owner) >= state.ownerColors.size()) throw std::out_of_range("소유자 색 표 범위 오류");
    // 원본은 표 값에서 1을 뺀다(dec eax). DWORD 감김을 그대로 둔다.
    return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(state.ownerColors[static_cast<std::size_t>(owner)]) - 1U);
}
void SetIslandOwner(SidPool& pool, const BridgeConnectState& state, Sid sid, std::uint32_t owner,
    const std::function<void(Sid, std::uint32_t)>& base) {
    if (!base) throw std::invalid_argument("섬 소유자 지정의 base 훅 누락");
    base(sid, owner);
    // 미션 중에는 중립 프레임의 섬을 색칠하지 않는다. base가 바꾼 뒤의 프레임을 읽는다.
    if (state.mission && ReadFrame(pool, sid) == kNeutralOwnerFrame) return;
    WriteFrame(pool, sid, OwnerColorFrame(state, std::bit_cast<std::int32_t>(owner)));
}
std::function<void(Sid, std::uint32_t)> MakeOwnerDispatch(SidPool& pool, const BridgeConnectState& state,
    std::function<void(Sid, std::uint32_t)> base) {
    if (!base) throw std::invalid_argument("소유자 분배의 base 훅 누락");
    const bool patch = pool.Edition() == OriginalEdition::Patch1078;
    const std::uint32_t island = patch ? kPatchIslandVtable : kCdIslandVtable, stalag = patch ? kPatchStalagVtable : kCdStalagVtable;
    return [&pool, &state, island, stalag, base = std::move(base)](Sid sid, std::uint32_t owner) {
        // 슬롯 맨 앞 DWORD가 가상 표 주소 기록값이다.
        const std::uint32_t vtable = Read(pool.Slot(sid), 0, 4);
        if (vtable == island || vtable == stalag) SetIslandOwner(pool, state, sid, owner, base);
        else base(sid, owner);
    };
}
// 외부 효과는 빠짐없이 연결되어야 하고 이웃 탐색기는 같은 풀을 읽어야 한다.
RawBridgeConnect::RawBridgeConnect(SidPool& pool, const RawSquidNeighbors& neighbors, const BridgeConnectState& state,
    BridgeConnectHooks hooks) : pool_(pool), neighbors_(neighbors), state_(state), hooks_(std::move(hooks)) {
    if (&neighbors_.Pool() != &pool_) throw std::invalid_argument("다리 연결의 이웃 탐색기 풀이 다르다");
    if (!hooks_.create || !hooks_.setOwner || !hooks_.pop || !hooks_.setFrame || !hooks_.notifySurface)
        throw std::invalid_argument("다리 연결 훅이 모두 연결되어야 한다");
}
std::int32_t RawBridgeConnect::Frame(Sid sid) const { return ReadFrame(pool_, sid); }
void RawBridgeConnect::SetFrameField(Sid sid, std::int32_t frame) const { WriteFrame(pool_, sid, frame); }
// 타입 번호가 표 밖이면 원본은 표 뒤의 메모리를 읽는다. 여기서는 거부한다.
std::uint32_t RawBridgeConnect::Genus(Sid sid) const {
    const auto types = neighbors_.Types();
    const std::size_t number = pool_.Slot(sid)[kType];
    if (number >= types.size()) throw std::out_of_range("다리 연결 타입 범위 오류");
    return types[number].flags2;
}
Sid RawBridgeConnect::FindTypeAt(float x, float y, std::uint32_t type) const {
    const auto types = neighbors_.Types();
    if (type >= types.size()) throw std::out_of_range("한 칸 조회 타입 범위 오류");
    // 0041d770으로 자른 값을 다시 _ftol로 읽는다. 결과는 0 방향으로 자른 칸 번호다.
    const int cellX = Ftol(static_cast<double>(x)), cellY = Ftol(static_cast<double>(y));
    RawSquidFinder finder(pool_, neighbors_.Hash(), types);
    // 원본은 현재 번호가 풀 범위 안인 동안 타입을 비교하고, 다르면 일반 Next로 넘어간다. 0이면 못 찾은 것이다.
    for (Sid current = finder.Begin({cellX, cellY, cellX, cellY}, (types[type].flags2 & kSurfaceLevelGenus) == 0 ? 1U : 0U);
         current.value != 0; current = finder.Next()) {
        if (pool_.Slot(current)[kType] == type) return current;
    }
    return {};
}
Sid RawBridgeConnect::Link(Sid first, Sid second, std::uint32_t owner, std::vector<Sid>* preview) const {
    const Sid born = hooks_.create(state_.connectorType, kConnectCreateFlags);
    // first의 위치를 읽고 second의 중심을 거의 올림한 정수 좌표로 만든다.
    const auto origin = pool_.Slot(first);
    float x = Coordinate(origin, kX), y = Coordinate(origin, kY);
    const auto center = neighbors_.Center(second);
    const float targetX = static_cast<float>(RoundUp(center[0])), targetY = static_cast<float>(RoundUp(center[1]));
    // first에서 그 중심을 보는 네 방향으로 한 칸 옮긴다. 합은 x87에서 구해 단정도로 저장한다.
    const int direction = RawSquidNeighbors::Direction(x, y, targetX, targetY);
    x = static_cast<float>(static_cast<double>(kStepX[static_cast<std::size_t>(direction)]) + static_cast<double>(x));
    y = static_cast<float>(static_cast<double>(kStepY[static_cast<std::size_t>(direction)]) + static_cast<double>(y));
    SetFrameField(born, direction / 2);
    hooks_.setOwner(born, owner);
    if (preview == nullptr) {
        hooks_.pop(born, x, y, 0);
        // 004ac220 ↔ CD 004abb00: 첫 참조 단어를 쓰고 연결 객체의 타입이 다리 타입 전역과 같으면 표면 알림을 부른다.
        auto raw = pool_.AllocatedBytes(born);
        Write(raw, kFirstReference, first.value, 2);
        if (static_cast<std::uint32_t>(raw[kType]) == state_.bridgeType) hooks_.notifySurface(born);
        Write(pool_.AllocatedBytes(born), kSecondReference, second.value, 2);
        return born;
    }
    // 미리보기: 등록하지 않고 위치와 화면 좌표만 쓴 뒤 목록에 넣는다. 참조 단어는 쓰지 않는다.
    auto raw = pool_.AllocatedBytes(born);
    Write(raw, kX, std::bit_cast<std::uint32_t>(x), 4);
    Write(raw, kY, std::bit_cast<std::uint32_t>(y), 4);
    Write(raw, kScreenX, static_cast<std::uint32_t>(Ftol(static_cast<double>(x) * kScreenScaleX + kScreenBias)), 2);
    Write(raw, kScreenY, static_cast<std::uint32_t>(Ftol(static_cast<double>(y) * kScreenScaleY + kScreenBias)), 2);
    preview->push_back(born);
    // 원본은 이 뒤에 004b2070(중심, 0x10000000)으로 그 칸의 객체를 조회하지만 결과를 쓰지 않는다. 상태를 바꾸지 않으므로 옮기지 않았다.
    return born;
}
void RawBridgeConnect::PropagateOwner(Sid first, Sid second) const {
    const std::size_t ownerOffset = OwnerOffset(pool_), extraOffset = ExtraOffset(pool_);
    const auto target = pool_.Slot(second);
    const Sid base = FindTypeAt(Coordinate(target, kX), Coordinate(target, kY), state_.islandType);
    // first가 abstract/buried이거나 섬 받침이 없으면 끝이다. 받침에 이미 주인이 있거나 abstract/buried여도 끝이다.
    if ((pool_.Slot(first)[extraOffset] & kAbstractOrBuried) != 0 || base.value == 0) return;
    if (pool_.Slot(base)[ownerOffset] != 0 || (pool_.Slot(base)[extraOffset] & kAbstractOrBuried) != 0) return;
    const std::uint32_t owner = pool_.Slot(first)[ownerOffset];
    if (owner == 0) throw std::logic_error("newPlayerId != INVALID_PLAYER_ID (Bridge.cpp assert)");
    hooks_.setOwner(base, owner);
    hooks_.setFrame(base, OwnerColorFrame(state_, static_cast<std::int32_t>(owner)), 0);
    // 받침의 현재 위치에서 종유석을 찾아 받침의 (방금 바뀐) 프레임을 그대로 준다.
    const auto island = pool_.Slot(base);
    const Sid stalag = FindTypeAt(Coordinate(island, kX), Coordinate(island, kY), state_.stalagType);
    if (stalag.value != 0) hooks_.setFrame(stalag, Frame(base), 0);
    // 0041def0(발자국, 0): 왼쪽/위는 자르고 오른쪽/아래는 거의 올림한 정수 사각형으로 일반 탐색을 한다.
    const auto foot = neighbors_.Footprint(base);
    RawSquidFinder finder(pool_, neighbors_.Hash(), neighbors_.Types());
    // 발자국에 걸리는 모든 객체 가운데 noIsland 타입에만 소유자를 준다. 훅이 바꾼 뒤의 체인/타입을 매번 다시 읽는다.
    for (Sid current = finder.Begin({Ftol(static_cast<double>(foot.left)), Ftol(static_cast<double>(foot.top)),
             RoundUp(foot.right), RoundUp(foot.bottom)});
         current.value != 0; current = finder.Next()) {
        if (static_cast<std::uint32_t>(pool_.Slot(current)[kType]) == state_.noIslandType) hooks_.setOwner(current, owner);
    }
}
void RawBridgeConnect::Connect(Sid sid, std::vector<Sid>* preview) const {
    const std::size_t ownerOffset = OwnerOffset(pool_);
    // 자기 genus로 상대를 정한다. 탐색기를 만들기 전에 한 번만 읽는다.
    const bool bridge = (Genus(sid) & TypeFlag2::kBridge) != 0;
    const std::uint32_t mask = bridge ? kIslandSurfaceMask : TypeFlag2::kBridge;
    RawSquidNeighborWalk walk(neighbors_, sid, NeighborFlag::kSkipAbstract);
    // 연결 객체의 등록과 소유자 지정이 풀·해시를 바꾸므로 다음 이웃은 그 뒤의 상태에서 읽는다.
    for (Sid other = walk.Current(); other.value != 0; other = walk.Next()) {
        if ((Genus(other) & mask) == 0) continue;
        // 다리 쪽이 첫 인자이고 연결 객체의 소유자는 다리 쪽의 현재 소유자다.
        const Sid first = bridge ? sid : other, second = bridge ? other : sid;
        Link(first, second, pool_.Slot(first)[ownerOffset], preview);
        PropagateOwner(first, second);
    }
}
void RawBridgeConnect::IslandPostPopPrefix(Sid island, std::uint32_t flags) const {
    if ((flags & 1U) == 0) return;
    const Sid stalag = hooks_.create(state_.stalagType, kConnectCreateFlags);
    hooks_.setOwner(stalag, pool_.Slot(island)[OwnerOffset(pool_)]);
    // 소유자 지정 뒤의 위치를 읽어 같은 자리에 등록한다.
    const auto raw = pool_.Slot(island);
    hooks_.pop(stalag, Coordinate(raw, kX), Coordinate(raw, kY), 0);
    Connect(island);
}
void RawBridgeConnect::IslandPostPop(Sid island, std::uint32_t flags, const std::function<void(Sid, std::uint32_t)>& base) const {
    if (!base) throw std::invalid_argument("섬 받침 postPop의 공통 후처리 훅 누락");
    IslandPostPopPrefix(island, flags);
    base(island, flags);
}
}
