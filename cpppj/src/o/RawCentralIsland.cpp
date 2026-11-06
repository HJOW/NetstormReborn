// 일반 중앙섬은 owner0/원점 배치이며 참가자별 전투섬과 다른 원문 관문을 사용한다.
#include "o/RawCentralIsland.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 지역0 생성 뒤의 실제 군도/거주 구역/Graph 효과를 생략하지 않는다.
namespace { // 선택된 하위만 해당 원문 위치에서 요구한다.
template<class F> F CentralNeed(const F& hook) { if (!hook) throw std::logic_error("중앙섬 생성의 선택된 실제 장치 미연결");return hook; } // 콜백 교체 중에도 현재 callable을 보유한다.
float CentralCell(std::int32_t chunk) { return static_cast<float>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(chunk)<<4)); } //0040d900의 DWORD 감김16배 뒤 float32 저장이다.
}
RawCentralIsland::RawCentralIsland(const SidPool& pool,ChunkMap& world,RawIslandRuntime& islands,std::span<const RiftTypeFrames> frames,std::uint32_t& puzzlePiece,CentralIslandGlobals globals,CentralIslandHooks hooks)
    :pool_(pool),world_(world),islands_(islands),frames_(frames),puzzlePiece_(puzzlePiece),globals_(globals),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078 || &islands.Pool()!=&pool) throw std::invalid_argument("중앙섬의 실제10.78 Pool 불일치"); }
// 원문 네 진단→현재 중심→패턴/지형→조건부 거주 구역/군도 분리→Graph0의 순서다.
void RawCentralIsland::Build(std::span<const std::uint8_t,6> record) const {
    RawIslandBuilderState state{{-1,-1,-1,-1},0,hooks_.decoderDiagnostics};
    if (!globals_.battle) CentralNeed(hooks_.assertion)("inBattleMode",0xea);
    if (record[2]) CentralNeed(hooks_.assertion)("terr.xChunkCoord == 0 && terr.yChunkCoord == 0",0xeb);
    if (islands_.Count()) CentralNeed(hooks_.assertion)("islandList.getTotal() == 0",0xec);
    if (state.owner) CentralNeed(hooks_.assertion)("playerId == INVALID_PLAYER_ID",0xed);
    state.bounds=CentralNeed(hooks_.placement)(record);
    globals_.position[0]=static_cast<float>((static_cast<double>(CentralCell(state.bounds.right))+CentralCell(state.bounds.left))*0.5);
    globals_.position[1]=static_cast<float>((static_cast<double>(CentralCell(state.bounds.bottom))+CentralCell(state.bounds.top))*0.5);
    RawIslandBuilder builder(world_,world_,islands_,frames_,puzzlePiece_,state,hooks_.builderAssertion);
    const auto island=builder.Place(record,static_cast<std::uint32_t>(globals_.unassignedTerritory),true);CentralNeed(hooks_.generate)(state,static_cast<std::int32_t>(island)); //00540a30의 현재127을 사용하며 builder owner0으로 대신하지 않는다.
    if (globals_.boss && !globals_.terrainEnabled) CentralNeed(hooks_.residence)(static_cast<std::int32_t>(island));
    if (globals_.archipelago) CentralNeed(hooks_.partition)();CentralNeed(hooks_.graph)(false);
}
}
