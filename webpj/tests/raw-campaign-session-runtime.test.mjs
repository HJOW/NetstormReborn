// Session1611..1649의 실제 Runtime 훅 값/현재 상태/후속 포인터 선택을 대조한다(WB04-E595).
// 보호 자산·기존 실제 모듈과 명시 소비자 계약을 사용한다. 전체 Session 공개/미션/실제 의식·주문 시전 관측은 아니다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { DaisRegularState } from '../www/core/raw-dais-regular.js';
import { SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put, get, floatToBits } from './raw-support.mjs';

// 선택하지 않은 외부 경계는 빈 성공으로 바꾸지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 같은 보호 자산/팔레트와 실제 소리 이름 표다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 실제 기반 owner만 구성하며 위 생성자 구간과 실제 외부 함수만 선택적으로 공급한다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 앞 생성자 구간을 실제 부모와 같은 순서로 조립한다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); }
// Runtime 훅까지 값으로 준비한다. 실제 World.BindRuntime/Session 공개는 하지 않는다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectWorldRuntimeHooks(); return o; }
// 실제 생성자에 고정 수치 입력만 둔다. 대상 Pop/공격/시전 성공을 가장하지 않는다.
function actor(o, type = 75, flags = 0) { const sid = o.world.create(type, flags); const raw = o.core.pool.allocated(sid); raw[34] = 2; put(raw, 14, floatToBits(21)); put(raw, 18, floatToBits(22)); put(raw, 26, 10000); return sid; }

test('Session Runtime: 선행/한 번 결선과 실제 함수 참조를 보존하며 World/Session은 게시하지 않는다', () => {
    const o = owner();
    try { assert.throws(() => o.connectWorldRuntimeHooks(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectWorldRuntimeHooks();
        assert.equal(o.runtimeModules.ready, true); assert.equal(o.worldHooks, o.runtimeModules.hooks); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed);
        assert.equal(o.worldHooks.factoryRegular.begin, o.assetModules.factoryRuntime.begin); assert.equal(o.worldHooks.residence.spawn, o.assetModules.residence.spawn); assert.equal(o.worldHooks.damageable.hitPoints, o.decorationHit); assert.equal(o.worldHooks.vortexRegular.leave, o.assetModules.vortexRuntime.leave);
        assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); assert.throws(() => o.connectWorldRuntimeHooks(), /반복/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectWorldRuntimeHooks(), /반납/); assert.throws(() => o.worldHooks.squidConnected(1), /반납/);
});

test('Session Runtime: 서비스 삭제 표와 초기 함수는 값 복사하며 포획/UI 후속 포인터는 호출 때 찾는다', () => {
    const calls = []; const deletion = { selected: () => 123, emit: (event) => calls.push(['initial', event]) }; const form = (packet) => calls.push(['form', packet]);
    const o = owner({ deletion, formPacket: form, dependencyTransmit: (...args) => calls.push(['dependency', ...args]), gumpNotice: (...args) => calls.push(['gump', ...args]) }); deletion.emit = forbidden; deletion.selected = forbidden;
    try { before(o); o.connectWorldRuntimeHooks(); const h = o.worldHooks; o.services.deletion.emit = forbidden; o.services.formPacket = forbidden;
        assert.equal(h.assets.selected(), 123); const event = { effect: 'test-selected-effect' }; h.assets.emit(event); const packet = Uint8Array.of(1, 2); h.squidPacket(packet); h.formTransmit(7, 9); h.gumpNotice(11, 12, 13);
        assert.deepEqual(calls, [['initial', event], ['form', packet], ['dependency', 7, 9], ['gump', 11, 12, 13]]);
        assert.throws(() => h.contentNotice(1, 2, 3), /내용물 통지/); assert.throws(() => h.capturedContent(1, 2), /사제 포획 안내/);
        o.contentNotice = { notice: (...args) => calls.push(['content', ...args]) }; o.priestTakenNotice = { notify: (...args) => calls.push(['captured', ...args]) };
        h.contentNotice(3, 4, 5); h.capturedContent(6, 0xfedcba98); assert.deepEqual(calls.slice(-2), [['content', 3, 4, 5], ['captured', 6, 0xfedcba98]]);
    } finally { o.dispose(); }
});

test('Session Runtime: 빈 Form/선택 함수는 최초 연결 위치에서 실패하고 다른 삭제 효과도 성공으로 대체하지 않는다', () => {
    const o = owner(); before(o); o.services.dependencyTransmit = null; o.connectWorldRuntimeHooks(); try { assert.throws(() => o.worldHooks.formTransmit(1, 2), /cpp:1629/); assert.throws(() => o.worldHooks.assets.selected(), /cpp:1630/); assert.throws(() => o.worldHooks.assets.emit({ effect: 'other' }), /다른 파생 삭제/); assert.equal(o.worldHooks.gumpNotice, undefined); } finally { o.dispose(); }
});

test('Session Runtime: 삭제는 현재 World 공용+58을 쓰며 훅을 준비한 것만으로 방송을 게시하지 않는다', () => {
    const packets = []; const o = scene({ formPacket: (packet) => packets.push(packet.slice()) });
    try { const sid = actor(o); const state = o.gameplayGlobals.gameAux().squidTransmission; state.buffer.fill(0xa5); state.deletion.sid = sid; state.deletion.token = 0x71;
        o.worldHooks.assets.emit({ effect: SQUID_DESTROY_EFFECT.transmit, sid, flags: 0xabcd0009 });
        assert.deepEqual([...state.buffer.subarray(0, 8)], [1, 0x2e, 8, 0, sid & 255, sid >>> 8, 0xf1, 0xab]); assert.equal(state.buffer[8], 0xa5); assert.equal(state.deletion.sid, 0); assert.deepEqual(packets, []);
    } finally { o.dispose(); }
});

test('Session Runtime: 연결 비트는 현재 Player+50을 읽고 대상 송신은 현재 서비스 슬롯/NULL을 보존한다', () => {
    const calls = []; const peers = new Array(9).fill(null); peers[8] = (packet) => calls.push(['first', packet]); const o = scene({ battlePeers: peers }); peers[8] = forbidden;
    try { const h = o.worldHooks; o.players[1 * 180 + 0x50] = 1; o.players[8 * 180 + 0x50] = 2;
        assert.deepEqual([-1, 0, 1, 8, 9].map((target) => h.squidConnected(target)), [false, false, true, false, false]); o.players[8 * 180 + 0x50] |= 1; assert.equal(h.squidConnected(8), true); assert.equal(h.squidConnected(0x100000001), true);
        const packet = Uint8Array.of(1, 2); h.squidSend(0, packet); h.squidSend(8, packet); o.services.battlePeers[8] = (value) => calls.push(['next', value]); h.squidSend(8, packet);
        assert.deepEqual(calls, [['first', packet], ['next', packet]]); assert.throws(() => h.squidSend(-1, packet), /범위/); assert.throws(() => h.squidSend(9, packet), /범위/);
    } finally { o.dispose(); }
});

test('Session Runtime: 표적 중심 호출 뒤 같은 raw의 현재 좌표/TYPE/owner/부호비트를 실제 MayTarget으로 보낸다', () => {
    const o = scene(); try { const sid = actor(o, 140); const raw = o.core.pool.physical(sid); const center = o.gunTarget.center.bind(o.gunTarget); const may = o.gunTarget.mayTarget.bind(o.gunTarget); let expectedCenter; let query;
        o.gunTarget.center = (parent) => { expectedCenter = center(parent); put(raw, 14, floatToBits(42)); put(raw, 18, floatToBits(43)); raw[34] = 7; put(raw, 36, 0x80000000); return expectedCenter; };
        o.gunMode.get = () => 0xfedcba98; o.gunTarget.mayTarget = (current, target) => { query = { ...current }; return may(current, target); };
        assert.equal(o.worldHooks.attacks.mayTarget(sid, 0), false); assert.deepEqual(query, { x: 42, y: 43, centerX: expectedCenter[0], centerY: expectedCenter[1], type: 140, direction: 0xfedcba98 | 0, owner: 7, frame: -2147483648 });
    } finally { o.dispose(); }
});

test('Session Runtime: 실제 Dude 진입 기하와 현재 이동 부모의 값형 목적지/반환을 보존한다', () => {
    const o = scene(); try { const sid = actor(o, 100); const point = o.hits.dudes.residencePoint(sid); assert.deepEqual(o.worldHooks.dude.entrance(sid), { x: point[0], y: point[1] });
        assert.equal(o.worldHooks.dude.movementType(158), o.priests.movementProcessType(158)); const calls = []; const original = o.hits.dudes.move;
        o.hits.dudes.move = (...args) => { calls.push(args); return false; }; const target = { x: 12.5, y: 19, extra: 7 }; assert.equal(o.worldHooks.dude.move(sid, target, true), false);
        assert.deepEqual(calls, [[sid, [12.5, 19], true]]); assert.deepEqual(target, { x: 12.5, y: 19, extra: 7 }); o.hits.dudes.move = original;
    } finally { o.dispose(); }
});

test('Session Runtime: 빔/분해는 같은 현재 입자 부모를 선택하며 미설치 몸체는 선택 지점에서 실패한다', () => {
    const o = scene(); try { const particles = o.particles; o.particles = null; assert.throws(() => o.worldHooks.attacks.beam(1, 2), /빔 몸체/); assert.throws(() => o.worldHooks.attacks.dissolve(1, 2, 3, true), /분해 몸체/); o.particles = particles;
        const beam = particles.addBeam; const dissolve = particles.addDissolve; const calls = []; const stop = new Error('현재 입자 호출 관측');
        particles.addBeam = (...args) => { calls.push(['beam', ...args]); throw stop; }; particles.addDissolve = (...args) => { calls.push(['dissolve', ...args]); throw stop; };
        assert.throws(() => o.worldHooks.attacks.beam(11, 12), (error) => error === stop); assert.throws(() => o.worldHooks.attacks.dissolve(13, 14, 15, true), (error) => error === stop);
        assert.deepEqual(calls, [['beam', 11, 12, 'shortZap.wav'], ['dissolve', 13, 14, 15, true]]); particles.addBeam = beam; particles.addDissolve = dissolve;
    } finally { o.dispose(); }
});

test('Session Runtime: 의식 미설치 관문은 Rune/동맹 준비보다 먼저이며 애니메이션 삭제는 별도다', () => {
    const o = scene(); try { const h = o.worldHooks.ritualRegular; o.state.runeState.playerFlags[0] = 0x77; o.players[0x50] = 0x11;
        // 각 의식 진입은 실제 후속 포인터가 없으면 공통 준비를 진행하지 않는다.
        for (const call of [() => h.progress(1), () => h.release(1), () => h.escapeFrame(1, 2), () => h.sacrificeAdvance(1), () => h.ritual(1)]) assert.throws(call, /설치 전 호출/);
        assert.equal(o.state.runeState.playerFlags[0], 0x77); assert.throws(() => h.removeAnimation(2), /애니메이션 제거/); const calls = []; o.ritualExplosion = (sid) => calls.push(sid); h.removeAnimation(91); assert.deepEqual(calls, [91]);
    } finally { o.dispose(); }
});

test('Session Runtime: 의식 준비는 현재 Player9/동맹81을 값 복사하고 여섯 실제 후단의 인자/순서를 유지한다', () => {
    const o = scene(); try { o.daisState = new DaisRegularState(o.state.ritualState, o.state.runeState, o.core.processState); const calls = [];
        // 원문 각각의 BYTE와 DWORD 부호 변환을 다른 값으로 구분한다.
        for (let index = 0; index < 9; index += 1) o.players[index * 180 + 0x50] = 0xa0 + index;
        o.alliances[0] = 0xffffffff; o.alliances[80] = 0x80000000;
        o.daisRegular = { progress: (sid) => calls.push(['progress', sid, o.state.runeState.playerFlags[0], o.daisState.alliances[0]]), sacrificeAdvance: (sid) => { calls.push(['advance', sid]); return -7; } };
        o.altarSacrifice = { releaseFromAltar: (sid) => calls.push(['release', sid]), escapeFrame: (sid, frame) => calls.push(['escape', sid, frame]) };
        o.templeRitual = { findStrongOwnedPriest: (sid) => { calls.push(['find', sid]); return 15123; }, ritual: (...args) => calls.push(['ritual', ...args]) };
        const h = o.worldHooks.ritualRegular; h.progress(15100); assert.deepEqual(o.state.runeState.playerFlags, [160, 161, 162, 163, 164, 165, 166, 167, 168]); assert.deepEqual([o.daisState.alliances[0], o.daisState.alliances[80]], [-1, -2147483648]);
        o.alliances[0] = 123; assert.equal(o.daisState.alliances[0], -1); h.release(15100); assert.equal(o.daisState.alliances[0], 123); h.escapeFrame(15100, 0xfedcba98); assert.equal(h.sacrificeAdvance(15100), -7); h.ritual(15100);
        assert.deepEqual(calls, [['progress', 15100, 160, -1], ['release', 15100], ['escape', 15100, 0xfedcba98], ['advance', 15100], ['find', 15100], ['ritual', 15100, 15123, 1]]);
    } finally { o.dispose(); }
});

test('Session Runtime: 의존 통지는 감시자의 가상 표로 원문 부모 삭제 통지(VT+34)를 고른다', () => {
    const o = scene(); try { const sid = actor(o, 110); const raw = o.core.pool.physical(sid); const calls = []; const destroy = o.world.destroy; const notify = o.core.processes.notifyDependency; const flyers = o.flyerModules;
        o.world.destroy = (...args) => { calls.push(['destroy', ...args]); }; o.core.processes.notifyDependency = (...args) => { calls.push(['notify', ...args]); }; o.flyerModules = { frameHooks: { homeNotice: (...args) => { calls.push(['home', ...args]); } } };
        const send = (vt, type = 110) => { raw[10] = type; put(raw, 0, vt); calls.length = 0; o.worldHooks.dependencyNotice(sid, 8, 9); return calls.map((call) => call.join(':')); };
        // 현재 자산을 flags 0 으로 가상 삭제하는 세 표(표시 더미 0x50eb98·Factory 0x509148·Outpost 0x509210): TYPE 은 보지 않는다(Factory 업그레이드 그림은 건물과 같은 TYPE).
        for (const vt of [0x50eb98, 0x509148, 0x509210]) assert.deepEqual(send(vt, 85), [`destroy:${sid}:0`], '0x' + vt.toString(16));
        // 비행장 홈 세 표는 홈 통지이고, 나머지 자산 표(base 0x501dd0 포함)와 Depend/Gump/Content Form 표는 빈 반환이다.
        for (const vt of [0x509930, 0x5099e8, 0x509aa0]) assert.deepEqual(send(vt), [`home:${sid}:0:0`], '0x' + vt.toString(16));
        for (const vt of [0x501dd0, 0x501ea0, 0x50f210, 0x512110, 0x512188, 0x512208]) assert.deepEqual(send(vt), [], '0x' + vt.toString(16));
        // Process Form 만 서버일 때의 PID 통지로 가고, 알 수 없는 표는 오류이다.
        assert.deepEqual(send(0x512098), [`notify:${sid}:8:9`]); assert.throws(() => send(0xcafebabe), /가상34 미연결/);
        o.world.destroy = destroy; o.core.processes.notifyDependency = notify; o.flyerModules = flyers;
    } finally { o.dispose(); }
});

test('Session Runtime: 주문 준비는 같은 현재 권한/소유자와 별도81DWORD 스냅샷을 사용한다', () => {
    const o = scene(); try { const s = o.state; const aliases = s.devastationBlastState.alliances; s.assetState.writeBoss(false); o.playerState.battleOwner = 7; o.alliances[0] = 0x80000000;
        o.runtimeModules.prepareDevastation(); assert.deepEqual([s.devastationTargetState.boss, s.devastationBlastState.boss, s.devastationEffectState.boss, s.devastationRegularState.boss, s.devastationRegularState.localOwner], [0, 0, 0, 0, 7]);
        assert.equal(s.devastationBlastState.alliances, aliases); assert.notEqual(aliases, o.alliances); assert.equal(aliases[0], 0x80000000); o.alliances[0] = 19; assert.equal(aliases[0], 0x80000000);
        s.assetState.writeBoss(true); o.runtimeModules.prepareDevastation(); assert.deepEqual([s.devastationTargetState.boss, s.devastationRegularState.boss, aliases[0]], [1, 1, 19]);
    } finally { o.dispose(); }
});

test('Session Runtime: 주문 검색은 준비 뒤 같은 출력 예약/용량/count를 실제 검색기 계약에 넘긴다', () => {
    const o = scene(); try { const h = o.worldHooks.bombRegular; const out = new Uint32Array(3); o.state.devastationRegularState.localOwner = 99; o.playerState.battleOwner = 6;
        assert.throws(() => h.targets(15100, out), /검색 몸체/); assert.equal(o.state.devastationRegularState.localOwner, 6);
        o.devastationTargets = { find: (sid, result) => { assert.equal(sid, 15100); assert.equal(result.storage, out); assert.deepEqual([result.capacity, result.count], [3, 0]); result.storage[0] = 15123; result.count = 1; } };
        assert.equal(h.targets(15100, out), 1); assert.equal(out[0], 15123); const calls = []; o.flare = { spawn: (...args) => calls.push(args) }; h.notice(15100, 0xffffffff, 0xfedcba98); assert.deepEqual(calls, [[15100, -1, 0xfedcba98]]);
    } finally { o.dispose(); }
});

test('Session Runtime: 주문 수신은 부모WORD/현재VT 관문 뒤 상태를 준비하고 sender를 읽지 않는다', () => {
    const o = scene(); try { const packet = new Uint8Array(10); const sid = actor(o); put(packet, 4, sid, 2); const h = o.worldHooks.bombRegular;
        o.state.devastationTargetState.boss = 99; assert.throws(() => h.receive(0, 0, packet.subarray(0, 5)), /부모 WORD/); assert.throws(() => h.receive(0, 0, packet), /다른 주문/); assert.equal(o.state.devastationTargetState.boss, 99);
        const raw = o.core.pool.physical(sid); put(raw, 0, 0x503188); assert.throws(() => h.receive(0, 0, packet), /수신 몸체/); assert.equal(o.state.devastationTargetState.boss, 1);
        const calls = []; o.devastationEffect = { receive: (input) => calls.push(input) }; const ignored = new Proxy({}, { get: forbidden }); h.receive(ignored, ignored, packet); assert.deepEqual(calls, [packet]);
        put(raw, 0, 0xdeadbeef); raw[11] |= 2; h.receive(ignored, ignored, packet); assert.equal(calls.length, 2);
    } finally { o.dispose(); }
});
