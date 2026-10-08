// 섬 받침 preDestroy와 noIsland postDestroy. 세 실제 PE의 제한 x86 기대값(islandlifecycle-x86.tsv)으로 검사한다.
#include "o/RawIslandLifecycle.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 공통 raw 필드: 타입(+10), 상태(+11), 위치 float x(+14)·y(+18).
constexpr std::size_t kType = 10, kState = 11, kX = 14, kY = 18;
// 상태의 dead 비트와 extra의 abstract(1)·buried(8) 마스크.
constexpr std::uint8_t kDead = 2, kAbstractOrBuried = 9;
// 한 칸 탐색기에 넘기는 flag 1: 해시 0단계(섬·다리 표면)를 건너뛴다(원본 `004202f0(x, y, 1)`).
constexpr std::uint32_t kSkipSurfaceLevel = 1;
// raw 좌표 float의 비트를 그대로 읽는다.
float Coordinate(std::span<const std::uint8_t> raw, std::size_t offset) {
    std::uint32_t bits = 0;
    // 낮은 바이트부터 합친다.
    for (std::size_t i = 0; i < 4; ++i) bits |= static_cast<std::uint32_t>(raw[offset + i]) << (8 * i);
    return std::bit_cast<float>(bits);
}
// 0041d770으로 자른 값을 다시 _ftol로 읽은 칸 번호다. 비유한/64비트 밖은 원본 정수 불확정값의 하위 0이다.
int Cell(float value) {
    const double wide = value;
    if (!(wide > -9223372036854775808.0 && wide < 9223372036854775808.0)) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(wide))));
}
}
// 외부 효과는 빠짐없이 연결되어야 하고 이웃 탐색기는 같은 풀을 읽어야 한다.
RawIslandLifecycle::RawIslandLifecycle(const SidPool& pool, const RawSquidNeighbors& neighbors, IslandLifecycleHooks hooks)
    : pool_(pool), neighbors_(neighbors), hooks_(std::move(hooks)) {
    if (&neighbors_.Pool() != &pool_) throw std::invalid_argument("섬 삭제 훅의 이웃 탐색기 풀이 다르다");
    if (!hooks_.scheduleFall || !hooks_.fallWalker || !hooks_.basePreDestroy || !hooks_.basePostDestroy)
        throw std::invalid_argument("섬 삭제 훅이 모두 연결되어야 한다");
}
// 타입 번호가 표 밖이면 원본은 표 뒤의 메모리를 읽는다. 여기서는 거부한다.
std::uint32_t RawIslandLifecycle::Genus(Sid sid) const {
    const auto types = neighbors_.Types();
    const std::size_t number = pool_.Slot(sid)[kType];
    if (number >= types.size()) throw std::out_of_range("섬 삭제 훅 타입 범위 오류");
    return types[number].flags2;
}
void RawIslandLifecycle::IslandPreDestroy(Sid island, std::uint32_t flags) const {
    const std::size_t extraOffset = pool_.Edition() == OriginalEdition::Patch1078 ? 0x28 : 0x23;
    if ((pool_.Slot(island)[extraOffset] & kAbstractOrBuried) == 0) {
        // 중심은 탐색기를 만들기 전에 한 번 구한다. 탐색기는 0x1000이 있어도 만들고 끝까지 순회한다.
        const auto center = neighbors_.Center(island);
        const bool keepBridges = (flags & kIslandDestroyKeepsBridges) != 0;
        RawSquidNeighborWalk walk(neighbors_, island, 0);
        // 예약 훅이 풀을 바꿀 수 있으므로 다음 이웃은 그 뒤의 상태에서 읽는다.
        for (Sid other = walk.Current(); other.value != 0; other = walk.Next()) {
            if (!keepBridges && (Genus(other) & TypeFlag2::kBridge) != 0 && (pool_.Slot(other)[kState] & kDead) == 0)
                hooks_.scheduleFall(other, center[0], center[1]);
        }
    }
    hooks_.basePreDestroy(island, flags);
}
void RawIslandLifecycle::SurfacePostDestroy(Sid surface, std::uint32_t flags) const {
    const std::size_t extraOffset = pool_.Edition() == OriginalEdition::Patch1078 ? 0x28 : 0x23;
    if ((pool_.Slot(surface)[extraOffset] & kAbstractOrBuried) == 0) {
        const auto raw = pool_.Slot(surface);
        const int x = Cell(Coordinate(raw, kX)), y = Cell(Coordinate(raw, kY));
        RawSquidFinder finder(pool_, neighbors_.Hash(), neighbors_.Types());
        // 그 칸에 걸린 객체를 일반 탐색 순서로 훑는다. 낙하 훅이 바꾼 뒤의 체인/타입을 매번 다시 읽는다.
        for (Sid current = finder.Begin({x, y, x, y}, kSkipSurfaceLevel); current.value != 0; current = finder.Next()) {
            if ((Genus(current) & TypeFlag2::kWalker) != 0) hooks_.fallWalker(current);
        }
    }
    hooks_.basePostDestroy(surface, flags);
}
}
