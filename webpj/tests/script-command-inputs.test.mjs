// 대본 일반 입력 분배(DestroyAt·AddRandom·MakeAi)를 원본 10.78 관측으로 대조한다(WB04-E21, cpppj DestroyAtTests·AddRandomCommandTests·MakeAiCommandInputTests 이식).
//  - destroy-at-input-x86.tsv 48행, add-random-input-x86.tsv 288행: 모드/뗌/전체 y 전달과 signed 255 비교·타입 조회 결과 전달. make-ai-input-x86.tsv 64행: Editor OR missionPost 만 관문으로 쓰는 호출과 전체 DWORD 인자.
//  - 계약: 정확한 명령 코드, 도달한 장치만 요구, Editor 비0 뒤 mission 조회 금지, 실패는 한 번 전파.
// cpppj 의 스크립트 큐(QueueScriptCommand)·GameMenuScriptCode 이름 해석·부모(DestroyObjectsAt, AddRandomObjects) 검사는 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E21: 대본 일반 입력 분배 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { InputCode } from '../www/core/input-queue.js';
import { DESTROY_AT_COMMAND, ADD_RANDOM_COMMAND, MAKE_AI_COMMAND, dispatchDestroyAtInput, dispatchAddRandomInput, dispatchMakeAiCommandInput, makeAiCommandScriptCode } from '../www/core/script-command-inputs.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E21)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const destroyAll = fixture('destroy-at-input-x86.tsv');
const addAll = fixture('add-random-input-x86.tsv');
const makeRows = fixture('make-ai-input-x86.tsv');
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const signed = (value) => value | 0;

// 입력의 모드/뗌·전체 y 전달을 원문 48입력에 대조한다.
test('DestroyAt 입력: 원본10.78 48행(모드·뗌·전체 y 전달)', { skip: !destroyAll && '원본 fixture 없음' }, () => {
    const rows = destroyAll.filter((row) => /^\d+$/.test(row[0]));
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 6);
        const mode = number(row[2]);
        const trace = [];
        const consumed = dispatchDestroyAtInput({ code: number(row[0]), x: 305419896, y: signed(number(row[1])) }, (mode & 1) !== 0, (mode & 2) !== 0, (token) => trace.push(`destroyAt:${token}`));
        assert.ok(consumed);
        assert.equal(trace.join(';') || '-', row[3], `행 ${count} ${row.join(',')}`);
        assert.ok(row[4] === '1' && row[5] === '0');
        count += 1;
    }
    assert.equal(count, 48);
});

// 실제로 도달하지 않은 하위는 요구하지 않으며 선택된 부모의 누락은 실패한다.
test('DestroyAt 입력: 정확한 코드와 도달한 장치만 요구', () => {
    assert.equal(dispatchDestroyAtInput({ code: 0x02000017, x: 0, y: 0 }, true, true, null), false);
    assert.equal(dispatchDestroyAtInput({ code: (DESTROY_AT_COMMAND | InputCode.release) >>> 0, x: 0, y: 0 }, true, true, null), true);
    assert.equal(dispatchDestroyAtInput({ code: DESTROY_AT_COMMAND, x: 0, y: 0 }, false, false, null), true);
    assert.throws(() => dispatchDestroyAtInput({ code: DESTROY_AT_COMMAND, x: 0, y: 0 }, true, false, null), /DestroyAt 의 실제 현재 부모 미연결/);
});

// 보호 원문의 signed 255 비교·현재 모드·뗌·조회 실패 결과 전달을 대조한다.
test('AddRandom 입력: 원본10.78 288행(signed 255 비교·모드·뗌·조회 결과 전달)', { skip: !addAll && '원본 fixture 없음' }, () => {
    const rows = addAll.filter((row) => /^\d+$/.test(row[0]));
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 7);
        const mode = number(row[2]);
        const lookupResult = signed(number(row[3]));
        const trace = [];
        const consumed = dispatchAddRandomInput({ code: number(row[0]), x: 305419896, y: signed(number(row[1])) }, (mode & 1) !== 0, (mode & 2) !== 0,
            (token) => { trace.push(`lookup:${token}`); return lookupResult; },
            (type, quantity) => trace.push(`add:${type}:${quantity}`));
        assert.ok(consumed);
        assert.equal(trace.join(';') || '-', row[4], `행 ${count} ${row.join(',')}`);
        assert.ok(row[5] === '1' && row[6] === '0');
        count += 1;
    }
    assert.equal(count, 288);
});

// 도달하지 않은 조회/배치 장치는 요구하지 않는다.
test('AddRandom 입력: 정확한 코드와 도달한 장치만 요구', () => {
    assert.equal(dispatchAddRandomInput({ code: 0x02000017, x: 0, y: 100 }, false, false, null, null), false);
    assert.equal(dispatchAddRandomInput({ code: (ADD_RANDOM_COMMAND | InputCode.release) >>> 0, x: 0, y: 100 }, true, true, null, null), true);
    assert.equal(dispatchAddRandomInput({ code: ADD_RANDOM_COMMAND, x: 0, y: 100 }, false, false, null, null), true);
    assert.throws(() => dispatchAddRandomInput({ code: ADD_RANDOM_COMMAND, x: 0, y: 100 }, true, false, null, null), /AddRandom 배치/);
    assert.throws(() => dispatchAddRandomInput({ code: ADD_RANDOM_COMMAND, x: 0, y: 300 }, true, false, null, () => {}), /타입 이름 조회/);
});

// 원문 Editor OR missionPost 만 관문으로 쓰며 정지/World 관문을 추가하지 않는다.
test('MakeAi 입력: 원본10.78 64행(모드 마스크·뗌·전체 DWORD·지연 읽기)', { skip: !makeRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of makeRows) {
        assert.equal(row.length, 6);
        let trace = '';
        const hooks = { editor: () => number(row[2]), missionPost: () => number(row[3]), makeAi: (owner) => { trace = `Make:${owner}`; } };
        assert.ok(dispatchMakeAiCommandInput({ code: number(row[0]), x: 37, y: signed(number(row[1])) }, hooks));
        assert.equal(trace || '-', row[4], `행 ${count} ${row.join(',')}`);
        count += 1;
    }
    assert.equal(count, 64);
    assert.equal(dispatchMakeAiCommandInput({ code: 0x4d, x: 0, y: 0 }, {}), false);
    assert.equal(dispatchMakeAiCommandInput({ code: (MAKE_AI_COMMAND | InputCode.release) >>> 0, x: 0, y: 0 }, {}), true);
});

// Editor 비0 뒤에는 mission 을 읽지 않고, 부모 실패는 한 번만 전파하며, 이름은 ASCII 대소문자와 첫 NUL 앞만 비교한다.
test('MakeAi 입력: 단락·실패 전파·이름표', () => {
    let calls = 0;
    const hooks = { editor: () => 1, missionPost: () => { throw new Error('Editor 비0 뒤 mission 조회 금지'); }, makeAi: (owner) => { assert.equal(owner, 0xffffffff); calls += 1; throw new Error('명시 현재 MakeAi 실패'); } };
    assert.throws(() => dispatchMakeAiCommandInput({ code: MAKE_AI_COMMAND, x: 9, y: -1 }, hooks), /명시 현재 MakeAi 실패/);
    assert.equal(calls, 1);
    assert.equal(makeAiCommandScriptCode('MakeAi\0ignored'), MAKE_AI_COMMAND);
    assert.equal(makeAiCommandScriptCode('mAkEaI'), MAKE_AI_COMMAND);
    assert.equal(makeAiCommandScriptCode('MakeAis'), 0xffffffff);
});
