// 선택 체력 막대 부모 sprite-hit-point-bar.js(readSpriteHitPointBar)를 원본 10.78 내부 부모 관측 252행과 계약으로 대조한다.
//  - sprite-hit-point-bar-x86.tsv 의 originals 252행: 입력(현재 HP·최대 HP 세 번의 getter 결과·getter 안에서 HP 를 바꾸는지·색을 바꾸는지)으로 실행한 결과 (색,현재,최대)와 읽기 추적(H=현재 HP, M=최대 HP getter), 끝 HP, 최대 HP getter 호출 수가 같다.
//    HP 존재/기하/픽셀은 이 부모의 범위 밖이며 최대 HP 자식, HP·색 변경은 명시 장치 입력이다. originalCD 행(clamp 경로)은 10.72 라 사용하지 않는다.
//  - 계약: 임계 경계(정확히 절반/1/4 포함), signed 32비트 정수 나눗셈의 0 방향 절삭, 색 배열을 호출 사이에 다시 읽음, 미선택 읽기 장치 비요구, 장치 누락 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E377: 5-A SpriteHitPointBar 이식과 함께 추가. 실제 SpriteDraw 선택 분기 시험은 SpriteDraw 이식 때 옮긴다)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { readSpriteHitPointBar } from '../www/core/sprite-hit-point-bar.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E377)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'sprite-hit-point-bar-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

test('SpriteHitPointBar: 원본 10.78 252행의 색·현재/최대 HP·읽기 추적·끝 HP·최대 HP getter 호출 수가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 동결 관측을 읽고 기존 구현으로 기대값을 만들지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const input = row[1].split(',');
        assert.equal(input.length, 6);
        let hp = Number(input[0]) | 0;
        const maxima = [Number(input[1]), Number(input[2]), Number(input[3])];
        const mutate = Number(input[4]) !== 0;
        const recolor = Number(input[5]) !== 0;
        const colors = [11, 22, 33];
        let calls = 0;
        const trace = [];
        const hooks = {
            current: () => { trace.push(`H${hp}`); return hp; },
            maximum: () => {
                assert.ok(calls < 3);
                const value = maxima[calls];
                calls += 1;
                trace.push(`M${value}`);
                if (mutate) hp = [-5, 17, 200][calls - 1];
                if (recolor) { colors[0] = 100 + calls; colors[1] = 200 + calls; colors[2] = 300 + calls; }
                return value;
            },
        };
        const result = readSpriteHitPointBar(colors, hooks);
        const where = `행 ${count} (${row[1]})`;
        assert.equal(`${result.color},${result.current},${result.maximum}`, row[2], `${where} 결과`);
        assert.equal(trace.join(';'), row[3], `${where} 추적`);
        assert.equal(hp, Number(row[4]), `${where} 끝 HP`);
        assert.equal(calls, Number(row[5]), `${where} 호출 수`);
        count += 1;
    }
    assert.equal(count, 252);
});

// 값을 차례로 돌려주는 HP/최대 HP 장치를 만든다(각 목록은 마지막 값을 반복). (2026-11-13 WB04-E377)
function devices(currents, maxima) {
    const log = [];
    let ci = 0;
    let mi = 0;
    return {
        log,
        hooks: {
            current: () => { log.push('H'); const value = currents[Math.min(ci, currents.length - 1)]; ci += 1; return value; },
            maximum: () => { log.push('M'); const value = maxima[Math.min(mi, maxima.length - 1)]; mi += 1; return value; },
        },
    };
}

test('SpriteHitPointBar: 절반/1/4 임계는 이하(포함)이고 나눗셈은 0 방향 절삭이며 읽기는 항상 H M H M H M 순서이다', () => {
    const colors = [11, 22, 33];
    const run = (hp, max) => { const d = devices([hp], [max]); const r = readSpriteHitPointBar(colors, d.hooks); assert.equal(d.log.join(''), 'HMHMHM'); return r; };
    // 최대 100: 51 정상, 50 절반 이하, 26 절반, 25 1/4 이하.
    assert.deepEqual(run(51, 100), { color: 11, current: 51, maximum: 100 });
    assert.deepEqual(run(50, 100), { color: 22, current: 50, maximum: 100 });
    assert.deepEqual(run(26, 100), { color: 22, current: 26, maximum: 100 });
    assert.deepEqual(run(25, 100), { color: 33, current: 25, maximum: 100 });
    // 홀수 최대 101: 절반 50(절삭), 1/4 25(절삭).
    assert.equal(run(51, 101).color, 11);
    assert.equal(run(50, 101).color, 22);
    assert.equal(run(26, 101).color, 22);
    assert.equal(run(25, 101).color, 33);
    // 음수 최대는 0 방향 절삭이다: -5/2 = -2, -5/4 = -1 (바닥 나눗셈이었다면 -3, -2 라 아래 두 번째 값은 정상 색이 된다).
    assert.equal(run(-2, -5).color, 33);
    assert.equal(run(-1, -5).color, 33);
    assert.equal(run(0, -5).color, 11);
    assert.equal(run(-1, -4).color, 33);
    assert.equal(run(-1, -3).color, 33);
    // 극단 int32 값(최소 최대 HP)도 감기지 않는다.
    assert.deepEqual(run(-2147483648, -2147483648), { color: 33, current: -2147483648, maximum: -2147483648 });
    assert.deepEqual(run(2147483647, 2147483647), { color: 11, current: 2147483647, maximum: 2147483647 });
});

test('SpriteHitPointBar: 호출 사이에 값과 색을 다시 읽고(캐시하지 않음) 장치 누락은 도달 시점에 거부한다', () => {
    // 첫 쌍은 낮은 HP, 둘째 쌍은 높은 HP: 첫 비교만 절반 색, 둘째 비교는 정상이라 colors[1] 유지, 마지막 쌍은 새 값.
    const colors = [11, 22, 33];
    const d = devices([10, 90, 60], [100]);
    assert.deepEqual(readSpriteHitPointBar(colors, d.hooks), { color: 22, current: 60, maximum: 100 });
    // 둘째 비교가 1/4 이하이면 마지막 색이 이긴다.
    assert.equal(readSpriteHitPointBar([1, 2, 3], devices([30, 10, 99], [100]).hooks).color, 3);
    // 색 배열은 진단 복귀 뒤 현재 값을 읽는다: 최대 장치가 colors 를 바꾸면 그 새 값이 쓰인다.
    const live = [1, 2, 3];
    const result = readSpriteHitPointBar(live, { current: () => 0, maximum: () => { live[1] = 77; live[2] = 88; return 100; } });
    assert.equal(result.color, 88);
    // 도달한 읽기 장치가 없으면 그 시점에서 오류이고 앞 장치의 부분 효과는 남는다.
    const order = [];
    assert.throws(() => readSpriteHitPointBar(colors, { current: () => { order.push('H'); return 1; } }), /조회 장치 미연결/u);
    assert.deepEqual(order, ['H']);
    assert.throws(() => readSpriteHitPointBar(colors, {}), /조회 장치 미연결/u);
});
