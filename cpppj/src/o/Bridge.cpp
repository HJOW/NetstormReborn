// 원본: Bridge.cpp 00421770/004217f0/00421c30. CD 00449e30/00449250의 인라인 판단/0044a1c0.
// 검증: recovery-bridge-evidence.json의 한 방향 검사·패치 열린 끝·수명 함수 접두 구간 기대값.
// 추가: 00441e40 ↔ CD 004d2e00 연결 계산, 004218b0 ↔ CD 00449f10 방문 목록은 recovery-surface-evidence.json.
#include "o/Bridge.h"
#include "o/TerrainBuilder.h"
#include "o/SquidFinder.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 0052f8dc / CD 0051c3d0: 북부터 시계 방향인 8방향을 연결 비트로 바꾼다.
constexpr int kDirectionBits[]{1,2,2,4,4,8,8,1};
// 정상 수명은 7까지이며 원본 debug 검사도 이 범위를 요구한다.
constexpr int kMaximumLife = 7;
// 00500edc의 실제 float 비트 3f7ff972(0.9998999834060669). 표면 조회의 거의 올림인 칸 변환이다.
constexpr float kSurfaceCoordinateBias = 0.9999f;
// 00441e40에서 다리/폭탄과 섬/건물 사이 연결을 보정하는 원본 타입 플래그 2 마스크다.
constexpr std::uint32_t kBridgeOrBombMask = 0x104, kLandObjectMask = 0x1044202;
// 원본의 고리 재귀에는 방문 제한이 없다. 새 코드에서 스택 고갈을 막는 한계이며 원본 상수가 아니다.
constexpr int kSafeRecursionDepth = 256;
// 0040eaf0의 어셈블리는 좌표에 위 상수를 더하고 _ftol한다. raw C에는 이 덧셈이 빠져 있다.
int CellCoordinate(float value) {
    // x87의 중간값처럼 float로 다시 좁히지 않는다. 정수 경계 바로 아래 값이 다음 칸으로 넘어가면 안 된다.
    const double shifted = static_cast<double>(value) + static_cast<double>(kSurfaceCoordinateBias);
    if (!std::isfinite(shifted) || shifted < std::numeric_limits<int>::min() || shifted > std::numeric_limits<int>::max())
        throw std::out_of_range("Bridge coordinate");
    return static_cast<int>(shifted);
}
}

// 타입/프레임 연결은 소유자/동맹/배치 가능 여부와 독립된 원본 계산이다.
bool Bridge::Connects(FrameCode first, std::uint32_t firstFlags2,
    FrameCode second, std::uint32_t secondFlags2, int direction) {
    if (direction<0 || direction>=8) throw std::out_of_range("Surface connection direction");
    int a=direction%2==0 ? first.side : first.variant;
    int b=direction%2==0 ? second.side : second.variant;
    if ((firstFlags2 & TypeFlag2::kEmplacement)!=0) a='P';
    if ((secondFlags2 & TypeFlag2::kEmplacement)!=0) b='P';
    if ((firstFlags2 & kBridgeOrBombMask)!=0 && (secondFlags2 & kLandObjectMask)!=0) b='A';
    else if ((secondFlags2 & kBridgeOrBombMask)!=0 && (firstFlags2 & kLandObjectMask)!=0) a='A';
    if (a<'A' || a>'P' || b<'A' || b>'P') throw std::out_of_range("Surface connection code");
    return (TerrainBuilder::Connection(static_cast<char>(a)) & kDirectionBits[direction])!=0 &&
        (TerrainBuilder::Connection(static_cast<char>(b)) & kDirectionBits[(direction+4)%8])!=0;
}
// 재귀 목록은 전체 방문 집합이 아니다. 부모만 제외하고 경계에서는 같은 번호를 모두 지운다.
BridgeDecayWalk Bridge::CollectDecay(const SurfaceFinder& surfaces, std::uint16_t root, std::size_t capacity) {
    const auto& start=surfaces.Object(root);
    if (start.dead || (start.flags2 & TypeFlag2::kBridge)==0 || capacity==0 || capacity>100)
        throw std::out_of_range("Bridge decay root/capacity");
    BridgeDecayWalk result; result.visited.push_back(root);
    std::vector<std::uint16_t> active;
    // 단단한 그림 여부는 이 함수가 거르지 않는다. 원본 호출자가 구동자와 프레임을 먼저 검사한다.
    const auto visit=[&](auto&& self,std::uint16_t id,int depth,std::uint16_t parent)->void {
        if (depth>=kSafeRecursionDepth || std::find(active.begin(),active.end(),id)!=active.end()) {
            result.complete=false; result.canDecay=false; return;
        }
        active.push_back(id);
        // 탐색기 순서와 중복 목록 추가/전체 삭제를 그대로 보존한다.
        for (const auto neighbor:surfaces.Neighbors(id)) {
            const auto& object=surfaces.Object(neighbor);
            const bool bridge=(object.flags2 & TypeFlag2::kBridge)!=0;
            const bool island=(object.flags2 & TypeFlag2::kIsland)!=0;
            if ((!bridge && !island) || neighbor==parent) continue;
            if (bridge && result.visited.size()<capacity) result.visited.push_back(neighbor);
            if (object.frame.side<'J' && !island) {
                if (depth==0 && surfaces.Neighbors(neighbor).size()<3) result.shortJunction=true;
                self(self,neighbor,depth+1,id);
                if (!result.complete) break;
            } else if (depth==0 || (depth==1 && result.shortJunction)) {
                result.canDecay=true; std::erase(result.visited,neighbor);
            } else {
                const auto count=bridge ? surfaces.Neighbors(neighbor).size() : 0;
                if (count<2) result.canDecay=true;
                else std::erase(result.visited,neighbor);
            }
        }
        active.pop_back();
    };
    visit(visit,root,0,0);
    return result;
}

// 이웃 목록에 없는 표면만 열린 방향이다. 지도 밖 또는 번호 0은 열린 것으로 남는다.
bool Bridge::IsOpen(char side, int direction, float x, float y,
    std::span<const std::uint16_t> surface, std::span<const int> neighbors) {
    if (direction < 0 || direction >= 8 || surface.size() != kWorldCells*kWorldCells || side < 'A' || side > 'P')
        throw std::out_of_range("Bridge direction/map/side");
    if ((TerrainBuilder::Connection(side) & kDirectionBits[direction]) == 0) return false;
    const int cellX = CellCoordinate(x + static_cast<float>(kNeighborCells[direction].first));
    const int cellY = CellCoordinate(y + static_cast<float>(kNeighborCells[direction].second));
    if (cellX < 0 || cellY < 0 || cellX >= kWorldCells || cellY >= kWorldCells) return true;
    const auto id = surface[static_cast<std::size_t>(cellY*kWorldCells + cellX)];
    return id == 0 || std::find(neighbors.begin(), neighbors.end(), static_cast<int>(id)) == neighbors.end();
}
// 방향을 북부터 시계 방향인 0,2,4,6 순서로 검사한다.
int Bridge::OpenDirection(char side, float x, float y,
    std::span<const std::uint16_t> surface, std::span<const int> neighbors) {
    if (surface.size() != kWorldCells*kWorldCells || side < 'A' || side > 'P')
        throw std::out_of_range("Bridge map/side");
    const int required = side >= 'B' && side <= 'E' ? 2 : side >= 'F' && side <= 'K' ? 1 : 0;
    if (required != 0) {
        int found = 0;
        // 원본은 두 번째/첫 번째 열린 방향을 만난 즉시 반환한다.
        for (int direction = 0; direction < 8; direction += 2)
            if (IsOpen(side, direction, x, y, surface, neighbors) && ++found == required) return direction;
    }
    if (side == 'L') return 0;
    if (side == 'M') return 2;
    if (side == 'N') return 4;
    if (side == 'O') return 6;
    return -1;
}
// 삭제가 예약된 분기는 상태를 저장하지 않는다. 나머지는 수명 비트만 바꾸고 뒤따르는 효과의 조건을 돌려준다.
BridgeLifeChange Bridge::ReduceLife(std::uint16_t word, int reduction, bool server, char side) {
    const int previous = (word & kBridgeLifeMask) >> 3;
    const auto remaining = std::max<std::int64_t>(0, static_cast<std::int64_t>(previous) - reduction);
    if (previous > kMaximumLife || remaining > kMaximumLife || side < 'A' || side > 'P')
        throw std::out_of_range("Bridge lifetime/side");
    const bool remove = server && remaining == 0;
    const auto next = remove ? word : static_cast<std::uint16_t>((word & ~kBridgeLifeMask) | (remaining << 3));
    BridgeLifeChange change{next, previous, static_cast<int>(remaining), remove,
        remove && (side == 'J' || side == 'K') ? kBridgePlankRemoval : 0};
    // 00421cbb 이후: 금은 5 이상에서 5 미만으로 내려갈 때만, 이동체 확인은 새 수명이 5 이하일 때마다 한다.
    change.crack = !remove && remaining < kBridgeCrackLife && previous >= kBridgeCrackLife;
    change.carriers = !remove && remaining <= kBridgeCarrierLife;
    return change;
}
namespace {
// 원본 프레임 번호는 signed char로 읽는다(movsx).
int FrameNumber(const RiftTypeFrames& frames, int frame) {
    const auto codes = frames.Codes();
    if (frame < 0 || static_cast<std::size_t>(frame) >= codes.size()) throw std::out_of_range("Bridge frame");
    return static_cast<std::int8_t>(codes[static_cast<std::size_t>(frame)].number);
}
// 004ad680: 현재 프레임의 방향 글자.
std::uint8_t FrameSide(const RiftTypeFrames& frames, int frame) {
    return frames.Codes()[static_cast<std::size_t>(frame)].side;
}
// 다리 프레임 검색이 쓰는 변형 글자(원본 push 0x50).
constexpr std::uint8_t kBridgeVariant = 'P';
// 금 간 프레임은 보통 번호에 10을 더한 번호, 단단한 프레임은 20 이상이다.
constexpr int kCrackedNumberOffset = 10, kHardNumber = 20;
// 원본 CRT _ftol(004e49c0 ↔ CD 004f161c): 0 방향 절삭한 64비트 정수의 하위 32비트. 범위 밖·NaN은 하위 0이다.
std::int32_t Ftol(double value) {
    if (!(value > -9223372036854775808.0 && value < 9223372036854775808.0)) return 0;
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
}
// 번호는 byte 덧셈이라 하위 8비트만 검색에 쓰인다. 마스크 0x20은 금 간 플래그다.
int Bridge::CrackedFrame(const RiftTypeFrames& frames, int frame, bool force) {
    int number = FrameNumber(frames, frame);
    if (force && number >= kHardNumber) number = 1;
    else if (number >= kCrackedNumberOffset) return -1;
    return frames.FindMasked(FrameSide(frames, frame), kBridgeVariant,
        static_cast<std::uint8_t>(number + kCrackedNumberOffset), kBridgeCrackedFrame);
}
// 단단한 번호는 1로, 금 간 번호는 10을 뺀 번호로 찾는다.
int Bridge::NormalFrame(const RiftTypeFrames& frames, int frame) {
    int number = FrameNumber(frames, frame);
    if (number >= kHardNumber) number = 1;
    else if (number >= kCrackedNumberOffset) number -= kCrackedNumberOffset;
    return frames.FindNumber(FrameSide(frames, frame), kBridgeVariant, static_cast<std::uint8_t>(number));
}
// 수명 비트를 4로 쓴 뒤에 프레임을 찾는다. 프레임을 못 찾아도 수명은 이미 바뀌어 있다.
BridgeFrameChange Bridge::Weaken(const RiftTypeFrames& frames, std::uint16_t word, int frame) {
    const auto next = static_cast<std::uint16_t>((word & ~kBridgeLifeMask) | ((kBridgeCrackLife - 1) << 3));
    const int found = CrackedFrame(frames, frame, true);
    return {next, found == -1 ? frame : found, found != -1};
}
// 수명 비트를 지운 뒤 보통 프레임을 찾는다.
BridgeFrameChange Bridge::Restore(const RiftTypeFrames& frames, std::uint16_t word, int frame) {
    const auto next = static_cast<std::uint16_t>(word & ~kBridgeLifeMask);
    const int found = NormalFrame(frames, frame);
    return {next, found == -1 ? frame : found, found != -1};
}
// 네 조건이 모두 맞을 때만 단단한 프레임/디버그 유지가 destroy를 막는다.
bool Bridge::DestroyProceeds(const BridgeDecayMode& mode, std::uint8_t extra, std::int16_t graphSurfaces,
    std::uint8_t frameFlags) {
    // extra의 abstract(1)·buried(8) 마스크.
    constexpr std::uint8_t kAbstractOrBuried = 9;
    if (!mode.editor && mode.authority && (extra & kAbstractOrBuried) == 0 && graphSurfaces >= kBridgeMinimumGraphSurfaces) {
        if ((frameFlags & kBridgeHardFrame) != 0 || mode.debugKeep) return false;
    }
    return true;
}
namespace {
// vtable +0x10(다리 destroy 재정의 004220f0): 막히지 않으면 기본 destroy가 먼저 dead 비트를 켠다.
// 그래프 254인 칸은 재정의가 NULL 레코드를 읽는 조건에서 거짓을 돌려준다(원본 예외 지점).
bool DestroyBridge(const RiftTypeFrames& frames, BridgeDecayState& state, std::uint32_t flags, const BridgeDecayMode& mode,
    std::vector<BridgeDecayEvent>& events) {
    FrameNumber(frames, state.frame);
    const auto frameFlags = frames.Codes()[static_cast<std::size_t>(state.frame)].flags;
    if (!mode.editor && mode.authority && (state.extra & 9) == 0 && !state.graphValid) return false;
    if (!Bridge::DestroyProceeds(mode, state.extra, state.graphValid ? state.graphSurfaces : std::int16_t{0}, frameFlags))
        return true;
    state.dead = true;
    events.push_back({BridgeDecayEventKind::Destroy, state.id, flags});
    return true;
}
}
// 제거 분기는 수명 단어를 쓰지 않는다. 금 간 프레임을 찾지 못하면 화면 갱신도 하지 않는다.
bool Bridge::ApplyLife(const RiftTypeFrames& frames, BridgeDecayState& state, int reduction, const BridgeDecayMode& mode,
    std::vector<BridgeDecayEvent>& events) {
    FrameNumber(frames, state.frame);
    const auto change = ReduceLife(state.word, reduction, mode.server, static_cast<char>(FrameSide(frames, state.frame)));
    if (change.remove) return DestroyBridge(frames, state, change.removalFlags, mode, events);
    state.word = change.word;
    if (change.crack) {
        const int cracked = CrackedFrame(frames, state.frame, false);
        if (cracked != -1) {
            state.frame = cracked;
            events.push_back({BridgeDecayEventKind::Redraw, state.id, 0});
        }
    } else if (mode.debugRedraw) {
        events.push_back({BridgeDecayEventKind::Redraw, state.id, 0});
    }
    if (change.carriers) events.push_back({BridgeDecayEventKind::Carriers, state.id, 0});
    return true;
}
// 원본의 호출 순서를 그대로 따른다. 수명 변경은 states의 사본에만 반영한다.
BridgeDecayCell Bridge::DecayCell(const SurfaceFinder& surfaces, const RiftTypeFrames& frames,
    std::span<const BridgeDecayState> states, std::uint16_t root, const BridgeDecayMode& mode) {
    // 원본 정적 방문 목록의 용량(0x64)과 한 칸 처리의 지역 이웃 목록 용량(10).
    constexpr std::size_t kVisitedCapacity = 100, kNeighborListCapacity = 10;
    BridgeDecayCell result;
    result.states.assign(states.begin(), states.end());
    // 번호로 상태를 찾는다. 없는 번호는 호출자의 입력 누락이다.
    const auto find = [&](std::uint16_t id) -> BridgeDecayState& {
        const auto found = std::find_if(result.states.begin(), result.states.end(),
            [id](const BridgeDecayState& state) { return state.id == id; });
        if (found == result.states.end()) throw std::out_of_range("Missing bridge decay state");
        return *found;
    };
    const auto& object = surfaces.Object(root);
    if ((object.flags2 & TypeFlag2::kBridge) == 0) throw std::invalid_argument("Bridge decay root is not a bridge");
    const auto codes = frames.Codes();
    // 프레임 번호는 표 안에 있어야 한다.
    const auto code = [&](const BridgeDecayState& state) -> const FrameCode& {
        if (state.frame < 0 || static_cast<std::size_t>(state.frame) >= codes.size()) throw std::out_of_range("Bridge frame");
        return codes[static_cast<std::size_t>(state.frame)];
    };
    bool faulted = false;
    // 시작 칸의 destroy(0)와 방문 칸의 수명 감소는 같은 보조 함수를 쓴다.
    const auto destroy = [&](BridgeDecayState& state, std::uint32_t flags) {
        faulted = !DestroyBridge(frames, state, flags, mode, result.events);
    };
    const auto reduce = [&](BridgeDecayState& state, int reduction) {
        faulted = !ApplyLife(frames, state, reduction, mode, result.events);
    };
    auto& self = find(root);
    if (!self.graphValid) { result.outcome = BridgeDecayOutcome::InvalidGraph; return result; }
    if (self.graphSurfaces < kBridgeMinimumGraphSurfaces) {
        destroy(self, 0);
        result.outcome = faulted ? BridgeDecayOutcome::Faulted : BridgeDecayOutcome::SmallGraph;
        return result;
    }
    if ((code(self).flags & kBridgeHardFrame) != 0) { result.outcome = BridgeDecayOutcome::Hard; return result; }
    // 004b2660: 다리의 flag 8 이웃. 목록에는 앞 10개만 들어가고 개수는 전체를 센다.
    const auto neighbors = surfaces.Neighbors(root);
    if (neighbors.empty()) {
        reduce(self, 1);
        result.outcome = faulted ? BridgeDecayOutcome::Faulted : BridgeDecayOutcome::Isolated;
        return result;
    }
    std::vector<int> listed;
    // 열린 방향 검사는 이 목록에 있는 번호만 이어진 것으로 본다.
    for (std::size_t i = 0; i < neighbors.size() && i < kNeighborListCapacity; ++i) listed.push_back(neighbors[i]);
    if (OpenDirection(static_cast<char>(code(self).side), static_cast<float>(object.x), static_cast<float>(object.y),
            surfaces.Map(), listed) == -1) {
        result.outcome = BridgeDecayOutcome::Closed;
        return result;
    }
    const auto walk = CollectDecay(surfaces, root, kVisitedCapacity);
    if (!walk.complete) { result.outcome = BridgeDecayOutcome::Incomplete; return result; }
    if (!walk.canDecay) { result.outcome = BridgeDecayOutcome::Blocked; return result; }
    // 0이 아닌 수명의 최솟값. 원본은 같은 값도 다시 대입한다(결과는 같다).
    int lowest = std::numeric_limits<int>::max();
    for (const auto id : walk.visited) {
        const int life = (find(id).word & kBridgeLifeMask) >> 3;
        if (life != 0 && life <= lowest) lowest = life;
    }
    const bool none = lowest == std::numeric_limits<int>::max();
    const int clamped = none ? 0 : std::min(lowest, kMaximumLife);
    const int target = none ? kMaximumLife : clamped - 1;
    result.lowest = clamped;
    result.target = target;
    // 방문 순서대로, 그 시점에 dead가 아닌 칸만 목표 수명으로 맞춘다. 중복 항목은 두 번째부터 감소량이 0이다.
    for (const auto id : walk.visited) {
        auto& state = find(id);
        if (state.dead) continue;
        reduce(state, ((state.word & kBridgeLifeMask) >> 3) - target);
        if (faulted) { result.outcome = BridgeDecayOutcome::Faulted; return result; }
    }
    if (!none && target < kBridgeCrackLife && clamped >= kBridgeCrackLife)
        result.events.push_back({BridgeDecayEventKind::CrackSound, root, 0});
    result.outcome = BridgeDecayOutcome::Decayed;
    return result;
}
// 초기화는 범위와 커서를 정하고 첫 주기의 끝 시각을 잡는다.
BridgeDecayScanState BridgeDecayScan::Reset(OriginalEdition edition, double now) {
    // 패치 00422490: 15000, 0x59d9. CD 00449200: 0x1770, 0x36b0.
    const std::int32_t first = edition == OriginalEdition::Cd1072 ? 6000 : 15000;
    const std::int32_t last = edition == OriginalEdition::Cd1072 ? 14000 : 23001;
    return {first, last, first, static_cast<double>(kPeriod) + now};
}
// x87의 delta / 10.0f × 범위는 53비트 정밀도에서 double 연산과 같다(64비트 정밀도와 다른 입력은 기대값에서 뺐다).
BridgeDecayScanStep BridgeDecayScan::Advance(BridgeDecayScanState& state, double now, double delta, bool paused,
    const BridgeDecayMode& mode) {
    if (paused || mode.editor || !mode.authority) return {state.cursor, state.cursor};
    const std::int32_t range = state.last - state.first;
    // 남은 시간이 0보다 클 때만 비례 개수를 쓴다. 0 이하이거나 NaN이면 범위 전체다.
    const std::int32_t count = state.next - now > 0.0
        ? Ftol(delta / static_cast<double>(kPeriod) * static_cast<double>(range)) : range;
    const std::int32_t begin = state.cursor;
    std::int32_t end = begin;
    // 개수가 남아 있고 커서가 끝 번호 이하인 동안 한 번호씩 전진한다.
    if (count > 0 && begin <= state.last) {
        const std::int64_t available = static_cast<std::int64_t>(state.last) - begin + 1;
        end = begin + static_cast<std::int32_t>(std::min<std::int64_t>(count, available));
    }
    state.cursor = end;
    // 주기가 지났으면 커서를 되돌리고 다음 시각을 now 기준으로 다시 잡는다(이전 예정 시각 기준이 아니다).
    if (now >= state.next) {
        state.cursor = state.first;
        state.next = static_cast<double>(kPeriod) + now;
    }
    return {begin, end};
}
// state의 free(1)·dead(2)·void(4)가 모두 꺼져 있고 genus에 다리 비트가 있으며 extra의 abstract(1)가 꺼져 있어야 한다.
bool BridgeDecayScan::Eligible(std::uint8_t state, std::uint32_t flags2, std::uint8_t extra) {
    return (state & 7) == 0 && (flags2 & TypeFlag2::kBridge) != 0 && (extra & 1) == 0;
}
}
