// 생성자1656..1658의 실제 예약/61h·애시드 공간·현재 역할 결선 계약이다(WB04-E607).
// 실제 미션/자동 포획·사제 피해/공중3종 생성·행동 관측은 하지 않는다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { typeNumber } from '../www/assets/type-table.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawBombPostPop } from '../www/core/raw-bomb-post-pop.js';
import { RawAcidZap } from '../www/core/raw-acid-zap.js';
import { RawAcidFence } from '../www/core/raw-acid-fence.js';
import { GameRandom } from '../www/core/random.js';
import { RegularProcess } from '../www/core/squid-process.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put, floatToBits } from './raw-support.mjs';

// 미선택 외부 서비스를 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 실제 보호 자산과 팔레트다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 외부 장치만 명시 입력으로 공급하는 실제 기반 owner다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 첫 저장 객체 전의 실제 생성자 순서다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); o.connectConstruction(); o.connectDamageable(); o.connectIceTower(); o.connectCarrierSteal(); }
// 세 효과 부모를 순서대로 구성한다. World/Session은 게시하지 않는다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectBombPostPop(); o.connectAcidZap(); o.connectAcidFence(); return o; }

test('Session Spells: 세 생성 구간의 선행/반복/반납과 같은 실제 몸체/무효과·미공개', () => {
    const o = owner(); try { assert.throws(() => o.connectAcidZap(), /초기화 전/); assert.throws(() => o.connectAcidFence(), /초기화 전/); assert.throws(() => o.connectBombPostPop(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectBombPostPop(); assert.ok(o.bombPostPop instanceof RawBombPostPop); assert.equal(o.spellModules.bombReady, true); assert.throws(() => o.connectBombPostPop(), /반복/); o.connectAcidZap(); assert.ok(o.acidZap instanceof RawAcidZap); assert.equal(o.spellModules.zapReady, true); assert.throws(() => o.connectAcidZap(), /반복/); o.connectAcidFence(); assert.ok(o.acidFence instanceof RawAcidFence); assert.equal(o.spellModules.ready, true); assert.throws(() => o.connectAcidFence(), /반복/); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    assert.throws(() => o.connectBombPostPop(), /반납/); assert.throws(() => o.spellModules.fenceHooks.range(1), /반납/);
});

test('Session Spells: 실제 세 주문 prefix가 같은 Host/난수를 사용하고 중복은 소비하지 않는다', () => {
    const o = scene(); try {
        // 세 실제 생성자/VT에 같은 prefix를 연결하고 RNG 소비 횟수를 독립 스트림으로 대조한다.
        for (const [type, random] of [[174, false], [176, true], [177, true]]) { const sid = o.world.create(type, 2); assert.equal(o.bombPostPop.handles(sid), true); const expected = new GameRandom(o.random.state); if (random) expected.next(65536); o.bombPostPop.prefix(sid, 1); assert.equal(o.random.state, expected.state); const task = o.core.processes.findEvent(sid, 1); assert.ok(task); assert.equal(task.count, 0); const seed = o.random.state; o.bombPostPop.prefix(sid, 1); assert.equal(o.core.processes.findEvent(sid, 1), task); assert.equal(o.random.state, seed); }
    } finally { o.dispose(); }
});

test('Session Spells: 주문 예약은 중첩/누락을 거부하고 같은 실제 객체에 효과 후 현재 시각을 쓴다', () => {
    const o = scene(); try { const sid = o.world.create(174, 2); const h = o.spellModules.bombHooks; assert.throws(() => h.addRegular(sid, 1, 0.05), /확보 누락/); h.reserveRegular(); const reserved = o.pendingBombPopRegular; assert.ok(reserved instanceof RegularProcess); assert.throws(() => h.reserveRegular(), /중복 예약/); assert.equal(o.pendingBombPopRegular, reserved); const attach = o.core.processes.attachRegular.bind(o.core.processes); o.core.processes.attachRegular = (...args) => { const task = attach(...args); task.setInitialCount(3); o.core.processState.now = 8.75; return task; }; h.addRegular(sid, 1, 0.05); assert.equal(o.pendingBombPopRegular, null); assert.equal(o.core.processes.findEvent(sid, 1), reserved); assert.equal(reserved.time, 8.75); assert.equal(reserved.count, 0); assert.equal(reserved.payload, Math.fround(0.05)); } finally { o.dispose(); }
});

test('Session Spells: 애시드 예약은 같은 실제 객체/Host와 분리된 예약 수명을 쓴다', () => {
    const o = scene(); try { const sid = o.world.create(145, 2); const h = o.spellModules.fenceHooks; assert.throws(() => h.regular(sid, 1, 0.2), /예약 누락/); h.reserveRegular(); const first = o.pendingAcidRegular; h.reserveRegular(); const pending = o.pendingAcidRegular; assert.notEqual(first, pending); assert.equal(first.pid, 0); const attach = o.core.processes.attachRegular.bind(o.core.processes); o.core.processes.attachRegular = (...args) => { const task = attach(...args); o.core.processState.now = 17.5; return task; }; h.regular(sid, 1, 0.2); assert.equal(o.pendingAcidRegular, null); assert.equal(o.core.processes.findEvent(sid, 1), pending); assert.equal(pending.time, 17.5); assert.equal(pending.count, 0); assert.equal(pending.payload, Math.fround(0.2)); o.spellModules.bombHooks.reserveRegular(); h.reserveRegular(); assert.notEqual(o.pendingBombPopRegular, o.pendingAcidRegular); } finally { o.dispose(); } assert.equal(o.pendingAcidRegular, null); assert.equal(o.pendingBombPopRegular, null);
});

test('Session Spells: 두 부착 실패는 현재 예약 소비 뒤 명시 오류로 전파한다', () => {
    const o = scene(); try { const sid = o.world.create(145, 2); const attach = o.core.processes.attachRegular; o.core.processes.attachRegular = () => null; o.spellModules.bombHooks.reserveRegular(); assert.throws(() => o.spellModules.bombHooks.addRegular(sid, 1, 0.05), /부착 실패/); assert.equal(o.pendingBombPopRegular, null); o.spellModules.fenceHooks.reserveRegular(); assert.throws(() => o.spellModules.fenceHooks.regular(sid, 1, 0.2), /부착 실패/); assert.equal(o.pendingAcidRegular, null); o.core.processes.attachRegular = attach; } finally { o.dispose(); }
});

test('Session Spells: 61h writer는 같은 현재 헤더와 세 WORD/방향 BYTE를 사용한다', () => {
    const o = scene(); try { assert.deepEqual([...o.acidZap.build(0x12345, 0x23456, 0x34567, -2)], [1, 0x61, 11, 0, 0x45, 0x23, 0x56, 0x34, 0x67, 0x45, 254]); Object.assign(o.spellModules.zapHeader, { protocol: 0xfe, message: 0x62, length: 0x1234 }); assert.deepEqual([...o.acidZap.build(1, 2, 3, 0).subarray(0, 4)], [0x7e, 0x62, 0x34, 0x12]); } finally { o.dispose(); }
});

test('Session Spells: 현재 수신 제외 TYPE를 효과 전에 읽고 위치 소리의 선택 실패를 전파한다', () => {
    const o = scene(); try { const victim = o.world.create(83, 2); const first = o.world.create(78, 2); const packet = o.acidZap.build(victim, first, first, 1); const state = o.spellModules.zapState; state.altar = 83; o.acidZap.receive(0xffffffff, 99, packet); state.altar = 164; state.growingIceTower = 83; o.acidZap.receive(0, 0, packet); state.growingIceTower = 150; const stop = new Error('현재 위치 소리'); o.services.sound.playOnceAt = () => { throw stop; }; assert.throws(() => o.acidZap.receive(0, 0, packet), (error) => error === stop); } finally { o.dispose(); }
});

test('Session Spells: 위치 소리→분해 인자→가상 삭제는 현재 같은 실제 장치로 전달한다', () => {
    const o = scene(); try { const h = o.spellModules.zapHooks; const calls = []; const dissolve = o.particles.addDissolve; const destroy = o.world.destroy; o.services.sound.playOnceAt = (...args) => calls.push(['sound', ...args]); o.particles.addDissolve = (...args) => calls.push(['dissolve', ...args]); o.world.destroy = (...args) => calls.push(['destroy', ...args]); assert.throws(() => h.sound(1, 2, 'dissolve.wav', 1), /단발 위치 음향/); assert.deepEqual(calls, []); h.sound(1.5, 2.5, 'dissolve.wav', 0); h.dissolve(50, 51, 52, -1); h.destroy(50, 0x300048); assert.deepEqual(calls, [['sound', 1.5, 2.5, o.soundNames.lookup('dissolve.wav'), 0, 0], ['dissolve', 50, 51, 52, -1], ['destroy', 50, 0x300048]]); o.particles.addDissolve = dissolve; o.world.destroy = destroy; } finally { o.dispose(); }
});

test('Session Spells: range는 현재 TYPE의 실제 필수 자산 수치를 float로 읽는다', () => {
    const o = scene(); try { const sid = o.world.create(145, 2); const h = o.spellModules.fenceHooks; assert.equal(h.range(sid), Math.fround(typeNumber(game.types()[75].definition, 'range'))); o.core.types[145].range = 9999; assert.equal(h.range(sid), Math.fround(typeNumber(game.types()[75].definition, 'range'))); const raw = o.core.pool.physical(sid); raw[10] = 1; assert.throws(() => h.range(sid), /TYPE 범위/); raw[10] = 145; } finally { o.dispose(); }
});

test('Session Spells: flags1 순회는 같은 Hash 체인/현재 후보와 소비 전 저장한 다음 링크를 유지한다', () => {
    const o = scene(); try { const first = o.world.create(83, 2); const second = o.world.create(83, 2); const a = o.core.pool.physical(first); const b = o.core.pool.physical(second);
        // 합성 공간 입력에서 표면0단계를 제외하고 두 자산의 실제1단계 체인을 순회한다.
        for (const raw of [a, b]) { put(raw, 14, floatToBits(20)); put(raw, 18, floatToBits(21)); raw[40] = 0; }
        put(a, 4, second, 2); put(b, 4, 0, 2); o.core.hash.setBucket(1, 20, 21, first); const calls = []; o.spellModules.fenceHooks.iterate({ left: 20, top: 21, right: 20, bottom: 21 }, (sid) => { calls.push(sid); put(o.core.pool.physical(sid), 4, 0, 2); }); assert.deepEqual(calls, [first, second]);
    } finally { o.dispose(); }
});

test('Session Spells: 피해 함수는 생성 시 한 번 선택한 실제 부모와 signed/소유자 BYTE를 유지한다', () => {
    const o = owner(); before(o); const original = o.hits.damageHooks; const calls = []; let made = 0; o.hits.damageHooks = () => { made += 1; return { damage: (...args) => calls.push(args) }; };
    try { o.connectBombPostPop(); o.connectAcidZap(); o.connectAcidFence(); o.hits.damageHooks = forbidden; o.spellModules.fenceHooks.damage(50, 0x80000000, 0x103, 51); assert.equal(made, 1); assert.deepEqual(calls, [[50, -2147483648, 3, 51]]); } finally { o.hits.damageHooks = original; o.dispose(); }
});

test('Session Spells: 현재 권한/편집/로컬과 친군사격 사본은 같은 owner 주소를 빌린다', () => {
    const o = scene(); try { const m = o.spellModules; o.gameplayGlobals.authority().boss = false; o.globals.editor = true; o.playerState.battleOwner = 4; assert.equal(m.fenceState.boss, false); assert.equal(m.fenceState.editor, true); assert.equal(m.fenceState.localOwner, 4); o.gameplayGlobals.gameAux().missionSettings()[3] = 0x80000000; o.state.gunTargetState.friendlyFire = 0; assert.equal(m.friendlyFire() >>> 0, 0x80000000); assert.deepEqual(m.fenceTypes, { rainFlyer: 144, sunFlyer: 73, windFlyer: 138, rainBalloon: 146, windBalloon: 141, sunBalloon: 111 }); } finally { o.dispose(); }
});

test('Session Spells: 같은 현재 World 검색/방송·프레임과 미연결 진단을 사용한다', () => {
    const o = scene(); try { const h = o.spellModules.fenceHooks; const calls = []; const saved = [o.world.find, o.tables.broadcast.send, o.core.frame.set]; o.world.find = (...args) => { calls.push(['find', ...args]); return 51; }; o.tables.broadcast.send = (...args) => calls.push(['send', ...args]); o.core.frame.set = (...args) => calls.push(['frame', ...args]); const packet = new Uint8Array(11); assert.equal(h.find(1.5, 2.5, 145), 51); h.broadcast(packet); h.setFrame(50, -1, 7); assert.throws(() => h.assertion(0x123), /진단 미연결/); assert.deepEqual(calls, [['find', 1.5, 2.5, 145], ['send', packet], ['frame', 50, -1, 7]]); [o.world.find, o.tables.broadcast.send, o.core.frame.set] = saved; } finally { o.dispose(); }
});
