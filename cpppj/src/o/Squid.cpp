#include "o/Squid.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 DAT_005424b8 / CD DAT_005395cc: 내부 spot 보정을 받는 건물군 genus 마스크다.
constexpr std::uint32_t kInteriorGenusMask=0x50444200;
// 원본 roof genus 비트. 중심 열의 윗부분은 안쪽 비트 8을 붙이지 않는다.
constexpr std::uint32_t kRoofGenus=0x400000;
// 원본 float 비트 3f7ff972. hash 버킷의 plain _ftol과 다른 좌표 변환이다.
constexpr float kSpotCoordinateBias=0.9999f;
}
// x87 53/64비트 중간 덧셈을 보존하며 float로 다시 좁혀 경계 칸을 바꾸지 않는다.
std::uint32_t Squid::EffectiveGenus(std::uint32_t flags2,float x,float y,int width,int height,int cellX,int cellY) {
    if (!std::isfinite(x) || !std::isfinite(y) || x<0 || y<0 || x>=kWorldCells || y>=kWorldCells ||
        width<1 || height<1 || width>kWorldCells || height>kWorldCells ||
        cellX<0 || cellY<0 || cellX>=kWorldCells || cellY>=kWorldCells)
        throw std::out_of_range("Squid effective genus footprint");
    const int right=static_cast<int>(static_cast<double>(x)+kSpotCoordinateBias);
    const int bottom=static_cast<int>(static_cast<double>(y)+kSpotCoordinateBias);
    const int left=right-width+1,top=bottom-height+1;
    const bool interior=cellX!=left && cellX!=right && cellY!=top && cellY!=bottom;
    const bool roofOpening=(flags2 & kRoofGenus)!=0 &&
        static_cast<std::int64_t>(cellX-left)*2==right-left && static_cast<std::int64_t>(cellY-top)*2<=bottom-top;
    return (flags2 & kInteriorGenusMask)!=0 && interior && !roofOpening ? flags2|8 : flags2;
}
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
