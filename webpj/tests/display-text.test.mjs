// cpppj KoreanTextTests·JapaneseTextTests의 표시 바이트와 예외 계약을 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { DisplayTextCodec } from '../www/assets/display-text.js';

// 한국어와 일본어 정적 범위는 원본 인덱스를 유지하고 각 문자마다 다른 시퀀스를 쓴다.
test('표시 부호화: 정적 범위 전체와 기존 1252 문자', () => {
    const codec = new DisplayTextCodec(), seen = new Set(), oracle = [];
    assert.deepEqual([...codec.encode('가각')], [31, 1, 1, 1, 31, 1, 1, 2]);
    assert.deepEqual([...codec.encode('Aé€')], [65, 233, 128]);
    const ranges = [[0xac00, 0xd7a3], [0x3131, 0x318e], [0x3000, 0x303f], [0xff01, 0xff5e], [0x3040, 0x30ff], [0xff61, 0xff9f]];
    // cpppj 정적 범위의 모든 문자에 대해 유일성과 복호화를 확인한다.
    for (const [first, last] of ranges) for (let code = first; code <= last; code += 1) {
        const text = String.fromCodePoint(code), bytes = codec.encode(text), key = bytes.join(',');
        assert.equal(bytes.length, 4); assert.equal(bytes[0], 31); assert.equal(codec.decode(bytes), text);
        assert.equal(seen.has(key), false); seen.add(key);
        oracle.push(`${code.toString(16)}\t${Buffer.from(bytes).toString('hex')}\n`);
    }
    assert.equal(seen.size, 11679);
    // cpppj 실행기로 같은 순서의 정적 전체와 한자 네 건을 출력한 TSV의 SHA-256이다.
    for (const code of [0x65e5, 0x672c, 0x8a9e, 0x65e5]) oracle.push(`${code.toString(16)}\t${Buffer.from(codec.encode(String.fromCodePoint(code))).toString('hex')}\n`);
    assert.equal(createHash('sha256').update(oracle.join('')).digest('hex'), '148b696c447278eb1dcde270891d7ebab46fbd90d44185dc5db8c4a339f90455');
    const mixed = 'Netstorm 한국어 café €ㄱ~r！';
    assert.equal(codec.decode(codec.encode(mixed)), mixed);
    assert.equal(codec.decode(Uint8Array.of(0xef, 0xbb, 0xbf, 0xed, 0x95, 0x9c)), '한');
});

// 동적 한자는 최초 사용 순서에 따라 같은 프로세스의 바이트를 재사용하며 용량 밖은 거부한다.
test('표시 부호화: 한자 배정과 용량 경계', () => {
    const codec = new DisplayTextCodec(), first = codec.encode('日'), second = codec.encode('本');
    assert.notDeepEqual(first, second); assert.deepEqual(codec.encode('日'), first);
    assert.equal(codec.decode(Uint8Array.from([...first, ...second])), '日本');
    const other = new DisplayTextCodec();
    assert.notEqual(other.decode(first), '日');
    // 이미 배정한 두 글자를 제외하고 남은 3,944칸을 채운다.
    for (let code = 0x4e00; code < 0x4e00 + 3944; code += 1) codec.encode(String.fromCodePoint(code));
    assert.throws(() => codec.encode('鿿'), /Windows-1252/);
    assert.equal(codec.decode(first), '日');
});

// 잘린 시퀀스와 미배정 인덱스는 원문 제어문자로 보존하고 표 밖 문자는 거부한다.
test('표시 부호화: 비정상 입력과 원문 바이트 보존', () => {
    const codec = new DisplayTextCodec();
    assert.equal(codec.decode(Uint8Array.of(31, 1, 2)), '\x1f\x01\x02');
    assert.equal(codec.decode(Uint8Array.of(31, 1, 65, 3)), '\x1f\x01A\x03');
    assert.equal(codec.decode(Uint8Array.of(31, 30, 30, 30)), '\x1f\x1e\x1e\x1e');
    assert.throws(() => codec.encode('㐀'), /Windows-1252/);
    assert.throws(() => codec.encode('😀'), /BMP/);
    assert.throws(() => codec.encode('\ud800'), /BMP/);
    assert.throws(() => codec.decode([65]), /바이트/);
});
