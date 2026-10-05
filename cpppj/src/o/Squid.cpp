#include "o/Squid.h"
#include <algorithm>
#include <cmath>

namespace netstorm::o {
// 현재 기준 칸에서 다음 칸의 방향을 원본 A~H 순서로 찾는다.
void Squid::Face(CellPoint nextCell) {
    const int dx=nextCell.x-cell.x,dy=nextCell.y-cell.y;
    // 여덟 방향 중 일치하는 한 걸음을 찾는다.
    for (std::size_t i=0;i<kNeighborCells.size();++i) if (kNeighborCells[i]==std::pair{dx,dy}) { heading=static_cast<int>(i); return; }
}
// 경로의 현재 걸음을 부드럽게 이동하며 도착 칸으로 논리 점유를 옮긴다.
bool Squid::Advance(double seconds,GroundGrid& grid,std::uint16_t allies) {
    if (!walking || seconds<=0 || !std::isfinite(seconds)) return false;
    double distance=speed*seconds; animation+=seconds;
    // 긴 프레임에서도 남은 거리를 여러 걸음에 나눠 소비한다.
    while (distance>0 && next<route.size()) {
        const auto target=route[next]; Face(target);
        const auto* ground=grid.At(target.x,target.y);
        if (!ground || (ground->occupant!=0 && ground->occupant!=id) || (!ground->land && (!ground->bridge || ground->owner<=0 || ground->owner>8 || (allies & (1u<<ground->owner))==0))) {
            walking=false; route.clear(); progress=0; x=cell.x; y=cell.y; break;
        }
        const double length=cell.x!=target.x && cell.y!=target.y ? std::sqrt(2.0) : 1.0;
        const double amount=std::min(distance,length-progress); progress+=amount; distance-=amount;
        const double fraction=progress/length; x=cell.x+(target.x-cell.x)*fraction; y=cell.y+(target.y-cell.y)*fraction;
        if (progress+1e-12>=length) {
            auto* previous=grid.At(cell.x,cell.y); if (previous && previous->occupant==id) previous->occupant=0;
            cell=target; grid.At(cell.x,cell.y)->occupant=id; progress=0; ++next;
        }
    }
    if (next>=route.size()) { walking=false; route.clear(); next=0; progress=0; x=cell.x; y=cell.y; }
    return true;
}
}
