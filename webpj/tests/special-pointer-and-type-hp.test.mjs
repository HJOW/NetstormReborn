// 전용 PE fixture가 없는 Ctrl+special 지점 진단(SpecialPointerTrace)과 현재 TYPE HP getter(CurrentTypeHitPoints)를 기준선 소스 계약으로 검증한다. (2026-11-14 WB04-E435)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/조건은 기준선 소스(SpecialPointerTrace.cpp, CurrentTypeHitPoints.cpp)에서 옮겼다. 투영/탐색/진단은 호출 기록 대역이고 TYPE HP 계산은 실제 readRiftHitPoints 다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { traceSpecialPointer } from '../www/ui/special-pointer-trace.js';
import { makeCurrentTypeHitPoints } from '../www/core/current-type-hit-points.js';

// 후보 목록을 순서대로 내놓는 탐색기 대역이다.
function finder(candidates, options = {}) {
    const log = []; let index = 0;
    const hooks = { project: (x, y) => { log.push(['project', x, y]); return [x * 2, y * 2]; }, begin: (point) => { log.push(['begin', point]); index = 0; return candidates[index++] ?? 0; },
        trace: (token) => { log.push(['trace', token]); options.onTrace?.(token); }, next: () => { log.push(['next']); return candidates[index++] ?? 0; } };
    for (const key of options.omit ?? []) delete hooks[key]; return { log, hooks };
}

test('traceSpecialPointer: Ctrl 이 눌렸고 special 이 있을 때만 사건 좌표 역투영→단일 지점 탐색→각 후보에 가상 70→다음 후보 순서다', () => {
    const f = finder([5, 6, 6, 7]); traceSpecialPointer(true, 9, { code: 0, x: 12, y: -3 }, f.hooks);
    assert.deepEqual(f.log, [['project', 12, -3], ['begin', [24, -6]], ['trace', 5], ['next'], ['trace', 6], ['next'], ['trace', 6], ['next'], ['trace', 7], ['next']]);
    // 첫 후보가 0 이면 진단/다음 조회가 없다.
    const none = finder([]); traceSpecialPointer(true, 1, { x: 0, y: 0 }, none.hooks); assert.deepEqual(none.log.map((entry) => entry[0]), ['project', 'begin']);
    // Ctrl 이 없거나 special 이 0 이면 어떤 경계도 부르지 않고 경계가 없어도 거부하지 않는다.
    const idle = finder([1]); traceSpecialPointer(false, 1, { x: 1, y: 1 }, idle.hooks); traceSpecialPointer(true, 0, { x: 1, y: 1 }, idle.hooks); traceSpecialPointer(false, 0, { x: 1, y: 1 }, {}); assert.deepEqual(idle.log, []);
});

test('traceSpecialPointer: 가상 70 이 현재 finder 를 바꿔도 보관한 후보 목록으로 대체하지 않고 다음 조회 결과를 쓴다', () => {
    const candidates = [3, 4, 5]; const f = finder(candidates, { onTrace: (token) => { if (token === 3) candidates.splice(1, 1); } }); traceSpecialPointer(true, 1, { x: 0, y: 0 }, f.hooks);
    assert.deepEqual(f.log.filter((entry) => entry[0] === 'trace'), [['trace', 3], ['trace', 5]]);
    // 도달한 경계만 요구한다: 사건 좌표 투영/탐색/진단/다음 조회가 각각 없으면 그 지점에서 거부한다.
    for (const [omit, count] of [['project', 0], ['begin', 1], ['trace', 2], ['next', 3]]) { const g = finder([1, 2]); const hooks = { ...g.hooks }; delete hooks[omit]; assert.throws(() => traceSpecialPointer(true, 1, { x: 0, y: 0 }, hooks), (error) => error.constructor === Error && /실제 하위 미연결/.test(error.message), omit); assert.equal(g.log.length, count, omit); }
});

// TYPE 레코드(flags1/maxHitPoints)와 전역(checking/quarter/priest/dais)을 가진 현재 표다.
const record = (maxHitPoints, flags1 = 0x10) => ({ flags1, maxHitPoints });
const globalsOf = (values = {}) => ({ checking: () => values.checking ?? 0, quarter: () => values.quarter ?? 0, priest: () => values.priest ?? 0, dais: () => values.dais ?? 0 });

test('makeCurrentTypeHitPoints: 번호의 TYPE 최대 HP 를 반환하고 사제 TYPE 이고 1/4 HP 전역이면 0 방향 절삭한 1/4 이다', () => {
    const types = [record(100), record(40), record(-7)]; const hp = makeCurrentTypeHitPoints(() => types, globalsOf(), () => assert.fail('진단 없음'));
    assert.deepEqual([hp(0), hp(1), hp(2)], [100, 40, -7]);
    // 1/4 HP: 사제 번호(1)만 1/4(40/4=10), 음수는 0 방향(-7/4 → -1). 다른 번호는 그대로다.
    const quarter = makeCurrentTypeHitPoints(() => types, globalsOf({ quarter: 1, priest: 1 }), () => assert.fail('진단 없음')); assert.deepEqual([quarter(0), quarter(1)], [100, 10]);
    const negative = makeCurrentTypeHitPoints(() => types, globalsOf({ quarter: 1, priest: 2 }), () => {}); assert.equal(negative(2), -1); assert.equal(Object.is(negative(2), -0), false);
    assert.throws(() => hp(3), (error) => error instanceof RangeError && /번호 범위/.test(error.message)); assert.throws(() => hp(0x100000000 + 3), /번호 범위/);
});

test('makeCurrentTypeHitPoints: 진단이 켜져 있고 HP 플래그가 없으면 getNum 을 다이스 TYPE 과 비교해 다를 때만 진단하고 현재 표 기준 번호를 쓴다', () => {
    const noHp = record(10, 0); const types = [record(1), noHp]; const diagnoses = []; const hp = makeCurrentTypeHitPoints(() => types, globalsOf({ checking: 1, dais: 1 }), (expression, file, line) => diagnoses.push([line]));
    assert.equal(hp(1), 10); assert.deepEqual(diagnoses, []); const other = makeCurrentTypeHitPoints(() => types, globalsOf({ checking: 1, dais: 5 }), (expression, file, line) => diagnoses.push([line])); assert.equal(other(1), 10); assert.deepEqual(diagnoses, [[0x1e3]]);
    // 선택 표와 현재 표가 달라도 같은 레코드의 현재 표 번호를 getNum 으로 쓴다(현재 표는 호출마다 다시 읽는다).
    let current = [record(1), record(2), noHp]; const lookups = []; const moved = makeCurrentTypeHitPoints(() => { lookups.push('lookup'); return current; }, globalsOf({ checking: 1, dais: 2 }), () => diagnoses.push(['moved']));
    assert.equal(moved(2), 10); assert.deepEqual(diagnoses, [[0x1e3]]); assert.equal(lookups.length >= 2, true);
});

test('makeCurrentTypeHitPoints: 현재 표 조회가 없으면 사용 시점에 거부하고 현재 표에 레코드가 없으면 서로 다른 물리 표로 거부한다', () => {
    const hp = makeCurrentTypeHitPoints(undefined, globalsOf(), () => {}); assert.throws(() => hp(0), (error) => error.constructor === Error && /실제 표 조회 미연결/.test(error.message));
    const selected = record(5, 0); let tables = [[selected], [record(5, 0)]]; let call = 0; const split = makeCurrentTypeHitPoints(() => tables[Math.min(call++, 1)], globalsOf({ checking: 1, dais: 0 }), () => {}); assert.throws(() => split(0), /서로 다른 물리 표 연결/);
    // 조회는 만든 시점이 아니라 호출마다 한다.
    let table = [record(1)]; const live = makeCurrentTypeHitPoints(() => table, globalsOf(), () => {}); assert.equal(live(0), 1); table = [record(2)]; assert.equal(live(0), 2);
});
