// 원문의 snapshot/재조회/반복·직접 DWORD/WORD 쓰기 위치를 보존한다.
#include "o/RawIslandPopulation.h"
#include "o/RawSquidHitPoints.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 새 규칙 없이 기존 배치/지도/HP 몸체를 원문 부모에 조립한다.
namespace { // 원문 little endian과 x87 저장 경계를 지역화한다.
template<class F> const F& Need(const F& callback) { if (!callback) throw std::logic_error("섬 population의 실제 하위 미연결");return callback; } //선택된 효과의 부재만 그 위치에서 거부한다.
float Cell(std::int32_t value) { return static_cast<float>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)<<4)); } //0040d900의 DWORD 감김16배를 float32로 저장한다.
std::int32_t Truncate(double value) { if (!std::isfinite(value) || std::fabs(value)>=9223372036854775808.0) return 0;return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value))); } //원문_ftol64 절삭의 하위DWORD다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) { for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } //정확한4BYTE만 쓰고 옆 필드를 보존한다.
FortRandomRectangle Center(const FortRandomRectangle& area) {
    const auto horizontal=static_cast<float>(static_cast<double>(area.right)-area.left-4.0);const auto vertical=static_cast<double>(area.bottom)-area.top-4.0; //x는실제fst32, y는x87 레지스터에 유지된다.
    const auto x=horizontal>=0 ? static_cast<double>(horizontal) : -static_cast<double>(area.right)-area.left-4.0;const auto y=vertical>=0 ? vertical : -static_cast<double>(area.bottom)-area.top-4.0; //원문의 음수 길이 대체식을 그대로 보존한다.
    return {static_cast<float>(Truncate(static_cast<double>(area.left)+x*0.5)),static_cast<float>(Truncate(static_cast<double>(area.top)+y*0.5)),static_cast<float>(Truncate(static_cast<double>(area.right)-x*0.5)),static_cast<float>(Truncate(static_cast<double>(area.bottom)-y*0.5))};
} //0040e2b0의 네_ftol 출력은 각signed 정수를 다시float32로 저장한다.
}
RawIslandPopulation::RawIslandPopulation(SidPool& pool,ChunkMap& target,ChunkMap& world,const RawIslandRuntime& islands,const RawFortFinishOwner& randomPlacement,IslandPopulationGlobals globals,IslandPopulationHooks hooks)
    :pool_(pool),target_(target),world_(world),islands_(islands),placement_(randomPlacement),globals_(globals),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || &islands.Pool()!=&pool || &randomPlacement.Pool()!=&pool) throw std::invalid_argument("섬 population의 실제 PATCH/Pool 불일치");
}
FortRandomRectangle RawIslandPopulation::Bounds(std::int32_t region) const {
    if (!islands_.Exists(region)) Need(hooks_.assertion)(IslandPopulationAssertion::BoundsMissing);const auto rect=target_.Bounds(region);return {Cell(rect.left),Cell(rect.top),Cell(rect.right),Cell(rect.bottom)};
} //진단 뒤 현재 target의 실제 청크를 읽으며 미존재16/16/1/1 결과를 임의 사각형으로 바꾸지 않는다.
void RawIslandPopulation::ResidenceIn(const FortRandomRectangle& area,std::int32_t region,std::uint32_t quantity) const {
    const auto record=FortTerritoryIslandRecord(world_,region);GameRandom random(std::uint32_t(record[3])*103U);static_cast<void>(placement_.PlaceRandom(random,area,region,quantity,globals_.residence,0));
} //0040e130의 world record 조회/현재 TYPE 시점과 실제 로컬 난수를 기존 완료 배치에 전달한다.
void RawIslandPopulation::Residence(std::int32_t region) const {
    const auto area=Bounds(region);if (!globals_.boss) { Need(hooks_.assertion)(IslandPopulationAssertion::ResidenceAuthority);if (!globals_.boss) return; }ResidenceIn(area,region,1);
} //0046df80은 부모가 준 record6BYTE를 읽지 않고 현재 지도 사각형을 다시 얻는다.
Sid RawIslandPopulation::Candidate(std::uint32_t value) const {
    if (value>std::numeric_limits<std::uint16_t>::max() || value>=pool_.Capacity()) throw std::out_of_range("섬 population의 실제 배치 반환 SID 범위");return {static_cast<std::uint16_t>(value)};
} //0반환의 원문 진단 뒤에는 실제 SID0 위치를 읽을 수 있으므로 임의 조기 반환하지 않는다.
void RawIslandPopulation::ReplaceInitial(const FortRandomRectangle& area,std::int32_t region) const {
    const auto rectangle=Center(area);const auto record=FortTerritoryIslandRecord(world_,region);GameRandom random(std::uint32_t(record[3])*103U);
    if (globals_.fort) random.SetState(RawFortFinishOwner::TimeSeed(Need(hooks_.wallSeconds)()));std::uint32_t result{}; //현재Fort에서만실제벽시각seed로교체한다.
    for (std::uint32_t attempt=0;attempt<10 && !result;++attempt) result=placement_.PlaceRandom(random,rectangle,region,1,globals_.primary,0); //타입은각시도에현재값을재조회한다.
    if (!result) for (std::uint32_t attempt=0;attempt<10 && !result;++attempt) result=placement_.PlaceRandom(random,rectangle,region,1,globals_.fallback,0); //기본10회가모두0일때만대체TYPE를읽는다.
    if (!result) Need(hooks_.assertion)(IslandPopulationAssertion::MissingPlacement);
    const auto position=Need(hooks_.position)(Candidate(result));Need(hooks_.destroy)(Candidate(result),0);const auto created=Need(hooks_.create)(globals_.replacement,0);
    const auto raw=pool_.AllocatedBytes(created);const auto type=raw[10];Put(raw,36,std::bit_cast<std::uint32_t>(Need(hooks_.defaultFrame)(type))); //같은새객체의현재TYPE+118을직접frameDWORD에쓴다.
    if (Need(hooks_.hasHitPoints)(created)) SetRawSquidHitPoints(pool_,created,Need(hooks_.typeHitPoints)(pool_.Slot(created)[10])); //판정뒤현재타입getter를다시읽고비가상HPsetter를쓴다.
    Need(hooks_.setOwner)(created,islands_.Owner(region));Need(hooks_.pop)(created,position[0],position[1],0);
} //0040e2b0의위치snapshot→삭제→새생성→frame/HP→현재owner→Pop순서다.
void RawIslandPopulation::WriteRecord(std::int32_t region,std::span<const std::uint8_t,6> record) const {
    if (!islands_.Exists(region)) Need(hooks_.assertion)(IslandPopulationAssertion::RecordMissing);
    for (std::int32_t y=0;y<target_.YLength();++y) for (std::int32_t x=0;x<target_.XLength();++x) { auto& entry=target_.At(x,y);if (static_cast<std::int32_t>(std::bit_cast<std::int8_t>(entry.island))==region) std::copy(record.begin(),record.end(),entry.record.begin()); } //현재실제폭/높이를y/x순서로읽고일치한6BYTE만쓴다.
    Need(hooks_.refreshProduction)();
} //004324e0의마지막통지는실제일치청크가없어도실행한다.
void RawIslandPopulation::Populate(std::int32_t region) const {
    const auto area=Bounds(region);auto record=FortTerritoryIslandRecord(target_,region);if (!globals_.boss) { Need(hooks_.assertion)(IslandPopulationAssertion::PopulationAuthority);if (!globals_.boss) return; }
    Residence(region);ReplaceInitial(area,region);record[1]|=8U;WriteRecord(region,record);
} //외부효과가currentrecord를바꿔도원문입구의record snapshot에bit0x800만켜서쓴다.
const SidPool& RawIslandPopulation::Pool() const { return pool_; } //상위실제Builder와Pool정체성을검사한다.
}
