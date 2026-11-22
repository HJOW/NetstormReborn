// 같은 실제 Session/World/Host의 도착 세 장치를 검사한다(WB04-E640). 경로 이동/Load/실제 플레이 검사는 아니다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { get, put, floatToBits } from './raw-support.mjs';
import { RawContainedLookup } from '../www/core/raw-contained-lookup.js';
import { RegularProcess } from '../www/core/squid-process.js';

// 외부 음향/화면/선택만 제공하고 실제 생성 순서와 건설 설치 뒤 도착 장치를 붙인다.
function scene(bind = true, extra = {}) {
    const calls = [], o = campaignOwners({ ...extra, deletion: { selected: () => 0 }, construction: { refresh: () => calls.push('refresh') },
        sound: { play: (sound) => sound, playOnceAt: (...args) => { calls.push(args); return args[2]; }, playLoopAt: (_x, _y, current, sound) => current || sound, stop: () => {}, isPlaying: () => 0 },
    });
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) }); o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding();
        if (bind) o.connectMotionArrivals(); return { o, calls };
    } catch (error) { o.dispose(); throw error; }
}
// 실제 서버 건설 대상을 비어 있는 같은 좌표에 배치한다. abstract는 bit1이다.
function target(o, type = 76) {
    const sid = o.world.create(type), raw = o.core.pool.allocated(sid); raw[40] |= 1; put(raw, 26, 1); o.core.owner.set(sid, 1); o.world.pop(sid, 32, 32); return sid;
}
// 실제 임시 운반 객체를 같은 칸에 배치한다.
function courier(o, type) {
    const sid = o.world.create(type, 2); o.core.owner.set(sid, 1); o.world.pop(sid, 32, 32); return sid;
}
// 제거하지 않는 contained 조회다. 삭제가 호출되면 잘못된 입력이다.
function form(o, sid, type) { return new RawContainedLookup(o.core.pool, () => { throw new Error('조회 중 삭제 금지'); }).find(sid, type); }

test('Session 도착 설치: 실제 세 장치·같은 Pool·프로세스/난수/카메라 불변·순서/반납', () => {
    const initial = campaignOwners(); try { assert.throws(() => initial.connectMotionArrivals(), /초기화 전/); } finally { initial.dispose(); }
    const { o } = scene(false);
    try {
        const camera = o.gameplayGlobals.camera().snapshot(); camera.data.setFloat32(0, 3.125, true);
        const bytes = o.core.pool.bytes(), seed = o.random.state, before = camera.bytes.slice(); o.connectMotionArrivals();
        assert.equal(o.arrivalModules.ready, true);
        for (const device of [o.buildArrival, o.manaArrival, o.manaRegular]) assert.equal(device.pool, o.core.pool); // 세 실제 객체가 기존 풀을 빌린다.
        assert.equal(o.manaArrival, o.localManaArrival); assert.equal(o.manaRegular, o.localManaRegular); assert.equal(o.manaPendingRegular, null);
        assert.deepEqual(o.core.pool.bytes(), bytes); assert.deepEqual(camera.bytes, before); assert.equal(o.random.state, seed); assert.equal(o.loaded, false);
        assert.throws(() => o.connectMotionArrivals(), /반복/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectMotionArrivals(), /반납한/); assert.throws(() => o.arrivalModules.regularHooks.project(0, 0), /반납/);
});

test('Session 건설 도착: 실제 finder→현재 시각 준비→30/17 생성→임시134 삭제', () => {
    const { o } = scene();
    try {
        const sid = target(o), builder = courier(o, 134); assert.ok(sid >= 15000); assert.ok(o.core.types[76].flags1 & 0x8000);
        o.core.processState.now = 12.5; assert.equal(o.buildArrival.arrive(builder), 1001);
        const process = o.core.processes.fromForm(form(o, sid, 30)), visual = o.core.processes.fromForm(form(o, sid, 17));
        assert.ok(process); assert.ok(visual); assert.equal(form(o, sid, 47), 0);
        assert.equal(process.fields.start, 12.5); assert.equal(process.fields.end, 12.5 + Math.fround(0.6));
        assert.equal(o.state.buildingState.now, 12.5); assert.equal(o.state.buildingState.localPlayer, o.playerState.battleOwner);
        assert.equal(o.state.buildingEffectsState.alliances, o.alliances); assert.ok(o.core.pool.slot(builder)[11] & 3); assert.equal(o.loaded, false);
    } finally { o.dispose(); }
});

test('Session 마나 도착: 실제46/61 제거·예약 객체 부착·현재 시각/event1·주기 후 대상 소실 삭제', () => {
    const { o, calls } = scene();
    try {
        const sid = target(o), bolt = courier(o, 105), c = o.core;
        const old = c.processes.addRegular(bolt, 0, 0.05), shared = c.processes.addSharedRegular(bolt, 9, 0.05), oldForm = old.form, sharedForm = shared.form;
        c.processState.now = 21.25; assert.equal(o.manaArrival.target(bolt), sid); assert.equal(o.manaArrival.arrive(bolt), 1000);
        const arrived = c.processes.findEvent(bolt, 1); assert.ok(arrived instanceof RegularProcess); assert.notEqual(arrived, old); assert.notEqual(arrived, shared);
        assert.equal(c.processes.findEvent(bolt, 0), null); assert.equal(form(o, bolt, 61), 0);
        assert.notEqual(c.processes.fromForm(oldForm), old); assert.notEqual(c.processes.fromForm(sharedForm), shared);
        assert.equal(o.manaPendingRegular, null); assert.equal(arrived.event, 1); assert.equal(arrived.count, 0);
        assert.equal(arrived.payload, Math.fround(0.05)); assert.equal(arrived.time, 21.25);
        assert.deepEqual(calls.at(-1), [32, 32, o.soundNames.lookup('sunCannonRaiseLower5.wav'), 0, 0]);
        arrived.runFrame(); assert.equal(arrived.count, 1); assert.equal(arrived.payload, Math.fround(0.05));
        assert.notDeepEqual([get(c.pool.slot(bolt), 14), get(c.pool.slot(bolt), 18)], [floatToBits(32), floatToBits(32)]);
        c.pool.allocated(sid)[40] &= ~1; c.processState.now = arrived.time; arrived.runFrame();
        assert.equal(c.processes.findEvent(bolt, 1), null); assert.ok(c.pool.slot(bolt)[11] & 3);
    } finally { o.dispose(); }
});

test('Session 도착 빈 대상: 실제 가상 삭제·도착음 순서와 불필요 예약 없음', () => {
    const { o, calls } = scene();
    try {
        const builder = courier(o, 134); assert.equal(o.buildArrival.arrive(builder), 1001); assert.ok(o.core.pool.slot(builder)[11] & 3);
        const bolt = courier(o, 105); assert.equal(o.manaArrival.arrive(bolt), 1001); assert.ok(o.core.pool.slot(bolt)[11] & 3);
        assert.equal(calls.length, 1); assert.equal(calls[0][2], o.soundNames.lookup('sunCannonRaiseLower5.wav')); assert.equal(o.manaPendingRegular, null);
    } finally { o.dispose(); }
});

test('Session 건설 도착 예측 대상: 현재12 contained만 삭제·1002 대기·checking이면 보존', () => {
    const { o } = scene();
    try {
        const sid = o.world.create(76, 2); assert.ok(sid >= 5 && sid < 15000);
        o.core.pool.allocated(sid)[40] |= 1; o.core.owner.set(sid, 1); o.world.pop(sid, 32, 32);
        o.state.buildArrivalState.checking = 0; const builder = courier(o, 134); assert.ok(form(o, builder, 12));
        assert.equal(o.buildArrival.arrive(builder), 1002); assert.equal(form(o, builder, 12), 0);
        assert.equal(form(o, sid, 30), 0); assert.equal(o.core.pool.slot(builder)[11] & 3, 0);
        o.state.buildArrivalState.checking = 1; o.particles.addStreak(builder); const retained = form(o, builder, 12);
        assert.ok(retained); assert.equal(o.buildArrival.arrive(builder), 1002); assert.equal(form(o, builder, 12), retained);
    } finally { o.dispose(); }
});

test('Session 도착 공유 입력: 현재 프로세스 번호와 분수 카메라의 동일 주소·두 투영 입력 구분', () => {
    const { o } = scene();
    try {
        const aux = o.gameplayGlobals.gameAux(), s = o.state.manaArrivalState, camera = o.gameplayGlobals.camera(), view = camera.snapshot(), hooks = o.arrivalModules.regularHooks, borrowed = o.contentCamera;
        aux.regularProcessType = 53; aux.sharedRegularProcessType = 62;
        assert.equal(s.regularTypeNow(), 53); assert.equal(s.sharedRegularTypeNow(), 62);
        view.data.setFloat32(0, 3.125, true); view.data.setFloat32(4, 7.75, true); view.data.setInt32(28, 127, true); view.data.setInt32(32, 319, true);
        camera.rendererView().data.setFloat32(0, 100, true);
        assert.deepEqual(hooks.project(20, 30), [193, 11]); assert.deepEqual(hooks.inverse(23, -9), [5.5625, Math.fround(7.75 + 2 * Math.fround(1 / 11))]);
        view.data.setFloat32(0, 1.0625, true); assert.equal(o.contentCamera, borrowed); assert.equal(borrowed[0], floatToBits(1.0625)); assert.equal(hooks.inverse(23, -9)[0], 3.5);
        s.delay = 0.25; assert.equal(s.delayNow(), 0.25); assert.equal(o.state.manaRegularState.interval, Math.fround(0.05));
    } finally { o.dispose(); }
});

test('Session 마나 예약: 누락/지연 불일치 거부·같은 객체 소유권 이동·반납 시 미부착 해제', () => {
    const { o } = scene(); const hooks = o.arrivalModules.manaHooks;
    try {
        const bolt = courier(o, 105); assert.throws(() => hooks.addRegular(bolt, 1, 0.05), /예약 누락/);
        assert.equal(hooks.reserveRegular(), true); const reserved = o.manaPendingRegular; assert.ok(reserved instanceof RegularProcess);
        assert.throws(() => hooks.addRegular(bolt, 1, 1), /예약 누락/); assert.equal(o.manaPendingRegular, reserved);
        o.core.processState.now = 31; hooks.addRegular(bolt, 7, o.state.manaArrivalState.delay); assert.equal(o.core.processes.findEvent(bolt, 7), reserved);
        assert.equal(reserved.time, 31); assert.equal(reserved.event, 7); assert.equal(o.manaPendingRegular, null);
        hooks.reserveRegular(); assert.ok(o.manaPendingRegular);
    } finally { o.dispose(); }
    assert.equal(o.manaPendingRegular, null); assert.throws(() => hooks.reserveRegular(), /반납/);
});

test('Session 마나 주기: frameZero 는 Rift SHP 접근으로 프레임 0 을 읽고 비정상 TYPE 은 frameCheck 진단을 표시한 뒤 계속한다(E1218)', () => {
    const { o } = scene(); try {
        const hooks = o.arrivalModules.regularHooks; const type = 105; const shape = o.core.shapes[type]; const record = o.core.types[type];
        const file = '..' + String.fromCharCode(92) + 'o' + String.fromCharCode(92) + 'rifttype.h'; const shown = []; o.world.storage.bindDisplayDiagnostics({ value: '' }, (...args) => shown.push(args));
        // 정상: 진단 없이 현재 SHP 의 프레임 0 이다.
        const frames = shape.frames; assert.ok(frames?.length > 0, '시험 TYPE 의 SHP 프레임'); assert.equal(hooks.frameZero(type), frames[0]); assert.deepEqual(shown, []);
        // 해제 표식이면 진단을 표시하고도 같은 프레임 0 을 돌려준다.
        const saved = record.maxHitPoints; record.maxHitPoints = -0x22222223; assert.equal(hooks.frameZero(type), frames[0]); record.maxHitPoints = saved;
        assert.deepEqual(shown, [['frameCheck( abstract, frame, false )', file, 0xe1]]);
        // SHP 가 적재되지 않았으면 진단 뒤 실제 SHP 범위 오류이다.
        const loaded = shape.loaded; shape.loaded = false; assert.throws(() => hooks.frameZero(type), /실제 SHP 범위/); shape.loaded = loaded; assert.equal(shown.length, 2);
    } finally { o.dispose(); }
});
