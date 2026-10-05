// 원본: Bridge.cpp 00421770/004217f0/00421c30. CD 00449e30/00449250의 인라인 판단/0044a1c0.
// 검증: recovery-bridge-evidence.json의 한 방향 검사·패치 열린 끝·수명 함수 접두 구간 기대값.
#include "o/Bridge.h"
#include "o/TerrainBuilder.h"
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
// 0040eaf0의 어셈블리는 좌표에 위 상수를 더하고 _ftol한다. raw C에는 이 덧셈이 빠져 있다.
int CellCoordinate(float value) {
    // x87의 중간값처럼 float로 다시 좁히지 않는다. 정수 경계 바로 아래 값이 다음 칸으로 넘어가면 안 된다.
    const double shifted = static_cast<double>(value) + static_cast<double>(kSurfaceCoordinateBias);
    if (!std::isfinite(shifted) || shifted < std::numeric_limits<int>::min() || shifted > std::numeric_limits<int>::max())
        throw std::out_of_range("Bridge coordinate");
    return static_cast<int>(shifted);
}
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
// 삭제가 예약된 분기는 상태를 저장하지 않는다. 나머지는 수명 비트만 바꾼다.
BridgeLifeChange Bridge::ReduceLife(std::uint16_t word, int reduction, bool battle, char side) {
    const int previous = (word & kBridgeLifeMask) >> 3;
    const auto remaining = std::max<std::int64_t>(0, static_cast<std::int64_t>(previous) - reduction);
    if (previous > kMaximumLife || remaining > kMaximumLife || side < 'A' || side > 'P')
        throw std::out_of_range("Bridge lifetime/side");
    const bool remove = battle && remaining == 0;
    const auto next = remove ? word : static_cast<std::uint16_t>((word & ~kBridgeLifeMask) | (remaining << 3));
    return {next, previous, static_cast<int>(remaining), remove,
        remove && (side == 'J' || side == 'K') ? kBridgePlankRemoval : 0};
}
}
