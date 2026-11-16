// InstallContents3135..3156의 실제 기술/집계·금지표/Deck/생산·동기화 부모를 대조한다(WB04-E598).
// 실제 보호 자산/모듈과 명시 할당 입력을 쓰며 Session/미션 플레이·실제 포획 관측은 아니다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawPlayerTechnology } from '../www/core/raw-player-technology.js';
import { RawProductionDeck } from '../www/core/raw-production-deck.js';
import { RawCampaignProductionBindings } from '../www/core/raw-campaign-production-bindings.js';
import { RawProductionSync } from '../www/core/raw-production-sync.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put, get } from './raw-support.mjs';

// 선택하지 않은 실제 외부 효과를 성공으로 대체하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 같은 보호 자산·팔레트와 실제 서비스 기반 owner다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 서비스 함수와 할당 입력만 검사별로 공급한다. 세계/생산 객체는 실제 구현이다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 생성자 앞 구간을 내용물 수명까지 원문 순서로 연결한다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); }
// 기술/생산 접두까지 연결한다. 실제 World/Session은 게시하지 않는다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectProduction(); return o; }
// Player 기술 체인의 실제 SID/원문 포인터 입력이다. 객체를 World에 Pop하지 않는다.
function technologyParent(o, owner = 1) { const sid = o.world.create(75, 0); put(o.core.pool.allocated(sid), 0, 0x50ee08); put(o.players, owner * 180 + 0x9c, sid); o.playerState.battleOwner = owner; o.playerState.templateOwner = owner; o.playerState.playerKnown = 1; return sid; }

test('Session Production: 선행/한 번/반납 관문과 실제 Pool·Player·Deck·World 미공개를 유지한다', () => {
    const calls = []; const o = owner({ production: { allocationBytes: () => calls.push('allocate') } });
    try { assert.throws(() => o.connectProduction(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); o.connectProduction(); assert.equal(o.productionModules.ready, true); assert.ok(o.technology instanceof RawPlayerTechnology); assert.ok(o.productionDeck instanceof RawProductionDeck); assert.ok(o.production instanceof RawCampaignProductionBindings); assert.ok(o.productionSync instanceof RawProductionSync); assert.equal(o.production.pool, o.core.pool); assert.equal(o.technology.pool, o.core.pool); assert.equal(o.productionDeck.player, o.player); assert.equal(o.production.deck, o.productionDeck); assert.deepEqual(o.core.pool.bytes(), bytes); assert.deepEqual(calls, []); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); assert.throws(() => o.connectProduction(), /반복/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectProduction(), /반납/); assert.throws(() => o.productionModules.syncHooks.deck(), /반납/);
});

test('Session Production: 기술 이름은 같은 원래 로딩 표를 읽으며 수정 가능한 World 이름과 구별한다', () => {
    const o = scene(); try { const name = o.assets.loadedTypes[94].name; assert.ok(name); assert.equal(o.productionModules.technologyHooks.findType(name.toUpperCase()), 94); o.core.types[94].name = 'changed-current-world-name'; assert.equal(o.productionModules.technologyHooks.findType(name), 94); assert.equal(o.productionModules.technologyHooks.findType('changed-current-world-name'), -1); assert.throws(() => o.technology.find(1, 188), /cpp:3135/); } finally { o.dispose(); }
});

test('Session Production: 실제 기술 Ensure/Find/중복 생략/Clear가 같은 내용물 생성·가상 삭제로 이어진다', () => {
    const selected = []; const o = scene({ deletion: { selected: () => { selected.push(0); return 0; } } }); try { const root = technologyParent(o); o.technology.ensure(1, 94); const form = o.technology.find(1, 94); assert.ok(form > 0); assert.equal(get(o.core.pool.slot(form), 16, 2), root); assert.equal(get(o.core.pool.slot(form), 20, 2), 4); const count = o.core.pool.freeCount; o.technology.ensure(1, 94); assert.equal(o.core.pool.freeCount, count); o.technology.clear(1); assert.equal(o.technology.find(1, 94), 0); assert.equal(o.core.pool.slot(form)[11] & 1, 1); assert.ok(selected.length > 0); } finally { o.dispose(); }
});

test('Session Production: TotalMade는 같은 현재 세 집계와 금지표이며 생산 허용 조회도 공유한다', () => {
    const o = scene(); try { o.core.bookkeeping.localCounts()[94] = 11; o.core.bookkeeping.localSecondaryCounts()[94] = 12; o.core.bookkeeping.globalCounts()[94] = 13; assert.deepEqual([o.totalMade.local(94), o.totalMade.built(94), o.totalMade.global(94)], [11, 12, 13]); o.totalMade.allow(94, 0); assert.equal(o.restrictions[94], 1); assert.equal(o.production.allowed(94), false); o.restrictions[94] = 0; assert.equal(o.production.allowed(94), true); assert.throws(() => o.totalMade.allowed(188), /cpp:3140/); } finally { o.dispose(); }
});

test('Session Production: Deck 할당 누락은 첫 Get에서 실패하고 현재 서비스로 같은 장기 메모리에 할당한다', () => {
    const o = scene(); const records = o.players; const memory = o.playerRecords.productionDeckMemory(); let token;
    try { assert.throws(() => o.productionDeck.get(1), /Deck 할당 입력 미연결/); assert.equal(get(records, 180 + 0x68), 0); const calls = []; o.services.production.allocationBytes = (bytes) => { calls.push(bytes.length); bytes.fill(0xa5); }; token = o.productionDeck.get(1); const bytes = o.productionDeck.bytes(token); assert.deepEqual(calls, [352]); assert.equal(memory.bytes(token), bytes); assert.equal(get(records, 180 + 0x68), token); assert.equal(bytes[0], 0xa5); assert.equal(get(bytes, 0x14c), 0); assert.equal(o.productionDeck.get(1), token); assert.equal(calls.length, 1);
    } finally { o.dispose(); }
    assert.equal(get(records, 180 + 0x68), 0); assert.equal(memory.has(token), false);
});

test('Session Production: 원문 서비스 표/효과는 값 복사하며 Deck 채움 함수는 호출 때 현재 슬롯을 사용한다', () => {
    const calls = []; const effects = { configInt: () => 7 }; const production = { allocationBytes: (bytes) => { calls.push('initial'); bytes.fill(0x11); }, effects }; const o = owner({ production }); production.allocationBytes = forbidden; effects.configInt = forbidden;
    try { before(o); o.connectProduction(); assert.equal(o.services.production.effects.configInt(), 7); o.productionDeck.get(1); o.services.production.allocationBytes = (bytes) => { calls.push('next'); bytes.fill(0x22); }; const token = o.productionDeck.get(2); assert.equal(o.productionDeck.bytes(token)[0], 0x22); assert.deepEqual(calls, ['initial', 'next']); } finally { o.dispose(); }
});

test('Session Production: 생산/동기화 상태는 현재 미션·장기 FullDeck/현재 TYPE·battleOwner를 조회한다', () => {
    const o = scene(); try { const s = o.productionModules.productionState; const sync = o.productionModules.syncState; o.objectState.mission = false; o.fullDeck = 0x12345678; o.freeTechnologyType = 91; o.templeProduct = 132; o.playerState.battleOwner = 7; assert.deepEqual([s.mission, s.fullDeck, s.freeTechnologyType, s.templeProduct, sync.localOwner(), sync.fullDeck()], [false, 0x12345678, 91, 132, 7, 0x12345678]); o.fullDeck = 3; assert.equal(sync.fullDeck(), 3); } finally { o.dispose(); }
});

test('Session Production: 동기화는 실제 신전DWORD+7c와 현재 Player 기술 부모를 읽는다', () => {
    const o = scene(); try { const h = o.productionModules.syncHooks; const root = technologyParent(o, 6); assert.equal(h.technology(), root); put(o.players, 6 * 180 + 0xb0, 0x1234); assert.equal(h.temple(6), false); put(o.players, 6 * 180 + 0x7c, 0x10000); assert.equal(h.temple(6), true); assert.throws(() => h.temple(9), /Player 범위/); o.playerState.playerKnown = 0; assert.equal(h.technology(), 0); } finally { o.dispose(); }
});

test('Session Production: 동기화 FindKind는 같은 실제 Carrier가 찾은 내용물의 WORD를 반환한다', () => {
    const o = scene(); try { const root = technologyParent(o); const h = o.productionModules.syncHooks; assert.equal(h.findKind(root, 8, 94, 0), 0); const form = h.create(root, 94, 5, 8, 0x10, 3); assert.equal(h.findKind(root, 8, 94, 0), 94); assert.notEqual(form, 94); assert.equal(get(o.core.pool.slot(form), 12, 2), 5); assert.equal(o.core.pool.slot(form)[30], 3); } finally { o.dispose(); }
});

test('Session Production: 현재 Deck 항목의 실제 동기화가 생성한 내용을 재조회하며 중복 생성하지 않는다', () => {
    const o = scene({ production: { allocationBytes: (bytes) => bytes.fill(0) } }); try { const root = technologyParent(o); const token = o.productionDeck.get(1); const deck = o.productionDeck.bytes(token); deck.set([94, 1, 1, 0, 0], 0); put(deck, 0x14c, 1); o.productionSync.synchronize(); assert.equal(o.productionModules.syncHooks.findKind(root, 8, 94, 0), 94); const count = o.core.pool.freeCount; o.productionSync.synchronize(); assert.equal(o.core.pool.freeCount, count); assert.equal(o.productionModules.syncHooks.deck(), deck); } finally { o.dispose(); }
});

test('Session Production: 동기화 커서는 현재 후설치 포인터이며 삭제는 같은 World 가상 경계다', () => {
    const selected = []; const o = scene({ deletion: { selected: () => { selected.push(0); return 0; } } }); try { const h = o.productionModules.syncHooks; assert.throws(() => h.cancelCursor(), /실제 커서 미설치/); const calls = []; o.cursor = { cancel: () => calls.push('first') }; h.cancelCursor(); o.cursor = { cancel: () => calls.push('next') }; h.cancelCursor(); const root = technologyParent(o); const form = h.create(root, 94, 0, 8, 0x10, 3); h.destroy(form, 0); assert.equal(o.core.pool.slot(form)[11] & 1, 1); assert.deepEqual(calls, ['first', 'next']); assert.ok(selected.length > 0); } finally { o.dispose(); }
});
