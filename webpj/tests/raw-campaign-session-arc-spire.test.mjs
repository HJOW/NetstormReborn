// ArcSpire의 실제 현재 참조/효과 표 계약이다(WB04-E610). 실제 교전·미션/Kernel 공격 검사는 하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { typeNumber } from '../www/assets/type-table.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawArcSpire } from '../www/core/raw-arc-spire.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put, floatToBits } from './raw-support.mjs';

// 선택되지 않은 외부 서비스를 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 기존 보호 자산을 읽어 같은 TYPE/프레임을 구성한다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 실제 기반 owner에 외부 장치만 명시 입력으로 공급한다.
function owner() { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden } }); }
// 첫 저장 객체 전에 원문 생성 순서만 진행한다. 공중3종은 생성/생산/행동을 실행하지 않는다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); o.connectConstruction(); o.connectDamageable(); o.connectIceTower(); o.connectCarrierSteal(); o.connectBombPostPop(); o.connectAcidZap(); o.connectAcidFence(); o.connectCarrierRegular(); o.connectSunAviary(); o.connectWindAviary(); o.connectRainAviary(); }
// 실제 현재 효과를 조립하되 World/Session을 게시하지 않는다.
function scene() { const o = owner(); before(o); o.connectArcSpire(); return o; }

test('Session ArcSpire: 선행/반복/반납과 같은 몸체·생성 중 Pool/난수 무효과', () => {
    const o = owner(); try { assert.throws(() => o.connectArcSpire(), /초기화 전/); before(o); const bytes = o.core.pool.bytes().slice(); const seed = o.random.state; o.connectArcSpire(); assert.ok(o.arcSpire instanceof RawArcSpire); assert.equal(o.arcSpireModules.ready, true); assert.throws(() => o.connectArcSpire(), /반복/); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    assert.throws(() => o.connectArcSpire(), /반납/); assert.throws(() => o.arcSpireModules.hooks.find(1, 2, 149), /반납/);
});

test('Session ArcSpire: range는 자산 정의이며 피해율과 편집은 같은 현재 참조다', () => {
    const o = scene(); try { const sid = o.world.create(149, 2); const h = o.arcSpireModules.hooks; assert.equal(h.range(sid), Math.fround(typeNumber(game.types()[79].definition, 'range'))); o.core.types[149].range = 9999; assert.notEqual(h.range(sid), 9999); o.hitTypes[149].damagePerSecond = 12.75; assert.equal(h.damageRate(sid), 12.75); o.globals.editor = 2; assert.equal(o.arcSpireModules.state.editor, 2); o.globals.editor = 0; assert.equal(o.arcSpireModules.state.editor, 0); const raw = o.core.pool.physical(sid); raw[10] = 1; assert.throws(() => h.range(sid), /TYPE 범위/); raw[10] = 149; } finally { o.dispose(); }
});

test('Session ArcSpire: flags1 실제 Hash 순서와 소비 전 next 저장을 유지한다', () => {
    const o = scene(); try { const first = o.world.create(83, 2); const second = o.world.create(83, 2); const a = o.core.pool.physical(first); const b = o.core.pool.physical(second);
        // 합성 공간 입력만 준비한다. 현재 finder 자체는 실제 몸체다.
        for (const raw of [a, b]) { put(raw, 14, floatToBits(20)); put(raw, 18, floatToBits(21)); raw[40] = 0; }
        put(a, 4, second, 2); put(b, 4, 0, 2); o.core.hash.setBucket(1, 20, 21, first); const seen = []; o.arcSpireModules.hooks.iterate({ left: 20, top: 21, right: 20, bottom: 21 }, (sid) => { seen.push(sid); put(o.core.pool.physical(sid), 4, 0, 2); }); assert.deepEqual(seen, [first, second]);
    } finally { o.dispose(); }
});

test('Session ArcSpire: World 검색·빔의 이름과 프레임 효과를 현재 같은 몸체에 전달한다', () => {
    const o = scene(); const saved = [o.world.find, o.particles.addBeam, o.core.frame.set]; try { const h = o.arcSpireModules.hooks; const calls = []; o.world.find = (...args) => { calls.push(['find', ...args]); return 51; }; o.particles.addBeam = (...args) => { calls.push(['beam', ...args]); return 99; }; o.core.frame.set = (...args) => calls.push(['frame', ...args]); assert.equal(h.find(1.5, 2.5, 149), 51); assert.equal(h.beam(50, 51), undefined); h.setFrame(50, -1, 7); assert.deepEqual(calls, [['find', 1.5, 2.5, 149], ['beam', 50, 51, 'shortZap.wav'], ['frame', 50, -1, 7]]); } finally { [o.world.find, o.particles.addBeam, o.core.frame.set] = saved; o.dispose(); }
});

test('Session ArcSpire: 피해 함수 값은 결선 시 한 번 선택하며 정수 폭을 보존한다', () => {
    const o = owner(); before(o); const original = o.hits.damageHooks; const calls = []; let reads = 0; o.hits.damageHooks = () => { reads += 1; return { damage: (...args) => calls.push(args) }; };
    try { o.connectArcSpire(); o.hits.damageHooks = forbidden; o.arcSpireModules.hooks.damage(50, 0x80000000, 0x103, 51); assert.equal(reads, 1); assert.deepEqual(calls, [[50, -2147483648, 3, 51]]); } finally { o.hits.damageHooks = original; o.dispose(); }
});

test('Session ArcSpire: 애니메이션은 같은 현재 프레임 표와 공통 Regular 반환을 사용한다', () => {
    const o = scene(); const regular = o.assetBindings.regular; const saved = [o.core.frame.set, regular.damageableHandle]; try { const sid = o.world.create(149, 2); const calls = []; o.core.frame.set = (...args) => calls.push(['frame', ...args]); regular.damageableHandle = (...args) => { calls.push(['regular', ...args]); return 0.2; }; const total = o.core.frames[149].length; assert.ok(total > 1); assert.equal(o.arcSpire.handle(sid, 1, 0, 0.25), Math.fround(0.2)); assert.deepEqual(calls, [['frame', sid, total - 1, 0], ['regular', sid, 1, 1, 0.25]]); calls.length = 0; assert.equal(o.arcSpire.handle(sid, 1, total, 0.25), 0); assert.deepEqual(calls, []); } finally { [o.core.frame.set, regular.damageableHandle] = saved; o.dispose(); }
});

test('Session ArcSpire: 선택된 실제 하위 오류는 앞선 효과 뒤 그대로 전파한다', () => {
    const o = scene(); const beam = o.particles.addBeam; try { const stop = new Error('현재 빔 오류'); o.particles.addBeam = () => { throw stop; }; assert.throws(() => o.arcSpireModules.hooks.beam(50, 51), (error) => error === stop); o.arcSpireModules.dispose(); assert.throws(() => o.arcSpireModules.hooks.iterate({ left: 0, top: 0, right: 0, bottom: 0 }, forbidden), /반납/); } finally { o.particles.addBeam = beam; o.dispose(); }
});
