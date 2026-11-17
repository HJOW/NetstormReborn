// RawCampaignSession::Impl의 캠페인 소유 자료를 실제 하위 생성자로 구성한다(225553df, WB04-E574).
// 사용: owner 생성 → initializePlayersAndSp → connectPlayerTablesAndPackets → connectIslandsAndPriests → connectPlayersAndOwnership → connectBodiesAndTypeTables → connectGunAndCursorViews → connectHitBodies → connectGunBodies → connectParticles → connectTeleport → connectCombat → connectVortex → connectAssets → connectWorldRuntimeHooks → connectContentNotices → connectContentLifetime → connectProduction → connectFactories → connectFlare → connectConstruction → connectDamageable → connectIceTower → connectCarrierSteal → connectBombPostPop → connectAcidZap → connectAcidFence → connectCarrierRegular → connectSunAviary → connectWindAviary → connectRainAviary → 나머지 모듈/Load.
// 이 클래스는 Impl 부품이다. connectTerrain은 World 내부 훅만 게시하고 외곽 공개는 raw-campaign-session.js가 맡는다. Load는 후속이다.
// Editor/LAN/메타/Native 장치는 이 캠페인 분기에 섞지 않는다. 반납은 하위 모듈을 정리한 부모가 마지막에 dispose로 한다.
import { GameAssets } from '../assets/game-assets.js';
import { FrameCodeTable } from '../assets/terrain-tiles.js';
import { buildTypeRecords } from '../assets/type-flags.js';
import { CampaignTemplateStorage } from './campaign-template-storage.js';
import { ClientChatWindowGlobals } from './client-chat-window-globals.js';
import { ClientConnectionActivityGlobals } from './client-connection-activity-globals.js';
import { ClientCoreDataGlobals } from './client-core-data-globals.js';
import { ClientGameplayGlobals } from './client-gameplay-globals.js';
import { ClientLocalStateGlobals } from './client-local-state-globals.js';
import { ClientPlayerRecords } from './client-player-records.js';
import { FortTypeConversionStorage } from './fort-type-conversion-storage.js';
import { createFortObjectLoadStateFromGlobals } from './fort-object-load.js';
import { PriestPlacementAssets } from './priest-placement-assets.js';
import { BridgeConnectState } from './raw-bridge-connect.js';
import { RawAiBrainMemory } from './raw-ai-brain.js';
import { createFortFinishOwnerState } from './raw-island-runtime.js';
import { PlayerParticipationState, RawPlayerParticipation } from './raw-player-participation.js';
import { RawPriestWorldState } from './raw-priest-world-bindings.js';
import { RawSquidLevel, makeSquidLevelAssertion } from './raw-squid-level.js';
import { RawSurfaceWorld } from './raw-surface-world.js';
import { RawCampaignSessionState } from './raw-campaign-session-state.js';
import { RawCampaignSessionTables } from './raw-campaign-session-tables.js';
import { RawCampaignSessionPriests } from './raw-campaign-session-priests.js';
import { RawCampaignSessionPlayers } from './raw-campaign-session-players.js';
import { RawCampaignSessionTypes, createCampaignTypeTables } from './raw-campaign-session-types.js';
import { RawCampaignSessionGunViews, createCampaignCursorTables } from './raw-campaign-session-gun-views.js';
import { RawCampaignSessionHits } from './raw-campaign-session-hits.js';
import { RawCampaignSessionGuns } from './raw-campaign-session-guns.js';
import { RawCampaignSessionParticles, createCampaignParticleTables } from './raw-campaign-session-particles.js';
import { RawCampaignSessionTeleport } from './raw-campaign-session-teleport.js';
import { RawCampaignSessionCombat, copyCampaignArcherEffects } from './raw-campaign-session-combat.js';
import { writeCampaignTerritory } from './raw-campaign-territory-write.js';
import { RawCampaignVortex } from './raw-campaign-vortex.js';
import { RawCampaignSessionAssets, copyCampaignAssetEffects } from './raw-campaign-session-assets.js';
import { RawCampaignSessionRuntime } from './raw-campaign-session-runtime.js';
import { RawCampaignSessionContents } from './raw-campaign-session-contents.js';
import { RawCampaignSessionProduction } from './raw-campaign-session-production.js';
import { RawCampaignSessionFactories } from './raw-campaign-session-factories.js';
import { RawCampaignSessionFlare } from './raw-campaign-session-flare.js';
import { RawCampaignSessionConstruction, copyCampaignConstructionEffects } from './raw-campaign-session-construction.js';
import { RawCampaignSessionDamageable } from './raw-campaign-session-damageable.js';
import { RawCampaignSessionIce } from './raw-campaign-session-ice.js';
import { RawCampaignSessionCarrierSteal } from './raw-campaign-session-carrier-steal.js';
import { RawCampaignSessionSpells } from './raw-campaign-session-spells.js';
import { RawCampaignSessionCarrierRegular } from './raw-campaign-session-carrier-regular.js';
import { RawCampaignSessionAviaries } from './raw-campaign-session-aviaries.js';
import { RawCampaignSessionArcSpire } from './raw-campaign-session-arc-spire.js';
import { RawCampaignSessionFlyers } from './raw-campaign-session-flyers.js';
import { RawCampaignSessionFence } from './raw-campaign-session-fence.js';
import { RawCampaignSessionGems } from './raw-campaign-session-gems.js';
import { GemRewardState } from './raw-gem-reward.js';
import { SquidPostPopList } from './squid-postpop-state.js';
import { RawCampaignSessionDeletion } from './raw-campaign-session-deletion.js';
import { RawCampaignSessionRegular } from './raw-campaign-session-regular.js';
import { RawCampaignSessionTransmission } from './raw-campaign-session-transmission.js';
import { RawCampaignSessionTerrain, createCampaignTerrainState } from './raw-campaign-session-terrain.js';
import { prepareCampaignFortTypeNames, readCampaignFortObjectStreams } from './raw-campaign-session-streams.js';
import { RawCampaignSessionInstall, createCampaignTutorialRoles } from './raw-campaign-session-install.js';
import { RawCampaignSessionAi, createCampaignAiState } from './raw-campaign-session-ai.js';
import { RawCampaignSessionAiWorld } from './raw-campaign-session-ai-world.js';
import { RawCampaignSessionAiProduction } from './raw-campaign-session-ai-production.js';
import { snapshotCampaignFortBasicSection } from './raw-campaign-snapshot-basics.js';
import { writeCampaignFortTechnology } from './raw-campaign-content-save.js';
import { writeCampaignFortObjectChunk } from './raw-campaign-object-save.js';
import { writeCampaignFortObjects } from './raw-campaign-objects-save.js';
import { writeCampaignFortTerritory } from './raw-campaign-territory-save.js';
import { refreshCampaignPlayerCore, writeCampaignFortCoreData } from './raw-campaign-core-write.js';
import { snapshotCampaignTemplate } from './raw-campaign-snapshot.js';
import { generateCampaignLocalGeysers, generateCampaignWorldGeysers, placeCampaignBattleNuggets, learnCampaignNewTechnology } from './raw-campaign-callbacks.js';
import { RawCampaignSessionBuilding, createCampaignBuildingTables, drawCampaignTempleFrames } from './raw-campaign-session-building.js';
import { RawCampaignSessionArrivals } from './raw-campaign-session-arrivals.js';
import { RawCampaignSessionMotion, copyCampaignMotionEffects } from './raw-campaign-session-motion.js';
import { RawCampaignSessionCarrierRouting } from './raw-campaign-session-carrier-routing.js';
import { RawCampaignSessionMotionRuntime } from './raw-campaign-session-motion-runtime.js';
import { RawCampaignSessionDevastation } from './raw-campaign-session-devastation.js';
import { RawCampaignSessionMeltdown } from './raw-campaign-session-meltdown.js';
import { RawCampaignSessionCarrierDrop } from './raw-campaign-session-carrier-drop.js';
import { RawCampaignSessionPanic } from './raw-campaign-session-panic.js';
import { RawCampaignSessionRitual } from './raw-campaign-session-ritual.js';
import { RegionPaintState } from './raw-region-paint.js';
import { RegionOwnershipState } from './raw-region-ownership.js';
import { CrtRandom, GameRandom, GameRandomSeed } from './random.js';
import { ScrambledSpStore } from './sp-store.js';
import { SquidReward, SquidRewardState } from './squid-reward.js';

// Impl 멤버의 참조 이름과 장기 GameAux 칸이다. getter/setter는 주소 역할만 하며 값 사본을 만들지 않는다.
const ROLE_ALIASES = Object.freeze({ terrainType: 'terrainType', geyserType: 'geyserType', freeTechnologyType: 'freeTechnologyType', templeProduct: 'templeProductType', flareType: 'bombFlareType', fenceShieldType: 'fenceShieldType', fenceNodeType: 'fenceMarkerType', sunAviaryType: 'sunAviaryProcessType', windAviaryType: 'windAviaryProcessType', rainAviaryType: 'rainAviaryProcessType' });

// 원문 CampaignLinks의 실제 초기 다리/섬 종류와 캠페인 모드다.
function campaignLinks() { return Object.assign(new BridgeConnectState(), { bridgeType: 82, islandType: 94, stalagType: 95, connectorType: 155, noIslandType: 157, battle: true, mission: true }); }

// 대여/fresh 전 같은 장기 Subscriber·권한·보조 전역인지 확인한다(원문344..362의 캠페인 분기).
function gameplayOwner(input) {
    if (input.installMode !== undefined && input.installMode !== 'campaign') throw new TypeError('캠페인 소유 자료의 설치 모드 불일치');
    if (input.adoptPreparedWorld || input.editorTransport || input.deferExistingEditorFile || input.generalBattleStart) throw new TypeError('캠페인의 준비된 Editor/LAN/메타 대여는 지원하지 않습니다');
    const gameplay = input.gameplayGlobals; const storage = input.surfaceStorage;
    if (gameplay && gameplay.subscribers() !== (input.subscriberGlobals ?? null)) throw new TypeError('Session의 실제 현재 전역/Subscriber 소유자 불일치');
    if (gameplay && storage && gameplay.authority() !== storage.authority) throw new TypeError('Session의 실제 현재 전역/표면 권한 소유자 불일치');
    if (gameplay && storage && gameplay.gameAux() !== storage.gameAux) throw new TypeError('Session의 실제 게임 보조 owner 불일치');
    return gameplay ?? new ClientGameplayGlobals(input.subscriberGlobals ?? null, storage?.authority ?? null, storage?.gameAux ?? null);
}

// SessionAssets는 같은 보호 자산과 VFS를 빌리고 현재188TYPE/프레임 표만 별도로 소유한다.
export class CampaignSessionAssets {
    // 현재 GameAssets와 VFS, 최초 로딩 표, 수정 가능한 현재 TYPE, 실제 프레임 코드 표다.
    game; files; loadedTypes; types; frames;
    // GameAssets의 PATCH 전용116블록을 확인한 뒤 번호70부터 원문 순서로 프레임을 채운다.
    constructor(game, files) {
        if (!(game instanceof GameAssets)) throw new TypeError('Session의 실제 PATCH GameAssets 필요');
        this.game = game; this.files = files; this.loadedTypes = buildTypeRecords(game.typeTable.definitions);
        this.types = this.loadedTypes.map((record) => ({ ...record, minUsage: [...record.minUsage], maxUsage: [...record.maxUsage] }));
        this.frames = this.types.map(() => new FrameCodeTable([]));
        // 보호 로딩 번호를 유지하고 검사 전용 프레임 수정은 하지 않는다.
        for (const asset of game.types()) {
            if (!asset.definition) throw new Error(`Session의 실제 TYPE 정의 누락: ${asset.assetName}`);
            this.frames[70 + asset.block] = FrameCodeTable.fromDefinition(asset.definition);
        }
    }
}

// 실제 Impl의 기반 owner와 고정뷰 대여다. 생성 자체로 게임/Fort를 설치하지 않고 부모가 명시한 순서로 World 훅을 연결한다.
export class RawCampaignSessionOwners {
    // 대여를 확보한 순서와 역순 반납을 유지하는 실제 소유자다.
    gameplayGlobals; gameplayUse = null; coreDataGlobals; playerRecords; playerUse = null;
    // 값형 서비스 표와 미션 이름, 장기 공유 자료 owner다. 서비스 표의 참조 대상은 복사하지 않는다.
    services; missionFortName; fortTypeConversion; chatGlobals; localStateGlobals; connectionActivityGlobals;
    // 실제 Template 헤더/자산/배치 메타데이터와 단일 월드·그 실제 코어 참조다.
    templateStorage; headers; assets; metadata; world = null; core;
    // 같은 장기 월드 물리의 청크/섬 바이트·Dude 목록과 재사용 지형 도장 캐시다.
    chunks; islandBytes; dudePopulation; stamp;
    // 같은 게임 전역의 현재 상태와 독립 캠페인의 실제 사제/보상/Fort 상태다.
    playerState; globals; cursorDrop; energyState; contentFinder; priestState; rewardState; objectState; finishState;
    // 독립 난수의 CRT0/Game0/mode0와 선택형 Client 공유 스트림, 실제 SP/HP·AI 할당 owner다.
    randomGlobals; random; particleCrt; seed; localSp; sp; hp; aiMemory;
    // 같은 Player9/81동맹/옵션87·숨김9·금지256 물리다. players는 원문 초기화 위치에서만 게시한다.
    players = new Uint8Array(0); alliances; battleOptions; hiddenPlayers; restrictions; participationState;
    // 상위가 실제 가상80 분배를 연결하며, 미연결 시 선택된 호출에서 오류다.
    rewardCount = null; soundNames; level = null;
    // 같은 Impl의 공유 입력·현재 상태와 Player/SP 이후 진단 결선이다.
    state; tables; priestModules = null; playerModules = null; typeModules = null; gunViewModules = null; hitModules = null; gunModules = null; particleModules = null;
    // 현재 Client의 같은 redraw 상자 또는 독립 Session의 지역0 상자다.
    worldRedraw;
    // 같은 장기 redraw/개정 상자를 빌리는 지역 도장/투표와 생성 시0인 TYPE별 두 벡터다.
    paint; votes; themes; elements;
    // 원문 선언/1004의 같은188칸 보충 벡터다. 실제 TYPE 채움은 후속 몸체/TYPE 구간에서 수행한다.
    builderTypes; gunTypes; hitTypes; firingIntervals; dudeSpeeds; archerTypes; missileTypes; projectileTypes;
    // 실제 생성 순서에서 채우는 섬·소리·사제와 참여 조회 모듈이다.
    islands = null; sounds = null; priests = null; priestParticipation = null;
    // 같은 Player9/Template를 빌리는 Player152·현재 State·정기 처리기다.
    player = null; templateState = null; playerRegular = null;
    // 후속 생성자가 설치하는 실제 순환 참조다. 현재 구간은 대체 객체를 만들지 않는다.
    ownership = null; hits = null; construction = null; placeBattleNuggets = null;
    carrierRegular = null; balloonBody = null; carrierFallStart = null; priestPanic = null;
    // 같은 공통 Carrier 후처리와 클라우드 플로터의 미부착 Regular 한 번 예약이다.
    carrier = null; pendingCloudRegular = null;
    // 같은 장기 좌표/반경과30칸 선택 목록, 실제 총 검색·공급·커서 표시 논리다.
    cursorManaGlobals; cursorRangeGlobals; energySources;
    gunMode = null; gunTarget = null; energySupply = null; cursorMana = null; cursorRange = null;
    // 같은 실제 AI 통지와 후속 피격 콜백이 호출 때 선택하는 모듈 슬롯이다.
    aiDamage = null; playerDamage = null; flyer = null; particles = null; motion = null; building = null;
    // Load의 첫 건설 모듈과 같은 현재30/17 수신/밀어내기 효과다. 생성자 자체는 설치하지 않는다.
    buildingModules = null; buildingData = null; buildingEffects = null;
    // 같은 실제 피해·투사체/배리어 기하와 기존 Gun 장애물 통지, 후속 자산 처리기의 현재 슬롯이다.
    gunHit = null; projectileGeometry = null; fenceTarget = null; gunBlocker = null; assetBindings = null;
    // 파편의 실제 지역 색표/현재 캡처 옵션·다리 그림이며 건물/공장 캡처는 후속 생성 구간의 같은 슬롯이다.
    shrapnelMapTokens; shrapnelOwnerColors; bridgeDrawState; shrapnelCaptureState;
    bridgeDraw = null; shrapnelCapture = null; buildingCapture = null; factoryDraw = null;
    // Teleport 접두와 Init/부착하지 않은 cdecl 반환값이다. 펜스 전체 몸체는 후속 설치 구간의 현재 슬롯이다.
    teleportModules = null; unstartedBombTimers = []; unstartedPaths = []; fenceRebuild = null;
    // 같은 Host의 Archer/Missile과 실제 반사 처리기, 보호 PE5429c8의 튜토리얼 충돌 명령이다.
    combatModules = null; combat = null; iceRicochet = null; tutorialTechnologyCommand = 0x020000b5;
    // 같은 실제 신전 상승 부모와 현재 Runtime/snapshot/공통 명령 슬롯이다. 상위 Load가 나중에 채운다.
    vortexModules = null; vortexAscend = null; vortexDeparture = null; vortexSnapshot = null; runtime = null; objectCommand = null; loaded = false;
    // 독립 Session의 지역없음 번호와 실제 Subscriber64BYTE 이름 주소다.
    noRegion = 127; handle;
    // 현재 자산 처리기/예약 피해와 후속 생성자에서 채울 실제 이동·건설·의식 슬롯이다.
    assetModules = null; developerDamage = null; decorationHit = null; archerCommand = 0x030000b0;
    dudeSpawn = null; dudeTask = null; nuggetPostPop = null; iceTower = null; factoryUpgrade = null; flare = null;
    contained = null; sunAviary = null; rainAviary = null; windAviary = null; fencePostPop = null; altarSacrifice = null; templeRitual = null;
    // 후속 실제 Login의 현재 소유 슬롯이다. 이 단계는 Login을 생성하거나 Session을 공개하지 않는다.
    login = null;
    // 외곽이 구성 완료 뒤 연결하는 현재 Core/세계 간헐천/로컬 의식 callback과 건설 pin 슬롯이다.
    writeCurrentFortCore = null; createWorldGeysers = null; refreshLocalRitual = null; constructionRuntimePin = null; constructionRuntime = null;
    // 생성자1611의 값형 World 훅과 뒤 생성자에서 채우는 실제 의식/내용물/주문 포인터다.
    runtimeModules = null; worldHooks = null; daisState = null; daisRegular = null; ritualExplosion = null;
    contentNotice = null; priestTakenNotice = null; devastationTargets = null; devastationEffect = null;
    // 회수 요청과 서버 수신의 같은 월드 처리기다.
    salvageCommand = null;
    // 같은 World의 운반체 이동 요청과 수신 처리기다.
    carrierMove = null;
    // 같은 AI 본체가 사용하는 이동 처리기와 가상 객체 이름이다.
    aiMovement = null; objectNames = null;
    // AI가 같은 Player·월드에서 간헐천을 선택하는 처리기다.
    aiResourceChoice = null;
    // AI가 같은 자원·경로를 사용하는 운반 행동 처리기다.
    aiCarrierActions = null;
    // AI가 같은 TYPE·spot과 배치 검사를 사용하는 탐색기다.
    aiBuildSearch = null;
    // AI 배치가 사용하는 지상 표적 조회와 제외 TYPE이다.
    aiGroundTarget = null; aiGroundExcludedType = 72;
    // AI 배치가 현재 표적·장애물을 평가하는 처리기다.
    aiDropTargets = null;
    // AI 공급 평가의 현재 탐색 TYPE과 배치 평가기다.
    aiManaProbeType = 71; aiDropEvaluation = null;
    // AI 배치 스캔의 원본 초기 영역과 같은 월드 탐색기다.
    aiDropScanBounds = { left: 10, top: 10, right: 246, bottom: 246 }; aiDropScan = null;
    // AI 배치 후보에서 현재 기술 종류를 고르는 선택기다.
    aiDropSelection = null;
    // 부족 좌표의 실제 기하와 원본 초기 디버그 표식 TYPE이다.
    aiDryGeometry = null; aiDryMarkerType = 104;
    // 현재 AI의 가짜 부족 좌표를 갱신하는 처리기다.
    aiFakeDrySpots = null;
    // 선택한 실제 후보를 생산하는 AI 배치 행동 처리기다.
    aiDropActions = null;
    // 사제 AI의 탐색·이동·생산 행동 처리기다.
    aiPriestActions = null;
    // AI 다리 경로가 읽는 원본 시작·끝·목표 표식 TYPE 초기값이다.
    aiBridgeStartType = 113; aiBridgeEndType = 119; aiBridgeTargetType = 117;
    // AI 다리 계획·전략과 원본 디버그 중단 초기값이다.
    aiBridgePlanning = null; aiBridgeStrategies = null; aiBridgeDisabled = false;
    // 현재 AI 전체 프레임 실행 장치다.
    aiRuntime = null;
    // 원본 실행 시간 측정 전역의 Session 초기값이다.
    aiTiming = 1;
    // 다리 위 이동체의 대피 처리기와 원본 수명 경계다.
    bridgeCarriers = null; bridgeAvoidLife = 7;
    // InstallContents가 같은 World에 등록하는 통지/수명 포인터와 독립 typed 분기의 실제 초기 사건 값이다.
    contentModules = null; gumpForms = null; playerContent = null; contentFormState = null; lifetime = null;
    contentGumpType = 3; contentEvent = 0x02000031;
    // 같은 내용물 수명/운반 부모와 원문 단일 Sound47 예약이다. 후속 생성자가 현재 커서·제단·연구·피해 모듈을 채운다.
    contentForms = null; carrierContentNotice = null; containedUnpop = null; pendingPickupSound = null;
    cursor = null; daisNotice = null; factoryResearch = null; damageablePost = null;
    contentUnpopEvent = 0x02000039; contentBombBlocked = 0;
    // 같은 Player 기술/허용표와 실제 장기 Deck 메모리를 빌리는 생산/동기화 부모다.
    productionModules = null; technology = null; totalMade = null; productionDeck = null; production = null; productionSync = null;
    // 현재 연구/업그레이드/입력과 실제 SharedRegular 예약·뒤 설치할 AI 생산 부모다.
    factoryModules = null; factoryUpgradeCommand = null; factoryInput = null; factoryUpgradePending = null; aiFactoryProduction = null;
    flyerDependType = 1;
    // 같은 실제 플레어와 미부착 Regular 한 번 예약의 소유 구간이다.
    flareModules = null; pendingFlareRegular = null;
    // 같은 건설/간헐천 본체와 뒤 설치될 실제 건물 효과·기준점/Builder·전체 MayPlace 파이프라인이다.
    constructionModules = null; buildingEffects = null; anchor = null; buildingMover = null; gameplayPipeline = null;
    // 원문의100개32BYTE 섬 기록에서 현재 제한/존재만 공급하는 지역 표다.
    gameplayRegions = Array.from({ length: 100 }, () => ({ restricted: 0, exists: 0 }));
    // 같은 실제 Dude 생성/행동·해방/공통 전후처리와 신전/간헐천 삭제를 묶는 생성 구간이다.
    damageableModules = null; dudePostPop = null; dudeBehavior = null; dudeRegular = null; dudeDrop = null;
    damageableRelease = null; damageablePre = null; bombPre = null; flyerPre = null; vortexPre = null; geyserLifetime = null; collapseEffect = null; aiWorld = null;
    // 현재 아이스 타워 몸체와 같은 Host에 부착할 한 번의 실제 Regular 예약이다.
    iceModules = null; pendingIceRegular = null;
    // 크리스탈 크랩의 실제 내용물/정지 부모와 모든 Carrier task가 공유할 미부착 예약이다.
    carrierStealModules = null; carrierSteal = null; carrierTarget = null; pendingCarrierTask = null;
    // 같은 원문60h 헤더와 Session의00545428 정지 DWORD다.
    stealHeader = { message: 0x60, protocol: 1, length: 10 }; stealHaltCounter = 0;
    // 세 주문 접두와 애시드 수신/펜스 몸체 및 서로 다른 미부착 Regular 예약이다.
    spellModules = null; bombPostPop = null; acidZap = null; acidFence = null; pendingBombPopRegular = null; pendingAcidRegular = null;
    // 같은 현재 운반/너겟 주기·미리보기 타이머와 UI 수명 토큰의 약한 참조다.
    carrierRegularModules = null; nuggetRegular = null; carrierFalling = null; carrierTask = null; contentPreviewProcess = null;
    pendingNuggetRegular = null; pendingContentPreview = null; activeSelection = null;
    // 세 실제 Aviary 생성 구간과 독립 typed의 현재 비행 역할·공유 미부착 예약이다.
    aviaryModules = null; sunFlyerType = 73; windFlyerType = 138; rainFlyerType = 144; windAviaryAsset = 139; pendingFlyer = null;
    // 같은 현재 공간·피해·빔을 빌리는 아크 스파이어 몸체와 대여 수명이다.
    arcSpireModules = null; arcSpire = null;
    // 같은 현재 Flyer Process/Frame/Target과 후처리 및 두 별도 미부착 예약이다.
    flyerModules = null; flyerFrame = null; flyerTarget = null; flyerPostPop = null; sunFlyerFrame = null; windFlyer = null;
    pendingSunFlyerSound = null; pendingWindRegular = null; flyerPaused = false; sunBombType = 163; broadcastFlyerCommand = null;
    // 같은 현재 펜스/표식·연결선/재생성·수명과 공통 미부착 Regular 예약이다.
    fenceModules = null; fenceNeighbors = null; fenceLine = null; fenceMarkerCreate = null; fenceShieldCreate = null;
    fenceRegular = null; fenceMarkerClear = null; fenceLifetime = null; fencePendingRegular = null;
    // 독립 typed의 원문 지역 보상 상태/건설 제외 목록이며 실제 Native 장기 대여는 후속이다.
    gemModules = null; gemReward = null; buriedPostPop = null; gemRewardState = new GemRewardState(); constructionExcludedTypes = new SquidPostPopList();
    // 현재 World 훅에 값으로 합친 독립 typed 자산 삭제 분배와 미설치 의식 수명 슬롯이다.
    deletionModules = null; daisLifecycle = null;
    // 같은 현재 전체 Regular/보상 처리기와 뒤 생성자가 채울 사제·마나·주문 슬롯이다.
    regularModules = null; buriedRegular = null; priestRitualFrame = null; priestPrayer = null; manaRegular = null; devastationRegular = null;
    // ConnectMotion 앞단의 도착 장치/미부착 Regular와 같은 장기 카메라 두 DWORD의 const 참조다.
    arrivalModules = null; buildArrival = null; manaArrival = null; localManaArrival = null; localManaRegular = null; manaPendingRegular = null; contentCamera;
    // Load의 같은 기준점/경로 함수 표와 일반 운반체 줍기 소유 구간이다.
    motionModules = null; carrierPickup = null;
    // 같은 장기 Board 실패점과 실제 운반 후보/대상/회피/재탐색의 지역 수명이다.
    carrierRoutingModules = null; boardTarget = null; carrierChoice = null; carrierEscape = null; pathRetarget = null;
    // 같은 월드의 채취/너겟 전달과 실제 이동 본체를 설치하는 지역 수명이다.
    motionRuntimeModules = null; geyserMined = null; carrierMining = null; carrierTransfer = null;
    // 파괴 주문의 현재 TYPE 표와 상태 주문 앞단이다.
    devastationModules = null; devastationRegularTypes = null; devastationEffectTypes = null; bombStatus = null;
    // 소환 주문 두 종류가 빌리는 현재 spawns와 각 주문의 지역 수명이다.
    windSpellCounts = null; windSpellState = { processType: 59, spells: Uint32Array.of(171, 172, 173), counts: null }; windSpell = null; manoSpell = null;
    // 현재 힐 버스트 TYPE와 세 대상 효과의 Session 수명이다.
    healBurstType = 176; bombTreason = null; bombHeal = null; bombHardener = null;
    // 네 직접 피해 주문이 같은 현재 장치와 난수를 빌리는 수명이다.
    bombStrike = null; devastationBlast = null;
    // 주문 활성화와 부착 전 실제 Regular 예약의 Session 수명이다.
    devastationActivation = null; pendingDevastationRegular = null; devastationSearchMemory = null;
    // 효과 대상 배열과 부착 전 상태 타이머의 Session 수명이다.
    devastationEffectMemory = null; pendingBombTimerStorage = null; pendingBombTimer = null; bombTimerType = 57;
    // 파괴 주문 끝에 설치하는 같은 Meltdown 몸체의 Session 수명이다.
    meltdownModules = null; meltdown = null;
    // 공통 객체 명령·투하점과 후속 Cast/Drop 수신의 Session 수명이다.
    carrierDropModules = null; carrierDropPoint = null; carrierDrop = null;
    // 원문 지역 상태의 TYPE 초기값과 같은 현재 동맹 표다.
    bombStatusState = { altar: 164, outpost: 126, priest: 158, fence: 136, flare: 106, alliances: null };
    // 같은 실제 이동 본체를 빌리는 사제 공황 결선의 수명이다.
    panicModules = null;
    // 사제 의식 프레임과 실제 Regular 예약의 지역 수명이다.
    ritualModules = null; ritualActions = null; ritualLookup = null; runePostPop = null; sacrificeRequest = null;
    // 같은 일반/전체 송신 분배·현재 패킷 버퍼와 다리/로딩 깊이 후단을 소유한다.
    transmissionModules = null;
    // 실제 World 게시 뒤 같은 풀의 일반 배치·지형·소유자 후처리 모듈과 선언부의 추가 목록이다.
    terrainModules = null; terrainGlobals; randomPlacement; additional = new SquidPostPopList();
    pipeline = null; normal = null; finishPlacement = null; finish = null;
    // Template 설치/WorldRuntime과 같은 현재 tutorial TYPE 역할의 독립 typed 소유 구간이다.
    installModules = null; tutorialWorkshopType; tutorialArcherType; tutorialTypes;
    // 같은 승패/AI 본체/참여와 현재 typed 전역이다. 연결 함수는 후속 Login이 나중에 채운다.
    aiModules = null; regionOwner = null; outcome = null; outcomeState; aiWorkshops; aiExcludedType; aiMissionFallback;
    participation = null; aiConstructor = null; aiInitial = null; aiCreate = null; aiWallet = null; connectionAssign = null; connectionAlliance = null;
    // 같은 실제 본체/World를 사용하는 통지·준비·전략/경제 구간이다. 행동은 뒤에서 설치한다.
    aiWorldModules = null; aiObjects = null; aiWorld = null; aiPreparation = null; aiStrategySetup = null; aiEconomy = null;
    // 같은 실제 배치/생산·다리 탐색/건설 선택 부모와 미부착 Regular 예약이다.
    aiProductionModules = null; aiDeployment = null; aiCollectors = null; aiBridgeCandidates = null; aiBridgeSearch = null; aiDrySpots = null; aiBuildChoice = null; aiPendingRegular = null;
    // C++ ContainedCreateState.bridgeType의 값형 기본 칸이다. sharedBridgeType를 쓰는 실제 Form 생성의 World links 조회와 구별한다.
    contentBridgeType = 82;
    // 독립 typed 분기만 지역으로 소유하는 현재 값이다. Native 전역을 임의로 빌리지 않는다.
    piece = 107; dudeProviderIndex = 0; windFlyerType = 138; rainFlyerType = 144; sunFlyerType = 73;
    // 기반 자료 반납 및 생성자 내 Player/SP 구간의 한 번 실행 여부다.
    #disposed = false; #playersStarted = false; #playersReady = false;

    // 원문990..1008의 기반 소유 순서다. 예외에는 완성된 World→Player→게임 전역 대여만 역순으로 반납한다.
    constructor(game, files, mission, input = {}) {
        this.gameplayGlobals = gameplayOwner(input); this.gameplayUse = this.gameplayGlobals.acquire();
        try {
            this.coreDataGlobals = input.coreDataGlobals ?? new ClientCoreDataGlobals();
            this.playerRecords = input.playerRecords ?? new ClientPlayerRecords(); this.playerUse = this.playerRecords.acquire();
            // 값형 서비스의 소비 묶음/peer9 표는 복사하고, 함수와 실제 공유 owner의 참조는 유지한다.
            this.services = { ...input, install: { ...input.install }, editorPlayerUi: { ...input.editorPlayerUi }, construction: copyCampaignConstructionEffects(input.construction), damageablePre: { ...input.damageablePre }, damageablePost: { ...input.damageablePost }, assets: copyCampaignAssetEffects(input.assets), deletion: { ...input.deletion }, playerContent: { ...input.playerContent }, contentLifetime: { ...input.contentLifetime }, carrierContent: { ...input.carrierContent }, production: { ...input.production, effects: { ...input.production?.effects } }, sound: { ...input.sound }, motion: copyCampaignMotionEffects(input.motion), priest: { ...input.priest, shield: { ...input.priest?.shield }, regen: { ...input.priest?.regen } }, player: { ...input.player }, ownership: { ...input.ownership }, newTechnology: { ...input.newTechnology }, missionOutcome: { ...input.missionOutcome }, ai: { ...input.ai, participation: { ...input.ai?.participation } }, hit: { ...input.hit, priest: { ...input.hit?.priest }, general: { ...input.hit?.general }, burning: { ...input.hit?.burning }, vortex: { ...input.hit?.vortex }, dudes: { ...input.hit?.dudes } }, archer: copyCampaignArcherEffects(input.archer), battlePeers: Array.isArray(input.battlePeers) ? input.battlePeers.slice() : input.battlePeers ?? new Array(9).fill(null) }; this.missionFortName = String(mission);
            this.worldRedraw = input.kencloudGlobals ? input.kencloudGlobals.worldRedraw() : { value: 0 };
            this.paint = Object.assign(new RegionPaintState(), { sharedWorldRedraw: this.worldRedraw });
            const revision = this.gameplayGlobals.mapRevision(); this.votes = new RegionOwnershipState({ sharedWorldRedraw: this.worldRedraw, sharedMapRevision: revision });
            this.fortTypeConversion = input.fortTypeConversion ?? new FortTypeConversionStorage();
            this.chatGlobals = input.chatGlobals ?? new ClientChatWindowGlobals();
            this.localStateGlobals = input.localStateGlobals ?? new ClientLocalStateGlobals();
            this.handle = input.subscriberGlobals ? input.subscriberGlobals.handle() : new Uint8Array(64);
            this.connectionActivityGlobals = input.connectionActivityGlobals ?? new ClientConnectionActivityGlobals();
            this.templateStorage = input.templateStorage ?? new CampaignTemplateStorage(); this.headers = this.templateStorage.headers();
            this.assets = new CampaignSessionAssets(game, files);
            this.metadata = new PriestPlacementAssets(game.typeTable, game.shapes, this.assets.loadedTypes);
            if (input.surfaceStorage && input.surfaceStorage.source !== game) throw new TypeError('Session의 실제 Client 월드 자산 불일치');
            const aux = this.gameplayGlobals.gameAux(); const inputDepth = this.templateStorage.serializationGlobals;
            this.world = new RawSurfaceWorld(this.assets.types, this.assets.frames, this.metadata.shapes, campaignLinks(), aux.terrainType, {
                storage: input.surfaceStorage ?? null, processKernel: input.processKernel ?? null, sharedSavedGame: this.localStateGlobals.savedGame(),
                authority: this.gameplayGlobals.authority(), gameAux: aux,
                loadingDepth: { get value() { return inputDepth.loadingDepth; }, set value(value) { inputDepth.loadingDepth = value; } },
            });
            this.core = this.world.core();
            this.themes = new Uint32Array(this.core.types.length); this.elements = new Int32Array(this.core.types.length);
            Object.assign(this, createCampaignTypeTables(this.core.types.length));
            Object.assign(this, createCampaignParticleTables());
            Object.assign(this, createCampaignBuildingTables());
            this.chunks = this.world.storage.chunks; this.islandBytes = this.world.storage.islands; this.dudePopulation = this.world.storage.sidLists.dudes; this.stamp = this.gameplayGlobals.terrainStamp();
            this.playerState = this.gameplayGlobals.player; this.globals = this.gameplayGlobals.world;
            this.cursorDrop = this.gameplayGlobals.cursor; this.energyState = this.gameplayGlobals.energy; this.contentFinder = this.gameplayGlobals.finder;
            this.randomGlobals = input.randomGlobals ?? { crt: new CrtRandom(0), game: new GameRandom(0), gameMode: 0 };
            this.random = this.randomGlobals.game; this.particleCrt = this.randomGlobals.crt;
            this.localSp = input.spGlobals ? null : new ScrambledSpStore(this.random, () => this.seed.reseed()); this.sp = input.spGlobals ? input.spGlobals.store() : this.localSp;
            this.rewardState = new SquidRewardState();
            this.hp = new SquidReward(this.core.pool, this.core.types, this.core.bookkeeping, this.rewardState, this.sp, {
                virtualCount: (sid) => { if (typeof this.rewardCount !== 'function') throw new Error('Session의 실제 가상80 분배 미연결'); return this.rewardCount(sid); }, refreshSp: input.refreshSp,
            });
            this.alliances = this.playerRecords.alliances(); this.battleOptions = input.missionOptions ?? new Uint8Array(87);
            if (!(this.battleOptions instanceof Uint8Array) || this.battleOptions.length !== 87) throw new TypeError('Session의 실제 옵션87BYTE 필요');
            this.hiddenPlayers = this.battleOptions.subarray(0x3e, 0x3e + 9); this.participationState = new PlayerParticipationState();
            this.aiMemory = new RawAiBrainMemory(0x20000000, (bytes) => {
                if (typeof this.services.ai?.allocationBytes !== 'function') throw new Error('실제 Session AI 할당 입력 바이트 미연결'); this.services.ai.allocationBytes(bytes);
            });
            this.priestState = new RawPriestWorldState({ priest: aux }); this.finishState = createFortFinishOwnerState();
            this.objectState = createFortObjectLoadStateFromGlobals(this.templateStorage.serializationGlobals);
            this.seed = new GameRandomSeed(this.randomGlobals, input.unixSeconds);
            if (!input.soundNames) throw new TypeError('세션의 실제 SoundList 누락'); this.soundNames = input.soundNames;
            this.restrictions = input.missionRestrictions ?? new Uint32Array(256);
            if (!(this.restrictions instanceof Uint32Array) || this.restrictions.length !== 256) throw new TypeError('Session의 실제 금지256DWORD 필요');
            // 멤버 선언의 참조는 장기 owner의 현재 칸에 직접 읽고 쓴다.
            for (const [name, key] of Object.entries(ROLE_ALIASES)) Object.defineProperty(this, name, { enumerable: true, get: () => aux[key], set: (value) => { aux[key] = value; } });
            this.state = new RawCampaignSessionState(this);
            Object.assign(this, createCampaignTerrainState());
            Object.assign(this, createCampaignTutorialRoles());
            Object.assign(this, createCampaignAiState());
            Object.assign(this, createCampaignCursorTables(this));
            this.tables = new RawCampaignSessionTables(this);
        } catch (error) { this.dispose(); throw error; }
    }

    // 원문1076..1121의 Player/SP와 후속 진단 구간이다. 부모가 한 번 호출하며 실패 접두를 되돌리거나 재실행하지 않는다.
    initializePlayersAndSp() {
        if (this.#disposed || this.#playersStarted) throw new Error('Session Player/SP 초기화의 반납/반복 호출');
        this.#playersStarted = true; const services = this.services;
        if (services.spGlobals && (!services.randomGlobals || services.spGlobals.randomGlobals() !== services.randomGlobals || !this.sp.initialized)) throw new Error('실제 공통 SP 초기화/같은 Client 난수 소유자 계약');
        if (services.coreDataGlobals && this.playerRecords.boundCoreData() !== this.coreDataGlobals) throw new TypeError('현재 Core와 Player base의 실제 소유자 불일치');
        if (services.coreDataGlobals && !services.adoptInitializedPlayers) { this.playerState.playerKnown = 0; this.coreDataGlobals.select(this.playerState, Number(this.core.ownerMode.challenge), 0, 1); }
        if (services.adoptInitializedPlayers) this.playerUse.adoptCurrent();
        else this.playerUse.initialize((owner, mask) => {
            const initial = new RawPlayerParticipation(this.playerRecords.records(), this.alliances, this.hiddenPlayers, this.participationState, {});
            initial.bindCurrentInputs({ localOwner: () => this.playerState.battleOwner, playerKnown: () => this.playerState.playerKnown, battle: () => this.core.ownerMode.battle, metamap: () => this.core.ownerMode.challenge, localTransport: () => false });
            initial.setAlliance(owner, mask);
        });
        this.players = this.playerRecords.records();
        if (!services.coreDataGlobals) this.coreDataGlobals.players = this.players;
        if (!services.spGlobals) { this.sp.bindTrackingOffset(this.priestState.regen.patch.tracking); this.sp.initialize(); }
        this.level = new RawSquidLevel(this.core.pool, this.core.types, makeSquidLevelAssertion((expression) => {
            if (typeof services.ai?.assertion !== 'function') throw new Error('Squid level 실제 진단 장치 미연결'); services.ai.assertion(expression);
        }));
        this.finishState.chaffMode = true; this.objectState.mission = true; this.objectState.chaffMode = true; this.playerState.playerKnown = 1;
        this.state.bindDiagnostics(); this.#playersReady = true;
    }

    // Player/SP와 후속 진단 결선이 정상 복귀했는지다. Session/World Load 완료를 뜻하지 않는다.
    get playersReady() { return this.#playersReady; }

    // Player/SP와 진단 뒤 원문1152..1186을 한 번 연결한다. 이후 Islands/Sound 등 전체 생성자는 부모의 잔여다.
    connectPlayerTablesAndPackets() {
        if (this.#disposed) throw new Error('반납한 Session Player 테이블 결선');
        this.tables.bind();
    }

    // 같은 실제 섬·소리·사제/참여 모듈을 생성한다. 나머지 순환 연결/전체 Session 공개는 후속 구간이다.
    connectIslandsAndPriests() {
        if (this.#disposed) throw new Error('반납한 Session 섬/사제 결선');
        this.priestModules ??= new RawCampaignSessionPriests(this); this.priestModules.bind();
    }

    // 같은 Player/Template/정기와 실제 섬/지역 소유권을 원문 순서로 연결한다. 전체 적재/공개는 후속 구간이다.
    connectPlayersAndOwnership() {
        if (this.#disposed) throw new Error('반납한 Session Player/소유권 결선');
        this.playerModules ??= new RawCampaignSessionPlayers(this); this.playerModules.bind();
    }

    // Carrier/Balloon과 TYPE·소리·SHP의 실제 보충 표를 같은 수명/순서로 연결한다.
    connectBodiesAndTypeTables() {
        if (this.#disposed) throw new Error('반납한 Session 몸체/TYPE 결선');
        this.typeModules ??= new RawCampaignSessionTypes(this); this.typeModules.bind();
    }

    // TYPE 파생 표 뒤 총 검색·에너지·마나/사정거리 표시를 같은 World와 현재 전역에 연결한다.
    connectGunAndCursorViews() {
        if (this.#disposed) throw new Error('반납한 Session 총/커서 결선');
        this.gunViewModules ??= new RawCampaignSessionGunViews(this); this.gunViewModules.bind();
    }

    // 총/커서 뒤 실제 피해 상태·AI 통지·Hit 효과를 같은 HP/Host/현재 장기 목록에 연결한다.
    connectHitBodies() {
        if (this.#disposed) throw new Error('반납한 Session 피격 결선');
        this.hitModules ??= new RawCampaignSessionHits(this); this.hitModules.bind();
    }

    // Hit 뒤 Gun 피해·투사체/배리어 기하·기존 Gun 장애물 통지를 같은 Core에 연결한다.
    connectGunBodies() {
        if (this.#disposed) throw new Error('반납한 Session Gun 몸체 결선');
        this.gunModules ??= new RawCampaignSessionGuns(this); this.gunModules.bind();
    }

    // Gun 뒤 같은 소유자색/보호 SHP 캡처·실제 팔레트·카메라/공유 난수의 입자 몸체를 연결한다.
    connectParticles() {
        if (this.#disposed) throw new Error('반납한 Session 입자 결선');
        this.particleModules ??= new RawCampaignSessionParticles(this); this.particleModules.bind();
    }

    // 입자 뒤 현재 생성자 ABI와 같은 바람/약화 HP/TYPE 전역을 연결한다.
    connectTeleport() {
        if (this.#disposed) throw new Error('반납한 Session Teleport 결선');
        this.teleportModules ??= new RawCampaignSessionTeleport(this); this.teleportModules.bind();
    }

    // Teleport 뒤 Archer/Missile·아이스 반사를 같은 기하/HP/난수/현재 장치에 연결한다.
    connectCombat() {
        if (this.#disposed) throw new Error('반납한 Session Archer 결선');
        this.combatModules ??= new RawCampaignSessionCombat(this); this.combatModules.bind();
    }

    // Archer 뒤 같은 실제 Template/섬/소유권/방송에 독립 typed 신전 상승 부모를 연결한다.
    connectVortex() {
        if (this.#disposed) throw new Error('반납한 Session Vortex 결선');
        this.vortexModules ??= new RawCampaignVortex(this); this.vortexModules.bind();
    }

    // 신전 뒤 실제 자산 효과/피해 분배와 같은 World의 PostPop 등록을 연결한다.
    connectAssets() {
        if (this.#disposed) throw new Error('반납한 Session 자산 결선');
        this.assetModules ??= new RawCampaignSessionAssets(this); this.assetModules.bind();
    }

    // 자산 뒤 현재 World 훅 표만 준비한다. 실제 BindRuntime/Session 공개는 뒤 생성 구간의 몫이다.
    connectWorldRuntimeHooks() {
        if (this.#disposed) throw new Error('반납한 Session Runtime 훅 결선');
        this.runtimeModules ??= new RawCampaignSessionRuntime(this); this.runtimeModules.bind();
    }

    // Runtime 훅 뒤 InstallContents의 실제 통지/공통 삭제 접두를 설치한다. 수명/생산과 World 공개는 뒤 구간이다.
    connectContentNotices() {
        if (this.#disposed) throw new Error('반납한 Session 내용물 통지 결선');
        this.contentModules ??= new RawCampaignSessionContents(this); this.contentModules.bindNotices();
    }

    // 통지 뒤 같은 Form6 수명·운반 통지·일반 Unpop/현재 부착·내용물 생성기를 등록한다.
    connectContentLifetime() {
        if (this.#disposed) throw new Error('반납한 Session 내용물 수명 결선');
        this.contentModules ??= new RawCampaignSessionContents(this); this.contentModules.bindLifetime();
    }

    // 내용물 수명 뒤 같은 Player/집계·금지표/Deck 메모리의 기술·생산·동기화를 구성한다.
    connectProduction() {
        if (this.#disposed) throw new Error('반납한 Session 생산 결선');
        this.productionModules ??= new RawCampaignSessionProduction(this); this.productionModules.bind();
    }

    // 생산 뒤 실제 연구·업그레이드 명령/진행과 입력을 구성하여 InstallContents를 마친다.
    connectFactories() {
        if (this.#disposed) throw new Error('반납한 Session Factory 결선');
        this.factoryModules ??= new RawCampaignSessionFactories(this); this.factoryModules.bind();
    }

    // Factory 뒤 현재 SHP/공간/표시·소리와 실제 Regular 예약에 typed 플레어를 연결한다.
    connectFlare() {
        if (this.#disposed) throw new Error('반납한 Session Flare 결선');
        this.flareModules ??= new RawCampaignSessionFlare(this); this.flareModules.bind();
    }

    // Flare 뒤 같은 World/SP/방송과 장기 최근 목록에 실제 typed 건설 본체를 연결한다.
    connectConstruction() {
        if (this.#disposed) throw new Error('반납한 Session Construction 결선');
        this.constructionModules ??= new RawCampaignSessionConstruction(this); this.constructionModules.bind();
    }

    // 건설 뒤 같은 Pool/Host/HP와 실제 Dude·해방·파생 삭제 부모를 원문 순서로 연결한다.
    connectDamageable() {
        if (this.#disposed) throw new Error('반납한 Session Damageable 결선');
        this.damageableModules ??= new RawCampaignSessionDamageable(this); this.damageableModules.bind();
    }

    // 피해 전후처리 뒤 같은 현재 시계/Host/역할에 성장형·완성형 아이스를 연결한다.
    connectIceTower() {
        if (this.#disposed) throw new Error('반납한 Session IceTower 결선');
        this.iceModules ??= new RawCampaignSessionIce(this); this.iceModules.bind();
    }

    // 아이스 뒤 같은 Form6/방송/현재 이동과 실제 크랩 PostPop을 연결한다.
    connectCarrierSteal() {
        if (this.#disposed) throw new Error('반납한 Session CarrierSteal 결선');
        this.carrierStealModules ??= new RawCampaignSessionCarrierSteal(this); this.carrierStealModules.bind();
    }

    // 크랩 뒤 실제 주문 최초 예약 접두를 같은 Host/게임 난수에 등록한다.
    connectBombPostPop() {
        if (this.#disposed) throw new Error('반납한 Session BombPostPop 결선');
        this.spellModules ??= new RawCampaignSessionSpells(this); this.spellModules.bindBombPostPop();
    }

    // 주문 접두 뒤 같은 현재 헤더/제외 역할과 실제 분해·가상 삭제의61h 본문을 구성한다.
    connectAcidZap() {
        if (this.#disposed) throw new Error('반납한 Session AcidZap 결선');
        this.spellModules ??= new RawCampaignSessionSpells(this); this.spellModules.bindAcidZap();
    }

    //61h 뒤 같은 실제 공간·피해/현재 친군사격과 애시드 전용 주기를 구성한다.
    connectAcidFence() {
        if (this.#disposed) throw new Error('반납한 Session AcidFence 결선');
        this.spellModules ??= new RawCampaignSessionSpells(this); this.spellModules.bindAcidFence();
    }

    // 애시드 뒤 같은 내용물 그림·너겟 예약과 현재 낙하/이동을 연결한다.
    connectCarrierRegular() {
        if (this.#disposed) throw new Error('반납한 Session CarrierRegular 결선');
        this.carrierRegularModules ??= new RawCampaignSessionCarrierRegular(this); this.carrierRegularModules.bind();
    }

    // 운반 주기 뒤 같은 현재 Host/역할과 Sun 생산기 부모를 구성한다.
    connectSunAviary() {
        if (this.#disposed) throw new Error('반납한 Session SunAviary 결선');
        this.aviaryModules ??= new RawCampaignSessionAviaries(this); this.aviaryModules.bindSun();
    }

    // Sun 뒤 같은 현재80BYTE/원주와 Wind 생산기 부모를 구성한다.
    connectWindAviary() {
        if (this.#disposed) throw new Error('반납한 Session WindAviary 결선');
        this.aviaryModules ??= new RawCampaignSessionAviaries(this); this.aviaryModules.bindWind();
    }

    // Wind 뒤 같은 현재48BYTE/Host와 Rain 생산기 부모를 구성한다.
    connectRainAviary() {
        if (this.#disposed) throw new Error('반납한 Session RainAviary 결선');
        this.aviaryModules ??= new RawCampaignSessionAviaries(this); this.aviaryModules.bindRain();
    }

    // 세 Aviary 뒤 같은 현재 TYPE/동맹·검색과 아크 스파이어 효과를 구성한다.
    connectArcSpire() {
        if (this.#disposed) throw new Error('반납한 Session ArcSpire 결선');
        this.arcSpireModules ??= new RawCampaignSessionArcSpire(this); this.arcSpireModules.bind();
    }

    // 아크 스파이어 뒤 실제 Flyer Process/Frame/Target과 공통 후처리를 같은 Host에 구성한다.
    connectFlyer() {
        if (this.#disposed) throw new Error('반납한 Session Flyer 결선');
        this.flyerModules ??= new RawCampaignSessionFlyers(this); this.flyerModules.bind();
    }

    // Flyer 뒤 같은 현재 공간/동맹·표식·연결선과 펜스 후처리/주기/수명을 구성한다.
    connectFence() {
        if (this.#disposed) throw new Error('반납한 Session Fence 결선');
        this.fenceModules ??= new RawCampaignSessionFence(this); this.fenceModules.bind();
    }

    // 펜스 뒤 같은 현재 보상/제외 목록·게임 난수와 Buried 최초 후처리를 구성한다.
    connectGemReward() {
        if (this.#disposed) throw new Error('반납한 Session GemReward 결선');
        this.gemModules ??= new RawCampaignSessionGems(this); this.gemModules.bind();
    }

    // 보상 뒤 실제 피해/내용물/건설·사제 래퍼와 파생 가상 삭제를 같은 World 훅에 합친다.
    connectWorldDeletion() {
        if (this.#disposed) throw new Error('반납한 Session 자산 삭제 결선');
        this.deletionModules ??= new RawCampaignSessionDeletion(this); this.deletionModules.bind();
    }

    // 삭제 분배 뒤 실제 보상/사제·자산·전투·다리와 특수 정기 처리를 같은 훅에 합친다.
    connectWorldRegular() {
        if (this.#disposed) throw new Error('반납한 Session 전체 Regular 결선');
        this.regularModules ??= new RawCampaignSessionRegular(this); this.regularModules.bind();
    }

    // 전체 Regular 뒤 실제 Process 송신 합성과 다리/깊이 후단을 같은 훅에 연결한다. World 게시는 다음 단계다.
    connectWorldTransmission() {
        if (this.#disposed) throw new Error('반납한 Session 송신 결선');
        this.transmissionModules ??= new RawCampaignSessionTransmission(this); this.transmissionModules.bind();
    }

    // 송신 합성 뒤 World 훅을 게시하고 실제 일반 지형/finishOwner를 연결한다. 외곽 Session/Load는 아직 공개하지 않는다.
    connectTerrain() {
        if (this.#disposed) throw new Error('반납한 Session 지형 결선');
        this.terrainModules ??= new RawCampaignSessionTerrain(this); this.terrainModules.bind();
    }

    // 지형 뒤 최초 Template를 선택하고 실제 WorldRuntime/사제 준비 부모를 만든다. Load와 외곽 공개는 다음이다.
    connectWorldInstall(fort) {
        if (this.#disposed) throw new Error('반납한 Session 설치 결선');
        this.installModules ??= new RawCampaignSessionInstall(this); this.installModules.bind(fort);
    }

    // 최초 Template/Runtime 뒤 같은 지역/사제/Player 승패 부모를 만든다. 사건 검사는 실행하지 않는다.
    connectOutcome() {
        if (this.#disposed) throw new Error('반납한 Session 승패 결선');
        this.aiModules ??= new RawCampaignSessionAi(this); this.aiModules.bindOutcome();
    }

    // 승패 부모 뒤 실제 AI 참여/본체/초기 갱신·HP 지갑만 연결한다. 전체 AI·Session은 후속 구간이다.
    connectAiBrains() {
        if (this.#disposed) throw new Error('반납한 Session AI 결선');
        this.aiModules ??= new RawCampaignSessionAi(this); this.aiModules.bindBrains();
    }

    // 실제 AI 본체 뒤 같은 공통 생성/삭제 통지와 World 등록·준비/전략·경제 부모를 만든다.
    connectAiWorld() {
        if (this.#disposed) throw new Error('반납한 Session AI 월드 결선');
        this.aiWorldModules ??= new RawCampaignSessionAiWorld(this); this.aiWorldModules.bind();
    }

    // 실제 World 등록 뒤 같은 배치/콜렉터·시설/다리 탐색과 건설 선택 부모를 만든다.
    connectAiProduction() {
        if (this.#disposed) throw new Error('반납한 Session AI 생산 결선');
        this.aiProductionModules ??= new RawCampaignSessionAiProduction(this); this.aiProductionModules.bind();
    }

    // 생성자 전체가 준비된 뒤 Load가 같은 실제 건설 캡처/프로세스/decoder를 설치한다.
    connectBuilding() {
        if (this.#disposed) throw new Error('반납한 Session 건설 결선');
        this.buildingModules ??= new RawCampaignSessionBuilding(this); this.buildingModules.bind();
    }

    // 원문의 ConnectMotion을 같은 World에서 순서대로 설치하며 실패한 앞부분을 보존한다.
    connectMotion() {
        if (this.#disposed || this.motion) throw new Error('Session 이동 전체 설치 전제/중복/반납 오류');
        this.connectMotionArrivals(); this.connectMotionInputs(); this.connectCarrierRouting(); this.connectMotionRuntime();
        this.connectPriestPanic(); this.connectRitualPriest(); this.connectRitualTemple(); this.connectRitualDais();
        this.connectRitualAltar(); this.connectDaisLifecycle(); this.connectDaisNotice(); this.connectSalvage(); this.connectCarrierMove();
        this.connectAiMovementAll(); this.connectPathTransmission(); this.connectBridgeCarriers();
    }

    // ConnectAiMovement의 현재 이름·행동·Runtime을 원문 순서로 한 번 설치한다.
    connectAiMovementAll() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 전체 이동 설치 전/반납');
        this.connectAiMovement(); this.connectAiResourceChoice(); this.connectAiCarrierActions(); this.connectAiBuildSearch();
        this.connectAiGroundTarget(); this.connectAiDropTargets(); this.connectAiDropEvaluation(); this.connectAiDropScan();
        this.connectAiDropSelection(); this.connectAiDryGeometry(); this.connectAiFakeDrySpots(); this.connectAiDropActions();
        this.connectAiPriestActions(); this.connectAiBridgeStrategies(); this.connectAiRuntime();
    }

    // 같은 건설 설치 뒤 ConnectMotion 앞단의 도착 세 장치만 연결한다. 전체 경로 설치는 후속이다.
    connectMotionArrivals() {
        if (this.#disposed) throw new Error('반납한 Session 도착 결선');
        this.arrivalModules ??= new RawCampaignSessionArrivals(this); this.arrivalModules.bind();
    }

    // 도착 장치 뒤 현재 기준점·경로 효과·줍기 부모를 한 번 설치한다. 후보/이동 본체는 후속이다.
    connectMotionInputs() {
        if (this.#disposed) throw new Error('반납한 Session 이동 입력 결선');
        this.motionModules ??= new RawCampaignSessionMotion(this); this.motionModules.bindInputs();
    }

    // 같은 이동 입력 뒤 실제 Board/후보/대상/회피/재탐색을 한 번 설치한다.
    connectCarrierRouting() {
        if (this.#disposed) throw new Error('반납한 Session 운반 경로 결선');
        this.carrierRoutingModules ??= new RawCampaignSessionCarrierRouting(this); this.carrierRoutingModules.bind();
    }

    // 운반 경로 뒤 채취/전달과 현재 TYPE 효과·실제 MotionBindings를 한 번 설치한다.
    connectMotionRuntime() {
        if (this.#disposed) throw new Error('반납한 Session 이동 본체 결선');
        this.motionRuntimeModules ??= new RawCampaignSessionMotionRuntime(this); this.motionRuntimeModules.bind();
    }

    // 실제 이동 본체 뒤 사제 공황 경로를 한 번 설치한다.
    connectPriestPanic() {
        if (this.#disposed) throw new Error('반납한 Session 사제 panic 결선');
        this.panicModules ??= new RawCampaignSessionPanic(this); this.panicModules.bind();
    }

    // 공황 뒤 사제 의식 프레임과 공유 예약 앞단을 설치한다.
    connectRitualPriest() {
        if (this.#disposed) throw new Error('반납한 Session 의식 결선');
        this.ritualModules ??= new RawCampaignSessionRitual(this); this.ritualModules.bind();
    }

    // 사제 의식 앞단 뒤 신전과 룬 후처리를 설치한다.
    connectRitualTemple() {
        if (this.#disposed || !this.ritualModules) throw new Error('Session 신전 의식 설치 전/반납');
        this.ritualModules.bindTemple();
    }

    // 신전과 룬 뒤 다이스 및 사제 희생 요청을 설치한다.
    connectRitualDais() {
        if (this.#disposed || !this.ritualModules) throw new Error('Session 다이스 의식 설치 전/반납');
        this.ritualModules.bindDais();
    }

    // 다이스 뒤 제단 정기와 해방 처리기를 같은 월드에 설치한다.
    connectRitualAltar() {
        if (this.#disposed || !this.ritualModules) throw new Error('Session 제단 의식 설치 전/반납');
        this.ritualModules.bindAltar();
    }

    // 제단 설치 뒤 다이스 수명 후처리를 같은 월드에 등록한다.
    connectDaisLifecycle() {
        if (this.#disposed || !this.ritualModules) throw new Error('Session 다이스 수명 설치 전/반납');
        this.ritualModules.bindDaisLifecycle();
    }

    // 다이스 수명 뒤 같은 내용물 분배기의 부모 통지를 설치한다.
    connectDaisNotice() {
        if (this.#disposed || !this.ritualModules) throw new Error('Session 다이스 통지 설치 전/반납');
        this.ritualModules.bindDaisNotice();
    }

    // 의식 뒤 현재 회수 명령과 내부 전송을 설치한다.
    connectSalvage() {
        if (this.#disposed || !this.ritualModules) throw new Error('Session 회수 명령 설치 전/반납');
        this.ritualModules.bindSalvage();
    }

    // 회수 명령 뒤 운반체 이동의 현재 선택·경로·전송을 설치한다.
    connectCarrierMove() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session 운반 이동 설치 전/반납');
        this.motionRuntimeModules.bindCarrierMove();
    }

    // 운반 이동 뒤 동일한 명령 헤더를 AI 이동에 연결한다.
    connectAiMovement() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 이동 설치 전/반납');
        this.motionRuntimeModules.bindAiMovement();
    }

    // AI 이동 뒤 현재 간헐천과 신전 자원 선택을 설치한다.
    connectAiResourceChoice() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 자원 선택 설치 전/반납');
        this.motionRuntimeModules.bindAiResourceChoice();
    }

    // AI 자원 선택 뒤 현재 운반 행동을 설치한다.
    connectAiCarrierActions() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 운반 행동 설치 전/반납');
        this.motionRuntimeModules.bindAiCarrierActions();
    }

    // AI 운반 행동 뒤 현재 월드의 배치 탐색을 설치한다.
    connectAiBuildSearch() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 배치 탐색 설치 전/반납');
        this.motionRuntimeModules.bindAiBuildSearch();
    }

    // AI 배치 탐색 뒤 같은 총기 월드의 지상 표적 조회를 설치한다.
    connectAiGroundTarget() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 지상 표적 설치 전/반납');
        this.motionRuntimeModules.bindAiGroundTarget();
    }

    // 지상 표적 조회 뒤 같은 월드의 배치 표적 평가기를 설치한다.
    connectAiDropTargets() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 배치 표적 설치 전/반납');
        this.motionRuntimeModules.bindAiDropTargets();
    }

    // 배치 표적 뒤 현재 공급·부족 좌표 평가기를 설치한다.
    connectAiDropEvaluation() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 배치 평가 설치 전/반납');
        this.motionRuntimeModules.bindAiDropEvaluation();
    }

    // 배치 평가 뒤 현재 지도와 난수를 사용하는 후보 스캔을 설치한다.
    connectAiDropScan() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 배치 스캔 설치 전/반납');
        this.motionRuntimeModules.bindAiDropScan();
    }

    // 스캔 뒤 현재 기술과 후보를 사용하는 배치 선택기를 설치한다.
    connectAiDropSelection() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 배치 선택 설치 전/반납');
        this.motionRuntimeModules.bindAiDropSelection();
    }

    // 배치 선택 뒤 같은 지면과 객체를 사용하는 부족 좌표 기하를 설치한다.
    connectAiDryGeometry() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 부족 좌표 기하 설치 전/반납');
        this.motionRuntimeModules.bindAiDryGeometry();
    }

    // 부족 좌표 기하 뒤 같은 AI 목록을 갱신하는 처리기를 설치한다.
    connectAiFakeDrySpots() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 부족 좌표 갱신 설치 전/반납');
        this.motionRuntimeModules.bindAiFakeDrySpots();
    }

    // 부족 좌표 갱신 뒤 현재 생산·후보를 사용하는 배치 행동을 설치한다.
    connectAiDropActions() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 배치 행동 설치 전/반납');
        this.motionRuntimeModules.bindAiDropActions();
    }

    // 배치 행동 뒤 현재 사제의 행동 처리기를 설치한다.
    connectAiPriestActions() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 사제 행동 설치 전/반납');
        this.motionRuntimeModules.bindAiPriestActions();
    }

    // 같은 세션의 현재 저장소로 AI 다리 경로와 호출자 scratch를 제공한다.
    aiBridgePath(endpointScratch = null) {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 다리 경로 설치 전/반납');
        return this.motionRuntimeModules.aiBridgePath(endpointScratch);
    }

    // 사제 행동 뒤 같은 월드의 AI 다리 계획과 전략을 설치한다.
    connectAiBridgeStrategies() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 다리 전략 설치 전/반납');
        this.motionRuntimeModules.bindAiBridgeStrategies();
    }

    // 다리 전략 뒤 같은 세션의 AI 전체 프레임을 설치한다.
    connectAiRuntime() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session AI 프레임 설치 전/반납');
        this.motionRuntimeModules.bindAiRuntime();
    }

    // AI 행동 설치 뒤 경로 직렬화기를 기존 Process 송신에 합성한다.
    connectPathTransmission() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session 경로 송신 설치 전/반납');
        this.motionRuntimeModules.bindPathTransmission();
    }

    // 경로 송신 뒤 같은 World의 다리 붕괴와 이동체 대피를 연결한다.
    connectBridgeCarriers() {
        if (this.#disposed || !this.motionRuntimeModules) throw new Error('Session 다리 대피 설치 전/반납');
        this.motionRuntimeModules.bindBridgeCarriers();
    }

    // 이동 설치 뒤 파괴 주문의 TYPE 표와 상태 주문 앞단을 설치한다.
    connectDevastationStatus() {
        if (this.#disposed) throw new Error('반납한 Session 파괴 주문');
        this.devastationModules ??= new RawCampaignSessionDevastation(this); this.devastationModules.bindStatus();
    }

    // 상태 주문과 Wind 생산기 뒤 같은 Session의 소환 주문을 연결한다.
    connectDevastationSummons() {
        if (this.#disposed || !this.devastationModules) throw new Error('Session 소환 주문 설치 전/반납');
        this.devastationModules.bindSummons();
    }

    // 소환 주문 뒤 같은 Session의 트리즌·힐·하드너를 설치한다.
    connectDevastationEffects() {
        if (this.#disposed || !this.devastationModules) throw new Error('Session 파괴 효과 설치 전/반납');
        this.devastationModules.bindEffects();
    }

    // 세 대상 효과 뒤 직접 피해 주문의 실제 효과를 설치한다.
    connectDevastationStrike() {
        if (this.#disposed || !this.devastationModules) throw new Error('Session 직접 피해 주문 설치 전/반납');
        this.devastationModules.bindStrike();
    }

    // 직접 피해 주문 뒤 같은 실제 표적 검색과 폭발을 연결한다.
    connectDevastationTargets() {
        if (this.#disposed || !this.devastationModules) throw new Error('Session 주문 표적 설치 전/반납');
        this.devastationModules.bindTargets();
    }

    // 표적 검색 뒤 현재 비용과 Regular 예약을 활성화 수신에 연결한다.
    connectDevastationActivation() {
        if (this.#disposed || !this.devastationModules) throw new Error('Session 주문 활성화 설치 전/반납');
        this.devastationModules.bindActivation();
    }

    // 활성화 뒤 같은 검색·패킷 방송을 사용하는 정기 주문을 설치한다.
    connectDevastationRegular() {
        if (this.#disposed || !this.devastationModules) throw new Error('Session 주문 정기 설치 전/반납');
        this.devastationModules.bindRegular();
    }

    // 정기 주문 뒤 같은 효과 분배와 상태 타이머 수신을 설치한다.
    connectDevastationEffect() {
        if (this.#disposed || !this.devastationModules) throw new Error('Session 주문 효과 설치 전/반납');
        this.devastationModules.bindEffect();
    }

    // 효과 수신 뒤 같은 World의 Meltdown 장치를 설치한다.
    connectMeltdown() {
        if (this.#disposed) throw new Error('반납한 Session Meltdown');
        this.meltdownModules ??= new RawCampaignSessionMeltdown(this); this.meltdownModules.bind();
    }

    // 원문의 파괴 주문 설치 순서를 같은 Session에서 실행하며 부분 실패를 보존한다.
    connectDevastation() {
        if (this.#disposed || this.devastationRegular) throw new Error('Session 파괴 주문 전체 설치 전제/중복/반납 오류');
        this.connectDevastationStatus(); this.connectDevastationSummons(); this.connectDevastationEffects(); this.connectDevastationStrike();
        this.connectDevastationTargets(); this.connectDevastationActivation(); this.connectDevastationRegular(); this.connectDevastationEffect(); this.connectMeltdown();
    }

    // 현재 Session의 실제 7d 상태 타이머 수신을 같은 Host로 전달한다.
    receiveBombTimer(packet) {
        if (this.#disposed || !this.devastationModules) throw new Error('Session 상태 타이머 수신 전/반납');
        this.devastationModules.receiveTimer(packet);
    }

    // 파괴 주문 설치 뒤 운반 투하의 공통 명령과 투하점 앞단을 연결한다.
    connectCarrierDropFront() {
        if (this.#disposed) throw new Error('반납한 Session 운반 투하');
        this.carrierDropModules ??= new RawCampaignSessionCarrierDrop(this); this.carrierDropModules.bindFront();
    }

    // 현재 운반 투하 상태를 요청 또는 수신 직전에 준비한다.
    prepareCarrierDrop() {
        if (this.#disposed || !this.carrierDropModules) throw new Error('Session 운반 투하 준비 전/반납');
        this.carrierDropModules.prepare();
    }

    // 공통 명령과 투하점 뒤 실제 요청 효과를 준비한다.
    connectCarrierDropRequest() {
        if (this.#disposed || !this.carrierDropModules) throw new Error('Session 운반 투하 요청 설치 전/반납');
        this.carrierDropModules.bindRequest();
    }

    // 요청 효과 뒤 서버 수신과 실제 Cast/Drop 본체를 설치한다.
    connectCarrierDropReceive() {
        if (this.#disposed || !this.carrierDropModules) throw new Error('Session 운반 투하 수신 설치 전/반납');
        this.carrierDropModules.bindReceive();
    }

    // 원문의 운반 투하 전체 설치 순서를 같은 Session에서 실행한다.
    connectCarrierDrop() {
        if (this.#disposed || this.carrierDrop) throw new Error('Session 운반 투하 전체 설치 전제/중복/반납 오류');
        this.connectCarrierDropFront(); this.connectCarrierDropRequest(); this.connectCarrierDropReceive();
    }

    // 서버 요청 수신 직전에 현재 권한과 지도 상태를 준비한다.
    receiveCarrierDrop(sender, packet) {
        if (this.#disposed || !this.carrierDrop) throw new Error('Session 운반 투하 수신 전/반납');
        this.prepareCarrierDrop(); this.carrierDrop.receive(sender, packet);
    }

    // 건설과 Renderer가 같은 전용 신전 프레임/음향 본문을 공유한다.
    drawTempleFrames(sid, frame) {
        if (this.#disposed) throw new Error('반납한 Session 신전 그림');
        drawCampaignTempleFrames(this, sid, frame);
    }

    // 수신 위치의 실제 현재 checking 복사와 기존 간헐천 처리기를 연결한다.
    receiveGeyserMined(packet) {
        if (this.#disposed) throw new Error('반납한 Session 간헐천 수신');
        if (!this.buildingModules) throw new Error('Session 건설 수신 결선 전 호출');
        this.buildingModules.receiveGeyserMined(packet);
    }

    // 신전 상승도 공유하는 현재 Template의 Territory120BYTE 쓰기다. 편집기 파일/화면은 만들지 않는다.
    writeTerritory(slot, records) {
        if (this.#disposed) throw new Error('반납한 Session Territory 쓰기');
        writeCampaignTerritory(this.templateStorage, slot, records);
    }

    // 원문 TypeNames 부모의 현재 manager/stream·변환표·깊이를 연결한다. 지역 Template도 같은 변환표 owner를 쓴다.
    prepareFortTypeNames(slot, sourceStorage = null) {
        if (this.#disposed) throw new Error('반납한 Session TypeNames 읽기');
        prepareCampaignFortTypeNames(this, slot, sourceStorage);
    }

    // 실제 로더가 읽으면서 즉시 만드는 원문 Objects/Contents 부모다. 기본20 Terr 뒤 Chaff도 같은 저장소에서 읽는다.
    readFortObjectStreams(objects, slot, start = 0, end = 20, sourceStorage = null) {
        if (this.#disposed) throw new Error('반납한 Session Objects 읽기');
        readCampaignFortObjectStreams(this, objects, slot, start, end, sourceStorage);
    }

    // 외곽 snapshot 부모가 같은 실제 Template/덱에 TypeNames 또는 Deck을 쓴다. 공개 loaded 관문은 외곽이 맡는다.
    snapshotFortBasicSection(slot, names) {
        if (this.#disposed) throw new Error('반납한 Session 기본 snapshot');
        snapshotCampaignFortBasicSection(this, slot, names);
    }

    // 전체 snapshot 부모가 이미 만든 같은 stream에 실제 기술 Form만 쓴다. 별도 Technology 노드를 만들지 않는다.
    writeFortTechnology(parent, stream) {
        if (this.#disposed) throw new Error('반납한 Session 기술 쓰기');
        writeCampaignFortTechnology(this, parent, stream);
    }

    // 공개 설치 관문 뒤 현재 기술 부모를 먼저 보관하고 실제 새 Technology 섹션을 한 번 만든다.
    snapshotFortTechnology(slot) {
        if (this.#disposed) throw new Error('반납한 Session 기술 snapshot');
        if (!this.loaded || !Number.isInteger(slot) || slot < 0 || slot >= 9) throw new Error('실제 Technology snapshot의 설치/슬롯 계약 위반');
        this.templateStorage.template(slot); const parent = this.player.currentTechnology(); const stream = this.templateStorage.createSection(slot, 'Technology'); this.writeFortTechnology(parent, stream);
    }

    // 실제 영역 부모가 이미 만든 같은 stream에 청크 검색/객체 저장 본문만 실행한다.
    writeFortObjectChunkBody(stream, chunkX, chunkY) {
        if (this.#disposed) throw new Error('반납한 Session 객체 청크 쓰기');
        writeCampaignFortObjectChunk(this, stream, chunkX, chunkY);
    }

    // 공개 설치 관문을 유지하며 내부 영역 writer의 설치 중 접근과 구별한다.
    writeFortObjectChunk(stream, chunkX, chunkY) {
        if (!this.loaded) throw new Error('실제 Session 설치 전 객체 청크 저장');
        this.writeFortObjectChunkBody(stream, chunkX, chunkY);
    }

    // 영역/Temp/Chaff가 같은 Template와 공간을 사용하는 전체 내부 Objects 부모다.
    writeFortObjects(slot) {
        if (this.#disposed) throw new Error('반납한 Session 전체 Objects 쓰기');
        writeCampaignFortObjects(this, slot);
    }

    // 공개 설치 계약을 유지하고 실제 전체 부모를 중복 생성하지 않는다.
    snapshotFortObjects(slot) {
        if (!this.loaded) throw new Error('실제 Session 설치 전 Objects 저장');
        this.writeFortObjects(slot);
    }

    // 전체 snapshot의 현재 Runtime 읽기·섬/청크·공유 난수와 새 Territory 노드를 연결한다.
    writeFortTerritory(slot) {
        if (this.#disposed) throw new Error('반납한 Session Territory snapshot 쓰기');
        writeCampaignFortTerritory(this, slot);
    }

    // 공개 설치 관문을 유지하며 신전 상승용120BYTE 일반 writer와 구별한다.
    snapshotFortTerritory(slot) {
        if (!this.loaded) throw new Error('실제 Session 설치 전 Territory 저장');
        this.writeFortTerritory(slot);
    }

    // 같은 현재 Core 포인터·기술 finder·실제 Pool의 다이스 가상80으로 Core를 갱신한다.
    refreshCurrentPlayerCore() {
        if (this.#disposed) throw new Error('반납한 Session 현재 Core 갱신');
        refreshCampaignPlayerCore(this);
    }

    // 원문 manager/모드 관문 뒤 실제 Player/Client seed·공유 난수를 같은 CoreData stream에 연결한다.
    writeFortCoreDataBody(slot) {
        if (this.#disposed) throw new Error('반납한 Session CoreData 쓰기');
        writeCampaignFortCoreData(this, slot);
    }

    // 전체 Fort 부모는 설치 관문 뒤 같은 Template/현재 child만 사용한다. Editor 우회는 제공하지 않는다.
    snapshotFort(slot) {
        if (this.#disposed) throw new Error('반납한 Session 전체 Fort snapshot');
        if (!this.loaded) throw new Error('실제 Session 설치 전 전체 Fort 저장');
        this.snapshotFortBody(slot);
    }

    // Battle은 자체 부모 순서를 사용하며 Fort 전용 TypeNames/Badges를 추가하지 않는다.
    snapshotBattle(slot) {
        if (this.#disposed) throw new Error('반납한 Session 전체 Battle snapshot');
        if (!this.loaded) throw new Error('실제 Session 설치 전 전체 Battle 저장');
        this.snapshotTemplateBody(slot, true);
    }

    // 외곽 callback에서 호출하는 같은 Fort 본문이다. 설치 중 Editor 예외는 웹 범위 밖이다.
    snapshotFortBody(slot) {
        if (this.#disposed) throw new Error('반납한 Session 전체 Fort snapshot 본문');
        this.snapshotTemplateBody(slot, false);
    }

    // 같은 실제 저장소의 전체 부모 결선이며 loaded를 임의로 변경하지 않는다.
    snapshotTemplateBody(slot, battle) {
        if (this.#disposed) throw new Error('반납한 Session 전체 snapshot 본문');
        snapshotCampaignTemplate(this, slot, battle);
    }

    // 실제 설치 뒤 로컬 간헐천의 같은 Player/상태/공간을 준비한다.
    generateLocalGeysers() {
        if (this.#disposed) throw new Error('반납한 Session 로컬 간헐천 생성');
        generateCampaignLocalGeysers(this);
    }

    // 전체 세계 간헐천은 같은 장기 옵션과 공간/난수를 사용하는 실제 부모에 위임한다.
    generateWorldGeysers() {
        if (this.#disposed) throw new Error('반납한 Session 세계 간헐천 생성');
        generateCampaignWorldGeysers(this);
    }

    // Impl의 placeBattleNuggets callback 슬롯과 구별하는 실제 배치 본문이다. 외곽이 이 메서드를 연결한다.
    placeBattleNuggetsBody(rectangle, quantity) {
        if (this.#disposed) throw new Error('반납한 Session 너겟 보충 배치');
        placeCampaignBattleNuggets(this, rectangle, quantity);
    }

    // 현재 기술 부모/실제 내용물·두 snapshot·안내의 원문 순서를 사용한다.
    learnNewTechnology(type) {
        if (this.#disposed) throw new Error('반납한 Session 기술 획득');
        learnCampaignNewTechnology(this, type);
    }

    // 생산/객체/기준점이 공유하는 현재 FullDeck DWORD다.
    get fullDeck() { return this.gameplayGlobals.fullDeck().value; }
    // 값 쓰기도 동일 장기 DWORD를 사용한다.
    set fullDeck(value) { this.gameplayGlobals.fullDeck().value = value; }
    // 소유권/카메라/렌더러의 현재 지도 개정 DWORD다.
    get revision() { return this.gameplayGlobals.mapRevision().value; }
    // 별도 Session 지역 개정을 만들지 않는다.
    set revision(value) { this.gameplayGlobals.mapRevision().value = value; }
    // 로그인/충돌/의식이 공유하는 같은 double 기한이다.
    get loginEnd() { return this.localStateGlobals.loginEnd().value; }
    // 같은 State owner에 쓰며 World 교체로 재설정하지 않는다.
    set loginEnd(value) { this.localStateGlobals.loginEnd().value = value; }
    // 내용물/월드 로더의 현재 저장 게임 관문이다.
    get contentSavedGame() { return this.localStateGlobals.savedGame().value; }
    // 같은 boolean 상자에 쓰고 복사본을 동기화하지 않는다.
    set contentSavedGame(value) { this.localStateGlobals.savedGame().value = value; }

    // C++ 기반 멤버의 역순 반납이다. 전체 Session 부모는 먼저 자기 모듈/현재 Runtime 공개를 닫아야 한다.
    dispose() {
        if (this.#disposed) return; this.#disposed = true;
        this.aiModules?.releaseBrains();
        try {
            if (this.priestModules || this.playerModules || this.typeModules || this.gunViewModules || this.hitModules || this.gunModules || this.particleModules) { this.world?.drainOwnedProcessesForShutdown(); this.aiProductionModules?.dispose(); this.aiWorldModules?.dispose(); this.aiModules?.dispose(); this.ritualModules?.dispose(); this.panicModules?.dispose(); this.motionRuntimeModules?.dispose(); this.carrierRoutingModules?.dispose(); this.motionModules?.dispose(); this.arrivalModules?.dispose(); this.buildingModules?.dispose(); this.installModules?.dispose(); this.terrainModules?.dispose(); this.transmissionModules?.dispose(); this.regularModules?.dispose(); this.deletionModules?.dispose(); this.gemModules?.dispose(); this.fenceModules?.dispose(); this.flyerModules?.dispose(); this.arcSpireModules?.dispose(); this.aviaryModules?.dispose(); this.carrierRegularModules?.dispose(); this.spellModules?.dispose(); this.carrierStealModules?.dispose(); this.iceModules?.dispose(); this.damageableModules?.dispose(); this.constructionModules?.dispose(); this.flareModules?.dispose(); this.factoryModules?.dispose(); this.productionModules?.dispose(); this.contentModules?.dispose(); this.runtimeModules?.dispose(); this.assetModules?.dispose(); this.vortexModules?.dispose(); this.teleportModules?.dispose(); this.combatModules?.dispose(); this.particleModules?.dispose(); this.gunModules?.dispose(); this.hitModules?.dispose(); this.gunViewModules?.dispose(); this.typeModules?.dispose(); this.playerModules?.dispose(); this.priestModules?.dispose(); }
        } finally {
            try { try { this.aiProductionModules?.dispose(); this.aiWorldModules?.dispose(); this.aiModules?.dispose(); this.ritualModules?.dispose(); this.panicModules?.dispose(); this.motionRuntimeModules?.dispose(); this.carrierRoutingModules?.dispose(); this.motionModules?.dispose(); this.arrivalModules?.dispose(); this.buildingModules?.dispose(); this.installModules?.dispose(); this.terrainModules?.dispose(); this.transmissionModules?.dispose(); this.regularModules?.dispose(); this.deletionModules?.dispose(); this.gemModules?.dispose(); this.fenceModules?.dispose(); this.flyerModules?.dispose(); this.arcSpireModules?.dispose(); this.aviaryModules?.dispose(); this.carrierRegularModules?.dispose(); this.spellModules?.dispose(); this.carrierStealModules?.dispose(); this.iceModules?.dispose(); this.damageableModules?.dispose(); this.constructionModules?.dispose(); this.flareModules?.dispose(); this.factoryModules?.dispose(); this.productionModules?.dispose(); this.contentModules?.dispose(); this.runtimeModules?.dispose(); this.assetModules?.dispose(); } finally { try { this.carrierDropModules?.dispose(); } finally { try { this.meltdownModules?.dispose(); } finally { this.devastationModules?.dispose(); this.world?.dispose(); } } } } finally {
                try { this.vortexModules?.dispose(); } finally { try { this.teleportModules?.dispose(); } finally { try { this.combatModules?.dispose(); } finally { try { this.particleModules?.dispose(); } finally { try { this.gunModules?.dispose(); } finally { try { this.hitModules?.dispose(); } finally { try { this.gunViewModules?.dispose(); } finally { try { this.typeModules?.dispose(); } finally { try { this.playerModules?.dispose(); } finally { try { this.priestModules?.dispose(); } finally { try { this.playerUse?.release(); } finally { this.gameplayUse?.release(); } } } } } } } } } } }
            }
        }
    }
}
