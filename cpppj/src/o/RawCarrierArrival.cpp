// 도착의 필수 lookup와 callback 뒤 현재 전역 재조회 및 양 판본 경제 차이를 보존한다.
#include "o/RawCarrierArrival.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kCarrier=0x30000,kPriest=0x200000,kPickup=0x20a000,kNuggetMask=32; // 실제 carrier/priest/포획 후보/mask 부류다.
constexpr std::uint32_t kDone=1000,kReplaced=1001,kObstacleEvent=0x1da1a; // 실제 부모 도착 상태와 회피 후속 사건이다.
constexpr float kAlmostCeil=0.9999899864196777F,kDaisOffset=3.0F,kLowerOffset=2.0F; // 현재 helper의 실제 PE float 상수다.
// 현재 비정렬 raw 필드를 원본 폭만큼 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 원본 WORD/BYTE 옆의 stale 필드를 섞지 않는다.
    for (std::size_t index=0;index<width;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return value;
}
}
// 첫 원본 조회 전에 실제 같은 월드와 모든 선택 가능한 큰 하위 경계를 확인한다.
RawCarrierArrival::RawCarrierArrival(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    const RawCarrierPreDestroy& carrier,const RawPathCleanup& cleanup,const RawPriestState& priest,
    const RawPlayerPlacementAnchor& anchor,const RawContainedCreate& create,const RawSquidNeighbors& footprint,
    CarrierArrivalState& state,CarrierArrivalHooks hooks)
    :pool_(pool),hash_(hash),types_(types),carrier_(carrier),cleanup_(cleanup),priest_(priest),anchor_(anchor),create_(create),footprint_(footprint),state_(state),hooks_(std::move(hooks)) {
    if (&pool!=&carrier.Pool() || &pool!=&cleanup.Pool() || &pool!=&priest.Pool() || &pool!=&anchor.Pool() || &pool!=&create.Pool() || &pool!=&footprint.Pool()
        || types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)
        || !hooks_.path || !hooks_.genusAt || !hooks_.cdPriestPickupPriest || !hooks_.pickup || !hooks_.escape
        || !hooks_.chooseReturn || !hooks_.chooseGeyser || !hooks_.goReturn || !hooks_.goGeyser || !hooks_.minedPacket
        || !hooks_.destroy || !hooks_.cdAddSp || !hooks_.cdOwnerPacket || !hooks_.playAt || !hooks_.robust)
        throw std::invalid_argument("Carrier 도착 실제 풀/필수 하위 효과 연결 누락");
}
// 현재 좌표는 같은 슬롯에서 float 비트를 그대로 읽는다.
std::array<float,2> RawCarrierArrival::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// 이미 복원한 타입 표 구조를 그대로 사용한다.
std::uint32_t RawCarrierArrival::Genus(Sid sid) const { const auto type=pool_.Slot(sid)[10];if (type>=types_.size()) throw std::out_of_range("Carrier 도착 현재 타입 범위");return types_[type].flags2; }
std::uint32_t RawCarrierArrival::Owner(Sid sid) const { return pool_.Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 34 : 32]; } // 원본 현재 owner BYTE다.
std::uint8_t RawCarrierArrival::Extra(Sid sid) const { return pool_.Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]; } // 현재 abstract/buried/특수extra BYTE다.
// 정수 좌표와 현재 biased float 합을 float으로 한 번 좁히지 않는다.
std::int32_t RawCarrierArrival::Cell(float value,bool biased) {
    const double adjusted=static_cast<double>(value)+(biased ? static_cast<double>(kAlmostCeil) : 0.0);
    if (!std::isfinite(adjusted) || adjusted<0 || adjusted>=256) throw std::out_of_range("Carrier 도착 정상 지도 좌표");
    return static_cast<std::int32_t>(adjusted);
}
// 실제 owner0은 Player AI 배열을 조회하지 않고 거짓이다.
bool RawCarrierArrival::Controlled(Sid sid) const { const auto owner=Owner(sid);if (owner>=state_.controlledOwners.size()) throw std::out_of_range("Carrier 도착 Player 범위");return owner && (hooks_.currentControlled ? hooks_.currentControlled(owner) : state_.controlledOwners[owner]); }
// 동맹표의 두 순서를 같다고 가정하지 않는다.
bool RawCarrierArrival::Allied(std::uint32_t first,std::uint32_t second) const { if (first>=9 || second>=9) throw std::out_of_range("Carrier 도착 동맹표 범위");return (hooks_.currentAlliance ? hooks_.currentAlliance(first,second) : state_.alliances[first*9+second])!=0; }
// 원본 공통 정상 반환 접두를 goto 분기에서도 빠뜨리지 않는다.
std::uint32_t RawCarrierArrival::CheckedDone(Sid sid) const { if (pool_.Edition()==OriginalEdition::Patch1078 && state_.debug && !hooks_.robust(sid)) throw std::logic_error("Carrier 도착 isRobust 원본 assert");return kDone; }
// 실제 현재 Path가 없으면 원본 helper처럼 대상 필드 쓰기도 없다.
void RawCarrierArrival::SetTargets(Sid sid,std::uint32_t first,std::uint32_t second) const { if (auto* path=hooks_.path(sid)) path->SetTargets(first,second); }
// 거의올림→단순절삭 점 finder와 현재 사제 불가/owner/설정 조건을 원본 순서로 읽는다.
std::pair<Sid,bool> RawCarrierArrival::PickupCandidate(Sid sid,float x,float y) const {
    const auto cx=Cell(x,true),cy=Cell(y,true);RawSquidFinder finder(pool_,hash_,types_);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    // buried/공간 순서는 일반 finder이며 추가 dead/free/owner 필터를 넣지 않는다.
    for (auto candidate=finder.Begin({cx,cy,cx,cy});candidate.value;candidate=finder.Next()) {
        const auto genus=Genus(candidate);if (!(genus&kPickup)) continue;bool accept=true;
        if (genus&kPriest) {
            const bool immobile=priest_.GroundImmobile(candidate);accept=immobile;
            if (patch) {
                accept=Owner(sid)!=Owner(candidate) && !(Genus(sid)&kPriest) && (immobile || (state_.allowAlliedPickup && Allied(Owner(sid),Owner(candidate))));
            } else if (Owner(candidate)==Owner(sid) || ((Genus(sid)&kPriest) && !hooks_.cdPriestPickupPriest())) accept=false;
            else if (state_.allowAlliedPickup && Allied(Owner(sid),Owner(candidate))) accept=true;
        }
        if (accept) return {candidate,(genus&0x2000)!=0};
    }
    return {{},false};
}
// 현재 지면/상태/타입 다음 fly 부류가 아니면 실제 Graph를 비교한다.
bool RawCarrierArrival::ValidGeyser(Sid sid,Sid target) const {
    if ((Extra(target)&9) || (pool_.Slot(target)[11]&2) || pool_.Slot(target)[10]!=state_.geyserType) return false;
    return (Genus(sid)&0x20000) || anchor_.GraphFor(sid.value)==anchor_.GraphFor(target.value);
}
// 패치의 현재 canProcessStormpower assert와 CD의 없는 assert를 구분한다.
bool RawCarrierArrival::ValidReturn(Sid sid,Sid target) const {
    if (pool_.Edition()==OriginalEdition::Patch1078 && state_.debug && !(Genus(target)&0x200) && pool_.Slot(target)[10]!=state_.outpostType) throw std::logic_error("Carrier return 대상 원본 assert");
    if (Owner(target)!=Owner(sid) || (Extra(target)&9) || (pool_.Slot(target)[11]&2)) return false;
    return (Genus(sid)&0x20000) || anchor_.GraphFor(sid.value)==anchor_.GraphFor(target.value);
}
// 현재 보조/포획/대상/깊이 뒤 회피와 마지막 Robust를 실제 부모 순서로 실행한다.
std::uint32_t RawCarrierArrival::Arrival(Sid sid,std::uint32_t targetA,std::uint32_t targetB) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (!(Genus(sid)&kCarrier)) { if (patch) throw std::logic_error("Carrier 도착 isCarrier 원본 assert");return kDone; }
    if (Genus(sid)&kPriest) { const auto* current=hooks_.path(sid);if (current && current->Helper()) return kDone; }
    if (!Owner(sid)) return kDone;
    auto nugget=carrier_.FindContained(sid,kNuggetMask,state_.nuggetType,0);auto point=Position(sid);auto ground=hooks_.genusAt(point[0],point[1],0x10000000);point=Position(sid);static_cast<void>(hooks_.genusAt(point[0],point[1],0x2000));point=Position(sid);auto place=cleanup_.ReturnPlaceAt(point[0],point[1],1);const auto captured=carrier_.FindContained(sid,kNuggetMask,state_.priestType,0);
    if (captured.value) { place={};ground={};nugget={}; }
    point=Position(sid);auto dais=cleanup_.NearbyType(point[0],point[1],1,state_.daisType);
    if (dais.value) { const auto daisPoint=Position(dais);if ((Extra(dais)&1) || static_cast<double>(point[0])!=static_cast<double>(daisPoint[0])-kDaisOffset || static_cast<double>(point[1])!=static_cast<double>(daisPoint[1])-kDaisOffset) dais={}; }
    point=Position(sid);const auto pickup=PickupCandidate(sid,point[0],point[1]);const auto currentContent=carrier_.FindContained(sid,kNuggetMask,0,0);static_cast<void>(carrier_.FindContained(sid,1,0,0));const auto* currentPath=hooks_.path(sid);
    if (!currentPath || !currentPath->MoveAwayDepth()) {
        if (!(types_[pool_.Slot(sid)[10]].flags1&0x1000000) && pickup.first.value && (pickup.second || !currentContent.value)) hooks_.pickup(sid);
        else if (dais.value) {
            if (state_.boss) {
                if (captured.value && Allied(Owner(sid),Owner(dais)) && !carrier_.FindContained(dais,kNuggetMask,state_.priestType,0).value) { create_.Create(dais,state_.priestType,Read(pool_.Slot(captured),12,2),kNuggetMask,0,0);hooks_.destroy(captured,0); }
                static_cast<void>(hooks_.escape(sid,5.0F,0));return kReplaced;
            }
            if (patch) return CheckedDone(sid);
        } else if (!place.value || !nugget.value) {
            if (ground.value && anchor_.GraphFor(ground.value)==anchor_.GraphFor(sid.value)) {
                if (patch && !state_.boss) return CheckedDone(sid);bool replaced=false;
                if (state_.boss && !Controlled(sid)) {
                    if (!targetB || !ValidReturn(sid,Sid{static_cast<std::uint16_t>(targetB)})) targetB=hooks_.chooseReturn(sid).value;
                    SetTargets(sid,ground.value,targetB);if (targetB) { replaced=true;hooks_.goReturn(sid,Sid{static_cast<std::uint16_t>(targetB)}); }
                }
                if (state_.boss && !nugget.value && Read(pool_.Slot(ground),12,2)) { create_.Create(sid,state_.nuggetType,0,kNuggetMask,0,0);hooks_.minedPacket(ground); }
                if (replaced) return kReplaced;
            }
        } else if (Owner(place)==Owner(sid) && anchor_.GraphFor(place.value)==anchor_.GraphFor(sid.value)) {
            if (!patch) {
                const auto kind=Read(pool_.Slot(nugget),18,2);if (kind>=types_.size() || !std::isfinite(types_[kind].cost)) throw std::out_of_range("CD 너겟 현재 비용 범위");const double cost=types_[kind].cost;if (cost<-2147483648.0 || cost>=2147483648.0) throw std::out_of_range("CD 너겟 비용 절삭 범위");
                hooks_.cdAddSp(Owner(sid),static_cast<std::int32_t>(cost));hooks_.cdOwnerPacket(Owner(sid));const auto center=footprint_.Center(place);hooks_.playAt(center[0],center[1],"guy.wav",0);if (state_.boss) hooks_.destroy(nugget,0);
            } else if (!state_.boss) return CheckedDone(sid);
            if (state_.boss && !Controlled(sid)) {
                if (!targetA || !ValidGeyser(sid,Sid{static_cast<std::uint16_t>(targetA)})) { targetA=hooks_.chooseGeyser(sid).value;SetTargets(sid,targetA,0); }
                if (targetA) { hooks_.goGeyser(sid,Sid{static_cast<std::uint16_t>(targetA)});return kReplaced; }
            }
        }
    }
    if (state_.boss) {
        point=Position(sid);
        if (!(Genus(sid)&kPriest) || !cleanup_.NearbyType(point[0],point[1],1,state_.daisType).value) {
            const auto x=Cell(point[0]),y=Cell(point[1]);RawSquidFinder finder(pool_,hash_,types_);
            // 현재 점의 기본 비표면 finder는 다른 운반체/특정 지면 자산을 원본 순서로 검사한다.
            for (auto candidate=finder.Begin({x,y,x,y},1);candidate.value;candidate=finder.Next()) {
                if ((Genus(candidate)&kCarrier) && candidate!=sid && candidate!=pickup.first) return hooks_.escape(sid,1.0F,0xffffffff);
                if (pool_.Slot(candidate)[10]==state_.isleStalagType && !Controlled(sid)) return hooks_.escape(sid,2.0F,kObstacleEvent);
            }
            if (!Controlled(sid) && !(Genus(sid)&0x20000)) {
                point=Position(sid);const std::array<std::array<float,2>,2> positions{{point,{point[0],static_cast<float>(static_cast<double>(point[1])+kLowerOffset)}}};const std::array<std::uint32_t,2> genus{{0x4200,0x200}};const std::array<float,2> delays{{1,2}};
                // 마지막 두 점은 현재 위치/y+2이며 각 점의 plain 절삭 비표면 finder다.
                for (std::size_t index=0;index<positions.size();++index) {
                    const auto px=Cell(positions[index][0]),py=Cell(positions[index][1]);RawSquidFinder obstacles(pool_,hash_,types_);
                    for (auto candidate=obstacles.Begin({px,py,px,py},1);candidate.value;candidate=obstacles.Next()) if (Genus(candidate)&genus[index]) return hooks_.escape(sid,delays[index],kObstacleEvent);
                }
            }
        }
    }
    return CheckedDone(sid);
}
// 실제 사제 Arrival/Path 연결은 다른 풀 혼합을 효과 전에 거부한다.
const SidPool& RawCarrierArrival::Pool() const { return pool_; }
}
