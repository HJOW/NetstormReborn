// 원본: FUN_0046dba0 @ 0046dba0(패턴 놓기, Islandbuilder.cpp) [신뢰도 A] ↔ CD 00490750,
//       FUN_0046dd70 @ 0046dd70(전체 놓기) [A] ↔ CD 00490980, FUN_004be2c0 @ 004be2c0(Template.cpp의 영역 청크 순회).
// 범위: 미션·요새를 읽는 경로(저장된 위치를 그대로 쓴다). 지도가 전역 청크 지도일 때만 섬 목록에 등록하는 조건은
//       항상 참으로 두었다(다른 지도에 놓는 경로를 옮기지 않았다).
// 검증: 실제 `.fort` 전체에서 영역마다 놓인 청크 수가 `TerrNN`의 청크 레코드 수와 같다(tools/cpp_fort_smoke.py).
#include "o/Islandbuilder.h"
#include <stdexcept>

namespace netstorm::o {
namespace {
// 레코드 첫 바이트: 하위 6비트는 모양 번호, 상위 2비트는 방향 값.
constexpr std::uint8_t kShapeMask = 0x3f;
constexpr unsigned kDirectionShift = 6;
// 레코드 둘째 바이트의 플래그: bit 0이 켜지고 bit 1이 꺼져야 놓는다.
constexpr std::uint8_t kActiveFlag = 0x01;
constexpr std::uint8_t kSkipFlag = 0x02;
}

// 범위가 정해져 있어야 한다(원본 assert "cRect.isValid()").
IslandBuilder::IslandBuilder(ChunkMap& map, IslandList& islands, const RiftTypeFrames& pieceFrames, int owner, ChunkRect rect)
    : map_(&map), islands_(&islands), pieceFrames_(&pieceFrames), rect_(rect), owner_(owner) {}

// 원본 0046dba0.
int IslandBuilder::Place(std::span<const std::uint8_t> record, int territory, bool ignorePosition) {
    if (record.size() < kTerritoryRecordBytes) throw std::invalid_argument("Territory record");
    // 위치 바이트: 하위 니블 x, 상위 니블 y(오브젝트 위치 바이트와 반대다).
    const int offsetX = ignorePosition ? 0 : record[2] & 0xf;
    const int offsetY = ignorePosition ? 0 : record[2] >> 4;
    const int island = islands_->Add(owner_, territory);
    const std::size_t shape = record[0] & kShapeMask;
    const auto patterns = TerritoryPatterns();
    if (shape >= patterns.size()) throw std::out_of_range("Territory shape");
    CanonDecoder decoder(*pieceFrames_, patterns[shape], record[0] >> kDirectionShift,
        static_cast<float>(rect_.left + offsetX), static_cast<float>(rect_.top + offsetY));
    // 유효한 칸마다 청크 항목을 채운다.
    for (int ordinal = 0; decoder.Valid(); ++ordinal) {
        auto& entry = map_->At(static_cast<int>(decoder.X()), static_cast<int>(decoder.Y()));
        entry.island = static_cast<std::uint8_t>(island);
        // 레코드 6바이트를 그대로 보관한다.
        for (std::size_t i = 0; i < kTerritoryRecordBytes; ++i) entry.record[i] = record[i];
        entry.ordinal = ordinal;
        entry.side = decoder.Side();
        decoder.Advance();
    }
    // [원본] 전역 청크 지도면 섬 목록에 그 섬의 범위(FUN_00432210)를 적는다(FUN_0046f190) — 범위는 ChunkMap::Bounds로 구할 수 있다.
    return island;
}
// 원본 0046dd70.
void IslandBuilder::PlaceAll(std::span<const std::uint8_t> territories, int start, int end) {
    if (start == end) throw std::invalid_argument("iStart != iEnd"); // 원본 assert.
    // 레코드를 번호순으로 본다.
    for (int index = start; index < end; ++index) {
        const auto offset = static_cast<std::size_t>(index) * kTerritoryRecordBytes;
        if (offset + kTerritoryRecordBytes > territories.size()) break;
        const auto flags = territories[offset + 1];
        if ((flags & kActiveFlag) != 0 && (flags & kSkipFlag) == 0) Place(territories.subspan(offset, kTerritoryRecordBytes), index);
    }
}
// 원본 004be2c0의 순회 순서.
std::vector<ChunkCoordinate> TerritoryChunks(const ChunkMap& map, const IslandList& islands, int owner, int territory) {
    std::vector<ChunkCoordinate> chunks;
    // y 바깥, x 안쪽.
    for (int y = 0; y < map.YLength(); ++y) {
        // 한 줄의 청크.
        for (int x = 0; x < map.XLength(); ++x)
            if (islands.Matches(map.At(x, y).island, owner, territory)) chunks.push_back({x, y});
    }
    return chunks;
}
}
