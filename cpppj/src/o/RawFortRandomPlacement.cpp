// 단일 배치의 실제 검사 순서·현재 타입 재조회·지면 교체/생성 효과를 보존한다.
#include "o/RawFortRandomPlacement.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 raw 타입/상태·HP와 판본별 프레임 위치다.
constexpr std::size_t kType=10,kState=11,kHp=26,kPatchFrame=36,kCdFrame=34;
// 단일 배치가 조회하는 특별 프레임·spot 하위 비트·void 상태다.
constexpr std::uint32_t kTerrainFrame=0x200000,kRandomFrame=0x100000,kSpotMask=0x3f,kVoid=4;
// 고정 P/P wildcard 검색의 최대 후보와 실제 방향 문자다.
constexpr std::size_t kCandidates=256;
constexpr std::uint8_t kP='P',kA='A';
// x87 CRT 절삭은 하위 DWORD를 사용한다. 원본이 주소를 벗어나는 비정상 입력은 거부한다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || std::fabs(value)>=9.2e18) throw std::out_of_range("Fort 배치 CRT 좌표 범위 오류");
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 판본 좁은 필드를 읽거나 쓰며 옆 payload는 보존한다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    std::uint32_t result=0;
    // 지정 폭만 little endian으로 조합한다.
    for (std::size_t byte=0;byte<width;++byte) result|=static_cast<std::uint32_t>(raw[offset+byte])<<(8*byte);return result;
}
// 지정한 DWORD/WORD/BYTE만 원본 raw 슬롯에 기록한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width) {
    // 실제 쓰기 폭만 변경한다.
    for (std::size_t byte=0;byte<width;++byte) raw[offset+byte]=static_cast<std::uint8_t>(value>>(8*byte));
}
}
// 자료/모든 필수 효과·실제 hash0을 검사한 뒤 빌린 현재 저장소를 유지한다.
RawFortRandomPlacement::RawFortRandomPlacement(SidPool& pool,const SquidHash& hash,std::span<const std::uint8_t> spots,
    std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,GameRandom& globalRandom,
    FortRandomPlacementState& state,FortRandomPlacementHooks hooks)
    :pool_(pool),hash_(hash),spots_(spots),types_(types),frames_(frames),random_(globalRandom),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || frames.size()!=types.size() ||
        spots.size()!=65536 || hash.Entries(0).size()!=65536 || !hooks_.mayPlace || !hooks_.chunkRegion || !hooks_.regionOwner ||
        !hooks_.create || !hooks_.construct || !hooks_.hasHitPoints || !hooks_.typeHitPoints || !hooks_.setOwner ||
        !hooks_.unpop || !hooks_.repop || !hooks_.pop) throw std::invalid_argument("Fort 단일 배치 판본/자료/실제 효과 오류");
}
// 요청/현재 raw 타입을 실제 자료 범위 안에서 읽는다.
const RiftTypeRecord& RawFortRandomPlacement::Type(std::uint32_t number) const {
    if (number>=types_.size()) throw std::out_of_range("Fort 단일 배치 타입 범위 오류");return types_[number];
}
// 현재 일반 hash/finder의 exact type/genus 검색이다. 초기 spot의 반올림 좌표와 구별한다.
Sid RawFortRandomPlacement::Find(float x,float y,std::uint32_t value,bool genus) const {
    const auto ix=Ftol(x),iy=Ftol(y);RawSquidFinder finder(pool_,hash_,types_);
    const auto flags=!genus && !(Type(value).flags2&6U) ? 1U : 0U; // exact type의 surface/bridge 부류가 아니면 hash0을 건너뛴다.
    // 원본 finder의 void/footprint/후행 next 규칙과 처음 통과 순서를 유지한다.
    for (auto sid=finder.Begin({ix,iy,ix,iy},flags);sid.value;sid=finder.Next()) {
        const auto type=pool_.Slot(sid)[kType];if (genus ? (Type(type).flags2&value)!=0 : type==value) return sid;
    }
    return {};
}
// WORD 표면 번호/type만 검사한다. 게임 상태 필터를 새로 넣지 않는다.
bool RawFortRandomPlacement::Island(std::int32_t x,std::int32_t y) const {
    if (x<0 || y<0 || x>=256 || y>=256) return false;const Sid sid{hash_.Entries(0)[static_cast<std::size_t>(y*256+x)]};
    return sid.value && pool_.Slot(sid)[kType]==state_.islandType;
}
// NW→NE→SE→SW의 원본 단락 평가를 유지한다.
bool RawFortRandomPlacement::MayReplace(std::int32_t x,std::int32_t y) const {
    if (x<0 || y<0 || x>=256 || y>=256) throw std::out_of_range("Fort 교체 좌표 범위 오류");
    if (!Island(x-1,y-1) && Island(x,y-1) && Island(x-1,y)) return false;
    if (!Island(x+1,y-1) && Island(x,y-1) && Island(x+1,y)) return false;
    if (!Island(x+1,y+1) && Island(x,y+1) && Island(x+1,y)) return false;
    if (!Island(x-1,y+1) && Island(x,y+1) && Island(x-1,y)) return false;return true;
}
// 원본 Retype는 void와 두 foot 동일성만 검사하고 현재 생성자만 재실행한다.
void RawFortRandomPlacement::Retype(Sid sid,std::uint32_t number) const {
    auto raw=pool_.AllocatedBytes(sid);const auto& before=Type(raw[kType]);const auto& after=Type(number);
    if (!(raw[kState]&kVoid) || before.footX!=after.footX || before.footY!=after.footY)
        throw std::logic_error("Fort Retype 원본 void/동일 foot assert");
    raw[kType]=static_cast<std::uint8_t>(number);hooks_.construct(sid,raw[kType]);
}
// 최초 프레임은 가상 SetFrame 없이 patch DWORD/CD BYTE로 직접 쓴다.
void RawFortRandomPlacement::Frame(Sid sid,std::uint32_t value) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;Put(pool_.AllocatedBytes(sid),patch ? kPatchFrame : kCdFrame,value,patch ? 4U : 1U);
}
// HasHitPoints 호출 뒤 현재 타입 HP를 읽고 공통 HP 필드에 직접 쓴다.
void RawFortRandomPlacement::HitPoints(Sid sid) const {
    if (hooks_.hasHitPoints(sid)) {
        const auto hp=hooks_.typeHitPoints(pool_.Slot(sid)[kType]);Put(pool_.AllocatedBytes(sid),kHp,static_cast<std::uint32_t>(hp),
            pool_.Edition()==OriginalEdition::Patch1078 ? 4U : 2U);
    }
}
// 실제 고정 P/P fallback·wildcard·후보 최대256과 global PRNG 한 번 소비다.
std::int32_t RawFortRandomPlacement::RandomFrame(std::uint32_t type) const {
    Type(type);const auto codes=frames_[type].Codes();std::array<std::int32_t,kCandidates> candidates{};std::size_t count=0;
    // P/P→변환 P/P(P 방향 mask0)→P/A→P/임의 변형의 첫 비어 있지 않은 집합이다.
    for (int pass=0;pass<4 && !count;++pass) {
        // 앞256개의 일치 인덱스만 저장하며 나머지 코드도 원본처럼 계속 읽는다.
        for (std::size_t index=0;index<codes.size();++index) {
            if (codes[index].side==kP && (pass==3 || codes[index].variant==(pass==2 ? kA : kP)) && count<kCandidates)
                candidates[count++]=static_cast<std::int32_t>(index);
        }
    }
    return count ? candidates[random_.Next(static_cast<std::uint32_t>(count))] : -1;
}
// 실제 단일 body의 조기 반환/지면 교체/새 객체 효과를 순서대로 실행한다.
std::uint32_t RawFortRandomPlacement::Place(GameRandom& localRandom,const FortRandomPlaceRequest& request) const {
    (void)localRandom;const auto sx=Ftol(static_cast<double>(request.x)+0.5),sy=Ftol(static_cast<double>(request.y)+0.5);
    if (sx<0 || sy<0 || sx>=256 || sy>=256) throw std::out_of_range("Fort 반올림 spot 주소 범위 오류");
    if (!(spots_[static_cast<std::size_t>(sy*256+sx)]&kSpotMask)) return 0;
    if (Find(request.x,request.y,state_.edgeFarmType,false).value) return 0;
    const CanonPlacementQuery query{request.type,0,request.x,request.y,0,0,0};
    if (!hooks_.mayPlace(query)) return 0;
    if (!request.force && static_cast<std::int32_t>(hooks_.chunkRegion(Ftol(request.x)/16,Ftol(request.y)/16))!=request.region) return 0;
    const auto& requested=Type(request.type);std::uint32_t frame=0;
    if (requested.flags1&kTerrainFrame) {
        const auto ground=Find(request.x,request.y,TypeFlag2::kIsland,true);
        if (!ground.value) throw std::logic_error("Fort terrain-frame 원본 지면 assert");
        if (request.mode) {
            if (!MayReplace(Ftol(request.x),Ftol(request.y))) return 1;
            hooks_.unpop(ground,0);Retype(ground,state_.edgeFarmType);HitPoints(ground);hooks_.repop(ground,64);return ground.value;
        }
        const bool patch=pool_.Edition()==OriginalEdition::Patch1078;frame=Read(pool_.Slot(ground),patch ? kPatchFrame : kCdFrame,patch ? 4U : 1U);
    }
    if (requested.flags1&kRandomFrame) frame=static_cast<std::uint32_t>(RandomFrame(request.type));
    if (frame==0xffffffffU) throw std::logic_error("Fort 단일 배치 프레임 -1 원본 assert");
    const auto sid=hooks_.create(request.type,0);Frame(sid,frame);HitPoints(sid);
    auto owner=request.force ? 0U : hooks_.regionOwner(request.region);if (state_.ownerOverride) owner=state_.ownerOverride;
    hooks_.setOwner(sid,owner);hooks_.pop(sid,request.x,request.y,0);return sid.value;
}
// 같은 pool을 연결해야 지역·공간·형식 효과가 다른 월드로 흐르지 않는다.
const SidPool& RawFortRandomPlacement::Pool() const { return pool_; }
// 이미 복원한 전체 공간 검사와 실제 factory/HP 조회를 조립한다.
FortRandomPlacementHooks MakeFortRandomPlacementHooks(const SidPool& pool,RawCanonPlacementPipeline& placement,
    SquidFactory& factory,const SquidReward& hp,FortRandomPlacementHooks hooks) {
    if (&pool!=&placement.Pool() || &pool!=&factory.Pool() || &pool!=&hp.Pool()) throw std::invalid_argument("Fort placement 어댑터 pool 불일치");
    hooks.mayPlace=[&placement](const CanonPlacementQuery& query) { return placement.MayPlace(query); };
    hooks.create=[&factory](std::uint32_t type,std::uint32_t flags) { return factory.Create(type,flags); };
    hooks.construct=[&factory](Sid sid,std::uint32_t type) { factory.Construct(type,sid); };
    hooks.hasHitPoints=[&hp](Sid sid) { return hp.HasHitPoints(sid); };
    hooks.typeHitPoints=[&hp](std::uint32_t type) { return hp.TypeHitPoints(type); };return hooks;
}
// FinishOwner의 local PRNG를 실제 단일 body에 그대로 전달한다.
FortFinishOwnerHooks MakeFortSinglePlacementHook(const SidPool& pool,const RawFortRandomPlacement& placement,FortFinishOwnerHooks hooks) {
    if (&pool!=&placement.Pool()) throw std::invalid_argument("Fort finish/단일 placement pool 불일치");
    hooks.place=[&placement](GameRandom& random,const FortRandomPlaceRequest& request) { return placement.Place(random,request); };return hooks;
}
}
