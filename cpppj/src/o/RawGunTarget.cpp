// 실제 Gun 후보 검색 순서·서버 SID 필터·판본별 사거리/사정선/각도 산술이다.
#include "o/RawGunTarget.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 실제 세션은 같은 Player+54를 다시 읽고, 독립 native 입력은 기존 배열을 사용한다.
std::uint32_t GunTargetState::AllyMask(std::uint32_t owner) const {
    if (owner>=allyMasks.size()) throw std::out_of_range("Gun 현재 동맹 소유자 범위 오류");
    return allyMaskRead ? allyMaskRead(owner) : allyMasks[owner];
}
// 진단 출력 분기가 도달한 시점의 현재 참여 플래그를 읽는다.
std::uint32_t GunTargetState::PlayerFlags(std::uint32_t owner) const {
    if (owner>=playerFlags.size()) throw std::out_of_range("Gun 현재 참여 소유자 범위 오류");
    return playerFlagsRead ? playerFlagsRead(owner) : playerFlags[owner];
}
namespace {
// 짝수 방향의 ray 이동 표이며 원본 0052f83c/0052f85c와 CD 0051c330/0051c350이다.
constexpr std::array<int,8> kDx{0,1,1,1,0,-1,-1,-1},kDy{-1,-1,0,1,1,1,0,-1};
// 원본 float 경계 확장·ground 후보 점수·직접 읽는 HP/프레임 플래그다.
constexpr float kEdgeLow=0.0001f,kEdgeHigh=0.1f,kGroundPenalty=1000.0f;
// 원본 ceil helper는0.9999f를 x87에서 더한 뒤 CRT 절삭하며 중간 float 저장을 하지 않는다.
constexpr float kCeilBias=0.9999f;
constexpr std::uint32_t kHitPoints=0x10,kFlyer=0x100000,kPriest=0x200000,kDais=0x400000,kDoubleFrames=0x440000;
// PE의 썬 캐논 방향별24프레임 표에서 bit8000인 프레임 번호다. 24~27은 별도 방향을 검사한다.
constexpr std::array<int,4> kSunAllowedFrame{1,3,0,2};
// unaligned raw little endian 필드의 폭을 보존한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    std::uint32_t value{};
    // 지정한 폭의 낮은 바이트부터 읽는다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(bytes[offset+i])<<(8*i);
    return value;
}
// 현재 raw 좌표의 float32 비트다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) { return std::bit_cast<float>(Read(raw,offset)); }
// 원본 4방향 BoardCoord 비교는 Y의 절댓값이 같으면 세로 방향을 우선한다.
int Direction(float x,float y,float targetX,float targetY) {
    const float dx=static_cast<float>(static_cast<double>(targetX)-x),dy=static_cast<float>(static_cast<double>(targetY)-y);
    if (std::abs(dx)<=std::abs(dy)) return dy>0 ? 4 : 0;
    return dx<=0 ? 6 : 2;
}
// 원본 외곽 사각형의 OR 교차는 완전 포함도 허용하며 모든 경계가 포함된다.
bool Intersects(GunTargetRect a,GunTargetRect b) {
    return (a.left<=b.left || a.left<=b.right) && (b.left<=a.right || b.right<=a.right) &&
        (a.top<=b.top || a.top<=b.bottom) && (b.top<=a.bottom || b.bottom<=a.bottom);
}
// 보드의 clamp/invalid 점 처리에서 사용되는 원본 유효 범위다.
bool ValidPoint(float x,float y) { return x>0 && y>0 && x<256 && y<256; }
// Clamp은 ray의 별도 사각형에만 적용하며 by-value 문맥 좌표는 변경하지 않는다.
void Clamp(float& x,float& y) { x=std::clamp(x,1.0f,255.0f);y=std::clamp(y,1.0f,255.0f); }
// CRT __ftol과 같은 정상 유한 값의 0방향 절삭이다.
int Truncate(float value) {
    if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("Gun 좌표 절삭 범위 오류");
    return static_cast<int>(value);
}
// 원본 BoardCoord.ceil의 정상 보드 좌표 정수 변환이다.
int Ceiling(float value) {
    const double biased=static_cast<double>(value)+kCeilBias;
    if (!std::isfinite(biased) || biased<-2147483648.0 || biased>=2147483648.0) throw std::out_of_range("Gun ceil 범위 오류");
    return static_cast<int>(biased);
}
// 00468e70/CD 인라인 angle 정규화는360을 그대로 허용하며 음수에서 1-trunc(각/360)을 더한다.
float Normalize(float angle) {
    if (!std::isfinite(angle)) throw std::invalid_argument("Gun 비유한 각도");
    if (angle<0) angle=static_cast<float>(static_cast<double>(1-Truncate(angle/360.0f))*360.0+angle);
    if (angle>360) angle=static_cast<float>(std::fmod(static_cast<double>(angle),360.0));
    return angle;
}
// 원본의 최소 circular 구간은 끝점을 포함하며180도보다 긴 구간은 바깥쪽을 선택한다.
bool ContainsAngle(float value,float first,float last) {
    if (first>last) std::swap(first,last);
    return last-first<=180.0f ? first<=value && value<=last : value<=first || last<=value;
}
}
// 같은 판본의 전체 타입/SHP 번호 체계를 검사하며 표와 상태를 복사하지 않는다.
RawGunTarget::RawGunTarget(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const GunTargetType> targetTypes,std::span<const SquidDisplayShape> shapes,GunTargetState& state,GunTargetHooks hooks)
    :pool_(pool),hash_(hash),types_(types),targetTypes_(targetTypes),shapes_(shapes),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || targetTypes.size()!=types.size() || shapes.size()!=types.size())
        throw std::invalid_argument("Gun 표적 타입/SHP 판본 불일치");
}
// normal 원본 문맥만 허용한다. 잘못된 타입·소유자·비유한 메모리 접근은 호스트 계약 밖이다.
void RawGunTarget::Validate(const GunTargetQuery& query) const {
    if (query.type>=types_.size() || query.owner>kPlayerCount || !std::isfinite(query.x) || !std::isfinite(query.y) ||
        !std::isfinite(query.centerX) || !std::isfinite(query.centerY)) throw std::out_of_range("Gun 표적 문맥 범위 오류");
}
// 서로 다른 두 special 타입은 저장된 방향 표보다 먼저 처리한다.
std::int32_t RawGunTarget::DirectionFrame(std::uint32_t type,std::int32_t direction) const {
    const int index=direction/2;
    if (index<0 || index>3) throw std::out_of_range("Gun 방향 프레임 범위 오류");
    if (type==state_.sunCannonType) return kSunAllowedFrame[static_cast<std::size_t>(index)];
    if (type==state_.sunFenceType) return index;
    const auto frame=targetTypes_[type].directionFrames[static_cast<std::size_t>(index)];return frame==-1 ? 0 : frame;
}
// 홀수 방향은 자연스럽게 거부하고 짝수 방향에서는 group1 원본 assert를 유지한다.
bool RawGunTarget::CannonDirection(const GunTargetQuery& query,std::int32_t direction) const {
    if (direction%2) return false;
    if (types_[query.type].group!=1) throw std::logic_error("Gun Cannon group 원본 assert");
    if (direction<0 || direction>7) throw std::out_of_range("Gun Cannon 방향 범위 오류");
    const auto theme=targetTypes_[query.type].theme;
    return (theme!=1 && theme!=2) || direction==query.direction;
}
// base Footprint가 사용하는 확장 높이와 by-value raw 좌표를 읽는다.
GunTargetRect RawGunTarget::Footprint(Sid target) const {
    if (!target.value || target.value>=pool_.Capacity()) throw std::out_of_range("Gun 표적 SID 오류");
    const auto raw=pool_.Slot(target);const auto number=raw[10];if (number>=types_.size()) throw std::out_of_range("Gun 표적 타입 오류");
    const auto& type=types_[number];const float x=Coordinate(raw,14),y=Coordinate(raw,18);
    float lowX=static_cast<float>(static_cast<double>(x)-(type.footX-1)),lowY=static_cast<float>(static_cast<double>(y)-(type.footY+targetTypes_[number].heightExtension-1));
    // 실제 min/max helper는 현재 원점이 아니라 들어오는 corner를 보드에 clamp한다.
    Clamp(lowX,lowY);float highX=x,highY=y,left=x,top=y;
    if (ValidPoint(left,top)) { left=std::min(left,lowX);top=std::min(top,lowY); } else { left=lowX;top=lowY; }
    if (ValidPoint(highX,highY)) { highX=std::max(highX,lowX);highY=std::max(highY,lowY); } else { highX=lowX;highY=lowY; }
    return {left,top,highX,highY};
}
// 실제 Center는 확장 높이를 쓰지 않고 footX/Y의 절반을 raw 좌표에서 뺀다.
std::array<float,2> RawGunTarget::Center(Sid target) const {
    const auto raw=pool_.Slot(target);const auto& type=types_[raw[10]];
    return {static_cast<float>(static_cast<double>(Coordinate(raw,14))-type.footX*0.5),static_cast<float>(static_cast<double>(Coordinate(raw,18))-type.footY*0.5)};
}
// 실제 Cannon ray의 별도 rectangle과4DWORD 반환을 보존한다.
GunTargetRect RawGunTarget::CannonRay(const GunTargetQuery& query,std::int32_t direction) const {
    Validate(query);if (!CannonDirection(query,direction)) return {-10000,-10000,-10000,-10000};
    if (!std::isfinite(state_.scaleX) || !std::isfinite(state_.scaleY) || state_.scaleX==0 || state_.scaleY==0)
        throw std::invalid_argument("Gun Cannon 픽셀 배율 오류");
    const auto frame=DirectionFrame(query.type,direction);const auto& shape=shapes_[query.type];
    if (!shape.loaded || frame<0 || static_cast<std::size_t>(frame)>=shape.frames.size()) throw std::out_of_range("Gun Cannon SHP 물리 프레임 오류");
    if (pool_.Edition()==OriginalEdition::Patch1078 && (frame>=shape.frameCount*((types_[query.type].flags1&kDoubleFrames) ? 2 : 1) ||
        types_[query.type].maxHitPoints==-0x22222223 || types_[query.type].maxHitPoints==-0x32323233))
        throw std::out_of_range("Gun Cannon frameCheck 원본 assert");
    const auto& header=shape.frames[static_cast<std::size_t>(frame)];const auto& type=types_[query.type];
    float x=static_cast<float>(static_cast<double>(header.hotspotX)/state_.scaleX+query.x),y=static_cast<float>(static_cast<double>(header.hotspotY)/state_.scaleY+query.y);
    x=static_cast<float>(static_cast<double>(-type.hotspotX)/state_.scaleX+x);y=static_cast<float>(static_cast<double>(-type.hotspotY)/state_.scaleY+y);
    const float endX=static_cast<float>(static_cast<double>(kDx[static_cast<std::size_t>(direction)])*targetTypes_[query.type].range+x);
    const float endY=static_cast<float>(static_cast<double>(kDy[static_cast<std::size_t>(direction)])*targetTypes_[query.type].range+y);
    GunTargetRect rect{x,y,x,y};
    float minimumX=endX,minimumY=endY;
    // 두 판본의 min/max helper 모두 별도 incoming endpoint를 clamp한다.
    Clamp(minimumX,minimumY);
    if (ValidPoint(rect.left,rect.top)) { rect.left=std::min(rect.left,minimumX);rect.top=std::min(rect.top,minimumY); } else { rect.left=minimumX;rect.top=minimumY; }
    if (ValidPoint(rect.right,rect.bottom)) { rect.right=std::max(rect.right,minimumX);rect.bottom=std::max(rect.bottom,minimumY); } else { rect.right=minimumX;rect.bottom=minimumY; }
    // 원본 width<2 또는 height<2인 얇은 ray는 floor/ceil 뒤 경계를 조금 확장한다.
    if (static_cast<double>(rect.right)-rect.left<2.0 || static_cast<double>(rect.bottom)-rect.top<2.0) {
        rect.left=static_cast<float>(static_cast<double>(Truncate(rect.left))+kEdgeLow);rect.top=static_cast<float>(static_cast<double>(Truncate(rect.top))+kEdgeLow);
        rect.right=static_cast<float>(static_cast<double>(Ceiling(rect.right))+kEdgeHigh);rect.bottom=static_cast<float>(static_cast<double>(Ceiling(rect.bottom))+kEdgeHigh);
        Clamp(rect.left,rect.top);Clamp(rect.right,rect.bottom);
    }
    if (state_.showRanges && !(state_.PlayerFlags(query.owner)&0x20)) {
        if (!hooks_.drawRange) throw std::logic_error("Gun Cannon 진단 출력 경계 누락");hooks_.drawRange(rect,false);
    }
    return rect;
}
// 발자국 사각형까지 최소 거리의 판본별 float 저장 위치를 유지한다.
bool RawGunTarget::ArcherShot(const GunTargetQuery& query,Sid target) const {
    const auto rect=Footprint(target);double dx=query.centerX<rect.left ? static_cast<double>(rect.left)-query.centerX : rect.right<query.centerX ? static_cast<double>(query.centerX)-rect.right : 0;
    double dy=query.centerY<rect.top ? static_cast<double>(rect.top)-query.centerY : rect.bottom<query.centerY ? static_cast<double>(query.centerY)-rect.bottom : 0;
    if (pool_.Edition()==OriginalEdition::Cd1072) { dx=static_cast<float>(dx);dy=static_cast<float>(dy); }
    double distance=std::sqrt(dx*dx+dy*dy);if (pool_.Edition()==OriginalEdition::Patch1078) distance=static_cast<float>(distance);
    if (!(distance<targetTypes_[query.type].range)) return false;
    if (query.type!=state_.windArcherType) return true;
    const auto center=Center(target);float angle=0;
    if (query.centerX!=center[0] || query.centerY!=center[1]) {
        // x87 fpatan 뒤 degree 환산은 원본 double 상수57.295827908797776를 사용한다.
        const double radians=std::atan2(-(static_cast<double>(center[1])-query.centerY),static_cast<double>(center[0])-query.centerX);
        float raw=static_cast<float>(450.0-radians*57.295827908797776);double normalized=std::fmod(static_cast<double>(raw),360.0);
        if (normalized<0) normalized+=360;
        if (pool_.Edition()==OriginalEdition::Patch1078) { if (normalized>=360) normalized=359.9999f; }
        else { float rounded=static_cast<float>(normalized);if (rounded>=360) rounded=359.9999f;normalized=rounded; }
        angle=Normalize(static_cast<float>(normalized));
    }
    const float base=Normalize(static_cast<float>(static_cast<double>(query.direction)*45.0));
    return ContainsAngle(angle,Normalize(static_cast<float>(static_cast<double>(base)-state_.windHalfAngle)),Normalize(static_cast<float>(static_cast<double>(base)+state_.windHalfAngle)));
}
// 원본은 현재 표적 중심의4방향을 구해 그 방향 ray와 확장 발자국을 비교한다.
bool RawGunTarget::CannonShot(const GunTargetQuery& query,Sid target) const {
    const auto center=Center(target);const auto ray=CannonRay(query,Direction(query.centerX,query.centerY,center[0],center[1]));
    return Intersects(Footprint(target),ray);
}
// 미지원 group은 release에서 flyer와 같은 직접 허용을 유지한다.
bool RawGunTarget::MayShoot(const GunTargetQuery& query,Sid target) const {
    Validate(query);const auto& type=types_[query.type];
    if (type.group==0) return ArcherShot(query,target);
    if (type.group==1) return CannonShot(query,target);
    if (type.group!=3 && pool_.Edition()==OriginalEdition::Patch1078 && state_.checking && !(type.flags2&kFlyer))
        throw std::logic_error("Gun MayShoot flyer 원본 assert");
    return true;
}
// 실제 Damageable·서버 번호·소유자·사제 수량만 검사하며 dead/free state 필터를 추가하지 않는다.
bool RawGunTarget::MayTarget(const GunTargetQuery& query,Sid target) const {
    Validate(query);if (!target.value || target.value>=pool_.Capacity()) throw std::out_of_range("Gun 후보 SID 오류");
    const auto raw=pool_.Slot(target);const auto number=raw[10];if (number>=types_.size()) throw std::out_of_range("Gun 후보 타입 오류");
    const auto& type=types_[number];const auto extra=raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40U : 35U];
    if ((type.flags2&kDais) ? !(extra&1) : !(type.flags1&kHitPoints)) return false;
    const auto reservedLast=pool_.Edition()==OriginalEdition::Patch1078 ? 14999U : 5999U;
    if (target.value>=5 && target.value<=reservedLast) return false;
    const auto owner=raw[pool_.Edition()==OriginalEdition::Patch1078 ? 34U : 32U];
    if (owner>kPlayerCount) throw std::out_of_range("Gun 후보 소유자 오류");
    if (!state_.friendlyFire && (state_.AllyMask(query.owner)&(1U<<owner))) return false;
    if (extra&0x11) return false;
    if (type.flags2&kPriest) {
        const auto quantity=pool_.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(raw,26)) : static_cast<std::int32_t>(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Read(raw,26,2))));
        if (quantity<=1) return false;
    }
    if (Coordinate(raw,14)==query.x && Coordinate(raw,18)==query.y) return false;
    return MayShoot(query,target);
}
// 밴더 타워는 flyer/balloon 외에는 MayTarget 이전에 거부한다.
bool RawGunTarget::ArcherTarget(const GunTargetQuery& query,Sid target) const {
    if (query.type==state_.thunderArcherType && !(types_[pool_.Slot(target)[10]].flags2&0x120000)) return false;
    return MayTarget(query,target) && ArcherShot(query,target);
}
// source→rect 중심의 반대 방향 edge midpoint를 원본 부동소수 저장 순서대로 반환한다.
std::array<float,2> RawGunTarget::HitPoint(const GunTargetQuery& query,GunTargetRect rect) const {
    const float midX=static_cast<float>((static_cast<double>(rect.left)+rect.right)*0.5),midY=static_cast<float>((static_cast<double>(rect.top)+rect.bottom)*0.5);
    const auto direction=(Direction(query.centerX,query.centerY,midX,midY)+4)&7;
    if (direction==0 || direction==4) return {static_cast<float>((static_cast<double>(rect.right)-rect.left)*0.5+rect.left),direction==0 ? rect.top : rect.bottom};
    return {direction==2 ? rect.right : rect.left,static_cast<float>((static_cast<double>(rect.bottom)-rect.top)*0.5+rect.top)};
}
// 실제 Archer finder는 center±trunc(range) 범위와 flags5를 쓰며 ground에1000을 더한다.
Sid RawGunTarget::FindArcher(const GunTargetQuery& query) const {
    const int range=Truncate(targetTypes_[query.type].range),x=Ceiling(query.centerX),y=Ceiling(query.centerY);
    std::vector<int> heights;heights.reserve(targetTypes_.size());
    // 일반 finder의 실제 타입 확장 높이 입력을 번호순으로 전달한다.
    for (const auto& type:targetTypes_) heights.push_back(type.heightExtension);
    RawSquidFinder finder(pool_,hash_,types_,heights);float best=1.0e13f;Sid result;
    const auto first=finder.Begin({x-range,y-range,x+range,y+range},5); // 원본은 첫 후보와 next를 준비한 뒤 자기 동맹을 읽는다.
    if (!state_.friendlyFire && !(state_.AllyMask(query.owner)&(1U<<query.owner))) throw std::logic_error("Gun 자기 동맹 원본 assert");
    // next는 현재 후보가 반환될 때 이미 저장된다. 후보의 현재 타입/좌표는 매번 다시 조회한다.
    for (auto sid=first;sid.value;sid=finder.Next()) {
        if (!(types_[pool_.Slot(sid)[10]].flags1&kHitPoints) || !ArcherTarget(query,sid)) continue;
        const auto point=HitPoint(query,Footprint(sid));const double dx=static_cast<double>(query.centerX)-point[0],dy=static_cast<double>(query.centerY)-point[1];
        const float distance=static_cast<float>(dx*dx+dy*dy);const double score=static_cast<double>(distance)+((types_[pool_.Slot(sid)[10]].flags2&kFlyer) ? 0 : kGroundPenalty);
        if (score<best) { best=static_cast<float>(score);result=sid; }
    }
    return result;
}
// Cannon은0/2/4/6 ray 범위마다 finder를 새로 시작한다. 같거나 먼 점수는 첫 표적을 유지한다.
Sid RawGunTarget::FindCannon(const GunTargetQuery& query) const {
    const auto range=targetTypes_[query.type].range;float best=static_cast<float>(static_cast<double>(range)*range*2.0);Sid result;
    std::vector<int> heights;heights.reserve(targetTypes_.size());
    // ray finder도 flags5의 현재 확장 높이를 사용한다.
    for (const auto& type:targetTypes_) heights.push_back(type.heightExtension);
    // 원본 네 짝수 방향의 순서를 유지한다.
    for (int direction=0;direction<=6;direction+=2) {
        const auto ray=CannonRay(query,direction);RawSquidFinder finder(pool_,hash_,types_,heights);
        // minimum은 floor, maximum은0.9999f를 더한 ceil을 사용한다.
        for (auto sid=finder.Begin({Truncate(ray.left),Truncate(ray.top),Ceiling(ray.right),Ceiling(ray.bottom)},5);sid.value;sid=finder.Next()) {
            if (!MayTarget(query,sid)) continue;
            const auto point=HitPoint(query,Footprint(sid));const double dx=static_cast<double>(query.centerX)-point[0],dy=static_cast<double>(query.centerY)-point[1];
            const double score=pool_.Edition()==OriginalEdition::Patch1078 ? static_cast<float>(dx*dx+dy*dy) : dx*dx+dy*dy;
            if (score<best) { best=static_cast<float>(score);result=sid; }
        }
    }
    return result;
}
// 외부 검색은 현재 타입 발자국 중심/공격 모드 프레임으로 문맥을 구성한다.
Sid RawGunTarget::FindTarget(float x,float y,std::uint32_t type,std::int32_t direction,std::uint32_t owner) const {
    if (type>=types_.size()) throw std::out_of_range("Gun 검색 타입 오류");
    GunTargetQuery query{x,y,static_cast<float>(static_cast<double>(x)-types_[type].footX*0.5),static_cast<float>(static_cast<double>(y)-types_[type].footY*0.5),type,direction,owner,DirectionFrame(type,direction)};
    Validate(query);const auto group=types_[type].group;
    if (group!=0 && group!=1) throw std::logic_error("Gun FindTarget group 원본 assert");
    const auto result=group==0 ? FindArcher(query) : FindCannon(query);
    if (result.value && state_.showRanges) { if (!hooks_.drawRange) throw std::logic_error("Gun 최종 표적 진단 출력 경계 누락");hooks_.drawRange(Footprint(result),true); }
    return result;
}
// 썬 캐논의 현재 그림 프레임이 그 방향으로 발사할 수 있는지 먼저 검사한다.
bool RawGunTarget::MayFire(const GunTargetQuery& query,Sid target) const {
    Validate(query);
    if (query.type==state_.sunCannonType) {
        const auto center=Center(target);const auto direction=Direction(query.centerX,query.centerY,center[0],center[1]);const auto frame=query.frame;
        if (frame<0 || frame>27) throw std::out_of_range("Gun 썬 캐논 발사 프레임 범위 오류");
        const bool allowed=frame<24 ? frame==kSunAllowedFrame[static_cast<std::size_t>(direction/2)] : direction==std::array<int,4>{4,0,6,2}[static_cast<std::size_t>(frame-24)];
        if (!allowed) return false;
    }
    return MayTarget(query,target);
}
// 같은 raw 풀을 검증하는 연결용 getter다.
const SidPool& RawGunTarget::Pool() const { return pool_; }
// 독립 실제 finder/판정만 교체하고 나머지 프로세스·애니메이션·전송/효과를 유지한다.
GunProcessHooks MakeGunTargetHooks(const SidPool& pool,const RawGunTarget& target,GunProcessHooks hooks) {
    if (&pool!=&target.Pool()) throw std::invalid_argument("GunTarget/GunProcess 풀 불일치");
    hooks.findTarget=[&target](float x,float y,std::uint32_t type,std::uint32_t mode,std::uint8_t owner) { return target.FindTarget(x,y,type,std::bit_cast<std::int32_t>(mode),owner); };
    hooks.shoot=[&target](const std::array<std::uint32_t,8>& words,Sid sid) {
        return target.MayFire({std::bit_cast<float>(words[0]),std::bit_cast<float>(words[1]),std::bit_cast<float>(words[2]),std::bit_cast<float>(words[3]),words[4],std::bit_cast<std::int32_t>(words[5]),words[6],std::bit_cast<std::int32_t>(words[7])},sid);
    };
    return hooks;
}
}
