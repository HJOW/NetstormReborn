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
