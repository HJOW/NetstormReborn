// 실제 Session 기반 owner에 공유 입력/생성자 진단을 결선해 기존 raw 소비자로 대조한다(WB04-E575).
// 단일 Session 공개/전체 Load/실제 미션 플레이를 대신하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawGeyserMined } from '../www/core/raw-geyser-mined.js';
import { RawGunTarget } from '../www/core/raw-gun-target.js';
import { currentForcePlacement } from '../www/core/raw-canon-placement.js';
import { allowAlliedPickupNow, geyserTypeNow, nuggetTypeNow } from '../www/core/raw-carrier-arrival.js';
import { currentNuggetType } from '../www/core/raw-path-cleanup.js';
import { createMissionFrameGateState } from '../www/core/raw-mission-frame-gate.js';
import { RawMissionStart, createMissionStartState } from '../www/core/raw-mission-start.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put, get, floatToBits } from './raw-support.mjs';

// 같은 보호 자료를 읽고 실제 owner/월드는 시험마다 새로 만든다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database);
// 읽기 호출을 하지 않는 이 생성 구간에서도 같은 VFS 참조를 보관한다.
const files = { read() { throw new Error('공유 입력 결선의 VFS 읽기 금지'); } };
// 실제 독립 캠페인 생성에 필요한 시각/소리 이름 표를 제공한다.
function scene(extra = {}) { return new RawCampaignSessionOwners(game, files, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), ...extra }); }
// 관측이 허용되지 않은 효과는 시험에서 선택 시 실패한다.
function forbidden() { throw new Error('이 공유 입력 검사에서 선택되지 않아야 하는 효과'); }

test('Session 공유 상태: Player/SP 전후 단계와 실제 경로/피해 참조를 유지한다', () => {
    const o = scene(); const s = o.state;
    try {
        assert.equal(s.process.clock, o.core.processState); assert.equal(s.start.clock, o.core.processState); assert.equal(s.generalDamageState.levels, s.priestDamageState); assert.equal(o.sp.initialized, false);
        s.start.retrying = true; assert.equal(s.retrying, true); s.snap = false; assert.equal(s.start.snapPriestDestination, false); o.priestState.movement.priest = 157; assert.equal(s.start.priestType, 157);
        assert.equal(s.gunState.checking, 0); assert.equal(o.metadata.geometry.decoderDiagnostics, null);
        o.initializePlayersAndSp(); assert.equal(o.playersReady, true); assert.equal(s.assetState.sharedAuthority, o.gameplayGlobals.authority()); assert.equal(s.regularState.sharedAuthority, s.assetState.sharedAuthority); assert.equal(s.gunState.sharedAuthority, s.assetState.sharedAuthority); assert.equal(s.assetState.inBattle, true); assert.equal(s.assetState.localOwner, 1);
        assert.throws(() => s.bindDiagnostics(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/);
    } finally { o.dispose(); }
});

test('Session HP/비용: 실제 현재 TYPE를 빌리며 최초 비용표는 현재 비용 변경과 구별한다', () => {
    const o = scene(); const aux = o.gameplayGlobals.gameAux();
    try {
        const originalCost = o.hp.typeCostUnits(75); o.core.types[75].cost = 98765; o.core.types[75].maxHitPoints = 321; assert.equal(o.hp.typeHitPoints(75), 321); assert.equal(o.hp.typeCostUnits(75), originalCost);
        aux.refundPercent = 17; aux.resourcePool = -19; aux.priestQuarterHp = true; aux.altarType = 160; aux.daisType = 161; aux.rainBlockerType = 145; aux.growingRainBlockerType = 146; aux.nuggetType = 155;
        assert.deepEqual([o.rewardState.currentPercent(), o.rewardState.currentGeyserPool(), o.rewardState.currentPriestQuarterHp(), o.rewardState.currentAltarType(), o.rewardState.currentDaisType(), o.rewardState.currentSilentType(), o.rewardState.currentAliasType(), o.rewardState.currentNuggetType()], [17, -19, true, 160, 161, 145, 146, 155]);
        o.rewardState.storeGeyserPool(-21); assert.equal(aux.resourcePool, -21); o.core.ownerMode.battle = false; assert.equal(o.rewardState.currentHalfHealthMode(), false); o.core.ownerMode.battle = true; assert.equal(o.rewardState.currentHalfHealthMode(), true);
    } finally { o.dispose(); }
});

test('Session 너겟/권한: 실제 채집 소리 콜백 뒤 같은 현재 TYPE·boss를 다시 읽는다', () => {
    const o = scene(); const s = o.state; const aux = o.gameplayGlobals.gameAux(); const auth = o.gameplayGlobals.authority();
    try {
        const sid = o.core.pool.allocate(0); const raw = o.core.pool.allocated(sid); raw[10] = 122; put(raw, 12, 40, 2); put(raw, 14, floatToBits(20)); put(raw, 18, floatToBits(21));
        o.core.types[155].cost = 40;
        const mined = new RawGeyserMined(o.core.pool, o.core.hash, o.core.types, o.random, s.geyserMinedState, { sound() { aux.nuggetType = 155; auth.boss = false; }, destroy: forbidden, create: forbidden, pop: forbidden });
        mined.handle(sid); assert.equal(get(raw, 12, 2), 0); assert.equal(s.geyserMinedState.boss, false); assert.equal(s.geyserMinedState.nuggetType, 155);
        assert.equal(s.carrierTransferState.currentNuggetType(), 155); assert.equal(s.contentLifetimeState.readNuggetType(), 155); assert.equal(nuggetTypeNow(s.motionState.carrier), 155); assert.equal(currentNuggetType(s.motionState.cleanup), 155); assert.equal(s.constructionState.place.nuggetTypeNow(), 155);
        assert.equal(s.priestDamageState.isBoss(), false); assert.equal(s.damageablePostState.currentBoss, false); assert.equal(s.contentLifetimeState.readBoss(), false); assert.equal(s.buildingState.isBoss(), false);
    } finally { o.dispose(); }
});

test('Session 같은55a490: 실제 Gun 표적 판정과 Gun 전역이 현재 전체 DWORD를 함께 읽는다', () => {
    const o = scene(); const s = o.state; const settings = o.gameplayGlobals.gameAux().missionSettings();
    try {
        const target = o.core.pool.allocate(0); const raw = o.core.pool.allocated(target); raw[10] = 75; raw[34] = 1; put(raw, 14, floatToBits(22)); put(raw, 18, floatToBits(23));
        o.core.types[75].flags1 |= 0x10; o.core.types[75].group = 3; s.gunTargetState.allyMasks[1] = 2;
        const body = new RawGunTarget(o.core.pool, o.core.hash, o.core.types, Array.from({ length: 188 }, () => ({ range: 20, theme: 0, directionFrames: [0, 0, 0, 0], heightExtension: 0 })), o.core.shapes, s.gunTargetState);
        const query = { x: 10, y: 11, centerX: 10, centerY: 11, type: 75, direction: 0, owner: 1, frame: 0 };
        settings[3] = 0; assert.equal(body.mayTarget(query, target), false); assert.equal(s.gunState.globals().friendlyFire(), false);
        settings[3] = 0x80000000; assert.equal(body.mayTarget(query, target), true); assert.equal(s.gunState.globals().friendlyFire(), true); assert.equal(settings[3], 0x80000000);
        settings[2] = 0x80000000; assert.equal(s.priestDamageState.isInvulnerable(), true); settings[1] = 0x80000000; assert.equal(currentForcePlacement(s.placement.placement), 0x80000000); assert.equal(currentForcePlacement(s.gameplayPlacement.placement), 0x80000000);
        s.gunTargetState.friendlyFire = 0; assert.equal(body.mayTarget(query, target), false); assert.equal(settings[3], 0x80000000); assert.equal(s.gunState.globals().friendlyFire(), true);
    } finally { o.dispose(); }
});

test('Session 미션 시작: 실제 Start가 장기 네 출력과 같은 SP를 쓰며 시작 관문을 갱신한다', () => {
    const o = scene();
    try {
        o.initializePlayersAndSp(); const aux = o.gameplayGlobals.gameAux(); const values = new Map([['denySalvage', '-7'], ['denyAscend', '3'], ['allowAnyCapture', '-1'], ['moreGeysers', '0'], ['myStartMoney', '2000']]); const events = [];
        const gate = createMissionFrameGateState(); const start = new RawMissionStart(gate, createMissionStartState(), o.state.missionStart, {
            get: (key) => values.get(key) ?? '', sectionNames: () => '', set: (key, value) => { values.set(key, value); }, gameSeconds: () => 12.5,
            setSp: (owner, value) => o.sp.set(owner, value), refreshSp: () => events.push('sp'), clearTechnology: (owner) => events.push(`clear${owner}`), addTechnology: (owner) => events.push(`tech${owner}`), hasPriest: () => false, createAi: forbidden, refreshCommandGump: () => events.push('gump'), randomGeysers: forbidden, resetCamera: () => events.push('camera'),
        });
        start.bindCurrentInputs({ localOwner: () => o.playerState.battleOwner, playerKnown: () => o.playerState.playerKnown, editor: () => o.globals.editor }); start.start();
        assert.equal(o.gameplayGlobals.denySalvage().value, -7); assert.equal(o.gameplayGlobals.denyAscend().value, 3); assert.equal(aux.allowAnyCapture, -1); assert.equal(aux.ordinaryGeysers, 1); assert.equal(o.sp.get(1), 2000); assert.equal(gate.started, 1); assert.equal(gate.startSeconds, 12.5); assert.deepEqual(events, ['sp', 'clear1', 'tech1', 'gump', 'camera']);
        assert.equal(allowAlliedPickupNow(o.state.motionState.carrier), true);
    } finally { o.dispose(); }
});

test('Session 공유 타입/깊이: Rune·Carrier·기도·경로가 현재 owner를 사용한다', () => {
    const o = scene(); const s = o.state; const aux = o.gameplayGlobals.gameAux();
    try {
        aux.runeType = 168; aux.altarType = 160; aux.daisType = 161; o.globals.missionPost = false;
        assert.equal(s.runeState.readRuneType(), 168); assert.equal(s.ritualState.currentRuneType, 168); assert.equal(s.runeState.readAltarType(), 160); assert.equal(s.runeState.readDaisType(), 161); assert.equal(s.runeState.readSuppressDuration(), false); assert.equal(s.damageablePostState.currentSilentSurfaceType, aux.rainBlockerType);
        o.geyserType = 121; o.objectState.loadingDepth = 9; assert.equal(s.carrierState.readGeyserType(), 121); assert.equal(o.priestState.carrier.readGeyserType(), 121); assert.equal(geyserTypeNow(s.motionState.carrier), 121); assert.equal(s.constructionState.geyser.readGeyserType(), 121); assert.equal(s.carrierState.readLoadingDepth(), 9); assert.equal(o.priestState.carrier.readLoadingDepth(), 9);
        o.gameplayGlobals.patchDiagnostic().value = 55; o.gameplayGlobals.patchSample().value = 66; assert.equal(s.motionState.search.sharedPolicySelector, o.gameplayGlobals.patchDiagnostic()); assert.equal(o.priestState.regen.patch.readChangedSample(), 55); assert.equal(o.priestState.regen.patch.readSample(), 66);
    } finally { o.dispose(); }
});

test('Session 건설 공유: 현재 권한·표면 종류·Player·이력·패턴을 소비 시점에 조회한다', () => {
    const o = scene(); const s = o.state; const aux = o.gameplayGlobals.gameAux(); const a = o.gameplayGlobals.authority();
    try {
        a.boss = false; a.server = false; aux.islandDropBlocked = 0x80000000; aux.islandStumpType = 95; o.core.bridgeState.islandType = 96; o.core.bridgeState.noIslandType = 156; o.core.bridgeState.stalagType = 94; o.core.bridgeState.bridgeType = 81; o.playerState.battleOwner = 6; o.playerState.playerKnown = 0;
        assert.equal(s.constructionState.place.authorityNow(), false); assert.equal(s.constructionState.clear.authorityNow(), false); assert.equal(s.constructionState.notice.authorityNow(), false); assert.equal(s.constructionState.geyser.readBoss(), false); assert.equal(s.constructionState.place.serverNow(), false);
        assert.equal(s.buildingEffectsState.blockedNow(), true); assert.equal(s.buildingEffectsState.serverNow(), false); assert.deepEqual([s.buildingEffectsState.islandTypeNow(), s.buildingEffectsState.noIslandTypeNow(), s.buildingEffectsState.stalagTypeNow(), s.buildingEffectsState.stumpTypeNow()], [96, 156, 94, 95]);
        assert.equal(s.constructionState.place.localPlayerNow(), 6); assert.equal(s.constructionState.notice.localPlayerNow(), 6); assert.equal(s.constructionState.place.battleShownNow(), false); assert.equal(s.constructionState.place.bridgeLifeNow(), 5);
        s.constructionState.notice.setHistoryCounter(4); assert.equal(o.gameplayGlobals.camera().constructionHistory().index, 4); assert.equal(s.constructionState.notice.historyPoints(), o.gameplayGlobals.camera().constructionHistory().points);
        o.piece = 108; s.gunState.thunderCannonType = 132; s.gunState.rainCannonType = 130; aux.windArcherType = 139; s.constructionState.confirm.patternTypes[7] = 143;
        assert.deepEqual(s.constructionState.place.currentPatternTypes(), [108, 81, 96, 156, 132, 130, 139, 143]); assert.deepEqual(s.constructionState.confirm.patternTypesRead(), [108, 81, 96, 156, 132, 130, 139, 143]);
        aux.constructionProcessType = 31; aux.quickBuild = 0x80000000; aux.quickVortex = 7; assert.equal(s.buildingState.currentType(), 31); assert.equal(s.buildingState.read('quickBuild'), 0x80000000); assert.equal(s.buildingState.read('quickVortex'), 7);
        aux.archipelago = 0x80000000; aux.worldGeyserMargin = -3; assert.equal(s.farm.isArchipelago(), true); assert.equal(s.farm.currentSpan(), -3); aux.resourcePool = -7; assert.equal(s.devastationEffectState.sharedSpentCounter.resourcePoolBits, 0xfffffff9);
    } finally { o.dispose(); }
});

test('Session FullDeck/장르: 같은 현재 DWORD를 세 소비자가 공유하고 typed 지역 값은 유지한다', () => {
    const o = scene(); const s = o.state; const aux = o.gameplayGlobals.gameAux();
    try {
        o.fullDeck = 0x80000000; o.gameplayGlobals.fortKeepGenus().value = 0xabcdef01;
        assert.equal(o.objectState.readFullDeck(), 0x80000000); assert.equal(s.constructionState.confirm.skipBuilderCheckNow(), true); assert.equal(s.anchorState.currentIgnoreRestrictions, 0x80000000); assert.equal(o.objectState.readFortKeepGenus(), 0xabcdef01); assert.equal(s.damageableReleaseState.currentAllowedGenus, 0xabcdef01);
        aux.windArcherType = 139; aux.thunderCannonType = 130; aux.sunCannonType = 127;
        assert.equal(s.gunState.readWindArcherType(), 140); assert.equal(s.gunState.readThunderCannonType(), 131); assert.equal(s.missileState.sunCannonType, 128); assert.equal(s.constructionState.request.localPlayer, 0); assert.equal(s.carrierTransferState.emptyType, 187);
    } finally { o.dispose(); }
});

test('Session checking: 한 현재 signed DWORD·같은 Canon 진단과 값 대입의 분리 의미', () => {
    const o = scene(); const s = o.state;
    try {
        o.initializePlayersAndSp(); const checking = o.gameplayGlobals.aiDebug().checking(); checking.value = -2147483648;
        // 생성자1104의 각 진단 칸은 현재 전체 비트를 읽는다.
        for (const [state, key] of [[s.gunState, 'checking'], [s.gunTargetState, 'checking'], [o.rewardState, 'debugAsserts'], [s.missionAiSettings, 'checkingAsserts'], [s.soundState, 'debug'], [s.buildArrivalState, 'checking'], [s.geyserMinedState, 'checking'], [s.assetState, 'checkingForms'], [s.regularState, 'checkingForms'], [s.motionState.carrier, 'debug'], [s.motionState.cleanup, 'checking'], [s.generalDamageState, 'checking'], [s.burningState, 'checking'], [o.priestState.spawn, 'checkingPlacement'], [o.priestState.forcefieldRegular, 'debug'], [s.carrierDropState, 'checking']]) assert.equal(state[key], 0x80000000, key);
        assert.equal(s.process.checking, 0x80000000); assert.equal(o.metadata.geometry.decoderDiagnostics.checking, 0x80000000);
        // 네 decoder는 동일 진단 객체다.
        for (const state of [s.constructionState.request, s.constructionState.confirm, s.constructionState.place, s.constructionState.clear]) assert.equal(state.decoderDiagnostics, o.metadata.geometry.decoderDiagnostics);
        s.gunTargetState.checking = 0; assert.equal(checking.value, -2147483648); checking.value = 19; assert.equal(s.gunTargetState.checking, 0); assert.equal(s.gunState.checking, 19); assert.equal(s.soundState.isDebug(), 19); assert.equal(s.generalDamageState.isChecking(), true); assert.equal(s.burningState.isChecking(), true);
    } finally { o.dispose(); }
});

test('Session 실제 진단: 호출 때 조회하는 장치와 생성 때 캡처한 장치를 구별한다', () => {
    const events = []; const ai = { assertion: (expression) => events.push(`A:${expression}`) }; const o = scene({ ai });
    try {
        o.initializePlayersAndSp(); o.gameplayGlobals.aiDebug().checking().value = 1; ai.assertion = (expression) => events.push(`B:${expression}`);
        o.state.anchorState.graphAssertion('anchor', '', 1); o.metadata.geometry.decoderDiagnostics.assertion('canon', '', 2);
        o.core.types[75].flags1 &= ~0x10; o.hp.typeHitPoints(75);
        assert.deepEqual(events.slice(0, 2), ['B:anchor', 'B:canon']); assert.ok(events.slice(2).length > 0); assert.ok(events.slice(2).every((value) => value.startsWith('A:')));
    } finally { o.dispose(); }
    const missing = scene(); try { missing.initializePlayersAndSp(); assert.throws(() => missing.state.anchorState.graphAssertion('x', '', 3), /미연결/); assert.throws(() => missing.metadata.geometry.decoderDiagnostics.assertion('x', '', 4), /미연결/); } finally { missing.dispose(); }
});

test('Session 연결 활동: 같은 두 표·현재 시각/서버와 미션 경고 억제를 적용한다', () => {
    const events = []; const o = scene({ sound: { wallSeconds: () => { events.push('wall'); return 123.75; } }, connectionFrameNumber: () => { events.push('frame'); return 456; }, removeConnectionStatus: (kind, player) => events.push([kind, player]) });
    try {
        const body = o.state.connectionActivity; body.receive(4); assert.deepEqual(events, ['wall', 'frame']); assert.equal(o.connectionActivityGlobals.tables().wallSeconds[4], 123); assert.equal(o.connectionActivityGlobals.tables().frames[4], 456);
        o.globals.missionPost = false; o.gameplayGlobals.authority().server = false; events.length = 0; body.receive(5); assert.deepEqual(events, ['wall', 'frame', [1, 0]]);
        o.gameplayGlobals.authority().server = true; events.length = 0; body.receive(6); assert.deepEqual(events, ['wall', 'frame', [2, 6]]);
        o.globals.missionPost = true; body.hooks.eraseStatus(11, 7); assert.deepEqual(events.at(-1), [11, 7]); body.hooks.eraseStatus(16, 8); assert.deepEqual(events.at(-1), [16, 8]);
    } finally { o.dispose(); }
});

test('Session 연결 활동 부분 실패: 도달한 필수 장치만 요구하며 앞선 store는 유지한다', () => {
    const o = scene();
    try {
        const body = o.state.connectionActivity; assert.throws(() => body.receive(4), /벽시각 미연결/); o.services.sound = { wallSeconds: () => 77.5 }; assert.throws(() => body.receive(4), /프레임 조회 미연결/); assert.equal(body.tables.wallSeconds[4], 0);
        o.services.connectionFrameNumber = () => 99; o.globals.missionPost = false; assert.throws(() => body.receive(4), /삭제 미연결/); assert.equal(body.tables.wallSeconds[4], 77); assert.equal(body.tables.frames[4], 99);
    } finally { o.dispose(); }
});
