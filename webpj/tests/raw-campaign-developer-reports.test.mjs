// 31h 타입·그래프 진단의 현재 기록과 출력 경계를 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { executeDeveloperReports } from '../www/core/raw-campaign-developer-reports.js';

// 현재 타입과 그림 기록 세 개를 만든다.
function core() {
    return { types: [{ name: 'Zero', flags1: 0, zOrder: 0 }, { name: 'One', flags1: 0xffffffff, zOrder: -2 }, { name: 'Two', flags1: 3, zOrder: 4 }], shapes: [{ frameCount: 0 }, { frameCount: 5 }, { frameCount: 6 }] };
}

// 도움말은 별칭과 대소문자를 처리하고 32행을 각각 같은 수준으로 출력한다.
test('개발자 도움말: 원문 출력 순서와 추적 오류 전파', () => {
    const calls = [];
    assert.equal(executeDeveloperReports('? ignored', core(), {}, (...args) => calls.push(args)), true);
    assert.equal(calls.length, 32);
    assert.ok(calls.every(([level, line]) => level === 29 && line.endsWith('\n')));
    assert.equal(calls[0][1], 'setOwner(so) sid newowner - sets the owner of sid via server.\n');
    assert.equal(calls[8][1], 'create(c) type frame - create type of frame at bc=<20,20>.\n');
    assert.equal(calls[31][1], 'getPriest(gp) - sticks a enemy priest next to your priest\n');
    let count = 0;
    assert.throws(() => executeDeveloperReports('HELP', core(), {}, () => { if (++count === 3) throw new Error('추적 실패'); }), /추적 실패/);
    assert.equal(count, 3);
    assert.throws(() => executeDeveloperReports('help', core(), {}), /추적 장치 미연결/);
});

// 전체 타입 출력은 콜백 뒤 현재 다음 행을 읽고 실제 별칭 순서를 유지한다.
test('개발자 타입 진단: 현재 행과 원본 별칭', () => {
    const data = core(), calls = [];
    assert.equal(executeDeveloperReports('dts', data, {}, (level, message) => { calls.push([level, message]); data.types[2].name = 'Changed'; }), true);
    assert.deepEqual(calls, [[29, '01:One, flags=-1, depth=-2 numFrames=5\n'], [29, '02:Changed, flags=3, depth=4 numFrames=6\n']]);
    assert.equal(executeDeveloperReports('dt 1', data, {}, () => {}), false);
    assert.throws(() => executeDeveloperReports('dumptype -1', data, {}, () => {}), /물리 범위/);
});

// 그래프는 signed WORD를 출력하고 미기입·범위 오류를 구별한다.
test('개발자 그래프 진단: 기록과 입력 경계', () => {
    const data = core(), graph = { records: [{ surfaces: 65535, inUse: 2 }] }, calls = [], trace = (...args) => calls.push(args);
    assert.equal(executeDeveloperReports('dg 0', data, graph, trace), true);
    assert.equal(executeDeveloperReports('dg', data, graph, trace), true);
    assert.deepEqual(calls, [[29, 'graph index: 0 numSurface: -1 inUse: 2\n'], [29, 'bad command\n']]);
    assert.throws(() => executeDeveloperReports('dg -1', data, graph, trace), /물리 범위/);
});
