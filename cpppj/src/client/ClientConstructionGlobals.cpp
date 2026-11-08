// 건설 상태는 별도 Client owner에 두어 Storage→Gameplay alias와 참조 순환을 만들지 않는다.
#include "client/ClientConstructionGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientBuildReplay.h" // 실제 현재 playing DWORD의 backing owner를 보유한다.
#include "client/ClientSpGlobals.h" //장기 본체가 실제 분산 SP와 난수 owner를 함께 보유한다.
#include "client/ClientPlayerRecords.h" //현재 base를 구성 시 조회하거나 초기화하지 않는다.
#include "client/PriestPlacementAssets.h" // 같은 Storage가 소유한 Canon 진단 주소를 빌린다.
#include "o/Bridge.h" // 원문 최초5인 같은 불변 다리 수명이다.
#include <stdexcept>
#include <utility>
#include <type_traits> //선택한 실제 콜백의 std::function 서명을 보존한다.

namespace netstorm::client { // 실제 owner 정체성과 수명만 결합하며 게임 함수를 실행하지 않는다.
namespace { //현재 큰 효과의 대여와 전체 Runtime pin에만 사용하는 내부 연결 함수다.
// 현재 콜백을 선택한 뒤 실제 외곽 수명을 보유하며 반납/예외 중에도 그 호출의 객체와 함수 사본을 유지한다.
template<class Select> auto CurrentConstructionCall(const std::weak_ptr<const ClientConstructionRuntimeBindings>& owner,Select select) {
    using Function=std::invoke_result_t<Select,const ClientConstructionRuntimeBindings&>; //원래 함수의 반환값/참조/인수 서명을 그대로 유지한다.
    return Function([&owner,select](auto&&... arguments)->decltype(auto) {
        const auto current=owner.lock();if (!current) throw std::logic_error("장기 건설의 선택된 현재 Runtime 효과 미연결");
        const auto pin=current->pin ? current->pin():std::shared_ptr<void>{}; //공개 전 직접 생성/로드는 실제 호출자가 보유하며 공개 뒤에는 전체 Runtime을 pin한다.
        const auto call=select(*current);if (!call) throw std::logic_error("장기 건설의 선택된 실제 효과 본문 누락");
        return call(std::forward<decltype(arguments)>(arguments)...);
    });
}
}
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
// 같은 현재 Session이 반납되지 않은 상태에서 다른 효과 owner로 몰래 교체하지 않는다.
void ClientConstructionGlobals::BindRuntime(const std::shared_ptr<ClientConstructionRuntimeBindings>& runtime) {
    const auto current=runtime_.lock();if (!storage_ || !runtime || (current && current!=runtime)) throw std::invalid_argument("건설 본체의 같은 실제 현재 대여 필요");runtime_=runtime;
}
// 장기 부품만 실제 본체에 고정하고 큰 효과는 각 원문 선택 위치에서 현재 짧은 대여를 조회한다.
RawCampaignConstructionBindings& ClientConstructionGlobals::Body(std::shared_ptr<ClientSpGlobals> sp,std::shared_ptr<ClientPlayerRecords> players,std::shared_ptr<std::array<std::uint8_t,87>> options) {
    if (!storage_ || !sp || !players || !options || options!=gameplay_->BattleOptionsOwner()) throw std::invalid_argument("건설 본체의 같은 실제 SP/Player/옵션 owner 필요");
    if (body_) { if (sp_!=sp || players_!=players || options_!=options) throw std::logic_error("장기 건설 본체의 다른 backing 재결합 금지");return *body_; }
    auto runtime=BindConstructionRuntime(storage_);runtime.players=[records=players] { return records->Records(); }; //원문이 고른 위치까지 Player 할당/조회와 현재 span 선택을 지연한다.
    auto& metadata=storage_->PlacementAssets();auto& aux=*storage_->GameAux(); //동일 Source의 실제 Canon/현재 TYPE이며 새 자산을 만들지 않는다.
    RawCampaignConstructionEffects effects; //큰 효과의 임시 Session 주소는 장기 본체에 저장하지 않는다.
    effects.configInt=CurrentConstructionCall(runtime_,[](const auto& current) { return current.effects.configInt; }); //현재 Config를 실제 선택 시 읽는다.
    effects.wallSeconds=CurrentConstructionCall(runtime_,[](const auto& current) { return current.effects.wallSeconds; }); //선택된 비결정적 시드의 실제 벽시각이다.
    effects.refresh=CurrentConstructionCall(runtime_,[](const auto& current) { return current.effects.refresh; }); //현재 Renderer의 실제 전체 갱신이다.
    effects.negativeQuantity=CurrentConstructionCall(runtime_,[](const auto& current) { return current.effects.negativeQuantity; }); //원문의 수량 진단을 선택 시 실행한다.
    effects.boundsAssertion=CurrentConstructionCall(runtime_,[](const auto& current) { return current.effects.boundsAssertion; }); //현재 Player/Bounds 진단의 복귀 계약이다.
    effects.islandExists=CurrentConstructionCall(runtime_,[](const auto& current) { return current.effects.islandExists; }); //선택된 실제 IslandList 존재 조회다.
    effects.presentation.sendMoney=CurrentConstructionCall(runtime_,[](const auto& current) { return current.effects.presentation.sendMoney; }); //같은 실제6ch 송신을 사용한다.
    effects.presentation.sendReject=CurrentConstructionCall(runtime_,[](const auto& current) { return current.effects.presentation.sendReject; }); //같은 실제2fh 취소 송신이다.
    effects.presentation.format=CurrentConstructionCall(runtime_,[](const auto& current) { return current.effects.presentation.format; }); //원문 비용 안내 서식을 그대로 전달한다.
    effects.presentation.tellOthers=CurrentConstructionCall(runtime_,[](const auto& current) { return current.effects.presentation.tellOthers; }); //현재 타인 안내의 실제 장치다.
    effects.presentation.tellPlayer=CurrentConstructionCall(runtime_,[](const auto& current) { return current.effects.presentation.tellPlayer; }); //현재 Player 안내를 같은 장치로 보낸다.
    effects.presentation.tell=CurrentConstructionCall(runtime_,[](const auto& current) { return current.effects.presentation.tell; }); //로컬 표시 부호화 경계는 기존 실제 콜백이 처리한다.
    RawCampaignConstructionGameplay gameplay; //큰 실제 모듈의 선택 위치에서만 짧은 Runtime 대여를 사용한다.
    gameplay.setOwner=CurrentConstructionCall(runtime_,[](const auto& current) { return current.gameplay.setOwner; }); //사제 가상 소유자 분배를 유지한다.
    gameplay.request.mayPlace=CurrentConstructionCall(runtime_,[](const auto& current) { return current.gameplay.request.mayPlace; }); //현재 전체 MayPlace를 선택한다.
    gameplay.request.displace=CurrentConstructionCall(runtime_,[](const auto& current) { return current.gameplay.request.displace; }); //실제 발자국 밀어내기다.
    gameplay.request.cancel=CurrentConstructionCall(runtime_,[](const auto& current) { return current.gameplay.request.cancel; }); //실패 뒤 실제 DropIsland를 실행한다.
    gameplay.confirm.findBuilder=CurrentConstructionCall(runtime_,[](const auto& current) { return current.gameplay.confirm.findBuilder; }); //현재 실제 Graph 기준점이다.
    gameplay.confirm.recorderActive=[replay=replay_] { return CurrentBuildReplayIsLabel(replay); }; //기록기 전역은 Session 없는 동일 장기 owner에서 매번 조회한다.
    gameplay.confirm.record=[replay=replay_](o::Sid sid) { NotifyCurrentBuildReplay(replay,sid.value); }; //같은 실제 현재 재생 객체를 선택하며 초기화하지 않는다.
    gameplay.notice.moveBuilder=CurrentConstructionCall(runtime_,[](const auto& current) { return current.gameplay.notice.moveBuilder; }); //실제 Builder 생성/이동이다.
    gameplay.notice.findPriest=CurrentConstructionCall(runtime_,[](const auto& current) { return current.gameplay.notice.findPriest; }); //현재 소유 사제를 조회한다.
    gameplay.notice.containedType=CurrentConstructionCall(runtime_,[](const auto& current) { return current.gameplay.notice.containedType; }); //현재 사제 이동 Form TYPE다.
    gameplay.notice.removeContained=CurrentConstructionCall(runtime_,[](const auto& current) { return current.gameplay.notice.removeContained; }); //같은 실제 가상 삭제로 종속을 제거한다.
    gameplay.notice.immobile=CurrentConstructionCall(runtime_,[](const auto& current) { return current.gameplay.notice.immobile; }); //현재 사제의 실제 이동 불가 판정이다.
    gameplay.notice.approach=CurrentConstructionCall(runtime_,[](const auto& current) { return current.gameplay.notice.approach; }); //현재 Graph/발자국 접근점이다.
    gameplay.notice.tryMove=CurrentConstructionCall(runtime_,[](const auto& current) { return current.gameplay.notice.tryMove; }); //실제 사제 경로 시작의 bool이다.
    gameplay.notice.start=CurrentConstructionCall(runtime_,[](const auto& current) { return current.gameplay.notice.start; }); //현재 시각의 실제 건설 시작이다.
    const o::PlayerGeyserBoundsState bounds{storage_->ownerMode.battle,storage_->links.mission,storage_->ownerMode.challenge,(*options)[6],aux.PuzzlePieceType(),storage_->geyserPlayerDirections,&metadata.geometry.decoderDiagnostics}; //모든 참조는 같은 장기 물리/옵션에서만 빌린다.
    auto body=std::make_unique<RawCampaignConstructionBindings>(std::move(runtime),storage_->Reward(),sp->Store(),storage_->RewardState(),metadata.frames,storage_->chunks,state_,bounds,storage_->recentGeysers,storage_->sidLists.geysers,std::move(effects),std::move(gameplay));
    sp_=std::move(sp);players_=std::move(players);options_=std::move(options);body_=std::move(body);return *body_; //완전 구성 성공 뒤에만 backing과 본체를 함께 공개한다.
}
// 구성 전의 빈 본체를 성공 대용으로 만들지 않는다.
RawCampaignConstructionBindings& ClientConstructionGlobals::Body() { if (!body_) throw std::logic_error("Client의 장기 건설 본체 미구성");return *body_; }
}
