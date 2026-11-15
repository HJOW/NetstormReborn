// Sun/Rain 아비어리 수신의 선택 필드 decoder raw-aviary-process-data.js(readAviaryProcessData)를 cpppj 계약으로 대조한다(WB04-E530).
//  - 계약(cpppj RawAviaryProcessData.cpp): 9BYTE 접두 부족 거부, dirty 비트 0(부모 DWORD)·1(fields[16..23] 8BYTE)·2(fields[24..27] 4BYTE)의 순서/폭, 부모 DWORD 는 반환으로만 조립하고 outer 이미지에는 쓰지 않음,
//    선택 필드 부족과 실제 저장 없는 선택 필드는 쓰기 전/후의 부분 효과를 보존한 명시 오류, dirty 가 없으면 빈 fields 도 오류가 아니다(Native NULL this).
// 이 단위에는 전용 원본 x86 관측이 없다(cpppj 는 `RawCampaignSession` 수신 경로로만 소비한다). 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E530)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readAviaryProcessData } from '../www/core/raw-aviary-process-data.js';

// 접두 8BYTE 와 dirty 1BYTE 뒤에 선택 필드를 이어 붙인 패킷을 만든다. (2026-11-15 WB04-E530)
const makePacket = (dirty, ...fields) => Uint8Array.from([0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, dirty, ...fields.flat()]);

test('아비어리 수신: dirty 비트마다 부모 DWORD·8BYTE·4BYTE 를 순서대로 읽고 필요한 필드만 outer 이미지에 쓴다', () => {
    const wide = [1, 2, 3, 4, 5, 6, 7, 8];
    const narrow = [9, 10, 11, 12];
    // 부모 + 8BYTE + 4BYTE: 부모는 반환으로만 나오고 fields 에는 쓰지 않는다.
    const fields = new Uint8Array(48).fill(0xee);
    assert.equal(readAviaryProcessData(fields, makePacket(7, [0x78, 0x56, 0x34, 0x12], wide, narrow)), 0x12345678);
    assert.deepEqual(Array.from(fields.subarray(16, 24)), wide);
    assert.deepEqual(Array.from(fields.subarray(24, 28)), narrow);
    // 그 밖의 바이트는 보존한다.
    assert.ok(fields.subarray(0, 16).every((byte) => byte === 0xee) && fields.subarray(28).every((byte) => byte === 0xee));
    // 부모 없이 4BYTE 만: 이어 붙은 위치가 앞당겨진다.
    const only = new Uint8Array(48).fill(0xee);
    assert.equal(readAviaryProcessData(only, makePacket(4, narrow)), 0);
    assert.deepEqual(Array.from(only.subarray(24, 28)), narrow);
    assert.ok(only.subarray(16, 24).every((byte) => byte === 0xee));
    // 최상위 비트의 부모 DWORD 는 부호 없는 값이다.
    assert.equal(readAviaryProcessData(new Uint8Array(48), makePacket(1, [0xff, 0xff, 0xff, 0xff])), 0xffffffff);
    // dirty 의 다른 비트는 무시하며 빈 fields 도 선택 필드가 없으면 오류가 아니다.
    assert.equal(readAviaryProcessData(new Uint8Array(0), makePacket(0xf8)), 0);
});

test('아비어리 수신: 접두/선택 필드 부족과 실제 저장 없는 필드는 거부하고 앞선 부분 쓰기는 되돌리지 않는다', () => {
    assert.throws(() => readAviaryProcessData(new Uint8Array(48), new Uint8Array(8)), RangeError);
    // 부모 4BYTE 가 부족하다.
    assert.throws(() => readAviaryProcessData(new Uint8Array(48), makePacket(1, [1, 2, 3])), RangeError);
    // 8BYTE 복사 뒤 4BYTE 가 부족하면 앞선 8BYTE 쓰기는 남는다.
    const fields = new Uint8Array(48).fill(0xee);
    assert.throws(() => readAviaryProcessData(fields, makePacket(6, [1, 2, 3, 4, 5, 6, 7, 8], [9, 10])), RangeError);
    assert.deepEqual(Array.from(fields.subarray(16, 24)), [1, 2, 3, 4, 5, 6, 7, 8]);
    assert.ok(fields.subarray(24, 28).every((byte) => byte === 0xee));
    // 객체 이미지가 선택 필드의 위치까지 없으면 명시 거부한다(쓰기 없음).
    const small = new Uint8Array(20).fill(0xee);
    assert.throws(() => readAviaryProcessData(small, makePacket(2, [1, 2, 3, 4, 5, 6, 7, 8])), /실제 객체 필드 미연결/);
    assert.ok(small.every((byte) => byte === 0xee));
    assert.throws(() => readAviaryProcessData(new Uint8Array(0), makePacket(4, [9, 10, 11, 12])), /실제 객체 필드 미연결/);
});
