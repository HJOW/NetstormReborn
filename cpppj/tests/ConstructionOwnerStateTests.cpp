// 현재 상위 검증과 Client 상태 수명/참조 순환을 패킷·게임 실행 없이 확인한다.
#include "ConstructionSupport.h"
#include "DudeSpawnSupport.h"
#include "client/ClientConstructionGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientBuildReplay.h"
#include "o/RawCanonPlacementCollision.h"

using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace { // 실제 부품과 소유권 경계만 구성하며 선택되지 않은 전송은 실패로 드러낸다.
// 성공 관찰로 대체하지 않을 외부 게임/전송 경계다.
template<class Result,class... Arguments> std::function<Result(Arguments...)> OwnerForbidden() { return [](Arguments...)->Result { throw std::logic_error("건설 상태 검사에서 선택 금지된 효과"); }; }
// 실제 계약 거부만 확인하며 다른 종류의 예외를 통과로 간주하지 않는다.
template<class Function> bool OwnerRejected(Function&& function) { try { function();return false; }catch (const std::logic_error&) { return true; } }
// Source/현재 GameAux/Authority는 같은 실제 owner를 사용하고 Native 장치의 실행은 금지한다.
std::shared_ptr<RawSurfaceStorage> OwnerStorage(const std::shared_ptr<ClientGameplayGlobals>& gameplay) {
    auto result=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game,gameplay->Authority(),gameplay->GameAux());result->BindTransmissionDevices({OwnerForbidden<void,std::span<std::uint8_t>>(),OwnerForbidden<bool,std::int32_t>(),OwnerForbidden<void,std::int32_t,std::span<std::uint8_t>>()});return result;
}
// 독립 typed World에 명시 현재 getter를 연결해 상위 검증이 raw 사본을 읽는 회귀를 검출한다.
struct OwnerBindingScene {
    std::vector<RiftTypeRecord> types{188}; // 합성 TYPE의 실제 물리 슬롯이며 객체/패킷 생성은 하지 않는다.
    std::vector<RiftTypeFrames> frames{188,RiftTypeFrames({})}; // 같은188TYPE 프레임 표다.
    std::vector<SquidDisplayShape> shapes{188}; // 빈 SHP 슬롯의 명시 입력이다.
    RawSurfaceWorld world{OriginalEdition::Patch1078,types,frames,shapes,Links(),97}; // 유일한 Pool/Hash/Host를 소유한다.
    RawWorldCore core{world.Core()}; // 같은 실제 부품 참조만 보관한다.
    GameRandom random{123}; // 수량0 관문에서 소비되지 않아야 하는 실제 난수다.
    ScrambledSpStore sp{random,[] { return 1U; }}; // 비용이 선택되지 않는 같은 SP 몸체다.
    SquidRewardState reward; // 같은 HP/자원 상태다.
    SquidReward hp{core.pool,core.types,core.bookkeeping,reward,&sp}; // 생성 없이 실제 결합 정체성을 검증한다.
    std::vector<PriestPlainCanonType> canon{188}; // 모듈보다 오래 사는 같은 Canon 표다.
    std::array<std::uint8_t,1620> players{}; // 모든 Player가 비활성인 실제 물리 입력이다.
    BattlePacketPeers peers; // 방송 객체 형식만 빌리며 실제 방송은 금지 함수로 덮는다.
    std::uint32_t localOwner{1},dais{165}; // 현재 Local/Dais 전역의 backing이다.
    bool server{true},boss{true},mission{}; // 선택된 현재 모드 backing이다.
    std::uint8_t mapSize{}; // 현재 간격 옵션이다.
    std::uint32_t piece{107},configs{}; // 실제 역할과 생성 입구의 설정 조회 수다.
    std::array<std::uint32_t,9> directions{0,4,0,6,2,7,3,5,1}; // 실제 Player 배치 방향 입력이다.
    ChunkMap chunks; // 동일 청크 지도다.
    RawBattleBroadcast broadcast{OriginalEdition::Patch1078,players,peers,localOwner,server}; // Native 실행 검사가 아닌 typed 명시 대여다.
    RawCampaignConstructionState state; // raw fallback과 current backing을 일부러 다르게 둔다.
    SquidPostPopList recent,all; // 전용 최근5/전체 목록을 분리한다.
    std::unique_ptr<RawCampaignConstructionBindings> body; // 앞의 실제 참조보다 먼저 반납한다.
    static BridgeConnectState Links() { BridgeConnectState value;value.bridgeType=82;value.islandType=94;value.stalagType=95;value.connectorType=155;value.noIslandType=157;return value; } // 기존 실제 역할 번호다.
    // 준비 사본이 current 참조를 덮거나 상위 검증에서 사용되지 않아야 한다.
    OwnerBindingScene() {
        recent.entries={1,2,3,4,5};recent.count=3;state.place.noIslandType=70;state.geyser.noIslandType=71;state.place.bridgeType=72;state.clear.bridgeType=73;state.place.localPlayer=8;state.notice.localPlayer=7;state.place.authority=false;state.geyser.fort=true;state.notice.daisType=100;state.confirm.patternTypes.fill(0);state.clear.patternTypes.fill(1);state.place.patternTypes.fill(2);
        state.place.sharedNoIslandType=state.geyser.sharedNoIslandType=&core.bridgeState.noIslandType;state.place.sharedBridgeType=state.clear.sharedBridgeType=&core.bridgeState.bridgeType;state.place.sharedLocalPlayer=state.notice.sharedLocalPlayer=&localOwner;state.place.sharedAuthority=state.clear.sharedAuthority=state.notice.sharedAuthority=state.geyser.sharedBoss=&boss;state.geyser.sharedFort=&core.ownerMode.fort;state.notice.sharedDaisType=&dais;
        state.place.patternTypesRead=[this] { return std::array<std::uint32_t,8>{piece,core.bridgeState.bridgeType,core.bridgeState.islandType,core.bridgeState.noIslandType,131,129,140,142}; };state.confirm.patternTypesRead=state.clear.patternTypesRead=state.place.patternTypesRead;
        RawCampaignConstructionEffects effects;effects.configInt=[this](auto,auto) { ++configs;return 1; };effects.wallSeconds=OwnerForbidden<double>();effects.refresh=OwnerForbidden<void>();effects.negativeQuantity=OwnerForbidden<void>();effects.boundsAssertion=OwnerForbidden<void,PlayerGeyserBoundsAssertion>();auto runtime=BindConstructionRuntime(world);runtime.players=[this] { return std::span<std::uint8_t>(players); };runtime.create=OwnerForbidden<Sid,std::uint32_t,std::uint32_t>();runtime.pop=OwnerForbidden<void,Sid,float,float,std::uint32_t>();runtime.destroy=OwnerForbidden<bool,Sid,std::uint32_t>();runtime.broadcast=OwnerForbidden<void,std::span<std::uint8_t>>();
        body=std::make_unique<RawCampaignConstructionBindings>(std::move(runtime),hp,sp,reward,canon,chunks,std::span<std::uint8_t>{},broadcast,state,PlayerGeyserBoundsState{core.ownerMode.battle,mission,core.ownerMode.challenge,mapSize,piece,directions},recent,all,std::move(effects));
    }
};
}

// 상위 constructor/수량0/전Player 비활성 생성 입구가 같은 current 상태를 사용하고 wire를 만들지 않는다.
TEST_CASE(ConstructionOwner_BindingsValidateCurrentValuesWithoutPacketEffects) {
    OwnerBindingScene scene;scene.body->GenerateLocalGeysers();CHECK(scene.configs==1 && scene.body->LastPacket().empty());scene.core.bridgeState.noIslandType=100;scene.localOwner=2;scene.body->PopulateGeysers(0,scene.random,{},{});CHECK(scene.recent.count==0 && scene.random.State()==123 && scene.body->LastPacket().empty());scene.core.ownerMode.fort=true;scene.boss=false;scene.recent.count=3;scene.body->PopulateGeysers(0,scene.random,{},{});CHECK(scene.recent.count==3);CHECK(OwnerRejected([&] { scene.body->ConfirmSupport({70,0,0,0}); }));CHECK(scene.body->LastPacket().empty());
    std::uint32_t wrong=101;scene.state.geyser.sharedNoIslandType=&wrong;CHECK(OwnerRejected([&] { scene.body->PopulateGeysers(0,scene.random,{},{}); }));CHECK(scene.recent.count==3 && scene.random.State()==123);
}

// 같은 Source 재결합은 상태를 유지하며 실제 참조 owner들이 사라질 때 참조 순환 없이 함께 반납된다.
TEST_CASE(ConstructionOwner_RetainsActualBackingAndReleasesWithoutCycles) {
    auto gameplay=std::make_shared<ClientGameplayGlobals>();auto storage=OwnerStorage(gameplay);auto replay=std::make_shared<ClientBuildReplayGlobals>();auto owner=std::make_shared<ClientConstructionGlobals>();CHECK(OwnerRejected([&] { owner->State(); }));owner->Bind(storage,gameplay,replay);auto* state=&owner->State();state->notice.historyCounter=37;owner->Bind(storage,gameplay,replay);CHECK(&owner->State()==state && state->notice.historyCounter==37 && storage->Generation()==0 && !storage->Borrowed());
    storage->BindPostPopDevices({OwnerForbidden<void,std::uint32_t,Sid,bool>(),[gameplay] { return gameplay->player.battleOwner; },OwnerForbidden<void>()}); // 실제 Storage→Gameplay 방향의 보유 관계도 포함한다.
    auto other=std::make_shared<ClientBuildReplayGlobals>();CHECK(OwnerRejected([&] { owner->Bind(storage,gameplay,other); }));CHECK(state->notice.historyCounter==37);other.reset();
    std::weak_ptr<RawSurfaceStorage> physicalWatch=storage;std::weak_ptr<ClientGameplayGlobals> gameplayWatch=gameplay;std::weak_ptr<ClientBuildReplayGlobals> replayWatch=replay;gameplay->player.battleOwner=2;gameplay->world.missionPost=true;replay->playing=1;storage.reset();gameplay.reset();replay.reset();CHECK(!physicalWatch.expired() && !gameplayWatch.expired() && !replayWatch.expired());CHECK(state->place.LocalPlayer()==2 && state->place.chargeBlockedRead() && state->confirm.RecorderEnabled());CHECK(state->place.CurrentPatternTypes()[0]==107);owner.reset();CHECK(physicalWatch.expired() && gameplayWatch.expired() && replayWatch.expired());
}

// Session 없이 현재 역할8개와 비용/모드 참조를 읽고 fresh helper가 역할을 되돌리지 않는다.
TEST_CASE(ConstructionOwner_StateReadsLongRolesBeforeAnySession) {
    auto gameplay=std::make_shared<ClientGameplayGlobals>();auto storage=OwnerStorage(gameplay);ClientConstructionGlobals owner;owner.Bind(storage,gameplay,{});auto& aux=*gameplay->GameAux();auto& state=owner.State();aux.PuzzlePieceType()=108;aux.ThunderCannonType()=130;aux.RainCannonType()=128;aux.WindBlockerType()=143;storage->links.noIslandType=100;gameplay->player.battleOwner=2;gameplay->world.missionPost=true;const std::array<std::uint32_t,8> expected{108,82,94,100,130,128,140,143};CHECK(state.place.CurrentPatternTypes()==expected && state.confirm.CurrentPatternTypes()==expected && state.clear.CurrentPatternTypes()==expected && state.request.CurrentPatternTypes()==expected);CHECK(state.place.LocalPlayer()==2 && state.command.LocalPlayer()==2 && state.place.NoIslandType()==100 && state.geyser.NoIslandType()==100 && state.place.chargeBlockedRead());aux.ResetForGeneralWorld();CHECK(state.place.CurrentPatternTypes()==expected && storage->Generation()==0 && !storage->Borrowed());
}

// 이전 후보의 지형 효과가 server를 바꾸면 다음 후보의 client SID 생략은 현재값을 읽는다.
TEST_CASE(ConstructionOwner_CollisionReadsCurrentServerBetweenCandidates) {
    SidPool pool(OriginalEdition::Patch1078,32768,false);SquidHash hash;std::vector<RiftTypeRecord> types(188);types[82].footX=types[82].footY=types[83].footX=types[83].footY=1;bool server=true;CanonPlacementCollisionState state;state.clientMode=0;state.sharedServer=&server;CanonPlacementPreviewState preview;std::vector<std::uint16_t> visited;
    for (const auto sid:{50,51}) { // 명시한 같은 칸의 실제 raw 후보 둘만 준비한다.
        auto raw=InputRaw(pool,Sid{static_cast<std::uint16_t>(sid)});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=83;Put(raw,14,Bits(20.F));Put(raw,18,Bits(21.F));if (sid==50) Put(raw,4,51,2);
    }
    hash.Bucket(0,20,21)=50;RawCanonPlacementCollision collision(pool,hash,types,state,preview,{OwnerForbidden<bool,const CanonPlacementQuery&,const std::function<bool(SquidSearchArea)>&>(),[&](auto,auto sid) { visited.push_back(sid.value);server=!server; }});CHECK(!collision.InspectArea({82,0,20,21,0,1,0},true,{20,21,20,21}));CHECK((visited==std::vector<std::uint16_t>{50}));
    visited.clear();server=false;state.clientMode=1;preview.blocked.fill(0);InputRaw(pool,Sid{50})[40]=1;CHECK(collision.InspectArea({82,0,20,21,0,1,0},true,{20,21,20,21}));CHECK((visited==std::vector<std::uint16_t>{50,51}) && std::count(preview.blocked.begin(),preview.blocked.end(),std::uint8_t{1})==1);
}

// 현재 삭제 장치가 없는 저장소는 일반 자산 삭제를 성공으로 대신하지 않는다.
TEST_CASE(ConstructionOwner_MissingCurrentDeletionDeviceRejectsBeforeWorldCreation) {
    auto gameplay=std::make_shared<ClientGameplayGlobals>();auto storage=OwnerStorage(gameplay);
    CHECK(OwnerRejected([&] { static_cast<void>(storage->DestroyAsset(Sid{5},0x1000)); }));
    CHECK(storage->Generation()==0 && !storage->Borrowed());
}

// 건설 실행기는 짧은 World 반납 뒤에도 실제 현재 장치의 거부/수락/예외를 보존하고 새 World를 만들지 않는다.
TEST_CASE(ConstructionOwner_RuntimeDeletionSurvivesWorldReturnAndPreservesResult) {
    auto gameplay=std::make_shared<ClientGameplayGlobals>();auto storage=OwnerStorage(gameplay);GameRandom random(123);
    storage->BindSerializationGlobals(std::make_shared<FortSerializationGlobals>()); //Native World의 실제 공통 로딩 깊이 owner를 준비한다.
    auto sp=std::make_shared<ScrambledSpStore>(random,[] { return 1U; }); //같은 물리 HP/보상이 요구하는 실제 SP 몸체이며 이 검사에서 소비하지 않는다.
    storage->BindRewardDevices({sp,std::shared_ptr<const std::uint32_t>(gameplay,&gameplay->world.editor),OwnerForbidden<void>(),OwnerForbidden<void,std::uint32_t,std::int32_t>()});
    storage->BindPostPopDevices({OwnerForbidden<void,std::uint32_t,Sid,bool>(),[gameplay] { return gameplay->player.battleOwner; },OwnerForbidden<void>()});
    storage->BindDeletionDevices({OwnerForbidden<void,std::uint32_t,std::uint32_t,std::uint32_t>(),OwnerForbidden<void,Sid>()}); //선택되지 않은 큰 효과는 계속 금지하며 실제 필수 장치 계약만 채운다.
    std::uint32_t calls=0;bool accepted=false,failed=false; //동일 현재 장치의 결과를 각 호출 사이에 변경한다.
    storage->BindResourceDevices({[&]()->GameRandom& { return random; },OwnerForbidden<void,Sid>(),OwnerForbidden<float,Sid,std::uint32_t,std::uint32_t,float>(),OwnerForbidden<void,Sid>(),[&](Sid sid,std::uint32_t flags) {
        CHECK(sid==Sid{5} && flags==0x1000);++calls;
        if (failed) throw std::runtime_error("선택된 실제 삭제 효과 실패");return accepted;
    },OwnerForbidden<void,Sid,std::uint32_t>()});
    std::optional<RawConstructionRuntime> runtime; //World를 반납한 뒤 장기 물리와 삭제 장치만 유지한다.
    {
        RawSurfaceWorld world(OriginalEdition::Patch1078,storage->types,storage->frames,storage->shapes,OwnerBindingScene::Links(),97,nullptr,storage);
        runtime.emplace(BindConstructionRuntime(world));CHECK(storage->Borrowed() && calls==0);
    }
    const auto generation=storage->Generation();std::weak_ptr<RawSurfaceStorage> physicalWatch=storage;
    CHECK(!storage->Borrowed() && !runtime->destroy(Sid{5},0x1000) && calls==1);
    accepted=true;CHECK(runtime->destroy(Sid{5},0x1000) && calls==2);
    failed=true;bool thrown=false;try { static_cast<void>(runtime->destroy(Sid{5},0x1000)); }catch (const std::runtime_error&) { thrown=true; }
    CHECK(thrown && calls==3 && storage->Generation()==generation && !storage->Borrowed() && random.State()==123);
    storage.reset();CHECK(!physicalWatch.expired());runtime.reset();CHECK(physicalWatch.expired());
}
