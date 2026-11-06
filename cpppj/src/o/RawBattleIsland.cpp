// 원문0046e2c0의 고정20영역 선택과 기존 builder/전체 하위 수명을 접합한다.
#include "o/RawBattleIsland.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 새 맵 snapshot이나 자기 Fort 대체 로드를 만들지 않는다.
namespace { // 선택된 실제 callable만 필요해진 위치에서 복사해 호출한다.
template<class F> F BattleIslandNeed(const F& hook) { if (!hook) throw std::logic_error("전투섬 생성의 선택된 실제 장치 미연결");return hook; } // 진단/파일 실패 전의 부분 월드 효과를 보존한다.
}
RawBattleIsland::RawBattleIsland(const SidPool& pool,ChunkMap& world,RawIslandRuntime& islands,std::span<const RiftTypeFrames> frames,std::uint32_t& puzzlePiece,BattleIslandHooks hooks)
    :pool_(pool),world_(world),islands_(islands),frames_(frames),puzzlePiece_(puzzlePiece),hooks_(std::move(hooks)) { if (&islands.Pool()!=&pool || pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("전투섬의 실제10.78 Pool 불일치"); } // CD00490fa0는 후속 판본 전체 복원 때 별도로 접합한다.
// 원문 지역 builder만 사용하며 현재 Player/섬 번호와 실제 객체 생성 시점을 바꾸지 않는다.
void RawBattleIsland::Build(std::uint32_t owner) const {
    RawIslandBuilderState state{{-1,-1,-1,-1},owner,hooks_.decoderDiagnostics}; // ctor의 invalid bounds 뒤 SetWorld는 지도/owner만 바꾼다.
    if (!state.owner) BattleIslandNeed(hooks_.assertion)(0x10f);
    std::array<std::uint8_t,120> records{};BattleIslandNeed(hooks_.territory)(state.owner,records);std::int32_t selected{};
    for (std::int32_t index=0;index<20;++index) { // 레코드의 flags BYTE+1에서0x04 비트가 켜진 첫 영역을 찾고 없으면 원문처럼0번을 유지한다.
        if (records[static_cast<std::size_t>(index)*6+1]&4U) { selected=index;break; }
    }
    const std::span<const std::uint8_t,6> record(records.data()+static_cast<std::size_t>(selected)*6,6);state.bounds=BattleIslandNeed(hooks_.placement)(state.owner,record);
    RawIslandBuilder builder(world_,world_,islands_,frames_,puzzlePiece_,state,hooks_.builderAssertion);const auto island=builder.Place(record,static_cast<std::uint32_t>(selected),true);
    BattleIslandNeed(hooks_.region)(state.owner,island);BattleIslandNeed(hooks_.generate)(state,static_cast<std::int32_t>(island));
    if (BattleIslandNeed(hooks_.boss)()) BattleIslandNeed(hooks_.objects)(state.owner,selected,selected+1);
    BattleIslandNeed(hooks_.graph)(false);
}
}
