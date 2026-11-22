// Flyer의 조립/현재 전역/불변 메타만 대조한다(WB04-E611).
// 공중3종의 새 전용 생성/생산/우클릭/행동·프레임 실행 검사는 추가하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { typeNumber, typeString } from '../www/assets/type-table.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { FlyerProcess, FlyerProcessFields, RawFlyerProcess, RawFlyerPostPop } from '../www/core/raw-flyer-process.js';
import { RawFlyerFrame } from '../www/core/raw-flyer-frame.js';
import { RawFlyerTarget } from '../www/core/raw-flyer-target.js';
import { RawSunFlyerFrame } from '../www/core/raw-sun-flyer-frame.js';
import { RawWindFlyer } from '../www/core/raw-wind-flyer.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { floatToBits } from './raw-support.mjs';

// 선택되지 않은 외부 서비스를 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 기존 보호 자료와 실제 팔레트를 공유한다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 현재 기반 owner에 외부 장치만 명시적으로 공급한다.
function owner() { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden } }); }
// 첫 저장 객체 전까지 원문 생성 순서로 조립한다. 유닛/생산 프로세스는 만들지 않는다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); o.connectConstruction(); o.connectDamageable(); o.connectIceTower(); o.connectCarrierSteal(); o.connectBombPostPop(); o.connectAcidZap(); o.connectAcidFence(); o.connectCarrierRegular(); o.connectSunAviary(); o.connectWindAviary(); o.connectRainAviary(); o.connectArcSpire(); }

test('Session Flyer 조립: 내부 전체 몸체와 현재 Pool·난수 무효과/선행/반복/반납', () => {
    const o = owner(); try { assert.throws(() => o.connectFlyer(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectFlyer(); assert.equal(o.flyerModules.ready, true); assert.ok(o.flyer instanceof RawFlyerProcess); assert.ok(o.flyerFrame instanceof RawFlyerFrame); assert.ok(o.flyerTarget instanceof RawFlyerTarget); assert.ok(o.flyerPostPop instanceof RawFlyerPostPop); assert.ok(o.sunFlyerFrame instanceof RawSunFlyerFrame); assert.ok(o.windFlyer instanceof RawWindFlyer); assert.equal(o.flyer.pool, o.core.pool); assert.equal(o.flyerTarget.pool, o.core.pool); assert.equal(o.flyerPostPop.pool, o.core.pool); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.equal(o.pendingFlyer, null); assert.equal(o.pendingSunFlyerSound, null); assert.equal(o.pendingWindRegular, null); assert.throws(() => o.connectFlyer(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    assert.throws(() => o.connectFlyer(), /반납/);
});

test('Session Flyer 참조: 같은 현재 시계·권한/역할과 복사 checking의 전역 포인터', () => {
    const o = owner(); try { before(o); o.connectFlyer(); const m = o.flyerModules; const aux = o.gameplayGlobals.gameAux(); const checking = o.gameplayGlobals.aiDebug().checking(); o.core.processState.server = false; o.core.processState.now = 13.75; o.gameplayGlobals.authority().boss = true; o.state.soundState.frame = 99; o.flyerPaused = true; o.sunBombType = 162; aux.flyerProcessType = 30; aux.processTypeEnd = 62; o.sunFlyerType = 74; o.windFlyerType = 137; o.rainFlyerType = 143; o.sunAviaryType = 31; o.flyerDependType = 2; checking.value = 0x80000000; o.state.gunState.checking = 0;
        // 전용 행동을 실행하지 않고 생성 시 넘긴 현재 getter만 조회한다.
        for (const globals of [m.processGlobals, m.frameGlobals, m.targetGlobals]) { assert.equal(Boolean(globals.server()), false); assert.equal(globals.now(), 13.75); assert.equal(globals.checking() >>> 0, 0x80000000); }
        assert.equal(m.processGlobals.processType(), 30); assert.equal(Boolean(m.frameGlobals.boss()), true); assert.equal(m.frameGlobals.frame(), 99); assert.equal(m.frameGlobals.sunType(), 74); assert.equal(m.frameGlobals.windType(), 137); assert.equal(m.frameGlobals.rainType(), 143); assert.equal(m.frameGlobals.sunAviaryProcess(), 31); assert.equal(m.frameGlobals.soundProcess(), o.state.soundState.currentType()); assert.equal(m.targetGlobals.processEnd(), 62); assert.equal(m.targetGlobals.dependType(), 2); assert.equal(m.sunGlobals.paused(), true); assert.equal(m.sunGlobals.bombType(), 162); assert.equal(m.windGlobals.now(), 13.75); assert.equal(m.windGlobals.frame(), 99); checking.value = 3; assert.equal(m.frameGlobals.checking(), 3); assert.equal(m.targetGlobals.checking(), 3); assert.equal(o.state.gunState.checking, 0);
    } finally { o.dispose(); }
});

test('Session Flyer 메타: 같은188TYPE의 speed 비트·소리·프레임 구간과 사거리', () => {
    const o = owner(); try { before(o); o.connectFlyer(); const m = o.flyerModules; assert.equal(m.postTypes.length, 188); assert.equal(m.frameTypes.length, 188); assert.equal(m.bombTypes.length, 188); assert.equal(m.targetRanges.length, 188);
        // 실제 자산 표의 모든 행을 원문처럼 단독 메타로만 대조한다.
        for (const asset of game.types()) { const type = 70 + asset.block; const impact = typeString(asset.definition, 'impactSound'); const active = typeString(asset.definition, 'activeSound'); assert.deepEqual(m.postTypes[type], { speed: floatToBits(Math.fround(typeNumber(asset.definition, 'speed') ?? 0)), frames: o.core.frames[type].codes.length, impactSound: impact ? o.soundNames.lookup(impact) : 0, activeSound: active ? o.soundNames.lookup(active) : 0 }); assert.equal(m.windActiveSounds[type], active ? o.soundNames.lookup(active) : 0); }
        // 빈 TYPE까지 포함한 원문188칸을 순서대로 확인한다.
        for (let type = 0; type < 188; type += 1) { const table = o.core.frames[type]; assert.deepEqual(m.frameTypes[type], { range: o.gunTypes[type].range, sound: o.missileTypes[type].firingSound }); assert.deepEqual(m.bombTypes[type], { fallingFrames: table.run('A').count, explosionFirst: table.run('B').first, explosionEnd: table.run('B').count, sound: o.missileTypes[type].firingSound }); assert.equal(m.targetRanges[type], o.gunTypes[type].range); }
    } finally { o.dispose(); }
});

// 현재 부모에 붙은 비행 프로세스의 필드를 돌려주는 연결(원본 CurrentFlyerProcess)이 설치되고, 붙은 Form 은 같은 필드를 돌려주며 범위 밖 부모는 거부한다. 이 연결이 없으면 Rain 피격이 오류로 프레임을 멈췄다. (2026-11-20 WB04-E1122)
test('Session Flyer: currentFlyerProcess 가 부모에 붙은 현재 프로세스의 필드를 돌려준다', () => {
    const o = owner();
    try {
        before(o); assert.equal(o.currentFlyerProcess, undefined); o.connectFlyer(); assert.equal(typeof o.currentFlyerProcess, 'function');
        const type = o.gameplayGlobals.gameAux().flyerProcessType, parent = o.world.create(71, 2);
        assert.throws(() => o.currentFlyerProcess(65536, 0x11e), /물리 범위/); assert.throws(() => o.currentFlyerProcess(o.core.pool.capacity, 0x11e), /물리 범위/); assert.throws(() => o.currentFlyerProcess(-1, 0x11e), /물리 범위/);
        const fields = new FlyerProcessFields(), process = new FlyerProcess(o.flyer, fields);
        assert.ok(o.core.processes.attach(process, type, parent, 0x50));
        assert.equal(o.currentFlyerProcess(parent, 0x11e), fields);
    } finally { o.dispose(); }
});

test('Session Flyer: 더스트 데빌 진단 두 개는 원문 식·파일·행으로 표시한 뒤 계속하고 다른 줄은 거부한다(E1211)', () => {
    const o = owner(); try {
        before(o); o.connectFlyer(); const h = o.flyerModules.windHooks; const bs = String.fromCharCode(92);
        assert.throws(() => h.assertion(0xa4), /진단 장치 미연결/); const shown = []; o.world.storage.bindDisplayDiagnostics({ value: '' }, (...args) => shown.push(args));
        h.assertion(0xa4); h.assertion(0x19e); assert.throws(() => h.assertion(0x123), /다른 원문 진단/);
        assert.deepEqual(shown, [['parent->getHead() != 0', bs + 'Ns' + bs + 'O' + bs + 'Baseprocess.cpp', 0xa4], ['period == WIND_FLYER_ATTACK_TIME', bs + 'Ns' + bs + 'O' + bs + 'Flyer.cpp', 0x19e]]);
    } finally { o.dispose(); }
});
