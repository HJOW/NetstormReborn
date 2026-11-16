// Session Fence의 현재 공간/예약·실제 부모 결선 계약이다(WB04-E612). 실제 미션/교전은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawFenceNeighbors } from '../www/core/raw-fence-neighbors.js';
import { RawFenceLine } from '../www/core/raw-fence-line.js';
import { RawFencePostPop } from '../www/core/raw-fence-post-pop.js';
import { RawFenceMarkerCreate, RawFenceShieldCreate } from '../www/core/raw-fence-create.js';
import { RawFenceRebuild } from '../www/core/raw-fence-rebuild.js';
import { RawFenceRegular } from '../www/core/raw-fence-regular.js';
import { RawFenceMarkerClear } from '../www/core/raw-fence-marker-clear.js';
import { RawFenceLifetime } from '../www/core/raw-fence-lifetime.js';
import { RegularProcess } from '../www/core/squid-process.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put, floatToBits } from './raw-support.mjs';

// 미선택 외부 서비스를 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 기존 보호 자료와 실제 팔레트를 공유한다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 외부 장치만 명시적으로 공급하는 실제 기반 owner다.
function owner() { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden } }); }
// 첫 저장 객체 전까지 원문 순서로 조립한다. 공중3종은 생성/생산/행동을 실행하지 않는다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); o.connectConstruction(); o.connectDamageable(); o.connectIceTower(); o.connectCarrierSteal(); o.connectBombPostPop(); o.connectAcidZap(); o.connectAcidFence(); o.connectCarrierRegular(); o.connectSunAviary(); o.connectWindAviary(); o.connectRainAviary(); o.connectArcSpire(); o.connectFlyer(); }
// 실제 몸체를 조립하되 World/Session을 게시하지 않는다.
function scene() { const o = owner(); before(o); o.connectFence(); return o; }

test('Session Fence: 아홉 실제 몸체/선행/반복/반납과 생성 중 Pool·난수 무효과', () => {
    const o = owner(); try { assert.throws(() => o.connectFence(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectFence(); assert.equal(o.fenceModules.ready, true); assert.ok(o.fenceNeighbors instanceof RawFenceNeighbors); assert.ok(o.fenceLine instanceof RawFenceLine); assert.ok(o.fencePostPop instanceof RawFencePostPop); assert.ok(o.fenceMarkerCreate instanceof RawFenceMarkerCreate); assert.ok(o.fenceShieldCreate instanceof RawFenceShieldCreate); assert.ok(o.fenceRebuild instanceof RawFenceRebuild); assert.ok(o.fenceMarkerClear instanceof RawFenceMarkerClear); assert.ok(o.fenceRegular instanceof RawFenceRegular); assert.ok(o.fenceLifetime instanceof RawFenceLifetime); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.throws(() => o.connectFence(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    assert.throws(() => o.connectFence(), /반납/); assert.throws(() => o.fenceModules.reserveRegular(), /반납/);
});

test('Session Fence: 현재 TYPE 사거리와 같은 역할/테마를 조회하며 지역 사본으로 바꾸지 않는다', () => {
    const o = scene(); try { const m = o.fenceModules; const sid = o.world.create(136, 2); o.core.types[136].range = 17.75; assert.equal(m.ranges[136], 17.75); o.core.types[136] = { ...o.core.types[136], range: 11.5 }; assert.equal(m.ranges[136], 11.5); o.fenceShieldType = 169; o.fenceNodeType = 91; assert.equal(m.shieldType(), 169); assert.equal(m.markerType(), 91); o.themes[136] = 3; assert.equal(m.postHooks.theme(sid), 3); o.themes[136] = 2; assert.equal(m.postHooks.theme(sid), 2); } finally { o.dispose(); }
});

test('Session Fence: 공유 예약은 교체/누락과 같은 Host 객체·효과 뒤 현재 시각을 유지한다', () => {
    const o = scene(); try { const m = o.fenceModules; const sid = o.world.create(136, 2); assert.equal(m.postHooks.reserveRegular, m.regularHooks.reserveRegular); assert.equal(m.regularHooks.reserveRegular, m.lifetimeHooks.reserveRegular); assert.equal(m.postHooks.addRegular, m.lifetimeHooks.addRegular); assert.throws(() => m.addRegular(sid, 2, 0.1), /예약 객체 누락/); m.reserveRegular(); const first = o.fencePendingRegular; m.reserveRegular(); const pending = o.fencePendingRegular; assert.ok(pending instanceof RegularProcess); assert.notEqual(first, pending); assert.equal(first.pid, 0); const attach = o.core.processes.attachRegular.bind(o.core.processes); o.core.processes.attachRegular = (...args) => { const task = attach(...args); task.setInitialCount(7); o.core.processState.now = 8.75; return task; }; m.addRegular(sid, 2, 0.1); assert.equal(o.fencePendingRegular, null); assert.equal(o.core.processes.findEvent(sid, 2), pending); assert.equal(pending.count, 0); assert.equal(pending.time, 8.75); assert.equal(pending.payload, Math.fround(0.1)); } finally { o.dispose(); }
});

test('Session Fence: 예약 부착 실패는 같은 예약 소비 뒤 전파하며 반납은 미부착만 해제한다', () => {
    const o = scene(); const attach = o.core.processes.attachRegular; try { const sid = o.world.create(136, 2); o.core.processes.attachRegular = () => null; o.fenceModules.reserveRegular(); assert.throws(() => o.fenceModules.addRegular(sid, 3, 0.1), /부모 부착 실패/); assert.equal(o.fencePendingRegular, null); o.fenceModules.reserveRegular(); } finally { o.core.processes.attachRegular = attach; o.dispose(); } assert.equal(o.fencePendingRegular, null);
});

test('Session Fence: 같은 현재 양방향 공간 입력과 동맹을 기존 최근접 몸체가 읽는다', () => {
    const o = scene(); try { const sid = o.world.create(136, 2); const other = o.world.create(136, 2); const a = o.core.pool.physical(sid); const b = o.core.pool.physical(other);
        // 좌표/소유자만 명시 입력으로 주며 검색/동맹 계산은 기존 실제 몸체에 맡긴다.
        for (const [raw, x, owner] of [[a, 50, 1], [b, 60, 2]]) { put(raw, 14, floatToBits(x)); put(raw, 18, floatToBits(50)); raw[34] = owner; raw[40] = 0; }
        o.core.types[136].range = 20; o.alliances[2 * 9 + 1] = 0; assert.equal(o.fenceNeighbors.select(sid, [other], true), 0); o.alliances[2 * 9 + 1] = 1; assert.equal(o.fenceNeighbors.select(sid, [other], true), other); o.core.types[136].range = 1; assert.equal(o.fenceNeighbors.select(sid, [other], true), 0);
    } finally { o.dispose(); }
});

test('Session Fence: 후처리/주기/수명의 이웃 제한과 연결선 mode0 인수를 보존한다', () => {
    const o = scene(); const saved = [o.fenceNeighbors.neighbor, o.fenceLine.clear]; try { const calls = []; const m = o.fenceModules; o.fenceNeighbors.neighbor = (...args) => { calls.push(['neighbor', ...args]); return 51; }; o.fenceLine.clear = (...args) => calls.push(['line', ...args]); assert.equal(m.postHooks.neighbor(50, 2), 51); assert.equal(m.regularHooks.neighbor(50, 4), 51); assert.equal(m.rebuildHooks.neighbor(50, 6), 51); assert.equal(m.lifetimeHooks.neighbor(50, 0, false), 51); const points = [1, 2, 3, 4]; m.lifetimeHooks.clearLine(points, 1, 2, 50, 51); assert.deepEqual(calls, [['neighbor', 50, 2], ['neighbor', 50, 4], ['neighbor', 50, 6, true], ['neighbor', 50, 0, false], ['line', points]]); } finally { [o.fenceNeighbors.neighbor, o.fenceLine.clear] = saved; o.dispose(); }
});

test('Session Fence: 가상 생성/표면/소유권·공통 사전/후처리와 수명 피해 전처리를 구별한다', () => {
    const o = scene(); const saved = [o.world.create, o.core.building.notifySurface, o.ownership.setOwner, o.world.pop, o.core.postPop.validateBase, o.carrier.damageablePostPop, o.damageablePre.preDestroy]; try { const calls = []; o.world.create = (...args) => { calls.push(['create', ...args]); return 50; }; o.core.building.notifySurface = (...args) => calls.push(['surface', ...args]); o.ownership.setOwner = (...args) => calls.push(['owner', ...args]); o.world.pop = (...args) => calls.push(['pop', ...args]); o.core.postPop.validateBase = (...args) => calls.push(['validate', ...args]); o.carrier.damageablePostPop = (...args) => calls.push(['post', ...args]); o.damageablePre.preDestroy = (...args) => calls.push(['pre', ...args]); const m = o.fenceModules; const projected = {}; assert.equal(m.createHooks.create(90, 2), 50); m.createHooks.notifySurface(50); m.createHooks.setOwner(50, 1); m.createHooks.pop(50, 1.5, 2.5, 0x40); m.postHooks.validateBase(50, 1, projected); m.postHooks.damageable(50, 1); m.lifetimeHooks.damageable(50, 0x40); assert.deepEqual(calls, [['create', 90, 2], ['surface', 50], ['owner', 50, 1], ['pop', 50, 1.5, 2.5, 0x40], ['validate', 50, 1, projected], ['post', 50, 1], ['pre', 50, 0x40]]); } finally { [o.world.create, o.core.building.notifySurface, o.ownership.setOwner, o.world.pop, o.core.postPop.validateBase, o.carrier.damageablePostPop, o.damageablePre.preDestroy] = saved; o.dispose(); }
});

test('Session Fence: 같은 현재 공통 Regular 반환과 선택된 하위 예외를 유지한다', () => {
    const o = scene(); const regular = o.assetBindings.regular; const saved = [regular.damageableHandle, o.fenceRebuild.rebuild]; try { const calls = []; regular.damageableHandle = (...args) => { calls.push(args); return 0.25; }; assert.equal(o.fenceModules.regularHooks.damageable(50, 7, 3, 0.2), 0.25); assert.deepEqual(calls, [[50, 7, 3, 0.2]]); const stop = new Error('현재 펜스 재생성 오류'); o.fenceRebuild.rebuild = () => { throw stop; }; assert.throws(() => o.fenceModules.regularHooks.rebuild(50), (error) => error === stop); } finally { [regular.damageableHandle, o.fenceRebuild.rebuild] = saved; o.dispose(); }
});
