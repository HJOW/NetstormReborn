// 선택 키 부모(H/N/P/R/F4/F5/F6)·AI/간헐천 일반 명령·편집 객체 취득을 원본 10.78 관측으로 대조한다(WB04-E23, cpppj SelectionKeyInputTests·AiUtilityCommandInputTests·CursorObjectPickupTests 이식).
//  - selection-key-input-x86.tsv 480행: 정지/Shift/Control/Alt, 대소문자, F4/F5의 모든 분기와 큐 예약 인자. ai-utility-commands-x86.tsv 72행: 선택 call 과 param2 전체 DWORD.
//  - cursor-object-pickup-x86.tsv 96행: 원문 TYPE/WORD/flags/Unpop 변경/생성 0 의 호출 순서·인자 폭·최종 슬롯.
// 메뉴 명령 주소→값 표는 cpppj GameMenuCommands 의 해당 세 항목만 시험 장면에 둔다(전체 표는 메뉴 단위). 이름→명령 해석(스크립트 큐)과 실제 선택/메뉴 연결은 해당 단위를 옮길 때 대조한다.
// (2026-11-08 WB04-E23: 선택 키·AI 일반 명령·편집 객체 취득 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { InputCode } from '../www/core/input-queue.js';
import { SidPool } from '../www/core/sid-pool.js';
import { dispatchSelectionKeyInput, dispatchAiUtilityCommandInput, aiUtilityCommandScriptCode, pickupObjectForCursor, CREATE_INITIAL_GEYSERS_COMMAND, BRAIN_CRANK_MONEY_COMMAND, AI_PARAM_SET_COMMAND } from '../www/core/client-input-commands.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E23)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const selectionRows = fixture('selection-key-input-x86.tsv');
const aiRows = fixture('ai-utility-commands-x86.tsv');
const pickupRows = fixture('cursor-object-pickup-x86.tsv');
const number = (text) => Number(BigInt(text) & 0xffffffffn);

// 메뉴 명령 주소→값 표의 세 항목이다(cpppj GameMenuCommands.cpp). (2026-11-08 WB04-E23)
const MENU_COMMANDS = new Map([[0x00542974, 0x020000a0], [0x005429ac, 0x020000ae], [0x005429d4, 0x020000b8]]);

// 정지/Shift/Control/Alt 와 대소문자·F4/F5 의 모든 원문 480행을 방문한다.
test('선택 키 부모: 원본10.78 480행(정지·수정키·대소문자·F4/F5/F6)', { skip: !selectionRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const fields of selectionRows) {
        assert.equal(fields.length, 2);
        const row = fields[0].split(',').map(number);
        assert.equal(row.length, 3);
        const trace = [];
        const event = (name, args = []) => trace.push(name + args.map((value) => `:${value >>> 0}`).join(''));
        const hooks = {
            paused: () => { event('P', [row[1]]); return row[1] !== 0; },
            command: (address) => MENU_COMMANDS.get(address),
            localOwner: () => row[2],
            queue: (code, first, second) => event('Q', [code, first, second]),
            next: (add) => event('N', [add]),
            priest: () => event('R'),
        };
        assert.ok(dispatchSelectionKeyInput({ code: row[0], x: 37, y: -19 }, hooks));
        assert.equal(trace.join(';'), fields[1], `행 ${count} ${fields[0]}`);
        count += 1;
    }
    assert.equal(count, 480);
});

// 뗌/명령 사건은 키로 오인하지 않으며 정지나 F4 수정키 거부는 실제 선택/카메라를 요구하지 않는다.
test('선택 키 부모: 지연 관문과 정확한 코드', () => {
    const hooks = {};
    for (const code of [(InputCode.release | 0x4e) >>> 0, (InputCode.command | 0x48) >>> 0, (InputCode.leftButton | 0x50) >>> 0, 0x56]) assert.equal(dispatchSelectionKeyInput({ code, x: 0, y: 0 }, hooks), false);
    hooks.paused = () => true;
    for (const code of [0x4e, 0x52, 0x740000]) assert.equal(dispatchSelectionKeyInput({ code, x: 0, y: 0 }, hooks), true);
    hooks.paused = () => false;
    assert.equal(dispatchSelectionKeyInput({ code: (0x730000 | InputCode.control) >>> 0, x: 0, y: 0 }, hooks), true);
    assert.equal(dispatchSelectionKeyInput({ code: (0x730000 | InputCode.shift) >>> 0, x: 0, y: 0 }, hooks), true);
    assert.throws(() => dispatchSelectionKeyInput({ code: 0x4e, x: 0, y: 0 }, hooks), /선택 키의 실제 장치 미연결/);
});

// 실제 선택된 call 과 param2 전체 DWORD 를 modifier/outside/release 마다 비교한다.
test('AI/간헐천 일반 명령: 원본10.78 72행(선택된 부모·param2 전체 DWORD·뗌)', { skip: !aiRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of aiRows) {
        assert.equal(row.length, 4);
        let trace = '';
        const hooks = { initialGeysers: () => { trace = 'Geysers'; }, crankMoney: () => { trace = 'Money'; }, parameter: (value) => { trace = `Param:${value}`; } };
        assert.ok(dispatchAiUtilityCommandInput({ code: number(row[0]), x: 37, y: number(row[1]) | 0 }, hooks));
        assert.equal(trace || '-', row[2], `행 ${count} ${row.join(',')}`);
        count += 1;
    }
    assert.equal(count, 72);
});

// 다른 코드는 소비하지 않고 뗌은 장치 없이 소비하며 도달한 부모 누락·실패를 구별한다.
test('AI/간헐천 일반 명령: 소비 범위·누락·실패와 이름표', () => {
    assert.equal(dispatchAiUtilityCommandInput({ code: 0x58, x: 0, y: 0 }, {}), false);
    assert.equal(dispatchAiUtilityCommandInput({ code: (BRAIN_CRANK_MONEY_COMMAND | InputCode.release) >>> 0, x: 0, y: 0 }, {}), true);
    assert.throws(() => dispatchAiUtilityCommandInput({ code: CREATE_INITIAL_GEYSERS_COMMAND, x: 0, y: 0 }, {}), /실제 부모 미연결/);
    let calls = 0;
    assert.throws(() => dispatchAiUtilityCommandInput({ code: AI_PARAM_SET_COMMAND, x: 123, y: 0x80000000 | 0 }, { parameter: (value) => { assert.equal(value, 0x80000000); calls += 1; throw new Error('명시 실제 본체 writer 실패'); } }), /writer 실패/);
    assert.equal(calls, 1);
    assert.deepEqual(['CreateInitialGeysers', 'BRAINCRANKMONEY', 'aiparamset', 'AiParamSets'].map(aiUtilityCommandScriptCode), [CREATE_INITIAL_GEYSERS_COMMAND, BRAIN_CRANK_MONEY_COMMAND, AI_PARAM_SET_COMMAND, 0xffffffff]);
});

// 기대값은 독립 CPU fixture 에서만 읽는다.
test('편집 객체 취득: 원본10.78 96행(TYPE/WORD/flags/Unpop 변경/생성 0)', { skip: !pickupRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of pickupRows) {
        assert.equal(row.length, 7);
        const pool = new SidPool(24000);
        const raw = pool.physical(50);
        raw.fill(0);
        raw[10] = number(row[1]) & 0xff;
        put(raw, 12, number(row[2]), 2);
        const trace = [];
        const hooks = {
            unpop: (sid, flags) => {
                trace.push(`U,${sid},${flags}`);
                if (number(row[3])) { const changed = pool.physical(sid); changed[10] = 120; put(changed, 12, 0x1234, 2); }
            },
            create: (sid, type, argument, flags, attach, quality) => { trace.push(`F,${sid},${type},${argument},${flags},${attach},${quality}`); return number(row[4]); },
            take: (form, flags) => trace.push(`T,${form},${flags}`),
        };
        pickupObjectForCursor(pool, 50, number(row[0]), hooks);
        assert.equal(trace.join(';'), row[5], `행 ${count} 호출`);
        assert.equal(`${pool.slot(50)[10]},${get(pool.slot(50), 12, 2)}`, row[6], `행 ${count} 슬롯`);
        count += 1;
    }
    assert.equal(count, 96);
});

// 누락한 하위에서 앞선 효과를 되돌리거나 무동작 성공을 만들지 않는다.
test('편집 객체 취득: 도달한 하위 누락은 앞선 Unpop 을 보존한다', () => {
    const pool = new SidPool(24000);
    const raw = pool.physical(50);
    raw.fill(0);
    raw[10] = 82;
    put(raw, 12, 7, 2);
    let unpopped = 0;
    let created = 0;
    const hooks = { unpop: () => { unpopped += 1; } };
    assert.throws(() => pickupObjectForCursor(pool, 50, 0, hooks), /하위 미연결/);
    assert.deepEqual([unpopped, created], [1, 0]);
    hooks.create = () => { created += 1; return 0; };
    assert.throws(() => pickupObjectForCursor(pool, 50, 0, hooks), /하위 미연결/);
    assert.deepEqual([unpopped, created], [2, 1]);
    assert.throws(() => pickupObjectForCursor(pool, 0xffffffff, 0, hooks), RangeError);
    assert.deepEqual([unpopped, created], [2, 1]);
});
