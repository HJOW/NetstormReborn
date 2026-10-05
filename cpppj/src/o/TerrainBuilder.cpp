#include "o/TerrainBuilder.h"
#include <algorithm>
#include <limits>
#include <queue>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 방향 A~P의 연결 표와 청크별 난수 시드 증가량.
constexpr std::array<int,16> kConnections{15,7,14,13,11,6,12,9,3,5,10,4,8,1,2,0};
constexpr std::uint32_t kChunkSeedStep = 0x10e3;
// 성장 실패가 연속되는 최대 횟수(004c0800).
constexpr int kGrowthRetries = 1000;
// 방향 동률은 대각선을 먼저 선택하는 관찰 기반 순서다.
constexpr std::array<int,8> kPathDirections{1,3,5,7,0,2,4,6};
}
// 시드 0 대체와 나머지 연산까지 원본과 같은 정수 난수다.
int TerrainBuilder::Next(std::uint32_t& state, int limit) {
    if (limit <= 0) throw std::invalid_argument("Terrain random limit must be positive");
    if (state == 0) state = 0x0bad0bad;
    state = state * 0x10003u + 3u;
    return static_cast<int>((state >> 16) % static_cast<std::uint32_t>(limit));
}
// 올바른 원본 연결 문자만 허용한다.
int TerrainBuilder::Connection(char side) {
    if (side < 'A' || side > 'P') throw std::invalid_argument("Invalid terrain side");
    return kConnections[static_cast<std::size_t>(side-'A')];
}
// 비트 표는 A~P의 역순이 아니라 원본 고정 표다.
char TerrainBuilder::Orientation(int mask) { return "PNOILJFBMHKEGDCA"[mask & 15]; }
// 양쪽 직선 이웃이 있는 대각선만 남기고 직선 연결을 다시 만든다.
std::pair<char,char> TerrainBuilder::Normalize(char cardinal, char diagonal) {
    const int a = Connection(cardinal); int b = Connection(diagonal), supported = 0, result = 0;
    // 북서·북동·남동·남서의 지지 쌍을 조사한다.
    for (int i=0;i<4;++i) { const int pair=(1<<i)|(1<<((i+3)&3)); if ((a & pair)==pair) supported |= 1<<i; }
    b &= supported;
    // 남은 대각선의 직선 지지 비트를 합한다.
    for (int i=0;i<4;++i) if ((b & (1<<i)) != 0) result |= (1<<i)|(1<<((i+3)&3));
    return {Orientation(result),Orientation(b)};
}
// 범위 밖은 허공이다.
std::uint8_t TerrainBuilder::At(int x,int y) const {
    return x>=0 && y>=0 && x<kWorldCells && y<kWorldCells ? land_[static_cast<std::size_t>(y*kWorldCells+x)] : kEmptyTerrain;
}
// 덮어쓰지 않고 새로 추가된 칸만 센다.
int TerrainBuilder::Put(int x,int y,std::uint8_t territory) {
    if (x<0 || y<0 || x>=kWorldCells || y>=kWorldCells || At(x,y)!=kEmptyTerrain) return 0;
    land_[static_cast<std::size_t>(y*kWorldCells+x)]=territory; return 1;
}
// 원본의 반복 순서를 유지한다.
int TerrainBuilder::Fill(int left,int top,int right,int bottom,std::uint8_t territory) {
    int count=0;
    // 사각형의 열을 먼저 순회한다.
    for (int x=left;x<right;++x)
        // 그 열의 행을 채운다.
        for (int y=top;y<bottom;++y) count+=Put(x,y,territory);
    return count;
}
// 저장 영역의 연결 모양과 시드로 본섬 마스크를 생성한다.
TerrainBuilder::TerrainBuilder(const ChunkMap& map,const IslandList& islands) {
    land_.fill(kEmptyTerrain);
    struct Work { int x,y,island,index,count,target; std::uint8_t territory,seed; int connections; };
    std::vector<Work> work; std::array<int,128> counts{}, indices{};
    // 통로와 성장에 쓸 영역별 청크 수를 먼저 계산한다.
    for (int y=0;y<map.YLength();++y)
        // 월드 x 순서로 유효한 섬 소속을 센다.
        for (int x=0;x<map.XLength();++x) if (islands.Exists(map.At(x,y).island)) ++counts[map.At(x,y).island];
    // 통로 생성 순서는 월드 y·x다.
    for (int y=0;y<map.YLength();++y)
        // 영역 번호 순으로 묶지 않는다.
        for (int x=0;x<map.XLength();++x) {
            const auto& chunk=map.At(x,y); if (!islands.Exists(chunk.island)) continue;
            const auto region=static_cast<std::uint8_t>(islands.Islands()[chunk.island].territory);
            const int index=indices[chunk.island]++, mask=Connection(chunk.side), left=x*16, top=y*16;
            const auto seed=chunk.record[3]; const int cx=left+4+(seed&7),cy=top+4+((9999-seed)&7);
            int added=Fill(cx-1,cy-1,cx+3,cy+3,region);
            added+=Fill((mask&8)?left:cx,(mask&1)?top:cy,(mask&2)?left+16:cx+2,(mask&4)?top+16:cy+2,region);
            if (index==1) added+=Fill(left+3,top+3,left+13,top+13,region);
            std::uint32_t state=static_cast<std::uint32_t>(index)*kChunkSeedStep+seed;
            const int target=std::max(20,Next(state,30)+50-added*100/256)*256/100;
            work.push_back({x,y,chunk.island,index,counts[chunk.island],target,region,seed,mask});
        }
    // 모든 통로가 있는 상태에서 3×3 덩어리를 성장시킨다.
    for (const auto& chunk:work) {
        std::uint32_t state=static_cast<std::uint32_t>(chunk.count+chunk.index)*kChunkSeedStep+chunk.seed;
        int remaining=chunk.target,retries=kGrowthRetries;
        // 연속 실패 한도 또는 목표량에서 끝난다.
        while (remaining>0 && retries>0) {
            --retries; const int x=chunk.x*16+Next(state,15),y=chunk.y*16+Next(state,15); Next(state,1);
            if (At(x,y)!=chunk.territory) continue;
            int added=0;
            // 성장 덩어리의 행을 먼저 순회한다.
            for (int dy=-1;dy<=1;++dy)
                // 다른 영역 청크로는 성장시키지 않는다.
                for (int dx=-1;dx<=1;++dx) {
                    const int nx=x+dx,ny=y+dy;
                    if (nx>=0 && ny>=0 && nx<kWorldCells && ny<kWorldCells && map.At(nx/16,ny/16).island==chunk.island)
                        added+=Put(nx,ny,chunk.territory);
                }
            if (added) { remaining-=added; retries=kGrowthRetries; }
        }
    }
    // 두 번의 순회 중 추가 칸을 즉시 반영한다.
    for (int pass=0;pass<2;++pass)
        // 월드 바깥 한 줄은 조사하지 않는다.
        for (int y=1;y<kWorldCells-1;++y)
            // 직선 이웃이 있는 빈 칸만 조사한다.
            for (int x=1;x<kWorldCells-1;++x) {
                if (At(x,y)!=kEmptyTerrain || (At(x,y-1)==kEmptyTerrain && At(x+1,y)==kEmptyTerrain && At(x,y+1)==kEmptyTerrain && At(x-1,y)==kEmptyTerrain)) continue;
                int run=0; std::uint8_t owner=kEmptyTerrain;
                // 북서·북의 경계에 걸친 연속 이웃도 확인한다.
                for (int direction=0;direction<13;++direction) {
                    const auto [dx,dy]=kNeighborCells[static_cast<std::size_t>(direction&7)]; const auto value=At(x+dx,y+dy);
                    if (value==kEmptyTerrain) { run=0; continue; }
                    if (run==0) owner=value;
                    if (value!=owner) { run=0; continue; }
                    if (++run>4 && (direction&1)!=0) { Put(x,y,owner); break; }
                }
            }
}
// 생성한 마스크의 소유권은 빌더에 남는다.
std::span<const std::uint8_t> TerrainBuilder::Mask() const { return land_; }
// 범위를 검사한 뒤 칸을 조회한다.
GroundCell* GroundGrid::At(int x,int y) { return x>=0 && y>=0 && x<kWorldCells && y<kWorldCells ? &cells_[static_cast<std::size_t>(y*kWorldCells+x)] : nullptr; }
// 읽기 전용 칸 조회도 같은 경계를 쓴다.
const GroundCell* GroundGrid::At(int x,int y) const { return x>=0 && y>=0 && x<kWorldCells && y<kWorldCells ? &cells_[static_cast<std::size_t>(y*kWorldCells+x)] : nullptr; }
// 중립 지면과 소유/동맹 다리는 통과하며 다른 오브젝트 점유는 막는다.
bool GroundGrid::Walkable(CellPoint p,std::uint32_t mover,std::uint16_t allies) const {
    const auto* cell=At(p.x,p.y);
    return cell && (cell->occupant==0 || cell->occupant==mover) &&
        (cell->land || (cell->bridge && cell->owner>0 && cell->owner<=8 && (allies & (1u<<cell->owner))!=0));
}
// 목표 쪽에서 거리장을 만든 뒤 시작점에서 대각선 동률을 우선하여 읽는다.
std::vector<CellPoint> GroundGrid::Path(CellPoint from,CellPoint to,std::uint32_t mover,std::uint16_t allies) const {
    if (!Walkable(from,mover,allies) || !Walkable(to,mover,allies) || from==to) return {};
    const auto step=[&](CellPoint a,CellPoint b) {
        if (!Walkable(b,mover,allies)) return false;
        if (a.x==b.x || a.y==b.y) return true;
        const CellPoint c{a.x,b.y},d{b.x,a.y};
        return At(a.x,a.y)->land && At(b.x,b.y)->land && Walkable(c,mover,allies) && Walkable(d,mover,allies) && At(c.x,c.y)->land && At(d.x,d.y)->land;
    };
    const auto index=[](CellPoint p) { return p.y*kWorldCells+p.x; };
    std::vector<int> distance(kWorldCells*kWorldCells,std::numeric_limits<int>::max());
    using Node=std::pair<int,int>; std::priority_queue<Node,std::vector<Node>,std::greater<Node>> queue;
    distance[static_cast<std::size_t>(index(to))]=0; queue.emplace(0,index(to));
    // 도달 가능한 칸의 최소 비용을 계산한다.
    while (!queue.empty()) {
        const auto [cost,i]=queue.top(); queue.pop(); if (cost!=distance[static_cast<std::size_t>(i)]) continue;
        const CellPoint p{i%kWorldCells,i/kWorldCells}; if (p==from) break;
        // 원본의 여덟 방향 좌표를 사용한다.
        for (int direction:kPathDirections) {
            const auto [dx,dy]=kNeighborCells[static_cast<std::size_t>(direction)]; const CellPoint next{p.x+dx,p.y+dy};
            if (!step(p,next)) continue;
            const int value=cost+((direction&1)?14:10),n=index(next);
            if (value<distance[static_cast<std::size_t>(n)]) { distance[static_cast<std::size_t>(n)]=value; queue.emplace(value,n); }
        }
    }
    std::vector<CellPoint> route; if (distance[static_cast<std::size_t>(index(from))]==std::numeric_limits<int>::max()) return route;
    // 거리가 감소하는 한 칸씩 선택한다.
    for (CellPoint p=from;p!=to;) {
        bool found=false;
        // 동률의 대각선 걸음을 먼저 선택한다.
        for (int direction:kPathDirections) {
            const auto [dx,dy]=kNeighborCells[static_cast<std::size_t>(direction)]; const CellPoint next{p.x+dx,p.y+dy};
            if (!step(p,next)) continue;
            const auto remaining=distance[static_cast<std::size_t>(index(next))];
            if (remaining!=std::numeric_limits<int>::max() && remaining+((direction&1)?14:10)==distance[static_cast<std::size_t>(index(p))]) {
                route.push_back(next); p=next; found=true; break;
            }
        }
        if (!found) return {};
    }
    return route;
}
}
