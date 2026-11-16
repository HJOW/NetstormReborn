// Session의 독립 typed 전체 Regular 현재 부모/우선순서/실패 로그 계약이다(WB04-E615).
// 실제 미션/적 공격·공중3종 새 전용 생성/행동은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawBuriedRegular } from '../www/core/raw-buried-regular.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put } from './raw-support.mjs';
// 미선택 외부 서비스를 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 보호 자료와 실제 팔레트다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 외부 장치만 명시적으로 공급한다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 첫 저장 객체 전에 원문 순서로 실제 모듈만 조립한다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); o.connectConstruction(); o.connectDamageable(); o.connectIceTower(); o.connectCarrierSteal(); o.connectBombPostPop(); o.connectAcidZap(); o.connectAcidFence(); o.connectCarrierRegular(); o.connectSunAviary(); o.connectWindAviary(); o.connectRainAviary(); o.connectArcSpire(); o.connectFlyer(); o.connectFence(); o.connectGemReward(); o.connectWorldDeletion(); }
// World/Session을 게시하지 않은 같은 실제 조립이다.
function scene() { const o = owner(); before(o); o.connectWorldRegular(); return o; }
// 가상 선택의 정적 입력만 쓴다. 객체 Pop/게임 행동은 실행하지 않는다.
function image(o, sid, type, vt) { const raw = o.core.pool.physical(sid); raw[10] = type; put(raw, 0, vt, 4); return raw; }

test('Session 전체 Regular: 같은 보상 몸체·선행/반복/반납과 Pool·난수 무효과', () => {
    const o = owner(); try { assert.throws(() => o.connectWorldRegular(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectWorldRegular(); assert.ok(o.buriedRegular instanceof RawBuriedRegular); assert.equal(o.regularModules.ready, true); assert.equal(o.worldHooks.regular, o.regularModules.regular); assert.equal(o.worldHooks.playerRegular, o.regularModules.playerHooks); assert.equal(o.worldHooks.priestRegular, o.regularModules.priestHooks); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.throws(() => o.connectWorldRegular(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); } assert.throws(() => o.connectWorldRegular(), /반납/); assert.throws(() => o.regularModules.regular(0, 0, 0, 0), /반납/);
});

test('Session Regular 조립: 실제 사제→보상→자산→전투→피해와 플레어/펜스 공장 순서', () => {
    const o = owner(); before(o); const calls = []; const saved = []; const targets = [[o.priests, 'regularHandler', 'priest'], [RawBuriedRegular.prototype, 'handler', 'buried'], [o.assetBindings, 'handler', 'asset'], [o.combat, 'handler', 'combat'], [o.hits, 'handler', 'hits'], [o.flare, 'handler', 'flare'], [o.fenceRegular, 'handler', 'fence']];
    // 실제 공장 호출을 유지하며 중첩 생성 순서만 관찰한다.
    for (const [body, key, name] of targets) { const original = body[key]; saved.push([body, key, original]); body[key] = function (fallback) { calls.push(name); return original.call(this, fallback); }; }
    try { o.connectWorldRegular(); assert.deepEqual(calls, ['priest', 'buried', 'asset', 'combat', 'hits', 'flare', 'fence']); } finally {
        // 기록용 장치를 원래 메서드로 먼저 돌려놓는다.
        for (const [body, key, original] of saved) body[key] = original; o.dispose();
    }
});

test('Session Regular 훅: 같은 현재 Template getter/Notify와 사제 고유 사건의 선택 경계', () => {
    const o = scene(); const template = o.templateState; const saved = [template.get, template.changed, o.priests.uniqueRegular]; try { const calls = []; template.get = (owner) => { calls.push(['get', owner]); return 7; }; template.changed = (owner) => calls.push(['changed', owner]); o.priests.uniqueRegular = (...args) => { calls.push(['priest', ...args]); return 0.5; }; const m = o.regularModules; assert.equal(m.playerHooks.state(1), 7); m.playerHooks.changed(8); assert.equal(m.priestHooks.unique(50, 602, 3, 0.25), 0.5); assert.throws(() => m.priestHooks.unique(50, 604, 0, 0), /사제604/); assert.throws(() => m.priestHooks.unique(50, 605, 0, 0), /사제605/); assert.throws(() => m.priestHooks.unique(50, 606, 0, 0), /다른 사제/); assert.deepEqual(calls, [['get', 1], ['changed', 8], ['priest', 50, 602, 3, 0.25]]); o.templateState = null; assert.throws(() => m.playerHooks.state(1), /State getter/); assert.throws(() => m.playerHooks.changed(1), /Notify/); } finally { o.templateState = template; [template.get, template.changed, o.priests.uniqueRegular] = saved; o.dispose(); }
});

test('Session Regular 안쪽 분배: ContentForm signed 횟수와 Bulwark 공통 피해 반환', () => {
    const o = scene(); const regular = o.assetBindings.regular; const saved = [o.contentForms.damage, regular.damageableHandle]; try { const calls = []; o.contentForms.damage = (...args) => { calls.push(['content', ...args]); return 0.25; }; regular.damageableHandle = (...args) => { calls.push(['damageable', ...args]); return 0.75; }; const sid = o.core.pool.allocate(2); image(o, sid, 83, 0x512208); assert.equal(o.regularModules.inner(sid, 9, 0xffffffff, 0.2), 0.25); image(o, sid, 83, 0x50ea08); assert.equal(o.regularModules.inner(sid, 7, 3, 0.4), 0.75); assert.deepEqual(calls, [['content', sid, -1, 0.2], ['damageable', sid, 7, 3, 0.4]]); } finally { [o.contentForms.damage, regular.damageableHandle] = saved; o.dispose(); }
});

test('Session Regular 장식: 실제 감소 계산·현재 가상98과 boss 종료 관문을 사용한다', () => {
    const o = scene(); const regular = o.assetBindings.regular; const saved = [o.hp.maxHitPoints, o.decorationHit, regular.damageableHandle, o.world.destroy]; try { const calls = []; const sid = o.core.pool.allocate(2); const raw = image(o, sid, 83, 0x50a820); put(raw, 26, 100, 4); o.hp.maxHitPoints = () => { o.decorationHit = (...args) => calls.push(['hit', ...args]); return 90; }; regular.damageableHandle = (...args) => { calls.push(['regular', ...args]); return 0.75; }; o.world.destroy = (...args) => calls.push(['destroy', ...args]); assert.equal(o.worldHooks.regular(sid, 0, 1, 0.25), 0.75); assert.deepEqual(calls, [['hit', sid, 40, 0, 0], ['regular', sid, 0, 1, 0.25]]); calls.length = 0; o.gameplayGlobals.authority().boss = false; assert.equal(o.worldHooks.regular(sid, 0, 3, 0.25), -1); assert.deepEqual(calls, []); o.gameplayGlobals.authority().boss = true; assert.equal(o.worldHooks.regular(sid, 0, 3, 0.25), -1); assert.deepEqual(calls, [['destroy', sid, 0]]); } finally { [o.hp.maxHitPoints, o.decorationHit, regular.damageableHandle, o.world.destroy] = saved; o.dispose(); }
});

test('Session Regular 다리: 같은 현재 이벤트 객체와 캡처한 전체 처리기 함수 값을 유지한다', () => {
    const o = scene(); const handle = o.core.bridgeEvents.handle; try { const calls = []; const sid = o.core.pool.allocate(2); image(o, sid, 82, 0x5034c8); o.core.bridgeEvents.handle = (...args) => { calls.push(args); return 0.5; }; o.regularModules.handler = forbidden; assert.equal(o.worldHooks.regular(sid, 0x2691, 2, 0.25), 0.5); assert.deepEqual(calls, [[sid, 0x2691, 2, 0.25]]); } finally { o.core.bridgeEvents.handle = handle; o.dispose(); }
});

test('Session Regular 실패: 현재 부모/사건/횟수/float 비트를 기록하고 로그 오류도 원래 예외를 보존한다', () => {
    const o = scene(); const log = console.error; const handle = o.dudeRegular.handle; try { const logs = []; console.error = (text) => logs.push(text); const sid = o.core.pool.allocate(2); image(o, sid, 83, 0xcafebabe); assert.throws(() => o.worldHooks.regular(sid, 0x123, 0x80000001, 1.25), /실제 Regular 부모 미연결/); assert.deepEqual(logs, ['실제 Regular 실패 SID' + sid + ' TYPE83 event291 count2147483649 payload3fa00000']); image(o, sid, 70, 0x508cc8); assert.equal(o.dudeRegular.handles(sid), true); const stop = new Error('원래 Dude 정기 오류'); o.dudeRegular.handle = () => { throw stop; }; console.error = () => { throw new Error('로그 장치 오류'); }; assert.throws(() => o.worldHooks.regular(sid, 1, 2, 0.25), (error) => error === stop); } finally { console.error = log; o.dudeRegular.handle = handle; o.dispose(); }
});
