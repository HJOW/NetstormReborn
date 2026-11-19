// Client 진단의 공유 로그와 웹 console·선택 Chat 전달을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientTraceGlobals, bindClientKernelDiagnostics, traceClientObject, writeClientDiagnosticLog, writeClientProcessSummary } from '../www/core/client-diagnostics.js';
import { Kernel } from '../www/core/kernel.js';

// 필터와 현재 들여쓰기·미러·Chat 관문은 같은 장기 로그 상태를 쓴다.
test('Client 진단: console·Chat 출력과 원본 필터', () => {
    const state = new ClientTraceGlobals(), events = [], chat = { color: () => 7, currentChat: () => 9, append: (token, value, context) => events.push(['chat', token, value, [...context]]) };
    const devices = { info: (text) => events.push(['info', text]), error: (text) => events.push(['error', text]), chat };
    state.log.level = 3; writeClientDiagnosticLog(state, 4, '생략', 0, devices); assert.deepEqual(events, []);
    state.traceToChat = 1; state.log.indent = 2; state.log.mirror = 1;
    writeClientDiagnosticLog(state, 3, '~1hello\r\n', 2, devices);
    assert.deepEqual(events, [['chat', 9, '~1  hello\n', [0, 1, 7, 0, 0, 0, 1, 1]], ['error', '~1  hello\n'], ['info', '~1  hello\n']]);
    assert.equal(state.log.stream, 1); assert.equal(state.log.indent, 2);
    traceClientObject(null, state, 0, devices); assert.equal(events.length, 3);
});

// 현재 풀과 이름 표를 쓰며 부모 0과 비0의 출력, 전체 프로세스 이름 및 종료 NUL을 구분한다.
test('Client 프로세스 요약: 현재 슬롯과 전체 이름을 순서대로 출력한다', () => {
    const kernel = new Kernel(3), slots = Array.from({ length: 8 }, () => new Uint8Array(12));
    const pool = { capacity: slots.length, slot: (sid) => slots[sid] };
    const types = Array.from({ length: 188 }, () => ({ name: '' }));
    slots[1][10] = 10; slots[2][10] = 1; slots[3][10] = 162; types[162].name = 'fakeSurface\0숨김';
    kernel.add({ form: 1, parent: 3, runFrame() {} });
    kernel.add({ form: 2, parent: 0, runFrame() {} });
    const lines = [];
    writeClientProcessSummary(kernel, { pool, types }, (line) => lines.push(line));
    assert.deepEqual(lines, ['pType=evilRaySystemProcessType sid=3, attachToType=fakeSurface\n', 'pType=MeteorEffect sid=0, attachToType=NULL\n']);
});

// 이름 조회 뒤 슬롯을 다시 읽고 비영 부모는 조건과 출력에서 각각 읽는다.
test('Client 프로세스 요약: 현재 슬롯과 부모를 원본 순서로 재조회한다', () => {
    const events = [], slots = Array.from({ length: 4 }, () => new Uint8Array(12));
    slots[1][10] = 10; slots[3][10] = 20;
    const types = Array.from({ length: 188 }, () => ({ name: 'parent' }));
    let reads = 0;
    const process = { get form() { events.push('form'); return 1; }, get parent() { events.push('parent'); reads += 1; return reads === 1 ? 2 : 3; } };
    const kernel = { capacity: 1, get: () => { events.push('slot'); return process; } };
    const surface = { pool: { capacity: 4, slot: (sid) => { events.push(`pool${sid}`); return slots[sid]; } }, types };
    const lines = [];
    writeClientProcessSummary(kernel, surface, (line) => lines.push(line));
    assert.deepEqual(events, ['slot', 'form', 'pool1', 'slot', 'parent', 'parent', 'pool3']);
    assert.deepEqual(lines, ['pType=evilRaySystemProcessType sid=3, attachToType=parent\n']);
});

// 소진은 같은 실제 Kernel의 요약→상세→단언 순서이며 예외가 반환되면 슬롯 0에 설치되지 않는다.
test('Client Kernel 소진 진단은 실제 슬롯과 Surface 상세·단언을 연결한다', () => {
    const kernel = new Kernel(1), slots = Array.from({ length: 3 }, () => new Uint8Array(12));
    slots[1][10] = 10; slots[2][10] = 88;
    const lines = [], order = [], error = new Error('단언');
    const surface = { pool: { capacity: 3, slot: (sid) => slots[sid] }, types: Array.from({ length: 188 }, () => ({ name: 'priest' })),
        recordAssertionDetail: (text) => { assert.match(text, /Out of processes/); order.push('detail'); },
        assertDisplay: (expression, file, line) => { assert.equal(expression, 'firstSlot != 0'); order.push('assertion'); throw error; } };
    kernel.add({ form: 1, parent: 2, runFrame() {} });
    bindClientKernelDiagnostics(kernel, surface, (line) => { lines.push(line); order.push('summary'); });
    assert.throws(() => kernel.add({ runFrame() {} }), (actual) => actual === error);
    assert.deepEqual(order, ['summary', 'detail', 'assertion']);
    assert.deepEqual(lines, ['pType=evilRaySystemProcessType sid=2, attachToType=priest\n']);
    assert.equal(kernel.get(0), null);
});

// 부모 필드가 없는 프로세스를 잘못된 NULL 부모로 보고하지 않는다.
test('Client 프로세스 요약: 부모 필드 누락을 거부한다', () => {
    const kernel = new Kernel(1), bytes = new Uint8Array(12);
    const surface = { pool: { capacity: 2, slot: () => bytes }, types: Array.from({ length: 188 }, () => ({ name: '' })) };
    kernel.add({ form: 1, runFrame() {} });
    assert.throws(() => writeClientProcessSummary(kernel, surface, () => {}), /부모 필드 누락/);
});
