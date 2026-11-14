// 포인터 안팎 전환 앞단(GumpPointerBoundary, 원본 004d6379..004d65a0)의 원본 10.78 독립 관측 3,878행 대조: 첫 폴링 사건·언어 3 의 Ctrl 합성 release·정적 초기화 비트 1/2·안→밖 좌/우 기록·밖→안 합성 release 예약·
// callback 뒤 현재 기록 재조회·wasInside 마지막 덮어쓰기와 실제 40칸 입력 큐(포화 prefill 포함)의 최종 사건 열을 cpppj `GumpPointerBoundaryTests`(Native3878)와 같은 입력 구성으로 비교한다.
// 입력 열(상태·사건·mutation·prefill)만 공급하고 관측 열은 비교에만 쓴다. 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E491)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { InputQueue } from '../www/core/input-queue.js';
import { GumpPointerBoundaryState, prepareGumpPointerBoundary } from '../www/ui/gump-pointer-boundary.js';

// 십진 입력의 하위 32비트이다.
const number = (text) => Number(BigInt.asUintN(32, BigInt(text)));
// 쉼표 세 값(code, x, y)을 같은 DWORD 비트로 읽는 사건이다(좌표는 signed 해석).
function pointerEvent(field) { const words = field.split(','); assert.equal(words.length, 3); return { code: number(words[0]), x: number(words[1]) | 0, y: number(words[2]) | 0 }; }
// 사건을 원문과 같은 DWORD 십진 문자열로 관찰한다.
const pointerWords = (event) => `${event.code >>> 0},${event.x >>> 0},${event.y >>> 0}`;

test('GumpPointerBoundary 원본 10.78 3,878행: 폴링·합성 release 예약·초기화 비트·안팎 기록·callback 재조회·큐 사건 열이 일치한다', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/gump-pointer-boundary-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#')).map((line) => line.split('\t'));
    assert.equal(table.length, 3878);
    const failures = [];
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 18, `행 ${index}`);
        const state = new GumpPointerBoundaryState(); state.initializationFlags = number(row[1]); state.wasInside = number(row[2]); state.leftOutside = pointerEvent(row[3]); state.rightOutside = pointerEvent(row[4]);
        const language = number(row[0]); const mutation = number(row[9]); const prefill = number(row[10]);
        const primary = pointerEvent(row[5]); const control = pointerEvent(row[6]); const left = pointerEvent(row[7]); const right = pointerEvent(row[8]);
        const queue = new InputQueue(); let polls = 0; let queued = 0; const trace = [];
        // 포화 입력의 기존 사건이다.
        for (let i = 0; i < prefill; i += 1) queue.push(0x02000100 + i, i, i + 100);
        // 원문과 같은 callback 입력의 현재 기록 변경만 주입한다(해당 단계에 도달했을 때만).
        const mutate = (stage) => {
            if (mutation === 1 && stage === 'P3') state.leftOutside = { code: 0x10000, x: 41, y: 42 };
            else if (mutation === 2 && stage === 'Q1') state.rightOutside = { code: 0x80020000, x: 51, y: 52 };
            else if (mutation === 3 && stage === 'Q1') state.rightOutside = { code: 0x20000, x: 61, y: 62 };
            else if (mutation === 4 && stage === 'P3') { state.leftOutside = { code: 0x80010000, x: 71, y: 72 }; state.wasInside = 0x12345678; state.initializationFlags = 0x40000003; }
            else if (mutation === 5 && stage === 'P1') { state.leftOutside = { code: 0x80010000, x: 81, y: 82 }; state.rightOutside = { code: 0x80020000, x: 83, y: 84 }; state.initializationFlags = 0x80000002; }
            else if (mutation === 6 && stage === 'Q1') { state.wasInside = 0x12345678; state.initializationFlags = 0x40000003; }
            else return;
            trace.push(`M${mutation}`);
        };
        const hooks = {
            poll: (code) => {
                polls += 1; let event;
                switch (code >>> 0) { case 0x120000: event = primary; break; case 0x110000: event = control; break; case 0x10000: event = left; break; case 0x20000: event = right; break; default: throw new Error('원문 밖 포인터 폴링 요청'); }
                trace.push(`P,${code >>> 0},${pointerWords(event)}`); mutate(`P${polls}`); return event;
            },
            queue: (event) => { queued += 1; trace.push(`Q,${pointerWords(event)}`); mutate(`Q${queued}`); queue.push(event.code, event.x, event.y); },
        };
        const pointer = prepareGumpPointerBoundary(state, language, hooks);
        const problems = [];
        if (pointerWords(pointer) !== row[11]) problems.push('pointer');
        if ((state.initializationFlags >>> 0) !== number(row[12]) || (state.wasInside >>> 0) !== number(row[13])) problems.push('state');
        if (pointerWords(state.leftOutside) !== row[14] || pointerWords(state.rightOutside) !== row[15]) problems.push('records');
        if (trace.join(';') !== row[17]) problems.push('trace');
        // 전체 기존 사건/보정 사건을 같은 큐에서 꺼내 비교한다(release 도 버리지 않는다).
        const drained = []; while (!queue.empty) drained.push(pointerWords(queue.pop(false)));
        if ((drained.join(';') || '-') !== row[16]) problems.push('queue');
        if (problems.length) failures.push(`${index}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});
