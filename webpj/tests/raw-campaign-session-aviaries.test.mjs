// Session의1665..1667 조립 순서/현재 참조만 확인한다(WB04-E609).
// 공중3종 전용 생산/생성/행동·우클릭 검사나 관측을 추가하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawSunAviaryProcess } from '../www/core/raw-sun-aviary-process.js';
import { RawWindAviaryProcess } from '../www/core/raw-wind-aviary-process.js';
import { RawRainAviaryProcess } from '../www/core/raw-rain-aviary-process.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';

// 미선택 외부 서비스를 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 실제 보호 자산과 팔레트다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 외부 장치만 명시 입력으로 공급하는 실제 기반 owner다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 첫 저장 객체 전의 실제 생성자 순서다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); o.connectConstruction(); o.connectDamageable(); o.connectIceTower(); o.connectCarrierSteal(); o.connectBombPostPop(); o.connectAcidZap(); o.connectAcidFence(); o.connectCarrierRegular(); }

test('Session Aviaries 조립: 원문 순서/반복/반납과 Pool·난수 무효과를 확인한다', () => {
    const o = owner(); try { assert.throws(() => o.connectSunAviary(), /초기화 전/); assert.throws(() => o.connectWindAviary(), /초기화 전/); assert.throws(() => o.connectRainAviary(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectSunAviary(); assert.ok(o.sunAviary instanceof RawSunAviaryProcess); assert.equal(o.aviaryModules.sunReady, true); assert.throws(() => o.connectSunAviary(), /반복/); assert.throws(() => o.connectRainAviary(), /초기화 전/); o.connectWindAviary(); assert.ok(o.windAviary instanceof RawWindAviaryProcess); assert.equal(o.aviaryModules.windReady, true); assert.throws(() => o.connectWindAviary(), /반복/); o.connectRainAviary(); assert.ok(o.rainAviary instanceof RawRainAviaryProcess); assert.equal(o.aviaryModules.ready, true); assert.throws(() => o.connectRainAviary(), /반복/); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.equal(o.pendingFlyer, null); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    assert.throws(() => o.connectSunAviary(), /반납/); assert.throws(() => o.aviaryModules.sunHooks.findProcess(50, 32), /반납/);
});

test('Session Aviaries 참조: 같은 현재 서버·시계·역할과 독립 fort/편집/권한을 읽는다', () => {
    const o = owner(); try { before(o); o.connectSunAviary(); o.connectWindAviary(); o.connectRainAviary(); const m = o.aviaryModules; assert.equal(m.sunGlobals.processType(), 32); assert.equal(m.windGlobals.processType(), 59); assert.equal(m.rainGlobals.processType(), 60); assert.equal(m.sunGlobals.flyerType(), 73); assert.equal(m.windGlobals.flyerType(), 138); assert.equal(m.rainGlobals.flyerType(), 144); assert.equal(m.windGlobals.aviaryType(), 139); o.core.processState.now = 12.75; o.core.processState.server = false; o.globals.editor = 2; o.gameplayGlobals.authority().boss = true;
        // 전용 생산/생성 메서드를 호출하지 않고 참조 getter만 조회한다.
        for (const globals of [m.sunGlobals, m.windGlobals, m.rainGlobals]) { assert.equal(globals.now(), 12.75); assert.equal(Boolean(globals.server()), false); assert.equal(globals.editor(), 2); assert.equal(Boolean(globals.boss()), true); }
        o.sunAviaryType = 31; o.windFlyerType = 137; o.windAviaryAsset = 140; assert.equal(m.sunGlobals.processType(), 31); assert.equal(m.windGlobals.flyerType(), 137); assert.equal(m.windGlobals.aviaryType(), 140);
    } finally { o.dispose(); }
});

test('Session Aviaries 참조: checking/친군사격 사본은 원문 전역 owner의 현재 DWORD를 유지한다', () => {
    const o = owner(); try { before(o); o.connectSunAviary(); o.connectWindAviary(); o.connectRainAviary(); const m = o.aviaryModules; const checking = o.gameplayGlobals.aiDebug().checking(); const settings = o.gameplayGlobals.gameAux().missionSettings(); checking.value = 0x80000000; settings[3] = 0x80; o.state.gunState.checking = 0; o.state.gunTargetState.friendlyFire = 0; assert.equal(m.sunGlobals.checking() >>> 0, 0x80000000); assert.equal(m.windGlobals.checking() >>> 0, 0x80000000); assert.equal(m.rainGlobals.checking() >>> 0, 0x80000000); assert.equal(m.windGlobals.friendlyFire(), 0x80); assert.equal(m.rainGlobals.friendlyFire(), 0x80); checking.value = 0; settings[3] = 0; assert.equal(m.windGlobals.checking(), 0); assert.equal(m.rainGlobals.friendlyFire(), 0); } finally { o.dispose(); }
});
