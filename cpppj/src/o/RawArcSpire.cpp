// 첫 동맹 스파이어까지의 원본 스캔과 빔/피해/생존 재조회를 보존한다.
#include "o/RawArcSpire.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr double kFtolLimit=9223372036854775808.0; //CRT signed64 불확정 반환의 경계다.
constexpr float kCeilBias=0.9999899864196777F; //41d7a0이 두 float에 더하는 원본502310의 정확한 값이다.
// 원본 _ftol의 하위 DWORD와 비정상 입력의0을 보존한다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || value<-kFtolLimit || value>=kFtolLimit) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 현재 비정렬 float의 네 바이트만 읽는다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits{};
    // 주변 필드는 포함하지 않는다.
    for (std::size_t index=0;index<4;++index) bits|=std::uint32_t(raw[offset+index])<<(8*index);
    return std::bit_cast<float>(bits);
}
// 호출한 효과만 필수로 요구하며 빈 성공으로 대체하지 않는다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("아크 스파이어 실제 하위 미연결");return function; }
}
// 최초 extra9 관문은 common 후처리 전에 한 번 읽는다.
void PostPopArcSpire(const SidPool& pool,Sid parent,std::uint32_t flags,
    const std::function<bool()>& reserve,const std::function<void(Sid,std::uint32_t,float)>& regular,
    const std::function<void(Sid,std::uint32_t)>& damageable) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("아크 스파이어 후처리 판본");
    if (!(pool.Slot(parent)[40]&9U) && (flags&1U) && Need(reserve)()) Need(regular)(parent,0,1.0F);
    Need(damageable)(parent,flags);
}
// 다른 판본/공간 사본을 섞지 않으며 참조는 현재 세션의 저장소를 사용한다.
RawArcSpire::RawArcSpire(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
    std::span<const std::uint8_t> spots,std::span<const std::uint32_t> alliances,const std::uint32_t& editor,ArcSpireHooks hooks)
    :pool_(pool),types_(types),frames_(frames),spots_(spots),alliances_(alliances),editor_(editor),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=188 || spots.size()!=65536 || alliances.size()!=81)
        throw std::invalid_argument("아크 스파이어 같은 원본 자료 크기/판본 오류");
}
// 현재 raw 좌표를 별도 캐시 없이 읽는다.
std::array<float,2> RawArcSpire::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {Coordinate(raw,14),Coordinate(raw,18)}; }
// 방향이 반대인 동맹값으로 대체하지 않는다.
bool RawArcSpire::Allied(std::uint32_t first,std::uint32_t second) const {
    if (first>8 || second>8) throw std::out_of_range("아크 스파이어 동맹 owner 범위");
    return alliances_[first*9+second]!=0;
}
// 손상된 좌표를 배열에 쓰지 않으며 원본의 포함 범위와 각 축 clamp를 유지한다.
std::uint8_t RawArcSpire::SpotUnion(SquidSearchArea area) const {
    area.left=std::clamp(area.left,1,255);area.right=std::clamp(area.right,1,255);
    area.top=std::clamp(area.top,1,255);area.bottom=std::clamp(area.bottom,1,255);std::uint8_t result{};
    // 사각형의 원본 y→x 순서로 OR한다.
    for (int y=area.top;y<=area.bottom;++y) {
        // 경계 두 칸도 포함한다.
        for (int x=area.left;x<=area.right;++x) result|=spots_[static_cast<std::size_t>(y*256+x)];
    }
    return result;
}
// buffer가 가득 찼어도 유효한 첫 이웃이면 원본처럼1을 반환한다.
bool RawArcSpire::Collect(Sid parent,std::span<Sid> buffer,std::uint32_t& count,std::int32_t x,std::int32_t y,std::uint32_t type) const {
    const auto other=Need(hooks_.find)(static_cast<float>(x),static_cast<float>(y),type);if (!other.value) return false;
    const auto raw=pool_.Slot(other);if ((raw[40]&0x21U) || !Allied(pool_.Slot(parent)[34],raw[34]) || !Allied(raw[34],pool_.Slot(parent)[34])) return false;
    const auto point=Position(other);if (point[0]!=static_cast<float>(x) || point[1]!=static_cast<float>(y)) return false;
    if (count<buffer.size()) buffer[count++]=other;
    return true;
}
// 적 후보는 먼저 모두 모으며, 각 빔/피해 뒤 현재 free/dead/void와 owner를 다시 읽는다.
void RawArcSpire::Strike(Sid parent,std::span<const Sid> neighbors,std::int32_t damage) const {
    for (const auto other:neighbors) { //각 동맹 이웃은 독립적인40항목 적 목록을 갖는다.
        const auto source=Position(parent),target=Position(other);
        const auto lowX=static_cast<float>(static_cast<double>(source[0])-2.0),lowY=static_cast<float>(static_cast<double>(source[1])-2.0);
        const SquidSearchArea area{Truncate(lowX),Truncate(lowY),Truncate(static_cast<double>(target[0])+kCeilBias),Truncate(static_cast<double>(target[1])+kCeilBias)};
        std::array<Sid,40> victims{};std::size_t size{};
        Need(hooks_.iterate)(area,[&](Sid candidate) {
            const auto raw=pool_.Slot(candidate);if ((raw[40]&1U) || candidate==parent || candidate==other || Allied(pool_.Slot(parent)[34],raw[34])) return;
            const auto& type=types_[raw[10]];
            const bool damageable=(type.flags2&0x400000U) ? (raw[40]&1U)!=0 : (type.flags1&0x10U)!=0;
            if (damageable && !(types_[pool_.Slot(candidate)[10]].flags2&0x120000U) && size<victims.size()) victims[size++]=candidate;
        });
        auto cursor=parent;
        for (std::size_t index=0;index<size;++index) { //살아남은 후보만 다음 빔의 출발점으로 사용한다.
            const auto candidate=victims[index];Need(hooks_.beam)(cursor,candidate);
            if (!editor_) Need(hooks_.damage)(candidate,damage,pool_.Slot(parent)[34],parent);
            if (!(pool_.Slot(candidate)[11]&7U)) cursor=candidate;
        }
        Need(hooks_.beam)(cursor,other);
    }
}
// 원본은 양의 두 축만 검사하고 첫 양방향 동맹을 찾으면 해당 축을 끝낸다.
void RawArcSpire::Attack(Sid parent,float period) const {
    const auto damage=Truncate(static_cast<double>(period)*Need(hooks_.damageRate)(parent));
    const auto source=Position(parent);const int x=Truncate(source[0]),y=Truncate(source[1]);
    const auto range=Truncate(Need(hooks_.range)(parent));
    const int endX=Truncate(std::min(static_cast<double>(source[0])+range,254.0)),endY=Truncate(std::min(static_cast<double>(source[1])+range,254.0));
    std::array<Sid,20> buffer{};std::uint32_t count{};
    if (SpotUnion({x,y,endX-1,y})&1U) {
        for (int at=x+1;at<endX;++at) { //표면 bit1이 있는 칸만 원본 위치 검색을 실행한다.
            if (at<0 || at>=256 || y<0 || y>=256) throw std::out_of_range("아크 스파이어 가로 지도 범위");
            if ((spots_[static_cast<std::size_t>(y*256+at)]&1U) && Collect(parent,buffer,count,at,y,pool_.Slot(parent)[10])) break;
        }
    }
    Strike(parent,std::span(buffer).first(count),damage);count=0;
    if (SpotUnion({x,y,x,endY-1})&1U) {
        for (int at=y+1;at<endY;++at) { //앞선 가로 효과 후에도 현재 같은 지도/부모를 읽는다.
            if (at<0 || at>=256 || x<0 || x>=256) throw std::out_of_range("아크 스파이어 세로 지도 범위");
            if ((spots_[static_cast<std::size_t>(at*256+x)]&1U) && Collect(parent,buffer,count,x,at,pool_.Slot(parent)[10])) break;
        }
    }
    Strike(parent,std::span(buffer).first(count),damage);
}
// signed count 증가는 DWORD 감김 뒤 비교하며, event0의 count0은 공격하지 않는다.
float RawArcSpire::Handle(Sid parent,std::uint32_t event,std::uint32_t count,float period) const {
    if (!(pool_.Slot(parent)[40]&0x20U)) {
        if (!event && count) Attack(parent,period);
        else if (event==1) {
            ++count;const auto total=static_cast<std::int32_t>(frames_[pool_.Slot(parent)[10]].Codes().size());
            if (std::bit_cast<std::int32_t>(count)>total) return 0;
            const auto frame=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(total)-count);
            Need(hooks_.setFrame)(parent,frame,0);
        }
    }
    return Need(hooks_.damageable)(parent,event,count,period);
}
}
