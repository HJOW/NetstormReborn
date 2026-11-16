// 독립 typed World 삭제의 실제 래퍼/현재 가상 자격·부모 분배 계약이다(WB04-E614).
// 정적 슬롯 입력만 사용하며 실제 미션/공중3종 생성·행동/교전은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { SQUID_DESTROY_EFFECT as E } from '../www/core/squid-destroy.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put } from './raw-support.mjs';
// 미선택 외부 서비스는 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 보호 자료와 실제 팔레트를 공유한다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 외부 경계만 명시적으로 공급한다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 원문 생성 순서만 진행하며 유닛/생산 프로세스를 만들지 않는다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); o.connectConstruction(); o.connectDamageable(); o.connectIceTower(); o.connectCarrierSteal(); o.connectBombPostPop(); o.connectAcidZap(); o.connectAcidFence(); o.connectCarrierRegular(); o.connectSunAviary(); o.connectWindAviary(); o.connectRainAviary(); o.connectArcSpire(); o.connectFlyer(); o.connectFence(); o.connectGemReward(); }
// 같은 실제 몸체를 조립하며 World/Session은 게시하지 않는다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectWorldDeletion(); return o; }
// 정적 가상 선택 입력을 같은 실제 풀에 쓴다. Pop/생산/게임 행동은 실행하지 않는다.
function image(o, sid, type, vt) { const raw = o.core.pool.physical(sid); raw[10] = type; put(raw, 0, vt, 4); return raw; }

test('Session 자산 삭제: 선행/반복/반납과 훅 값 구성의 Pool·난수 무효과', () => {
    const o = owner(); try { assert.throws(() => o.connectWorldDeletion(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectWorldDeletion(); assert.equal(o.deletionModules.ready, true); assert.equal(o.worldHooks.destroy, o.deletionModules.destroy); assert.equal(typeof o.worldHooks.assets.emit, 'function'); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.throws(() => o.connectWorldDeletion(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); } assert.throws(() => o.connectWorldDeletion(), /반납/); assert.throws(() => o.deletionModules.emit({ sid: 0, flags: 0, effect: E.preDestroy }), /반납/);
});

test('Session 자산 삭제: 실제 피해→내용물→건설과 사제 래퍼를 원문 순서로 합친다', () => {
    const o = owner(); before(o); const calls = []; const saved = [];
    // 실제 세 공장을 대체하지 않고 호출 순서만 기록한다.
    for (const key of ['hits', 'lifetime', 'construction']) { const body = o[key]; const hooks = body.destroyHooks; const destroy = body.destroy; saved.push([body, 'destroyHooks', hooks], [body, 'destroy', destroy]); body.destroyHooks = function (fallback) { calls.push(key + ' hooks'); return hooks.call(this, fallback); }; body.destroy = function (fallback) { calls.push(key + ' destroy'); return destroy.call(this, fallback); }; }
    const priest = o.priests.destroyHooks; saved.push([o.priests, 'destroyHooks', priest]); o.priests.destroyHooks = function (fallback) { calls.push('priest hooks'); return priest.call(this, fallback); };
    try { o.connectWorldDeletion(); assert.deepEqual(calls, ['hits hooks', 'lifetime hooks', 'construction hooks', 'hits destroy', 'lifetime destroy', 'construction destroy', 'priest hooks']); } finally {
        // 기록용 장치를 먼저 원래 실제 메서드로 되돌린다.
        for (const [body, key, method] of saved) body[key] = method; o.dispose();
    }
});

test('Session 삭제 자격: VT 전용과 TYPE/VT 쌍·현재 전체 DWORD 역할을 구별한다', () => {
    const o = scene(); try { const sid = o.core.pool.allocate(2); const k = o.deletionModules.kinds; image(o, sid, 83, 0x509470); assert.equal(k.sunFence(sid), true); image(o, sid, 255, 0x501ea0); assert.equal(k.plainDamageable(sid), true); image(o, sid, 83, 0x50dc30); assert.equal(k.nugget(sid), true); image(o, sid, 158, 0x50f210); assert.equal(k.priest(sid), true); o.core.pool.physical(sid)[10] = 157; assert.equal(k.priest(sid), false); image(o, sid, 82, 0x5034c8); assert.equal(k.bridge(sid), true); o.core.bridgeState.bridgeType = 338; assert.equal(k.bridge(sid), false); image(o, sid, 112, 0x50a748); assert.equal(k.buried(sid), true); o.gemRewardState.buriedType = 368; assert.equal(k.buried(sid), false); o.gemRewardState.buriedType = 112; image(o, sid, 167, 0x507770); assert.equal(k.priestForcefield(sid), true); o.core.pool.physical(sid)[10] = 166; assert.equal(k.priestForcefield(sid), false); image(o, sid, 83, 0x5075d8); assert.equal(k.rune(sid), true); image(o, sid, 90, 0x5092d0); assert.equal(k.fenceCommon(sid), true); image(o, sid, 90, 0x509388); assert.equal(k.fenceCommon(sid), false); image(o, sid, 110, 0x50eb98); assert.equal(k.visualDummy(sid), true); o.core.pool.physical(sid)[10] = 111; assert.equal(k.visualDummy(sid), false); } finally { o.dispose(); }
});

test('Session 삭제 분배: 현재 VT의 피해·펜스·아이스·공통·신전 Pre/Post 부모를 고른다', () => {
    const o = scene(); const common = o.world.commonDeletion; const calls = []; const targets = [[o.damageablePre, 'preDestroy', 'damagePre'], [o.damageableModules, 'preparePost', 'preparePost'], [o.damageablePost, 'postDestroy', 'damagePost'], [o.fenceLifetime, 'preDestroy', 'fencePre'], [o.iceTower, 'growingPostDestroy', 'growingPost'], [o.iceTower, 'postDestroy', 'icePost'], [common, 'preDestroy', 'commonPre'], [common, 'postDestroy', 'commonPost'], [o.vortexPre, 'preDestroy', 'vortexPre']]; const saved = targets.map(([body, key]) => body[key]);
    // 게임 부모의 계산 대신 실제 선택된 메서드와 전달 인수만 관찰한다.
    targets.forEach(([body, key, name]) => { body[key] = (...args) => calls.push([name, ...args]); });
    try { const sid = o.core.pool.allocate(2);
        // 정적 VT 입력마다 같은 현재 Pool에서 Pre와 Post를 따로 선택한다.
        for (const [vt, pre, post] of [[0x501ea0, 'damagePre', 'damagePost'], [0x509470, 'fencePre', 'damagePost'], [0x50dc30, 'damagePre', 'damagePost'], [0x50e888, 'commonPre', 'growingPost'], [0x50ea08, 'damagePre', 'icePost'], [0x5075d8, 'commonPre', 'commonPost'], [0x5196f0, 'vortexPre', 'damagePost']]) { image(o, sid, 83, vt); calls.length = 0; o.worldHooks.assets.emit({ sid, flags: 0x40, effect: E.preDestroy }); o.worldHooks.assets.emit({ sid, flags: 0x40, effect: E.postDestroy }); const expected = [[pre, sid, 0x40]]; if (post === 'damagePost') expected.push(['preparePost']); expected.push([post, sid, 0x40]); assert.deepEqual(calls, expected); }
    } finally { targets.forEach(([body, key], index) => { body[key] = saved[index]; }); o.dispose(); }
});

test('Session 삭제 분배: 다른 효과·기존 함수 값과 실제 삭제 송신을 유지한다', () => {
    const calls = []; const o = scene({ deletion: { emit: (event) => calls.push(['previous', event]) } }); const transmit = o.world.transmitDeletion; try { const sid = o.core.pool.allocate(2); image(o, sid, 83, 0xcafebabe); const event = { sid, flags: 7, effect: E.clearSelection }; o.services.deletion.emit = forbidden; o.worldHooks.assets.emit(event); o.world.transmitDeletion = (...args) => calls.push(['transmit', ...args]); o.worldHooks.assets.emit({ sid, flags: 8, effect: E.transmit }); assert.deepEqual(calls, [['previous', event], ['transmit', sid, 8]]); } finally { o.world.transmitDeletion = transmit; o.dispose(); }
});

test('Session 가상 삭제: 알려진 자격은 같은 실제 분배기에, 나머지는 기존 반환 함수에 보낸다', () => {
    const calls = []; const o = scene({ destroy: (...args) => { calls.push(['previous', ...args]); return 42; } }); const destroy = o.core.virtualDestroy.destroy; try { const sid = o.core.pool.allocate(2); const effects = {}; o.core.virtualDestroy.destroy = (...args) => { calls.push(['virtual', ...args]); return 7; }; image(o, sid, 83, 0x509470); assert.equal(o.worldHooks.destroy(sid, 1, effects), 7); image(o, sid, 83, 0xcafebabe); o.services.destroy = forbidden; assert.equal(o.worldHooks.destroy(sid, 2, effects), 42); assert.deepEqual(calls, [['virtual', sid, 1, effects], ['previous', sid, 2, effects]]); } finally { o.core.virtualDestroy.destroy = destroy; o.dispose(); }
});

test('Session 삭제 분배: 선택된 부모와 기존 fallback의 예외를 우회하지 않는다', () => {
    const stop = new Error('선택된 현재 삭제 부모'); const o = scene({ deletion: { emit: () => { throw stop; } } }); const post = o.iceTower.growingPostDestroy; try { const sid = o.core.pool.allocate(2); image(o, sid, 83, 0xcafebabe); assert.throws(() => o.worldHooks.assets.emit({ sid, flags: 0, effect: E.clearSelection }), (error) => error === stop); image(o, sid, 83, 0x50e888); o.iceTower.growingPostDestroy = () => { throw stop; }; assert.throws(() => o.worldHooks.assets.emit({ sid, flags: 0, effect: E.postDestroy }), (error) => error === stop); } finally { o.iceTower.growingPostDestroy = post; o.dispose(); }
});

test('Session Bomb 삭제 자격: 전용163 그림·일반 genus/VT와 TYPE 물리 범위를 구별한다', () => {
    const o = scene(); try { const sid = o.core.pool.allocate(2); const k = o.deletionModules.kinds; o.core.types[163].flags2 = 0; image(o, sid, 163, 0x509b98); assert.equal(k.bomb(sid), true); o.core.pool.physical(sid)[10] = 162; assert.equal(k.bomb(sid), false); image(o, sid, 174, 0x502370); o.core.types[174].flags2 |= 0x100; assert.equal(k.bomb(sid), true); o.core.types[174].flags2 &= ~0x100; assert.equal(k.bomb(sid), false); image(o, sid, 255, 0x502370); assert.equal(k.bomb(sid), false); } finally { o.dispose(); }
});
