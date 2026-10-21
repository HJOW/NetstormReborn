// 별도 지상 검색의 실제 후보 순서/Depend 필터/좌표 보정과 float 저장 위치를 보존한다.
#include "o/RawGunGroundTarget.h"
#include "o/RawSquidNeighbors.h"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kSnapBias=0.99999F; //00502310의원본float이며일반ceil의0.9999와다르다.
constexpr std::uint32_t kHitPoints=0x10,kAirborne=0x120000,kWalker=0x10000; //후보flags1/flags2의원본비트다.
// 실제종속next/head의비정렬WORD를읽는다.
Sid Link(std::span<const std::uint8_t> raw,std::size_t at) { return Sid{static_cast<std::uint16_t>(raw[at]|(static_cast<std::uint16_t>(raw[at+1])<<8))}; }
// 정상지도/사거리의signed절삭이다.물리주소범위밖입력은자동clamp하지않는다.
int Integer(double value) { if (!std::isfinite(value) || value<std::numeric_limits<int>::min() || value>std::numeric_limits<int>::max()) throw std::out_of_range("지상 표적 정수 좌표 범위");return static_cast<int>(value); }
//00422230은먼저0041d7a0로보정·float저장한뒤다시ftol한다.
int Cell(float value) { const auto snapped=static_cast<float>(Integer(static_cast<double>(value)+kSnapBias));return Integer(snapped); }
// 범위조합이호스트signed넘침으로다른주소가되지않게거부한다.
int Bound(int center,int distance,bool maximum) { const auto value=static_cast<std::int64_t>(center)+(maximum ? std::int64_t(distance) : -std::int64_t(distance));if (value<std::numeric_limits<int>::min() || value>std::numeric_limits<int>::max()) throw std::out_of_range("지상 표적 검색 사각형 범위");return static_cast<int>(value); }
}
// 실제검색은호출시에만수행한다.
RawGunGroundTarget::RawGunGroundTarget(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const GunTargetType> targetTypes,const RawGunTarget& targeting,const GunTargetState& state,GunGroundTargetGlobals globals):pool_(pool),hash_(hash),types_(types),targetTypes_(targetTypes),targeting_(targeting),state_(state),globals_(std::move(globals)) { if (pool.Edition()!=OriginalEdition::Patch1078 || &pool!=&targeting.Pool() || types.size()!=188 || targetTypes.size()!=types.size()) throw std::invalid_argument("지상 표적 같은 PATCH 풀/TYPE 누락"); }
// 진단뒤현재head를읽으며free/dead자식과TYPE0도원본필터대로통과시킬수있다.
bool RawGunGroundTarget::HasDependency(Sid parent) const {
    const auto raw=pool_.Slot(parent);if (globals_.checking && !globals_.metamap && (raw[11]&2U) && !globals_.boss) { if (!globals_.assertion) throw std::logic_error("지상 표적 Depend 진단 미연결");globals_.assertion("inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy",0x158); }
    auto current=Link(pool_.Slot(parent),6);std::unordered_set<std::uint16_t> visited;
    // next는후보가반환되기전에저장되며자산TYPE70이상만먼저건너뛴다.
    while (current.value) { if (current.value>=pool_.Capacity() || !visited.insert(current.value).second) throw std::logic_error("지상 표적 Depend 체인 손상");const auto candidate=pool_.Slot(current);const auto next=Link(candidate,4);if (candidate[10]<70 && candidate[10]==globals_.dependType) return true;current=next; }
    return false;
}
// 보행운반체의제외TYPE는Depend조회보다먼저판단한다.
bool RawGunGroundTarget::MayTarget(const GunTargetQuery& query,Sid target) const {
    const auto raw=pool_.Slot(target);if (raw[10]>=types_.size()) throw std::out_of_range("지상 표적 현재TYPE 범위");if (types_[raw[10]].flags2&kAirborne) return false;
    if (types_[pool_.Slot(target)[10]].flags2&kWalker) if (query.type==globals_.excludedType || HasDependency(target)) return false;
    return targeting_.MayTarget(query,target);
}
//0041d530(flags1)의현재중심/반대방향경계점을같은원본4방향규칙으로계산한다.
double RawGunGroundTarget::Distance(const GunTargetQuery& query,Sid target) const {
    const auto rect=targeting_.Footprint(target);const auto midX=static_cast<float>((static_cast<double>(rect.left)+rect.right)*0.5),midY=static_cast<float>((static_cast<double>(rect.top)+rect.bottom)*0.5);const auto direction=(RawSquidNeighbors::Direction(query.centerX,query.centerY,midX,midY)+4)&7;
    float x{},y{};if (direction==0 || direction==4) { x=static_cast<float>((static_cast<double>(rect.right)-rect.left)*0.5+rect.left);y=direction==0 ? rect.top : rect.bottom; }else { x=direction==2 ? rect.right : rect.left;y=static_cast<float>((static_cast<double>(rect.bottom)-rect.top)*0.5+rect.top); }
    const double dx=static_cast<double>(query.centerX)-x,dy=static_cast<double>(query.centerY)-y;return dx*dx+dy*dy;
}
// 최초후보준비뒤자기동맹을검사하며등거리/NaN후보는기존최초선택을유지한다.
Sid RawGunGroundTarget::Find(float x,float y,std::uint32_t type,std::uint32_t owner) const {
    if (type>=types_.size() || owner>kPlayerCount || !std::isfinite(x) || !std::isfinite(y)) throw std::out_of_range("지상 표적 검색 문맥 범위");const GunTargetQuery query{x,y,static_cast<float>(static_cast<double>(x)-types_[type].footX*0.5),static_cast<float>(static_cast<double>(y)-types_[type].footY*0.5),type,0,owner,0};
    const auto range=targetTypes_[type].range;const auto distance=Integer(range),cx=Cell(query.centerX),cy=Cell(query.centerY);std::vector<int> heights;heights.reserve(targetTypes_.size());
    // 실제일반finder5에전체현재TYPE의높이확장을전달한다.
    for (const auto& targetType:targetTypes_) heights.push_back(targetType.heightExtension);
    RawSquidFinder finder(pool_,hash_,types_,heights);const auto first=finder.Begin({Bound(cx,distance,false),Bound(cy,distance,false),Bound(cx,distance,true),Bound(cy,distance,true)},5);float best=static_cast<float>(static_cast<double>(range)*range);Sid result;
    if (!state_.friendlyFire && !(state_.AllyMask(owner)&(1U<<owner))) { if (!globals_.assertion) throw std::logic_error("지상 표적 자기 동맹 진단 미연결");globals_.assertion("shootMyFriends || me->isAlly(me->getPlayerId())",0x2fc); }
    // 후보반환시에저장된next와매후보의현재raw를사용한다.추가ground페널티는없다.
    for (auto sid=first;sid.value;sid=finder.Next()) { const auto raw=pool_.Slot(sid);if (raw[10]>=types_.size()) throw std::out_of_range("지상 표적 후보TYPE 범위");if (!(types_[raw[10]].flags1&kHitPoints) || !MayTarget(query,sid)) continue;const auto score=Distance(query,sid);if (score<best) { result=sid;best=static_cast<float>(score); } }
    return result;
}
// 동일실제풀연결확인용이다.
const SidPool& RawGunGroundTarget::Pool() const { return pool_; }
}
