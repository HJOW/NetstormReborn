// 건설 상태는 별도 Client owner에 두어 Storage→Gameplay alias와 참조 순환을 만들지 않는다.
#include "client/ClientConstructionGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientBuildReplay.h" // 실제 현재 playing DWORD의 backing owner를 보유한다.
#include "client/PriestPlacementAssets.h" // 같은 Storage가 소유한 Canon 진단 주소를 빌린다.
#include "o/Bridge.h" // 원문 최초5인 같은 불변 다리 수명이다.
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 실제 owner 정체성과 수명만 결합하며 게임 함수를 실행하지 않는다.
void ClientConstructionGlobals::Bind(std::shared_ptr<RawSurfaceStorage> storage,std::shared_ptr<ClientGameplayGlobals> gameplay,std::shared_ptr<ClientBuildReplayGlobals> replay) {
    if (!storage || !gameplay || !storage->UsesNativeTransmissionDevices() || storage->GameAux()!=gameplay->GameAux() || storage->Authority()!=gameplay->Authority()) throw std::invalid_argument("건설 상태의 같은 Native Client 전역/물리 계약 오류");
    if (storage_ && !Matches(storage,gameplay,replay)) throw std::logic_error("건설 상태의 다른 Client owner 재결합 금지");
    auto& metadata=storage->PlacementAssets(); // 이미 로딩한 같은 자산만 빌리며 World/Player/난수를 만들지 않는다.
    storage_=std::move(storage);gameplay_=std::move(gameplay);replay_=std::move(replay);
    auto& aux=*storage_->GameAux(); // 상태의 모든 raw 주소는 앞의 실제 장기 owner가 보증한다.
    state_.place.sharedAuthority=state_.clear.sharedAuthority=state_.notice.sharedAuthority=state_.geyser.sharedBoss=&gameplay_->Authority()->boss;
    state_.place.sharedServer=&gameplay_->Authority()->server;state_.place.sharedLocalPlayer=state_.notice.sharedLocalPlayer=state_.request.sharedLocalPlayer=state_.command.sharedLocalPlayer=&gameplay_->player.battleOwner;
    state_.place.sharedPlayerKnown=&gameplay_->player.playerKnown;state_.place.sharedBridgeType=state_.clear.sharedBridgeType=&storage_->links.bridgeType;
    state_.place.sharedNoIslandType=state_.request.sharedNoIslandType=state_.geyser.sharedNoIslandType=&storage_->links.noIslandType;state_.request.sharedIslandType=&storage_->links.islandType;
    state_.request.sharedEditor=state_.geyser.sharedFort=&storage_->ownerMode.fort;state_.request.sharedDisplaceGenus=&aux.ConstructionDisplaceGenus();state_.command.sharedBlocked=&aux.ConstructionBlocked();
    state_.place.sharedNuggetType=&aux.NuggetType();state_.place.sharedBridgeLife=&o::kBridgeCrackLife;state_.confirm.sharedFullDeck=&gameplay_->FullDeck();
    state_.notice.sharedDaisType=&aux.DaisType();state_.notice.sharedPriestType=&aux.PriestType();state_.notice.sharedHistory=&gameplay_->Camera().ConstructionHistory();
    state_.geyser.sharedGeyserType=&aux.GeyserType();state_.geyser.sharedEmptyGeyserType=&aux.EmptyGeyserType();state_.geyser.sharedIsleType=&aux.TerrainType();state_.geyser.sharedFirstQuantity=&aux.FirstLocalGeyserQuantity();state_.geyser.sharedLastQuantity=&aux.LastLocalGeyserQuantity();
    state_.place.patternTypesRead=[physical=storage_,owner=storage_->GameAux()] { return std::array<std::uint32_t,8>{owner->PuzzlePieceType(),physical->links.bridgeType,physical->links.islandType,physical->links.noIslandType,owner->ThunderCannonType(),owner->RainCannonType(),owner->WindArcherType(),owner->WindBlockerType()}; };
    state_.confirm.patternTypesRead=state_.request.patternTypesRead=state_.clear.patternTypesRead=state_.place.patternTypesRead;
    state_.command.chargeBlockedRead=state_.place.chargeBlockedRead=[owner=gameplay_] { return owner->world.missionPost; }; // 소유자는 별도 Client 객체이므로 Gameplay를 보유해도 역참조 순환이 없다.
    state_.confirm.sharedRecorderEnabled=replay_ ? &replay_->playing:nullptr;
    for (auto* diagnostics:{&state_.request.decoderDiagnostics,&state_.confirm.decoderDiagnostics,&state_.place.decoderDiagnostics,&state_.clear.decoderDiagnostics}) *diagnostics=&metadata.geometry.decoderDiagnostics; // 같은 장기 Canon 진단을 모든 원문 소비자가 빌린다.
}
// 비교는 조회뿐이며 누락된 Source/World나 새로운 상태를 만들지 않는다.
bool ClientConstructionGlobals::Matches(const std::shared_ptr<RawSurfaceStorage>& storage,const std::shared_ptr<ClientGameplayGlobals>& gameplay,const std::shared_ptr<ClientBuildReplayGlobals>& replay) const { return storage_ && storage_==storage && gameplay_==gameplay && replay_==replay; }
RawCampaignConstructionState& ClientConstructionGlobals::State() { if (!storage_) throw std::logic_error("건설 상태의 실제 Client owner 미결합");return state_; } // 결합 전에는 임시 전역으로 진행하지 않는다.
const RawCampaignConstructionState& ClientConstructionGlobals::State() const { if (!storage_) throw std::logic_error("건설 상태의 실제 Client owner 미결합");return state_; } // 같은 장기 상태만 조회한다.
}
