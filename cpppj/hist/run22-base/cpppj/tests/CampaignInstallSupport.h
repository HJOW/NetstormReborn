#pragma once
// 보호 캠페인 자료와 동일 Pool의 실제 설치 모듈을 조합한다. GUI/전투 프레임은 실행하지 않는다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignWorldRuntime.h"
#include "client/RawCampaignAssetBindings.h"
#include "client/RawPriestWorldBindings.h"
#include "client/PriestPlacementAssets.h"
#include "o/RawSquidFinder.h"
#include "o/RawSquidLevel.h"
#include "client/RawCampaignGunQueries.h"
#include "o/RawPlayerRegular.h"
#include "o/RawPlayerTemplateState.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 원본 초기 다리/받침 전역과 실제 전투 모드를 입력한다.
BridgeConnectState CampaignLinks() {
    BridgeConnectState result;result.bridgeType=82;result.islandType=94;result.stalagType=95;
    result.connectorType=155;result.noIslandType=157;result.battle=true;result.mission=true;return result;
}
// 선택되지 않은 하위 효과는 성공으로 덮지 않고 해당 단계가 필요함을 보고한다.
template<class Result,class... Arguments> std::function<Result(Arguments...)> InstallForbidden() {
    return [](Arguments...)->Result { throw std::logic_error(std::string("초기 캠페인 검사에서 미연결 하위 효과 요청: ")+__FUNCSIG__); };
}
// 실제 보호 타입/프레임/SHP·난수·Player·섬·프로세스 저장소를 한 번 구성한다.
struct InstallScene {
    const netstorm::test::dudespawn::DudeAssets& assets; // 보호 archive의 현재 실제 자료다.
    PriestPlacementAssets metadata; // 실제 프레임/핫스폿/SHP이며 합성 그림 자료가 아니다.
    RawSurfaceWorld world; // 모든 생성/Pop/Graph가 사용하는 단일 raw 월드다.
    RawWorldCore core; // 같은 Pool/장부/해시의 참조만 유지한다.
    GameRandom random{0x12345678}; // 게임 난수의 명시 초기 입력이다.
    ScrambledSpStore sp; // 같은 게임 난수를 사용하는 실제 패치 SP 저장소다.
    SquidRewardState rewardState; // 실제 HP getter의 장기 전역이다.
    SquidReward hp; // 같은 Pool/TYPE/장부로 최대HP와 실제HP를 읽는다.
    std::unique_ptr<RawSquidLevel> level; // 실제 base 가상80/84의 Factory 저장 상태를 처리한다.
    ChunkMap chunks; // 실제 Builder/성장/객체 선택의 지도다.
    std::array<std::uint8_t,RawIslandRuntime::kBytes> islandBytes{}; // 실제3204BYTE 섬 목록이다.
    std::vector<std::uint8_t> players=std::vector<std::uint8_t>(9*0xb4); // 부모/Owner/신전이 공유하는 실제9 기록이다.
    FortFinishOwnerState finishState; // 실제 미션 chaffMode의 자연 반환 조건이다.
    AssetPostPopState assetState; // 실제 신전/레지던스/간헐천/아웃포스트 목록이다.
    RawPriestWorldState priestState; // 실제 사제/Carrier/보호막 장기 전역이다.
    PlayerWorldState playerState; // 현재 battleOwner와 templateOwner는 별도 상태다.
    FortObjectLoadState objectState; // 저장객체의 실제 미션/로딩 상태다.
    CampaignWorldGlobals globals; // 실제 로그인과 missionPost의 장기 전역이다.
    std::uint32_t revision{},ownerChanged{},vortexRevision{},piece{107},crt{7}; // 원본 섬/신전/패턴/CRT 전역이다.
    std::int32_t noRegion{127}; // 정상 지형 생성의 실제 현재 지역없음 번호다.
    std::unique_ptr<RawIslandRuntime> islands; // 위3204BYTE와 동일 Pool을 공유한다.
    std::unique_ptr<RawPriestWorldBindings> priests; // 저장 사제의 실제 Owner/PostPop/보호막 분배기다.
    std::unique_ptr<RawPlayerWorld> player; // 최초152 생성자/실제 PostPop/9기록을 연결한다.
    std::unique_ptr<RawPlayerRegular> playerRegular; // 현재 Player의 실제 사건1/2를 처리한다.
    std::unique_ptr<RawPlayerTemplateState> templateState; // 같은Template header/원본State28BYTE를 직접 읽는다.
    std::array<std::vector<std::uint8_t>,9> stateBytes; // 실제 선택한 State 섹션의 writable 원문이다.
    std::array<PlayerTemplateStateStream,9> stateStreams; // 같은 State cursor/end를 공유한다.
    std::array<std::string,9> stateNames; // 같은 현재 Template manager의 실제 이름이다.
    RegionPaintState paint; // 실제 지역 도장의 장기 상태다.
    RegionOwnershipState votes; // 실제 현재 신전/워크샾 투표 상태다.
    std::vector<std::uint32_t> themes; // 현재 TYPE의 같은 원소 번호다.
    std::vector<std::int32_t> elements; // 자산 Regular가 읽는 같은 원소 번호다.
    std::unique_ptr<RawCampaignOwnership> ownership; // 실제 Vortex/섬/Player/지역 연결이다.
    CarrierPostPopState carrierState; // Damageable의 실제 로딩 전역이다.
    std::unique_ptr<RawCarrierPostPop> carrier; // 같은 공통 장부와 실제 지면 소유자 경계다.
    AssetRegularState regularState; // 실제 예약 사건의 현재 전역이다.
    GunProcessState gunState; // 실제 Gun 초기 Form/시각/서버 전역이다.
    GunTargetState gunTargetState; // 실제 자기 동맹/검색의 현재 상태다.
    std::vector<GunTargetType> gunTypes; // 보호 TYPE의 사거리/방향/높이 보충 자료다.
    std::vector<GunHitType> hitTypes; // 보호 TYPE의 HP/초와 별도 피해 보충 자료다.
    std::vector<float> firingIntervals; // 실제 타입별 발사 간격이다.
    std::unique_ptr<RawGunMode> gunMode; // 현재 raw 공격 방향을 매 검색마다 읽는다.
    std::unique_ptr<RawGunTarget> gunTarget; // 실제 finder/ray/필터의 같은 Core 인스턴스다.
    std::unique_ptr<RawGunHit> gunHit; // 실제 Q16 시간 압축과 현재 피해 getter다.
    std::uint32_t archerCommand{0x030000b0}; // 보호 PE의 실제 입력 토큰이다.
    std::unique_ptr<RawCampaignAssetBindings> assetBindings; // 실제 저장 건물 후처리와 Gun/Regular다.
    CanonPlacementPipelineState placement; // 실제 공간/기하/지형 허용 상태다.
    SquidPostPopList additional; // 기존 pipeline이 읽는 실제 추가 목록이다.
    ContainedFinderState contentFinder; // 기존 내용물 검색의 장기 전역이다.
    std::unique_ptr<RawCanonPlacementPipeline> pipeline; // 실제 Same Core MayPlace다.
    TerrainContextGlobals terrainGlobals; // 정상 문자 mapping의 실제 전역이다.
    TerrainStampState stamp; // 첫 생성 이후에도 유지되는 실제99cache다.
    FortRandomPlacementState randomPlacement; // 실제 지형/edgeFarm 배치 상태다.
    FarmClusterState farm; // 실제 정상 군집 모드다.
    std::unique_ptr<RawNormalTerrainWorldBinding> normal; // 실제 성장/도장/Normalize/군집 몸체다.
    std::unique_ptr<RawFortFinishOwner> finish; // 같은9 Player의 최초 지역을 읽는다.
    ContainedCreateState contentState; // 저장 내용물은 실제 Form6으로 생성한다.
    std::unique_ptr<ContentFormHost> contentForms; // ProcessForm과 다른 실제 Form6의 정렬 부착 몸체다.
    std::unique_ptr<RawContainedCreate> contained; // 실제 Factory/ContentFormHost의 저장 내용물 생성이다.
    std::array<FortTemplate,9> templates; // 실제 선택 자료는1번 Template에 둔다.
    std::array<FortTerritoryStream,9> streams; // 같은 Template의 실제120BYTE view다.
    std::array<std::uint8_t,180> headers{}; // 실제 Template9 header의 현재 바이트다.
    std::array<std::uint8_t,64> handle{}; // 현재 이름을 쓸 실제 저장소다.
    std::array<std::uint8_t,4> translated{'Y','o','u',0}; // 검사 경계의 명시 영어 번역 입력이다.
    std::vector<std::string> external; // 실제 UI/소리/전송 요청의 검사 관찰 기록이다.
    std::unique_ptr<RawCampaignWorldRuntime> runtime; // 위 실제 하위를 단일 캠페인 부모에 연결한다.

    // 보호 자료를 읽기 전용으로 사용하며 모든 필수 후처리를 첫 생성 전에 등록한다.
    explicit InstallScene(std::string_view mission="thewarbegins"):assets(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078)),metadata(assets.game),
        world(OriginalEdition::Patch1078,assets.types,assets.frames,metadata.shapes,CampaignLinks(),97),core(world.Core()),
        sp(random,[] { return 1U; }),hp(core.pool,core.types,core.bookkeeping,rewardState,&sp),themes(core.types.size()),elements(core.types.size()),
        gunTypes(core.types.size()),hitTypes(core.types.size()),firingIntervals(core.types.size()) {
        sp.Initialize();level=std::make_unique<RawSquidLevel>(core.pool,core.types,InstallForbidden<void,SquidLevelAssertion>());
        priestState.regen.patch.sequence=std::bit_cast<std::int32_t>(sp.Offset());
        priestState.regen.patch.observedSequence=priestState.regen.patch.sequence;
        finishState.chaffMode=true;objectState.mission=true;objectState.chaffMode=true;
        playerState.playerKnown=1;assetState.boss=true;assetState.inBattle=true;assetState.localOwner=1;gunState.boss=true;
        priestState.priests.priests.entries.resize(100);assetState.residences.entries.resize(100);
        assetState.temples.entries.resize(100);assetState.outposts.entries.resize(100);assetState.geysers.entries.resize(100);
        // 원본 Player의 첫 지역 sentinel은9기록에 직접 쓴다. 별도 playerRegions 배열을 사용하지 않는다.
        for (std::uint32_t owner=0;owner<9;++owner) {
            Put(players,owner*0xb4+0x78,127);Put(players,owner*0xb4+0x54,1U<<owner);
            gunTargetState.allyMasks[owner]=1U<<owner;gunState.allies[owner][owner]=true;
        }
        islands=std::make_unique<RawIslandRuntime>(core.pool,core.hash,islandBytes,finishState,assetState.residences,revision,ownerChanged,
            IslandRuntimeHooks{[this](Sid sid,std::uint32_t owner) { ownership->SetOwner(sid,owner); },InstallForbidden<void,IslandRuntimeAssertion>()});
        islands->Reset();
        RawPriestWorldEffects priestEffects;
        priestEffects.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        priestEffects.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };
        priestEffects.repop=[this](Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
        priestEffects.destroy=[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        priestEffects.claimGround=[this](float x,float y,std::uint8_t owner) { world.SetSupportOwner(x,y,owner); };
        priestEffects.ownerNotice=[this](Sid) { external.push_back("사제 Owner 통지"); };
        priestEffects.notifySurface=[this](Sid sid) { core.building.NotifySurface(sid); };
        priestEffects.dependentFound=InstallForbidden<void>();priestEffects.reserveRegular=[] { return true; };
        priestEffects.shield.reserveSound=[] { return true; };
        priestEffects.shield.loadSound=InstallForbidden<std::uint32_t,std::string_view>();
        priestEffects.shield.attachSound=InstallForbidden<void,Sid,std::uint32_t,std::uint32_t,std::uint32_t>();
        priestEffects.shield.noticeSound=InstallForbidden<void,const PriestShieldNotice&>();
        priestEffects.shield.translate=InstallForbidden<std::string,std::string_view>();priestEffects.shield.tell=InstallForbidden<void,std::string_view>();
        priestEffects.fallSound=InstallForbidden<void,Sid,float,float>();priestEffects.otherWalkerFall=InstallForbidden<bool,Sid>();
        priestEffects.regen.refresh=[this](Sid sid) { core.display.Update(core.pool.Slot(sid)); };
        priestEffects.regen.neutralAction=InstallForbidden<void,Sid>();
        // 실제 SP 회전 몸체의 현재 offset을 같은 회복 추적 전역에 반영한다. 저장 값/게임 난수를 유지한다.
        priestEffects.regen.patchAudit=[this] { sp.Rotate();priestState.regen.patch.observedSequence=std::bit_cast<std::int32_t>(sp.Offset()); };
        priestEffects.regen.measurement=InstallForbidden<double>();
        priests=std::make_unique<RawPriestWorldBindings>(RawPriestWorldCore{core.pool,core.hash,core.types,core.frames,core.spots,core.hash.Entries(0),hp,random,core.factory,core.owner,core.ownerMode,core.postPop,core.frame},priestState,std::move(priestEffects));
        priests->BindProcesses(core.processes);priests->SetAuthority(true);priests->SetLocalPlayer(1);
        PlayerWorldHooks playerHooks;
        playerHooks.assertion=InstallForbidden<void,PlayerWorldAssertion>();
        playerHooks.find=[this](float x,float y,std::uint32_t type) { RawSquidFinder finder(core.pool,core.hash,core.types);return finder.Begin({static_cast<int>(x),static_cast<int>(y),static_cast<int>(x),static_cast<int>(y)},0,[this,type](Sid sid) { return core.pool.Slot(sid)[10]==type; }); };
        playerHooks.create=[this](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };
        playerHooks.setOwner=[this](Sid sid,std::uint32_t owner) { core.owner.Set(sid,owner); };
        playerHooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        playerHooks.reserveRegular=[] { return true; };
        playerHooks.addRegular=[this](Sid sid,std::uint32_t event,float payload) { CHECK(core.processes.AddRegular(sid,event,payload)!=nullptr); };
        player=std::make_unique<RawPlayerWorld>(core.pool,core.postPop,players,playerState,std::move(playerHooks));core.postPop.SetPlayerPostPop(player.get());
        templateState=std::make_unique<RawPlayerTemplateState>(core.pool.Edition(),headers,stateStreams,stateNames,
            objectState.loadingDepth,objectState.mission,InstallForbidden<void,PlayerTemplateStateAssertion>(),[this](std::uint32_t depth) { world.SetLoadingDepth(depth); });
        PlayerRegularHooks playerRegularHooks;playerRegularHooks.assertion=InstallForbidden<void,PlayerRegularAssertion>();
        playerRegularHooks.log=[this](std::uint32_t,std::string_view text) { external.push_back(std::string(text)); };
        playerRegularHooks.reserveRegular=[] { return true; };
        playerRegularHooks.addRegular=[this](Sid sid,std::uint32_t event,float payload) { CHECK(core.processes.AddRegular(sid,event,payload)!=nullptr); };
        playerRegularHooks.templateState=[this](std::uint32_t slot) { return templateState->Get(slot); };
        playerRegularHooks.templateChanged=[this](std::uint32_t slot) { templateState->Changed(slot); };
        playerRegular=std::make_unique<RawPlayerRegular>(*player,core.ownerMode,std::move(playerRegularHooks));
        ownership=std::make_unique<RawCampaignOwnership>(world,*islands,assetState,players,metadata.frames,themes,random,paint,votes,vortexRevision,97,121,
            CampaignOwnershipEffects{[this](Sid sid,std::uint32_t owner) { priests->SetOwner(sid,owner); },[this] { external.push_back("생산 Gump 요청"); },[] { return 0U; },InstallForbidden<void,Sid,std::int32_t,std::uint32_t>()});
        carrier=std::make_unique<RawCarrierPostPop>(core.pool,core.types,carrierState,
            MakeCarrierPostPopHooks(core.pool,core.postPop,[this](Sid sid) { static_cast<void>(priests->CheckCarrier(sid)); },[this](float x,float y,std::uint8_t owner) { world.SetSupportOwner(x,y,owner); }));
        // 보호 .type의 실제 숫자/문자 Run을 같은 TYPE 번호의 보충 표에 연결한다.
        for (const auto& asset:assets.game.Types()) {
            const auto type=70+asset.block;auto& target=gunTypes[type];target.range=static_cast<float>(asset.definition.Number("range").value_or(0));
            target.heightExtension=static_cast<int>(asset.definition.Number("height").value_or(0));
            const auto theme=asset.definition.String("theme").value_or("sun");target.theme=theme=="wind" ? 0 : theme=="rain" ? 1 : theme=="thunder" ? 2 : 3;
            // 원본 방향별 L/M/N/O 첫 프레임 표다. 없는Run의-1은 실제 검색 몸체가 처리한다.
            for (std::uint8_t direction=0;direction<4;++direction) target.directionFrames[direction]=core.frames[type].Run(static_cast<std::uint8_t>('L'+direction)).first;
            hitTypes[type].damagePerSecond=static_cast<float>(asset.definition.Number("hpPerSec").value_or(0));
            firingIntervals[type]=static_cast<float>(asset.definition.Number("delayBetweenShots").value_or(0));
        }
        gunMode=std::make_unique<RawGunMode>(core.pool,core.frames,gunState.windArcherType);
        gunTarget=std::make_unique<RawGunTarget>(core.pool,core.hash,core.types,gunTypes,core.shapes,gunTargetState);
        gunHit=std::make_unique<RawGunHit>(core.pool,core.types,hitTypes,GunHitHooks{InstallForbidden<void,Sid,std::int32_t,std::uint8_t,Sid>(),[this](Sid sid) { core.building.NotifySurface(sid); }});
        RawCampaignAssetEffects assetEffects;assetEffects.postPop.reserveRegular=[] { return true; };
        assetEffects.gun=MakeCampaignGunQueryHooks(core,*gunMode,*gunTarget,*gunHit,gunState,firingIntervals,std::move(assetEffects.gun));
        assetEffects.factory.warnDuplicate=InstallForbidden<void,Sid,std::int32_t,std::int32_t>();
        assetEffects.levelAssertion=InstallForbidden<void,SquidLevelAssertion>();
        assetEffects.factory.regionOwnership=[this](float x,float y) { ownership->UpdateRegion(x,y); };
        assetEffects.postPop.setOwner=[this](Sid sid,std::uint32_t owner) { ownership->SetOwner(sid,owner); };
        // PATCH0040d980은 현재float를절삭한 뒤16으로 나눈 청크의 실제 BYTE를 읽는다.
        assetEffects.postPop.regionNumber=[this](float x,float y) { return static_cast<std::int32_t>(chunks.At(static_cast<int>(x)/16,static_cast<int>(y)/16).island); };
        // 사운드 핸들은 검사 외부 경계의 명시 TYPE번호 토큰이다. 실제 자산 파일명을 관찰하며 PE 포인터로 주장하지 않는다.
        assetEffects.postPop.typeDword=[this](Sid sid,std::uint32_t offset) { CHECK(offset==0x74);return static_cast<std::uint32_t>(core.pool.Slot(sid)[10]); };
        assetEffects.postPop.playImpactSound=[this](std::uint32_t token) {
            CHECK(token>=70 && token<core.types.size());
            const auto sound=assets.game.Types()[token-70].definition.String("impactSound");
            external.push_back("신전 impactSound:"+std::string(sound.value_or("")));
        };
        assetEffects.queueCommand=[this](std::uint32_t command,std::int32_t sid,std::int32_t argument) { CHECK(command==archerCommand && sid>0 && argument==0);external.push_back("발사기 입력 큐"); };
        assetEffects.sendGunPacket=[this](std::span<const std::uint8_t> packet) { CHECK(!packet.empty());external.push_back("Gun 전송"); };
        assetBindings=std::make_unique<RawCampaignAssetBindings>(world,*carrier,assetState,random,*islands,players,playerState,RawCampaignAssetGlobals{regularState,gunState,elements,archerCommand},std::move(assetEffects));assetBindings->ActivatePostPop();
        RawWorldRuntimeHooks worldHooks;
        // 서버 Form 전송 요청은 검사 외부 경계에서 현재 타입/flags를 관찰한다. 실제 네트워크 구현을 뜻하지 않는다.
        worldHooks.formTransmit=[this](Sid form,std::uint32_t flags) { CHECK(core.pool.Slot(form)[10]==1 && flags==0x108);external.push_back("감시 Form 전송"); };
        // 삭제 전송만 검사 외부 경계로 관찰한다. 그 외의 gameplay 수명 효과는 실제 분배기가 처리해야 한다.
        worldHooks.assets.emit=[this](const SquidDestroyEvent& event) {
            if (event.effect!=SquidDestroyEffect::Transmit) throw std::logic_error("캠페인 검사에서 미연결 자산 삭제 수명 효과");
            external.push_back("삭제 전송");
        };worldHooks.assets.selected=[] { return Sid{}; };
        worldHooks.destroy=InstallForbidden<bool,Sid,std::uint32_t,const SquidDestroyHooks&>();
        worldHooks.regular=assetBindings->Handler(priests->RegularHandler([this](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
            if (playerRegular->Handles(sid)) return playerRegular->Handle(sid,event,count,payload);
            throw std::logic_error("캠페인 현재 Kernel의 다른 실제 Regular 부모 미연결: TYPE"+
                std::to_string(core.pool.Slot(sid)[10])+" event"+std::to_string(event));
        }));
        worldHooks.transmit=assetBindings->Transmission();worldHooks.walkerFall=[this](Sid sid) { return priests->FallWalker(sid); };
        worldHooks.loadingDepth=[this](std::uint32_t depth) { objectState.loadingDepth=depth;carrierState.loadingDepth=depth;priests->SetLoadingDepth(depth);assetBindings->SetLoadingDepth(depth); };
        world.BindRuntime(std::move(worldHooks));
        placement.geometry=metadata.geometry;placement.placement.localPlayer=1;placement.permission.alliances.fill(1);
        pipeline=std::make_unique<RawCanonPlacementPipeline>(core.pool,core.hash,CanonPlacementPipelineData{core.types,metadata.frames,core.shapes,metadata.hotspots,core.spots,core.hash.Entries(0),{}},core.bookkeeping,additional,contentFinder,placement);
        NormalTerrainDiagnostics diagnostics{InstallForbidden<void,TerrainContextAssertion>(),InstallForbidden<void,TerrainStampAssertion>(),InstallForbidden<void,SquidRegionAssert>(),InstallForbidden<void,FarmClusterAssertion>(),InstallForbidden<void,TerrainFarmPostpassAssertion>()};
        normal=std::make_unique<RawNormalTerrainWorldBinding>(world,chunks,*islands,random,crt,terrainGlobals,stamp,randomPlacement,farm,piece,*pipeline,hp,*carrier,std::move(diagnostics));
        finish=std::make_unique<RawFortFinishOwner>(core.pool,core.types,assetState.residences,finishState,FortFinishOwnerHooks{InstallForbidden<FortRandomRectangle,std::int32_t>(),InstallForbidden<double>(),InstallForbidden<std::uint32_t,GameRandom&,const FortRandomPlaceRequest&>(),InstallForbidden<void>()});
        ContainedCreateHooks contentHooks;contentHooks.notifySurface=[this](Sid sid) { core.building.NotifySurface(sid); };
        contentForms=std::make_unique<ContentFormHost>(core.pool,core.types,[this](Sid,Sid,std::uint32_t) { external.push_back("내용물 부모 통지"); });
        contained=std::make_unique<RawContainedCreate>(core.pool,contentState,contentForms->BindCreate(core.factory,std::move(contentHooks)));
        templates[1]=FortTemplate::Parse(assets.files.Read("d/"+std::string(mission)+".fort"),assets.game.TypeTable());
        const auto state=templates[1].Section("State");stateBytes[1].assign(state.begin(),state.end());
        stateStreams[1]={stateBytes[1],0,static_cast<std::uint32_t>(stateBytes[1].size()),!stateBytes[1].empty()};stateNames[1]=std::string(mission);
        streams[1]={templates[1].territory,0,static_cast<std::uint32_t>(templates[1].territory.size())};headers[20]=1;
        runtime=MakeRuntime();
    }
    // 외부 config/UI 요청은 검사 관찰이며 실제 UI 구현/동일성의 증거로 세지 않는다.
    std::unique_ptr<RawCampaignWorldRuntime> MakeRuntime(std::size_t templateCount=9) {
        RawCampaignInstallEffects hooks;
        hooks.translateYou=[this] { return std::span<const std::uint8_t>(translated); };
        hooks.setConfig=[this](std::string_view key,std::span<const std::uint8_t> bytes) { CHECK(key=="fort.mySubHandle" && bytes.size()==4);external.push_back("이름 config"); };
        hooks.refreshFortSpecification=[this] { external.push_back("사양 재조회"); };hooks.resetCamera=[this] { external.push_back("카메라"); };
        hooks.builderAssertion=InstallForbidden<void,RawIslandBuilderAssertion>();hooks.territoryAssertion=InstallForbidden<void,FortWorldLoadAssertion>();
        hooks.notifySurface=[this](Sid sid) { core.building.NotifySurface(sid); };
        hooks.setState=[this](Sid sid,std::uint32_t value) { assetBindings->SetState(sid,value); };
        hooks.virtualCount=[this](Sid sid) { return static_cast<std::int32_t>(level->Get(sid)); };
        return std::make_unique<RawCampaignWorldRuntime>(world,chunks,*islands,*player,*normal,random,hp,*ownership,*finish,*contained,objectState,globals,piece,noRegion,RawCampaignInstallData{std::span<const FortTemplate>(templates).first(templateCount),streams,headers,handle},std::move(hooks));
    }
    // 외부 콜백이 참조하는 모듈을 해제하기 전에 현재 Kernel을 비운다.
    ~InstallScene() { world.ClearProcesses();core.postPop.SetPlayerPostPop(nullptr); }
};
}
