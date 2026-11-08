// 객체 표시 부모 raw-squid-display-parent.js(runRawSquidDisplayParent)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: squid-display-parent-x86.tsv 696행(최초 804 관측: 큰 TYPE 번호 getter 대체 장치·첫 로그 인자만)과 squid-display-number-x86.tsv 696행(실제 TYPE 번호 getter·세 오류 인자)이다.
//    cpppj SquidDisplayParent_NativeWholeParent696/NativeNumberAndThreeErrorArguments696 과 같은 입력(checking·프레임·TYPE 네 필드·변이 종류·진단 중단)에서
//    효과 순서·raw 50바이트·두 TYPE 필드·checking·결과(return/assertion)를 비교한다.
//  - 계약: SquidDisplayParent_SelectedDevicesAndStableBorrowedFields(선택하지 않은 장치 비요구·누락 거부·50바이트 슬롯).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E58: 객체 표시 부모 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { runRawSquidDisplayParent, SQUID_DISPLAY_ASSERTION } from '../www/core/raw-squid-display-parent.js';

// 원본 관측 파일 두 개이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E58)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const parentRows = readRows(path.join(FIXTURES, 'squid-display-parent-x86.tsv'));
const numberRows = readRows(path.join(FIXTURES, 'squid-display-number-x86.tsv'));

// 표 한 행은 원문 입력 7개 DWORD(checking·프레임·TYPE0 flags·loaded·header·변이 종류·진단 중단 여부)이다. 쉼표 목록을 부호 없는 DWORD 로 읽는다. (2026-11-08 WB04-E58)
const numbers = (text) => text.split(',').map((part) => Number(part) >>> 0);

// 문자열의 ASCII 바이트를 소문자 hex 로 만든다(cpppj DisplayParentHex). (2026-11-08 WB04-E58)
const asciiHex = (text) => Buffer.from(text, 'latin1').toString('hex');

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E58)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 예외가 나는지 본다. (2026-11-08 WB04-E58)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 한 관측 행을 같은 순서로 재생한다. actualNumber 가 참이면 실제 번호 getter·세 인자 오류 상세 행이다. (2026-11-08 WB04-E58)
function replayRow(row, actualNumber) {
    assert.equal(row.length, 6);
    const input = numbers(row[0]);
    assert.equal(input.length, 7);
    const raw = new Uint8Array(50);
    // 무관한 바이트까지 같은 표식으로 대조한다.
    for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 17 + 3) & 0xff;
    raw[10] = 70;
    raw[40] = 5;
    put(raw, 36, input[1]);
    put(raw, 14, 0x40500000);
    put(raw, 18, 0xc0200000);
    // 두 TYPE 의 네 필드를 같은 객체에 두고 뷰가 그 필드를 호출 순간에 읽게 한다.
    const types = [
        { flags: input[2], count: 4, loaded: input[3] !== 0, header: input[4] | 0 },
        { flags: 0x440000, count: 2, loaded: true, header: 200 }
    ];
    let checking = input[0];
    let lookups = 0;
    let mutated = false;
    let outcome = 'return';
    const events = [];
    const event = (value) => events.push(value);
    // 같은 현재 raw/전역만 바꾸는 명시 변이 장치이다(지역 프레임은 다시 계산되지 않는다).
    const mutate = (kind) => {
        if (!kind || mutated || input[5] !== kind) return;
        mutated = true;
        checking = 0;
        raw[10] = 71;
        put(raw, 36, 2);
        put(raw, 14, 0x40f00000);
        put(raw, 18, 0x3f400000);
        raw[40] = 0;
        if (kind === 3) {
            types[0].count = 1;
            types[0].header = 0xdddddddd | 0;
        }
    };
    const hooks = {
        type(selected) {
            assert.ok(selected === 70 || selected === 71);
            event(`T:${selected}`);
            lookups += 1;
            mutate(lookups === 1 ? 1 : 0);
            if (lookups === (input[0] ? 2 : 1)) mutate(5);
            const record = types[selected - 70];
            return {
                token: selected,
                get flags1() { return record.flags; },
                get frameCount() { return record.count; },
                get loaded() { return record.loaded; },
                get header() { return record.header; }
            };
        },
        typeNumber(view) {
            if (actualNumber) {
                event(`N:${view.token}`);
                mutate(2);
                return view.token;
            }
            const bits = input[1];
            event(`F:${view.token}:${bits}:${raw[40] & 1}`);
            mutate(2);
            return (0x70ad0000 + (view.token - 70) * 0x100 + (bits & 255)) >>> 0;
        },
        frameError(token, frame, abstract) {
            event(`L:${token}${actualNumber ? `:${frame >>> 0}:${abstract}` : ''}`);
            mutate(3);
        },
        assertion(expression, file, line) {
            event(`D:${asciiHex(expression)}:${asciiHex(file)}:${line}`);
            if (!input[6]) throw new Error('명시 진단 중단');
            mutate(4);
        },
        mark(type, frame, x, y, flags) {
            event(`M:${type}:${frame >>> 0}:${x}:${y}:${flags}`);
        }
    };
    try {
        runRawSquidDisplayParent(raw, () => checking, hooks);
    } catch (error) {
        if (error.message !== '명시 진단 중단') throw error;
        outcome = 'assertion';
    }
    // 두 현재 TYPE 의 네 필드를 원문 순서(header·flags·count·loaded)로 직렬화한다.
    const fields = types.flatMap((type) => [type.header >>> 0, type.flags >>> 0, type.count >>> 0, type.loaded ? 1 : 0]).join(',');
    assert.equal(events.join(';'), row[1]);
    assert.equal(hex(raw), row[2]);
    assert.equal(fields, row[3]);
    assert.equal(checking >>> 0, Number(row[4]));
    assert.equal(outcome, row[5]);
}

test('SquidDisplayParent: 원본 804 관측 696행(번호 getter 대체 장치·첫 로그 인자)의 효과·raw·TYPE 필드·결과가 같다', { skip: !parentRows && '원본 fixture 없음' }, () => {
    assert.equal(parentRows.length, 696);
    // 모든 관측 행을 순서대로 재생한다.
    for (const row of parentRows) replayRow(row, false);
});

test('SquidDisplayParent: 실제 번호 getter와 세 오류 인자 696행도 같다', { skip: !numberRows && '원본 fixture 없음' }, () => {
    assert.equal(numberRows.length, 696);
    // 모든 관측 행을 순서대로 재생한다.
    for (const row of numberRows) replayRow(row, true);
});

test('SquidDisplayParent: 진단 상수는 원문 846 식·파일·줄이다', () => {
    assert.deepEqual({ ...SQUID_DISPLAY_ASSERTION }, { expression: 'getTypeStruct()->frameCheck( isAbstract(), theFrame, true )', file: '\\Ns\\O\\Squid.cpp', line: 0x846 });
});

test('SquidDisplayParent: 선택하지 않은 장치는 요구하지 않고 필요한 장치 누락과 잘못된 슬롯은 거부한다', () => {
    const raw = new Uint8Array(50);
    raw[10] = 70;
    put(raw, 36, 0xffffffff);
    const type = { flags: 0, count: 4, loaded: true, header: 100 };
    let checking = 0;
    let calls = 0;
    const hooks = {
        type(number) {
            assert.equal(number, 70);
            calls += 1;
            return { token: 70, get flags1() { return type.flags; }, get frameCount() { return type.count; }, get loaded() { return type.loaded; }, get header() { return type.header; } };
        }
    };
    runRawSquidDisplayParent(raw, () => checking, hooks);
    assert.equal(calls, 1);
    checking = 1;
    assert.ok(throws(() => runRawSquidDisplayParent(raw, () => checking, hooks)), 'TYPE 번호 getter 누락에서 중단');
    put(raw, 36, 0);
    type.loaded = false;
    checking = 0;
    runRawSquidDisplayParent(raw, () => checking, hooks);
    checking = 1;
    assert.ok(throws(() => runRawSquidDisplayParent(raw, () => checking, hooks)), 'SHP 만 없으면 assertion 요구');
    type.loaded = true;
    checking = 0;
    assert.ok(throws(() => runRawSquidDisplayParent(raw, () => checking, hooks)), '정상 프레임은 mark 요구');
    assert.ok(throws(() => runRawSquidDisplayParent(raw.subarray(0, 49), () => checking, hooks)));
    assert.ok(throws(() => runRawSquidDisplayParent(raw, 0, hooks)));
});
