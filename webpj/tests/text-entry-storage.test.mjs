// 문자 편집 저장소(TextEntryStorage)의 가상 주소 할당/조회/재할당과 편집 버퍼 객체 수명을 기준선 소스 계약과 손 계산 주소로 검증한다. (2026-11-14 WB04-E438)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(TextEntryStorage.cpp)에서 옮겼다. 웹 Uint8Array 는 0 으로 채워지므로 미초기화 내용에는 의존하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { TextEntryStorage } from '../www/ui/text-entry-storage.js';

test('allocate: 첫 주소는 0x20000000 이고 다음 주소는 (요청+31)을 16 배수로 내린 간격만큼 단조 증가한다(0바이트도 16)', () => {
    const storage = new TextEntryStorage();
    // 100→128, 0→16, 1→32, 16→32, 17→48.
    const addresses = [100, 0, 1, 16, 17, 5].map((size) => storage.allocate(size));
    assert.deepEqual(addresses, [0x20000000, 0x20000080, 0x20000090, 0x200000b0, 0x200000d0, 0x20000100]); assert.equal(storage.allocationCount(), 6);
});

test('allocate: 간격 범위가 0x60000000 에 닿으면 Error(주소 범위 소진)이고 그 앞 요청은 거부하지 않는다', () => {
    const storage = new TextEntryStorage();
    assert.throws(() => storage.allocate(0x3fffffe1), { name: 'Error', message: /주소 범위 소진/ }); assert.throws(() => storage.allocate(0xffffffff), /주소 범위 소진/);
    // 소진 오류는 상태를 바꾸지 않는다: 다음 할당은 여전히 첫 주소다.
    assert.equal(storage.allocate(8), 0x20000000); assert.equal(storage.allocationCount(), 1);
});

test('bytes: 주소+오프셋 뒤 남은 범위를 같은 물리에서 빌리고 할당 끝은 빈 범위, 그 너머/빈틈/앞 주소는 RangeError 다', () => {
    const storage = new TextEntryStorage(); const first = storage.allocate(100); const zero = storage.allocate(0); const second = storage.allocate(40);
    const whole = storage.bytes(first); assert.equal(whole.length, 100); whole[5] = 0x77; assert.equal(storage.bytes(first + 5)[0], 0x77); assert.equal(storage.bytes(first + 5).length, 95);
    storage.bytes(first + 99)[0] = 0x12; assert.equal(whole[99], 0x12);
    // 끝 주소(first+100)는 빈 범위이고 한 칸 더는 간격 안이라 오프셋 범위 오류다.
    assert.equal(storage.bytes(first + 100).length, 0); assert.throws(() => storage.bytes(first + 101), { name: 'RangeError', message: /오프셋 범위/ });
    assert.equal(storage.bytes(zero).length, 0); assert.throws(() => storage.bytes(zero + 1), RangeError); assert.equal(storage.bytes(second + 39).length, 1);
    assert.throws(() => storage.bytes(0x1fffffff), { name: 'RangeError', message: /주소 없음/ }); assert.throws(() => storage.bytes(0), /주소 없음/);
    // 이 둘의 간격(128-100=28바이트 간격)의 사이는 앞 블록 기준 오프셋 오류다.
    assert.throws(() => storage.bytes(first + 127), /오프셋 범위/);
});

test('release: null 은 무시하고 시작 주소만 해제하며 내부 오프셋/이중 해제/없는 주소는 Error 다', () => {
    const storage = new TextEntryStorage(); const a = storage.allocate(64); const b = storage.allocate(64);
    storage.release(0); assert.equal(storage.allocationCount(), 2);
    assert.throws(() => storage.release(a + 1), { name: 'Error', message: /주소 불일치/ }); assert.equal(storage.allocationCount(), 2);
    storage.release(a); assert.equal(storage.allocationCount(), 1); assert.throws(() => storage.release(a), /주소 불일치/);
    // 해제한 주소는 더 이상 조회되지 않고 뒤 블록은 그대로다.
    assert.throws(() => storage.bytes(a), RangeError); assert.equal(storage.bytes(b).length, 64); storage.release(b); assert.equal(storage.allocationCount(), 0);
    // 해제한 간격은 재사용하지 않는다(단조 증가).
    assert.equal(storage.allocate(8), 0x200000a0);
});

test('resize: 0 주소는 새 할당·크기 0 은 해제 뒤 0·모르는 주소는 Error 이고 이동 때 min(새,옛) 바이트를 보존하며 옛 블록을 반납한다', () => {
    const storage = new TextEntryStorage(); assert.equal(storage.resize(0, 16), 0x20000000); assert.equal(storage.allocationCount(), 1);
    const grow = storage.allocate(10); storage.bytes(grow).set([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
    const bigger = storage.resize(grow, 40); assert.notEqual(bigger, grow); assert.deepEqual(Array.from(storage.bytes(bigger).subarray(0, 10)), [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]); assert.equal(storage.bytes(bigger).length, 40); assert.throws(() => storage.bytes(grow + 5), RangeError);
    // 축소는 새 크기만큼만 보존한다.
    const smaller = storage.resize(bigger, 4); assert.deepEqual(Array.from(storage.bytes(smaller)), [1, 2, 3, 4]);
    const count = storage.allocationCount(); assert.equal(storage.resize(smaller, 0), 0); assert.equal(storage.allocationCount(), count - 1); assert.throws(() => storage.bytes(smaller), RangeError);
    assert.throws(() => storage.resize(0x20009999, 8), { name: 'Error', message: /재할당 주소 불일치/ });
});

test('create/find/releaseBuffer: 토큰 0·중복은 Error, 같은 처리기를 돌려주고 살아 있는 본문/줄표가 있으면 해제를 거부한다', () => {
    const storage = new TextEntryStorage(); assert.throws(() => storage.create(0), { name: 'Error', message: /토큰 중복/ });
    const buffer = storage.create(0x1000); assert.equal(storage.find(0x1000), buffer); assert.equal(storage.find(0x1000), buffer); assert.equal(storage.find(0x2000), null); assert.equal(storage.bufferCount(), 1);
    assert.throws(() => storage.create(0x1000), /토큰 중복/); assert.equal(storage.bufferCount(), 1);
    // 생성 전(본문 없음)은 해제 가능하고 construct 뒤에는 거부한다. destroy 뒤에 해제할 수 있다.
    buffer.construct(); assert.throws(() => storage.releaseBuffer(0x1000), { name: 'Error', message: /본문 반납 전/ }); assert.equal(storage.bufferCount(), 1);
    assert.equal(storage.allocationCount(), 2); buffer.destroy(); assert.equal(storage.allocationCount(), 0); storage.releaseBuffer(0x1000); assert.equal(storage.bufferCount(), 0); assert.equal(storage.find(0x1000), null);
    assert.throws(() => storage.releaseBuffer(0x1000), /본문 반납 전/);
    const fresh = storage.create(0x3000); storage.releaseBuffer(0x3000); assert.equal(storage.bufferCount(), 0); assert.notEqual(fresh, null);
});

test('clipboard 장치는 실제 입력 분기에서만 요구하고 없으면 Error, 있으면 같은 바이트를 그대로 넘긴다', () => {
    const none = new TextEntryStorage(); const buffer = none.create(1); buffer.construct(); buffer.insert('abc');
    assert.throws(() => buffer.paste(), { name: 'Error', message: /읽기 미연결/ }); assert.throws(() => buffer.copy(), { name: 'Error', message: /쓰기 미연결/ });
    // 빈 본문 복사는 장치를 요구하지 않는다.
    const empty = none.create(2); empty.construct(); empty.copy();
    const written = []; const storage = new TextEntryStorage((bytes) => { bytes.set(Buffer.from('xyz\0')); }, (bytes) => written.push(Buffer.from(bytes).toString('latin1')));
    const live = storage.create(5); live.construct(); live.paste(); assert.equal(live.text(), 'xyz'); live.copy(); assert.deepEqual(written, ['xyz']);
});

test('releaseBuffer: 본문(data)이나 줄표(offsets) 중 하나라도 살아 있으면 해제를 거부한다', () => {
    const storage = new TextEntryStorage(); const buffer = storage.create(7); buffer.construct(); const state = buffer.state(); const offsets = state.offsets; const data = state.data;
    state.offsets = 0; assert.throws(() => storage.releaseBuffer(7), { name: 'Error', message: /본문 반납 전/ });
    state.offsets = offsets; state.data = 0; assert.throws(() => storage.releaseBuffer(7), /본문 반납 전/);
    state.offsets = 0; storage.releaseBuffer(7); assert.equal(storage.bufferCount(), 0); state.data = data;
});

test('allocate/resize: 실제 확보 실패(RangeError)는 0 이며 주소를 소모하지 않고 원래 할당을 유지하고, 그 밖의 오류는 그대로 던진다', () => {
    const Native = globalThis.Uint8Array;
    // 특정 크기의 확보만 실패시키는 대역이다(엔진의 OOM 을 재현한다).
    class Failing extends Native { constructor(...args) { if (args[0] === 12345) throw new RangeError('Array buffer allocation failed'); if (args[0] === 54321) throw new TypeError('다른 오류'); super(...args); } }
    const storage = new TextEntryStorage(); const first = storage.allocate(8); storage.bytes(first)[0] = 0x42;
    globalThis.Uint8Array = Failing;
    try {
        assert.equal(storage.allocate(12345), 0); assert.equal(storage.allocationCount(), 1); assert.throws(() => storage.allocate(54321), TypeError);
        // 재할당 실패: 0 을 돌려주고 원래 블록/내용을 보존한다.
        assert.equal(storage.resize(first, 12345), 0); assert.equal(storage.allocationCount(), 1); assert.equal(storage.bytes(first)[0], 0x42);
    } finally { globalThis.Uint8Array = Native; }
    // 실패는 주소를 쓰지 않았다: 다음 할당은 첫 블록(간격 32) 바로 다음 칸이다.
    assert.equal(storage.allocate(8), first + 32);
});
