// 너겟 PostPop(00484f90)과 Gunanim 방향 계산(00467610)을 원본 10.78 관측으로 대조한다(WB04-E16, cpppj NuggetPostPopTests·GunAngleTests 이식).
//  - nugget-postpop-x86.tsv 288행: 최초 flags·기존 예약·두 할당 실패·난수 소비·두 예약과 기본 호출을 호출 인자(float 비트)까지 순서 그대로 대조한다(두 정밀도 열 모두).
//  - gun-angle-x86.tsv 518행: 음수 -4..-1 과 0..512 및 INT_MAX 의 방향. gun-frame-checking-x86.tsv 384행: signed 인덱스와 현재 checking 진단의 복귀/중단.
//  - 계약: 현재 checking 을 읽고 진단 복귀 뒤에도 음수 표 밖 역참조를 명시 범위 실패로 남김.
// (2026-11-08 WB04-E16: 너겟 PostPop·Gunanim 방향 계산 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawNuggetPostPop } from '../www/core/raw-nugget-post-pop.js';
import { patchGunFrameIndex, patchGunGraphFromFrame } from '../www/core/raw-gun-angle.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E16)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const nuggetRows = fixture('nugget-postpop-x86.tsv');
const angleRows = fixture('gun-angle-x86.tsv');
const frameRows = fixture('gun-frame-checking-x86.tsv');
const number = (text) => Number(BigInt(text) & 0xffffffffn);

// 원문 경계의 unsigned DWORD 인자를 같은 순서로 기록한다. (2026-11-08 WB04-E16)
const nuggetEvent = (trace, name, values) => trace.push(`${name}${values.map((value) => `:${value >>> 0}`).join('')}`);

// 기대 분기는 새 독립 PE 288행에서만 읽는다.
test('너겟 PostPop: 원본10.78 288행(조회·할당·난수 소비·두 예약·기본 호출)', { skip: !nuggetRows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000, { server: true });
    let count = 0;
    for (const fields of nuggetRows) {
        assert.equal(fields.length, 3);
        const row = fields[0].split(',').map(number);
        assert.equal(row.length, 4);
        let allocations = 0;
        let randoms = 0;
        const trace = [];
        // 명시 난수 반환 입력이다.
        const values = [[0x3dcccccd, 0x3e4ccccd], [0, 0], [0x3e99999a, 0x3f000000], [0x7fc01234, 0x3e800000]];
        const logic = new RawNuggetPostPop(pool, {
            hasRegular: (sid, event) => { nuggetEvent(trace, 'H', [sid, event]); return row[1] !== 0; },
            allocate: () => { nuggetEvent(trace, 'A', [40]); const result = ((row[2] >>> allocations) & 1) !== 0; allocations += 1; return result; },
            random: (low, high) => { nuggetEvent(trace, 'R', [floatToBits(low), floatToBits(high)]); const result = bitsToFloat(values[row[3]][randoms]); randoms += 1; return result; },
            construct: (sid, event, payload) => nuggetEvent(trace, 'C', [event, sid, floatToBits(payload)]),
            damageable: (sid, flags) => nuggetEvent(trace, 'D', [sid, flags]),
        });
        logic.postPop(50, row[0]);
        assert.equal(logic.pool, pool);
        assert.equal(trace.join(';'), fields[1], `행 ${count} ${fields[0]}`);
        assert.equal(fields[1], fields[2], `행 ${count} 두 정밀도 동일`);
        count += 1;
    }
    assert.equal(count, 288);
});

// 선택한 경계만 요구하고 다른 판본의 풀은 거부한다.
test('너겟 PostPop: 미리보기는 예약 장치를 요구하지 않는다', () => {
    const pool = new SidPool(24000, { server: true });
    const calls = [];
    new RawNuggetPostPop(pool, { damageable: (sid, flags) => calls.push([sid, flags]) }).postPop(50, 4);
    assert.deepEqual(calls, [[50, 4]]);
    assert.throws(() => new RawNuggetPostPop({ edition: 'cd1072' }, {}), TypeError);
    assert.throws(() => new RawNuggetPostPop(pool, {}).postPop(50, 1), /하위 몸체 미연결/);
});

// 음수 -4..-1 과 0..512 및 INT_MAX 는 모두 원본의 유효 표 범위에 도달한다.
test('Gunanim 방향: 원본10.78 518행(음수 -4..-1·0..512·INT_MAX)', { skip: !angleRows && '원본 fixture 없음' }, () => {
    const rows = angleRows.filter((row) => row[0] !== 'frame');
    assert.equal(rows.length, 518);
    for (const row of rows) {
        assert.equal(row.length, 2);
        assert.equal(patchGunGraphFromFrame(Number(row[0]) | 0), number(row[1]), `프레임 ${row[0]}`);
    }
});

// 원문이 진단 뒤 표 밖 메모리를 읽는 음수 인덱스는 정상 방향 0 으로 가장하지 않는다.
test('Gunanim 방향: 음수 인덱스는 거부한다', () => {
    assert.throws(() => patchGunGraphFromFrame(-5), RangeError);
});

// signed 인덱스와 현재 진단의 앞단을 384개 원문 입력/복귀/중단 경계와 대조한다.
test('Gunanim 인덱스: 원본10.78 진단 앞단 384행(복귀·중단·원래 인자 변경)', { skip: !frameRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of frameRows) {
        assert.equal(row.length, 6);
        const input = row[0].split(',');
        assert.equal(input.length, 4);
        let frame = Number(input[0]) | 0;
        let checking = number(input[1]);
        let trace = '';
        let outcome = 'body';
        let index = null;
        // 진단이 원래 인자 저장소를 바꿔도 이미 계산된 지역 인덱스를 유지한다.
        const assertion = (expression, file, line) => {
            assert.deepEqual([expression, file, line], ['0 <= index && index < 20', '\\Ns\\O\\Gunanim.cpp', 0x387]);
            trace = `D:${Buffer.from(expression).toString('hex')}:${Buffer.from(file).toString('hex')}:903`;
            if (!number(input[2])) throw new Error('명시 진단 중단');
            if (number(input[3])) { checking = 0; frame = 95; }
        };
        try {
            index = patchGunFrameIndex(frame, () => checking, assertion);
            trace += `${trace ? ';' : ''}B`;
        } catch (error) {
            if (error.message !== '명시 진단 중단') throw error;
            outcome = 'assertion';
        }
        const label = `행 ${count} ${row[0]}`;
        assert.equal(trace, row[1], `${label} 호출`);
        if (index !== null) assert.equal(index, Number(row[2]) | 0, `${label} 인덱스`);
        assert.equal(checking >>> 0, number(row[3]), `${label} checking`);
        assert.equal(frame >>> 0, number(row[4]), `${label} 프레임`);
        assert.equal(outcome, row[5], `${label} 결과`);
        count += 1;
    }
    assert.equal(count, 384);
});

// 같은 현재 checking 을 읽고 진단 복귀 뒤에도 음수 표 밖 역참조를 명시 범위 실패로 남긴다.
test('Gunanim 인덱스: 현재 checking 과 음수 읽기 경계', () => {
    let checking = 0;
    let diagnostics = 0;
    const assertion = (_expression, _file, line) => { assert.equal(line, 0x387); diagnostics += 1; checking = 0; };
    assert.equal(patchGunFrameIndex(-5, () => checking), -1);
    checking = 0x80000000;
    assert.equal(patchGunFrameIndex(-10, () => checking, assertion), -2);
    assert.deepEqual([diagnostics, checking], [1, 0]);
    checking = 0xffffffff;
    assert.throws(() => patchGunGraphFromFrame(-10, () => checking, assertion), RangeError);
    assert.equal(diagnostics, 2);
    checking = 1;
    assert.throws(() => patchGunFrameIndex(-5, () => checking), /진단 장치 미연결/);
    // 유효 인덱스는 진단 장치 없이 기존 표/방향을 사용한다.
    assert.equal(patchGunGraphFromFrame(-4, () => checking), 0);
    assert.equal(patchGunGraphFromFrame(2147483647, () => checking), 0);
    assert.equal(diagnostics, 2);
});
