// Client 임시 문자열 고리의 실제 물리·예약과 이전 할당 수명을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientStaticStrings } from '../www/core/client-static-strings.js';
import { RawFileName } from '../www/core/raw-file-name.js';

// 좁은 NUL 종료 뷰를 현재 바이트까지 읽는다.
function text(bytes) { const end = bytes.indexOf(0); assert.notEqual(end, -1); return String.fromCharCode(...bytes.subarray(0, end)); }

// 지연 8192와 명시 10240, 예약 뒤 물리와 이전 할당을 구별한다.
test('Client 임시 고리: 지연 할당·재초기화와 별도 파일 이름', () => {
    const names = new RawFileName(), diagnostics = [], owner = new ClientStaticStrings(names, (line) => diagnostics.push(line));
    assert.equal(owner.fileNames(), names); assert.equal(owner.globals().base, 0);
    const first = owner.reserve(30); assert.equal(owner.globals().size, 8192); assert.equal(first.length, 8191); assert.equal(first[30], 0xd5);
    first[0] = 81; first[1] = 0; owner.reserve(8160); const wrapped = owner.reserve(30);
    assert.equal(wrapped.buffer, first.buffer); assert.equal(wrapped.byteOffset, first.byteOffset); assert.equal(text(wrapped), 'Q');
    owner.initialize(10240); assert.deepEqual(diagnostics, [17]); assert.equal(owner.globals().size, 10240); assert.equal(text(first), 'Q');
    assert.notEqual(owner.reserve(30).buffer, first.buffer); assert.equal(owner.fileNames(), names);
});

// 원본의 80 예약은 출력 절단이 아니며 물리 끝에서는 앞선 쓰기를 유지한다.
test('Client 임시 고리: 긴 캡처 안내와 물리 범위', () => {
    const owner = new ClientStaticStrings(); owner.initialize(10240);
    const first = 'a'.repeat(90), last = 'b'.repeat(91), result = owner.formatCaptureRange(first, last);
    assert.equal(text(result), `Wrote ${first} through ${last}.\n`); assert.equal(result[80], 97);
    const small = new ClientStaticStrings(); small.initialize(100);
    assert.throws(() => small.formatCaptureRange('a'.repeat(150), 'b'), /실제 공유 고리 물리/);
    assert.equal(small.globals().last, small.globals().base + 1); assert.equal(small.globals().current, small.globals().base + 82);
});
