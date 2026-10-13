// 원본 cleanup의 helper 쓰기·현재 소유/Graph 비교·의식/환불/삭제 순서를 보존한다.
#include "o/RawPathCleanup.h"
#include "o/RawCarrierPreDestroy.h"
#include "o/RawPlayerPlacementAnchor.h"
#include "o/RawConstructionPlace.h"
#include "o/SquidProcess.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kHelperGenus=0x404200,kReturnPlaceGenus=0x200,kNuggetMask=0x20; // 원본 helper 부류·귀환 장소 부류·contained mask다.
constexpr float kFinderBias=0.9999899864196777F; // 0041d7a0/CD00440a80의 실제 float bias다.
constexpr double kFtolLimit=9223372036854775808.0; // CRT 불확정 signed64의 경계다.
}
// 현재 자료 폭과 필수 실제 하위 경계를 첫 효과 전에 확인한다.
RawPathCleanup::RawPathCleanup(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    PathCleanupState& state,PathCleanupHooks hooks):pool_(pool),hash_(hash),types_(types),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || !hooks_.reserveShared || !hooks_.addShared || !hooks_.contained || !hooks_.graphFor || !hooks_.ritual || !hooks_.refund || !hooks_.destroy)
        throw std::invalid_argument("Path cleanup 실제 하위 연결 누락");
}
// 원본 x87 덧셈/절삭 뒤 하위DWORD만 읽는다. 좁은 좌표 사각형을 임의로 반올림하지 않는다.
std::int32_t RawPathCleanup::Cell(float coordinate,bool biased) {
    const double value=static_cast<double>(coordinate)+(biased ? static_cast<double>(kFinderBias) : 0.0);
    return Truncate(value);
}
// 현재 x87 값의 __ftol/EAX 규약을 좌표 범위 조립에도 그대로 사용한다.
std::int32_t RawPathCleanup::Truncate(double value) {
    if (!std::isfinite(value) || value>=kFtolLimit || value< -kFtolLimit) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 정렬되지 않은 float 좌표의 바이트를 순서대로 조합한다.
float RawPathCleanup::Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits=0;
    // 지정 DWORD만 읽는다.
    for (std::size_t i=0;i<4;++i) bits|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return std::bit_cast<float>(bits);
}
// 현재 raw 타입으로 현재 타입 표의 flags2를 조회한다.
std::uint32_t RawPathCleanup::Genus(Sid sid) const {
    const auto type=pool_.Slot(sid)[10];if (type>=types_.size()) throw std::out_of_range("cleanup 현재 raw 타입 범위");return types_[type].flags2;
}
// 소유자 BYTE는 patch+34/CD+32다.
std::uint8_t RawPathCleanup::Owner(Sid sid) const { return pool_.Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 34 : 32]; }
// abstract/매장 extra BYTE는 patch+40/CD+35다.
std::uint8_t RawPathCleanup::Extra(Sid sid) const { return pool_.Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]; }
// radius0은 단일 절삭 칸이며 그 밖에는 원본 fild radius와 좌표±radius의 사각형이다.
Sid RawPathCleanup::ReturnPlaceAt(float x,float y,std::int32_t radius) const {
    RawSquidFinder finder(pool_,hash_,types_);
    const double dx=x,dy=y,dr=radius;
    // 실제 기본 finder는 0단계를 포함한다. free/void를 별도 조건으로 추가하지 않는다.
    for (auto sid=finder.Begin({Truncate(dx-dr),Truncate(dy-dr),Truncate(dx+dr),Truncate(dy+dr)});sid.value;sid=finder.Next()) {
        const auto raw=pool_.Slot(sid);
        if (!(Extra(sid)&1) && !(raw[11]&2) && ((Genus(sid)&kReturnPlaceGenus)!=0 || raw[10]==state_.outpostType)) return sid;
    }
    return {};
}
// 원본 ctor가 almostceil 좌표를 먼저 만들고 radius만큼 사각형을 넓힌다.
Sid RawPathCleanup::NearbyType(float x,float y,std::uint32_t radius,std::uint32_t type) const {
    if (type>=types_.size()) throw std::out_of_range("cleanup 근처 타입 범위");const auto cx=Cell(x,true),cy=Cell(y,true);
    const auto distance=std::bit_cast<std::int32_t>(radius);RawSquidFinder finder(pool_,hash_,types_);
    // radius는 캠페인 본문에서2이며 원본 일반 finder의 순서/부류 시작 단계를 재사용한다.
    for (auto sid=finder.Begin({cx-distance,cy-distance,cx+distance,cy+distance},(types_[type].flags2&6)==0 ? 1U : 0U);sid.value;sid=finder.Next())
        if (pool_.Slot(sid)[10]==type) return sid;
    return {};
}
// 공간 접두 뒤에도 원래 부모를 보존하고, callback 후 helper/권한/타입은 현재 상태로 다시 읽는다.
void RawPathCleanup::AfterSpatial(RawPathRuntime& runtime,Sid parent) const {
    if (runtime.Bytes().size()!=RawPathRuntime::Layout(pool_.Edition()).size) throw std::invalid_argument("cleanup 경로 판본 폭 불일치");
    const auto helper=runtime.Helper();
    if (helper && helper<pool_.Capacity() && (Genus(Sid{static_cast<std::uint16_t>(helper)})&kHelperGenus)!=0) {
        const auto raw=pool_.Slot(parent);const auto x=Cell(Coordinate(raw,14),true),y=Cell(Coordinate(raw,18),true);RawSquidFinder finder(pool_,hash_,types_);
        // helper가 실제 현재 사각형 안에 있을 때만 예약한다. 원본처럼 첫 일치에서 종료한다.
        for (auto sid=finder.Begin({x-2,y-2,x+2,y+2});sid.value;sid=finder.Next()) {
            if (sid.value!=helper) continue;
            if (hooks_.reserveShared()) hooks_.addShared(Sid{static_cast<std::uint16_t>(runtime.Helper())},state_.helperEvent,1.0F);
            runtime.ClearHelper();break;
        }
    }
    if (pool_.Edition()==OriginalEdition::Patch1078 && !(pool_.Slot(parent)[11]&2)) {
        const auto raw=pool_.Slot(parent);const auto place=ReturnPlaceAt(Coordinate(raw,14),Coordinate(raw,18));
        const auto nugget=hooks_.contained(parent,kNuggetMask,state_.nuggetType,0);
        if (place.value && nugget.value && Owner(place)==Owner(parent)) {
            const auto parentGraph=hooks_.graphFor(parent),placeGraph=hooks_.graphFor(place);
            if (placeGraph==parentGraph && state_.boss) hooks_.destroy(nugget,0);
        }
    }
    if (pool_.Slot(parent)[10]==state_.priestType) {
        const auto raw=pool_.Slot(parent);const auto temple=NearbyType(Coordinate(raw,14),Coordinate(raw,18),2,state_.daisType);
        if (temple.value && !(Extra(temple)&8)) hooks_.ritual(temple,parent,0);
    }
    const auto remaining=runtime.Helper();
    if (state_.boss && remaining) {
        if (remaining>=pool_.Capacity()) throw std::out_of_range("cleanup 남은 helper SID 범위");const Sid target{static_cast<std::uint16_t>(remaining)};
        if (!(pool_.Slot(target)[11]&1)) {
            if (!(Extra(target)&1) || !(Genus(target)&kHelperGenus)) {
                if (state_.checking && pool_.Edition()==OriginalEdition::Patch1078) throw std::logic_error("cleanup shouldDestroy 원본 assert");return;
            }
            const auto raw=pool_.Slot(target);hooks_.refund(raw[10],Owner(target));hooks_.destroy(target,0);
        }
    }
}
// 같은 raw 풀을 사용하는 실제 소비자만 연결한다.
const SidPool& RawPathCleanup::Pool() const { return pool_; }
// 기존 모듈 몸체를 다시 구현하지 않고 정확한 인자/순서의 callback에 연결한다.
PathCleanupHooks MakePathCleanupHooks(const SidPool& pool,const RawCarrierPreDestroy& carrier,const RawPlayerPlacementAnchor& anchor,
    SquidProcessHost& host,RawConstructionPlace& place,PathCleanupHooks hooks) {
    if (&carrier.Pool()!=&pool || &anchor.Pool()!=&pool || &host.Pool()!=&pool || &place.Pool()!=&pool) throw std::invalid_argument("cleanup 하위 모듈 SID 풀 불일치");
    hooks.contained=[&carrier](Sid sid,std::uint32_t mask,std::uint32_t kind,std::uint32_t index) { return carrier.FindContained(sid,mask,kind,index); };
    hooks.graphFor=[&anchor](Sid sid) { return anchor.GraphFor(sid.value); };
    hooks.addShared=[&host](Sid sid,std::uint32_t event,float payload) { host.AddSharedRegular(sid,event,payload); };
    hooks.refund=[&place](std::uint32_t type,std::uint32_t owner) { place.Refund(type,owner); };
    return hooks;
}
}
