// RawCampaignSession::Impl의 캠페인 소유 자료를 실제 하위 생성자로 구성한다(225553df, WB04-E574).
// 사용: owner 생성 → 나머지 공유 상태 결선 → initializePlayersAndSp → 나머지 모듈/Load 연결.
// 이 단계는 Session이 아니며 현재 Runtime에 공개하지 않는다. 외곽 Session과 Load는 아직 미구현이다.
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

// 실제 Impl의 기반 owner와 고정뷰 대여다. 부모의 결선 전까지 게임 생성·Fort 설치·Runtime 공개를 하지 않는다.
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
            this.services = { ...input }; this.missionFortName = String(mission);
            this.fortTypeConversion = input.fortTypeConversion ?? new FortTypeConversionStorage();
            this.chatGlobals = input.chatGlobals ?? new ClientChatWindowGlobals();
            this.localStateGlobals = input.localStateGlobals ?? new ClientLocalStateGlobals();
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
        } catch (error) { this.dispose(); throw error; }
    }

    // 원문1076..1095 구간이다. 앞선 공유 상태 결선 뒤 부모가 한 번 호출하며 실패 접두를 되돌리거나 재실행하지 않는다.
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
        this.finishState.chaffMode = true; this.objectState.mission = true; this.objectState.chaffMode = true; this.playerState.playerKnown = 1; this.#playersReady = true;
    }

    // Player/SP 구간만 정상 복귀했는지다. Session/World Load 완료를 뜻하지 않는다.
    get playersReady() { return this.#playersReady; }

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
        try { this.world?.dispose(); } finally { try { this.playerUse?.release(); } finally { this.gameplayUse?.release(); } }
    }
}
