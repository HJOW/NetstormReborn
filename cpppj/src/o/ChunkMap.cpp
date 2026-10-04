// 원본: FUN_00432180 @ 00432180(항목 찾기, Chunkmap.cpp) [신뢰도 A] ↔ CD 00404090, FUN_00432210(섬의 범위), FUN_00432300,
//       FUN_00431fc0(기본 범위), FUN_0046ee60·FUN_0046ee30·FUN_0046f200(섬 목록, Islandlist.cpp).
// 범위: 영역 배치에 필요한 부분. 섬 목록의 나머지 필드와 전투용 배치는 옮기지 않았다.
#include "o/ChunkMap.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::o {
// 원본 00431fc0.
ChunkRect DefaultChunkRect() { return {1, 1, 15, 15}; }

// 번호는 더하기 전의 개수다.
int IslandList::Add(int owner, int territory) {
    islands_.push_back({owner, territory});
    return static_cast<int>(islands_.size()) - 1;
}
// 범위 안의 번호만 있다.
bool IslandList::Exists(int island) const { return island >= 0 && static_cast<std::size_t>(island) < islands_.size(); }
// 원본은 실행 상태(DAT_005c85a4·DAT_00594fa4)에 따라 소유자 비교를 건너뛰기도 한다. 여기서는 일반 경로만 옮겼다.
bool IslandList::Matches(int island, int owner, int territory) const {
    if (!Exists(island)) return false;
    const auto& info = islands_[static_cast<std::size_t>(island)];
    return info.owner == owner && info.territory == territory;
}
// 등록 순서의 섬 목록.
const std::vector<IslandInfo>& IslandList::Islands() const { return islands_; }

// 모든 청크를 "섬 없음"으로 둔다.
ChunkMap::ChunkMap(int xLength, int yLength) : xLength_(xLength), yLength_(yLength) {
    if (xLength < 1 || yLength < 1 || xLength > kChunkMapSide || yLength > kChunkMapSide) throw std::invalid_argument("Chunk map size");
}
// 원본 배열은 x × 16 + y 순서다.
ChunkEntry& ChunkMap::At(int x, int y) {
    x = std::clamp(x, 0, xLength_ - 1);
    y = std::clamp(y, 0, yLength_ - 1);
    return entries_[static_cast<std::size_t>(x * kChunkMapSide + y)];
}
// 읽기 전용.
const ChunkEntry& ChunkMap::At(int x, int y) const { return const_cast<ChunkMap*>(this)->At(x, y); }
// 섬 번호가 "섬 없음"이 아니면 섬이 있다.
bool ChunkMap::HasIsland(int x, int y) const { return At(x, y).island != kNoIsland; }
// 원본 00432210: 찾지 못하면 (16, 16, 1, 1)이 된다(원본 그대로).
ChunkRect ChunkMap::Bounds(int island) const {
    int left = kChunkMapSide, top = kChunkMapSide, right = 0, bottom = 0;
    // 지도 전체를 y 바깥, x 안쪽으로 훑는다.
    for (int y = 0; y < yLength_; ++y) {
        // 한 줄의 청크.
        for (int x = 0; x < xLength_; ++x) {
            if (At(x, y).island != island) continue;
            left = std::min(left, x); top = std::min(top, y);
            right = std::max(right, x); bottom = std::max(bottom, y);
        }
    }
    return {left, top, right + 1, bottom + 1};
}
// 가로 청크 수.
int ChunkMap::XLength() const { return xLength_; }
// 세로 청크 수.
int ChunkMap::YLength() const { return yLength_; }
}
