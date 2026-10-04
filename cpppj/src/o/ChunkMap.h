// 원본 ChunkMap.cpp: 월드를 청크(16 × 16칸) 격자로 나눈 지도. 청크마다 어느 섬(영역)에 속하는지를 담는다.
// 섬 목록(Islandlist.cpp)의 등록 부분도 함께 둔다. 전투의 플레이어별 섬 배치(FUN_00432050의 뒤쪽)는 옮기지 않았다.
#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace netstorm::o {
// 섬이 없는 청크의 섬 번호(원본 DAT_0050c038 = 0x7f).
inline constexpr std::uint8_t kNoIsland = 0x7f;
// 청크 지도의 최대 크기와 한 청크의 칸 수.
inline constexpr int kChunkMapSide = 16;
inline constexpr int kCellsPerChunk = 16;

// 청크 하나의 항목(원본 12바이트).
struct ChunkEntry {
    std::uint8_t island{kNoIsland};        // +0: 섬 목록의 번호.
    char side{};                           // +1: 영역 패턴 칸의 방향 글자(회전 반영).
    std::array<std::uint8_t, 6> record{};  // +2: `Territory` 레코드 6바이트.
    std::int32_t ordinal{};                // +8: 영역 안에서 몇 번째 칸인가.
};

// 청크 좌표의 사각형. right·bottom은 포함하지 않는다.
struct ChunkRect { int left{}, top{}, right{}, bottom{}; };
// 원본 FUN_00431fc0: 영역을 놓을 수 있는 범위의 기본값 (1,1)~(15,15).
ChunkRect DefaultChunkRect();

// 원본 섬 목록(DAT_00568b60)의 항목 가운데 영역 배치에 쓰는 필드.
struct IslandInfo {
    int owner{};        // +0xc, +0x11: 소유 플레이어.
    int territory{};    // +0x10: 영역 번호(0~19).
};
class IslandList {
public:
    // 원본 FUN_0046ee60: 끝에 더하고 그 번호를 돌려준다.
    int Add(int owner, int territory);
    // 원본 FUN_0046ee30: 그 번호의 섬이 있는가.
    bool Exists(int island) const;
    // 원본 FUN_0046f200의 일반 경로: 그 섬이 owner의 territory번 영역인가.
    bool Matches(int island, int owner, int territory) const;
    const std::vector<IslandInfo>& Islands() const;
private:
    std::vector<IslandInfo> islands_;
};

class ChunkMap {
public:
    // 가로·세로 청크 수(원본 +4, +8). 미션 맵은 16 × 16이다.
    ChunkMap(int xLength = kChunkMapSide, int yLength = kChunkMapSide);
    // 원본 FUN_00432180: 범위를 벗어난 좌표는 원본처럼 가장자리로 당긴다(원본은 그 전에 assert 한다).
    ChunkEntry& At(int x, int y);
    const ChunkEntry& At(int x, int y) const;
    // 원본 FUN_00432300: 그 청크에 섬이 있는가.
    bool HasIsland(int x, int y) const;
    // 원본 FUN_00432210: 그 섬이 차지한 청크를 모두 감싸는 사각형.
    ChunkRect Bounds(int island) const;
    int XLength() const;
    int YLength() const;
private:
    int xLength_{}, yLength_{};
    std::array<ChunkEntry, kChunkMapSide * kChunkMapSide> entries_{};
};
}
