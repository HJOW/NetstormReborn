// 원본 검색/픽셀 좌표/교차 배리어의 계산을 빈 표적 좌표 콜백으로 대체하지 않는다.
#include "o/RawFenceTarget.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 BoardCoord의 invalid와 현재 hash 최댓값 경계의 float 상수다.
constexpr std::array<float,2> kInvalid{-10000,-10000};
constexpr float kHashEpsilon=0.001f,kCeilBias=0.9999f;
// 비정렬 little endian DWORD/WORD를 지정 폭대로 조립한다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 인접 필드를 섞지 않고 해당 폭의 바이트만 읽는다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[offset+i])<<(8*i);return value;
}
// 실제 유효 좌표는 각 성분이0보다 크고256보다 작다.
bool Valid(std::array<float,2> point) { return point[0]>0 && point[0]<256 && point[1]>0 && point[1]<256; }
// 원본 Rect min/max는 들어오는 점을1..255로 자르고 기존 각 corner의 invalid를 별도로 처리한다.
void Include(GunTargetRect& rect,std::array<float,2> point) {
    point={std::clamp(point[0],1.0f,255.0f),std::clamp(point[1],1.0f,255.0f)};
    if (Valid({rect.left,rect.top})) { rect.left=std::min(rect.left,point[0]);rect.top=std::min(rect.top,point[1]); }
    else { rect.left=point[0];rect.top=point[1]; }
    if (Valid({rect.right,rect.bottom})) { rect.right=std::max(rect.right,point[0]);rect.bottom=std::max(rect.bottom,point[1]); }
    else { rect.right=point[0];rect.bottom=point[1]; }
}
// 원본 Geometry의 포함은 경계 접촉을 허용하되 min/min·max/max 대각 끝점은 제외한다.
bool Interior(GunTargetRect rect,std::array<float,2> point) {
    return rect.left<=point[0] && point[0]<=rect.right && rect.top<=point[1] && point[1]<=rect.bottom &&
        (point[0]!=rect.left || point[1]!=rect.top) && (point[0]!=rect.right || point[1]!=rect.bottom);
}
// 0041ce90의 좌표 차는 먼저float에 저장한다. cardinal이면 Y 동률을 우선한다.
int Direction(std::array<float,2> from,std::array<float,2> to,bool cardinal) {
    const float x=static_cast<float>(static_cast<double>(to[0])-from[0]),y=static_cast<float>(static_cast<double>(to[1])-from[1]);
    if (cardinal) return std::abs(x)<=std::abs(y) ? (y>0 ? 4 : 0) : (x<=0 ? 6 : 2);
    if (x<0) return y<0 ? 7 : y>0 ? 5 : 6;
    if (x>0) return y<0 ? 1 : y>0 ? 3 : 2;
    return y<0 ? 0 : y>0 ? 4 : -1;
}
// 실제 BoardCoord.angle의 float 입력 저장·double fpatan 환산·정규화 뒤 CRT 절삭이다.
int Angle(std::array<float,2> from,std::array<float,2> to) {
    if (from==to) return 0;
    const double radians=std::atan2(-(static_cast<double>(to[1])-from[1]),static_cast<double>(to[0])-from[0]);
    const float degrees=static_cast<float>(450.0-radians*57.295827908797776);
    double value=std::fmod(static_cast<double>(degrees),360.0);
    if (value<0) value+=360;
    if (value>=360) value=359.9999f;
    return static_cast<int>(value);
}
}
// 같은 Pool/TYPE 계약을 확인하며 CD의 다른 fence 몸체를 PATCH 규칙으로 실행하지 않는다.
RawFenceTarget::RawFenceTarget(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const ProjectileGeometryType> geometry,const RawGunTarget& target,
    const GunTargetState& targeting,const GunProcessState& process,std::uint32_t fenceType)
    :pool_(pool),hash_(hash),types_(types),geometry_(geometry),target_(target),targeting_(targeting),process_(process),fenceType_(fenceType) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || &target.Pool()!=&pool || types.size()!=188 || geometry.size()!=types.size())
        throw std::invalid_argument("fencemark 표적의 판본/같은 Pool/TYPE 계약 오류");
}
// 원본 정상 자산 범위를 벗어나는 포인터 산술은 호스트에서 거부한다.
std::span<const std::uint8_t> RawFenceTarget::Slot(Sid sid) const {
    if (sid.value<5 || sid.value>=pool_.Capacity()) throw std::out_of_range("fencemark 표적 SID 오류");
    const auto raw=pool_.Slot(sid);if (raw[10]<70 || raw[10]>=types_.size()) throw std::out_of_range("fencemark 표적 TYPE 오류");return raw;
}
// 현재 슬롯의 binary32 위치를 읽는다.
std::array<float,2> RawFenceTarget::Position(Sid sid) const { const auto raw=Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// 원본 x87 나눗셈/덧셈 후 각 성분을float에 저장한다.
std::array<float,2> RawFenceTarget::Add(std::array<float,2> point,int x,int y) const {
    if (!std::isfinite(targeting_.scaleX) || !std::isfinite(targeting_.scaleY) || targeting_.scaleX<=0 || targeting_.scaleY<=0)
        throw std::invalid_argument("fencemark 현재 픽셀 배율 오류");
    return {static_cast<float>(static_cast<double>(x)/targeting_.scaleX+point[0]),static_cast<float>(static_cast<double>(y)/targeting_.scaleY+point[1])};
}
// 발자국의 실제 마지막 칸까지 포함하는 flags0 사각형이다.
GunTargetRect RawFenceTarget::Bounds(Sid sid) const {
    const auto position=Position(sid);const auto& type=types_[Slot(sid)[10]];
    GunTargetRect rect{position[0],position[1],position[0],position[1]};
    Include(rect,{static_cast<float>(static_cast<double>(position[0])-(type.footX-1)),static_cast<float>(static_cast<double>(position[1])-(type.footY-1))});return rect;
}
// 일반 포구는 현재 SHP 좌표를 더한 뒤 TYPE 핫스폿을 빼며 아처 전용 기본프레임 포구와 구별한다.
std::array<float,2> RawFenceTarget::Muzzle(Sid firing) const {
    const auto raw=Slot(firing);const auto number=raw[10];const auto frame=std::bit_cast<std::int32_t>(Read(raw,36));
    if (frame<0 || static_cast<std::size_t>(frame)>=geometry_[number].shotOffsets.size()) throw std::out_of_range("fencemark 현재 포구 SHP 프레임 오류");
    const auto offset=geometry_[number].shotOffsets[static_cast<std::size_t>(frame)];
    return Add(Add(Position(firing),offset[0],offset[1]),-types_[number].hotspotX,-types_[number].hotspotY);
}
// 실제 기본 발자국의 두 corner에 현재 포구/TYPE 픽셀 보정을 각각 적용한다.
GunTargetRect RawFenceTarget::SightBounds(Sid firing) const {
    const auto raw=Slot(firing);const auto number=raw[10];const auto frame=std::bit_cast<std::int32_t>(Read(raw,36));
    if (frame<0 || static_cast<std::size_t>(frame)>=geometry_[number].shotOffsets.size()) throw std::out_of_range("fencemark 시야 SHP 프레임 오류");
    const auto offset=geometry_[number].shotOffsets[static_cast<std::size_t>(frame)];const auto rect=Bounds(firing);
    const auto low=Add(Add({rect.left,rect.top},offset[0],offset[1]),-types_[number].hotspotX,-types_[number].hotspotY);
    const auto high=Add(Add({rect.right,rect.bottom},offset[0],offset[1]),-types_[number].hotspotX,-types_[number].hotspotY);
    return {low[0],low[1],high[0],high[1]};
}
// 두 비수직 선의 첫 절편과 결과 x/y만float로 저장하는 실제 x87 순서를 유지한다.
std::array<float,2> RawFenceTarget::Intersection(std::array<float,2> a,std::array<float,2> b,std::array<float,2> c,std::array<float,2> d) {
    std::array<float,2> point{};
    if (a[0]==b[0] || c[0]==d[0]) {
        if (a[0]==b[0] && c[0]==d[0]) point=b;
        else {
            const auto vertical=a[0]==b[0] ? a : c,first=a[0]==b[0] ? c : a,last=a[0]==b[0] ? d : b;
            const double slope=(static_cast<double>(last[1])-first[1])/(static_cast<double>(last[0])-first[0]);
            point={vertical[0],static_cast<float>(slope*vertical[0]+(first[1]-first[0]*slope))};
        }
    } else {
        const double first=(static_cast<double>(b[1])-a[1])/(static_cast<double>(b[0])-a[0]);
        const double second=(static_cast<double>(d[1])-c[1])/(static_cast<double>(d[0])-c[0]);
        const float intercept=static_cast<float>(a[1]-a[0]*first);
        point[0]=static_cast<float>(((c[1]-c[0]*second)-intercept)/(first-second));
        point[1]=static_cast<float>(point[0]*first+intercept);
    }
    GunTargetRect first{a[0],a[1],a[0],a[1]},second{c[0],c[1],c[0],c[1]};Include(first,b);Include(second,d);
    return Interior(first,point) && Interior(second,point) ? point : kInvalid;
}
// 실제 fencemark의 WORD 끝점과 분리된 high24/low8 끝점을 원본 순서로 읽는다.
std::array<float,2> RawFenceTarget::Across(Sid mark,Sid firing,Sid target) const {
    const auto raw=Slot(mark);const auto firingRaw=Slot(firing);const Sid firstSid{static_cast<std::uint16_t>(Read(raw,12,2))};
    const auto firstRaw=Slot(firstSid);const auto owner=firstRaw[34],firingOwner=firingRaw[34];
    if (owner>=9 || firingOwner>=9) throw std::out_of_range("fencemark 동맹 Player 번호 오류");
    if (owner==firingOwner || process_.allies[firingOwner][owner]) return kInvalid;
    const auto secondId=(Read(raw,26)<<8)|raw[30];
    if (secondId>65535) throw std::out_of_range("fencemark 압축 끝점 SID 오류");
    const auto originalA=Position(firstSid),originalB=Position(Sid{static_cast<std::uint16_t>(secondId)});
    const auto firingPosition=Position(firing),targetPosition=Position(target);
    auto a=originalA,b=originalB;const auto direction=Direction(a,b,true);const int axis=(direction==2 || direction==6) ? 1 : 0;
    const auto& targetType=types_[Slot(target)[10]];const auto& firingType=types_[firingRaw[10]];
    int width=targetType.footX,height=targetType.footY;const int extent=axis ? height : width;
    const auto coordinate=static_cast<std::size_t>(axis);
    if (firingPosition[coordinate]==a[coordinate]) return kInvalid;
    if (extent==3 && targetPosition[coordinate]==a[coordinate]) return kInvalid;
    // 폭/높이3보다 큰 표적이 배리어 축에 걸친 원본 내부 칸을 검사한다.
    for (int index=0;extent>3 && index<extent-2;++index)
        if (a[coordinate]==static_cast<double>(targetPosition[coordinate])-index) return kInvalid;
    const bool cannon=firingType.group==1;
    if (cannon && (axis ? (a[0]==firingPosition[0] || b[0]==firingPosition[0]) : (a[1]==firingPosition[1] || b[1]==firingPosition[1]))) return kInvalid;
    if (axis) { if (a[0]<=b[0]) { b=Add(b,-32,-11);a=Add(a,0,-11); } else { a=Add(a,-32,-11);b=Add(b,0,-11); } }
    else { if (a[1]<=b[1]) { b=Add(b,-16,-22);a=Add(a,-16,0); } else { a=Add(a,-16,-22);b=Add(b,-16,0); } }
    if (width==1 && height==1) { width=2;height=2; }
    // 004567f0의 배리어 띠/발자국 겹침은 반환 AL의 참/거짓만 사용한다.
    const auto overlaps=[&](std::array<float,2> position,int footX,int footY) {
        const auto edge=Add(originalA,axis ? 0 : -32,axis ? -22 : 0);
        const auto low=Add(position,axis ? 0 : footX*-16+16,axis ? footY*-11+11 : 0);
        return position[coordinate]>=edge[coordinate] && low[coordinate]<=originalA[coordinate];
    };
    const int firingDirection=Direction(originalA,firingPosition,false),targetDirection=Direction(originalA,targetPosition,false);
    const auto side=[axis](int way) { // 원본8방향의 배리어 양쪽 분류이며 축 위는 -1이다.
        if (axis) return way==0 || way==1 || way==7 ? 1 : way==3 || way==4 || way==5 ? 0 : -1;
        return way==1 || way==2 || way==3 ? 0 : way==5 || way==6 || way==7 ? 1 : -1;
    };
    const int firstSide=side(targetDirection),secondSide=side(firingDirection);const bool firingSide=secondSide==1;
    bool ambiguous=firstSide<0 || secondSide<0;const int targetSide=firstSide==1 ? 1 : 0;
    if (overlaps(targetPosition,width,height) || overlaps(firingPosition,firingType.footX,firingType.footY)) {
        auto nearTarget=targetPosition;
        if (!targetSide) nearTarget=Add(nearTarget,axis ? 0 : width*-16+16,axis ? height*-11+11 : 0);
        if (a[coordinate]==nearTarget[coordinate]) ambiguous=true;
        auto nearFiring=firingPosition;
        if (!firingSide) nearFiring=Add(nearFiring,axis ? 0 : firingType.footX*-16+16,axis ? firingType.footY*-11+11 : 0);
        if (a[coordinate]==nearFiring[coordinate]) return kInvalid;
        if (targetSide==static_cast<int>(firingSide) && !ambiguous) return kInvalid;
    }
    const auto targetCenter=target_.Center(target),firingCenter=target_.Center(firing);
    auto point=Intersection(a,b,firingCenter,targetCenter);
    if (!Valid(point)) return point;
    const int angle=Angle(point,targetCenter);
    if (axis) return Add(point,0,firingSide && !cannon && angle<192 ? -11 : -22);
    if (cannon) { point[1]=firingCenter[1];return Add(point,0,-15); }
    if (!firingSide) return Add(point,0,-11);
    return angle<=60 ? Add(point,-16,0) : point;
}
// 실제 발사/표적 범위를 합친 뒤 현재 해시 단계 경계에 도장하고 일반 finder1로 순회한다.
std::array<float,2> RawFenceTarget::Point(Sid firing,Sid target) const {
    auto rect=SightBounds(firing);const auto muzzle=Muzzle(firing);const auto bounds=Bounds(target);
    Include(rect,{bounds.left,bounds.top});Include(rect,{bounds.right,bounds.bottom});
    const int level=hash_.Selected(),scale=SquidHash::kScales[static_cast<std::size_t>(level)],last=SquidHash::Side(level)-1;
    const auto lower=[scale,last](float value) { return scale*std::min(static_cast<int>(value)/scale,last); }; // 00432580의 절삭/정수 나눗셈이다.
    const auto low=std::array<float,2>{static_cast<float>(lower(rect.left)),static_cast<float>(lower(rect.top))};
    const auto high=std::array<float,2>{static_cast<float>(static_cast<double>(lower(rect.right)+scale)-kHashEpsilon),static_cast<float>(static_cast<double>(lower(rect.bottom)+scale)-kHashEpsilon)};
    Include(rect,low);Include(rect,high);
    const auto ceil=[](float value) { return static_cast<int>(static_cast<double>(value)+kCeilBias); }; // 원본 오른쪽/아래 int 사각형 변환이다.
    RawSquidFinder finder(pool_,hash_,types_);auto result=kInvalid;float best=10000;
    // 실제 해시/체인 순서에서 현재 float 최솟값과 거리를 엄격히 비교하고 선택 후float에 저장한다.
    for (auto sid=finder.Begin({static_cast<int>(rect.left),static_cast<int>(rect.top),ceil(rect.right),ceil(rect.bottom)},1);sid.value;sid=finder.Next()) {
        if (pool_.Slot(sid)[10]!=fenceType_) continue;
        const auto point=Across(sid,firing,target);if (!Valid(point)) continue;
        const double x=static_cast<double>(point[0])-muzzle[0],y=static_cast<double>(point[1])-muzzle[1],distance=std::sqrt(x*x+y*y);
        if (distance<best) { best=static_cast<float>(distance);result=point; }
    }
    return result;
}
}
