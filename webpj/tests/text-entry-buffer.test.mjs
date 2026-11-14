// 문자 편집 버퍼(TextEntryBuffer)의 구성/선택/삽입/줄 offset/복사/clipboard 를 기준선 소스 계약과 손 계산 값으로 검증한다. (2026-11-14 WB04-E438)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(TextEntryBuffer.cpp)에서 옮겼다. 메모리는 실제 TextEntryStorage 위에서 확인하고 실패 분기는 훅을 바꿔 만든다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { TextEntryBuffer, TextEntryBufferState } from '../www/ui/text-entry-buffer.js';
import { TextEntryStorage } from '../www/ui/text-entry-storage.js';

// 실제 저장소 위의 구성된 버퍼 틀이다. clip.text 가 clipboard 읽기 내용이고 log 는 clipboard 효과를 기록한다.
function rig(clip = { text: '' }) {
    const log = [];
    const storage = new TextEntryStorage((bytes) => { log.push(['read']); bytes.fill(0); bytes.set(Buffer.from(`${clip.text}\0`, 'latin1')); }, (bytes) => log.push(['write', Buffer.from(bytes).toString('latin1')]));
    const buffer = storage.create(1); buffer.construct(); return { storage, buffer, state: buffer.state(), log, clip };
}
// 훅 일부를 바꿀 수 있는 직접 틀이다(구성하지 않은 상태).
function direct(overrides = {}) {
    const storage = new TextEntryStorage(); const state = new TextEntryBufferState();
    const hooks = { allocate: (size) => storage.allocate(size), release: (address) => storage.release(address), resize: (address, size) => storage.resize(address, size), bytes: (address) => storage.bytes(address), ...overrides };
    return { storage, state, buffer: new TextEntryBuffer(state, hooks), hooks };
}
// 저장소 주소에서 DWORD n 개를 읽는다.
function dwords(storage, address, n) { const bytes = storage.bytes(address); const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.length); return Array.from({ length: n }, (_, index) => view.getUint32(index * 4, true)); }

test('생성자: 메모리 네 훅은 필수이고 clipboard 훅은 선택이다', () => {
    const state = new TextEntryBufferState(); const ok = { allocate() {}, release() {}, resize() {}, bytes() {} };
    assert.doesNotThrow(() => new TextEntryBuffer(state, ok)); assert.throws(() => new TextEntryBuffer(state, null), TypeError); assert.throws(() => new TextEntryBuffer(state), TypeError);
    for (const name of Object.keys(ok)) assert.throws(() => new TextEntryBuffer(state, { ...ok, [name]: undefined }), { name: 'TypeError', message: /메모리장치/ }, name);
});

test('construct: 가상표/2048 본문/512 줄표/용량 0x800·줄표 128칸·dirty 1 을 쓰고, 요청 크기와 무관하게 성공 시 용량은 2048이다', () => {
    const r = direct(); r.state.selectionBegin = 5; r.state.selectionEnd = 6; r.state.length = 9; r.state.lineCount = 4; r.state.capacity = 77;
    r.buffer.construct();
    assert.deepEqual({ ...r.state }, { vtable: 0x508da8, offsets: 0x20000810, offsetCapacity: 128, lineCount: 0, dirty: 1, selectionBegin: 0, selectionEnd: 0, length: 0, capacity: 0x800, data: 0x20000000 });
    assert.equal(r.storage.bytes(r.state.data).length, 2048); assert.equal(r.storage.bytes(r.state.offsets).length, 512);
    // 요청 100 은 100바이트만 할당하지만 capacity 필드는 2048이다(원문 그대로).
    const small = direct(); small.buffer.construct(100); assert.equal(small.storage.bytes(small.state.data).length, 100); assert.equal(small.state.capacity, 0x800);
    // 본문 할당 실패(0): data/capacity 0 이지만 줄표는 계속 할당한다.
    const failing = direct({ allocate: (size) => (size === 2048 ? 0 : 0x20000000) }); failing.state.capacity = 99; failing.buffer.construct(); assert.equal(failing.state.data, 0); assert.equal(failing.state.capacity, 0); assert.equal(failing.state.offsets, 0x20000000); assert.equal(failing.state.dirty, 1); assert.equal(failing.state.offsetCapacity, 128);
});

test('destroy: data 가 있으면 본문→줄표 순으로 반납하고 필드를 비우며 data 가 0 이면 줄표도 건드리지 않는다', () => {
    const released = []; const r = direct({ release: (address) => { released.push(address); } }); r.buffer.construct();
    const data = r.state.data; const offsets = r.state.offsets; r.buffer.destroy(); assert.deepEqual(released, [data, offsets]); assert.equal(r.state.data, 0); assert.equal(r.state.capacity, 0); assert.equal(r.state.offsets, 0); assert.equal(r.state.vtable, 0x508da8);
    // data 0 이면 아무것도 반납하지 않고 줄표 주소도 남는다.
    released.length = 0; r.state.data = 0; r.state.offsets = 0x1234; r.state.vtable = 0; r.buffer.destroy(); assert.deepEqual(released, []); assert.equal(r.state.offsets, 0x1234); assert.equal(r.state.vtable, 0x508da8);
    const real = rig(); assert.equal(real.storage.allocationCount(), 2); real.buffer.destroy(); assert.equal(real.storage.allocationCount(), 0);
});

test('select: 두 위치를 정렬해 0..length 로 clamp 한다', () => {
    const r = rig(); r.buffer.insert('hello');
    for (const [args, expected] of [[[3, 1], [1, 3]], [[-5, 99], [0, 5]], [[9, 9], [5, 5]], [[2, -7], [0, 2]], [[7, 3], [3, 5]], [[8, 9], [5, 5]], [[0, 0], [0, 0]], [[2, 2], [2, 2]], [[5, 0], [0, 5]]]) { r.buffer.select(...args); assert.deepEqual([r.state.selectionBegin, r.state.selectionEnd], expected, args.join(',')); }
});

test('insert: 선택을 NUL 앞 바이트로 바꾸고(꼬리 이동·길이·끝 NUL) 선택은 삽입 끝, dirty 1 이며 같은 폭/축소/확장/빈 입력을 모두 처리한다', () => {
    const r = rig(); const s = r.state; r.state.dirty = 0;
    r.buffer.insert('hello'); assert.equal(r.buffer.text(), 'hello'); assert.deepEqual([s.length, s.selectionBegin, s.selectionEnd, s.dirty], [5, 5, 5, 1]); assert.equal(r.buffer.data()[5], 0);
    r.buffer.insert(' world'); assert.equal(r.buffer.text(), 'hello world'); assert.equal(s.length, 11);
    // 축소: 5바이트 선택을 1바이트로.
    r.buffer.select(0, 5); r.buffer.insert('J'); assert.equal(r.buffer.text(), 'J world'); assert.deepEqual([s.length, s.selectionBegin, s.selectionEnd], [7, 1, 1]); assert.equal(r.buffer.data()[7], 0);
    // 확장: 선택 없이 3바이트를 가운데에.
    r.buffer.select(1, 1); r.buffer.insert('XYZ'); assert.equal(r.buffer.text(), 'JXYZ world'); assert.deepEqual([s.length, s.selectionBegin, s.selectionEnd], [10, 4, 4]);
    r.buffer.select(2, 4); r.buffer.insert('Q'); assert.equal(r.buffer.text(), 'JXQ world'); assert.deepEqual([s.length, s.selectionBegin, s.selectionEnd], [9, 3, 3]);
    // 같은 폭: 이동 없이 덮어쓴다.
    r.buffer.select(0, 1); r.buffer.insert('K'); assert.equal(r.buffer.text(), 'KXQ world'); assert.equal(s.length, 9);
    // 빈 입력(null/빈 문자열/빈 배열)은 선택 삭제다.
    r.buffer.select(0, 3); r.buffer.insert(null); assert.equal(r.buffer.text(), ' world'); assert.deepEqual([s.length, s.selectionBegin, s.selectionEnd], [6, 0, 0]);
    r.buffer.select(0, 1); r.buffer.insert(''); assert.equal(r.buffer.text(), 'world'); r.buffer.select(0, 1); r.buffer.insert(new Uint8Array(0)); assert.equal(r.buffer.text(), 'orld');
    // 첫 NUL 앞만 쓴다(문자열/바이트 배열). NUL 이 없는 바이트 배열은 TypeError(길이를 알 수 없다).
    r.buffer.select(0, 4); r.buffer.insert('ab\0cd'); assert.equal(r.buffer.text(), 'ab'); r.buffer.select(0, 2); r.buffer.insert(Uint8Array.from([0x78, 0x79, 0, 0x7a])); assert.equal(r.buffer.text(), 'xy');
    assert.throws(() => r.buffer.insert(Uint8Array.from([1, 2, 3])), { name: 'TypeError', message: /NUL입력/ }); assert.equal(r.buffer.text(), 'xy');
    // 확장 바이트(>127)는 그대로 쓴다.
    r.buffer.select(0, 2); r.buffer.insert('\xe9\xff'); assert.equal(r.buffer.text(), '\xe9\xff'); assert.deepEqual(Array.from(r.buffer.data().subarray(0, 3)), [0xe9, 0xff, 0]);
});

test('insert: 용량(필드)이 모자라면 1024 배수로 재할당해 이동하고(내용 보존), 같으면 재할당하지 않으며 재할당 실패는 아무것도 바꾸지 않는다', () => {
    const r = rig(); const s = r.state; const first = s.data;
    r.buffer.insert('x'.repeat(2047)); assert.equal(s.capacity, 2048); assert.equal(s.data, first); assert.equal(s.length, 2047);
    // 필요 2049 > 2048 → ((2049-1)>>10)+1)<<10 = 3072.
    r.buffer.insert('yy'); assert.equal(s.capacity, 3072); assert.notEqual(s.data, first); assert.equal(s.length, 2049); assert.equal(r.storage.bytes(s.data).length, 3072); assert.equal(r.buffer.text(), `${'x'.repeat(2047)}yy`);
    assert.throws(() => r.storage.bytes(first + 1), RangeError);
    // 필요 3071 ≤ 3072 → 재할당 없음. 필요 3073 → ((3073-1)>>10)+1)<<10 = 4096.
    const second = s.data; r.buffer.insert('z'.repeat(1022)); assert.equal(s.capacity, 3072); assert.equal(s.data, second); assert.equal(s.length, 3071);
    r.buffer.insert('ab'); assert.equal(s.capacity, 4096); assert.equal(s.length, 3073); assert.equal(r.buffer.text().slice(-4), 'zzab');
    // 선택을 지우는 삽입은 필요 길이에서 지운 폭을 뺀다: 길이 3073 에서 2000바이트 선택을 2001바이트로 바꾸면 필요 3074 ≤ 4096 이라 그대로다.
    r.buffer.select(0, 2000); r.buffer.insert('q'.repeat(2001)); assert.equal(s.capacity, 4096); assert.equal(s.length, 3074);
    // 재할당 실패(0)는 상태를 그대로 두고 돌아온다.
    const bad = direct({ resize: () => 0 }); bad.buffer.construct(); bad.buffer.insert('abc'); const before = { ...bad.state };
    bad.buffer.insert('x'.repeat(3000)); assert.deepEqual({ ...bad.state }, before); assert.equal(bad.buffer.text(), 'abc');
});

test('insert: 필드 용량과 달리 실제 물리 할당 범위 밖은 RangeError 이다(끝 NUL 한 바이트 포함)', () => {
    const r = rig(); assert.throws(() => r.buffer.insert('x'.repeat(2048)), { name: 'RangeError', message: /현재메모리범위/ });
    const small = direct(); small.buffer.construct(100); small.buffer.insert('x'.repeat(99)); assert.equal(small.buffer.text().length, 99); assert.throws(() => small.buffer.insert('y'), RangeError);
    // 구성 전(본문 없음)에도 삽입은 용량 0 < 필요 1 이라 1024 로 재할당한 새 본문에서 시작한다.
    const none = direct(); none.buffer.insert('a'); assert.equal(none.state.capacity, 1024); assert.equal(none.buffer.text(), 'a'); assert.equal(none.storage.bytes(none.state.data).length, 1024);
});

test('reset: 첫 NUL·길이·선택·dirty 를 초기화한 뒤 삽입하며 본문이 없으면 RangeError 다', () => {
    const r = rig(); r.buffer.insert('zzzzzz'); r.buffer.select(1, 3); r.state.dirty = 0;
    r.buffer.reset('abc'); assert.equal(r.buffer.text(), 'abc'); assert.deepEqual([r.state.length, r.state.selectionBegin, r.state.selectionEnd, r.state.dirty], [3, 3, 3, 1]);
    r.buffer.reset(); assert.equal(r.buffer.text(), ''); assert.equal(r.state.length, 0); r.buffer.reset(null); assert.equal(r.buffer.data()[0], 0);
    assert.throws(() => direct().buffer.reset('x'), RangeError);
});

test('ensureLines/lineCount/lineLength: LF 마다 줄 offset 을 쓰고 마지막 칸은 length 이며 LF 는 앞 행에 포함하고 dirty 가 0 이면 다시 만들지 않는다', () => {
    const r = rig(); r.buffer.insert('a\nbb\n\nccc'); assert.equal(r.state.dirty, 1);
    assert.equal(r.buffer.ensureLines(), true); assert.equal(r.state.dirty, 0); assert.equal(r.state.lineCount, 4); assert.deepEqual(dwords(r.storage, r.state.offsets, 5), [0, 2, 5, 6, 9]);
    assert.equal(r.buffer.lineCount(), 4); assert.deepEqual([0, 1, 2, 3].map((line) => r.buffer.lineLength(line)), [2, 3, 1, 3]); assert.equal(r.buffer.lineLength(-1), -1); assert.equal(r.buffer.lineLength(4), -1);
    // dirty 0 이면 줄표를 다시 쓰지 않는다.
    r.storage.bytes(r.state.offsets).fill(0x55); assert.equal(r.buffer.ensureLines(), true); assert.equal(r.storage.bytes(r.state.offsets)[0], 0x55);
    // 삽입이 dirty 를 켜서 다시 만든다. LF 로 끝나면 마지막 줄이 빈 줄이다.
    r.buffer.select(0, 9); r.buffer.insert('x\n'); assert.equal(r.buffer.lineCount(), 2); assert.deepEqual(dwords(r.storage, r.state.offsets, 3), [0, 2, 2]); assert.equal(r.buffer.lineLength(1), 0);
    // 빈 본문은 한 줄이다.
    r.buffer.select(0, 2); r.buffer.insert(''); assert.equal(r.buffer.lineCount(), 1); assert.equal(r.buffer.lineLength(0), 0); assert.deepEqual(dwords(r.storage, r.state.offsets, 2), [0, 0]);
});

test('ensureLines: 줄표 128칸이 차면 4*칸+512바이트로 확장해 내용을 복사하고 옛 줄표를 반납하며, 할당 실패는 부분 상태를 남기고 false 다', () => {
    const r = rig(); r.buffer.insert('a\n'.repeat(200)); const first = r.state.offsets;
    assert.equal(r.buffer.ensureLines(), true); assert.equal(r.state.lineCount, 201); assert.equal(r.state.offsetCapacity, 256); assert.notEqual(r.state.offsets, first); assert.equal(r.storage.allocationCount(), 2);
    const offsets = dwords(r.storage, r.state.offsets, 202); assert.deepEqual(offsets.slice(0, 201), Array.from({ length: 201 }, (_, index) => index * 2)); assert.equal(offsets[201], 400); assert.equal(r.storage.bytes(r.state.offsets).length, 1024);
    // 확장 실패: 1024바이트 할당만 0 을 준다.
    let fail = true; const storage = new TextEntryStorage(); const state = new TextEntryBufferState();
    const buffer = new TextEntryBuffer(state, { allocate: (size) => (size === 1024 && fail ? 0 : storage.allocate(size)), release: (address) => storage.release(address), resize: (address, size) => storage.resize(address, size), bytes: (address) => storage.bytes(address) });
    buffer.construct(); buffer.insert('a\n'.repeat(200)); assert.equal(buffer.ensureLines(), false); assert.equal(state.dirty, 1); assert.equal(state.lineCount, 128); assert.equal(state.offsetCapacity, 128);
    assert.equal(buffer.lineCount(), 128); fail = false; assert.equal(buffer.ensureLines(), true); assert.equal(state.lineCount, 201); assert.equal(state.dirty, 0);
});

test('copyLine: 행을 NUL 과 함께 복사하고 선택 구간 쌍을 행 기준으로 쓰며 범위/용량 거부는 0xffffffff 이고 출력을 보존한다', () => {
    const r = rig(); r.buffer.insert('ab\ncd\nef'); const out = new Uint8Array(16); const sel = [9, 9];
    const text = (n) => Buffer.from(out.subarray(0, n)).toString('latin1');
    r.buffer.select(1, 4);
    assert.equal(r.buffer.copyLine(0, out, sel), 3); assert.equal(text(4), 'ab\n\0'); assert.deepEqual(sel, [1, 3]);
    assert.equal(r.buffer.copyLine(1, out, sel), 3); assert.equal(text(4), 'cd\n\0'); assert.deepEqual(sel, [0, 1]);
    // 선택이 행 밖이면 -1/-1 이고 행 복사는 이미 끝났다.
    assert.equal(r.buffer.copyLine(2, out, sel), 2); assert.equal(text(3), 'ef\0'); assert.deepEqual(sel, [-1, -1]);
    // 캐럿이 끝(begin==length)이면 -1 검사를 건너뛰고 그대로 계산한다.
    r.buffer.select(8, 8); assert.equal(r.buffer.copyLine(0, out, sel), 3); assert.deepEqual(sel, [8, 3]); assert.equal(r.buffer.copyLine(2, out, sel), 2); assert.deepEqual(sel, [2, 2]);
    // 범위 밖/출력 부족은 0xffffffff, 선택 쌍과 출력은 그대로다. 정확히 count+1 이면 성공한다.
    sel[0] = 7; sel[1] = 8; out.fill(0x55); assert.equal(r.buffer.copyLine(-1, out, sel), 0xffffffff); assert.equal(r.buffer.copyLine(3, out, sel), 0xffffffff); assert.equal(r.buffer.copyLine(0, out.subarray(0, 3), sel), 0xffffffff);
    assert.deepEqual(sel, [7, 8]); assert.equal(out[0], 0x55); assert.equal(r.buffer.copyLine(0, out.subarray(0, 4), sel), 3); assert.deepEqual(Array.from(out.subarray(0, 4)), [0x61, 0x62, 0x0a, 0]);
});

test('copyAll: 용량이 length+1 이상일 때만 NUL 까지 복사해 length 를 돌려주고 모자라면 0xffffffff 이며 출력을 건드리지 않는다', () => {
    const r = rig(); r.buffer.insert('hello'); const out = new Uint8Array(10).fill(0x55);
    assert.equal(r.buffer.copyAll(out.subarray(0, 5)), 0xffffffff); assert.deepEqual(Array.from(out.subarray(0, 5)), [0x55, 0x55, 0x55, 0x55, 0x55]);
    assert.equal(r.buffer.copyAll(out.subarray(0, 6)), 5); assert.deepEqual(Array.from(out.subarray(0, 7)), [0x68, 0x65, 0x6c, 0x6c, 0x6f, 0, 0x55]);
    out.fill(0x55); assert.equal(r.buffer.copyAll(out), 5); assert.equal(out[6], 0x55); r.buffer.select(0, 5); r.buffer.insert(''); assert.equal(r.buffer.copyAll(out.subarray(0, 1)), 0); assert.equal(out[0], 0);
});

test('copy/cut: 빈 선택은 전체 본문을 복사하고(저장 선택은 바꾸지 않음) 길이 0 은 장치를 요구하지 않으며 cut 은 복사 뒤 삭제하고 dirty 1 이다', () => {
    const r = rig(); r.buffer.insert('hello world'); r.buffer.select(0, 5); r.buffer.copy(); assert.deepEqual(r.log, [['write', 'hello']]); assert.deepEqual([r.state.selectionBegin, r.state.selectionEnd], [0, 5]);
    r.log.length = 0; r.buffer.select(3, 3); r.buffer.copy(); assert.deepEqual(r.log, [['write', 'hello world']]); assert.deepEqual([r.state.selectionBegin, r.state.selectionEnd], [3, 3]);
    r.log.length = 0; r.buffer.select(0, 5); r.state.dirty = 0; r.buffer.cut(); assert.deepEqual(r.log, [['write', 'hello']]); assert.equal(r.buffer.text(), ' world'); assert.deepEqual([r.state.selectionBegin, r.state.selectionEnd, r.state.dirty], [0, 0, 1]);
    r.log.length = 0; r.buffer.cut(); assert.deepEqual(r.log, [['write', ' world']]); assert.equal(r.buffer.text(), ''); assert.equal(r.state.length, 0);
    // 빈 본문: 복사는 효과가 없고 cut 도 dirty 만 남긴다.
    r.log.length = 0; r.state.dirty = 0; r.buffer.cut(); assert.deepEqual(r.log, []); assert.equal(r.state.dirty, 1); r.buffer.copy(); assert.deepEqual(r.log, []);
    const noWrite = direct(); noWrite.buffer.construct(); noWrite.buffer.insert('a'); assert.throws(() => noWrite.buffer.copy(), { name: 'Error', message: /clipboard쓰기 없음/ }); noWrite.buffer.select(0, 0); const empty = direct(); empty.buffer.construct(); empty.buffer.copy();
});

test('paste: clipboard 문자열을 선택 위치에 넣고 제외문자는 지운 뒤 현재 칸을 다시 검사하며 비면 삽입 없이 dirty 1 만 남긴다', () => {
    const clip = { text: 'abc' }; const r = rig(clip); r.buffer.insert('XY'); r.buffer.select(1, 1); r.buffer.paste(); assert.equal(r.buffer.text(), 'XabcY'); assert.deepEqual([r.state.selectionBegin, r.state.selectionEnd], [4, 4]);
    // 선택은 대체한다.
    r.buffer.select(0, 5); clip.text = 'q'; r.buffer.paste(); assert.equal(r.buffer.text(), 'q');
    // 제외문자: 연속 제외도 다시 검사해 모두 지운다. 여러 제외문자.
    r.buffer.select(0, 1); r.buffer.insert(''); clip.text = 'a-b-c'; r.buffer.paste('-'); assert.equal(r.buffer.text(), 'abc');
    r.buffer.select(0, 3); clip.text = 'a--b'; r.buffer.paste('-'); assert.equal(r.buffer.text(), 'ab');
    r.buffer.select(0, 2); clip.text = 'a-+b+-c'; r.buffer.paste('-+'); assert.equal(r.buffer.text(), 'abc');
    // 제외 목록의 첫 NUL 앞만 쓴다(바이트 배열도 가능).
    r.buffer.select(0, 3); clip.text = 'xy-z'; r.buffer.paste(Uint8Array.from([0x2d, 0, 0x78])); assert.equal(r.buffer.text(), 'xyz');
    r.buffer.select(0, 3); clip.text = 'x1y2'; r.buffer.paste('12'); assert.equal(r.buffer.text(), 'xy'); assert.throws(() => r.buffer.paste(Uint8Array.from([0x78])), { name: 'TypeError', message: /NUL입력/ });
    // 모두 제외되거나 빈 clipboard 는 삽입하지 않고(선택 유지) dirty 만 켠다.
    r.state.dirty = 0; r.buffer.select(0, 1); clip.text = '---'; r.buffer.paste('-'); assert.equal(r.buffer.text(), 'xy'); assert.deepEqual([r.state.selectionBegin, r.state.selectionEnd, r.state.dirty], [0, 1, 1]);
    r.state.dirty = 0; clip.text = ''; r.buffer.paste(); assert.equal(r.buffer.text(), 'xy'); assert.equal(r.state.dirty, 1);
    // 확장 바이트와 255자 한도.
    r.buffer.select(0, 2); clip.text = '\xe9\xe8'; r.buffer.paste(); assert.equal(r.buffer.text(), '\xe9\xe8'); r.buffer.select(0, 2); clip.text = 'm'.repeat(255); r.buffer.paste(); assert.equal(r.state.length, 255);
});

test('paste: clipboard 장치 누락은 Error 이고 NUL 없이 256바이트를 채우면 TypeError 이며 제외문자 없을 때는 원문 그대로 삽입한다', () => {
    const none = direct(); none.buffer.construct(); assert.throws(() => none.buffer.paste(), { name: 'Error', message: /clipboard읽기 없음/ });
    const full = direct({ clipboardRead: (bytes) => bytes.fill(0x41) }); full.buffer.construct(); assert.throws(() => full.buffer.paste(), { name: 'TypeError', message: /NUL입력/ }); assert.equal(full.buffer.text(), '');
    const ok = direct({ clipboardRead: (bytes) => bytes.set(Buffer.from('hi\0')) }); ok.buffer.construct(); ok.buffer.paste(); ok.buffer.paste(); assert.equal(ok.buffer.text(), 'hihi');
});

test('state/data/text: 같은 상태 참조를 돌려주고 data 는 현재 주소의 남은 범위, text 는 length 앞 바이트 문자열이며 본문이 없으면 빈 값이다', () => {
    const r = rig(); assert.equal(r.buffer.state(), r.state); r.buffer.insert('\xe9ab'); assert.equal(r.buffer.text(), '\xe9ab'); assert.equal(r.buffer.data().length, 2048);
    const before = r.buffer.data(); r.buffer.insert('x'.repeat(2100)); assert.equal(r.buffer.data().length, 3072); assert.equal(before.length, 2048);
    const none = direct(); assert.equal(none.buffer.data().length, 0); assert.equal(none.buffer.text(), ''); none.state.length = 3; assert.throws(() => none.buffer.text(), RangeError);
    const big = rig(); big.buffer.insert('a'.repeat(5000)); assert.equal(big.buffer.text().length, 5000);
});

test('TextEntryBufferState: 새 상태는 0x28 필드가 모두 0 이다', () => {
    assert.deepEqual({ ...new TextEntryBufferState() }, { vtable: 0, offsets: 0, offsetCapacity: 0, lineCount: 0, dirty: 0, selectionBegin: 0, selectionEnd: 0, length: 0, capacity: 0, data: 0 });
});

test('insert: 입력이 자기 본문과 겹치면 원문 REP MOVSD 처럼 4바이트씩 읽은 뒤 쓰므로 겹침 결과가 바이트 단위 전방 복사와 다르다', () => {
    const r = rig(); r.buffer.insert('ABCD'); r.buffer.select(2, 2);
    // 입력은 자기 본문 'ABCD\0'의 뷰다. 꼬리(CD)를 뒤로 민 뒤에도 입력 바이트는 그대로고, 복사 목적지가 입력보다 2바이트 뒤라 겹친다.
    r.buffer.insert(r.buffer.data().subarray(0, 5)); assert.equal(r.buffer.text(), 'ABABCDCD'); assert.equal(r.state.length, 8);
});

test('insert: 선택 삭제로 줄어든 필요 길이는 지운 폭을 빼서 용량을 정하고 같은 폭이면 용량을 점검하지 않으며 필요가 용량과 같으면 재할당하지 않는다', () => {
    // 필요 2050 > 2048 → ((2049>>10)+1)<<10 = 3072. 지운 폭(2046)을 뺀 값이라야 한다(반으로 줄인 값으로 계산하면 4096).
    const r = rig(); r.buffer.insert('x'.repeat(2047)); r.buffer.select(0, 2046); r.buffer.insert('y'.repeat(2049)); assert.equal(r.state.capacity, 3072); assert.equal(r.state.length, 2050);
    // 같은 폭의 덮어쓰기는 용량 점검을 하지 않는다(필드 용량이 작아도 그대로).
    const same = rig(); same.buffer.insert('abc'); same.state.capacity = 2; same.buffer.select(0, 1); same.buffer.insert('Z'); assert.equal(same.state.capacity, 2); assert.equal(same.buffer.text(), 'Zbc');
    // 필요가 용량과 같으면(물리 할당이 충분할 때) 이동하지 않고 NUL 까지 쓴다.
    const exact = direct(); exact.buffer.construct(4096); const first = exact.state.data; exact.buffer.insert('x'.repeat(2048)); assert.equal(exact.state.data, first); assert.equal(exact.state.capacity, 2048); assert.equal(exact.state.length, 2048);
});

test('reset: 삽입이 재할당 실패로 돌아와도 첫 NUL·길이·선택·dirty 초기화는 남는다', () => {
    const bad = direct({ resize: () => 0 }); bad.buffer.construct(); bad.buffer.insert('abc'); bad.state.dirty = 0;
    bad.buffer.reset('x'.repeat(3000)); assert.equal(bad.state.length, 0); assert.equal(bad.buffer.data()[0], 0); assert.equal(bad.buffer.text(), ''); assert.deepEqual([bad.state.selectionBegin, bad.state.selectionEnd, bad.state.dirty], [0, 0, 1]);
});

test('ensureLines: 본문 주소가 0 이거나 NUL 없이 범위가 끝나면 RangeError 이고 줄표 마지막 칸(127번)도 정확히 4바이트로 읽고 쓴다', () => {
    const r = rig(); r.buffer.insert('abc'); r.state.dirty = 1; r.state.data = 0; assert.throws(() => r.buffer.ensureLines(), { name: 'RangeError', message: /줄검색NUL범위/ });
    const full = rig(); full.buffer.insert('x'.repeat(2047)); full.buffer.data()[2047] = 0x41; full.state.dirty = 1; assert.throws(() => full.buffer.ensureLines(), { name: 'RangeError', message: /줄검색NUL범위/ });
    // 126개 LF → 127줄: 줄 수 127 < 128 이라 확장 없이 마지막 length 칸이 줄표의 127번째(마지막 4바이트)다.
    const edge = rig(); edge.buffer.insert(`${'a\n'.repeat(126)}b`); assert.equal(edge.buffer.ensureLines(), true); assert.equal(edge.state.lineCount, 127); assert.equal(edge.state.offsetCapacity, 128);
    assert.equal(edge.buffer.lineLength(126), 1); assert.equal(edge.buffer.lineLength(125), 2); assert.equal(dwords(edge.storage, edge.state.offsets, 128)[127], 253);
});

test('lineLength: 줄 offset DWORD 는 정렬되지 않은 4바이트를 little-endian 으로 읽으며 16/24비트 위 길이도 다룬다', () => {
    // 65 536 바이트가 넘는 첫 줄(offset 하위 3바이트 사용).
    const mid = rig(); const middle = new Uint8Array(70000 + 3).fill(0x78); middle[70000] = 0x0a; middle[70001] = 0x7a; middle[70002] = 0;
    mid.buffer.insert(middle); assert.equal(mid.buffer.lineLength(0), 70001); assert.equal(mid.buffer.lineLength(1), 1); assert.equal(mid.buffer.lineCount(), 2);
    // 0x01020304(16 909 060)바이트의 첫 줄: offset 상위 바이트까지 서로 다르게 읽는다(네 바이트가 1,2,3,4).
    const wide = rig(); const total = 0x01020303; const bytes = new Uint8Array(total + 3).fill(0x78); bytes[total] = 0x0a; bytes[total + 1] = 0x7a; bytes[total + 2] = 0;
    wide.buffer.insert(bytes); assert.equal(wide.buffer.lineLength(0), 0x01020304); assert.equal(wide.buffer.lineLength(1), 1); assert.equal(wide.state.length, total + 2);
    // 줄표의 둘째 칸에 정확한 DWORD 가 쓰였다.
    assert.equal(dwords(wide.storage, wide.state.offsets, 3)[1], 0x01020304);
});

test('copyLine: 선택이 행 끝/시작과 정확히 맞닿는 경계의 선택 쌍을 원문 부등호 그대로 계산한다', () => {
    const r = rig(); r.buffer.insert('ab\ncd\nef'); const out = new Uint8Array(16); const sel = [9, 9];
    // 선택 시작이 행 0 의 끝(3)과 같으면 그 행에는 선택이 없다(-1,-1). 선택 끝이 행 1 의 시작(3)과 같으면 행 1 은 [0,0].
    r.buffer.select(3, 5); r.buffer.copyLine(0, out, sel); assert.deepEqual(sel, [-1, -1]); r.buffer.copyLine(1, out, sel); assert.deepEqual(sel, [3 - 3, 5 - 3]);
    r.buffer.select(1, 3); r.buffer.copyLine(1, out, sel); assert.deepEqual(sel, [0, 0]); r.buffer.copyLine(0, out, sel); assert.deepEqual(sel, [1, 3]);
});

test('paste: clipboard 장치에는 256바이트 버퍼를 주고, 제외문자가 있으면 255자 본문도 마지막 NUL 만 남기고 그대로 넣는다', () => {
    const lengths = []; const clip = { text: 'm'.repeat(255) };
    const storage = new TextEntryStorage((bytes) => { lengths.push(bytes.length); bytes.fill(0); bytes.set(Buffer.from(`${clip.text}\0`, 'latin1')); }, () => {});
    const buffer = storage.create(1); buffer.construct(); buffer.paste('-'); assert.deepEqual(lengths, [256]); assert.equal(buffer.state().length, 255); assert.equal(buffer.text(), 'm'.repeat(255));
    // 제외문자 처리 뒤 마지막 칸은 NUL 이어야 하므로 clipboard 가 마지막 칸에 쓴 값은 지워진다.
    const dirty = new TextEntryStorage((bytes) => { bytes.set(Buffer.from('ab\0', 'latin1')); bytes[255] = 0x41; }, () => {}); const other = dirty.create(1); other.construct(); other.paste('-'); assert.equal(other.text(), 'ab');
});

test('text: 8192바이트를 넘는 본문도 나눠서 빠짐없이 바이트 문자열로 만든다', () => {
    const r = rig(); const pattern = Array.from({ length: 20000 }, (_, index) => String.fromCharCode(0x41 + (index % 26))).join(''); r.buffer.insert(pattern); const text = r.buffer.text(); assert.equal(text.length, 20000); assert.equal(text, pattern);
});
