// 면적 횟수만 가장자리를 순회하고 실패 때 원래 사각형의 안쪽으로 재탐색한다.
#include "o/RawBoardApproach.h"
#include "o/RawSquidNeighbors.h"
#include <bit>
#include <cmath>
#include <stdexcept>
namespace netstorm::o {
namespace {
// 실제 _ftol signed64 반환의 하위 DWORD다. invalid의 하위 DWORD도0이다.
std::int32_t Integer(double v) {
    if (!std::isfinite(v) || v < -0x1p63 || v>=0x1p63) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(v)));
}
// 실제41d400은 대각선이 아닌 네 방향의 중간점을 최종 float에 저장한다.
ConstructionNoticePoint Side(const std::array<float,4>& r,std::uint32_t side) {
    if (side==0 || side==4) return {static_cast<float>((static_cast<double>(r[2])-r[0])*0.5+r[0]),r[side==0 ? 1 : 3]};
    if (side==2 || side==6) return {r[side==2 ? 2 : 0],static_cast<float>((static_cast<double>(r[3])-r[1])*0.5+r[1])};
    throw std::logic_error("접근점41d400 비정상 방향");
}
}
// 이 객체는 좌표/Graph 사본을 만들지 않으며 선택된 경계를 매번 호출한다.
RawBoardApproach::RawBoardApproach(std::function<std::uint32_t(float,float)> graph,std::function<void()> assertion)
    :graph_(std::move(graph)),assertion_(std::move(assertion)) { if (!graph_) throw std::invalid_argument("접근점 실제 GraphAt 미연결"); }
// 유효 게임 사각형에 실제 float 저장 위치와 정수 순회 조건을 적용한다.
ConstructionNoticePoint RawBoardApproach::Find(std::array<float,4>& r,float x,float y,std::uint32_t flags,bool checking) const {
    if (!std::isfinite(x) || !std::isfinite(y) || r[2]<r[0] || r[3]<r[1]) throw std::invalid_argument("접근점 사각형/좌표 오류");
    const auto original=r;const float cx=static_cast<float>((static_cast<double>(r[0])+r[2])*0.5),cy=static_cast<float>((static_cast<double>(r[1])+r[3])*0.5);
    const auto side=(static_cast<std::uint32_t>(RawSquidNeighbors::Direction(x,y,cx,cy))-4U)&7U;const auto destinationGraph=graph_(x,y);
    if (flags&4) { r[0]=static_cast<float>(static_cast<double>(r[0])-1);r[1]=static_cast<float>(static_cast<double>(r[1])-1);r[2]=static_cast<float>(static_cast<double>(r[2])+1);r[3]=static_cast<float>(static_cast<double>(r[3])+1); }
    auto point=Side(r,side);
    // 실제41d0d0의1~255 보정 뒤 사각형 보정이 다시 적용된다.
    if (!(point.x>=1)) point.x=1;if (!(point.y>=1)) point.y=1;if (point.x>255) point.x=255;if (!(point.y<=255)) point.y=255;
    if ((flags&2) && !side) point.x=(static_cast<double>(r[0])+r[2])*0.5<=x ? r[2] : r[0];
    if (r[2]<point.x) point.x=r[2];if (point.x<r[0]) point.x=r[0];if (r[3]<point.y) point.y=r[3];if (point.y<r[1]) point.y=r[1];
    auto remaining=static_cast<std::uint32_t>(Integer((static_cast<double>(r[3])-r[1])*(static_cast<double>(r[2])-r[0])));
    // 원본 반복 상한은 둘레가 아니라 현재 사각형 면적을 절삭한 DWORD다.
    while (remaining) {
        if (graph_(point.x,point.y)==destinationGraph) { r=original;return point; }
        if (Integer(point.y)==Integer(r[1])) {
            if (Integer(r[0])==Integer(point.x)) point.y=static_cast<float>(static_cast<double>(point.y)+1);else point.x=static_cast<float>(static_cast<double>(point.x)-1);
        } else if (Integer(point.x)==Integer(r[2])) point.y=static_cast<float>(static_cast<double>(point.y)-1);
        else if (Integer(point.y)==Integer(r[3])) point.x=static_cast<float>(static_cast<double>(point.x)+1);
        else if (Integer(point.x)==Integer(r[0])) point.y=static_cast<float>(static_cast<double>(point.y)+1);
        else {
            if (checking) { if (!assertion_) throw std::logic_error("접근점 가장자리 assertion 미연결");assertion_(); }
            point={r[0],r[1]};
        }
        --remaining;
    }
    std::array<float,4> inner{static_cast<float>(static_cast<double>(original[0])+1),static_cast<float>(static_cast<double>(original[1])+1),static_cast<float>(static_cast<double>(original[2])-1),static_cast<float>(static_cast<double>(original[3])-1)};
    if ((static_cast<double>(inner[2])-inner[0])*(static_cast<double>(inner[3])-inner[1])>4) return Find(inner,x,y,flags,checking);
    return point;
}
}
