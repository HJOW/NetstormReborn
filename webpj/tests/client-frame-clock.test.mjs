// 실제 Client 시각 공급원과 게임 시계가 동일한 보정 상자·프레임 수명으로 동작하는지 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientFrameClock } from '../www/core/client-frame-clock.js';
import { ClientLocalStateGlobals } from '../www/core/client-local-state-globals.js';
import { ClientTimeSource } from '../www/core/client-time-source.js';

// 고정 시각은 포착 전에 전진하고 현재 시각 조회는 이미 고정한 프레임을 변경하지 않는다.
test('Client 프레임 시계: 원본 순서와 공유 보정·현재 시각', () => {
    const source = new ClientTimeSource(), local = new ClientLocalStateGlobals();
    source.enableFixed({ frameMilliseconds: 25, originMilliseconds: 0xfffffff0 });
    const clock = new ClientFrameClock(source, local.gameTicks()), shared = clock.time();
    assert.deepEqual(shared, { game: 0, wall: 0, delta: 0, number: 0 });
    assert.equal(clock.captureFrame(), shared);
    assert.deepEqual(shared, { game: 0.025, wall: 0.025, delta: 0.025, number: 1 });
    local.gameTicks().value = 10; source.advanceFixed();
    assert.equal(clock.milliseconds(), 34); assert.equal(clock.wallSeconds(), 0.05); assert.equal(clock.gameSeconds(), 0.04);
    assert.equal(clock.time().game, 0.025); assert.equal(clock.unixSeconds(), 946684800);
    assert.equal(clock.captureFrame(), shared);
    assert.deepEqual(shared, { game: 0.065, wall: 0.075, delta: 0.04, number: 2 });
    assert.throws(() => new ClientFrameClock(null, local.gameTicks()), /시각 공급원/);
    assert.throws(() => new ClientFrameClock(source, null), /동기화 DWORD/);
});

// 실제 중첩 정지는 벽시계를 진행시키면서 게임 프레임 시간을 유지하고 마지막 해제에서만 복귀한다.
test('Client 프레임 시계: 중첩 정지와 재개', () => {
    const source = new ClientTimeSource(), local = new ClientLocalStateGlobals(); source.enableFixed({ frameMilliseconds: 100 });
    const clock = new ClientFrameClock(source, local.gameTicks()); clock.captureFrame();
    clock.pushGamePause(); clock.pushGamePause(); clock.captureFrame();
    assert.deepEqual(clock.time(), { game: 0.1, wall: 0.2, delta: 0, number: 2 });
    clock.popGamePause(); assert.equal(clock.paused(), true); clock.captureFrame();
    clock.popGamePause(); assert.equal(clock.paused(), false); clock.captureFrame();
    assert.ok(Math.abs(clock.time().game - 0.2) < 1e-12);
    assert.equal(clock.time().wall, 0.4); assert.equal(clock.time().number, 4);
    clock.resetGameClock(); assert.equal(clock.gameSeconds(), 0.4);
});
