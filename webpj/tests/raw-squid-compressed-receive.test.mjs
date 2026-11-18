// 압축 객체의 원문 필드 순서·부분 효과와 송신 관찰을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { createSquidTransmissionState } from '../www/core/raw-squid-transmission.js';
import { decodeCompressedSquid, observeCompressedSquid } from '../www/core/raw-squid-compressed-receive.js';

// 명시한 마스크와 바이트로 7ah 패킷을 만든다.
function packet(mask, data = []) {
    const bytes = new Uint8Array(9 + data.length), view = new DataView(bytes.buffer);
    bytes.set([1, 0x7a]); view.setUint16(4, 0x1234, true); view.setUint16(6, mask, true);
    bytes[8] = data.length; bytes.set(data, 9); return bytes;
}

// 현재 기본 타입의 HP와 zOrder를 조회하는 독립 장치이다.
function hooks(extra = {}) { return { defaultType: () => 70, type: (type) => ({ type, flags1: 0x10, zOrder: 9 }), maximumHp: () => 4321, assertion: () => {}, ...extra }; }

// 필드 생략은 좌표·TYPE·owner·HP 기본값을 채우고 명시 값은 순서대로 덮는다.
test('압축 객체: 기본값과 명시 필드', () => {
    const state = createSquidTransmissionState(); state.lastOwner = 3;
    const empty = decodeCompressedSquid(packet(0), state, hooks()), e = new DataView(empty.buffer);
    assert.deepEqual([e.getUint16(4, true), e.getFloat32(6, true), e.getFloat32(10, true), empty[16], empty[18], e.getUint16(27, true), empty[29]], [0x1234, -10000, -10000, 70, 3, 4321, 9]);
    const image = decodeCompressedSquid(packet(0x3b75, [12, 34, 0x56, 71, 0x80, 4, 0x77, 0x88, 0x99, 0xaa]), state, hooks()), v = new DataView(image.buffer);
    assert.deepEqual([v.getFloat32(6, true), v.getFloat32(10, true), image[14], image[16], image[17], image[18], image[27], image[29], image[30], image[34]], [12, 34, 0x56, 71, 0x90, 4, 0x77, 0x88, 0x99, 0xaa]);
    assert.equal(state.lastOwner, 4);
});

// 충돌과 길이 부족 진단은 해당 위치에서 발생하며 앞선 owner 갱신을 유지한다.
test('압축 객체: 충돌 진단과 부분 효과', () => {
    const state = createSquidTransmissionState(), calls = [];
    const image = decodeCompressedSquid(packet(0x180, [5]), state, hooks({ assertion: (_expression, _file, line) => calls.push(line) }));
    assert.equal(image[18], 5); assert.equal(state.lastOwner, 5); assert.deepEqual(calls, [0x1af, 0x1bd]);
    calls.length = 0;
    assert.throws(() => decodeCompressedSquid(packet(0x300, [7]), state, hooks({ assertion: (_expression, _file, line) => calls.push(line) })), /스택 읽기/);
    assert.equal(state.lastOwner, 7); assert.deepEqual(calls, [0x137]);
    assert.throws(() => decodeCompressedSquid(new Uint8Array(8), state, hooks()), /9BYTE/);
});

// 압축 자료는 콜백 전에 복사하며 관찰기는 진단 이후 현재 길이를 읽는다.
test('압축 객체: 지역 복사와 송신 관찰', () => {
    const state = createSquidTransmissionState(), source = packet(0x100, [9]);
    const image = decodeCompressedSquid(source, state, hooks({ type: () => { source[9] = 2; return { flags1: 0, zOrder: 0 }; } }));
    assert.equal(image[18], 9); assert.equal(state.lastOwner, 9);
    const bytes = new Uint8Array(9); bytes[8] = 255; observeCompressedSquid(bytes, false);
    observeCompressedSquid(bytes, true, (_expression, _file, line) => { assert.equal(line, 0xb83); bytes[8] = 4; });
    assert.equal(new DataView(bytes.buffer).getUint16(2, true), 13);
});
