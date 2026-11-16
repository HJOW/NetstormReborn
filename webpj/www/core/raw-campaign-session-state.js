// RawCampaignSession::Impl의 공유 상태와 생성자1010..1121 결선이다(225553df, WB04-E575).
// 기반 owner가 만든 같은 World/Template/Player/SP를 사용한다. 전체 Session 공개/Load와 Native 분기는 후속이다.
import { ContentLifetimeState } from './content-form-lifetime.js';
import { AssetPostPopState } from './raw-asset-post-pop-state.js';
import { AssetRegularState } from './raw-asset-regular.js';
import { ArcherAnimationState } from './raw-archer-animation.js';
import { RawCampaignConstructionState } from './raw-campaign-construction-bindings.js';
import { RawCampaignMotionState } from './raw-campaign-motion-bindings.js';
import { createCanonPlacementPipelineState } from './raw-canon-placement-pipeline.js';
import { CarrierPostPopState } from './raw-carrier-post-pop.js';
import { CarrierTransferState } from './raw-carrier-transfer.js';
import { RawConnectionActivity } from './raw-connection-activity.js';
import { ConstructionArrivalState } from './raw-construction-arrival.js';
import { ConstructionEffectsState } from './raw-construction-effects.js';
import { ConstructionProcessState } from './raw-construction-process.js';
import { DamageableBurningState } from './raw-damageable-burning.js';
import { DamageableHitState } from './raw-damageable-hit.js';
import { DamageablePostDestroyState } from './raw-damageable-post-destroy.js';
import { DamageableReleaseState } from './raw-damageable-release.js';
import { FarmClusterState } from './raw-farm-cluster.js';
import { GunProcessState } from './raw-gun-process.js';
import { createMissionStartGlobals } from './raw-mission-start.js';
import { MissileState } from './raw-missile.js';
import { RawPathProcessState } from './raw-path-process.js';
import { PlayerPlacementAnchorState } from './raw-player-placement-anchor.js';
import { PriestDamageState } from './raw-priest-damage.js';
import { GunImpactState } from './raw-gun-impact.js';
import { RunePostPopState } from './raw-rune-post-pop.js';
import { SoundProcessState } from './sound-process.js';
import { TempleRitualState } from './raw-temple-ritual.js';

// RawCheckingFlag 복사 때 같은 현재 포인터 또는 지역 DWORD를 고르는 비공개 복사 장치다.
const FLAG_COPIERS = new WeakMap();
// 값형 struct에 플래그를 복사한다. 이후 원래 struct의 값 대입은 복사본의 포인터를 바꾸지 않는다.
export function captureCampaignFlag(target, key) {
    const copy = FLAG_COPIERS.get(target)?.get(key);
    if (copy) return copy();
    const value = target[key]; return () => Number(typeof value === 'function' ? value() : value) >>> 0;
}

// 실제 owner의 칸을 {value} 참조로 빌린다. 스칼라 스냅샷을 만들어 동기화하지 않는다.
function reference(owner, key) { return { get value() { return owner[key]; }, set value(value) { owner[key] = value; } }; }
// RawCheckingFlag의 명시 Bind/값 대입 의미다. 값 대입은 지역 입력으로 돌아가며 외부 DWORD를 쓰지 않는다.
function bindFlag(target, key, read) {
    let source = read; let local = target[key];
    Object.defineProperty(target, key, { enumerable: true, configurable: true,
        get() { return Number(source ? source() : typeof local === 'function' ? local() : local) >>> 0; },
        set(value) { source = null; local = value; },
    });
    // 복사 순간의 포인터/지역 입력을 캡처하며 전역 DWORD 자체는 호출 때 현재 값을 읽는다.
    const copies = FLAG_COPIERS.get(target) ?? new Map(); FLAG_COPIERS.set(target, copies);
    copies.set(key, () => { const selected = source; const value = local; return () => Number(selected ? selected() : typeof value === 'function' ? value() : value) >>> 0; });
}
// shared* 포인터를 지원하지 않는 기존 값형 JS 소비자에 같은 현재 읽기를 제공한다.
// 지역 필드 대입은 C++ shared 포인터 뒤에 가려진 fallback만 바꾸며 owner를 덮어쓰지 않는다.
function borrowRead(target, key, read) {
    let local = target[key];
    Object.defineProperty(target, key, { enumerable: true, configurable: true, get() { return read(); }, set(value) { local = value; } });
    return () => local;
}
// 복구 가능한 원문 진단은 도달한 위치에서만 실제 장치를 요구한다.
function assertion(owner, label, capture = false) {
    const initial = owner.services.ai?.assertion;
    return (expression, file, line) => {
        const report = capture ? initial : owner.services.ai?.assertion;
        if (typeof report !== 'function') throw new Error(`${label} 실제 진단 장치 미연결${line === undefined ? '' : ` ${line}`}`);
        report(expression);
    };
}
// 웹 RawGunTarget가 받는 값형 상태이며 C++ GunTargetState의 원문 기본값이다.
function gunTargetState() { return { friendlyFire: 0, checking: 0, showRanges: false, sunCannonType: 128, sunFenceType: 136, windArcherType: 140, thunderArcherType: 148, windHalfAngle: 30, scaleX: 16, scaleY: 11, allyMasks: new Uint32Array(9), playerFlags: new Uint8Array(9), allyMaskRead: null, playerFlagsRead: null, angles: {} }; }
// 웹 RawCarrierDrop의 현재 헤더/명령 입력이며 C++ CarrierDropState 기본값이다.
function carrierDropState() { return { request: { message: 0x50, protocol: 1, length: 9 }, activation: { message: 0x5c, protocol: 1, length: 6 }, localOwner: 1, server: 1, checking: 0, numTypes: 188, contentType: 6, notifyProcessType: 51, queueCommand: 0x030000b2, selectCommand: 0x03000073, castCommand: 0x0300011a, dropCommand: 0x03000076, pickupCommand: 0x03000075, mapLimit: 256 }; }

// 같은 Impl 안의 실제 상태를 한 수명으로 보관한다. 각 상태는 기존 raw 모듈에 그대로 전달한다.
export class RawCampaignSessionState {
    // 기반 owner와 Player/SP 뒤 진단 결선의 한 번 실행 여부다.
    #owner; #diagnosticsBound = false;
    // 현재 보상/의식·운반·피해·생성 입력이다.
    runeState = new RunePostPopState(); ritualState = new TempleRitualState();
    damageablePostState = new DamageablePostDestroyState(); damageableReleaseState = new DamageableReleaseState();
    carrierTransferState = new CarrierTransferState(); contentLifetimeState = new ContentLifetimeState(); carrierState = new CarrierPostPopState();
    // RawGeyserMined는 값형 상태를 받는다. TYPE/권한의 현재 읽기는 아래에서 같은 owner로 연결한다.
    geyserMinedState = { boss: true, checking: 0, nuggetType: 154, emptyGeyserType: 84, isleType: 97, bridgeType: 82 };
    // 실제 경로/건설·배치·시작/AI의 상태다.
    motionState = new RawCampaignMotionState(); constructionState = new RawCampaignConstructionState();
    missionStart = createMissionStartGlobals(); priestDamageState = new PriestDamageState();
    gunTargetState = gunTargetState(); gunState = new GunProcessState();
    placement = createCanonPlacementPipelineState(); gameplayPlacement = createCanonPlacementPipelineState();
    buildingState = new ConstructionProcessState(); buildingEffectsState = new ConstructionEffectsState();
    farm = new FarmClusterState(); anchorState = new PlayerPlacementAnchorState();
    // 실제 주문 검색/폭발/정기/수신의 값형 상태다. Native 장기 대여는 별도이며 준비 함수가 현재 권한/동맹을 공급한다.
    devastationTargetState = { boss: 0 };
    devastationBlastState = { boss: 0, growingRainBlockerType: 150, priestType: 158, alliances: new Uint32Array(81) };
    devastationRegularState = { boss: 0, localOwner: 1, effect: { message: 0x5f, protocol: 1, length: 10 } };
    devastationEffectState = { boss: 0, firstType: 117, secondType: 118, processType: 57, spentCounter: 0, sharedSpentCounter: null };
    // RawMissionAiSettings의 실제 값형 상태다(CD 전용 시작 SP는 웹 범위 밖).
    missionAiSettings = { stuffRefreshRate: 3, startingSp: 2500, checkingAsserts: 0 };
    // 같은 실제 시계의 경로/애니메이션·음향·피해/정기·반환 상태다.
    process; start; snap = true; retrying = false;
    archerState = new ArcherAnimationState(); missileState = new MissileState(); soundState = new SoundProcessState();
    buildArrivalState = new ConstructionArrivalState(); assetState = new AssetPostPopState(); regularState = new AssetRegularState();
    generalDamageState; burningState = new DamageableBurningState(); carrierDropState = carrierDropState();
    // 원문666의 충돌/Flare 억제이며 같은 로그인 기한 double을 빌린다.
    impactState;
    // 원문660의 TYPE 전체 최대 검색 반경과 스톤 타워 번호다. 실제 채움은 Gun 몸체 생성 구간이다.
    gunBlockerState = { cannonRange: 0, archerRange: 0, stoneTowerType: 78 };
    // 수신 등록을 하지 않는 실제 연결 활동 몸체다. 등록과 전송은 전체 Session 부모의 몫이다.
    connectionActivity;

    // 기반 owner의 동일 참조를 사용하며 Player/시계/SP를 초기화하지 않는다.
    constructor(owner) {
        this.#owner = owner; this.process = new RawPathProcessState(owner.core.processState); this.generalDamageState = new DamageableHitState(this.priestDamageState);
        this.impactState = new GunImpactState({ sharedSuppressedTime: owner.localStateGlobals.loginEnd() });
        const movement = owner.priestState.movement; const state = this;
        this.start = { clock: owner.core.processState,
            get dudeType() { return movement.dude; }, get teleportType() { return movement.teleport; }, get manaBoltType() { return movement.manaBolt; }, get priestType() { return movement.priest; }, get geyserType() { return owner.geyserType; },
            get snapPriestDestination() { return state.snap; }, set snapPriestDestination(value) { state.snap = value; }, get retrying() { return state.retrying; }, set retrying(value) { state.retrying = value; },
        };
        this.#bindShared();
    }

    // 원문1010..1075의 캠페인/독립 typed 공유 입력 결선이다. Native 전용1051..1062는 적용하지 않는다.
    #bindShared() {
        const o = this.#owner; const g = o.gameplayGlobals; const aux = g.gameAux(); const authority = g.authority(); const core = o.core;
        const boss = () => authority.boss; const server = () => authority.server; const nugget = () => aux.nuggetType; const geyser = () => o.geyserType;
        o.rewardState.bindGameAux(aux); o.rewardState.share('halfHealthMode', core.ownerMode, 'battle'); o.rewardState.share('priestType', o.priestState.movement, 'priest');
        this.runeState.sharedRuneType = () => aux.runeType; this.ritualState.sharedRuneType = reference(aux, 'runeType'); this.runeState.sharedSuppressedDuration = () => o.globals.missionPost;
        this.runeState.sharedAltarType = () => aux.altarType; this.runeState.sharedDaisType = () => aux.daisType;
        this.damageablePostState.bindSilentSurfaceType(reference(aux, 'rainBlockerType'));
        this.contentLifetimeState.currentNuggetType = nugget; this.carrierTransferState.sharedNuggetType = nugget; borrowRead(this.geyserMinedState, 'nuggetType', nugget);
        this.motionState.carrier.sharedNuggetType = nugget; this.motionState.cleanup.sharedNuggetType = nugget; this.constructionState.place.sharedNuggetType = nugget;
        o.hp.bindCurrentTypes(core.types); o.hp.bindLoadedCosts(o.assets.loadedTypes);
        o.priestState.regen.patch.sharedChangedSample = g.patchDiagnostic(); o.priestState.regen.patch.sharedSample = g.patchSample(); this.motionState.search.sharedPolicySelector = g.patchDiagnostic(); o.priestState.regen.sharedPriestType = () => aux.priestType;
        this.missionStart.shared = { denySalvage: g.denySalvage(), denyAscend: g.denyAscend(), allowAnyCapture: reference(aux, 'allowAnyCapture'), ordinaryGeysers: reference(aux, 'ordinaryGeysers') };
        this.motionState.carrier.sharedAllowAlliedPickup = () => aux.allowAnyCapture; this.motionState.carrier.sharedGeyserType = geyser; this.carrierState.sharedGeyserType = geyser; this.constructionState.geyser.sharedGeyserType = geyser;
        this.carrierState.sharedBoss = boss; o.contentFinder.bindBoss(g.boss()); this.carrierState.sharedLoadingDepth = () => o.objectState.loadingDepth;
        o.priestState.carrier.sharedLoadingDepth = () => o.objectState.loadingDepth; o.priestState.carrier.sharedGeyserType = geyser;
        this.priestDamageState.sharedBoss = boss; this.damageablePostState.bindBoss(g.boss()); this.contentLifetimeState.currentBoss = boss;
        this.priestDamageState.sharedInvulnerable = () => aux.missionSettings()[2];
        bindFlag(this.gunTargetState, 'friendlyFire', () => aux.missionSettings()[3]); bindFlag(this.gunState, 'friendlyFire', () => aux.missionSettings()[3]);
        this.placement.placement.sharedForcePlacement = this.gameplayPlacement.placement.sharedForcePlacement = () => aux.missionSettings()[1];
        this.constructionState.place.sharedAuthority = this.constructionState.clear.sharedAuthority = this.constructionState.notice.sharedAuthority = this.constructionState.geyser.sharedBoss = boss;
        this.buildingState.sharedBoss = boss; borrowRead(this.geyserMinedState, 'boss', boss); this.buildingState.currentProcessType = () => aux.constructionProcessType;
        bindFlag(this.buildingState, 'quickBuild', () => aux.quickBuild); bindFlag(this.buildingState, 'quickVortex', () => aux.quickVortex);
        this.constructionState.place.sharedServer = this.buildingEffectsState.sharedServer = server;
        this.buildingEffectsState.sharedBlocked = () => aux.islandDropBlocked; this.buildingEffectsState.sharedStumpType = () => aux.islandStumpType;
        this.buildingEffectsState.sharedIslandType = () => core.bridgeState.islandType; this.buildingEffectsState.sharedNoIslandType = () => core.bridgeState.noIslandType; this.buildingEffectsState.sharedStalagType = () => core.bridgeState.stalagType;
        this.constructionState.notice.sharedHistory = g.camera().constructionHistory();
        this.constructionState.place.sharedLocalPlayer = this.constructionState.notice.sharedLocalPlayer = () => o.playerState.battleOwner;
        this.constructionState.place.sharedPlayerKnown = () => o.playerState.playerKnown; this.constructionState.place.sharedBridgeType = () => core.bridgeState.bridgeType; this.constructionState.place.sharedNoIslandType = () => core.bridgeState.noIslandType;
        this.constructionState.place.sharedBridgeLife = () => 5;
        this.constructionState.notice.sharedDaisType = () => aux.daisType; this.constructionState.notice.sharedPriestType = () => aux.priestType;
        this.constructionState.place.patternTypesRead = this.constructionState.confirm.patternTypesRead = () => [o.piece, core.bridgeState.bridgeType, core.bridgeState.islandType, core.bridgeState.noIslandType, this.gunState.readThunderCannonType(), this.gunState.readRainCannonType(), aux.windArcherType, this.constructionState.confirm.patternTypes[7]];
        this.farm.sharedArchipelago = () => aux.archipelago; this.farm.sharedArchipelagoSpan = () => aux.worldGeyserMargin; this.devastationEffectState.sharedSpentCounter = aux;
        o.objectState.readFullDeck = this.constructionState.confirm.sharedFullDeck = () => o.fullDeck; this.anchorState.bindFullDeck(() => o.fullDeck);
        o.objectState.readFortKeepGenus = () => g.fortKeepGenus().value; this.damageableReleaseState.bindAllowedGenus(g.fortKeepGenus());
        this.connectionActivity = new RawConnectionActivity(o.connectionActivityGlobals.tables(), {
            wallSeconds: () => { if (typeof o.services.sound?.wallSeconds !== 'function') throw new Error('실제 연결 활동 벽시각 미연결'); return o.services.sound.wallSeconds(); },
            frameNumber: () => { if (typeof o.services.connectionFrameNumber !== 'function') throw new Error('실제 연결 활동 프레임 조회 미연결'); return o.services.connectionFrameNumber(); },
            server: () => core.processState.server,
            eraseStatus: (kind, player) => { if (o.globals.missionPost && kind !== 11 && kind !== 16) return; if (typeof o.services.removeConnectionStatus !== 'function') throw new Error('실제 연결 상태 WarningMessages 삭제 미연결'); o.services.removeConnectionStatus(kind, player); },
        });
    }

    // 원문1098..1121을 Player/SP 정상 복귀 뒤 한 번 결선한다. 진단 자체는 선택된 소비 지점에서만 실행한다.
    bindDiagnostics() {
        if (this.#diagnosticsBound) throw new Error('Session 진단 결선 반복');
        const o = this.#owner; const g = o.gameplayGlobals; const core = o.core; const storage = o.world.storage; const checking = () => g.aiDebug().checking().value;
        this.archerState.angles = storage.angleDiagnostics(); this.gunTargetState.angles = this.archerState.angles; this.process.bindChecking(checking);
        // 각 RawCheckingFlag는 같은 현재 DWORD를 읽으며 값 대입 시 독립 입력으로 분리된다.
        for (const [state, key] of [[this.gunState, 'checking'], [this.gunTargetState, 'checking'], [o.rewardState, 'debugAsserts'], [this.missionAiSettings, 'checkingAsserts'], [this.soundState, 'debug'], [this.buildArrivalState, 'checking'], [this.geyserMinedState, 'checking'], [this.assetState, 'checkingForms'], [this.regularState, 'checkingForms'], [this.motionState.carrier, 'debug'], [this.motionState.cleanup, 'checking'], [this.generalDamageState, 'checking'], [this.burningState, 'checking'], [o.priestState.spawn, 'checkingPlacement'], [o.priestState.forcefieldRegular, 'debug'], [this.carrierDropState, 'checking']]) bindFlag(state, key, checking);
        const diagnostics = { checking: 0, assertion: assertion(o, 'CanonDecoder') }; bindFlag(diagnostics, 'checking', checking); o.metadata.geometry.decoderDiagnostics = diagnostics;
        // 네 실제 Canon 소비자는 같은 진단 객체를 빌린다.
        for (const state of [this.constructionState.request, this.constructionState.confirm, this.constructionState.place, this.constructionState.clear]) state.decoderDiagnostics = diagnostics;
        core.postPop.bindManaDiagnostics({ checking, assertion: assertion(o, '마나 공급원', true) });
        core.unpop.bindDiagnostics({ checking, types: core.types, assertion: assertion(o, 'Unpop', true) });
        core.pop.bindDiagnostics({ checking, types: core.types, metamap: () => storage.ownerMode.challenge, trace: (level, text) => storage.traceGraph(level, text), assertion: assertion(o, 'Pop', true), worldCells: () => g.worldCells().value });
        core.factory.bindDiagnostics({ types: core.types, assertion: assertion(o, 'Take/postTake', true), detail: (text) => storage.recordAssertionDetail(text), checking });
        o.hp.bindHitPointDiagnostics(assertion(o, '보상 TYPE HP', true)); core.building.bindNotifyDiagnostics(checking, assertion(o, '지면 통지', true));
        core.graph.bindDiagnostics({ checking, assertion: assertion(o, 'Graph 참조') }); this.anchorState.graphAssertion = assertion(o, 'Graph SID 조회'); this.placement.anchor.graphAssertion = this.anchorState.graphAssertion;
        this.assetState.sharedAuthority = this.gunState.sharedAuthority = this.regularState.sharedAuthority = g.authority(); this.assetState.inBattle = true; this.assetState.localOwner = 1;
        this.#diagnosticsBound = true;
    }
}
