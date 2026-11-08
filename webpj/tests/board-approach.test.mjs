// 건설 접근점 raw-board-approach.js(RawBoardApproach)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: board-approach-x86.tsv 696행(두 정밀도에서 동일) — 입력 9 DWORD(사각형 네 float·출발점 x/y·flags·Graph 응답 모드 0..5·checking)
//    → 종료 뒤 사각형 네 float 의 전체 비트(복구 또는 flags 4 확장 유지), 반환 점의 두 float 비트, 외부 효과 순서(G:x:y:Graph 번호 — 모든 Graph 조회 / A:565 — 가장자리 진단).
//    Graph 응답은 입력 스크립트이며 좌표 선택/순회/재귀 자체는 모듈이 실행한다.
//  - 계약: 유한하지 않은 좌표·거꾸로 된 사각형·Graph 조회 미연결을 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E104: 건설 접근점 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits } from './raw-support.mjs';
import { RawBoardApproach } from '../www/core/raw-board-approach.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E104)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'board-approach-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E104)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const bits = (value) => floatToBits(value);

test('BoardApproach: 원본 10.78 696입력의 Graph 조회 순서·점·사각형 복구/확장 비트가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 두 정밀도에서 동일했던 696개 전체 입력을 순서대로 대조한다.
    for (const row of rows) {
        assert.equal(row.length, 4);
        const input = row[0].split(',').map(Number);
        assert.equal(input.length, 9);
        const rectangle = [bitsToFloat(input[0]), bitsToFloat(input[1]), bitsToFloat(input[2]), bitsToFloat(input[3])];
        const original = [...rectangle];
        const x = bitsToFloat(input[4]);
        const y = bitsToFloat(input[5]);
        const mode = input[7];
        let calls = 0;
        const events = [];
        const approach = new RawBoardApproach((px, py) => {
            const xb = bits(px);
            const yb = bits(py);
            let value;
            if (mode === 0) value = 1;
            else if (mode === 1) value = xb === input[4] && yb === input[5] ? 1 : 2;
            else if (mode === 2) value = (Math.trunc(px) + Math.trunc(py)) % 3 === 0 ? 1 : 2;
            else if (mode === 3) value = !calls || calls % 5 === 0 ? 1 : 2;
            else if (mode === 4) value = 254;
            else value = (xb === input[4] && yb === input[5]) || (Math.trunc(px) === Math.trunc(original[0]) && Math.trunc(py) === Math.trunc(original[3])) ? 1 : 2;
            events.push(`G:${xb}:${yb}:${value}`);
            calls += 1;
            return value;
        }, () => events.push('A:565'));
        const result = approach.find(rectangle, x, y, input[6], input[8] !== 0);
        const expected = row[1].split(',').map(Number);
        const label = `행 ${count}: 입력 ${row[0]}`;
        // 사각형의 복구 또는 flags 4 확장 유지도 원본의 전체 DWORD 와 같아야 한다.
        for (let index = 0; index < 4; index += 1) assert.equal(bits(rectangle[index]), expected[index], label);
        assert.equal(`${bits(result.x)},${bits(result.y)}`, row[2], label);
        assert.equal(events.join(';'), row[3], label);
        count += 1;
    }
    assert.equal(count, 696);
});

test('BoardApproach: 유한하지 않은 좌표·거꾸로 된 사각형·Graph 조회 미연결은 거부한다', () => {
    const approach = new RawBoardApproach(() => 1);
    assert.ok(throws(() => approach.find([1, 2, 5, 6], Number.NaN, 3, 0)));
    assert.ok(throws(() => approach.find([1, 2, 5, 6], 3, Infinity, 0)));
    assert.ok(throws(() => approach.find([5, 2, 1, 6], 3, 3, 0)));
    assert.ok(throws(() => approach.find([1, 6, 5, 2], 3, 3, 0)));
    assert.ok(throws(() => new RawBoardApproach(null)));
    // 같은 Graph 응답이면 첫 점에서 바로 끝나고 사각형은 복구된다.
    const rectangle = [1, 2, 5, 6];
    const point = approach.find(rectangle, 3, 3, 4);
    assert.deepEqual(rectangle, [1, 2, 5, 6]);
    assert.ok(Number.isFinite(point.x) && Number.isFinite(point.y));
});
