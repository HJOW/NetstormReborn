// 일반 전투 복귀 State6의 원본 배경·지역값·효과 순서를 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { installNativeBattleReturnState, completeNativeBattleReturn } from '../www/core/native-battle-return-state.js';

// 각 검사에 독립 State·전역과 선택 효과를 준비한다.
function scene(outcome = 1, source = 0, phase = 14) {
    const state = { current: new Uint32Array(36), phase: new Uint32Array(36) }; state.current[0] = 6; state.phase[0] = phase;
    const background = new Uint8Array(80).fill(0x3f), calls = [], g = { state, sourceType: { value: source }, outcome: { value: outcome }, background, battle: () => true };
    const hooks = { deleteSavedState: () => calls.push('delete'), requestMusic: (name) => calls.push(['music', name]), sourceBackground: (value) => { calls.push(['source', value]); return null; },
        leaveBattle: () => calls.push('leave'), prefix: () => calls.push('prefix'), installFort: (saved) => { calls.push(['install', { ...saved }]); return 123; } };
    return { state, background, calls, g, hooks };
}

// 좁은 문자열은 처음 NUL 전까지만 확인한다.
function text(bytes) { return String.fromCharCode(...bytes.subarray(0, bytes.indexOf(0))); }

// 지역 결과는 정리 전의 값을 유지하고 출처는 음악 뒤 현재 전역을 읽는다.
test('전투 복귀 State6: 승리·출처 조회·이전 전투 통지 순서', () => {
    const { calls, g, hooks, background } = scene(1, 3);
    hooks.deleteSavedState = () => { calls.push('delete'); g.sourceType.value = 7; };
    assert.equal(installNativeBattleReturnState(g, hooks), 123);
    assert.equal(text(background), 'victory'); assert.equal(g.outcome.value, 0); assert.equal(g.sourceType.value, 7);
    assert.deepEqual(calls, ['delete', ['music', 'fanfare.mus'], 'delete', ['music', 'fanfare.mus'], ['source', 7], 'leave', 'prefix', ['install', { outcome: 1, source: 0 }]]);
});

// 결과 0은 배경 첫 BYTE만 지우고 패배는 삭제·배경·음악을 순서대로 실행한다.
test('전투 복귀 State6: 빈 결과와 패배 배경', () => {
    const blank = scene(0, 0, 3); assert.equal(installNativeBattleReturnState(blank.g, blank.hooks), 123);
    assert.deepEqual([...blank.background.subarray(0, 3)], [0, 0x3f, 0x3f]); assert.deepEqual(blank.calls, ['prefix', ['install', { outcome: 0, source: 0 }]]);
    const defeat = scene(2, 12, 3); assert.equal(installNativeBattleReturnState(defeat.g, defeat.hooks), 123);
    assert.equal(defeat.g.sourceType.value, 0); assert.equal(text(defeat.background), 'defeat');
    assert.deepEqual(defeat.calls, ['delete', ['music', 'defeat.mus'], 'prefix', ['install', { outcome: 2, source: 0 }]]);
});

// 출처 배경을 찾은 경우 지역 출처를 유지하고 callback 뒤 현재 전투 모드를 다시 조회한다.
test('전투 복귀 State6: 출처 배경과 변경된 전투 관문', () => {
    const { calls, g, hooks, background } = scene(2, 3); let battle = true; g.battle = () => battle;
    hooks.sourceBackground = (value) => { calls.push(['source', value]); battle = false; return 'storm'; };
    installNativeBattleReturnState(g, hooks);
    assert.equal(text(background), 'storm'); assert.equal(background[6], 0); assert.equal(background[7], 0x3f);
    assert.deepEqual(calls, ['delete', ['music', 'defeat.mus'], 'delete', ['music', 'fanfare.mus'], ['source', 3], 'prefix', ['install', { outcome: 2, source: 3 }]]);
});

// 선택된 삭제 실패는 이전 첫 BYTE 쓰기만 남기며 후속 음악·결과 초기화는 하지 않는다.
test('전투 복귀 State6: 선택 장치 실패의 부분 상태', () => {
    const { g, hooks, background, calls } = scene(1);
    hooks.deleteSavedState = () => { calls.push('delete'); throw new Error('삭제 실패'); };
    assert.throws(() => installNativeBattleReturnState(g, hooks), /삭제 실패/);
    assert.equal(g.outcome.value, 1); assert.deepEqual([...background.subarray(0, 3)], [0, 0x3f, 0x3f]); assert.deepEqual(calls, ['delete']);
    g.state.current[0] = 3; assert.throws(() => installNativeBattleReturnState(g, hooks), /현재 State6/);
});

// 후반은 결과 0의 옵션과 비0 결과 안내를 구별하고 안내 복귀 뒤 현재 결과를 지운다.
test('전투 복귀 후반: 결과 안내와 외부 옵션 경계', () => {
    const calls = [], outcome = { value: 99 }, hooks = { multiplayerOptions: () => calls.push('options'), tell: (name) => { calls.push(name); outcome.value = 88; } };
    completeNativeBattleReturn({ outcome: 0, source: 0 }, outcome, hooks); assert.equal(outcome.value, 99);
    completeNativeBattleReturn({ outcome: 0xfffffffe, source: 0 }, outcome, hooks); assert.equal(outcome.value, 0);
    outcome.value = 55; completeNativeBattleReturn({ outcome: 1, source: 1 }, outcome, hooks); assert.equal(outcome.value, 0);
    assert.deepEqual(calls, ['options', 'BattleOver-2']);
});
