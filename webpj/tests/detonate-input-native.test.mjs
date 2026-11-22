// DetonateBomb 일반 입력 분배(dispatchDetonateInput, 원문 4d6eca..4da477)를 원본 10.78 x86 관측 768행과 대조한다. (2026-11-22 WB04-E1199)
// 관측 입력은 (객체 칸 주소 16종, 사건 코드 4종, 대상 y 4종, 변형 3종)이고 열은 `입력 / 객체 호출 흔적 / 호출 뒤 객체 50바이트 / 팝업 경계 호출 수`이다.
// 흔적이 `object:대상:사건코드;return:0` 이면 가상 6c 가 한 번 불렸고 `-` 이면 불리지 않았다. 웹 분배기는 객체 칸·변형과 무관하게 코드(수정키 제거)·뗌·y 낮은 WORD 만 본다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { DETONATE_BOMB_COMMAND, dispatchDetonateInput } from '../www/core/detonate-input.js';

// 관측 파일(주석 `#` 줄 제외)을 열 배열로 읽는다. 파일이 없으면 null 이다(원본 fixture 가 없는 환경에서는 건너뛴다).
const file = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'detonate-input-x86.tsv');
const rows = fs.existsSync(file) ? fs.readFileSync(file, 'utf8').split(/\r?\n/u).filter((line) => line && !line.startsWith('#')).map((line) => line.split('\t')) : null;

test('DetonateBomb 입력 분배: 원본 768행의 객체 호출 흔적과 반환이 모두 일치한다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 768);
    let called = 0;
    for (const [input, expected] of rows) {
        const [, code, y] = input.split(',').map(Number);
        const trace = [];
        // 뗌 비트 포함 코드는 부호 있는 정수 표현일 수 있어 DWORD 로 맞춘다.
        const handled = dispatchDetonateInput({ code: code >>> 0, x: 0, y: y >>> 0 }, (target, event) => { trace.push(`object:${target}:${event >>> 0};return:0`); return 0; });
        const actual = trace.length ? trace.join('|') : '-';
        assert.equal(actual, expected, input);
        // 명령 코드가 맞으면 뗌이든 대상 0 이든 소비(true)하고 호출 여부만 다르다. 이 관측의 사건 코드는 모두 DetonateBomb 이다.
        assert.equal(handled, true, input);
        if (trace.length) called += 1;
    }
    // 호출 288행(대상 50 또는 65586 × 눌림 3종 × 객체 16 × 변형 3 중 눌림 코드)과 비호출 480행으로 갈린다.
    assert.equal(called, 288); assert.equal(rows.length - called, 480);
});

test('DetonateBomb 입력 분배: 다른 명령 코드는 분배하지 않는다(false)', () => {
    let calls = 0;
    for (const code of [0, DETONATE_BOMB_COMMAND + 1, DETONATE_BOMB_COMMAND ^ 0x00000100, 0x02000074]) assert.equal(dispatchDetonateInput({ code, x: 0, y: 50 }, () => { calls += 1; }), false);
    assert.equal(calls, 0);
});
