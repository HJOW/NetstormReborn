// CP1375의 epoch DWORD·UTC초 경계·최초Capture와 취소를 독립 입력/실제 Client 시계에서 검증한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { CLIENT_CLOCK_EPOCH, prepareClientClockOrigin } from '../www/core/client-clock-origin.js';
import { GameClock } from '../www/core/game-clock.js';
import { ClientFrameClock } from '../www/core/client-frame-clock.js';
import { ClientTimeSource } from '../www/core/client-time-source.js';
import { Netstorm } from '../www/netstorm.js';

// JS 비트연산과 독립인 BigInt unsigned DWORD 연산으로 기대 초를 계산한다.
function seconds(now, origin, base, ticks = 0) {
    const value = BigInt.asUintN(32, BigInt(now) - BigInt(origin) - BigInt(ticks) + BigInt(base) * 1000n);
    return Number(value / 1000n) + Number(value % 1000n) * 0.001;
}

// epoch*1000도32비트로 먼저 감기며 현재 보정은Game만 적용한다. 큰epoch를double초에 나중에 더하는 변형을 검출한다.
test('GameClock 초기 초: DWORD 곱셈/덧셈·밀리초 래핑·현재 동기화', () => {
    for (const base of [0, 1, 4294967, 0x80000001, 0xffffffff]) for (const [origin, now] of [[0, 123], [0xfffffff0, 9], [5000, 4000]]) {
        const clock = new GameClock(origin, base), ticks = { value: 17 }; clock.bindSynchronizationTicks(ticks);
        assert.equal(clock.wallSeconds(now), seconds(now, origin, base)); assert.equal(clock.gameSeconds(now), seconds(now, origin, base, 17));
        ticks.value = 0x80000001; assert.equal(clock.gameSeconds(now), seconds(now, origin, base, ticks.value));
    }
    for (const value of [-1, 0x100000000, 1.5, NaN]) assert.throws(() => new GameClock(0, value), /초기 초/);
});

// 대기 후 현재 millis→UTC초 순서와 고정 모드의 대기 생략을 독립 공급원 호출표로 확인한다.
test('Client 초기 원점: UTC초 경계 뒤 현재 밀리초와 epoch를 선택한다', async () => {
    let current = CLIENT_CLOCK_EPOCH + 123, milliseconds = 1000, waits = 0; const calls = [];
    const source = { fixedEnabled: false, milliseconds: () => { calls.push('ms'); return milliseconds; }, unixSeconds: () => { calls.push('utc'); return current; } };
    const result = await prepareClientClockOrigin(source, { wait: async () => { calls.push('wait'); waits += 1; milliseconds += 7; if (waits === 2) current += 1; } });
    assert.deepEqual(result, { originMilliseconds: 1014, baseSeconds: 124, initialCapture: true });
    assert.deepEqual(calls, ['ms', 'utc', 'utc', 'wait', 'utc', 'wait', 'utc', 'ms', 'utc']);
    const fixed = new ClientTimeSource(); fixed.enableFixed({ frameMilliseconds: 25 });
    assert.deepEqual(await prepareClientClockOrigin(fixed, { wait: () => { throw new Error('고정 모드 대기 금지'); } }), { originMilliseconds: 1000000, baseSeconds: 0, initialCapture: false });
});

// 취소는 대기 뒤 추가 UTC/millis 조회도 중단한다. 시계를 적용하지 않아 이전 앱이 새 수명에 영향을 줄 수 없다.
test('Client 초기 원점: 취소된 준비는 추가 시각 조회 없이 반납한다', async () => {
    let cancelled = false; const calls = [];
    const source = { milliseconds: () => { calls.push('ms'); return 0; }, unixSeconds: () => { calls.push('utc'); return CLIENT_CLOCK_EPOCH; } };
    assert.equal(await prepareClientClockOrigin(source, { cancelled: () => cancelled, wait: async () => { cancelled = true; } }), null);
    assert.deepEqual(calls, ['ms', 'utc', 'utc']);
    await assert.rejects(prepareClientClockOrigin(null), /공급원/);
});

// 최초Capture는 보정 연결 전이다. 다음 프레임은 epoch 전체가 아닌 실제 차이만delta로 보관한다.
test('Client 프레임 초기화: 최초Capture·같은 프레임 주소·보정 연결 순서', async () => {
    let milliseconds = 1000, utc = CLIENT_CLOCK_EPOCH + 123;
    const source = new ClientTimeSource({ hostMilliseconds: () => milliseconds, hostUnixSeconds: () => utc }), ticks = { value: 7 }, clock = new ClientFrameClock(source, ticks), frame = clock.time();
    assert.equal(await clock.prepareOrigin({ wait: async () => { milliseconds += 2; utc += 1; } }), true);
    assert.equal(clock.time(), frame); assert.deepEqual(frame, { game: 124, wall: 124, delta: 124, number: 1 });
    milliseconds += 25; assert.equal(clock.captureFrame(), frame);
    assert.equal(frame.game, 124.018); assert.equal(frame.wall, 124.025); assert.ok(Math.abs(frame.delta - 0.018) < 1e-12); assert.equal(frame.number, 2);
    await assert.rejects(clock.prepareOrigin(), /이미 프레임/);
});

// 초기 비활성 창이 보유한 중첩 깊이를 그대로 유지하며 고정 시각의 원점/4틱은 다시 잡지 않는다.
test('Client 프레임 초기화: 초기 정지 깊이와 고정 모드 원점 보존', async () => {
    let milliseconds = 0, utc = CLIENT_CLOCK_EPOCH + 4;
    const source = new ClientTimeSource({ hostMilliseconds: () => milliseconds, hostUnixSeconds: () => utc }), clock = new ClientFrameClock(source, { value: 0 });
    clock.pushGamePause(); clock.pushGamePause(); await clock.prepareOrigin({ wait: async () => { utc += 1; } });
    milliseconds = 100; clock.popGamePause(); assert.equal(clock.paused(), true); clock.popGamePause(); assert.equal(clock.paused(), false);
    assert.ok(Math.abs(clock.captureFrame().game - 5) < 1e-12); assert.equal(clock.time().wall, 5.1);
    const fixed = new ClientTimeSource(); fixed.enableFixed({ frameMilliseconds: 25 }); const deterministic = new ClientFrameClock(fixed, { value: 0 });
    for (let index = 0; index < 4; index += 1) fixed.advanceFixed();
    await deterministic.prepareOrigin({ wait: () => { throw new Error('고정 대기 금지'); } });
    assert.equal(deterministic.time().number, 0); assert.equal(deterministic.captureFrame().game, 0.125);
});

// 실제 앱 관문은 준비 중 게임 시계·Guide를 실행하지 않고 로딩표시만 그린다.
test('Netstorm 초기 시계: 준비 중Client 프레임을 실행하지 않는다', () => {
    const app = new Netstorm(), calls = []; app._canvas = {}; app._clientClockPreparing = true;
    app._gameClock = { captureFrame: () => calls.push('clock') }; app._clientGuide = { run: () => calls.push('guide') };
    app._render = () => calls.push('render'); app._startFrameLoop = () => calls.push('next'); app._onFrame(0);
    assert.deepEqual(calls, ['render', 'next']);
});
