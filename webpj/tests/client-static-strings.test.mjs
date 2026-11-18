// Client 임시 문자열 고리의 실제 물리·예약과 이전 할당 수명을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientStaticStrings } from '../www/core/client-static-strings.js';
import { RawFileName } from '../www/core/raw-file-name.js';

// 좁은 NUL 종료 뷰를 현재 바이트까지 읽는다.
function text(bytes) { const end = bytes.indexOf(0); assert.notEqual(end, -1); return String.fromCharCode(...bytes.subarray(0, end)); }

// 원본의 좁은 서식과 종료 NUL을 만든다.
function pattern(value) { return Uint8Array.from([...value].map((character) => character.charCodeAt(0)).concat(0)); }

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

// 원본 정수 서식은 실제 고리 물리에서 긴 출력과 같은 주소의 서식 생략을 유지한다.
test('Client 임시 고리: 정수 서식·예약 초과·같은 주소 별칭', () => {
    const owner = new ClientStaticStrings(); owner.initialize(10240);
    assert.equal(text(owner.formatInteger(30, pattern('L'.repeat(40) + '%03d'), 7)), 'L'.repeat(40) + '007');
    // 각각 독립적인 C 정수 변환의 부호·기수·폭·정밀도다.
    for (const [format, value, expected] of [['%+06d', 17, '+00017'], ['% d', 7, ' 7'], ['%-5i', -3, '-3   '], ['%u', -1, '4294967295'],
        ['%#08x', 42, '0x00002a'], ['%#X', 42, '0X2A'], ['%#.0o', 0, '0'], ['%.0d', 0, ''], ['%.4u', 7, '0007'], ['%3c', 65, '  A'], ['%05c', 65, '0000A'], ['%05.3c', 65, '0000A'], ['%% %d', 3, '% 3']]) {
        assert.equal(text(owner.formatInteger(60, pattern(format), value)), expected, format);
    }
    const alias = new ClientStaticStrings(), first = alias.reserve(30); first.set(Uint8Array.of(81, 0)); alias.reserve(8160);
    const same = alias.formatInteger(30, first, 7); assert.equal(same.buffer, first.buffer); assert.equal(same.byteOffset, first.byteOffset); assert.equal(text(same), 'Q');
});

// 서식 오류는 예약 후 발생하고 실제 물리 초과는 앞선 출력 바이트만 남긴다.
test('Client 임시 고리: 정수 서식 오류와 부분 출력', () => {
    const owner = new ClientStaticStrings();
    assert.throws(() => owner.formatInteger(30, pattern('%s'), 7), /한 정수 외/);
    assert.equal(owner.globals().size, 8192); assert.equal(owner.globals().current, owner.globals().base + 32); assert.equal(owner.globals().last, owner.globals().base + 1);
    assert.throws(() => owner.formatInteger(30, pattern('%d %d'), 7), /인자 하나 초과/);
    assert.throws(() => owner.formatInteger(30, Uint8Array.of(65), 7), /NUL/);
    const small = new ClientStaticStrings(); small.initialize(100);
    assert.throws(() => small.formatInteger(30, pattern('x'.repeat(150)), 7), /실제 고리|실제 주소|물리 폭/);
    const bytes = small.reserve(30); assert.equal(bytes[0], 120);
});
