// 원본 모양 탐색 생성자와 flag 8 지도 반복/필터/주변 권한 구간의 읽기 순서를 보존한다.
#include "o/RawCanonPlacementSurrounding.h"
#include "o/Bridge.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 모서리 clamp 범위와 기준점 spot 절삭 보정이다.
constexpr float kFirst=1.0F,kLast=255.0F,kBias=0.9999F;
// signed WORD 중복 캐시가 정상 비교하는 SID와 실제 캐시 용량이다.
constexpr std::size_t kCacheSize=64;
constexpr std::uint16_t kLargestSid=32767;
// 비정렬 raw 필드를 little endian DWORD로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 지정한 필드 폭만 낮은 바이트부터 조립한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 지도 안의 유한 float 계약을 확인하고 원본 CRT처럼 0 방향으로 절삭한다.
int Cell(float value) {
    if (!std::isfinite(value) || value<0 || value>=256) throw std::out_of_range("주변 표면 좌표 오류");
    return static_cast<int>(value);
}
}
// decoder의 현재 x/y를 사용하며 지형용 snapped 좌표나 충돌 사각형을 대신 쓰지 않는다.
RawCanonSurfaceWalk::RawCanonSurfaceWalk(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const PriestPlainCanonType> frames,std::span<const std::uint16_t> islands,
    std::span<const std::uint8_t> spots,const CanonPlacementQuery& query,const CanonPlacementCell& cell)
    :pool_(pool),types_(types),frames_(frames),islands_(islands),spots_(spots),sourceType_(query.type) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || frames.size()!=types.size() || islands.size()!=65536 || spots.size()!=65536)
        throw std::invalid_argument("주변 표면 자료 크기 오류");
    if (sourceType_>=types.size()) throw std::out_of_range("주변 표면 배치 타입 오류");
    const auto& type=types[sourceType_];const auto codes=frames[sourceType_].frames.Codes();
    if (type.footX<1 || type.footY<1 || type.footX>12 || type.footY>12 || cell.frame<0 || static_cast<std::size_t>(cell.frame)>=codes.size())
        throw std::out_of_range("주변 표면 발자국/프레임 오류");
    Cell(cell.x);Cell(cell.y);sourceFrame_=cell.frame;
    left_=std::clamp(cell.x-static_cast<float>(type.footX-1),kFirst,kLast);
    top_=std::clamp(cell.y-static_cast<float>(type.footY-1),kFirst,kLast);
    right_=std::max(cell.x,left_);bottom_=std::max(cell.y,top_);
    centerX_=static_cast<float>(static_cast<double>(cell.x)-static_cast<double>(type.footX)*0.5);
    centerY_=static_cast<float>(static_cast<double>(cell.y)-static_cast<double>(type.footY)*0.5);
    std::uint8_t bits=0xff;
    // 원본은 발자국을 1..255로 자른 뒤 y/x 순서로 spot을 AND한다.
    for (int y=Cell(top_);y<=std::min(255,Cell(bottom_));++y)
        // 발자국 안 모든 칸의 공통 내부 비트만 사용한다.
        for (int x=Cell(left_);x<=std::min(255,Cell(right_));++x) bits=static_cast<std::uint8_t>(bits&spots_[static_cast<std::size_t>(y*256+x)]);
    if (bits&8) { ended_=true;return; }
    state_.left=static_cast<int>(left_-1.0F);state_.top=static_cast<int>(top_-1.0F);
    state_.right=static_cast<int>(right_+1.0F);state_.bottom=static_cast<int>(bottom_+1.0F);
    state_.x=state_.left;state_.y=state_.top;Next();
}
// 필터는 원본처럼 후보 spot부터 읽고, 접합 계산 뒤 죽음/내부 비트를 검사한다.
bool RawCanonSurfaceWalk::Accept(Sid candidate) const {
    const auto raw=pool_.Slot(candidate);const float x=std::bit_cast<float>(Read(raw,14)),y=std::bit_cast<float>(Read(raw,18));
    Cell(x);Cell(y);const auto spot=static_cast<std::size_t>(static_cast<int>(static_cast<double>(y)+kBias)*256+static_cast<int>(static_cast<double>(x)+kBias));
    if (spot>=spots_.size()) throw std::out_of_range("주변 표면 기준점 spot 오류");
    const auto bits=spots_[spot];if (raw[10]>=types_.size()) throw std::out_of_range("주변 표면 후보 타입 오류");
    const auto& type=types_[raw[10]];if (!(type.flags1&TypeFlag1::kSurface)) return false;
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto frame=Read(raw,patch ? 36 : 34,patch ? 4 : 1);
    const auto codes=frames_[raw[10]].frames.Codes();if (frame>=codes.size()) throw std::out_of_range("주변 표면 후보 프레임 오류");
    const float cx=static_cast<float>(static_cast<double>(x)-static_cast<double>(type.footX)*0.5);
    const float cy=static_cast<float>(static_cast<double>(y)-static_cast<double>(type.footY)*0.5);
    const int direction=RawSquidNeighbors::Direction(cx,cy,centerX_,centerY_);
    const auto sourceCodes=frames_[sourceType_].frames.Codes();
    if (static_cast<std::size_t>(sourceFrame_)>=sourceCodes.size()) throw std::out_of_range("주변 표면 현재 배치 프레임 오류");
    if (!Bridge::Connects(codes[frame],type.flags2,sourceCodes[static_cast<std::size_t>(sourceFrame_)],types_[sourceType_].flags2,direction)) return false;
    return !(raw[11]&2) && !(bits&8);
}
// 확장 사각형의 네 모서리를 제외하고 다음 지도 WORD를 그 시점에 읽는다.
Sid RawCanonSurfaceWalk::Next() {
    state_.current={};if (ended_) return {};
    // 반환 가능한 새 후보가 나오거나 마지막 행을 소진할 때까지 원본 커서를 진행한다.
    for (;;) {
        ++state_.x;
        if (state_.y==state_.top || state_.y==state_.bottom) {
            if (state_.x>=state_.right) { if (++state_.y>state_.bottom) { ended_=true;return {}; }state_.x=state_.left; }
        } else if (state_.x>state_.right) { ++state_.y;state_.x=state_.left+(state_.y==state_.bottom ? 1 : 0); }
        if (state_.x<0 || state_.y<0 || state_.x>=256 || state_.y>=256) continue;
        const Sid candidate{islands_[static_cast<std::size_t>(state_.y*256+state_.x)]};if (!candidate.value) continue;
        if (candidate.value>kLargestSid) throw std::out_of_range("주변 표면 signed SID 범위 오류");
        const auto raw=pool_.Slot(candidate);const float x=std::bit_cast<float>(Read(raw,14)),y=std::bit_cast<float>(Read(raw,18));
        if (left_<=x && x<=right_ && top_<=y && y<=bottom_) continue;
        if (!Accept(candidate) || std::find(state_.returned.begin(),state_.returned.end(),candidate.value)!=state_.returned.end()) continue;
        if (state_.returned.size()==kCacheSize) throw std::out_of_range("주변 표면 중복 캐시 용량 오류");
        state_.returned.push_back(candidate.value);state_.current=candidate;return candidate;
    }
}
// 현재 후보만 반환하고 지도를 다시 읽지 않는다.
Sid RawCanonSurfaceWalk::Current() const { return state_.current; }
// 물리 포인터를 제외한 원본 커서/캐시다.
const CanonSurfaceCursor& RawCanonSurfaceWalk::State() const { return state_; }
// 같은 자료로 만든 권한 helper를 확인한다.
RawCanonPlacementSurrounding::RawCanonPlacementSurrounding(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const PriestPlainCanonType> frames,std::span<const std::uint16_t> islands,std::span<const std::uint8_t> spots,
    const RawCanonPlacementPermission& permission)
    :pool_(pool),types_(types),frames_(frames),islands_(islands),spots_(spots),permission_(permission) {
    if (&pool!=&permission.Pool()) throw std::invalid_argument("주변 표면/권한 SID 풀 불일치");
}
// 바깥 관계 검사와 실제 권한 helper를 모든 반환 표면에 적용하며 true를 누적한다.
bool RawCanonPlacementSurrounding::Inspect(const CanonPlacementQuery& query,const CanonPlacementCell& cell) const {
    RawCanonSurfaceWalk walk(pool_,types_,frames_,islands_,spots_,query,cell);bool result=false;
    const auto owner=std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(query.owner))));
    // 처음 권한을 얻은 뒤에도 원본은 현재 모양의 표면 순회를 계속한다.
    for (Sid candidate=walk.Current();candidate.value;candidate=walk.Next()) if (permission_.Surrounding(candidate,owner)) result=true;
    return result;
}
// 지형 어댑터가 동일 풀을 확인할 수 있게 한다.
const SidPool& RawCanonPlacementSurrounding::Pool() const { return pool_; }
// 모양 종료에서 초기/후보 권한이 false일 때만 실제 주변 순회에 들어간다.
CanonPlacementTerrainHooks MakeCanonSurroundingTerrainHooks(const SidPool& pool,
    const RawCanonPlacementSurrounding& surrounding,CanonPlacementTerrainHooks hooks) {
    if (&pool!=&surrounding.Pool()) throw std::invalid_argument("주변 표면/지형 SID 풀 불일치");
    hooks.shapePermission=[&surrounding](const CanonPlacementQuery& query,const CanonPlacementCell& cell) { return surrounding.Inspect(query,cell); };return hooks;
}
}
