// 원본의 float 저장·영역 순서·효과 뒤 현재 raw 재조회를 보존한다.
#include "o/RawAcidFence.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kCeilBias=0.9999899864196777F; //41d7a0의 실제502310이다.
constexpr double kHighFlyerOffset=5.3; //50e870은binary64이며float5.3으로먼저좁히지않는다.
// 원본_ftol의 signed64 절삭/하위DWORD와 불확정0을 보존한다.
std::int32_t Truncate(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value))); }
// 중간float저장은명시지점에서만적용한다.
float Stored(double value) { return static_cast<float>(value); }
// 현재 비정렬 float 네 바이트를 읽는다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) { std::uint32_t word{};for (std::size_t index=0;index<4;++index) word|=std::uint32_t(raw[offset+index])<<(index*8);return std::bit_cast<float>(word); } //주변 raw는 포함하지 않는다.
// 선택된 실제 하위 경계가 없으면 빈 성공으로 대신하지 않는다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("애시드 실제 하위 경계 미연결");return function; }
// 같은48c300이 사용하는 find만 연결한다.
ArcSpireHooks NeighborHooks(const AcidFenceHooks& hooks) { ArcSpireHooks result;result.find=hooks.find;return result; }
// 현재 패킷의 little endian WORD 하나다.
std::uint16_t Word(std::span<const std::uint8_t> packet,std::size_t at) { return static_cast<std::uint16_t>(packet[at]|(std::uint16_t(packet[at+1])<<8)); }
}
// 최초 관문은 예약/공통 후처리 전에 한 번만 읽는다.
void PostPopAcidFence(const SidPool& pool,Sid parent,std::uint32_t flags,const std::function<bool()>& reserve,
    const std::function<void(Sid,std::uint32_t,float)>& regular,const std::function<void(Sid,std::uint32_t)>& damageable) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("애시드 후처리 판본 오류");
    if (!(pool.Slot(parent)[40]&9U) && (flags&1U) && Need(reserve)()) Need(regular)(parent,0,.3F);Need(damageable)(parent,flags);
}
// 다른 월드/판본/부분지도를 섞지 않는다.
RawAcidFence::RawAcidFence(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
    std::span<const std::uint8_t> spots,std::span<const std::uint32_t> alliances,const bool& boss,const std::uint32_t& editor,
    const bool& friendlyFire,const std::uint32_t& localOwner,const AcidFenceTypes& special,const RawAcidZap& zap,AcidFenceHooks hooks)
    :pool_(pool),types_(types),frames_(frames),spots_(spots),alliances_(alliances),boss_(boss),editor_(editor),friendlyFire_(friendlyFire),localOwner_(localOwner),special_(special),zap_(zap),hooks_(std::move(hooks)),neighbors_(pool,types,frames,spots,alliances,editor,NeighborHooks(hooks_)) {}
// 원래 부모/이웃 번호의 현재 float 좌표다.
std::array<float,2> RawAcidFence::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {Coordinate(raw,14),Coordinate(raw,18)}; }
// 역방향 동맹은 이 피해 후보 필터에 추가하지 않는다.
bool RawAcidFence::Allied(std::uint32_t first,std::uint32_t second) const { if (first>8 || second>8) throw std::out_of_range("애시드 현재 동맹owner 오류");return alliances_[first*9+second]!=0; }
// 지도 끝도 포함하며 각 축의 원래clamp를 보존한다.
std::uint8_t RawAcidFence::SpotUnion(SquidSearchArea area) const {
    area.left=std::clamp(area.left,1,255);area.right=std::clamp(area.right,1,255);area.top=std::clamp(area.top,1,255);area.bottom=std::clamp(area.bottom,1,255);std::uint8_t result{};
    for (int y=area.top;y<=area.bottom;++y) for (int x=area.left;x<=area.right;++x) result|=spots_[static_cast<std::size_t>(y*256+x)]; //원래y→x포함범위의OR다.
    return result;
}
// 수집 시 free/dead는 걸러내지 않으며 최종 실행 직전에 별도 확인한다.
void RawAcidFence::Scan(Sid parent,Sid other,std::array<float,4> bounds,std::uint32_t genus,const std::uint32_t* exactType,std::span<Sid> victims,std::size_t& count) const {
    const SquidSearchArea area{Truncate(bounds[0]),Truncate(bounds[1]),Truncate(static_cast<double>(bounds[2])+kCeilBias),Truncate(static_cast<double>(bounds[3])+kCeilBias)};
    Need(hooks_.iterate)(area,[&](Sid candidate) {
        const auto raw=pool_.Slot(candidate);if ((raw[40]&1U) || candidate==parent || candidate==other || (Allied(pool_.Slot(parent)[34],raw[34]) && !friendlyFire_)) return;
        if (raw[10]>=types_.size()) throw std::out_of_range("애시드 후보TYPE 오류");const auto flags=types_[raw[10]].flags2;
        if (!(flags&genus) || (flags&0x200U) || !boss_ || editor_ || (exactType && raw[10]!=*exactType) || count>=victims.size()) return;victims[count++]=candidate;
    });
}
// 이웃마다 독립40개버퍼이며13영역의 중복과 순서를 그대로 유지한다.
void RawAcidFence::Strike(Sid parent,std::span<const Sid> neighbors,std::int32_t xOriented) const {
    for (const auto other:neighbors) { //이웃의 실제애니메이션예약을 모든영역검색보다먼저수행한다.
        if (Need(hooks_.reserveRegular)()) Need(hooks_.regular)(other,1,.2F);std::array<Sid,40> victims{};std::size_t count{};
        auto source=Position(parent),target=Position(other);auto ly=Stored(static_cast<double>(source[1])-1),hy=Stored(static_cast<double>(target[1])-1);
        if (xOriented) { ly=Stored(static_cast<double>(ly)+.5);hy=Stored(static_cast<double>(hy)-.5); }
        Scan(parent,other,{Stored(static_cast<double>(source[0])-1),ly,Stored(static_cast<double>(target[0])-1),hy},0x50100,nullptr,victims,count);
        for (std::size_t region=0;region<6;++region) { //144/73/138각각의부모와이웃발자국영역이다.
            const auto point=Position(region%2 ? other : parent);const auto& record=types_[pool_.Slot(parent)[10]];auto lowX=Stored(static_cast<double>(point[0])-Stored(record.footX));auto lowY=Stored(static_cast<double>(point[1])-Stored(record.footY));auto highY=point[1];
            const auto* type=region<2 ? &special_.rainFlyer : region<4 ? &special_.sunFlyer : &special_.windFlyer;
            if (region<2) lowX=Stored(static_cast<double>(lowX)+1);
            else if (region<4) { lowX=Stored(static_cast<double>(lowX)+2);lowY=Stored(static_cast<double>(lowY)-4);if (xOriented) highY=Stored(static_cast<double>(highY)-2); }
            Scan(parent,other,{lowX,lowY,point[0],highY},0x100000,type,victims,count);
        }
        for (std::size_t region=0;region<6;++region) { //비행체세종과운반체세종의서로다른높이연결영역이다.
            source=Position(parent);target=Position(other);float lowX{},lowY{},highX{},highY{};const std::uint32_t* type{};const std::uint32_t genus=region<3 ? 0x100000U : 0x20000U; //타입전역의주소를보관하여finder뒤현재값을읽는다.
            if (region==0) {
                lowX=Stored(static_cast<double>(source[0])-2);lowY=Stored(static_cast<double>(source[1])-3);if (xOriented) lowY=Stored(static_cast<double>(lowY)+1);else lowX=Stored(static_cast<double>(lowX)+.5);
                highX=xOriented ? target[0] : Stored(static_cast<double>(target[0])-.5);highY=Stored(static_cast<double>(target[1])-1);if (xOriented) highY=Stored(static_cast<double>(highY)-1.5);type=&special_.rainFlyer;
            } else if (region==1) {
                lowX=Stored(static_cast<double>(source[0])-2.5);lowY=Stored(static_cast<double>(source[1])-1);highX=Stored(static_cast<double>(target[0])-1);highY=Stored(static_cast<double>(target[1])-1);
                if (xOriented) { lowX=Stored(static_cast<double>(lowX)+2.5);lowY=Stored((static_cast<double>(lowY)-3)+.5);highX=Stored(static_cast<double>(highX)-1);highY=Stored(static_cast<double>(highY)-1.5); }type=&special_.windFlyer;
            } else if (region==2) {
                lowX=Stored(static_cast<double>(source[0])-1);lowY=Stored(static_cast<double>(source[1])-1);highX=Stored(static_cast<double>(target[0])+1);highY=Stored(static_cast<double>(target[1])-1);
                if (xOriented) { lowX=Stored(static_cast<double>(lowX)+1);lowY=Stored((static_cast<double>(lowY)-7)+1.5);highX=Stored(static_cast<double>(highX)-3);highY=Stored((static_cast<double>(highY)-kHighFlyerOffset)-.5); }type=&special_.sunFlyer;
            } else if (region==3) {
                lowY=Stored(static_cast<double>(source[1])-1);if (xOriented) lowY=Stored((static_cast<double>(lowY)-2)+.5);lowX=Stored((static_cast<double>(source[0])-2.5)+(xOriented ? 2.5 : 1));
                highX=Stored(static_cast<double>(target[0])-1);if (xOriented) highX=Stored(static_cast<double>(highX)-1);highY=Stored((static_cast<double>(target[1])-1)-(xOriented ? 1 : 5));type=&special_.rainBalloon;
            } else if (region==4) {
                lowY=Stored(static_cast<double>(source[1])-1);if (xOriented) lowY=Stored(static_cast<double>(lowY)+.5);lowX=Stored((static_cast<double>(source[0])-2.5)+(xOriented ? 2.5 : 2));
                highX=Stored(static_cast<double>(target[0])-1);if (xOriented) highX=Stored(static_cast<double>(highX)-1);highY=Stored((static_cast<double>(target[1])-1)-(xOriented ? 1 : 6));type=&special_.windBalloon;
            } else {
                lowX=Stored(static_cast<double>(source[0])-2.5);lowY=Stored(static_cast<double>(source[1])-1);highX=Stored(static_cast<double>(target[0])-1);highY=Stored(static_cast<double>(target[1])-1);
                if (xOriented) { lowX=Stored(static_cast<double>(lowX)+2.5);lowY=Stored((static_cast<double>(lowY)-2)+.5);highX=Stored(static_cast<double>(highX)-1); }else highY=Stored(static_cast<double>(highY)-5);type=&special_.sunBalloon;
            }
            Scan(parent,other,{lowX,lowY,highX,highY},genus,type,victims,count);
        }
        for (std::size_t index=0;index<count;++index) { //송신/피해후앞선중복대상이죽으면다음실행을건너뛴다.
            const auto victim=victims[index];if (!victim.value || victim.value>=pool_.Capacity() || (pool_.Slot(victim)[11]&3U)) continue;const auto raw=pool_.Slot(victim);
            if (types_[raw[10]].flags2&0x200000U) { Need(hooks_.damage)(victim,1000,pool_.Slot(parent)[34],parent);continue; }
            auto packet=zap_.Build(victim,parent,other,std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(xOriented)));Need(hooks_.broadcast)(packet);
            if (Word(packet,4)!=victim.value) Need(hooks_.assertion)(0x2fa);if (Word(packet,6)!=parent.value) Need(hooks_.assertion)(0x2fb);if (Word(packet,8)!=other.value) Need(hooks_.assertion)(0x2fc);if (std::bit_cast<std::int8_t>(packet[10])!=xOriented) Need(hooks_.assertion)(0x2fd);
            zap_.Receive(localOwner_,0,packet);
        }
    }
}
// 양의두축에서첫동맹까지만연결하고표면비트가없는곳은find를호출하지않는다.
void RawAcidFence::Attack(Sid parent) const {
    const auto source=Position(parent);const int x=Truncate(source[0]),y=Truncate(source[1]),range=Truncate(Need(hooks_.range)(parent));const int endX=Truncate(std::min(static_cast<double>(source[0])+range,254.0)),endY=Truncate(std::min(static_cast<double>(source[1])+range,254.0));
    std::array<Sid,20> buffer{};std::uint32_t count{};bool connected{};
    if (SpotUnion({x,y,endX-1,y})&1U) for (int at=x+1;at<endX;++at) { if (at<0 || at>=256 || y<0 || y>=256) throw std::out_of_range("애시드가로지도범위");if ((spots_[static_cast<std::size_t>(y*256+at)]&1U) && neighbors_.Collect(parent,buffer,count,at,y,pool_.Slot(parent)[10])) break; } //가로첫이웃이다.
    connected=count!=0;Strike(parent,std::span(buffer).first(count),1);count=0;
    if (SpotUnion({x,y,x,endY-1})&1U) for (int at=y+1;at<endY;++at) { if (at<0 || at>=256 || x<0 || x>=256) throw std::out_of_range("애시드세로지도범위");if ((spots_[static_cast<std::size_t>(at*256+x)]&1U) && neighbors_.Collect(parent,buffer,count,x,at,pool_.Slot(parent)[10])) break; } //앞선효과뒤같은지도/현재타입을읽는다.
    connected=connected || count!=0;Strike(parent,std::span(buffer).first(count),0);if (connected && Need(hooks_.reserveRegular)()) Need(hooks_.regular)(parent,1,.2F);
}
// event1의마지막프레임은numFrames-1을기준으로하며감김뒤signed비교다.
float RawAcidFence::Handle(Sid parent,std::uint32_t event,std::uint32_t count,float period) const {
    if (!(pool_.Slot(parent)[40]&0x20U)) {
        if (!event) Attack(parent);
        else if (event==1) { ++count;const auto last=static_cast<std::int32_t>(frames_[pool_.Slot(parent)[10]].Codes().size())-1;if (last<std::bit_cast<std::int32_t>(count)) return 0;Need(hooks_.setFrame)(parent,std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(last)-count),0); }
    }
    return period;
}
}
