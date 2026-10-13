// 원본 일반 탐색은 buried만 제외한다. free/dead/void를 임의로 추가 필터링하지 않는다.
#include "o/RawSquidFinder.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 두 판본의 공통 슬롯 위치. extra는 패치 +40, CD +35다.
constexpr std::size_t kNext=4,kType=10,kX=14,kY=18;
// 정렬되지 않은 little endian WORD/DWORD를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    std::uint32_t value=0;
    // 원본 바이트 순서를 보존한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
}
// 배열은 복사하지 않는다. 효과 콜백 뒤의 실제 풀/해시 변경을 다음 조회에 반영한다.
RawSquidFinder::RawSquidFinder(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const int> heightExtensions):pool_(pool),hash_(hash),types_(types),heightExtensions_(heightExtensions) {}
// 원본은 Begin 자체에서 첫 Next를 호출한다. 재시작은 이전 커서/필터를 덮어쓴다.
Sid RawSquidFinder::Begin(SquidSearchArea area,std::uint32_t flags,std::function<bool(Sid)> filter) {
    if ((flags&~5u)!=0) throw std::invalid_argument("Raw finder flags");
    if ((flags&4u)!=0 && heightExtensions_.size()<types_.size()) throw std::invalid_argument("Raw finder extended heights");
    area.left=std::max(area.left,0); area.top=std::max(area.top,0);
    area.right=std::min(area.right==-1 ? kWorldCells : area.right,kWorldCells-1);
    area.bottom=std::min(area.bottom==-1 ? kWorldCells : area.bottom,kWorldCells-1);
    state_={}; state_.area=area; state_.skipSurface=(flags&1u)!=0; state_.extendedHeight=(flags&4u)!=0;
    filter_=std::move(filter); started_=true; SetLevel(state_.skipSurface ? 1 : 0);
    state_.next={hash_.Cell(state_.level,state_.x,state_.y)};
    return Next();
}
// 정수 나눗셈은 0 방향 절삭이다. 상한 밖 시작점도 원본 helper처럼 마지막 버킷으로 자른다.
void RawSquidFinder::SetLevel(int level) {
    state_.level=level;
    const int scale=SquidHash::kScales.at(static_cast<std::size_t>(level)),last=SquidHash::Side(level)-1;
    state_.firstX=std::min(state_.area.left/scale,last); state_.firstY=std::min(state_.area.top/scale,last);
    state_.lastX=std::clamp(std::min(state_.area.right/scale,last)+1,0,last);
    state_.lastY=std::clamp(std::min(state_.area.bottom/scale,last)+1,0,last);
    state_.x=state_.firstX; state_.y=state_.firstY; state_.current={}; chain_.clear();
}
// 반환한 객체 다음 체인을 먼저 소비하고 그 뒤 버킷을 전진한다.
bool RawSquidFinder::AdvanceBucket() {
    ++state_.x;
    if (state_.x>state_.lastX) {
        state_.x=state_.firstX; ++state_.y;
        if (state_.y>state_.lastY) {
            ++state_.level;
            if (state_.level>3) { state_.current={}; return false; }
            SetLevel(state_.level);
        }
    }
    chain_.clear(); state_.next={hash_.Cell(state_.level,state_.x,state_.y)}; return true;
}
// 패치는 보드 밖 좌표를 건너뛰고 CD는 assert한다. CD의 손상 좌표는 새 코드에서 예외로 거부한다.
bool RawSquidFinder::Intersects(Sid sid) const {
    const auto raw=pool_.Slot(sid);
    const float x=std::bit_cast<float>(Read(raw,kX,4)),y=std::bit_cast<float>(Read(raw,kY,4));
    const bool valid=std::isfinite(x) && std::isfinite(y) && x>0 && y>0 && x<kWorldCells && y<kWorldCells &&
        (pool_.Edition()==OriginalEdition::Patch1078 || (x>=1 && y>=1));
    if (!valid) {
        if (pool_.Edition()==OriginalEdition::Cd1072) throw std::out_of_range("CD raw finder coordinate");
        return false;
    }
    const auto number=raw[kType];
    if (number>=types_.size()) throw std::out_of_range("Raw finder type");
    const auto& type=types_[number];
    const auto height=static_cast<std::int64_t>(type.footY)+(state_.extendedHeight ? heightExtensions_[number] : 0);
    if (type.footX<1 || height<1 || height>std::numeric_limits<int>::max()) throw std::out_of_range("Raw finder footprint");
    const int right=static_cast<int>(x),bottom=static_cast<int>(y);
    const auto left=static_cast<std::int64_t>(right)-type.footX+1,top=static_cast<std::int64_t>(bottom)-height+1;
    return (state_.area.left<=right || state_.area.left<=left) && (right<=state_.area.right || left<=state_.area.right) &&
        (state_.area.top<=bottom || state_.area.top<=top) && (bottom<=state_.area.bottom || top<=state_.area.bottom);
}
// 원본처럼 후보 next를 먼저 저장한다. 필터/호출자 변경 뒤 그 후보의 변경된 필드를 읽는다.
Sid RawSquidFinder::Next() {
    if (!started_) throw std::logic_error("Raw finder not started");
    if (state_.level>3) return {};
    // 단계별 제한된 버킷 범위와 원본 체인을 순서대로 방문한다.
    for (;;) {
        if (state_.next.value==0) { if (!AdvanceBucket()) return {}; continue; }
        state_.current=state_.next;
        if (state_.current.value>=pool_.Capacity()) throw std::out_of_range("Raw finder SID");
        if (!chain_.insert(state_.current.value).second) throw std::logic_error("Raw finder chain cycle");
        const auto raw=pool_.Slot(state_.current);
        state_.next={static_cast<std::uint16_t>(Read(raw,kNext,2))};
        if ((raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&8u)!=0) continue;
        if (Intersects(state_.current) && (!filter_ || filter_(state_.current))) return state_.current;
    }
}
// 원본 finder의 커서를 읽기만 한다. 외부에서 상태를 조작하지 않는다.
const SquidSearchState& RawSquidFinder::State() const { return state_; }
}
