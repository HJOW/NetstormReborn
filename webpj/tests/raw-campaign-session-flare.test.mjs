// ConnectFlare의 실제 SHP/World·Regular/현재 상태 부모를 대조한다(WB04-E600).
// 보호 자산/실제 모듈·명시 소비자 계약이며 실제 게임/연구·주문 효과 관측은 아니다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawFlare } from '../www/core/raw-flare.js';
import { RegularProcess } from '../www/core/squid-process.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put } from './raw-support.mjs';

// 선택하지 않은 외부 효과는 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 같은 보호 자산·팔레트와 실제 기반 owner다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 서비스만 명시 입력으로 공급하고 같은 실제 하위 모듈을 사용한다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 원문 생성자 순서로 Factory까지 연결한다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); }
// 현재 Flare까지 구성한다. World/Session은 게시하지 않는다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectFlare(); return o; }

test('Session Flare: 선행/반복/반납 관문과 같은 실제 풀·무효과 생성/미공개를 유지한다', () => {
    const o = owner(); try { assert.throws(() => o.connectFlare(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectFlare(); assert.equal(o.flareModules.ready, true); assert.ok(o.flare instanceof RawFlare); assert.equal(o.flare.pool, o.core.pool); assert.equal(o.pendingFlareRegular, null); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.throws(() => o.connectFlare(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    assert.throws(() => o.connectFlare(), /반납/); assert.throws(() => o.flareModules.hooks.reserveRegular(), /반납/);
});

test('Session Flare: 억제/로딩/현재 역할은 같은 loginEnd/Template·GameAux를 읽고 실제 조기는 난수를 쓰지 않는다', () => {
    const o = scene(); try { const s = o.flareModules.state; const bytes = o.core.pool.bytes(); const seed = o.random.state; o.loginEnd = 1.5; o.flare.spawn(0x7fffffff, 3, 4); assert.equal(s.suppressedTime, 1.5); o.loginEnd = 0; o.objectState.loadingDepth = 2; o.flare.spawn(0x7fffffff, 3, 4); assert.equal(s.loadingDepth, 2); o.flareType = 105; assert.equal(s.type, 105); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); } finally { o.dispose(); }
});

test('Session Flare: 기준점은 현재 TYPE/프레임의 보호 SHP 헤더이고 음수/범위 밖을 거부한다', () => {
    const o = scene(); try { const sid = o.world.create(85, 0); const raw = o.core.pool.physical(sid); const h = o.flareModules.hooks; const frame = o.core.shapes[85].frames[0]; put(raw, 36, 0); assert.deepEqual(h.sourceHotspot(sid), [frame.hotspotX, frame.hotspotY]); assert.notDeepEqual(h.sourceHotspot(sid), [frame.width, frame.height]); put(raw, 36, 0xffffffff); assert.throws(() => h.sourceHotspot(sid), /기준점 헤더 범위/); put(raw, 36, o.core.shapes[85].frames.length); assert.throws(() => h.sourceHotspot(sid), /기준점 헤더 범위/); put(raw, 36, 0); raw[10] = 255; assert.throws(() => h.sourceHotspot(sid), /기준점 헤더 범위/); raw[10] = 85; } finally { o.dispose(); }
});

test('Session Flare: 생성/소유자는 실제 World/Owner이며 현재 Kernel 개수를 사용한다', () => {
    const o = scene(); try { const h = o.flareModules.hooks; const sid = h.create(106, 2); assert.equal(o.flare.handles(sid), true); h.owner(sid, 2); assert.equal(o.core.pool.slot(sid)[34], 2); h.owner(sid, 0); assert.equal(o.core.pool.slot(sid)[34], 0); assert.equal(h.processCount(), o.core.kernel.size); } finally { o.dispose(); }
});

test('Session Flare: 실제 Regular 예약을 한 번 부착하고 부착 효과 뒤 현재 시각을 다음 시각에 쓴다', () => {
    const o = scene(); try { const h = o.flareModules.hooks; const sid = h.create(106, 2); const previous = h.processCount(); assert.throws(() => h.addRegular(sid, 5, 0.1), /예약 누락/); assert.equal(h.reserveRegular(), true); const reserved = o.pendingFlareRegular; assert.ok(reserved instanceof RegularProcess); assert.throws(() => h.reserveRegular(), /예약 중첩/); const attach = o.core.processes.attachRegular.bind(o.core.processes); o.core.processState.now = 1.5; o.core.processes.attachRegular = (...args) => { const result = attach(...args); o.core.processState.now = 9.75; return result; }; h.addRegular(sid, 5, 0.1); assert.equal(o.pendingFlareRegular, null); assert.equal(o.core.processes.findEvent(sid, 5), reserved); assert.equal(reserved.shared, false); assert.equal(reserved.payload, Math.fround(0.1)); assert.equal(reserved.time, 9.75); assert.equal(h.processCount(), previous + 1); } finally { o.dispose(); }
});

test('Session Flare: 선택된 부착 실패는 예약을 소비한 뒤 오류이며 성공 대역을 만들지 않는다', () => {
    const o = scene(); try { const h = o.flareModules.hooks; const sid = h.create(106, 2); const attach = o.core.processes.attachRegular; o.core.processes.attachRegular = () => null; h.reserveRegular(); assert.throws(() => h.addRegular(sid, 1, 0.1), /실제 부착 실패/); assert.equal(o.pendingFlareRegular, null); o.core.processes.attachRegular = attach; } finally { o.dispose(); }
});

test('Session Flare: 프레임·표시·공간/삭제는 같은 현재 부모에 원문 인자를 전달한다', () => {
    const o = scene(); try { const h = o.flareModules.hooks; const sid = h.create(106, 2); const calls = []; const frame = o.core.frame.set; const display = o.core.display.update; const pop = o.world.pop; const destroy = o.world.destroy; const stop = new Error('현재 실제 부모 경계');
        o.core.frame.set = (...args) => calls.push(['frame', ...args]); o.core.display.update = (raw) => { assert.equal(raw.buffer, o.core.pool.physical(sid).buffer); assert.equal(raw.byteOffset, o.core.pool.physical(sid).byteOffset); calls.push(['update']); }; o.world.pop = (...args) => { calls.push(['pop', ...args]); throw stop; }; o.world.destroy = (...args) => { calls.push(['destroy', ...args]); throw stop; }; h.frame(sid, -3, 0x1234); h.update(sid); assert.throws(() => h.pop(sid, 12.5, 13.75, 7), (error) => error === stop); assert.throws(() => h.destroy(sid, 9), (error) => error === stop); assert.deepEqual(calls, [['frame', sid, -3, 0x1234], ['update'], ['pop', sid, 12.5, 13.75, 7], ['destroy', sid, 9]]); o.core.frame.set = frame; o.core.display.update = display; o.world.pop = pop; o.world.destroy = destroy;
    } finally { o.dispose(); }
});

test('Session Flare: 위치 소리는 현재 서비스/이름표와 원문 flags·한도0을 사용한다', () => {
    const calls = []; const o = scene(); try { const h = o.flareModules.hooks; const sound = o.soundNames.lookup('powerup.wav'); o.services.sound.playOnceAt = (...args) => calls.push(['first', ...args]); h.sound(1.5, 2.5, 'powerup.wav', 0x80000000); o.services.sound.playOnceAt = (...args) => calls.push(['next', ...args]); h.sound(3, 4, 'powerup.wav', 8); assert.deepEqual(calls, [['first', 1.5, 2.5, sound, 0x80000000, 0], ['next', 3, 4, sound, 8, 0]]); o.services.sound.playOnceAt = null; assert.throws(() => h.sound(1, 2, 'powerup.wav', 0), /위치 음향 미연결/); } finally { o.dispose(); }
});

test('Session Flare: 기존 연구/업그레이드/Runtime 주문 훅이 같은 실제 억제 부모로 이어진다', () => {
    const o = scene(); try { o.loginEnd = 1; const bytes = o.core.pool.bytes(); const seed = o.random.state; o.factoryModules.researchHooks.flare(0x7fffffff, 2, 3); o.factoryModules.upgradeHooks.flare(0x7fffffff, 4, 5); o.worldHooks.bombRegular.notice(0x7fffffff, 6, 7); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); } finally { o.dispose(); }
});

test('Session Flare: 미부착 Regular 예약은 부모 반납에서 해제하고 SID/PID를 만들지 않는다', () => {
    const o = scene(); o.flareModules.hooks.reserveRegular(); const reserved = o.pendingFlareRegular; o.dispose(); assert.equal(o.pendingFlareRegular, null); assert.equal(reserved.form, 0); assert.equal(reserved.pid, 0);
});
