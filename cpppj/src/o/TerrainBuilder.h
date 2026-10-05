// Terrainbuilder.cpp의 저장 영역 지면 생성. 전투의 무작위 영역 재배치는 별도 복원 대상이다.
#pragma once
#include "o/ChunkMap.h"
#include <array>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace netstorm::o {
// 원본 월드의 한 변은 16청크 × 16칸이다.
inline constexpr int kWorldCells = kChunkMapSide * kCellsPerChunk;
// 본섬 영역 마스크에서 허공을 나타내는 검사 스트림의 값.
inline constexpr std::uint8_t kEmptyTerrain = 255;
// 0052f83c/0052f85c의 북부터 시계 방향인 여덟 이웃.
inline constexpr std::array<std::pair<int,int>,8> kNeighborCells{{{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1}}};
class TerrainBuilder {
public:
    // 004c10a0: 전체 연결 통로 → 청크별 성장 → 두 차례 빈틈 보정 순서를 보존한다.
    TerrainBuilder(const ChunkMap& chunks, const IslandList& islands);
    // 월드 밖과 허공은 255이며 지면은 원본 저장 영역 번호다.
    std::uint8_t At(int x, int y) const;
    // 독립 마스크 대조용 y·x 순서의 65,536바이트.
    std::span<const std::uint8_t> Mask() const;
    // 004bca20: 32비트 오버플로를 사용하는 정수 난수. limit은 양수여야 한다.
    static int Next(std::uint32_t& state, int limit);
    // 0052f910의 방향 문자 A~P를 연결 비트로 바꾼다.
    static int Connection(char side);
    // 0052f8fc의 연결 비트 → 문자 변환.
    static char Orientation(int mask);
    // 0041cd20의 직선/대각선 방향 정규화.
    static std::pair<char,char> Normalize(char cardinal, char diagonal);
private:
    // 빈 칸에 지면을 넣고 실제 추가 여부를 돌려준다.
    int Put(int x, int y, std::uint8_t territory);
    // 004c0930: x 바깥·y 안쪽 순서로 반열린 사각형을 채운다.
    int Fill(int left, int top, int right, int bottom, std::uint8_t territory);
    std::array<std::uint8_t,kWorldCells*kWorldCells> land_{};
};
// 지상 이동에 쓰는 논리 칸. 표시와 경로 탐색이 같은 지지 정보를 공유한다.
struct GroundCell { bool land{}, bridge{}; int owner{}; std::uint32_t occupant{}; };
struct CellPoint {
    int x{}, y{};
    // 경로의 시작·끝과 도착 판정에 두 칸을 비교한다.
    bool operator==(const CellPoint&) const = default;
};
class GroundGrid {
public:
    // 월드 밖은 nullptr이며 가장자리로 끌어당기지 않는다.
    GroundCell* At(int x, int y);
    // 읽기 전용 조회도 같은 월드 경계를 검사한다.
    const GroundCell* At(int x, int y) const;
    // 8방향 최단 경로 어댑터. 원본 탐색 함수 자체를 복원한 것으로 주장하지 않는다.
    // 대각선은 네 칸이 모두 지면이며 통과 가능할 때만 허용한다. 시작 칸은 반환 목록에서 뺀다.
    std::vector<CellPoint> Path(CellPoint from, CellPoint to, std::uint32_t mover, std::uint16_t allies) const;
private:
    // 지면/동맹 다리와 점유 상태로 보행 가능 여부를 판정한다.
    bool Walkable(CellPoint p, std::uint32_t mover, std::uint16_t allies) const;
    std::array<GroundCell,kWorldCells*kWorldCells> cells_{};
};
}
