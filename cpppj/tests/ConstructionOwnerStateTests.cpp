// 현재 상위 검증과 Client 상태 수명/참조 순환을 패킷·게임 실행 없이 확인한다.
#include "ConstructionSupport.h"
#include "DudeSpawnSupport.h"
#include "client/ClientConstructionGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientBuildReplay.h"
#include "client/ClientSpGlobals.h" //장기 본체의 실제 SP backing과 초기화 지연을 확인한다.
#include "client/ClientPlayerRecords.h" //본체 구성 전 Player base0을 유지하는 실제 할당 owner다.
#include "o/RawCanonPlacementCollision.h"
#include "o/ConfigInterface.h" //NoWorld 장치가 같은 실제 Config의 현재값/fallback을 읽는지 확인한다.

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

namespace { //World/Session/패킷을 만들지 않고 장기 본체의 실제 backing을 조립한다.
// 선택되지 않은 실제 UI/시계/진단을 성공 대용으로 실행하지 않는 명시 입력 장치다.
ClientConstructionDevices OwnerClientDevices() {
    return {OwnerForbidden<std::int32_t,std::string_view,std::int32_t>(),OwnerForbidden<double>(),OwnerForbidden<void>(),OwnerForbidden<void>(),OwnerForbidden<void,PlayerGeyserBoundsAssertion>(),OwnerForbidden<std::string,std::string_view,std::string_view>(),OwnerForbidden<void,std::string_view>()};
}
struct LongConstructionScene {
    std::shared_ptr<ClientGameplayGlobals> gameplay{std::make_shared<ClientGameplayGlobals>()}; //실제 현재 모드/TYPE/옵션 owner다.
    std::shared_ptr<RawSurfaceStorage> storage{OwnerStorage(gameplay)}; //같은 원본 자산/Pool/목록이며 Native 전송은 금지한다.
    std::shared_ptr<ClientRandomGlobals> random{std::make_shared<ClientRandomGlobals>()}; //같은 장기 난수이며 생성 중 seed를 쓰지 않는다.
    std::shared_ptr<ClientSpGlobals> sp{std::make_shared<ClientSpGlobals>(random,[] { return 123; })}; //초기화 전의 실제 분산 SP 몸체다.
    std::shared_ptr<ClientPlayerRecords> players{std::make_shared<ClientPlayerRecords>()}; //실제 Player base0이며 할당/대여를 하지 않는다.
    std::shared_ptr<ClientConstructionGlobals> owner{std::make_shared<ClientConstructionGlobals>()}; //본체와 같은 State/backing의 실제 장기 owner다.
    //장치 계약을 연결만 하며 큰 효과/전송은 선택 시 실패하게 한다.
    LongConstructionScene() {
        RawSurfaceDisplayBindings display;display.view=[] { return SquidDisplayView{0,0,65536,{0,0,1024,768}}; }; //Client가 미리 공개하는 현재 논리 카메라/지도 입력이며 실제 출력은 금지한다.
        display.suppressed=OwnerForbidden<bool>();display.invalidate=OwnerForbidden<void,SquidDisplayRect,std::uint32_t>();storage->BindDisplayDevices(std::move(display));
        storage->BindSerializationGlobals(std::make_shared<FortSerializationGlobals>());
        storage->BindRewardDevices({std::shared_ptr<ScrambledSpStore>(sp,&sp->Store()),std::shared_ptr<const std::uint32_t>(gameplay,&gameplay->world.editor),OwnerForbidden<void>(),OwnerForbidden<void,std::uint32_t,std::int32_t>()});
        storage->BindPostPopDevices({OwnerForbidden<void,std::uint32_t,Sid,bool>(),[current=gameplay] { return current->player.battleOwner; },OwnerForbidden<void>()});
        storage->BindDeletionDevices({OwnerForbidden<void,std::uint32_t,std::uint32_t,std::uint32_t>(),OwnerForbidden<void,Sid>()});owner->Bind(storage,gameplay,{});owner->BindDevices(OwnerClientDevices());
    }
    RawCampaignConstructionBindings& Body() { return owner->Body(sp,players,gameplay->BattleOptionsOwner()); } //동일 실제 backing만 전달하며 현재 Player를 조회하지 않는다.
};
}

// 전체 body를 World/Player 구성 전에 만들고 같은 backing과 현재 상태를 재초기화하지 않는다.
TEST_CASE(ConstructionOwner_LongBodyBeforeWorldAndPlayerAllocation) {
    LongConstructionScene scene;CHECK(OwnerRejected([&] { scene.owner->Body(); }));auto* body=&scene.Body();scene.owner->State().notice.historyCounter=17;
    CHECK(&scene.Body()==body && &scene.owner->Body()==body && scene.storage->Generation()==0 && !scene.storage->Borrowed() && !scene.players->Initialized() && scene.players->Base()==0);
    auto options=std::make_shared<std::array<std::uint8_t,87>>();CHECK(OwnerRejected([&] { scene.owner->Body(scene.sp,scene.players,options); }));
    auto otherPlayers=std::make_shared<ClientPlayerRecords>();CHECK(OwnerRejected([&] { scene.owner->Body(scene.sp,otherPlayers,scene.gameplay->BattleOptionsOwner()); }));
    GameRandom random(123);body->PopulateGeysers(0,random,{},{});CHECK(random.State()==123 && scene.owner->State().notice.historyCounter==17 && scene.players->Base()==0);
    CHECK(OwnerRejected([&] { body->GenerateLocalGeysers(); }));CHECK(scene.players->Base()==0 && scene.storage->Generation()==0 && !scene.storage->Borrowed()); //실제 설정 효과가 없으면 Player/생성/방송 전에 명시 실패한다.
}

// 실제큰밀어내기콜백의대여/전체pin은외부owner반납뒤에도예외복귀까지살아있고다음대여만새로선택한다.
TEST_CASE(ConstructionOwner_LongBodyWeakRuntimePinsSelectedEffectAndRebinds) {
    LongConstructionScene scene;auto* body=&scene.Body();scene.owner->State().notice.historyCounter=23;scene.storage->GameAux()->MissionSettings()[1]=1;scene.storage->GameAux()->ConstructionDisplaceGenus()=0xffffffffU; //원문의명시강제배치입력뒤실제남은밀어내기효과를선택하며게임프로필은변경하지않는다.
    auto runtime=std::make_shared<ClientConstructionRuntimeBindings>();auto whole=std::make_shared<int>(7);std::weak_ptr<int> wholeWatch=whole;std::weak_ptr<ClientConstructionRuntimeBindings> runtimeWatch=runtime;std::uint32_t calls=0;
    runtime->pin=[wholeWatch]()->std::shared_ptr<void> { auto current=wholeWatch.lock();if (!current) throw std::logic_error("검사의 실제 선택 pin 만료");return current; }; //선택 호출만 실제 외곽 owner를 strong으로 빌린다.
    runtime->gameplay.request.displace=[&](float,float,std::uint32_t,std::uint32_t)->bool {
        CHECK(!wholeWatch.expired() && !runtimeWatch.expired());++calls;runtime.reset();whole.reset();CHECK(!wholeWatch.expired() && !runtimeWatch.expired());throw std::runtime_error("선택 배치의 원래 실패"); //Player/패킷/실제 생성에 들어가기 전 큰 효과의 예외 경계를 확인한다.
    };
    scene.owner->BindRuntime(runtime);auto other=std::make_shared<ClientConstructionRuntimeBindings>();CHECK(OwnerRejected([&] { scene.owner->BindRuntime(other); }));
    bool failed=false;try { static_cast<void>(body->Request({75,1,20,21})); }catch (const std::runtime_error& error) { failed=std::string_view(error.what())=="선택 배치의 원래 실패"; }
    CHECK(failed && calls==1 && wholeWatch.expired() && runtimeWatch.expired() && scene.players->Base()==0);
    other->gameplay.request.displace=[&](float,float,std::uint32_t,std::uint32_t)->bool { ++calls;throw std::runtime_error("다음 배치의 원래 실패"); }; //반납된Session콜백을다시실행하지않고같은원래실패/수명단언을유지한다.
    scene.owner->BindRuntime(other);failed=false;try { static_cast<void>(body->Request({75,1,20,21})); }catch (const std::runtime_error& error) { failed=std::string_view(error.what())=="다음 배치의 원래 실패"; }
    CHECK(failed && calls==2 && &scene.Body()==body && scene.owner->State().notice.historyCounter==23 && scene.players->Base()==0 && !scene.storage->Borrowed());other.reset();CHECK(OwnerRejected([&] { static_cast<void>(body->Request({75,1,20,21})); }));
}

// 전체 body가 모든 실제 backing을 보유하고 최종 owner 반납 뒤 참조 순환 없이 함께 해제한다.
TEST_CASE(ConstructionOwner_LongBodyRetainsAndReleasesActualBacking) {
    LongConstructionScene scene;auto* body=&scene.Body();std::weak_ptr<RawSurfaceStorage> physical=scene.storage;std::weak_ptr<ClientGameplayGlobals> gameplay=scene.gameplay;std::weak_ptr<ClientSpGlobals> sp=scene.sp;std::weak_ptr<ClientPlayerRecords> players=scene.players;
    scene.storage.reset();scene.gameplay.reset();scene.sp.reset();scene.players.reset();scene.random.reset();CHECK(!physical.expired() && !gameplay.expired() && !sp.expired() && !players.expired() && &scene.owner->Body()==body);
    scene.owner.reset();CHECK(physical.expired() && gameplay.expired() && sp.expired() && players.expired());
}

// 같은 실제 Config/Player 기록으로 World/Runtime 없이 생성 입구를 실행하고 현재 설정/시계 선택을 확인한다.
TEST_CASE(ConstructionOwner_ClientDevicesGenerateWithoutWorldOrRuntime) {
    LongConstructionScene scene;auto* body=&scene.Body();ConfigInterface config([](auto)->std::optional<std::vector<std::uint8_t>> { return {}; });std::uint32_t reads=0,clocks=0,alliances=0;
    auto devices=OwnerClientDevices();devices.configInt=[&](auto key,auto fallback) { CHECK(key=="predictableGeysers");++reads;config.ReadInt(key,fallback);return fallback; };devices.wallSeconds=[&] { ++clocks;return 27.5; }; //실제 Config의 현재값과 명시 벽시각 입력만 선택한다.
    scene.owner->BindDevices(std::move(devices));scene.players->Initialize([&](auto,auto) { ++alliances; });auto records=scene.players->Records();
    for (std::size_t owner=0;owner<9;++owner) records[owner*0xb4+0x50]=0; //아홉 실제 기록의 비활성 Player 입력으로 생성/방송/전투를 선택하지 않는다.
    const auto randomBefore=scene.random->Game().State();scene.owner->State().notice.historyCounter=31;
    body->GenerateLocalGeysers();CHECK(reads==1 && clocks==0);CHECK(!config.Set("predictableGeysers","0"));body->GenerateLocalGeysers();CHECK(reads==2 && clocks==1);CHECK(config.Set("predictableGeysers","1"));body->GenerateLocalGeysers();CHECK(reads==3 && clocks==1);
    CHECK(alliances!=0 && scene.players->Initialized() && &scene.Body()==body && scene.storage->Generation()==0 && !scene.storage->Borrowed() && scene.random->Game().State()==randomBefore && scene.owner->State().notice.historyCounter==31 && body->LastPacket().empty());scene.owner->ReleaseDevices();
}

// 재결합 중 선택된 장치/함수의 수명과 예외를 보존하고 반납 뒤 Session 효과로 우회하지 않는다.
TEST_CASE(ConstructionOwner_ClientDevicesKeepSelectedCallAndDoNotFallbackAfterRelease) {
    LongConstructionScene scene;auto* body=&scene.Body();std::weak_ptr<int> watch;std::uint32_t calls=0,runtimeCalls=0;
    auto next=OwnerClientDevices();next.configInt=[&](auto,auto)->std::int32_t { ++calls;throw std::runtime_error("다음 Client 설정 실패"); }; //다음 호출은 새 실제 장치 집합만 선택한다.
    {
        auto selected=std::make_shared<int>(9);watch=selected;auto first=OwnerClientDevices();first.configInt=[selected,&watch,&scene,&next,&calls](auto,auto)->std::int32_t { ++calls;scene.owner->BindDevices(next);CHECK(!watch.expired());throw std::runtime_error("선택 Client 설정 실패"); };scene.owner->BindDevices(std::move(first)); //선택 중 외부 집합 교체도 현재 함수/집합을 무효화하지 않는다.
    }
    bool failed=false;try { body->GenerateLocalGeysers(); }catch (const std::runtime_error& error) { failed=std::string_view(error.what())=="선택 Client 설정 실패"; }CHECK(failed && calls==1 && watch.expired());
    auto invalid=OwnerClientDevices();invalid.tell={};CHECK(OwnerRejected([&] { scene.owner->BindDevices(std::move(invalid)); }));failed=false;try { body->GenerateLocalGeysers(); }catch (const std::runtime_error& error) { failed=std::string_view(error.what())=="다음 Client 설정 실패"; }CHECK(failed && calls==2);
    auto runtime=std::make_shared<ClientConstructionRuntimeBindings>();runtime->effects.configInt=[&](auto,auto)->std::int32_t { ++runtimeCalls;throw std::logic_error("반납 뒤 선택 금지된 Session 설정"); };scene.owner->BindRuntime(runtime);scene.owner->ReleaseDevices();CHECK(OwnerRejected([&] { body->GenerateLocalGeysers(); }));CHECK(runtimeCalls==0 && calls==2 && scene.players->Base()==0 && scene.storage->Generation()==0 && !scene.storage->Borrowed());
}

// 원문조기반환은실제Bounds/SP/Player와큰배치를구성하거나조회하지않는다.
TEST_CASE(ConstructionOwner_ReplenishModesBeforeBodyAndPlayer) {
    LongConstructionScene scene;scene.gameplay->Boss()=true;const auto seed=scene.random->Game().State();
    for (int mode=0;mode<3;++mode) { //각원문single/fort/editor관문을독립입력으로확인한다.
        scene.gameplay->world.missionPost=mode==0;scene.storage->ownerMode.fort=mode==1;scene.gameplay->world.editor=mode==2 ? 7U:0U;scene.owner->ReplenishNuggets();CHECK(OwnerRejected([&] { static_cast<void>(scene.owner->Body()); }));
    }
    std::optional<std::string> detail;std::uint32_t assertions{};scene.storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(expression=="boss" && file=="\\Ns\\O\\Nugget.cpp" && line==0x246);++assertions; });scene.gameplay->Boss()=false;scene.owner->ReplenishNuggets();
    CHECK(assertions==1 && scene.players->Base()==0 && scene.storage->Generation()==0 && !scene.storage->Borrowed() && scene.random->Game().State()==seed);
}

// 장기backing은초기화없이보유하고다른owner교체를거부하며반납시순환없이소멸한다.
TEST_CASE(ConstructionOwner_ReplenishBackingBeforeBodyRetainsIdentity) {
    LongConstructionScene scene;scene.owner->BindBacking(scene.sp,scene.players,scene.gameplay->BattleOptionsOwner());CHECK(OwnerRejected([&] { static_cast<void>(scene.owner->Body()); }));
    auto other=std::make_shared<ClientPlayerRecords>();CHECK(OwnerRejected([&] { scene.owner->BindBacking(scene.sp,other,scene.gameplay->BattleOptionsOwner()); }));std::weak_ptr<ClientPlayerRecords> watch=scene.players;scene.players.reset();CHECK(!watch.expired());scene.owner.reset();CHECK(watch.expired());
}

namespace { // 실제선택된배치효과와그효과를소유한외곽수명을함께모델링한다.
// 콜백몸체를포함한실제검사런타임을소유해외곽pin의반납순서를검사한다.
struct ReplenishRuntimeOwner {
    std::shared_ptr<ClientConstructionRuntimeBindings> binding{std::make_shared<ClientConstructionRuntimeBindings>()}; //실제함수몸체를소유하며Global에서는weak로만빌린다.
};
}

// 같은현재Bounds/여백을다시읽고배치호출중외곽owner반납·예외에도실제선택수명을유지한다.
TEST_CASE(ConstructionOwner_ReplenishBoundsAndActualPlacementPin) {
    LongConstructionScene scene;scene.owner->BindBacking(scene.sp,scene.players,scene.gameplay->BattleOptionsOwner());scene.gameplay->Boss()=true;scene.gameplay->world.missionPost=false;scene.storage->ownerMode.fort=false;scene.gameplay->world.editor=0;scene.storage->chunks.At(0,0).island=0;scene.storage->chunks.At(0,0).record={0,0,0,0,0,0};
    auto whole=std::make_shared<ReplenishRuntimeOwner>();std::weak_ptr<ReplenishRuntimeOwner> watch=whole;whole->binding->pin=[watch]()->std::shared_ptr<void> { return watch.lock(); };std::vector<FortRandomRectangle> areas;bool fail{};
    whole->binding->replenishPlacement=[&](auto area,auto quantity) { CHECK(quantity==1 && !watch.expired());areas.push_back(area);if (fail) { whole.reset();CHECK(!watch.expired());throw std::runtime_error("선택한 실제 보충 배치 실패"); } };scene.owner->BindRuntime(whole->binding);
    scene.owner->ReplenishNuggets();auto* body=&scene.owner->Body();scene.storage->GameAux()->WorldGeyserMargin()+=1;scene.owner->ReplenishNuggets();CHECK(areas.size()==2);
    if (areas.size()==2) CHECK(areas[1].left==areas[0].left-16 && areas[1].top==areas[0].top-16 && areas[1].right==areas[0].right+16 && areas[1].bottom==areas[0].bottom+16);
    fail=true;bool thrown{};try { scene.owner->ReplenishNuggets(); }catch (const std::runtime_error& error) { thrown=std::string_view(error.what())=="선택한 실제 보충 배치 실패"; }
    CHECK(thrown && watch.expired() && &scene.owner->Body()==body && areas.size()==3);CHECK(OwnerRejected([&] { scene.owner->ReplenishNuggets(); }));CHECK(scene.players->Base()==0 && scene.storage->Generation()==0 && !scene.storage->Borrowed());
}

// 실제전체MayPlace는World/Runtime/Player할당없이같은공간에서판정하고Session콜백을선택하지않는다.
TEST_CASE(ConstructionOwner_PlacementBeforeWorldAndPlayerUsesFullBody) {
    LongConstructionScene scene;scene.owner->BindBacking(scene.sp,scene.players,scene.gameplay->BattleOptionsOwner());std::uint32_t borrowed{};auto runtime=std::make_shared<ClientConstructionRuntimeBindings>();runtime->gameplay.request.mayPlace=[&](const auto&)->bool { ++borrowed;throw std::logic_error("장기 MayPlace의 Session 우회"); };scene.owner->BindRuntime(runtime);
    const auto seed=scene.random->Game().State();CHECK(!scene.owner->MayPlace({75,0,128,128,0,1,0}));auto* pipeline=&scene.owner->PlacementPipeline();runtime.reset();CHECK(!scene.owner->MayPlace({75,0,128,128,0,1,0}));
    CHECK(&scene.owner->PlacementPipeline()==pipeline && borrowed==0 && scene.players->Base()==0 && !scene.storage->Borrowed() && scene.storage->Generation()==0 && scene.random->Game().State()==seed);CHECK(OwnerRejected([&] { static_cast<void>(scene.owner->Body()); }));
}

// 현재Player/방향동맹/editor/Graph/TYPE와강제배치DWORD를실제owner에서다시읽는다.
TEST_CASE(ConstructionOwner_PlacementReadsCurrentGlobalsAndAlliances) {
    LongConstructionScene scene;scene.owner->BindBacking(scene.sp,scene.players,scene.gameplay->BattleOptionsOwner());scene.storage->GameAux()->MissionSettings()[1]=0x80000000U;scene.gameplay->player.battleOwner=0x80000001U;scene.gameplay->world.editor=0x10001U;scene.owner->PlacementAnchor().graphReady=9;scene.players->Alliances()[11]=0xfedcba98U;scene.storage->GameAux()->PuzzlePieceType()=108;
    scene.owner->PlacementState().placement.blockedRelation=0xabcdef01U;CHECK(scene.owner->MayPlace({75,0,128,128,0,1,0}));auto& state=scene.owner->PlacementState();CHECK(state.placement.blockedRelation==0 && state.placement.localPlayer==std::bit_cast<std::int32_t>(0x80000001U));CHECK(state.permission.editor==0x10001U && state.permission.graphReady==9 && state.permission.alliances[11]==0xfedcba98U && state.geometry.patternTypes[0]==108);
    scene.gameplay->world.editor=0;scene.players->Alliances()[11]=7;scene.owner->PlacementAnchor().graphReady=0;CHECK(scene.owner->MayPlace({75,0,128,128,0,1,0}));CHECK(state.permission.editor==0 && state.permission.graphReady==0 && state.permission.alliances[11]==7 && scene.players->Base()==0);
}

// 같은Client재결합에서실제배치본체/현재상태/추가목록을재생성하거나초기화하지않는다.
TEST_CASE(ConstructionOwner_PlacementRebindPreservesActualStateAndLifetime) {
    LongConstructionScene scene;auto* pipeline=&scene.owner->PlacementPipeline();auto* state=&scene.owner->PlacementState();auto* anchor=&scene.owner->PlacementAnchor();auto* additional=&scene.owner->PlacementAdditional();state->permission.useAlliances=7;anchor->graphReady=9;additional->entries={123};additional->count=1;
    scene.owner->Bind(scene.storage,scene.gameplay,{});CHECK(&scene.owner->PlacementPipeline()==pipeline && &scene.owner->PlacementState()==state && &scene.owner->PlacementAnchor()==anchor && &scene.owner->PlacementAdditional()==additional);CHECK(state->permission.useAlliances==7 && anchor->graphReady==9 && additional->count==1 && additional->entries[0]==123);
    std::weak_ptr<RawSurfaceStorage> storage=scene.storage;std::weak_ptr<ClientGameplayGlobals> gameplay=scene.gameplay;scene.storage.reset();scene.gameplay.reset();CHECK(!storage.expired() && !gameplay.expired());scene.owner.reset();CHECK(storage.expired() && gameplay.expired());
}
