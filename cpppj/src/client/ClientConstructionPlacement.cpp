// 원문49b510의전체배치본체와상태를같은Client수명에연결한다.
#include "client/ClientConstructionGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/PriestPlacementAssets.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { //새World/Session/Player배열없이실제장기물리만빌린다.
namespace { //섬기록의비정렬DWORD는현재바이트에서읽는다.
// 현재섬기록의개수/제한/존재를소비하는정확네바이트만조합한다.
std::uint32_t PlacementDword(std::span<const std::uint8_t> bytes,std::size_t offset) {
    if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("장기 배치의 현재 섬 DWORD 물리 부족");
    std::uint32_t value{};for (std::size_t i=0;i<4;++i) value|=std::uint32_t(bytes[offset+i])<<(i*8);return value; //인접필드를섞지않고little endian으로읽는다.
}
}
// 실제PE의Graph활성/useAlliances초기1과기존기준점TYPE를한번보유하고나머지는현재owner를빌린다.
void ClientConstructionGlobals::BindPlacementGlobals() {
    if (placementBound_) return;
    if (!storage_ || !gameplay_) throw std::logic_error("게임 배치의 실제 장기 owner 미결합");
    auto& aux=*storage_->GameAux();placementState_.geometry=storage_->PlacementAssets().geometry;
    placementState_.placement.sharedForcePlacement=&aux.MissionSettings()[1];placementState_.collision.sharedIgnoredGenus=&aux.ConstructionDisplaceGenus();placementState_.collision.sharedServer=&gameplay_->Server();
    placementState_.permission.useAlliances=1;placementAnchor_.graphReady=1;placementAnchor_.fenceType=90;placementAnchor_.windArcherType=140;placementAnchor_.sharedFullDeck=&gameplay_->FullDeck();
    placementAnchor_.graphAssertion=[storage=storage_](auto expression,auto file,auto line) { storage->AssertDisplay(expression,file,line); }; //현재Client의실제진단/정상복귀를사용하고Session주소를남기지않는다.
    placementBound_=true;
}
// 현재커서/요청의같은거부상태를반환하며별도초기화는하지않는다.
o::CanonPlacementPipelineState& ClientConstructionGlobals::PlacementState() { if (!placementBound_) throw std::logic_error("장기 게임 배치 상태 미결합");return placementState_; }
// 실제Graph/전체덱참조를기준점과후속도착조회가함께빌린다.
o::PlayerPlacementAnchorState& ClientConstructionGlobals::PlacementAnchor() { if (!placementBound_) throw std::logic_error("장기 게임 배치 기준점 미결합");return placementAnchor_; }
// 첫전역생성뒤목록내용과주소를World교체에서유지한다.
o::SquidPostPopList& ClientConstructionGlobals::PlacementAdditional() { if (!placementBound_) throw std::logic_error("장기 게임 배치 추가목록 미결합");return placementAdditional_; }
// 전체기하/픽셀/충돌/지형/관계/기준점모듈은같은Pool/장부/현재finder를직접참조한다.
o::RawCanonPlacementPipeline& ClientConstructionGlobals::PlacementPipeline() {
    static_cast<void>(PlacementState());
    if (!placementPipeline_) {
        auto& metadata=storage_->PlacementAssets();
        placementPipeline_=std::make_unique<o::RawCanonPlacementPipeline>(storage_->pool,storage_->hash,
            o::CanonPlacementPipelineData{storage_->types,metadata.frames,storage_->shapes,metadata.hotspots,storage_->spots,storage_->hash.Entries(0),placementRegions_},
            storage_->postPopState,placementAdditional_,gameplay_->finder,placementState_);
    }
    return *placementPipeline_;
}
// 기존게임Caller가공급하던현재전역을같은owner에서조회하고전체원문파이프라인을실행한다.
bool ClientConstructionGlobals::MayPlace(const o::CanonPlacementQuery& query) {
    if (!players_) throw std::logic_error("장기 게임 배치의 실제 Player owner 미결합");
    auto& state=PlacementState();state.placement.localPlayer=std::bit_cast<std::int32_t>(gameplay_->player.battleOwner);state.permission.editor=gameplay_->world.editor;
    state.permission.graphReady=placementAnchor_.graphReady;state.permission.alliances=players_->Alliances();state.anchor=placementAnchor_;
    state.collision.clientMode=gameplay_->Server() ? 1U:0U;state.collision.ignoredGenus=storage_->GameAux()->ConstructionDisplaceGenus();state.geometry.patternTypes=state_.confirm.CurrentPatternTypes();
    state.relations.islandCount=PlacementDword(storage_->islands.records,0);
    for (std::size_t i=0;i<placementRegions_.size();++i) placementRegions_[i]={PlacementDword(storage_->islands.records,4+i*32),PlacementDword(storage_->islands.records,8+i*32)}; //현재실제100행의제한/존재만읽는다.
    return PlacementPipeline().MayPlace(query);
}
}
