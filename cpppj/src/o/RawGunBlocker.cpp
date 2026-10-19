// 원본00469590의 후보 순서·엄격한 점수 비교·목록 용량을 보존한다.
#include "o/RawGunBlocker.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kCeilBias=0.9999F,kSnapBias=0.99999F; // 사각형 ceil과422230 중심 snap의 서로 다른 실제 상수다.
constexpr std::array<int,4> kDx{0,1,0,-1},kDy{-1,0,1,0}; // 원본0/2/4/6 순서의 현재 기본 방향표다.
// 비정렬 좌표 DWORD를 현재 슬롯에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t result{};
    // 원본 little endian 저장 순서만 조합한다.
    for (std::size_t index=0;index<4;++index) result|=std::uint32_t(raw[offset+index])<<(index*8);
    return result;
}
// 정상 보드 좌표와 최대 사거리의 CRT 절삭을 수행한다.
int Truncate(double value) { if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("장애물 검색 정수 좌표 범위 오류");return static_cast<int>(value); }
// 원본 BoardCoord의 유효 범위다.
bool Valid(std::array<float,2> point) { return point[0]>0 && point[0]<256 && point[1]>0 && point[1]<256; }
// min/max helper는 새 점을 먼저 보드에 clamp하고 무효인 기존 점을 교체한다.
void Include(GunTargetRect& rect,std::array<float,2> point) {
    point[0]=std::clamp(point[0],1.0F,255.0F);point[1]=std::clamp(point[1],1.0F,255.0F);
    if (Valid({rect.left,rect.top})) { rect.left=std::min(rect.left,point[0]);rect.top=std::min(rect.top,point[1]); } else { rect.left=point[0];rect.top=point[1]; }
    if (Valid({rect.right,rect.bottom})) { rect.right=std::max(rect.right,point[0]);rect.bottom=std::max(rect.bottom,point[1]); } else { rect.right=point[0];rect.bottom=point[1]; }
}
// 현재 float 사각형의 중심을 한 번float에 저장한다.
std::array<float,2> Center(GunTargetRect rect) { return {static_cast<float>((static_cast<double>(rect.left)+rect.right)*0.5),static_cast<float>((static_cast<double>(rect.top)+rect.bottom)*0.5)}; }
// 원본41d530의 네 방향 edge midpoint를 동일한 float 차이/세로 동률 규칙으로 고른다.
std::array<float,2> Edge(std::array<float,2> source,GunTargetRect rect) {
    const auto center=Center(rect);const float dx=static_cast<float>(static_cast<double>(center[0])-source[0]),dy=static_cast<float>(static_cast<double>(center[1])-source[1]);
    const int direction=(std::abs(dx)<=std::abs(dy) ? (dy>0 ? 4 : 0) : (dx<=0 ? 6 : 2))+4;
    if (direction%8==0 || direction%8==4) return {static_cast<float>((static_cast<double>(rect.right)-rect.left)*0.5+rect.left),direction%8==0 ? rect.top : rect.bottom};
    return {direction%8==2 ? rect.right : rect.left,static_cast<float>((static_cast<double>(rect.bottom)-rect.top)*0.5+rect.top)};
}
// 두 대각선과 실제462800 교차를 차례로 확인한다. 사각형 경계는 포함하며 교차 helper의 끝점 규칙은 별개다.
bool Crosses(GunTargetRect rect,std::array<float,2> from,std::array<float,2> to) {
    auto point=RawFenceTarget::Intersection(from,to,{rect.left,rect.top},{rect.right,rect.bottom});
    if (rect.left<=point[0] && point[0]<=rect.right && rect.top<=point[1] && point[1]<=rect.bottom) return true;
    point=RawFenceTarget::Intersection(from,to,{rect.left,rect.bottom},{rect.right,rect.top});
    return rect.left<=point[0] && point[0]<=rect.right && rect.top<=point[1] && point[1]<=rect.bottom;
}
}
// 10.78의 전체 실제 TYPE 저장소를 공유하며 다른 월드와의 혼합을 거부한다.
RawGunBlocker::RawGunBlocker(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const GunTargetType> targetTypes,const RawGunTarget& targeting,const GunProcessState& process,
    const GunBlockerState& state,GunBlockerHooks hooks)
    :pool_(pool),hash_(hash),types_(types),targetTypes_(targetTypes),targeting_(targeting),process_(process),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || targetTypes.size()!=188 || &targeting.Pool()!=&pool || state.stoneTowerType>=types.size()) throw std::invalid_argument("장애물 통지의 판본/실제 저장소 오류");
}
// 원본0표적의 점수 조회도 허용하고 풀 밖 입력만 거부한다.
std::span<const std::uint8_t> RawGunBlocker::Slot(Sid sid) const { if (sid.value>=pool_.Capacity()) throw std::out_of_range("장애물 통지 SID 오류");const auto raw=pool_.Slot(sid);if (raw[10]>=types_.size()) throw std::out_of_range("장애물 통지 TYPE 오류");return raw; }
// 같은 raw float 좌표를 매번 다시 읽는다.
std::array<float,2> RawGunBlocker::Position(Sid sid) const { const auto raw=Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// 확장된 표적 발자국은 SID0의 원본 빈 슬롯도 동일한 min/max helper로 계산한다.
GunTargetRect RawGunBlocker::Bounds(Sid sid) const {
    const auto point=Position(sid);const auto type=Slot(sid)[10];GunTargetRect rect{point[0],point[1],point[0],point[1]};
    Include(rect,{static_cast<float>(static_cast<double>(point[0])-(types_[type].footX-1)),static_cast<float>(static_cast<double>(point[1])-(types_[type].footY+targetTypes_[type].heightExtension-1))});return rect;
}
// 발사기 점수는float 저장 뒤 지상에1000을 더하고 캐논은x87 제곱거리 전체를 사용한다.
double RawGunBlocker::Score(std::array<float,2> center,Sid target,bool archer) const {
    const auto point=Edge(center,Bounds(target));const double dx=static_cast<double>(center[0])-point[0],dy=static_cast<double>(center[1])-point[1];
    if (!archer) return dx*dx+dy*dy;
    const float distance=static_cast<float>(dx*dx+dy*dy);return static_cast<double>(distance)+((types_[Slot(target)[10]].flags2&0x100000U) ? 0.0 : 1000.0);
}
// 현재 source owner 방향의 동맹만 검사하며 이미 가득 찬 목록에서도 원본 후보 순회를 끝낸다.
std::vector<std::uint32_t> RawGunBlocker::Collect(std::uint32_t owner,std::array<float,2> point,std::size_t capacity) const {
    if (owner>kPlayerCount || capacity>20 || !hooks_.currentTarget || !hooks_.mode || !hooks_.muzzle) throw std::invalid_argument("장애물 후보 수집 실제 경계/용량 오류");
    const auto& stone=types_[state_.stoneTowerType];GunTargetRect box{point[0],point[1],point[0],point[1]};
    Include(box,{static_cast<float>(static_cast<double>(point[0])-(stone.footX-1)),static_cast<float>(static_cast<double>(point[1])-(stone.footY-1))});
    const auto middle=Center(box);std::vector<std::uint32_t> result;result.reserve(capacity);std::vector<int> heights;heights.reserve(targetTypes_.size());
    // flags5의 현재 확장 발자국 높이를 같은 번호순으로 전달한다.
    for (const auto& type:targetTypes_) heights.push_back(type.heightExtension);
    const int radius=Truncate(state_.archerRange),x=Truncate(static_cast<float>(Truncate(static_cast<double>(middle[0])+kSnapBias))),y=Truncate(static_cast<float>(Truncate(static_cast<double>(middle[1])+kSnapBias)));
    RawSquidFinder near(pool_,hash_,types_,heights);
    // 먼저group0 원본 일반 finder를 순회한다. dead/free 비트를 별도로 더해 배제하지 않는다.
    for (auto sid=near.Begin({x-radius,y-radius,x+radius,y+radius},5);sid.value;sid=near.Next()) {
        const auto raw=Slot(sid);if (types_[raw[10]].group!=0 || (raw[40]&1) || (!process_.friendlyFire && process_.IsAlly(owner,raw[34]))) continue;
        const auto center=targeting_.Center(sid);const float distance=static_cast<float>((static_cast<double>(center[0])-middle[0])*(static_cast<double>(center[0])-middle[0])+(static_cast<double>(center[1])-middle[1])*(static_cast<double>(center[1])-middle[1]));
        // 원본 최대 반경 검사는 경계를 포함하고, 아래의 기존 표적과 비교는 엄격한 작음이다.
        if (!(distance<=static_cast<double>(state_.archerRange)*state_.archerRange)) continue;
        const auto current=hooks_.currentTarget(sid);static_cast<void>(hooks_.mode(sid));
        if (current && distance<Score(targeting_.Center(sid),*current,true) && Crosses(box,hooks_.muzzle(sid),Position(*current)) && result.size()<capacity) result.push_back(sid.value);
    }
    // 캐논은새사각형에서 네방향으로 뻗는 원본 ray 영역의 일반 finder를 순서대로 사용한다.
    for (int direction=0;direction<=6;direction+=2) {
        auto ray=box;Include(ray,{static_cast<float>(static_cast<double>(kDx[direction/2])*state_.cannonRange+middle[0]),static_cast<float>(static_cast<double>(kDy[direction/2])*state_.cannonRange+middle[1])});
        RawSquidFinder finder(pool_,hash_,types_,heights);
        // 최소점은plain ftol, 최대점은0.9999f를 더한 뒤 절삭한다.
        for (auto sid=finder.Begin({Truncate(ray.left),Truncate(ray.top),Truncate(static_cast<double>(ray.right)+kCeilBias),Truncate(static_cast<double>(ray.bottom)+kCeilBias)},5);sid.value;sid=finder.Next()) {
            const auto raw=Slot(sid);if (types_[raw[10]].group!=1 || (raw[40]&1) || (!process_.friendlyFire && process_.IsAlly(owner,raw[34])) || hooks_.mode(sid)!=static_cast<std::uint32_t>((direction-4)&7)) continue;
            const auto current=hooks_.currentTarget(sid);static_cast<void>(hooks_.mode(sid));
            const auto center=targeting_.Center(sid);const float dx=static_cast<float>(static_cast<double>(center[0])-middle[0]),dy=static_cast<float>(static_cast<double>(center[1])-middle[1]);
            if (current && static_cast<double>(dx)*dx+static_cast<double>(dy)*dy<Score(center,*current,false) && result.size()<capacity) result.push_back(sid.value);
        }
    }
    return result;
}
// 현재 수집을 마친 뒤 각 번호의 기존 Gun을 다시 조회하므로 도중 삭제된 Gun도 자연스럽게 건너뛴다.
void RawGunBlocker::Notify(Sid blocker) const {
    if (!process_.boss) return;
    if (!hooks_.setTarget) throw std::logic_error("장애물 통지 실제 Gun 표적 지정 누락");
    const auto owner=Slot(blocker)[34];const auto candidates=Collect(owner,Position(blocker));
    // 실제20DWORD 목록 순서를 유지하며 같은 번호의 중복 통지도 보존한다.
    for (const auto value:candidates) { const Sid sid{static_cast<std::uint16_t>(value)};if (hooks_.currentTarget(sid)) hooks_.setTarget(sid,blocker); }
}
}
