// 원본 81입력의 전체 공유 버퍼/출력 인자/전역과 실제 재진입 순서를 확인한다. (2026-11-14 WB04-E451)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { TraceLogState, writeRawTraceLog, flushRawTraceLog } from '../www/core/raw-trace-log.js';

// 원문 바이트 문자열을 인코딩 변환 없이 hex로 표시한다.
const hex = (text) => Buffer.from(text, 'latin1').toString('hex');
// 같은 물리의 NUL까지만 덮고 그 뒤 stale BYTE는 유지한다.
function replace(state, text) { state.buffer.set(Buffer.from(text, 'latin1')); state.buffer[text.length] = 0; }
// 원문 관찰의 상태/장치 효과를 독립 입력만으로 구성한다.
function scene(columns) {
    const state = new TraceLogState(); const events = []; let writes = 0;
    state.level = Number(columns[1]); state.indent = Number(columns[2]); state.stream = Number(columns[3]); state.mirror = Number(columns[4]); state.flush = Number(columns[5]);
    const mutation = Number(columns[8]); state.buffer.fill(0xa5); replace(state, 'old');
    const hooks = {
        standardOut: 1, standardError: 2,
        console: (value) => {
            events.push(`C:${hex(value)}`);
            if (mutation === 1) state.stream = 2000;
            else if (mutation === 2) { state.mirror = 1; state.flush = 1; }
            else if (mutation === 3) { state.stream = 0; replace(state, 'changed\n'); }
            else if (mutation === 4) replace(state, 'nested replacement');
            else if (mutation === 8) { state.level = 0; state.indent = 99; }
        },
        write: (stream, value) => {
            events.push(`W:${stream}:${hex(value)}`); writes += 1;
            if (writes === 1 && mutation === 5) { state.stream = 3000; state.flush = 1; replace(state, 'after mirror'); }
            else if (writes === 1 && mutation === 6) state.flush = 0;
        },
        flush: (stream) => { events.push(`F:${stream}`); if (mutation === 7) state.stream = 0; },
    };
    return { state, hooks, result: () => `${events.join(';')}|${state.stream},${state.level >>> 0},${state.indent},${state.mirror},${state.flush}|${Buffer.from(state.buffer).toString('hex')}` };
}

test('RawTraceLog_IndependentNative81FilterMarkupLinesAndCurrentState: 전체 81입력 일치', () => {
    const rows = fs.readFileSync(new URL('../../cpppj/tests/fixtures/trace-log-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#'));
    assert.equal(rows.length, 81);
    // 출력 callback의 전역/공유 버퍼 변경까지 포함해 전체 물리를 원문과 비교한다.
    for (const line of rows) {
        const columns = line.split('\t'); const r = scene(columns); const text = columns[7] === '-' ? '' : Buffer.from(columns[7], 'hex').toString('latin1');
        writeRawTraceLog(r.state, Number(columns[0]), text, Number(columns[6]), r.hooks); assert.equal(r.result(), columns[9], columns.slice(0, 9).join(','));
    }
});

test('필터는 입력/장치를 읽지 않으며 출력 누락과 물리 초과의 선행 효과를 유지한다', () => {
    const state = new TraceLogState(); writeRawTraceLog(state, -1, null, 999, null); writeRawTraceLog(state, 30, null, 999, null); assert.equal(state.stream, 0);
    let calls = 0; const hooks = { console: () => { calls += 1; } };
    assert.throws(() => writeRawTraceLog(state, 29, 'line\r\n', 0, hooks), /실제 trace 로그 장치/);
    assert.equal(calls, 1); assert.equal(state.stream, 1); assert.equal(Buffer.from(state.buffer.subarray(0, 6)).toString('latin1'), 'line\n\0');
    const before = state.buffer.slice(); assert.throws(() => writeRawTraceLog(state, 29, 'x'.repeat(1024), 0, hooks), RangeError); assert.deepEqual(state.buffer, before);
    assert.throws(() => writeRawTraceLog(state, 29, 'bad', 1, hooks), TypeError); assert.deepEqual(state.buffer, before);
    assert.throws(() => writeRawTraceLog(state, 29, 'bad', 2, hooks), /접두 쌍/); assert.deepEqual(state.buffer, before);
});

test('FlushRawTraceLog_FlushUsesCurrentStreamAndZeroNeedsNoDevice: 현재 stream만 사용하고 변경을 복구하지 않는다', () => {
    const state = new TraceLogState(); flushRawTraceLog(state, {}); state.stream = 1000; let calls = 0;
    flushRawTraceLog(state, { flush: (stream) => { assert.equal(stream, 1000); calls += 1; state.stream = 0; } }); assert.equal(calls, 1); assert.equal(state.stream, 0);
});

test('RawTraceLog_ReentrantConsoleUsesOneActualBuffer: 중첩 출력이 바꾼 버퍼를 바깥 write/flush도 읽는다', () => {
    const state = new TraceLogState(); state.stream = 1000; state.indent = 3; state.mirror = 1; state.flush = 1; let consoles = 0; let writes = 0; let flushes = 0;
    const hooks = {
        console: () => { consoles += 1; if (consoles === 1) writeRawTraceLog(state, 29, '~1~ynested\r\n', 4, hooks); },
        write: (stream, text) => { assert.ok(stream === 2 || stream === 1000); assert.equal(text, '~1~y   nested\n'); writes += 1; },
        flush: (stream) => { assert.equal(stream, 1000); flushes += 1; },
    };
    writeRawTraceLog(state, 29, 'outer\n', 0, hooks); assert.deepEqual([consoles, writes, flushes], [2, 4, 2]);
});

test('1023BYTE 경계와 NUL 뒤 stale/CRLF 정규화·signed indent·현재 버퍼 NUL 부재를 구분한다', () => {
    const state = new TraceLogState(); const writes = []; const hooks = { console: () => {}, write: (stream, text) => writes.push(text) };
    writeRawTraceLog(state, 29, 'x'.repeat(1023), 0, hooks); assert.equal(writes[0].length, 1023); assert.equal(state.buffer[1023], 0);
    state.indent = 0xffffffff; writeRawTraceLog(state, 29, 'a\r\n\n\0tail', 0, hooks); assert.equal(writes[1], 'a\n'); assert.equal(state.buffer[3], 0); assert.equal(state.buffer[10], 120);
    const before = state.buffer.slice(); assert.throws(() => writeRawTraceLog(state, 29, 'a\0x', 2, hooks), TypeError); assert.deepEqual(state.buffer, before);
    const broken = { ...hooks, console: () => state.buffer.fill(1) }; assert.throws(() => writeRawTraceLog(state, 29, 'a', 0, broken), /NUL 없음/);
});
