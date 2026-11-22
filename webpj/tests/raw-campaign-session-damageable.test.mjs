// ConnectDamageable의 같은 World/목록/현재 삭제·효과 장치 계약이다(WB04-E604).
// 실제 미션·사제 포획/희생/특정 적 공격과 공중3종의 새 전용 관측은 하지 않는다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawDudeDrop } from '../www/core/raw-dude-drop.js';
import { RawDamageablePreDestroy, DAMAGEABLE_SOUND } from '../www/core/raw-damageable-pre-destroy.js';
import { RawDamageablePostDestroy } from '../www/core/raw-damageable-post-destroy.js';
import { ShrapnelKind } from '../www/core/raw-shrapnel-frame.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put, floatToBits } from './raw-support.mjs';

// 미선택 외부 서비스를 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 실제 보호 자산/팔레트다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 외부 서비스만 명시 입력으로 주고 실제 하위 몸체를 사용한다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 첫 객체 전의 원문 생성자 순서다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); o.connectConstruction(); }
// 실제 typed Damageable까지 구성하며 World/Session은 게시하지 않는다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectDamageable(); return o; }

test('Session Damageable: 선행/반복/반납 관문과 같은 Pool/무효과 생성/미공개', () => {
    const o = owner(); try { assert.throws(() => o.connectDamageable(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectDamageable(); assert.equal(o.damageableModules.ready, true); assert.ok(o.dudeDrop instanceof RawDudeDrop); assert.ok(o.damageablePre instanceof RawDamageablePreDestroy); assert.ok(o.damageablePost instanceof RawDamageablePostDestroy); assert.equal(o.damageablePost.pool, o.core.pool); assert.equal(o.dudeSpawn.pool, o.core.pool); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.throws(() => o.connectDamageable(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    assert.throws(() => o.connectDamageable(), /반납/); assert.throws(() => o.damageableModules.preHooks.collapse(50), /반납/);
});

test('Session Damageable: Dude의 population/현재 공급 목록/순환 DWORD는 같은 실제 장부를 빌린다', () => {
    const o = scene(); try { const m = o.damageableModules; assert.equal(m.spawnState.population, o.dudePopulation); assert.equal(o.dudeBehavior.population, o.dudePopulation); assert.equal(m.regularState.providers, o.core.bookkeeping.currentProviders()); assert.equal(m.regularState.providers, o.world.storage.sidLists.providers); o.dudeProviderIndex = 7; assert.equal(m.regularState.providerIndex, 7); m.regularState.providerIndex = 0xffffffff; assert.equal(o.dudeProviderIndex, -1); o.gameplayGlobals.aiDebug().checking().value = 0x80000000; assert.equal(m.behaviorState.checking() >>> 0, 0x80000000); assert.equal(m.regularState.checking() >>> 0, 0x80000000); } finally { o.dispose(); }
});

test('Session Damageable: 실제 Dude 생성 차단은 같은 목록/상태를 읽고 Pool/난수를 소비하지 않는다', () => {
    const o = scene(); try { const bytes = o.core.pool.bytes(); const seed = o.random.state; o.damageableModules.spawnState.blocked = true; assert.equal(o.dudeSpawn.spawn(NaN, NaN, 2), 0); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); } finally { o.dispose(); }
});

test('Session Damageable: 파괴 후 준비는 현재 전투 모드/signed population만 공급한다', () => {
    const o = scene(); try { o.core.ownerMode.battle = false; o.dudePopulation.count = 0xffffffff; o.damageableModules.preparePost(); assert.equal(o.state.damageablePostState.battle, false); assert.equal(o.state.damageablePostState.dudeCount, -1); o.dudePopulation.count = 0; o.core.ownerMode.battle = true; o.damageableModules.preparePost(); assert.equal(o.state.damageablePostState.battle, true); assert.equal(o.state.damageablePostState.dudeCount, 0); } finally { o.dispose(); }
});

test('Session Damageable: 붕괴/폭발은 같은 효과 표와 억제/현재 좌표/발자국 순서를 보존한다', () => {
    const o = scene(); try { const sid = o.world.create(83, 0); const raw = o.core.pool.physical(sid); put(raw, 14, floatToBits(20.5)); put(raw, 18, floatToBits(30.25)); const calls = []; const shrapnel = o.particles.addShrapnel; const visual = o.hits.impact.visual; o.particles.addShrapnel = (...args) => calls.push(['shrapnel', ...args]); o.hits.impact.visual = (...args) => calls.push(['visual', ...args]);
        o.collapseEffect(sid); o.ritualExplosion(sid); assert.deepEqual(calls[0], ['shrapnel', sid, ShrapnelKind.Collapse]); assert.deepEqual(calls[1], ['visual', Math.fround(20.5 - o.core.types[83].footX * 0.5), 30.25, 1, Math.fround(0.04)]); assert.deepEqual(calls[2], ['shrapnel', sid, ShrapnelKind.Explosion]); calls.length = 0; o.loginEnd = NaN; o.ritualExplosion(sid); assert.deepEqual(calls, [['shrapnel', sid, ShrapnelKind.Explosion]]); o.particles.addShrapnel = shrapnel; o.hits.impact.visual = visual;
    } finally { o.dispose(); }
});

test('Session Damageable: 외부 전/후처리 효과는 값으로 복사하고 실제 내부 부모만 덮어쓴다', () => {
    const collapse = () => 1; const drop = () => 2; const pre = { collapse }; const post = { dropDude: drop }; const o = owner({ damageablePre: pre, damageablePost: post }); pre.collapse = forbidden; post.dropDude = forbidden;
    try { before(o); o.connectDamageable(); assert.equal(o.collapseEffect, collapse); assert.equal(o.damageableModules.postHooks.dropDude, drop); assert.equal(o.damageableModules.preHooks.collapse, collapse); } finally { o.dispose(); }
});

test('Session Damageable: 해방/공통 삭제는 현재 World를 조회하며 선택된 실패를 전파한다', () => {
    const o = scene(); try { const m = o.damageableModules; const release = o.damageableRelease.release; const common = o.world.commonDeletion; const base = common.preDestroy; const calls = []; o.damageableRelease.release = (sid) => { calls.push(['release', sid, o.state.damageableReleaseState.battle]); }; common.preDestroy = (...args) => calls.push(['base', ...args]); o.core.ownerMode.battle = false; m.preHooks.releaseContained(50); m.preHooks.base(51, 9); assert.deepEqual(calls, [['release', 50, false], ['base', 51, 9]]); o.damageableRelease.release = release; common.preDestroy = base; } finally { o.dispose(); }
});

test('Session Damageable: 원문 전역/위치 소리는 현재 서비스/이름표와 여섯/다섯 인자를 사용한다', () => {
    const o = scene(); try { const calls = []; o.services.sound.play = (...args) => calls.push(['play', ...args]); o.services.sound.playOnceAt = (...args) => calls.push(['at', ...args]); const sound = o.damageableModules.preHooks.sound; sound({ sound: DAMAGEABLE_SOUND.priestFree }); sound({ sound: DAMAGEABLE_SOUND.collapse, x: 2.5, y: 3.5 }); sound({ sound: DAMAGEABLE_SOUND.explosion, x: 4, y: 5 }); assert.deepEqual(calls, [['play', o.soundNames.lookup('priestFree.wav'), 0, 0, 0, 1, 0], ['at', 2.5, 3.5, o.soundNames.lookup('collapse.wav'), 0, 0], ['at', 4, 5, o.soundNames.lookup('explosion.wav'), 0, 0]]); o.services.sound.playOnceAt = null; assert.throws(() => sound({ sound: DAMAGEABLE_SOUND.collapse, x: 1, y: 2 }), /위치 소리 미연결/); } finally { o.dispose(); }
});

test('Session Damageable: 드롭/섬 정리는 현재 실제 부모와 시각을 사용한다', () => {
    const o = scene(); try { const h = o.damageableModules.postHooks; assert.throws(() => h.dropIsland(1, 2, 3, 4), /설치 전/); const original = o.dudeDrop.drop; const calls = []; o.dudeDrop.drop = (...args) => calls.push(['dude', ...args]); o.buildingEffects = { dropIsland: (...args) => { assert.equal(o.state.buildingState.now, 12.5); calls.push(['island', ...args]); } }; o.core.processState.now = 12.5; h.dropDude(50, 2, false); h.dropIsland(1, 2, 3, 4); assert.deepEqual(calls, [['dude', 50, 2, false], ['island', 1, 2, 3, 4]]); o.dudeDrop.drop = original; } finally { o.dispose(); }
});

test('Session Damageable: 피해 Regular는 같은 실제 Host 객체에 초기 count를 쓰고 가상 예약 여부를 조회한다', () => {
    const o = scene(); try { const sid = o.world.create(83, 0); const h = o.damageableModules.postHooks; h.regular(sid, 6699, 0.1, 0x80000001); const process = o.core.processes.findEvent(sid, 6699); assert.ok(process); assert.equal(process.count, 0x80000001); assert.equal(process.payload, Math.fround(0.1)); assert.equal(h.hasBridgeFall(sid), false); } finally { o.dispose(); }
});

test('Session Damageable: 신전 삭제의 섬 비움은 실제 목록을 수정한 뒤 현재 생산 갱신을 부른다', () => {
    const calls = []; const o = scene({ ownership: { refreshProduction: () => calls.push('initial') } }); try { const h = o.damageableModules.vortexHooks; const events = []; assert.throws(() => h.clearIslandTemple(1), /진단 장치 미연결/);
    // 원문처럼 섬이 없으면 Vortex.cpp 0x16f 진단을 표시한 뒤 정상 복귀해 섬 비움과 생산 갱신을 계속한다(E1207). 신전 삭제 진단 두 종류도 같은 장치에 표시한다.
    o.world.storage.bindDisplayDiagnostics({ value: '' }, (...args) => events.push(args)); const lines = []; const setBefore = o.islands.setTemple; o.islands.setTemple = (index, value) => lines.push([index, value]); h.clearIslandTemple(1); o.islands.setTemple = setBefore;
    h.assertion(0x2ab); h.assertion(0x2ac); const vortex = '\\Ns\\O\\Vortex.cpp';
    assert.deepEqual(events, [['islandList.exists( islandId )', vortex, 0x16f], ['islandList.exists( islandId )', vortex, 0x2ab], ['islandList.getVortexSid( islandId ) == getSid()', vortex, 0x2ac]]); assert.deepEqual(lines, [[1, 0]]); assert.deepEqual(calls, ['initial']);
    const exists = o.islands.exists; const set = o.islands.setTemple; o.islands.exists = () => true; o.islands.setTemple = (index, value) => calls.push([index, value]); o.services.ownership.refreshProduction = () => calls.push('current'); h.clearIslandTemple(2); h.refreshProduction(); assert.deepEqual(calls, ['initial', [2, 0], 'current', 'initial']); o.islands.exists = exists; o.islands.setTemple = set; } finally { o.dispose(); }
});

test('Session Damageable: 간헐천 기본 프레임은 실제 자산의 현재 TYPE이고 AI0 목록은 미설치 본체를 요구하지 않는다', () => {
    const o = scene(); try { const h = o.damageableModules.geyserHooks; const sid = o.world.create(83, 0); assert.equal(h.defaultFrame(sid), game.types()[13].definition.defaultFrame >>> 0); const raw = o.core.pool.physical(sid); raw[10] = 69; assert.throws(() => h.defaultFrame(sid), /TYPE 오류/); raw[10] = 83; assert.equal(o.aiWorld, null); h.registerAi(sid, 0); const original = o.damageablePost.postDestroy; const calls = []; o.damageablePost.postDestroy = (...args) => calls.push([...args, o.state.damageablePostState.battle]); o.core.ownerMode.battle = false; h.damageablePost(sid, 9); assert.deepEqual(calls, [[sid, 9, false]]); o.damageablePost.postDestroy = original; } finally { o.dispose(); }
});

test('Session Damageable: 두 표시 경계는 같은 현재 물리 span을 빌려 효과의 쓰기를 보존한다', () => {
    const o = scene(); try { const sid = o.world.create(83, 0); const raw = o.core.pool.physical(sid); const update = o.core.display.update; let calls = 0;
        o.core.display.update = (current) => { assert.equal(current.buffer, raw.buffer); assert.equal(current.byteOffset, raw.byteOffset); current[36] = ++calls; };
        o.damageableModules.postHooks.displayUpdate(sid); assert.equal(raw[36], 1); o.damageableModules.geyserHooks.update(sid); assert.equal(raw[36], 2); o.core.display.update = update;
    } finally { o.dispose(); }
});

// 실제 Session의 다리 약화가 같은 World 프레임 코드 배열을 읽고 원본 수명·표시를 갱신한다.
test('Session Damageable: 실제 다리 프레임 표로 약화한다', () => {
    const o = scene();
    try {
        const sid = o.world.create(o.core.bridgeState.bridgeType, 2), raw = o.core.pool.allocated(sid);
        put(raw, 12, 56, 2); put(raw, 36, 0);
        assert.doesNotThrow(() => o.damageablePost.weaken(sid));
        assert.equal((raw[12] | (raw[13] << 8)) & 0x38, (o.state.damageablePostState.bridgeCrackLife - 1) * 8);
    } finally { o.dispose(); }
});

test('Session Damageable: Dude 부모/정기 진단은 원문 DudeAssert 와 같은 식·행으로 표시한 뒤 계속한다(E1211)', () => {
    const o = scene(); try {
        const m = o.damageableModules; const file = '.' + String.fromCharCode(92) + 'Dude.cpp';
        assert.throws(() => m.behaviorHooks.assertion(0xde), /진단 장치 미연결/); const shown = []; o.world.storage.bindDisplayDiagnostics({ value: '' }, (...args) => shown.push(args));
        for (const line of [0xde, 0x127, 0x9c, 0x153]) { m.behaviorHooks.assertion(line); m.regularHooks.assertion(line); }
        const expected = [['!s.isDead() && !s.isFree()', 0xde], ['walkSid', 0x127], ['task == taskFALL', 0x9c], ['0', 0x153]].flatMap(([expression, line]) => [[expression, file, line], [expression, file, line]]);
        assert.deepEqual(shown, expected);
    } finally { o.dispose(); }
});

test('Session Damageable: Dude 낙하 task 교체는 원문처럼 진단(Dude.cpp 0x9c)을 표시한 뒤 새 task 를 계속 쓴다(E1211)', () => {
    const o = scene(); try {
        const sid = o.world.create(83, 0); const raw = o.core.pool.physical(sid); raw[12] = 0x05; raw[13] = 0x51; // 낙하(0x5105) 중인 task WORD 이다.
        assert.throws(() => o.dudeTask.set(sid, 0x5100, 0.5), /진단 장치 미연결/); assert.equal(o.core.pool.slot(sid)[12] | (o.core.pool.slot(sid)[13] << 8), 0x5105, '진단 실패 앞에서는 task WORD 를 쓰지 않는다');
        const shown = []; o.world.storage.bindDisplayDiagnostics({ value: '' }, (...args) => shown.push(args)); o.dudeTask.set(sid, 0x5100, 0.5);
        assert.deepEqual(shown, [['task == taskFALL', '.' + String.fromCharCode(92) + 'Dude.cpp', 0x9c]]); assert.equal(o.core.pool.slot(sid)[12] | (o.core.pool.slot(sid)[13] << 8), 0x5100);
        assert.ok(o.core.processes.findEvent(sid, 0x5100), '새 task Regular 가 같은 Host 에 부착된다');
    } finally { o.dispose(); }
});
