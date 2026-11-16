// GemReward/Buried의 실제 Deck 장치·현재 소유 상태·공통 Host 결선이다(WB04-E613).
// 실제 미션/교전·공중3종 새 전용 실행은 하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawGemReward } from '../www/core/raw-gem-reward.js';
import { RawBuriedPostPop } from '../www/core/raw-buried-post-pop.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
// 미선택 외부 서비스를 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 보호 자료와 실제 팔레트를 빌린다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 외부 장치만 명시적으로 공급한다. 부분 sound 입력도 나머지 실제 계약을 보존한다.
function owner(extra = {}) { const sound = { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden, ...extra.sound }; return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), ...extra, sound }); }
// 원문 생성 순서만 진행한다. 공중3종은 생성/생산/행동을 실행하지 않는다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); o.connectConstruction(); o.connectDamageable(); o.connectIceTower(); o.connectCarrierSteal(); o.connectBombPostPop(); o.connectAcidZap(); o.connectAcidFence(); o.connectCarrierRegular(); o.connectSunAviary(); o.connectWindAviary(); o.connectRainAviary(); o.connectArcSpire(); o.connectFlyer(); o.connectFence(); }
// 같은 실제 몸체를 조립하며 World/Session은 게시하지 않는다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectGemReward(); return o; }
// 보상 후보가 하나뿐인 명시 TYPE 입력이다. 추첨/제외 계산은 기존 실제 몸체가 수행한다.
function singleCandidate(o) { o.gemRewardState.typeCount = 79; for (let type = 0; type < 79; type += 1) o.core.types[type].flags2 &= ~0x100; o.core.types[78].flags2 |= 0x100; o.core.types[78].level = 0; }

test('Session GemReward: 같은 두 몸체/지역 상태와 선행·반복·반납/Pool·난수 무효과', () => {
    const o = owner(); try { assert.throws(() => o.connectGemReward(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectGemReward(); assert.equal(o.gemModules.ready, true); assert.ok(o.gemReward instanceof RawGemReward); assert.ok(o.buriedPostPop instanceof RawBuriedPostPop); assert.equal(o.gemReward.pool, o.core.pool); assert.equal(o.buriedPostPop.pool, o.core.pool); assert.equal(o.gemRewardState.typeCount, 188); assert.equal(o.gemRewardState.buriedType, 112); assert.equal(o.constructionExcludedTypes.readCount(), 0); assert.equal(o.constructionExcludedTypes.capacity(), 0); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.throws(() => o.connectGemReward(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); } assert.throws(() => o.connectGemReward(), /반납/); assert.throws(() => o.gemModules.buriedHooks.reserveRegular(), /반납/);
});

test('Session GemReward: 할당/벽시각은 함수 값 사본이고 mismatch는 현재 로그 장치다', () => {
    const calls = []; const o = scene({ production: { allocationBytes: (raw) => { calls.push(['fill', raw.length]); raw.fill(0x5a); } }, sound: { wallSeconds: () => 123 } }); try { const h = o.gemModules.rewardHooks; o.services.production.allocationBytes = forbidden; o.services.sound.wallSeconds = forbidden; const raw = new Uint8Array(4); h.fill(raw); assert.deepEqual([...raw], [90, 90, 90, 90]); assert.equal(h.deck.wallSeconds(), 123); o.services.player.log = (...args) => calls.push(['log', ...args]); h.deck.mismatch(0xfffffffe, 0xffffffff); assert.deepEqual(calls, [['fill', 4], ['log', 0x1d, 'Deck chance mismatch. -2!=-1\n']]); assert.throws(() => h.assertion(0xf6), /보상 진단 246/); assert.throws(() => h.deckAssertion('Capacity'), /실제 진단 미연결/); } finally { o.dispose(); }
});

test('Session GemReward: 실제 추첨이 같은 현재 TYPE/제외 목록과 게임 난수를 사용한다', () => {
    const o = scene({ production: { allocationBytes: (raw) => raw.fill(0) }, sound: { wallSeconds: () => 123 } }); try { singleCandidate(o); const seed = o.random.state; assert.equal(o.gemReward.choose(5), 78); assert.notEqual(o.random.state, seed); o.constructionExcludedTypes.entries = [78]; o.constructionExcludedTypes.count = 1; const after = o.random.state; assert.throws(() => o.gemReward.choose(5), /보상 진단 260/); assert.equal(o.random.state, after); o.constructionExcludedTypes.count = 0; assert.equal(o.gemReward.choose(5), 78); } finally { o.dispose(); }
});

test('Session Buried: 현재 boss/로딩/Bridge 참조와 후설치 보상 선택을 읽는다', () => {
    const o = scene(); const choose = o.gemReward.choose; try { const m = o.gemModules; o.gameplayGlobals.authority().boss = false; o.objectState.loadingDepth = 2; o.core.bridgeState.bridgeType = 83; assert.equal(Boolean(m.boss()), false); assert.equal(m.loading(), 2); assert.equal(m.bridgeType(), 83); o.gameplayGlobals.authority().boss = true; assert.equal(Boolean(m.boss()), true); const calls = []; o.gemReward.choose = (value) => { calls.push(value); return 78; }; assert.equal(m.buriedHooks.chooseType(5), 78); assert.deepEqual(calls, [5]); } finally { o.gemReward.choose = choose; o.dispose(); }
});

test('Session Buried: 실제 Host 두 사건과 현재 시각/지연·실패 전파를 유지한다', () => {
    const o = scene(); const add = o.core.processes.addRegular; try { const sid = o.world.create(112, 2); assert.equal(o.buriedPostPop.handles(sid), true); const h = o.gemModules.buriedHooks; o.core.processState.now = 8.25; assert.equal(h.reserveRegular(), true); h.addRegular(sid, 0, 0); h.addRegular(sid, 1, Math.fround(0.01)); const first = o.core.processes.findEvent(sid, 0); const second = o.core.processes.findEvent(sid, 1); assert.ok(first); assert.ok(second); assert.notEqual(first, second); assert.equal(first.count, 0); assert.equal(first.time, 8.25); assert.equal(first.payload, 0); assert.equal(second.payload, Math.fround(0.01)); o.core.processes.addRegular = () => null; assert.throws(() => h.addRegular(sid, 2, 0), /실제 Regular 부착 실패/); } finally { o.core.processes.addRegular = add; o.dispose(); }
});

test('Session Gem/Buried: 실제 가상 생성/공간/삭제와 공통 장부·표면 부모를 구별한다', () => {
    const o = scene(); const saved = [o.world.create, o.world.pop, o.world.destroy, o.core.postPop.validateBase, o.core.postPop.postPopBase, o.core.building.notifySurface]; try { const calls = []; o.world.create = (...args) => { calls.push(['create', ...args]); return 50; }; o.world.pop = (...args) => calls.push(['pop', ...args]); o.world.destroy = (...args) => calls.push(['destroy', ...args]); o.core.postPop.validateBase = (...args) => calls.push(['validate', ...args]); o.core.postPop.postPopBase = (...args) => calls.push(['base', ...args]); o.core.building.notifySurface = (...args) => calls.push(['surface', ...args]); const h = o.gemModules.rewardHooks; const b = o.gemModules.buriedHooks; const projected = {}; assert.equal(h.create(78, 0), 50); h.pop(50, 1.5, 2.5, 0); h.destroy(51, 0); b.validateBase(50, 1, projected); b.base(50, 1); b.notifySurface(50); assert.deepEqual(calls, [['create', 78, 0], ['pop', 50, 1.5, 2.5, 0], ['destroy', 51, 0], ['validate', 50, 1, projected], ['base', 50, 1], ['surface', 50]]); } finally { [o.world.create, o.world.pop, o.world.destroy, o.core.postPop.validateBase, o.core.postPop.postPopBase, o.core.building.notifySurface] = saved; o.dispose(); }
});

test('Session GemReward: 선택된 빈 할당/시계는 나중 장치로 우회하지 않고 앞선 난수 효과를 보존한다', () => {
    const o = scene(); try { singleCandidate(o); const seed = o.random.state; assert.throws(() => o.gemReward.choose(), /미연결/); o.services.production.allocationBytes = (raw) => raw.fill(0); assert.throws(() => o.gemReward.choose(), /미연결/); assert.equal(o.random.state, seed); } finally { o.dispose(); }
    const missingClock = owner({ production: { allocationBytes: (raw) => raw.fill(0) } }); before(missingClock); missingClock.services.sound.wallSeconds = undefined; missingClock.connectGemReward(); try { singleCandidate(missingClock); const seed = missingClock.random.state; assert.throws(() => missingClock.gemReward.choose(), /미연결/); assert.notEqual(missingClock.random.state, seed); missingClock.services.sound.wallSeconds = () => 123; assert.throws(() => missingClock.gemReward.choose(), /미연결/); } finally { missingClock.dispose(); }
});
