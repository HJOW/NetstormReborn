// 원본 신전 의식의 좌표 저장 경계와 하위 효과 뒤 현재 상태 재조회를 유지한다.
#include "o/RawTempleRitual.h"
#include "o/RawPathAnimation.h"
#include "o/RawSquidHitPoints.h"
#include "o/SquidFactory.h"
#include "o/SquidOwner.h"
#include "o/SquidProcess.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::array<std::int32_t,4> kCountFirst{{0,3,64,120}}; // 실제 dais count1..3의 첫 프레임이다. index0은 사용하지 않는다.
constexpr std::array<std::int32_t,5> kStageOffsets{{0,11,22,33,44}}; // 원본 각 의식 단계의 첫 프레임 오프셋이다.
constexpr std::array<std::string_view,5> kStageSounds{{"forWind2.wav","forSun2.wav","forRain2.wav","forThunder2.wav","forStorm2.wav"}}; // 현재 단계의 위치 음향이다.
constexpr std::array<float,5> kStageDelays{{1.9600000381469727F,2.25F,1.8799999952316284F,2.2200000286102295F,2.4100000858306885F}}; // 실제0x307 payload의 원본 float 비트다.
constexpr float kAlmostCeil=0.9999899864196777F; // 사각형 우하단만 원본0041d7a0/CD00440a80으로 거의 올린다.
constexpr std::uint32_t kDaisType=165,kRitualEvent=0x307; // 이 몸체가 처리하는 신전 타입과 후속 공유 사건이다.
// 정렬되지 않은 현재 raw 필드를 지정 폭의 little endian 값으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // CD의 BYTE/WORD 옆 필드를 섞지 않는다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 실제 finder 정상 좌표 범위를 0방향 절삭한다. 손상/무한 좌표의 원본 불확정 읽기는 실행하지 않는다.
int Cell(double value) {
    if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("신전 의식 좌표 절삭 범위");
    return static_cast<int>(value);
}
}
// 모든 하위 효과와 같은 풀/해시/판본 목록 크기를 첫 요청 전에 확인한다.
RawTempleRitual::RawTempleRitual(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const RiftTypeFrames> frames,std::span<const SquidDisplayShape> shapes,std::span<const std::uint8_t> spots,
    const RawSquidNeighbors& footprint,const SquidReward& hitPoints,const RawPriestRitualActions& actions,
    TempleRitualState& state,TempleRitualHooks hooks)
    :pool_(pool),hash_(hash),types_(types),frames_(frames),shapes_(shapes),spots_(spots),footprint_(footprint),hitPoints_(hitPoints),actions_(actions),state_(state),hooks_(std::move(hooks)) {
    const auto count=pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if (&pool!=&footprint.Pool() || &hash!=&footprint.Hash() || &pool!=&hitPoints.Pool() || &pool!=&actions.Pool() ||
        types.size()!=count || frames.size()!=count || shapes.size()!=count || spots.size()!=kWorldCells*kWorldCells ||
        !hooks_.create || !hooks_.setHitPoints || !hooks_.setOwner || !hooks_.pop || !hooks_.playAt || !hooks_.reserveShared || !hooks_.addShared)
        throw std::invalid_argument("신전 의식 실제 하위/판본 연결 누락");
}
// 패치 signedDWORD/CDBYTE의 현재 프레임을 원본 폭대로 읽는다.
std::int32_t RawTempleRitual::CurrentFrame(Sid sid) const {
    const auto raw=pool_.Slot(sid);return pool_.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(raw,36)) : static_cast<std::int32_t>(raw[34]);
}
// 효과 뒤의 현재 사제/신전 좌표를 raw에서 다시 읽는다.
TempleRitualPoint RawTempleRitual::Position(Sid sid) const {
    const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))};
}
// owner의 판본별BYTE를 그대로 읽는다.
std::uint32_t RawTempleRitual::Owner(Sid sid) const { return pool_.Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 34 : 32]; }
// 실제 dais +0x80은 raw 상위 state/level 대신 현재 방향과 프레임 경계를 읽는다.
std::uint32_t RawTempleRitual::Count(Sid temple) const {
    const auto raw=pool_.Slot(temple);if (raw[10]!=kDaisType) throw std::invalid_argument("신전 의식 dais count 타입 오류");
    const auto frame=CurrentFrame(temple);const auto direction=RawPathAnimation::Direction(frames_[raw[10]],frame);
    if (direction==13) return 1;if (direction==12) return 2;if (direction==11) return 3;
    return frame<64 ? 1U : frame<=119 ? 2U : 3U;
}
// 명시 frame도 현재 raw count를 사용한다. frame-1만 현재 프레임으로 대체한다.
std::uint32_t RawTempleRitual::Stage(Sid temple,std::int32_t frame) const {
    if (frame==-1) frame=CurrentFrame(temple);const auto relative=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(frame)-static_cast<std::uint32_t>(kCountFirst[Count(temple)]));
    if (relative<11) return 0;if (relative<22) return 1;if (relative<33) return 2;if (relative<44) return 3;return relative<=54 ? 4U : 5U;
}
// 원본419850 반환 헤더+20/+22의 의식 목표점이다. 표시 기준점(+1c/+1e)과 구별한다.
std::array<std::int16_t,2> RawTempleRitual::Hotspot(std::uint32_t type,std::int32_t frame) const {
    if (type>=shapes_.size() || frame<0) throw std::out_of_range("신전 의식 SHP 타입/프레임 범위");const auto& shape=shapes_[type];
    const auto limit=static_cast<std::int64_t>(shape.frameCount)*((types_[type].flags1&0x440000) ? 2 : 1);
    if (!shape.loaded || frame>=limit || static_cast<std::size_t>(frame)>=shape.frames.size()) throw std::out_of_range("신전 의식 SHP frameCheck 오류");
    const auto& current=shape.frames[static_cast<std::size_t>(frame)];return {current.muzzleX,current.muzzleY};
}
// 원본 x87의 int/float+float 계산은 각 축 저장에서만 단정도로 좁힌다.
TempleRitualPoint RawTempleRitual::AddPixels(TempleRitualPoint point,std::int32_t x,std::int32_t y) const {
    if (!std::isfinite(state_.pixelScaleX) || !std::isfinite(state_.pixelScaleY) || state_.pixelScaleX==0 || state_.pixelScaleY==0) throw std::out_of_range("신전 의식 픽셀 배율 오류");
    point.x=static_cast<float>(static_cast<double>(x)/static_cast<double>(state_.pixelScaleX)+static_cast<double>(point.x));
    point.y=static_cast<float>(static_cast<double>(y)/static_cast<double>(state_.pixelScaleY)+static_cast<double>(point.y));return point;
}
// 신전의 첫 count1 구간 핫스폿이 모든 단계 Anchor의 기준이다. 현재 count별 첫 프레임을 넣지 않는다.
TempleRitualPoint RawTempleRitual::StageAnchor(Sid temple,std::uint32_t stage) const {
    if (stage>=kStageOffsets.size()) throw std::out_of_range("신전 의식 numBridgesMissing<=4 assert");const auto hotspot=Hotspot(pool_.Slot(temple)[10],kCountFirst[1]+kStageOffsets[stage]);return AddPixels(Position(temple),hotspot[0],hotspot[1]);
}
// stage4의 지면 검사는 거의올림이 아닌 원본 __ftol 절삭 칸이다.
TempleRitualPoint RawTempleRitual::StagePoint(Sid temple,std::uint32_t stage) const {
    auto point=StageAnchor(temple,stage);const auto hotspot=Hotspot(state_.runeType,static_cast<std::int32_t>(stage));point=AddPixels(point,hotspot[0],hotspot[1]);
    if (stage==4) {
        const auto index=static_cast<std::int64_t>(Cell(point.y))*kWorldCells+Cell(point.x);if (index<0 || index>=static_cast<std::int64_t>(spots_.size())) throw std::out_of_range("신전 의식 마지막 목표 지면 범위");
        const auto spot=spots_[static_cast<std::size_t>(index)];if ((spot&6) && !(spot&8)) point=AddPixels(point,-9,-6);
    }
    return point;
}
// 현재 runeType의genus에 따라0/1단계부터 실제 점 finder를 탐색한다.
Sid RawTempleRitual::RuneAt(Sid temple,TempleRitualPoint point) const {
    const auto type=state_.runeType;if (type>=types_.size()) throw std::out_of_range("신전 의식 현재Rune 타입 범위");const auto hotspot=Hotspot(type,static_cast<std::int32_t>(Stage(temple)));point=AddPixels(point,-static_cast<std::int32_t>(hotspot[0]),-static_cast<std::int32_t>(hotspot[1]));
    const auto x=Cell(point.x),y=Cell(point.y);RawSquidFinder finder(pool_,hash_,types_);
    // 원본 첫 타입 일치에서 종료하며 별도dead/free/abstract 필터를 넣지 않는다.
    for (auto sid=finder.Begin({x,y,x,y},(types_[type].flags2&6)==0 ? 1U : 0U);sid.value;sid=finder.Next()) if (pool_.Slot(sid)[10]==type) return sid;
    return {};
}
// float 발자국을 사각형 ctor의 절삭 좌상단/거의올림 우하단으로 바꾼다.
SquidSearchArea RawTempleRitual::SearchArea(const SquidFootprint& area) const {
    return {Cell(area.left),Cell(area.top),Cell(static_cast<double>(area.right)+static_cast<double>(kAlmostCeil)),Cell(static_cast<double>(area.bottom)+static_cast<double>(kAlmostCeil))};
}
// 기존 실제 발자국만±1 확장하고 후보의 signed 현재HP와현재 최대HP를 비교한다.
Sid RawTempleRitual::FindStrongOwnedPriest(Sid temple) const {
    auto area=footprint_.Footprint(temple);area.left-=1;area.top-=1;area.right+=1;area.bottom+=1;RawSquidFinder finder(pool_,hash_,types_);
    // 일반finder 순서에서 현재 type/owner/HP를 읽는다. 사제 자체extra/state는 조건이 아니다.
    for (auto sid=finder.Begin(SearchArea(area));sid.value;sid=finder.Next()) {
        const auto raw=pool_.Slot(sid);if (raw[10]!=state_.priestType || Owner(sid)!=Owner(temple)) continue;
        const auto hp=pool_.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(raw,26)) : static_cast<std::int32_t>(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Read(raw,26,2))));
        if (hp>=hitPoints_.MaxHitPoints(sid)/2) return sid;
    }
    return {};
}
// 현재 실제 발자국과일반finder를 재사용해추상Rune 존재만읽는다.
bool RawTempleRitual::HasAbstractRune(Sid temple) const {
    RawSquidFinder finder(pool_,hash_,types_);
    // 첫 현재RuneType/extra1 일치만 참이며 일반finder의 buried 제외를 그대로 유지한다.
    for (auto sid=finder.Begin(SearchArea(footprint_.Footprint(temple)));sid.value;sid=finder.Next()) { const auto raw=pool_.Slot(sid);if (raw[10]==state_.runeType && (raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&1)) return true; }
    return false;
}
// HP/소유자 효과 이후 각각현재 단계를 다시읽는 실제Rune 생성 순서다.
void RawTempleRitual::SpawnRune(Sid temple) const {
    const auto rune=hooks_.create(state_.runeType,0);if (!rune.value || rune.value>=pool_.Capacity()) throw std::out_of_range("신전 의식Rune 생성SID 오류");hooks_.setHitPoints(rune,0);const auto stage=Stage(temple);auto raw=pool_.AllocatedBytes(rune);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    // 원본 패치DWORD/CDBYTE 프레임만 쓰고 옆필드는 보존한다.
    for (std::size_t i=0;i<(patch ? 4U : 1U);++i) raw[(patch ? 36U : 34U)+i]=static_cast<std::uint8_t>(stage>>(8*i));
    if (!(raw[11]&4)) throw std::logic_error("신전 의식Rune isVoid assert");raw[patch ? 40 : 35]|=1;hooks_.setOwner(rune,Owner(temple));const auto point=StageAnchor(temple,Stage(temple));hooks_.pop(rune,point.x,point.y,0);
}
// 초기단계는 mode0 음향/지연에 유지하고후속mode는각효과뒤현재값을재조회한다.
void RawTempleRitual::Ritual(Sid temple,Sid priest,std::uint32_t mode) const {
    const auto stage=Stage(temple);if (stage>4) return;auto expected=StagePoint(temple,stage);auto actual=Position(priest);
    if (actual.x!=expected.x || actual.y!=expected.y) { if (stage>=4) return;expected=StagePoint(temple,stage+1);actual=Position(priest);if (actual.x!=expected.x || actual.y!=expected.y) return; }
    if (RuneAt(temple,Position(priest)).value) return;
    if (!mode) {
        actions_.Begin(priest.value,stage);actual=Position(priest);hooks_.playAt(actual.x,actual.y,kStageSounds[stage],0);
        if (hooks_.reserveShared()) hooks_.addShared(temple,kRitualEvent,kStageDelays[stage]);return;
    }
    const auto found=FindStrongOwnedPriest(temple);if (found.value && found.value<pool_.Capacity()) actions_.Continue(found.value,Stage(temple));
    if (state_.boss && !HasAbstractRune(temple)) SpawnRune(temple);
}
// 실제 PathCleanup/Factory/owner/Host와같은풀연결임을검사한다.
const SidPool& RawTempleRitual::Pool() const { return pool_; }
// 공통몸체는새로계산하지않으며필수Rune Pop과위치음향은명시연결로남긴다.
TempleRitualHooks MakeTempleRitualHooks(SidPool& pool,SquidFactory& factory,SquidOwner& owner,SquidProcessHost& host,TempleRitualHooks hooks) {
    if (&pool!=&factory.Pool() || &pool!=&owner.Pool() || &pool!=&host.Pool()) throw std::invalid_argument("신전 의식 Factory/owner/Host SID 풀 불일치");
    hooks.create=[&factory](std::uint32_t type,std::uint32_t flags) { return factory.Create(type,flags); };
    hooks.setHitPoints=[&pool](Sid sid,std::int32_t value) { SetRawSquidHitPoints(pool,sid,value); };
    hooks.setOwner=[&owner](Sid sid,std::uint32_t player) { owner.Set(sid,player); };
    hooks.addShared=[&host](Sid sid,std::uint32_t event,float payload) { host.AddSharedRegular(sid,event,payload); };
    return hooks;
}
}
