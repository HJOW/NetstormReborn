// ConnectCarrierSteal의 같은 Form6/현재 전역·방송·이동/492 예약 계약이다(WB04-E606).
// 실제 미션 포획/사제 포획·희생·공중3종 관측과 전체 Session 검사는 아니다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawCarrierSteal } from '../www/core/raw-carrier-steal.js';
import { RegularProcess } from '../www/core/squid-process.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { get, put } from './raw-support.mjs';

// 미선택 외부 효과를 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 같은 보호 자산과 실제 팔레트다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 외부 서비스만 명시 입력으로 공급하는 실제 기반 owner다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 첫 저장 객체 전의 Source 생성자 순서다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); o.connectConstruction(); o.connectDamageable(); o.connectIceTower(); }
// 전체 World/Session 게시 전의 실제 크랩 결선이다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectCarrierSteal(); return o; }

test('Session CarrierSteal: 선행/반복/반납과 실제 동일 Pool·무효과 생성/미공개', () => {
    const o = owner(); try { assert.throws(() => o.connectCarrierSteal(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectCarrierSteal(); assert.equal(o.carrierStealModules.ready, true); assert.ok(o.carrierSteal instanceof RawCarrierSteal); assert.equal(o.carrierSteal.pool, o.core.pool); assert.equal(o.pendingCarrierTask, null); assert.equal(o.stealHaltCounter, 0); assert.deepEqual(o.stealHeader, { message: 0x60, protocol: 1, length: 10 }); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.throws(() => o.connectCarrierSteal(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    assert.throws(() => o.connectCarrierSteal(), /반납/); assert.throws(() => o.carrierStealModules.hooks.find(50, 0x20, 0, 0), /반납/);
});

test('Session CarrierSteal: 모든 부모의 정지 DWORD가 하나이며 현재 boss와 실제 공통 정지를 사용한다', () => {
    const o = scene(); try { const calls = []; o.motion = { commonHalt: (sid) => { calls.push(sid); return 12; } }; o.gameplayGlobals.authority().boss = false; o.stealHaltCounter = 0; assert.equal(o.carrierSteal.halt(50), 12); assert.equal(o.carrierSteal.halt(51), 12); assert.equal(o.carrierSteal.halt(52), 12); assert.equal(o.carrierSteal.halt(53), 10); assert.deepEqual(calls, [50, 51, 52]); assert.equal(o.stealHaltCounter, 0); o.carrierStealModules.haltCounter.value = 0xffffffff; assert.equal(o.carrierSteal.halt(54), 10); assert.equal(o.stealHaltCounter, 0); o.motion = null; assert.throws(() => o.carrierSteal.halt(55), /정지/); } finally { o.dispose(); }
});

test('Session CarrierSteal: 실제 공유 예약과float1 관문/효과 후 시각·초기 count0을 보존한다', () => {
    const o = scene(); try { const sid = o.world.create(156, 2); const h = o.carrierStealModules.hooks; assert.throws(() => h.regular(sid, 0x492, 1), /예약 누락/); assert.equal(h.allocateRegular(), true); const pending = o.pendingCarrierTask; assert.ok(pending instanceof RegularProcess); assert.throws(() => h.regular(sid, 0x492, 2), /예약 누락/); assert.equal(o.pendingCarrierTask, pending); const attach = o.core.processes.attachRegular.bind(o.core.processes); o.core.processes.attachRegular = (...args) => { const task = attach(...args); task.setInitialCount(7); o.core.processState.now = 13.5; return task; }; h.regular(sid, 0x492, 1 + 1e-8); assert.equal(o.pendingCarrierTask, null); assert.equal(o.core.processes.findEvent(sid, 0x492), pending); assert.equal(pending.count, 0); assert.equal(pending.payload, 1); assert.equal(pending.time, 13.5); assert.equal(pending.shared, false); } finally { o.dispose(); }
});

test('Session CarrierSteal: 부착 실패와 미부착 예약 교체/반납은 부분 수명을 보존한다', () => {
    const o = scene(); try { const sid = o.world.create(156, 2); const h = o.carrierStealModules.hooks; h.allocateRegular(); const first = o.pendingCarrierTask; h.allocateRegular(); assert.notEqual(o.pendingCarrierTask, first); assert.equal(first.pid, 0); const attach = o.core.processes.attachRegular; o.core.processes.attachRegular = () => null; assert.throws(() => h.regular(sid, 0x492, 1), /부착 실패/); assert.equal(o.pendingCarrierTask, null); o.core.processes.attachRegular = attach; h.allocateRegular(); assert.equal(o.pendingCarrierTask.pid, 0); o.stealHaltCounter = 3; } finally { o.dispose(); } assert.equal(o.pendingCarrierTask, null); assert.equal(o.stealHaltCounter, 3);
});

test('Session CarrierSteal: detach flags는 실제 분리 앞에서 거부하고 현재 Form6/부착에 원문 인자를 보낸다', () => {
    const o = scene(); try { const h = o.carrierStealModules.hooks; const calls = []; const unpop = o.contentForms.unpop; const attach = o.contentForms.attach; o.contentForms.unpop = (...args) => calls.push(['detach', ...args]); o.contentForms.attach = (...args) => calls.push(['attach', ...args]); assert.throws(() => h.detach(50, 1), /flags 오류/); assert.deepEqual(calls, []); h.detach(50, 0); h.attach(50, 51, 0x10); assert.deepEqual(calls, [['detach', 50], ['attach', 50, 51, 0x10]]); o.contentForms.unpop = unpop; o.contentForms.attach = attach; } finally { o.dispose(); }
});

test('Session CarrierSteal: 같은 공통 Form6의 실제 정렬 부착/분리가 현재 부모 WORD를 옮긴다', () => {
    const o = scene(); try { const first = o.world.create(152); const second = o.world.create(152); put(o.core.pool.allocated(first), 0, 0x50ee08); put(o.core.pool.allocated(second), 0, 0x50ee08); const form = o.contained.create(first, 94, 7, 0x20, 0x10, 8); const h = o.carrierStealModules.hooks; assert.equal(get(o.core.pool.slot(form), 16, 2), first); h.detach(form, 0); assert.equal(get(o.core.pool.slot(first), 6, 2), 0); h.attach(form, second, 0x10); assert.equal(get(o.core.pool.slot(form), 16, 2), second); assert.equal(get(o.core.pool.slot(second), 6, 2), form); } finally { o.dispose(); }
});

test('Session CarrierSteal: 미리보기는 현재 로딩/extra/TYPE 관문과 실제491 Regular를 사용한다', () => {
    const o = scene(); try { const sid = o.world.create(156, 2); const h = o.carrierStealModules.hooks; o.objectState.loadingDepth = 1; h.preview(sid); assert.equal(o.core.processes.findEvent(sid, 0x491), null); o.objectState.loadingDepth = 0; const raw = o.core.pool.physical(sid); raw[40] = 9; h.preview(sid); assert.equal(o.core.processes.findEvent(sid, 0x491), null); raw[40] = 0; h.preview(sid); const task = o.core.processes.findEvent(sid, 0x491); assert.ok(task); assert.equal(task.payload, Math.fround(0.2)); const form = o.world.create(6, 2); assert.throws(() => h.preview(form), /Form 진단/); } finally { o.dispose(); }
});

test('Session CarrierSteal: 실제 방송 표의 현재 접속/로컬 관문과 같은 wire를 사용한다', () => {
    const calls = []; const peers = new Array(9).fill(null); const packet = new Uint8Array([1, 0x60, 10, 0, 0, 0, 0, 0, 0, 0]); let o;
    peers[2] = (bytes) => { assert.equal(bytes, packet); calls.push(2); o.playerState.battleOwner = 3; }; peers[3] = () => calls.push(3); peers[4] = (bytes) => { assert.equal(bytes, packet); calls.push(4); }; o = scene({ battlePeers: peers });
    try { o.playerState.battleOwner = 1; o.players[2 * 180 + 0x50] |= 1; o.players[3 * 180 + 0x50] |= 1; o.players[4 * 180 + 0x50] |= 1; o.carrierStealModules.hooks.broadcast(packet); assert.deepEqual(calls, [2, 4]); } finally { o.dispose(); }
});

test('Session CarrierSteal: 내용물/이동 종류/운반 검사와 피해 PostPop은 현재 같은 부모에 연결한다', () => {
    const o = scene(); try { const h = o.carrierStealModules.hooks; const calls = []; const saved = [o.priests.carrier.findContained, o.priests.movementProcessType, o.priests.carrierCheck.check, o.carrier.damageablePostPop]; o.priests.carrier.findContained = (...args) => { calls.push(['find', ...args]); return 52; }; o.priests.movementProcessType = (type) => { calls.push(['type', type]); return 40; }; o.priests.carrierCheck.check = (...args) => calls.push(['check', ...args]); o.carrier.damageablePostPop = (...args) => calls.push(['post', ...args]); assert.equal(h.find(50, 0x20, 154, 2), 52); assert.equal(h.movementType(133), 40); h.check(51); h.damageablePostPop(52, 7); assert.deepEqual(calls, [['find', 50, 0x20, 154, 2], ['type', 133], ['check', 51], ['post', 52, 7]]); [o.priests.carrier.findContained, o.priests.movementProcessType, o.priests.carrierCheck.check, o.carrier.damageablePostPop] = saved; } finally { o.dispose(); }
});

test('Session CarrierSteal: 이동 제거는 실제 종속 조회/가상 삭제로 같은 Host의 예약을 반납한다', () => {
    let selectionReads = 0; const o = scene({ deletion: { selected: () => { selectionReads += 1; return 0; }, emit: forbidden } });
    try { const parent = o.world.create(152); const task = o.core.processes.addRegular(parent, 0x432, 1); assert.ok(task); const form = task.form; assert.equal(o.core.processes.fromForm(form), task); o.carrierStealModules.hooks.removeMovement(parent, 46); assert.equal(o.core.processes.fromForm(form), null); assert.equal(get(o.core.pool.slot(parent), 6, 2), 0); assert.notEqual(o.core.pool.slot(form)[11] & 1, 0); assert.ok(selectionReads > 0); } finally { o.dispose(); }
});

test('Session CarrierSteal: 전역 음향과 현재 귀환/정지는 실제6인자·미설치 오류·실패를 보존한다', () => {
    const o = scene(); try { const h = o.carrierStealModules.hooks; const calls = []; o.services.sound.play = (...args) => calls.push(['sound', ...args]); h.sound('crabSteal.wav', 0, 0, 0, 1, 0); assert.deepEqual(calls, [['sound', o.soundNames.lookup('crabSteal.wav'), 0, 0, 0, 1, 0]]); assert.throws(() => h.goHome(50), /귀환/); o.carrierTarget = { goHome: (sid) => calls.push(['home', sid]) }; h.goHome(51); assert.deepEqual(calls.at(-1), ['home', 51]); o.motion = { commonHalt: () => 10 }; assert.equal(h.commonHalt(52), 10); o.services.sound.play = null; assert.throws(() => h.sound('crabSteal.wav', 0, 0, 0, 1, 0), /전역 소리 미연결/); } finally { o.dispose(); }
});
