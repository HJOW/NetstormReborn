// 원본 SquidFinder의 표면 지도 탐색. recovery-surface-evidence.json의 정상 반환 기대값으로 검증한다.
#include "o/SquidFinder.h"
#include "o/Bridge.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 spot 비트 8은 섬 안쪽이다. 후보 기준점과 조회 사각형의 공통 비트를 각각 검사한다.
constexpr std::uint8_t kInteriorBit=8;
// 원본 이웃 캐시가 부호 있는 short로 비교하는 정상 오브젝트 번호 범위다.
constexpr std::uint16_t kMaximumSurfaceId=32767;
// 004adea0/0041ce90: 두 발자국 중심 차이의 절댓값 동률이면 세로 방향을 먼저 고른다.
int Direction(const SurfaceObject& from,const SurfaceObject& to) {
    const float x=static_cast<float>(to.x)-static_cast<float>(to.width)*0.5f-
        (static_cast<float>(from.x)-static_cast<float>(from.width)*0.5f);
    const float y=static_cast<float>(to.y)-static_cast<float>(to.height)*0.5f-
        (static_cast<float>(from.y)-static_cast<float>(from.height)*0.5f);
    if (std::abs(x)<=std::abs(y)) return y>0 ? 4 : 0;
    return x<=0 ? 6 : 2;
}
}
// 발자국이 지도 안에 놓인 정수 스냅샷만 지원한다. 런타임 float 사각형 경로를 암묵적으로 근사하지 않는다.
SurfaceFinder::SurfaceFinder(std::span<const SurfaceObject> objects,std::span<const std::uint16_t> map,
    std::span<const std::uint8_t> spots) : map_(map.begin(),map.end()),spots_(spots.begin(),spots.end()) {
    if (map.size()!=kWorldCells*kWorldCells || spots.size()!=map.size()) throw std::out_of_range("Surface map size");
    // 유효한 배열 번호·발자국·중복 번호를 확인한 뒤 입력 필드를 그대로 보존한다.
    for (const auto& object:objects) {
        if (object.id==0 || object.id>kMaximumSurfaceId || object.width<1 || object.height<1 ||
            object.x<object.width-1 || object.y<object.height-1 || object.x>=kWorldCells || object.y>=kWorldCells ||
            !objects_.emplace(object.id,object).second) throw std::out_of_range("Surface object footprint/id");
    }
}
// 표면 지도에 살아 있는 객체 배열의 번호가 있어야 한다.
const SurfaceObject& SurfaceFinder::Object(std::uint16_t id) const {
    const auto found=objects_.find(id);
    if (found==objects_.end()) throw std::out_of_range("Missing surface object");
    return found->second;
}
// 동일한 중심/방향 동률 규칙과 원본 프레임 접합 표를 사용한다.
bool SurfaceFinder::Connects(const SurfaceObject& from,const SurfaceObject& to) {
    return Bridge::Connects(from.frame,from.flags2,to.frame,to.flags2,Direction(from,to));
}
// 영역 전체가 내부인지 검사하는 원본 AND 계산을 보존한다.
bool SurfaceFinder::Interior(const SurfaceObject& object) const {
    const int left=std::clamp(object.x-object.width+1,1,kWorldCells-1),right=std::clamp(object.x,1,kWorldCells-1);
    const int top=std::clamp(object.y-object.height+1,1,kWorldCells-1),bottom=std::clamp(object.y,1,kWorldCells-1);
    std::uint16_t bits=0xffff;
    // 원본은 y 바깥·x 안쪽 순서로 모든 바이트를 AND한다.
    for (int y=top;y<=bottom;++y)
        // 각 행의 사각형 전체를 확인한다.
        for (int x=left;x<=right;++x) bits&=spots_[static_cast<std::size_t>(y*kWorldCells+x)];
    return (bits & kInteriorBit)!=0;
}
// 이웃 지도에는 셀 번호가 아니라 객체 번호가 들어 있다. 같은 객체가 여러 칸을 차지해도 한 번만 반환한다.
std::vector<std::uint16_t> SurfaceFinder::Neighbors(std::uint16_t id) const {
    const auto& source=Object(id);
    std::vector<std::uint16_t> result;
    if (Interior(source)) return result;
    const int left=source.x-source.width+1,top=source.y-source.height+1;
    // 발자국을 한 칸 넓힌 사각형을 y/x 순서로 훑고 네 모서리는 제외한다.
    for (int y=top-1;y<=source.y+1;++y) {
        const bool edge=y==top-1 || y==source.y+1;
        // 맨 위/아래 행은 확장 사각형의 두 모서리를 읽지 않는다.
        for (int x=left-1+(edge ? 1 : 0);x<=source.x+1-(edge ? 1 : 0);++x) {
            if (x<0 || y<0 || x>=kWorldCells || y>=kWorldCells) continue;
            const auto candidate=map_[static_cast<std::size_t>(y*kWorldCells+x)];
            if (candidate==0) continue;
            const auto& object=Object(candidate);
            if (left<=object.x && object.x<=source.x && top<=object.y && object.y<=source.y) continue;
            if ((object.flags1 & TypeFlag1::kSurface)==0 || object.dead ||
                (spots_[static_cast<std::size_t>(object.y*kWorldCells+object.x)] & kInteriorBit)!=0) continue;
            if (!Connects(object,source)) continue;
            if (std::find(result.begin(),result.end(),candidate)==result.end()) result.push_back(candidate);
        }
    }
    return result;
}
}
