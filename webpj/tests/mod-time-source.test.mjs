// 실제 원래 Client 시계로 무모드 경로의 비트와 중첩 정지/uint32 감김을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientTimeSource } from '../www/core/client-time-source.js';
import { ClientFrameClock } from '../www/core/client-frame-clock.js';
import { ModTimeSource } from '../www/modding/mod-time-source.js';

// 정수 경계와 timeGetTime 감김 부근을 각각 독립 원점으로 검사한다.
for (const origin of [0, 1000000, 0xffffff00]) test(`모드 꺼짐의 원래 프레임 비트 보존: ${origin}`, () => {
    let raw = origin;
    const native = new ClientTimeSource({ hostMilliseconds: () => raw, hostUnixSeconds: () => 1000 }), source = new ModTimeSource(native), plain = new ClientFrameClock(native, { value: 0 }), wrapped = new ClientFrameClock(source, { value: 0 });
    // 각 실제 경과의 모든 공개 프레임 필드를 원래 공급원과 그대로 대조한다.
    for (const delta of [0, 1, 25, 999, 5000]) { raw = origin + delta; assert.equal(source.milliseconds(), native.milliseconds()); assert.deepEqual({ ...wrapped.captureFrame() }, { ...plain.captureFrame() }); }
});
test('중첩 대기와 원래 사용자 pause는 game/wall을 각각 보존한다', () => {
    let raw = 1000000;
    const native = new ClientTimeSource({ hostMilliseconds: () => raw, hostUnixSeconds: () => 1000 }), source = new ModTimeSource(native), clock = new ClientFrameClock(source, { value: 0 });
    raw += 500; assert.equal(clock.captureFrame().game, .5); source.enter(); raw += 100; clock.pushGamePause(); source.enter(); raw += 200; source.leave(); raw += 300;
    assert.equal(clock.gameSeconds(), .5); assert.equal(clock.wallSeconds(), .5); source.leave(); assert.equal(clock.wallSeconds(), .5);
    raw += 200; assert.equal(clock.gameSeconds(), .5); assert.equal(clock.wallSeconds(), 700 * .001); clock.popGamePause(); raw += 25;
    assert.equal(clock.captureFrame().game, 725 * .001 - (700 * .001 - 500 * .001)); assert.equal(clock.wallSeconds(), 725 * .001);
});
test('대기 중 uint32가 감겨도 시간 제외는 unsigned 경과 한 번이다', () => {
    let raw = 0xfffffff0;
    const source = new ModTimeSource(new ClientTimeSource({ hostMilliseconds: () => raw, hostUnixSeconds: () => 1000 }));
    source.enter(); raw = 0x20; assert.equal(source.milliseconds(), 0xfffffff0); source.leave(); assert.equal(source.milliseconds(), 0xfffffff0); raw += 25; assert.equal(source.milliseconds(), 9); assert.throws(() => source.leave(), /불균형/u);
});
test('고정 시계 판정/프레임 전진/초 초기값은 원래 공급원에 위임한다', () => {
    const native = new ClientTimeSource(), source = new ModTimeSource(native); native.enableFixed({ frameMilliseconds: 25, unixSeconds: 123 });
    assert.equal(source.fixedEnabled, true); source.advanceFixed(); assert.equal(source.milliseconds(), native.milliseconds()); assert.equal(source.unixSeconds(), 123);
});
