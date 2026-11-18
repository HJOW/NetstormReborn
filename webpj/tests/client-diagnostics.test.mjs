// Client 진단의 공유 로그와 웹 console·선택 Chat 전달을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientTraceGlobals, traceClientObject, writeClientDiagnosticLog } from '../www/core/client-diagnostics.js';

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
