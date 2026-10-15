// 기존 원본 몸체의 단일 저장소 조립을 실제 클라이언트 세션이 소유한다.
#include "client/RawCampaignSession.h"
#include "client/PriestPlacementAssets.h"
#include "client/RawCampaignGunQueries.h"
#include "o/RawGameRandomSeed.h"
#include "o/RawSquidFinder.h"
#include "o/RawSquidLevel.h"
#include "o/RawPlayerTemplateState.h"
#include "o/RawArcherTypeData.h"
#include "o/RawProjectileGeometry.h"
#include "o/RawFormPacket.h"
#include "o/RawContentParentNotice.h"
#include "o/RawPlayerDamageNotice.h"
#include "o/RawFenceTarget.h"
#include "o/RawRainVortexDraw.h"
#include <bit>
#include <stdexcept>
using namespace netstorm::o;
namespace netstorm::client {
namespace {
// 아직 미공급인 실제 하위는 선택 시 구체적 함수 인자형을 포함한 오류를 반환한다.
template<class Result,class... Args> std::function<Result(Args...)> SessionMissing() {
    return [](Args...)->Result { throw std::logic_error(std::string("실제 세션 하위 소비자 미연결: ")+__FUNCSIG__); };
}
// optional 경계를 성공으로 채우지 않고 실제 선택 시 요청을 보고한다.
template<class Result,class... Args> void FillMissing(std::function<Result(Args...)>& callback) { if (!callback) callback=SessionMissing<Result,Args...>(); }
// 동일 풀의 실제 예약/부착 실패를 즉시 보고한다.
void SessionCheck(bool value) { if (!value) throw std::logic_error("실제 캠페인 세션 예약/자료 조건 오류"); }
// 하나의 실제 클라이언트 소리 이름표만 사용한다.
SoundList& RequireSounds(const RawCampaignSessionServices& services) { if (!services.soundNames) throw std::invalid_argument("세션의 실제 SoundList 누락");return *services.soundNames; }
// 현재 원본 초기 다리/받침/미션 전역이다.
BridgeConnectState CampaignLinks() { BridgeConnectState result;result.bridgeType=82;result.islandType=94;result.stalagType=95;result.connectorType=155;result.noIslandType=157;result.battle=true;result.mission=true;return result; }
// 보호 GameAssets의 실제 TYPE/프레임을 보관하며 검사 전용74 변경을 적용하지 않는다.
struct SessionAssets {
    const GameAssets& game; // 클라이언트가 소유하는 단일 자산이다.
    const BaseFileSystem& files; // 동일 보호 VFS 참조다.
    std::vector<RiftTypeRecord> types; // 실제 원본 전체 TYPE 자료다.
    std::vector<RiftTypeFrames> frames; // 같은 물리 블록의 실제 프레임 Run이다.
    // PATCH 판본과 actual 자료를 첫 모듈 생성 전에 확인한다.
    SessionAssets(const GameAssets& source,const BaseFileSystem& storage):game(source),files(storage),types(source.TypeTable().Types().begin(),source.TypeTable().Types().end()),frames(types.size(),RiftTypeFrames({})) {
        if (source.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("실제 세션의 CD판 조립 미연결");
        // 실제 로딩 번호70부터 프레임 표를 직접 채운다.
        for (const auto& asset:source.Types()) frames[70+asset.block]=asset.definition.FrameTable();
    }
};
// 원본 비정렬 DWORD를 지정 raw 필드에 쓴다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    // 원본 little endian의 네 바이트만 기록한다.
    for (std::size_t i=0;i<4;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
}
// 실제 월드/상태와 모든 callback 대상을 같은 고정 주소에 소유한다.
struct RawCampaignSession::Impl {
    RawCampaignSessionServices services; // 실제 클라이언트의 선택 서비스이며 검사 관찰 기본값을 사용하지 않는다.

    SessionAssets assets; // 보호 archive의 현재 실제 자료다.
    PriestPlacementAssets metadata; // 실제 프레임/핫스폿/SHP이며 합성 그림 자료가 아니다.
    RawSurfaceWorld world; // 모든 생성/Pop/Graph가 사용하는 단일 raw 월드다.
    RawWorldCore core; // 같은 Pool/장부/해시의 참조만 유지한다.
    GameRandom random; // 게임 난수의 명시 초기 입력이다.
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
    std::uint32_t revision{},ownerChanged{},vortexRevision{},piece{107},crt{}; // 원본 섬/신전/패턴/CRT 전역이다.
    std::uint32_t randomMode{}; // 실제 재시드에서 임시로 변경하는 현재 DWORD다.
    RawGameRandomSeed seed; // 같은 CRT/게임 난수와 실제 Unix 초 입력을 연결한다.
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
    PriestDamageState priestDamageState; // 실제 미션 레벨보정 차단/HP1 감시 권한이다.
    DamageableBurningState burningState; // 현재 nonzero damageEffect의 실제 생성 판정 전역이다.
    GunImpactState impactState; // 현재 충돌 시각 효과 억제/anim160 전역이다.
    std::vector<DudeNoticeTypeData> dudeSpeeds; // 실제 TYPE 이동속도의 현재 mutable 보충 표다.
    std::unique_ptr<RawPlayerDamageNotice> playerDamage; // 같은 Player9/모드의 실제 AI 조회와 피해 통지 래퍼다.
    std::unique_ptr<RawCampaignHitBindings> hits; // 같은 피해/감시/충돌/주변Dude/자산수명 처리기다.
    SoundList& soundNames; // 실제 보호막 파일명의 원본 이름표/opaque handle 조회다.
    SoundProcessState soundState; // 실제47 프로세스의 현재 frame/TYPE 중복 재생 전역이다.
    std::unique_ptr<SoundProcessSystem> sounds; // 같은 Host/Kernel의 실제 보호막 sound47 수명을 수행한다.
    std::vector<ArcherTypeData> archerTypes; // 원본 orientations/클러스터 수의 실제 보충 표다.
    std::vector<MissileTypeData> missileTypes; // 원본 투사체 사거리/방향/소리 입력이다.
    std::vector<ProjectileGeometryType> projectileTypes; // 실제 SHP 포구 WORD와 기본/물리 프레임 표다.
    ArcherAnimationState archerState; // 같은 Kernel의 게임 시각/경과다.
    MissileState missileState; // 같은 투사체 게임 시각이다.
    std::unique_ptr<RawProjectileGeometry> projectileGeometry; // 실제 포구/표적 치수/충돌 상자다.
    std::unique_ptr<RawFenceTarget> fenceTarget; // 같은 해시/배리어 끝점/현재 동맹의 실제 표적 코너 계산이다.
    std::unique_ptr<RawCampaignArcherBindings> combat; // 같은 Archer/Missile 생성/공간/Host를 소유한다.
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
    std::unique_ptr<RawContentParentNotice> contentNotice; // 부모의 실제 현재 VT로 기본/파생 +24를 분배한다.
    std::unique_ptr<RawContainedCreate> contained; // 실제 Factory/ContentFormHost의 저장 내용물 생성이다.
    std::array<FortTemplate,9> templates; // 실제 선택 자료는1번 Template에 둔다.
    std::array<FortTerritoryStream,9> streams; // 같은 Template의 실제120BYTE view다.
    std::array<std::uint8_t,180> headers{}; // 실제 Template9 header의 현재 바이트다.
    std::array<std::uint8_t,64> handle{}; // 현재 이름을 쓸 실제 저장소다.
    std::unique_ptr<RawCampaignWorldRuntime> runtime; // 위 실제 하위를 단일 캠페인 부모에 연결한다.
    PlayerPlacementAnchorState anchorState; // 현재 같은 Graph의 배치/도착 조회 전역이다.
    std::unique_ptr<RawPlayerPlacementAnchor> anchor; // 같은 hash0/장부를 직접 읽는 getter다.
    RawCampaignMotionState motionState; // 실제 Halt/Arrival/cleanup/BFS의 현재 상태다.
    std::uint32_t geyserType{122}; // 실제 현재 가이저 자산 전역이다.
    bool snap{true},retrying{}; // 원본 사제 목적지 스냅/재시도 guard다.
    RawPathStartState start; // 같은 현재 시계/이동 타입을 참조한다.
    RawPathProcessState process; // 같은 Host의 경로 프로세스 전역이다.
    std::unique_ptr<RawCampaignMotionBindings> motion; // 실제 Start/BFS/도착/cleanup의 모듈 소유자다.
    RawPathDataHeader pathHeader; // 실제 경로 writer가 사용하는 현재 프로토콜 header다.
    std::array<std::uint8_t,16384> pathBuffer{}; // 원본 경로 직렬화의 현재 송신 버퍼다.
    std::unique_ptr<RawBattleBroadcast> broadcast; // 동일 Player9와 실제 현재 연결 표를 사용하는 송신 분배기다.
    FormPacketHeaders formHeaders; // 원본 현재 감시/내용물/삭제 헤더 전역이다.
    FormDeletionStamp deletionStamp; // 실제 삭제 writer가 한 번 소비하는 최근 명령 표식이다.
    std::unique_ptr<RawFormPacket> formWriter; // 동일 Pool의 실제 가상+54/+58 직렬화 몸체다.
    std::array<std::uint8_t,2048> formBuffer{}; // 감시/내용물/삭제 요청이 공유하는 원본 크기의 현재 송신 버퍼다.

    // 보호 자료를 읽기 전용으로 사용하며 모든 필수 후처리를 첫 생성 전에 등록한다.
    Impl(const GameAssets& game,const BaseFileSystem& files,FortTemplate fort,std::string mission,RawCampaignSessionServices input):
        services(std::move(input)),assets(game,files),metadata(assets.game),
        world(OriginalEdition::Patch1078,assets.types,assets.frames,metadata.shapes,CampaignLinks(),97),core(world.Core()),
        sp(random,[this] { return seed.Reseed(); }),hp(core.pool,core.types,core.bookkeeping,rewardState,&sp),seed(random,crt,randomMode,services.unixSeconds),themes(core.types.size()),elements(core.types.size()),
        gunTypes(core.types.size()),hitTypes(core.types.size()),firingIntervals(core.types.size()),dudeSpeeds(core.types.size()),
        soundNames(RequireSounds(services)),archerTypes(core.types.size()),missileTypes(core.types.size()),projectileTypes(core.types.size()),
        start{core.processState,priestState.movement.dude,priestState.movement.teleport,priestState.movement.manaBolt,priestState.movement.priest,geyserType,snap,retrying},process{core.processState} {
        sp.Initialize();level=std::make_unique<RawSquidLevel>(core.pool,core.types,SessionMissing<void,SquidLevelAssertion>());
        priestState.regen.patch.sequence=std::bit_cast<std::int32_t>(sp.Offset());
        priestState.regen.patch.observedSequence=priestState.regen.patch.sequence;
        finishState.chaffMode=true;objectState.mission=true;objectState.chaffMode=true;
        playerState.playerKnown=1;assetState.boss=true;assetState.inBattle=true;assetState.localOwner=1;gunState.boss=true;
        broadcast=std::make_unique<RawBattleBroadcast>(core.pool.Edition(),players,services.battlePeers,assetState.localOwner,assetState.boss);
        // 별도 관찰/장치 송신이 없으면 실제 분배기를 사용한다. NULL 연결은 원본 조건에서 자연스럽게 생략한다.
        if (!services.assets.sendGunPacket) services.assets.sendGunPacket=[this](std::span<const std::uint8_t> bytes) { broadcast->Send(bytes); };
        if (!services.pathPacket) services.pathPacket=[this](std::span<const std::uint8_t> bytes) { broadcast->Send(bytes); };
        if (!services.formPacket) services.formPacket=[this](std::span<const std::uint8_t> bytes) { broadcast->Send(bytes); };
        formWriter=std::make_unique<RawFormPacket>(core.pool,formHeaders,deletionStamp);
        // 실제 Form Pop이 정규화한 현재 상태 BYTE를 writer가 읽은 다음 같은 송신 분배기로 보낸다.
        if (!services.dependencyTransmit) services.dependencyTransmit=[this](Sid form,std::uint32_t) {
            const auto size=formWriter->WriteData(form,formBuffer); // 원본 반환 크기13/18이며 헤더의 선언 길이와 별개다.
            services.formPacket(std::span<const std::uint8_t>(formBuffer).first(size));
        };
        priestState.priests.priests.entries.resize(100);assetState.residences.entries.resize(100);
        assetState.temples.entries.resize(100);assetState.outposts.entries.resize(100);assetState.geysers.entries.resize(100);
        // 원본 Player의 첫 지역 sentinel은9기록에 직접 쓴다. 별도 playerRegions 배열을 사용하지 않는다.
        for (std::uint32_t owner=0;owner<9;++owner) {
            Put(players,owner*0xb4+0x78,127);Put(players,owner*0xb4+0x54,1U<<owner);
            gunTargetState.allyMasks[owner]=1U<<owner;gunState.allies[owner][owner]=true;
        }
        islands=std::make_unique<RawIslandRuntime>(core.pool,core.hash,islandBytes,finishState,assetState.residences,revision,ownerChanged,
            IslandRuntimeHooks{[this](Sid sid,std::uint32_t owner) { ownership->SetOwner(sid,owner); },SessionMissing<void,IslandRuntimeAssertion>()});
        islands->Reset();
        auto priestEffects=services.priest;
        priestEffects.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        priestEffects.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };
        priestEffects.repop=[this](Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
        priestEffects.destroy=[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        priestEffects.claimGround=[this](float x,float y,std::uint8_t owner) { world.SetSupportOwner(x,y,owner); };
        // 사제 Owner의 004214a0도 동일한 실제 무상태 다리 진단 몸체다. 별도 AI 통지로 대체하지 않는다.
        if (!priestEffects.ownerNotice) priestEffects.ownerNotice=[this](Sid sid) { core.building.NotifySurface(sid); };
        priestEffects.notifySurface=[this](Sid sid) { core.building.NotifySurface(sid); };
        priestEffects.dependentFound=SessionMissing<void>();priestEffects.reserveRegular=[] { return true; };
        priestEffects.shield.reserveSound=[] { return true; };
        sounds=std::make_unique<SoundProcessSystem>(core.processes,soundState,services.sound);
        priestEffects.shield.loadSound=[this](std::string_view name) { return soundNames.Lookup(name); };
        priestEffects.shield.attachSound=[this](Sid parent,std::uint32_t sound,std::uint32_t alternate,std::uint32_t flags) {
            if (!sounds->Add(parent,sound,alternate,flags)) throw std::logic_error("실제 보호막 Sound47 부착 실패");
        };
        FillMissing(priestEffects.shield.noticeSound);FillMissing(priestEffects.shield.translate);FillMissing(priestEffects.shield.tell);
        FillMissing(priestEffects.fallSound);FillMissing(priestEffects.otherWalkerFall);
        priestEffects.regen.refresh=[this](Sid sid) { core.display.Update(core.pool.Slot(sid)); };
        FillMissing(priestEffects.regen.neutralAction);
        // 실제 SP 회전 몸체의 현재 offset을 같은 회복 추적 전역에 반영한다. 저장 값/게임 난수를 유지한다.
        priestEffects.regen.patchAudit=[this] { sp.Rotate();priestState.regen.patch.observedSequence=std::bit_cast<std::int32_t>(sp.Offset()); };
        FillMissing(priestEffects.regen.measurement);
        priestEffects.regen.damageable=[this](Sid sid) { hits->Burning().Refresh(sid); };
        priests=std::make_unique<RawPriestWorldBindings>(RawPriestWorldCore{core.pool,core.hash,core.types,core.frames,core.spots,core.hash.Entries(0),hp,random,core.factory,core.owner,core.ownerMode,core.postPop,core.frame},priestState,std::move(priestEffects));
        priests->BindProcesses(core.processes);priests->SetAuthority(true);priests->SetLocalPlayer(1);
        PlayerWorldHooks playerHooks;
        playerHooks.assertion=SessionMissing<void,PlayerWorldAssertion>();
        playerHooks.find=[this](float x,float y,std::uint32_t type) { RawSquidFinder finder(core.pool,core.hash,core.types);return finder.Begin({static_cast<int>(x),static_cast<int>(y),static_cast<int>(x),static_cast<int>(y)},0,[this,type](Sid sid) { return core.pool.Slot(sid)[10]==type; }); };
        playerHooks.create=[this](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };
        playerHooks.setOwner=[this](Sid sid,std::uint32_t owner) { core.owner.Set(sid,owner); };
        playerHooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        playerHooks.reserveRegular=[] { return true; };
        playerHooks.addRegular=[this](Sid sid,std::uint32_t event,float payload) { SessionCheck(core.processes.AddRegular(sid,event,payload)!=nullptr); };
        player=std::make_unique<RawPlayerWorld>(core.pool,core.postPop,players,playerState,std::move(playerHooks));core.postPop.SetPlayerPostPop(player.get());
        templateState=std::make_unique<RawPlayerTemplateState>(core.pool.Edition(),headers,stateStreams,stateNames,
            objectState.loadingDepth,objectState.mission,SessionMissing<void,PlayerTemplateStateAssertion>(),[this](std::uint32_t depth) { world.SetLoadingDepth(depth); });
        auto playerRegularHooks=services.player;playerRegularHooks.assertion=SessionMissing<void,PlayerRegularAssertion>();
        FillMissing(playerRegularHooks.log);
        playerRegularHooks.reserveRegular=[] { return true; };
        playerRegularHooks.addRegular=[this](Sid sid,std::uint32_t event,float payload) { SessionCheck(core.processes.AddRegular(sid,event,payload)!=nullptr); };
        playerRegularHooks.templateState=[this](std::uint32_t slot) { return templateState->Get(slot); };
        playerRegularHooks.templateChanged=[this](std::uint32_t slot) { templateState->Changed(slot); };
        playerRegular=std::make_unique<RawPlayerRegular>(*player,core.ownerMode,std::move(playerRegularHooks));
        auto ownerEffects=services.ownership;
        ownerEffects.otherOwner=[this](Sid sid,std::uint32_t owner) { priests->SetOwner(sid,owner); };
        FillMissing(ownerEffects.refreshProduction);FillMissing(ownerEffects.themeMode);
        ownerEffects.missingIsland=SessionMissing<void,Sid,std::int32_t,std::uint32_t>();
        ownership=std::make_unique<RawCampaignOwnership>(world,*islands,assetState,players,metadata.frames,themes,random,paint,votes,vortexRevision,97,121,std::move(ownerEffects));
        carrier=std::make_unique<RawCarrierPostPop>(core.pool,core.types,carrierState,
            MakeCarrierPostPopHooks(core.pool,core.postPop,[this](Sid sid) { static_cast<void>(priests->CheckCarrier(sid)); },[this](float x,float y,std::uint8_t owner) { world.SetSupportOwner(x,y,owner); }));
        // 빈/내장 타입을 포함해 실제 TYPE+98의 판본 초기값과 파싱값을 같은 번호의 모든 소비자에게 전달한다.
        for (std::size_t type=0;type<core.types.size();++type) {
            gunTypes[type].theme=core.types[type].theme;themes[type]=std::bit_cast<std::uint32_t>(core.types[type].theme);elements[type]=core.types[type].theme;
        }
        // 보호 .type의 실제 숫자/문자 Run을 같은 TYPE 번호의 보충 표에 연결한다.
        for (const auto& asset:assets.game.Types()) {
            const auto type=static_cast<std::uint32_t>(70+asset.block);auto& target=gunTypes[type];target.range=static_cast<float>(asset.definition.Number("range").value_or(0));
            target.heightExtension=static_cast<int>(asset.definition.Number("height").value_or(0));
            dudeSpeeds[type].speed=static_cast<float>(asset.definition.Number("speed").value_or(0));
            // 원본 방향별 L/M/N/O 첫 프레임 표다. 없는Run의-1은 실제 검색 몸체가 처리한다.
            for (std::uint8_t direction=0;direction<4;++direction) target.directionFrames[direction]=core.frames[type].Run(static_cast<std::uint8_t>('L'+direction)).first;
            hitTypes[type].damagePerSecond=static_cast<float>(asset.definition.Number("hpPerSec").value_or(0));
            firingIntervals[type]=static_cast<float>(asset.definition.Number("delayBetweenShots").value_or(0));
            archerTypes[type]=ReadRawArcherTypeData(asset.definition);missileTypes[type].range=target.range;
            missileTypes[type].frameCount=archerTypes[type].directions;
            const auto firingSound=asset.definition.String("firingSound"),impactSound=asset.definition.String("impactSound");
            missileTypes[type].firingSound=firingSound && !firingSound->empty() ? soundNames.Lookup(*firingSound) : 0;
            missileTypes[type].impactSound=impactSound && !impactSound->empty() ? soundNames.Lookup(*impactSound) : 0;
            auto& geometry=projectileTypes[type];geometry.defaultFrame=asset.definition.specialFrames.defaultFrame;
            geometry.frameCount=archerTypes[type].frameCount;geometry.heightExtension=target.heightExtension;
            // 각 물리 SHP header 마지막 두 WORD를 그대로 읽으며 표시 hotspot과 혼용하지 않는다.
            for (std::size_t frame=0;frame<core.shapes[type].frames.size();++frame) {
                const auto header=assets.game.Shapes().SquidHeader(asset.block,frame);
                geometry.shotOffsets.push_back({std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(header[32]|(std::uint16_t(header[33])<<8))),
                    std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(header[34]|(std::uint16_t(header[35])<<8)))});
            }
        }
        gunMode=std::make_unique<RawGunMode>(core.pool,core.frames,gunState.windArcherType);
        gunTarget=std::make_unique<RawGunTarget>(core.pool,core.hash,core.types,gunTypes,core.shapes,gunTargetState);
        priestDamageState.levelCorrectionDisabled=globals.missionPost;priestDamageState.boss=assetState.boss;
        auto hitEffects=services.hit;
        hitEffects.priest.defaultFrame=[this](Sid sid) { return projectileTypes[core.pool.Slot(sid)[10]].defaultFrame; };
        playerDamage=std::make_unique<RawPlayerDamageNotice>(players,core.ownerMode);
        // 실제 Player AI가 없는 자연 반환은 래퍼가 판단한다. 비영 AI의 미복원 몸체는 명시 오류를 유지한다.
        if (!hitEffects.priest.ownerDamageNotice) hitEffects.priest.ownerDamageNotice=
            [this](std::uint8_t owner,Sid victim,Sid attacker) { playerDamage->Notice(owner,victim,attacker); };
        FillMissing(hitEffects.burning.create);FillMissing(hitEffects.burning.bulfSound);
        FillMissing(hitEffects.dudes.createBurning);FillMissing(hitEffects.dudes.pathEndpoint);FillMissing(hitEffects.dudes.startPath);
        hits=std::make_unique<RawCampaignHitBindings>(world,*priests,hp,random,dudeSpeeds,assetState.residences,
            priestDamageState,burningState,impactState,std::move(hitEffects));
        gunHit=std::make_unique<RawGunHit>(core.pool,core.types,hitTypes,hits->DamageHooks(GunHitHooks{SessionMissing<void,Sid,std::int32_t,std::uint8_t,Sid>(),[this](Sid sid) { core.building.NotifySurface(sid); }}));
        projectileGeometry=std::make_unique<RawProjectileGeometry>(core.pool,core.types,projectileTypes);
        fenceTarget=std::make_unique<RawFenceTarget>(core.pool,core.hash,core.types,projectileTypes,*gunTarget,gunTargetState,gunState);
        auto combatEffects=services.archer;
        // 실제 SID의 현재 좌표와 같은 이름표/위치 재생 함수를 사용한다. 설치 전 world_ UI 포인터를 캡처하지 않는다.
        if (!combatEffects.animation.common.sound) combatEffects.animation.common.sound=[this](Sid sid,std::string_view name) {
            const auto raw=core.pool.Slot(sid); // 비정렬 원본 float 좌표를 읽는 현재 부모 슬롯이다.
            std::array<std::uint32_t,2> bits{}; // little endian x/y의 원본 DWORD 비트다.
            // 두 좌표의 각 네 바이트를 정확한 자리로 조립한다.
            for (std::size_t i=0;i<4;++i) { bits[0]|=std::uint32_t(raw[14+i])<<(8*i);bits[1]|=std::uint32_t(raw[18+i])<<(8*i); }
            services.sound.playOnceAt(std::bit_cast<float>(bits[0]),std::bit_cast<float>(bits[1]),soundNames.Lookup(name),0,0);
        };
        if (!combatEffects.animation.common.targetPoint) combatEffects.animation.common.targetPoint=
            [this](Sid firing,Sid target) { return fenceTarget->Point(firing,target); };
        combatEffects.missile.muzzle=[this](Sid sid) { return projectileGeometry->ArcherMuzzle(sid); };
        combatEffects.missile.targetDimensions=[this](Sid sid) { return projectileGeometry->Dimensions(sid); };
        combatEffects.missile.targetBounds=[this](Sid sid,std::array<float,2> point) { return projectileGeometry->TargetBounds(sid,point); };
        FillMissing(combatEffects.missile.firingSound);FillMissing(combatEffects.missile.impactSound);
        combatEffects.missile=hits->MissileHooks(*gunHit,missileState,std::move(combatEffects.missile));
        combat=std::make_unique<RawCampaignArcherBindings>(world,*gunTarget,random,archerTypes,missileTypes,archerState,missileState,std::move(combatEffects));
        auto assetEffects=services.assets;assetEffects.postPop.reserveRegular=[] { return true; };
        // 레지던스는 현재 일치 신전을 찾은 호출 위치에서 실제 theammode를 다시 읽는다.
        if (!assetEffects.regular.themeMode) assetEffects.regular.themeMode=services.ownership.themeMode;
        FillMissing(assetEffects.regular.themeMode);
        assetEffects.gun=MakeCampaignGunQueryHooks(core,*gunMode,*gunTarget,*gunHit,gunState,firingIntervals,std::move(assetEffects.gun));
        assetEffects.gun=MakeGunTargetHooks(core.pool,*gunTarget,std::move(assetEffects.gun));
        assetEffects.gun=combat->GunHooks(std::move(assetEffects.gun));
        assetEffects.factory.warnDuplicate=SessionMissing<void,Sid,std::int32_t,std::int32_t>();
        assetEffects.levelAssertion=SessionMissing<void,SquidLevelAssertion>();
        assetEffects.factory.regionOwnership=[this](float x,float y) { ownership->UpdateRegion(x,y); };
        assetEffects.postPop.setOwner=[this](Sid sid,std::uint32_t owner) { ownership->SetOwner(sid,owner); };
        // PATCH0040d980은 현재float를절삭한 뒤16으로 나눈 청크의 실제 BYTE를 읽는다.
        assetEffects.postPop.regionNumber=[this](float x,float y) { return static_cast<std::int32_t>(chunks.At(static_cast<int>(x)/16,static_cast<int>(y)/16).island); };
        // 실제 TYPE 사운드 이름을 클라이언트의 단일 이름표에서 조회한다. TYPE번호를 토큰으로 대신하지 않는다.
        assetEffects.postPop.typeDword=[this](Sid sid,std::uint32_t offset) {
            if (offset!=0x74) throw std::logic_error("세션 TYPE 사운드 필드 미지원");
            const auto name=assets.game.Types()[core.pool.Slot(sid)[10]-70].definition.String("impactSound");
            return name && !name->empty() ? soundNames.Lookup(*name) : 0U;
        };
        FillMissing(assetEffects.postPop.playImpactSound);FillMissing(assetEffects.queueCommand);FillMissing(assetEffects.sendGunPacket);
        assetBindings=std::make_unique<RawCampaignAssetBindings>(world,*carrier,assetState,random,*islands,players,playerState,RawCampaignAssetGlobals{regularState,gunState,elements,archerCommand},std::move(assetEffects));assetBindings->ActivatePostPop();
        RawWorldRuntimeHooks worldHooks;
        worldHooks.formTransmit=services.dependencyTransmit;
        FillMissing(worldHooks.formTransmit);
        worldHooks.assets=services.deletion;FillMissing(worldHooks.assets.selected);
        // 공통 삭제의 -1/-2/0/flags 요청은 실제+58을 실행한다. 다른 파생/선택 효과는 실제 상위 소비자로 전달한다.
        worldHooks.assets.emit=[this,other=services.deletion.emit](const SquidDestroyEvent& event) {
            if (event.effect==SquidDestroyEffect::Transmit) {
                const auto size=formWriter->WriteDeletion(event.sid,event.flags,formBuffer); // 아직 반납 전인 같은 SID다.
                services.formPacket(std::span<const std::uint8_t>(formBuffer).first(size));return;
            }
            if (!other) throw std::logic_error("실제 세션의 다른 파생 삭제/선택 효과 미연결");
            other(event);
        };
        worldHooks.assets=hits->DestroyHooks(std::move(worldHooks.assets));
        worldHooks.destroy=hits->Destroy(services.destroy);
        worldHooks.regular=hits->Handler(combat->Handler(assetBindings->Handler(priests->RegularHandler([this](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
            if (playerRegular->Handles(sid)) return playerRegular->Handle(sid,event,count,payload);
            throw std::logic_error("캠페인 현재 Kernel의 다른 실제 Regular 부모 미연결: TYPE"+
                std::to_string(core.pool.Slot(sid)[10])+" event"+std::to_string(event));
        }))));
        worldHooks.transmit=assetBindings->Transmission();worldHooks.walkerFall=[this](Sid sid) { return priests->FallWalker(sid); };
        worldHooks.loadingDepth=[this](std::uint32_t depth) { objectState.loadingDepth=depth;carrierState.loadingDepth=depth;priests->SetLoadingDepth(depth);assetBindings->SetLoadingDepth(depth); };
        world.BindRuntime(std::move(worldHooks));
        placement.geometry=metadata.geometry;placement.placement.localPlayer=1;placement.permission.alliances.fill(1);
        pipeline=std::make_unique<RawCanonPlacementPipeline>(core.pool,core.hash,CanonPlacementPipelineData{core.types,metadata.frames,core.shapes,metadata.hotspots,core.spots,core.hash.Entries(0),{}},core.bookkeeping,additional,contentFinder,placement);
        NormalTerrainDiagnostics diagnostics{SessionMissing<void,TerrainContextAssertion>(),SessionMissing<void,TerrainStampAssertion>(),SessionMissing<void,SquidRegionAssert>(),SessionMissing<void,FarmClusterAssertion>(),SessionMissing<void,TerrainFarmPostpassAssertion>()};
        normal=std::make_unique<RawNormalTerrainWorldBinding>(world,chunks,*islands,random,crt,terrainGlobals,stamp,randomPlacement,farm,piece,*pipeline,hp,*carrier,std::move(diagnostics));
        finish=std::make_unique<RawFortFinishOwner>(core.pool,core.types,assetState.residences,finishState,FortFinishOwnerHooks{SessionMissing<FortRandomRectangle,std::int32_t>(),SessionMissing<double>(),SessionMissing<std::uint32_t,GameRandom&,const FortRandomPlaceRequest&>(),SessionMissing<void>()});
        ContainedCreateHooks contentHooks;contentHooks.notifySurface=[this](Sid sid) { core.building.NotifySurface(sid); };
        contentNotice=std::make_unique<RawContentParentNotice>(core.pool,services.contentNotice);
        contentForms=std::make_unique<ContentFormHost>(core.pool,core.types,
            [this](Sid parent,Sid form,std::uint32_t reason) { contentNotice->Notice(parent,form,reason); },services.dependencyTransmit);
        contained=std::make_unique<RawContainedCreate>(core.pool,contentState,contentForms->BindCreate(core.factory,std::move(contentHooks)));
        templates[1]=std::move(fort);
        const auto state=templates[1].Section("State");stateBytes[1].assign(state.begin(),state.end());
        stateStreams[1]={stateBytes[1],0,static_cast<std::uint32_t>(stateBytes[1].size()),!stateBytes[1].empty()};stateNames[1]=std::string(mission);
        streams[1]={templates[1].territory,0,static_cast<std::uint32_t>(templates[1].territory.size())};headers[20]=1;
        runtime=MakeRuntime();
    }
    // 외부 config/UI는 현재 클라이언트 서비스를 사용하고 게임 계산은 동일 원본 모듈을 사용한다.
    std::unique_ptr<RawCampaignWorldRuntime> MakeRuntime(std::size_t templateCount=9) {
        auto hooks=services.install;
        FillMissing(hooks.translateYou);FillMissing(hooks.setConfig);
        FillMissing(hooks.refreshWindowTitle);FillMissing(hooks.resetCamera);
        hooks.builderAssertion=SessionMissing<void,RawIslandBuilderAssertion>();hooks.territoryAssertion=SessionMissing<void,FortWorldLoadAssertion>();
        hooks.notifySurface=[this](Sid sid) { core.building.NotifySurface(sid); };
        hooks.setState=[this](Sid sid,std::uint32_t value) { assetBindings->SetState(sid,value); };
        hooks.virtualCount=[this](Sid sid) { return static_cast<std::int32_t>(level->Get(sid)); };
        return std::make_unique<RawCampaignWorldRuntime>(world,chunks,*islands,*player,*normal,random,hp,*ownership,*finish,*contained,objectState,globals,piece,noRegion,RawCampaignInstallData{std::span<const FortTemplate>(templates).first(templateCount),streams,headers,handle},std::move(hooks));
    }
    // 설치를 마친 동일 Graph/Host에 실제 사제 경로 모듈을 한 번 연결한다.
    void ConnectMotion() {
        if (motion) throw std::logic_error("실제 세션 경로 중복 연결");
        motionState.halt.battle=true;anchorState.graphReady=1;anchorState.fenceType=90;anchorState.windArcherType=140;
        anchor=std::make_unique<RawPlayerPlacementAnchor>(core.pool,core.types,core.hash.Entries(0),core.bookkeeping,additional,contentFinder,anchorState);
        auto effects=services.motion;
        if (!effects.cleanup.reserveShared) effects.cleanup.reserveShared=[] { return true; };
        FillMissing(effects.cleanup.ritual);FillMissing(effects.cleanup.refund);
        FillMissing(effects.halt.carrierPickup);FillMissing(effects.halt.nuggetTransfer);
        effects.carrier.genusAt=[this](float x,float y,std::uint32_t genus) {
            RawSquidFinder finder(core.pool,core.hash,core.types);
            // 실제 일반 getter의 첫 현재 후보/표면 생략 순서를 사용한다.
            for (auto sid=finder.Begin({static_cast<int>(x),static_cast<int>(y),static_cast<int>(x),static_cast<int>(y)},(genus&6)==0 ? 1U : 0U);sid.value;sid=finder.Next())
                if (core.types[core.pool.Slot(sid)[10]].flags2&genus) return sid;
            return Sid{};
        };
        FillMissing(effects.carrier.cdPriestPickupPriest);FillMissing(effects.carrier.pickup);FillMissing(effects.carrier.escape);
        FillMissing(effects.carrier.chooseReturn);FillMissing(effects.carrier.chooseGeyser);FillMissing(effects.carrier.goReturn);FillMissing(effects.carrier.goGeyser);
        FillMissing(effects.carrier.minedPacket);FillMissing(effects.carrier.cdAddSp);FillMissing(effects.carrier.cdOwnerPacket);FillMissing(effects.carrier.playAt);FillMissing(effects.carrier.robust);
        FillMissing(effects.priest.neutralReturn);
        effects.path.metadata=[this](Sid sid) {
            const auto& definition=assets.game.Types()[core.pool.Slot(sid)[10]-70].definition;
            return RawPathStartType{static_cast<float>(definition.Number("speed").value()),static_cast<float>(definition.Number("turningSpeed").value()),definition.specialFrames.defaultFrame};
        };
        effects.path.soundId=[this](Sid sid) {
            const auto sound=assets.game.Types()[core.pool.Slot(sid)[10]-70].definition.String("stepSound");
            return sound && !sound->empty() ? soundNames.Lookup(*sound) : 0U;
        };
        FillMissing(effects.path.stepSound);FillMissing(effects.path.startBlocked);FillMissing(effects.path.arrivalBlocked);FillMissing(effects.path.retarget);
        motion=std::make_unique<RawCampaignMotionBindings>(world,*priests,*islands,*contained,*anchor,hp,metadata.frames,motionState,start,process,std::move(effects));
        FillMissing(services.pathPacket);
        auto pathTx=motion->Paths().Transmission(pathHeader,pathBuffer,[this](std::int32_t length) {
            if (length<=0 || static_cast<std::size_t>(length)>pathBuffer.size()) throw std::logic_error("세션 Path 송신 길이 오류");
            services.pathPacket(std::span<const std::uint8_t>(pathBuffer).first(static_cast<std::size_t>(length)));
        });
        core.processes.SetFormTransmit(assetBindings->Transmission(std::move(pathTx)));
    }
    // 외부 콜백이 참조하는 모듈을 해제하기 전에 현재 Kernel을 비운다.
    ~Impl() { world.ClearProcesses();core.postPop.SetPlayerPostPop(nullptr); }

};
// 현재 실제 자산/Template/서비스를 보관하고 모든 하위 모듈을 구성한다.
RawCampaignSession::RawCampaignSession(const GameAssets& assets,const BaseFileSystem& files,FortTemplate fort,std::string mission,RawCampaignSessionServices services)
    :impl_(std::make_unique<Impl>(assets,files,std::move(fort),std::move(mission),std::move(services))) {}
// Impl이 Kernel을 비운 뒤 실제 모듈을 해제한다.
RawCampaignSession::~RawCampaignSession()=default;
// 동일 원본 부모를 실행한다.
void RawCampaignSession::Load() { impl_->runtime->Load();impl_->ConnectMotion(); }
// UI의 현재 크기/변경 전역을 빌리며 게임 소유자·모드는 실제 세션에서 다시 읽는다.
void RawCampaignSession::ResetCamera(CameraHomeState& state,std::function<void(std::int32_t,std::int32_t)> apply) const {
    state.localPlayer=impl_->assetState.localOwner;state.fort=impl_->core.ownerMode.fort;
    state.battle=impl_->core.ownerMode.battle;state.challenge=impl_->core.ownerMode.challenge;
    CameraHome(impl_->core.pool,impl_->core.types,state,std::move(apply)).Reset();
}
// 현재 같은 시각을 공격/소리/실제 Kernel에 공급한다.
void RawCampaignSession::RunFrame(FrameTime time,bool paused) { impl_->assetBindings->PrepareFrame(time.game);impl_->combat->PrepareFrame(time);impl_->soundState.frame=static_cast<std::uint32_t>(time.number);impl_->world.RunFrame(time,paused); }
// 실제 동일 Host54/BFS 경로를 시작하며 원본 재귀의 외부nullptr 뒤 현재 경로도 확인한다.
bool RawCampaignSession::MovePriest(Sid sid,RawPathPoint destination) {
    if (!impl_->motion) throw std::logic_error("실제 세션 설치 전 사제 명령");
    auto* path=impl_->motion->Paths().StartPriest({sid,destination,1,0,0,0,0});
    return path!=nullptr || impl_->motion->Paths().CurrentPath(sid)!=nullptr;
}
// Draw의 조건부 소리도 기존 세션의 단일 실제 이름표와 장치 경계를 사용한다.
void RawCampaignSession::DrawRainTemple(Sid sid,const std::function<void(std::int32_t)>& frame) const {
    const auto raw=impl_->core.pool.Slot(sid);
    DrawRainVortex(impl_->core.pool.Edition(),raw,impl_->core.frames[raw[10]],{frame,[this] {
        if (!impl_->services.sound.play) throw std::logic_error("레인 신전 상승음 실제 재생 미연결");
        impl_->services.sound.play(impl_->soundNames.Lookup("templeAscend.wav"),0,0,0,0,0);
    }});
}
// 화면과 입력은 같은 살아 있는 상태를 읽는다.
RawSurfaceWorld& RawCampaignSession::World() { return impl_->world; }
RawWorldCore RawCampaignSession::Core() { return impl_->core; }
const SquidReward& RawCampaignSession::HitPoints() const { return impl_->hp; }
const RawPriestWorldBindings& RawCampaignSession::Priests() const { return *impl_->priests; }
}
