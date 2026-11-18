// 31h 다리 붕괴 주기 명령의 입력·출력과 현재 상태 재조회를 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { executeDeveloperBridge } from '../www/core/raw-campaign-developer-bridge.js';

// 현재 주기와 시계를 외부에서 바꿀 수 있는 독립 저장소다.
function storage() { return { bridgeDecayPeriod: 10, bridgeDecayScan: { first: 1, last: 2, cursor: 3, next: 4 }, readGraphClock: () => 12.25 }; }

// 정수·소수·지수·16진 접두와 단정도 출력은 같은 스캔에 반영된다.
test('개발자 다리 주기: 별칭·단정도·원문 스캔 초기화', () => {
    const state = storage(), calls = [], trace = (...args) => calls.push(args);
    assert.equal(executeDeveloperBridge('sbkp 1.2 trailing', state, trace), true);
    assert.equal(state.bridgeDecayPeriod, Math.fround(1.2));
    assert.deepEqual(state.bridgeDecayScan, { first: 15000, last: 23001, cursor: 15000, next: 12.25 + Math.fround(1.2) });
    assert.deepEqual(calls.pop(), [29, 'bkPERIOD now 1.200000\nresetting bridge killer\n']);
    executeDeveloperBridge('SETBRIDGEKILLPERIOD 0x1.8p+1', state, trace);
    assert.equal(state.bridgeDecayPeriod, 3); assert.equal(state.bridgeDecayScan.next, 15.25);
    executeDeveloperBridge('sbkp -0x0p9999', state, trace); assert.ok(Object.is(state.bridgeDecayPeriod, -0));
    executeDeveloperBridge('sbkp -0', state, trace); assert.ok(Object.is(state.bridgeDecayPeriod, -0));
    assert.deepEqual(calls.pop(), [29, 'bkPERIOD now -0.000000\nresetting bridge killer\n']);
    executeDeveloperBridge('sbkp 0.0078125', state, trace);
    assert.deepEqual(calls.pop(), [29, 'bkPERIOD now 0.007813\nresetting bridge killer\n']);
    executeDeveloperBridge('sbkp -nan', state, trace); assert.ok(Number.isNaN(state.bridgeDecayPeriod));
    assert.deepEqual(calls.pop(), [29, 'bkPERIOD now -nan\nresetting bridge killer\n']);
    executeDeveloperBridge('sbkp inf', state, trace); assert.equal(state.bridgeDecayPeriod, Infinity);
    assert.deepEqual(calls.pop(), [29, 'bkPERIOD now inf\nresetting bridge killer\n']);
    assert.equal(executeDeveloperBridge('other 2', state, trace), false);
});

// 추적이 재진입해 현재 주기·시계를 바꾸면 초기화는 복귀 뒤의 값을 읽는다.
test('개발자 다리 주기: 추적 순서·실패 접두·현재 시각', () => {
    const state = storage();
    assert.throws(() => executeDeveloperBridge('sbkp missing', state, () => {}), /미정 입력/);
    assert.throws(() => executeDeveloperBridge('sbkp 1e+', state, () => {}), /미정 입력/);
    assert.throws(() => executeDeveloperBridge('sbkp 0x', state, () => {}), /미정 입력/);
    assert.throws(() => executeDeveloperBridge('sbkp nan(foo', state, () => {}), /미정 입력/);
    assert.equal(state.bridgeDecayPeriod, 10); assert.equal(state.bridgeDecayScan.cursor, 3);
    assert.throws(() => executeDeveloperBridge('sbkp 2', state, () => { throw new Error('추적 실패'); }), /추적 실패/);
    assert.equal(state.bridgeDecayPeriod, 2); assert.equal(state.bridgeDecayScan.cursor, 3);
    executeDeveloperBridge('sbkp 4', state, () => { state.bridgeDecayPeriod = 7; state.readGraphClock = () => 20.5; });
    assert.deepEqual(state.bridgeDecayScan, { first: 15000, last: 23001, cursor: 15000, next: 27.5 });
});
