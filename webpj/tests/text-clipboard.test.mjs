// CF_TEXT clipboard 부모(TextClipboard)의 읽기/쓰기 장치 호출 순서와 웹 에뮬레이터를 기준선 소스 계약으로 검증한다. (2026-11-14 WB04-E439)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(TextClipboard.cpp)에서 옮겼다. 실제 Win32 장치는 웹에 없다(브라우저 스냅숏 에뮬레이터로 대신한다).
import test from 'node:test';
import assert from 'node:assert/strict';
import { TextClipboard, bindTextClipboard, createWebTextClipboardDevices } from '../www/ui/text-clipboard.js';
import { createRawTextEntryContext } from '../www/ui/text-entry-gump.js';

const bytes = (text) => Uint8Array.from(Buffer.from(text, 'latin1'));
// 모든 장치 호출을 순서대로 기록하는 장치 묶음이다. 인자로 각 호출의 반환/원본 바이트를 바꾼다.
function rig({ available = 1, opened = 1, handle = 7, source = bytes('hello\0'), allocated = 9, target = new Uint8Array(32) } = {}) {
    const log = []; const state = { available, opened, handle, source, allocated, target };
    const devices = {
        formatAvailable: (format) => { log.push(['formatAvailable', format]); return state.available; },
        open: (window) => { log.push(['open', window]); return state.opened; },
        get: (format) => { log.push(['get', format]); return state.handle; },
        lock: (value) => { log.push(['lock', value]); return value === state.allocated ? state.target : state.source; },
        unlock: (value) => { log.push(['unlock', value]); return 5; },
        empty: () => { log.push(['empty']); return 0; },
        allocate: (flags, length) => { log.push(['allocate', flags, length]); return state.allocated; },
        set: (format, value) => { log.push(['set', format, value]); return 0; },
        close: () => { log.push(['close']); return 0; },
    };
    return { log, state, devices, clipboard: new TextClipboard(devices) };
}
const names = (log) => log.map((entry) => entry[0]);

test('read: output[0]=0 을 먼저 쓰고 형식→Open(NULL 창)→Get→Lock→복사→Unlock→Close 순으로 첫 NUL 앞만 복사하고 남은 칸은 0 으로 채운다', () => {
    const r = rig(); const out = new Uint8Array(16).fill(0x55); r.clipboard.read(out);
    assert.deepEqual(Array.from(out), [0x68, 0x65, 0x6c, 0x6c, 0x6f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]); assert.deepEqual(r.log, [['formatAvailable', 1], ['open', 0], ['get', 1], ['lock', 7], ['unlock', 7], ['close']]);
});

test('read: 원본이 capacity 보다 길면 마지막 바이트를 덮지 않고 비 NUL 로 가득 채우며 NUL 없는 짧은 원본은 끝에서 NUL 로 본다', () => {
    const long = rig({ source: bytes(`${'A'.repeat(40)}\0`) }); const out = new Uint8Array(8); long.clipboard.read(out); assert.deepEqual(Array.from(out), new Array(8).fill(0x41));
    const short = rig({ source: Uint8Array.from([0x42, 0x43]) }); const second = new Uint8Array(6).fill(0x55); short.clipboard.read(second); assert.deepEqual(Array.from(second), [0x42, 0x43, 0, 0, 0, 0]);
    const exact = rig({ source: bytes('abc\0') }); const three = new Uint8Array(3).fill(0x55); exact.clipboard.read(three); assert.deepEqual(Array.from(three), [0x61, 0x62, 0x63]);
});

test('read: 형식이 없거나 Open 이 실패하면 output[0] 만 0 이 되고 Close 도 하지 않으며, Get/Lock 이 0 이면 그 위치에서 Close 만 한다', () => {
    const none = rig({ available: 0 }); const out = new Uint8Array(4).fill(0x55); none.clipboard.read(out); assert.deepEqual(Array.from(out), [0, 0x55, 0x55, 0x55]); assert.deepEqual(none.log, [['formatAvailable', 1]]);
    const closed = rig({ opened: 0 }); closed.clipboard.read(out); assert.deepEqual(names(closed.log), ['formatAvailable', 'open']);
    const noHandle = rig({ handle: 0 }); noHandle.clipboard.read(out); assert.deepEqual(names(noHandle.log), ['formatAvailable', 'open', 'get', 'close']);
    const noLock = rig({ source: null }); noLock.clipboard.read(out); assert.deepEqual(names(noLock.log), ['formatAvailable', 'open', 'get', 'lock', 'close']);
});

test('read: 빈 출력은 RangeError 이고 장치 누락은 도달한 위치에서만 Error 다', () => {
    const r = rig(); assert.throws(() => r.clipboard.read(new Uint8Array(0)), { name: 'RangeError', message: /출력 물리 범위/ }); assert.deepEqual(r.log, []);
    const out = new Uint8Array(4).fill(0x55); assert.throws(() => new TextClipboard({}).read(out), { name: 'Error', message: /선택된 실제 OS 장치/ }); assert.equal(out[0], 0);
    const partial = rig(); const devices = { ...partial.devices, close: undefined }; assert.throws(() => new TextClipboard(devices).read(out), /선택된 실제 OS 장치/); assert.deepEqual(names(partial.log), ['formatAvailable', 'open', 'get', 'lock', 'unlock']);
    // Open 이 실패하는 경로는 Get/Close 장치를 요구하지 않는다.
    const early = rig({ opened: 0 }); new TextClipboard({ formatAvailable: early.devices.formatAvailable, open: early.devices.open }).read(out);
});

test('write: Open→Empty→Alloc(0x2000,길이+1)→Lock→복사+NUL→Unlock→Set(1)→Close 순서이고 명시 길이는 첫 NUL 에서 자르지 않는다', () => {
    const r = rig(); r.clipboard.write(bytes('a\0b')); assert.deepEqual(r.log, [['open', 0], ['empty'], ['allocate', 0x2000, 4], ['lock', 9], ['unlock', 9], ['set', 1, 9], ['close']]);
    assert.deepEqual(Array.from(r.state.target.subarray(0, 5)), [0x61, 0, 0x62, 0, 0]);
    // 빈 입력도 NUL 한 바이트를 쓴다.
    const empty = rig({ target: new Uint8Array(4).fill(0x55) }); empty.clipboard.write(new Uint8Array(0)); assert.deepEqual(empty.log[2], ['allocate', 0x2000, 1]); assert.equal(empty.state.target[0], 0); assert.equal(empty.state.target[1], 0x55);
});

test('write: Open 실패는 무효과, 할당 실패(0)는 Close 만, NULL Lock 은 RangeError(Close 하지 않음), 장치 누락은 도달한 위치에서만 Error 다', () => {
    const closed = rig({ opened: 0 }); closed.clipboard.write(bytes('x')); assert.deepEqual(names(closed.log), ['open']);
    const failed = rig({ allocated: 0 }); failed.clipboard.write(bytes('x')); assert.deepEqual(names(failed.log), ['open', 'empty', 'allocate', 'close']);
    const nullLock = rig({ target: null }); assert.throws(() => nullLock.clipboard.write(bytes('x')), { name: 'RangeError', message: /NULL GlobalLock/ }); assert.deepEqual(names(nullLock.log), ['open', 'empty', 'allocate', 'lock']);
    assert.throws(() => new TextClipboard({}).write(bytes('x')), { name: 'Error', message: /선택된 실제 OS 장치/ });
    const noSet = rig(); assert.throws(() => new TextClipboard({ ...noSet.devices, set: undefined }).write(bytes('x')), /선택된 실제 OS 장치/); assert.deepEqual(names(noSet.log), ['open', 'empty', 'allocate', 'lock', 'unlock']);
});

test('bindTextClipboard: 같은 clipboard 로 읽기/쓰기 두 callback 을 설치하고 다른 callback 은 보존하며 clipboard 가 없으면 TypeError 다', () => {
    const r = rig(); const context = { now: () => 5, recipient: () => true };
    assert.equal(bindTextClipboard(context, r.clipboard), context); const out = new Uint8Array(8); context.clipboardRead(out); assert.equal(Buffer.from(out.subarray(0, 5)).toString('latin1'), 'hello');
    r.log.length = 0; context.clipboardWrite(bytes('zz')); assert.deepEqual(names(r.log), ['open', 'empty', 'allocate', 'lock', 'unlock', 'set', 'close']); assert.equal(context.now(), 5); assert.equal(context.recipient(), true);
    assert.throws(() => bindTextClipboard({}, null), TypeError); assert.throws(() => bindTextClipboard({}), TypeError);
});

test('createWebTextClipboardDevices: 스냅숏을 Windows-1252 바이트 + NUL 로 읽고 쓰기 결과를 NUL 앞 바이트의 문자열로 store 에 넘긴다', () => {
    const stored = []; const host = { text: 'héllo €', snapshot: () => host.text, store: (text) => stored.push(text) };
    const clipboard = new TextClipboard(createWebTextClipboardDevices(host)); const out = new Uint8Array(16).fill(0x55); clipboard.read(out);
    assert.deepEqual(Array.from(out.subarray(0, 9)), [0x68, 0xe9, 0x6c, 0x6c, 0x6f, 0x20, 0x80, 0, 0]);
    // 스냅숏이 없으면 형식이 없는 것이다(output[0]=0).
    host.text = null; out.fill(0x55); clipboard.read(out); assert.equal(out[0], 0); assert.equal(out[1], 0x55); host.text = undefined; out.fill(0x55); clipboard.read(out); assert.equal(out[0], 0);
    // 쓰기: 첫 NUL 앞만 문자열이 되고 Windows-1252 로 해독한다.
    clipboard.write(Uint8Array.from([0x61, 0x62, 0, 0x63])); clipboard.write(Uint8Array.from([0x68, 0xe9, 0x80])); clipboard.write(new Uint8Array(0)); assert.deepEqual(stored, ['ab', 'hé€', '']);
    // 쓰기마다 새 핸들을 쓰고 읽기 핸들(1)과 겹치지 않는다.
    const devices = createWebTextClipboardDevices({ snapshot: () => 'x', store() {} }); assert.equal(devices.open(0), 1); assert.equal(devices.close(), 1); assert.equal(devices.allocate(0x2000, 3), 2); assert.equal(devices.allocate(0x2000, 3), 3); assert.equal(devices.lock(99), null); assert.equal(devices.get(1), 1);
});

test('createRawTextEntryContext: 선택 콜백 여섯 칸이 모두 null 이고 지정한 필드만 채우며 bindTextClipboard 는 clipboard 두 칸만 설치한다', () => {
    const empty = createRawTextEntryContext(); assert.deepEqual(empty, { now: null, clipboardRead: null, clipboardWrite: null, recipient: null, complete: null, submit: null });
    const now = () => 5; const context = createRawTextEntryContext({ now }); assert.equal(context.now, now); assert.equal(context.submit, null);
    bindTextClipboard(context, rig().clipboard); assert.equal(typeof context.clipboardRead, 'function'); assert.equal(typeof context.clipboardWrite, 'function'); assert.equal(context.now, now); assert.equal(context.recipient, null);
});
