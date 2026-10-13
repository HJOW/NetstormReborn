// 원본 Islandbuilder.cpp: `Territory` 레코드의 영역 패턴을 청크 지도에 놓는다.
// 섬 지형 만들기(FUN_0046da70 이후), 전투용 섬 배치는 아직 옮기지 않았다.
#pragma once
#include "o/CanonDecoder.h"
#include "o/ChunkMap.h"
#include "o/RiftType.h"
#include <cstdint>
#include <span>
#include <vector>

namespace netstorm::o {
// `Territory` 섹션의 레코드 하나(6바이트)와 영역 수.
inline constexpr std::size_t kTerritoryRecordBytes = 6;
inline constexpr int kTerritoryCount = 20;

// 원본 빌더 객체: 청크 지도(+0), 놓을 범위(+4~+0x10), 소유 플레이어(+0x14).
class IslandBuilder {
public:
    // 원본 FUN_0046d9b0 + FUN_0046da20. pieceFrames는 영역 패턴 타입(puzzlePiece)의 프레임 코드 표다.
    IslandBuilder(ChunkMap& map, IslandList& islands, const RiftTypeFrames& pieceFrames, int owner,
        ChunkRect rect = DefaultChunkRect());
    // 원본 FUN_0046dba0 ↔ CD 00490750: 레코드 하나의 패턴을 지도에 놓고 섬 번호를 돌려준다.
    // ignorePosition이 참이면 레코드의 위치 바이트를 쓰지 않는다(원본 네 번째 인자).
    int Place(std::span<const std::uint8_t> record, int territory, bool ignorePosition = false);
    // 원본 FUN_0046dd70 ↔ CD 00490980: [start, end) 영역 가운데 플래그 bit 0이 켜지고 bit 1이 꺼진 것만 놓는다.
    void PlaceAll(std::span<const std::uint8_t> territories, int start = 0, int end = kTerritoryCount);
private:
    // map_: 월드를 청크 격자로 나누어 섬·영역 소속을 조회하는 지도 (ChunkMap).
    ChunkMap* map_;
    // islands_: 청크 배치에 사용하는 섬의 소유자·영역 목록 (IslandList).
    IslandList* islands_;
    // pieceFrames_: 타입 프레임의 코드와 방향별 번호 구간을 조회하는 표 (RiftTypeFrames).
    const RiftTypeFrames* pieceFrames_;
    // rect_: 출력·적중·배치 검사에 사용할 사각형.
    ChunkRect rect_;
    // owner_: 객체나 지역을 소유한 플레이어 번호.
    int owner_{};
};

// 월드 청크 좌표.
struct ChunkCoordinate { /* x: X 좌표. */ /* y: Y 좌표. */ int x{}, y{}; };
// 원본 FUN_004be2c0 ↔ CD 00428a00의 순회: 지도를 y 바깥·x 안쪽으로 훑어 owner의 territory번 영역에 속한 청크를 모은다.
// `TerrNN` 섹션의 청크 레코드는 이 순서다.
std::vector<ChunkCoordinate> TerritoryChunks(const ChunkMap& map, const IslandList& islands, int owner, int territory);
}
