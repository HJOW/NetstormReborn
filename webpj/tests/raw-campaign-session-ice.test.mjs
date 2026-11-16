// ConnectIceTower의 같은 Host/현재 권한·좌표 절삭/WORD·후처리 결선 계약이다(WB04-E605).
// 실제 미션 적재/전체 Kernel 재성장 반복과 자연 플레이 관측은 아니다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawIceTower } from '../www/core/raw-ice-tower.js';
import { RegularProcess } from '../www/core/squid-process.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put } from './raw-support.mjs';

// 미선택 외부 효과는 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 보호 자산과 실제 팔레트다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 외부 서비스만 명시 입력으로 공급하는 실제 기반 owner다.
function owner() { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden } }); }
// 첫 객체 이전의 Source 생성자 순서다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); o.connectConstruction(); o.connectDamageable(); }
// 전체 World/Session 게시 전의 실제 아이스 부모다.
function scene() { const o = owner(); before(o); o.connectIceTower(); return o; }

test('Session IceTower: 선행/반복/반납 관문과 실제 몸체/무효과 생성/미공개', () => {
    const o = owner(); try { assert.throws(() => o.connectIceTower(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectIceTower(); assert.equal(o.iceModules.ready, true); assert.ok(o.iceTower instanceof RawIceTower); assert.equal(o.pendingIceRegular, null); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.deepEqual(o.iceModules.types, { ice: 147, growing: 150, noIsland: 157 }); assert.throws(() => o.connectIceTower(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    assert.throws(() => o.connectIceTower(), /반납/); assert.throws(() => o.iceModules.hooks.grid(0, 0), /반납/);
});

test('Session IceTower: 같은 Regular 예약을 재초기화/한 번 부착하고 효과 뒤 현재 시각으로 ResetTask한다', () => {
    const o = scene(); try { const sid = o.world.create(150, 2); const h = o.iceModules.hooks; assert.throws(() => h.regular(sid, 0x3a6, 0.25), /예약 누락/); assert.equal(h.reserveRegular(), true); const pending = o.pendingIceRegular; assert.ok(pending instanceof RegularProcess); const attach = o.core.processes.attachRegular.bind(o.core.processes); o.core.processes.attachRegular = (...args) => { const task = attach(...args); task.setInitialCount(99); o.core.processState.now = 19.75; return task; }; h.regular(sid, 0x3a6, 0.25); assert.equal(o.pendingIceRegular, null); assert.equal(o.core.processes.findEvent(sid, 0x3a6), pending); assert.equal(pending.count, 0); assert.equal(pending.time, 19.75); assert.equal(pending.payload, 0.25); assert.equal(pending.shared, false); } finally { o.dispose(); }
});

test('Session IceTower: 부착 실패는 예약 소비 뒤 전파하고 미부착 예약 교체/반납은 PID를 만들지 않는다', () => {
    const o = scene(); try { const sid = o.world.create(150, 2); const h = o.iceModules.hooks; const attach = o.core.processes.attachRegular; h.reserveRegular(); const first = o.pendingIceRegular; h.reserveRegular(); assert.notEqual(o.pendingIceRegular, first); assert.equal(first.pid, 0); o.core.processes.attachRegular = () => null; assert.throws(() => h.regular(sid, 0x3a6, 1), /부착 실패/); assert.equal(o.pendingIceRegular, null); o.core.processes.attachRegular = attach; h.reserveRegular(); const last = o.pendingIceRegular; assert.equal(last.pid, 0); } finally { o.dispose(); } assert.equal(o.pendingIceRegular, null);
});

test('Session IceTower: grid는 float32/signed64 절삭 하위DWORD와 비유한0을 같은 실제 Hash에 적용한다', () => {
    const o = scene(); try { const grid = o.iceModules.hooks.grid; const hash = o.core.hash; hash.setBucket(0, 0, 0, 101); hash.setBucket(0, 2, 3, 102); hash.setBucket(0, 255, 0, 103);
        // 합성 Hash 입력으로 원문 캐스트/보드 범위만 검사하며 실제 지형을 만들지 않는다.
        for (const [x, y, expected] of [[NaN, Infinity, 101], [-Infinity, NaN, 101], [-0.5, 0.5, 101], [2 ** 32, 0, 101], [-(2 ** 63), 0, 101], [2 ** 63, 0, 101], [-1, 0, 0], [256, 0, 0], [0, -1, 0], [1.999999999, 2.999999999, 102], [255.9, 0.1, 103]]) assert.equal(grid(x, y), expected, `${x},${y}`);
    } finally { o.dispose(); }
});

test('Session IceTower: WORD 두 BYTE를 쓴 뒤 현재 Bridge TYPE일 때만 실제 표면에 통지한다', () => {
    const o = scene(); try { const sid = o.world.create(83, 2); const raw = o.core.pool.physical(sid); raw[10] = o.core.bridgeState.bridgeType; raw[14] = 0x67; const notify = o.core.building.notifySurface; const calls = []; o.core.building.notifySurface = (parent) => calls.push([parent, raw[12], raw[13]]); o.iceModules.hooks.word(sid, 0x12345); assert.deepEqual(calls, [[sid, 0x45, 0x23]]); assert.equal(raw[14], 0x67); raw[10] = 83; o.iceModules.hooks.word(sid, 0xffff); assert.equal(calls.length, 1); assert.deepEqual([...raw.subarray(12, 14)], [255, 255]); o.core.building.notifySurface = notify; } finally { o.dispose(); }
});

test('Session IceTower: 소유자 BYTE/가상 공간·삭제와 프레임은 현재 실제 부모에 원문 인자를 보낸다', () => {
    const o = scene(); try { const h = o.iceModules.hooks; const calls = []; const saved = [o.ownership.setOwner, o.world.unpop, o.world.pop, o.world.destroy, o.core.frame.set]; o.ownership.setOwner = (...args) => calls.push(['owner', ...args]); o.world.unpop = (...args) => calls.push(['unpop', ...args]); o.world.pop = (...args) => calls.push(['pop', ...args]); o.world.destroy = (...args) => calls.push(['destroy', ...args]); o.core.frame.set = (...args) => calls.push(['frame', ...args]); h.owner(50, 0x102); h.unpop(51, 9); h.pop(52, 10.5, 11.5, 8); h.destroy(53, 0x200000); h.setFrame(54, -1, 7); assert.deepEqual(calls, [['owner', 50, 2], ['unpop', 51, 9], ['pop', 52, 10.5, 11.5, 8], ['destroy', 53, 0x200000], ['frame', 54, -1, 7]]); [o.ownership.setOwner, o.world.unpop, o.world.pop, o.world.destroy, o.core.frame.set] = saved; } finally { o.dispose(); }
});

test('Session IceTower: 받침/피해 후처리는 효과 직전 현재 시각/전투 모드/population을 준비한다', () => {
    const o = scene(); try { const h = o.iceModules.hooks; assert.throws(() => h.dropIsland(1, 2, 3, 4), /받침 정리 미설치/); const calls = []; o.core.processState.now = 21; o.buildingEffects = { dropIsland: (...args) => { assert.equal(o.state.buildingState.now, 21); calls.push(args); } }; h.dropIsland(1, 2, 3, 4); const post = o.damageablePost.postDestroy; o.core.ownerMode.battle = false; o.dudePopulation.count = 3; o.damageablePost.postDestroy = (sid, flags) => { assert.equal(o.state.damageablePostState.battle, false); assert.equal(o.state.damageablePostState.dudeCount, 3); calls.push([sid, flags]); }; h.damageablePost(50, 9); assert.deepEqual(calls, [[1, 2, 3, 4], [50, 9]]); o.damageablePost.postDestroy = post; o.dudePopulation.count = 0; } finally { o.dispose(); }
});

test('Session IceTower: 완성형 재성장은 현재 Server와 loading을 읽으며 battle 값으로 대체하지 않는다', () => {
    const o = scene(); try { const sid = o.world.create(147, 2); const raw = o.core.pool.physical(sid); raw[40] = 0; const unpop = o.world.unpop; const post = o.damageablePost.postDestroy; const stop = new Error('현재 재성장 Unpop'); let posts = 0; o.world.unpop = () => { throw stop; }; o.damageablePost.postDestroy = () => { posts += 1; }; o.core.ownerMode.battle = false; o.gameplayGlobals.authority().server = true; assert.throws(() => o.iceTower.postDestroy(sid, 0), (error) => error === stop); o.gameplayGlobals.authority().server = false; o.core.ownerMode.battle = true; o.iceTower.postDestroy(sid, 0); assert.equal(posts, 1); o.gameplayGlobals.authority().server = true; o.objectState.loadingDepth = 1; o.iceTower.postDestroy(sid, 0); assert.equal(posts, 2); o.objectState.loadingDepth = 0; o.world.unpop = unpop; o.damageablePost.postDestroy = post; } finally { o.dispose(); }
});

test('Session IceTower: 성장 완료는 현재 boss/역할 TYPE을 읽고 실제 생성 실패를 전파한다', () => {
    const o = scene(); try { const sid = o.world.create(150, 2); const raw = o.core.pool.physical(sid); raw[40] = 0; put(raw, 36, o.core.frames[150].codes.length - 1); const create = o.world.create; const stop = new Error('현재 완성형 생성'); const calls = []; o.world.create = (...args) => { calls.push(args); throw stop; }; o.gameplayGlobals.authority().boss = false; assert.equal(o.iceTower.handle(sid, 0x3a6, 0, 1), Math.fround(40 / o.core.frames[150].codes.length)); assert.deepEqual(calls, []); o.gameplayGlobals.authority().boss = true; o.iceModules.types.ice = 148; assert.throws(() => o.iceTower.handle(sid, 0x3a6, 0, 1), (error) => error === stop); assert.deepEqual(calls, [[148, 0]]); o.world.create = create; } finally { o.dispose(); }
});
