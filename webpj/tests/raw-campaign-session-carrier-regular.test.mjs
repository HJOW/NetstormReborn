// 생성자1659..1664의 같은 Host/난수·현재 공간·미리보기와 task 결선 계약이다(WB04-E608).
// 실제 사제 포획/희생·적 공격·공중3종 전용 생성/행동은 실행하지 않는다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { typeNumber } from '../www/assets/type-table.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawCarrierRegular } from '../www/core/raw-carrier-regular.js';
import { RawCarrierFalling } from '../www/core/raw-carrier-falling.js';
import { RawCarrierFallStart } from '../www/core/raw-carrier-fall-start.js';
import { RawCarrierTask } from '../www/core/raw-carrier-task.js';
import { RawNuggetPostPop } from '../www/core/raw-nugget-post-pop.js';
import { RawNuggetRegular } from '../www/core/raw-nugget-regular.js';
import { ContentPreviewFields, ContentPreviewProcess } from '../www/core/raw-content-preview-process.js';
import { GameRandom } from '../www/core/random.js';
import { RegularProcess } from '../www/core/squid-process.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put, get, floatToBits } from './raw-support.mjs';

// 미선택 외부 서비스를 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 실제 보호 자산과 팔레트다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 외부 장치만 명시 입력으로 공급하는 실제 기반 owner다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 첫 저장 객체 전의 실제 생성자 순서다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); o.connectConstruction(); o.connectDamageable(); o.connectIceTower(); o.connectCarrierSteal(); o.connectBombPostPop(); o.connectAcidZap(); o.connectAcidFence(); }
// 운반 부모까지 순서대로 구성한다. World/Session은 게시하지 않는다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectCarrierRegular(); return o; }

test('Session CarrierRegular: 선행/중복/반납 관문과 실제 같은 몸체·무효과 생성/미공개', () => {
    const o = owner(); try { assert.throws(() => o.connectCarrierRegular(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectCarrierRegular(); assert.ok(o.carrierRegular instanceof RawCarrierRegular); assert.ok(o.nuggetPostPop instanceof RawNuggetPostPop); assert.ok(o.nuggetRegular instanceof RawNuggetRegular); assert.ok(o.carrierFalling instanceof RawCarrierFalling); assert.ok(o.carrierTask instanceof RawCarrierTask); assert.ok(o.carrierFallStart instanceof RawCarrierFallStart); assert.equal(o.carrierRegular.pool, o.core.pool); assert.equal(o.carrierRegularModules.ready, true); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.throws(() => o.connectCarrierRegular(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    assert.throws(() => o.connectCarrierRegular(), /반납/); assert.throws(() => o.carrierRegularModules.hooks.hasPath(50, 46), /반납/);
});

test('Session CarrierRegular: 같은188 A구간·기본 프레임·casttime을 보호 정의와 대조한다', () => {
    const o = scene(); try { const m = o.carrierRegularModules; assert.equal(m.aFrames.length, 188); assert.equal(m.types.length, 188);
        // 기본0 영역과 실제116 정의의 같은 번호를 모두 확인한다.
        for (let type = 0; type < 188; type += 1) assert.equal(m.aFrames[type], o.core.frames[type].run('A').count);
        for (const asset of game.types()) assert.deepEqual(m.types[asset.block + 70], { defaultFrame: asset.definition.defaultFrame, castTime: Math.trunc(typeNumber(asset.definition, 'casttime') ?? 0) });
        assert.deepEqual(m.types[0], { defaultFrame: 0, castTime: 0 });
    } finally { o.dispose(); }
});

test('Session CarrierRegular: casttime 범위 거부는 앞선 너겟 생성을 보존하고 뒤 타이머를 만들지 않는다', () => {
    const o = owner(); const definition = game.types()[0].definition; const property = { name: 'casttime', value: 2147483647.5 }; try { before(o); definition.properties.push(property); assert.throws(() => o.connectCarrierRegular(), /정수 범위/); assert.ok(o.nuggetRegular); assert.ok(o.nuggetPostPop); assert.equal(o.contentPreviewProcess, null); assert.equal(o.carrierRegular, null); assert.equal(o.carrierRegularModules.ready, false); assert.throws(() => o.connectCarrierRegular(), /반복/); } finally { definition.properties.pop(); o.dispose(); }
});

test('Session CarrierRegular: 실제 너겟 PostPop이 같은 난수·Host에 두 예약을 붙이며 중복은 하나만 소비한다', () => {
    const o = scene(); try { const sid = o.world.create(154, 2); const calls = []; const saved = o.carrier.damageablePostPop; o.carrier.damageablePostPop = (...args) => calls.push(args); const rng = new GameRandom(o.random.state); const first = Math.fround(rng.next(65536) / 65536 * Math.fround(0.3)); const second = Math.fround(rng.next(65536) / 65536 * 0.5); o.core.processState.now = 12.25; o.nuggetPostPop.postPop(sid, 1); assert.equal(o.core.processes.findEvent(sid, 1).payload, first); assert.equal(o.core.processes.findEvent(sid, 3).payload, second); assert.equal(o.core.processes.findEvent(sid, 1).time, 12.25); assert.equal(o.random.state, rng.state); assert.deepEqual(calls, [[sid, 1]]); rng.next(65536); o.nuggetPostPop.postPop(sid, 1); assert.equal(o.random.state, rng.state); assert.deepEqual(calls.at(-1), [sid, 1]); o.carrier.damageablePostPop = saved; } finally { o.dispose(); }
});

test('Session CarrierRegular: 너겟 양 끝점이 PostPop과 같은 예약을 쓰고 정수 난수는 상한을 제외한다', () => {
    const o = scene(); try { const sid = o.world.create(154, 2); const m = o.carrierRegularModules; assert.equal(m.nuggetHooks.allocate, m.nuggetEvents.allocate); assert.equal(m.nuggetHooks.construct, m.nuggetEvents.construct); put(o.core.pool.physical(sid), 36, m.aFrames[154] - 1); const rng = new GameRandom(o.random.state); const delay = 2 + rng.next(6); assert.equal(o.nuggetRegular.handle(sid, 1, 1, 0), 0); const task = o.core.processes.findEvent(sid, 0); assert.ok(task); assert.equal(task.payload, delay); assert.equal(o.random.state, rng.state); assert.throws(() => m.nuggetEvents.randomInteger(8, 8), /구간오류/); assert.equal(o.random.state, rng.state); } finally { o.dispose(); }
});

test('Session CarrierRegular: 너겟 예약의 중첩·누락/부착 뒤 시각·NULL 부착을 원문대로 보존한다', () => {
    const o = scene(); try { const sid = o.world.create(154, 2); const h = o.carrierRegularModules.nuggetHooks; assert.throws(() => h.construct(sid, 1, 2), /확보 메모리 없음/); h.allocate(); const pending = o.pendingNuggetRegular; assert.throws(() => h.allocate(), /중복/); const attach = o.core.processes.attachRegular.bind(o.core.processes); o.core.processes.attachRegular = (...args) => { const task = attach(...args); task.setInitialCount(7); o.core.processState.now = 19.75; return task; }; h.construct(sid, 1, 0.3); assert.equal(o.pendingNuggetRegular, null); assert.equal(o.core.processes.findEvent(sid, 1), pending); assert.equal(pending.time, 19.75); assert.equal(pending.count, 0); o.core.processes.attachRegular = () => null; h.allocate(); h.construct(sid, 3, 1); assert.equal(o.pendingNuggetRegular, null); o.core.processes.attachRegular = attach; h.allocate(); } finally { o.dispose(); } assert.equal(o.pendingNuggetRegular, null);
});

test('Session CarrierRegular: 실제 타이머가 같은 필드·현재 시계/타입으로 Host에 붙어 정확한 기한에 부모를 삭제한다', () => {
    const o = scene(); try { const sid = o.world.create(152, 2); const h = o.carrierRegularModules.hooks; assert.throws(() => h.constructTimer(sid, 1), /메모리 미확보/); h.allocateTimer(); const pending = o.pendingContentPreview; assert.ok(pending instanceof ContentPreviewProcess); assert.throws(() => h.allocateTimer(), /중복 소유권/); const fields = pending.fields; fields.bytes[12] = 0xa5; o.core.processState.now = 9.25; o.state.soundState.sharedProcessType = () => 48; h.constructTimer(sid, 0.2); assert.equal(o.pendingContentPreview, null); assert.equal(o.core.processes.fromForm(pending.form), pending); assert.equal(o.core.pool.slot(pending.form)[10], 48); assert.equal(fields.parent(), sid); assert.equal(fields.bytes[12], 0xa5); assert.equal(fields.deadline(), 9.25 + Math.fround(0.2)); const calls = []; const destroy = o.world.destroy; o.world.destroy = (...args) => calls.push(args); o.core.processState.now = fields.deadline() - 1e-6; pending.runFrame(); assert.deepEqual(calls, []); o.core.processState.now = fields.deadline(); pending.runFrame(); assert.deepEqual(calls, [[sid, 0x400000]]); o.world.destroy = destroy; } finally { o.dispose(); }
});

test('Session CarrierRegular: 타이머 실제 필드 불일치/부착 실패와 반납은 확보 수명을 보존한다', () => {
    const o = scene(); try { const sid = o.world.create(152, 2); const m = o.carrierRegularModules; m.hooks.allocateTimer(); const pending = o.pendingContentPreview; assert.throws(() => m.previewHooks.attach(new ContentPreviewFields(), sid, 47, 0x10), /불일치/); assert.equal(o.pendingContentPreview, pending); const attach = o.core.processes.attach; o.core.processes.attach = () => null; m.hooks.constructTimer(sid, 2); assert.equal(o.pendingContentPreview, null); assert.equal(pending.pid, 0); o.core.processes.attach = attach; m.hooks.allocateTimer(); } finally { o.dispose(); } assert.equal(o.pendingContentPreview, null);
});

test('Session CarrierRegular: 현재 표면 지도·TYPE/프레임 코드와 맵 제한을 같은 배열에서 읽는다', () => {
    const o = scene(); try { const sid = o.world.create(82, 2); const raw = o.core.pool.physical(sid); put(raw, 36, 0); const old = o.core.frames[82]; o.core.frames[82] = { codes: [{ flags: 0x20 }] }; o.core.types[82].flags2 |= 4; assert.equal(o.carrierRegularModules.hooks.needsEscape(20, 21), false); o.core.hash.setCell(0, 20, 21, sid); assert.equal(o.carrierRegularModules.hooks.needsEscape(20, 21), true); o.core.frames[82].codes[0].flags = 0; assert.equal(o.carrierRegularModules.hooks.needsEscape(20, 21), false); o.core.hash.setCell(0, 20, 21, 0); o.core.frames[82] = old; o.cursorManaGlobals.mapLimit = 20; assert.equal(o.carrierFalling.validPoint(19, 19), true); assert.equal(o.carrierFalling.validPoint(20, 19), false); } finally { o.dispose(); }
});

test('Session CarrierRegular: 실제 task가 크랩과 예약을 공유하고 Regular/SharedRegular 및 현재 기한을 유지한다', () => {
    const o = scene(); try { const sid = o.world.create(152, 2); const h = o.carrierRegularModules.taskHooks; o.carrierStealModules.hooks.allocateRegular(); const pending = o.pendingCarrierTask; assert.throws(() => h.allocate(), /중복/); assert.equal(o.pendingCarrierTask, pending); const attached = h.construct(sid, 0x5105, false); assert.equal(attached, pending); assert.equal(attached.payload, 5); assert.equal(attached.shared, false); o.core.processState.now = 11.25; o.carrierTask.set(sid, 0x5105, 0.2); assert.equal(attached.time, 11.25 + Math.fround(0.2)); assert.equal(attached.count, 0); assert.throws(() => o.carrierTask.set(sid, 0x5101, 0), /407/); const second = o.world.create(152, 2); o.carrierTask.set(second, 0x5101, 0.3); const task = h.find(second); assert.ok(task.shared); assert.equal(task.payload, 5); assert.equal(task.time, 11.25 + Math.fround(0.3)); assert.equal(o.pendingCarrierTask, null); } finally { o.dispose(); }
});

test('Session CarrierRegular: 실제 Host 경로 조회/제거와 mask 없는 첫 kind 조회를 함께 사용한다', () => {
    const o = scene({ deletion: { selected: () => 0, emit: forbidden } }); try { const parent = o.world.create(152, 2); const task = o.core.processes.addRegular(parent, 0x432, 1); const type = o.core.pool.slot(task.form)[10]; const h = o.worldHooks.carrierRegular; assert.equal(h.hasPath(parent, type), true); o.carrierRegularModules.beginHooks.removeMovement(parent, type); assert.equal(h.hasPath(parent, type), false); assert.equal(o.core.processes.fromForm(task.form), null);
        // 가상 내용물/실제 Form6 두 개를 사용하며 사제 포획이나 실제 이동을 만들지 않는다.
        const first = o.world.create(6, 2); const second = o.world.create(6, 2); const a = o.core.pool.physical(first); const b = o.core.pool.physical(second); put(o.core.pool.physical(parent), 6, first, 2); put(a, 4, second, 2); put(a, 18, 154, 2); put(a, 20, 0, 2); put(b, 18, 154, 2); put(b, 20, 1, 2); assert.equal(o.carrierRegularModules.fallingHooks.findKind(parent, 154), first); assert.equal(o.carrierRegularModules.beginHooks.findKind(parent, 154), first); assert.equal(o.carrierRegularModules.beginHooks.findKind(parent, 0x1009a), 0); put(o.core.pool.physical(parent), 6, 0, 2);
    } finally { o.dispose(); }
});

test('Session CarrierRegular: 현재 UI 약한 토큰만 선택을 해제하며 입력 서비스는 값으로 복사한다', () => {
    const calls = []; const input = { carrier: { escape: (...args) => { calls.push(args); return 99; } } }; const o = scene({ motion: input }); try { input.carrier.escape = forbidden; const h = o.worldHooks.carrierRegular; assert.equal(h.escape(50, 1, 0xffffffff), undefined); assert.deepEqual(calls, [[50, 1, 0xffffffff]]); o.services.motion.carrier.escape = null; assert.throws(() => h.escape(50, 1, 0), /회피429010/); const begin = o.carrierRegularModules.beginHooks; assert.throws(() => begin.clearSelection(), /UI 수명/); const link = { selection: { select: (...args) => { calls.push(args); return 1; } } }; o.activeSelection = new WeakRef(link); assert.equal(begin.clearSelection(), undefined); assert.deepEqual(calls.at(-1), [0, 0, 1]); link.selection = null; assert.throws(() => begin.clearSelection(), /UI 수명/); } finally { o.dispose(); }
});

test('Session CarrierRegular: 하위WORD는 현재 다리에만 통지하고 같은 표시 슬롯·기본 피해 몸체에 위임한다', () => {
    const o = scene(); try { const m = o.carrierRegularModules; const sid = o.world.create(83, 2); const calls = []; const notify = o.core.building.notifySurface; const update = o.core.display.update; const damage = o.assetBindings.regular.damageableHandle; o.core.building.notifySurface = (...args) => calls.push(['notify', ...args]); o.core.display.update = (raw) => { assert.equal(raw.buffer, o.core.pool.physical(sid).buffer); calls.push(['display', raw[12]]); }; o.assetBindings.regular.damageableHandle = (...args) => { calls.push(['damage', ...args]); return 0.125; }; m.hooks.setWord(sid, 0x12345); assert.equal(get(o.core.pool.slot(sid), 12, 2), 0x2345); assert.deepEqual(calls, []); o.core.bridgeState.bridgeType = 83; m.hooks.setWord(sid, 0xabcd); assert.deepEqual(calls, [['notify', sid]]); m.nuggetEvents.refresh(sid); assert.equal(m.hooks.damageable(sid, 8, 9, 0.2), 0.125); assert.deepEqual(calls.slice(1), [['display', 0xcd], ['damage', sid, 8, 9, 0.2]]); o.core.building.notifySurface = notify; o.core.display.update = update; o.assetBindings.regular.damageableHandle = damage; } finally { o.dispose(); }
});

test('Session CarrierRegular: 낙하 checking 사본은 Gun의 지역 대입 뒤에도 같은 전역 owner를 읽는다', () => {
    const o = scene(); try { const sid = o.world.create(152, 2); const checking = o.gameplayGlobals.aiDebug().checking(); checking.value = 0x80000000; o.gameplayGlobals.authority().boss = false; assert.throws(() => o.carrierFalling.releasePriest(sid), /959/); o.state.gunState.checking = 0; assert.throws(() => o.carrierFalling.releasePriest(sid), /959/); checking.value = 0; assert.equal(o.carrierFalling.releasePriest(sid), undefined); } finally { o.dispose(); }
});

test('Session CarrierRegular: 새 task는 부착 효과의 event/count를 보존하고 그 뒤 현재 시각만 설정한다', () => {
    const o = scene(); try { const sid = o.world.create(152, 2); const attach = o.core.processes.attachRegular.bind(o.core.processes); o.core.processes.attachRegular = (...args) => { const task = attach(...args); task.resetTask(0x5104, 0); task.setInitialCount(9); o.core.processState.now = 21.5; return task; }; o.carrierTask.set(sid, 0x5101, 0.2); const task = o.carrierRegularModules.taskHooks.find(sid); assert.equal(task.event, 0x5104); assert.equal(task.count, 9); assert.equal(task.time, 21.5 + Math.fround(0.2)); assert.equal(task.payload, 5); o.core.processes.attachRegular = attach; } finally { o.dispose(); }
});

test('Session CarrierRegular: 실제 만료 부모 삭제가 같은 타이머의 Host/Kernel 수명을 함께 반납한다', () => {
    const o = scene({ deletion: { selected: () => 0, emit: forbidden } }); try { const parent = o.world.create(152, 2); const h = o.carrierRegularModules.hooks; h.allocateTimer(); const timer = o.pendingContentPreview; h.constructTimer(parent, 0.2); const form = timer.form; const pid = timer.pid; const fields = timer.fields; o.core.processState.now = fields.deadline(); timer.runFrame(); assert.equal(o.core.processes.fromForm(form), null); assert.equal(o.core.processes.isRegistered(pid, timer), false); assert.notEqual(o.core.pool.slot(parent)[11] & 1, 0); assert.equal(get(fields.bytes, 0), 0x5019ec); assert.equal(get(fields.bytes, 8), 0xffffcfc7); } finally { o.dispose(); }
});

test('Session Carrier: 내용물 타이머 부모 진단은 원문처럼 Baseprocess.cpp 0xa4 를 표시한 뒤 계속한다(E1211)', () => {
    const o = scene(); try {
        const h = o.carrierRegularModules.previewHooks; const bs = String.fromCharCode(92);
        assert.throws(() => h.assertion(), /진단 장치 미연결/); const shown = []; o.world.storage.bindDisplayDiagnostics({ value: '' }, (...args) => shown.push(args)); h.assertion();
        assert.deepEqual(shown, [['parent->getHead() != 0', bs + 'Ns' + bs + 'O' + bs + 'Baseprocess.cpp', 0xa4]]);
    } finally { o.dispose(); }
});
