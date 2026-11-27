// Client 진단의 공유 로그와 웹 console·선택 Chat 전달을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientTraceGlobals, bindClientKernelDiagnostics, logClientDisplayAssertion, logRegularFailure, traceClientObject, writeClientDiagnosticLog, writeClientProcessSummary } from '../www/core/client-diagnostics.js';
import { Kernel } from '../www/core/kernel.js';
import { SidPool } from '../www/core/sid-pool.js';

// 상세가 있을 때만 기존 assertion보다 먼저 기록하고 없는 경우에도 기존 위치/조건은 유지한다.
test('Client 오류 문맥: 표시 상세와 기존 assertion의 출력 순서', () => {
    const calls = [], devices = { warning: (text) => calls.push(['detail', text]), error: (text) => calls.push(['assertion', text]) };
    logClientDisplayAssertion('TYPE83 frame-4', 'frame >= 0', 'asset.cpp', 42, devices);
    logClientDisplayAssertion(null, 'ok', 'next.cpp', 7, devices); logClientDisplayAssertion('', 'empty', 'none.cpp', 8, devices);
    assert.deepEqual(calls, [['detail', '원본 표시 상세: TYPE83 frame-4'], ['assertion', '원본 표시 진단: asset.cpp:42 frame >= 0'], ['assertion', '원본 표시 진단: next.cpp:7 ok'], ['assertion', '원본 표시 진단: none.cpp:8 empty']]);
});

// raw42의 비정렬 DWORD와 signed프레임·float32 payload를 현재 실제 풀에서 읽는다. 진단은 풀을 수정하지 않는다.
test('Client 오류 문맥: 실제 슬롯의 전체 Regular 비트와 범위 밖 SID', () => {
    const pool = new SidPool(), sid = pool.allocate(1), raw = pool.physical(sid), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength), calls = [];
    raw[10] = 83; view.setInt32(36, -4, true); view.setUint32(42, 0x89abcdef, true); const before = pool.bytes().slice();
    logRegularFailure(pool, sid, 0x123, 0x80000001, 1.25, (text) => calls.push(text));
    logRegularFailure(pool, -1, 2, 3, -0, (text) => calls.push(text));
    assert.deepEqual(calls, [`Regular failure SID${sid} TYPE83 event291 count2147483649 payload3fa00000 frame-4 raw42=89abcdef`, 'Regular failure SID4294967295 TYPEunknown event2 count3 payload80000000 (pool capacity 32768)']);
    assert.deepEqual(pool.bytes(), before);
    assert.doesNotThrow(() => logRegularFailure(pool, sid, 0, 0, 0, () => { throw new Error('기록 실패'); }));
    assert.doesNotThrow(() => logRegularFailure({ get capacity() { throw new Error('풀 조회 실패'); } }, sid, 0, 0, 0));
});

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
