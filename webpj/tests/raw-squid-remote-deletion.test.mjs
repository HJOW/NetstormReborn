// 일반 Pool의 원격 삭제 범위·현재 패킷 재조회·부분 효과만 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { receiveRemoteDeletedSquid, receiveRemoteDeletedSquids, observeRemoteDeletedSquids } from '../www/core/raw-squid-remote-deletion.js';

// 단일 삭제는 예약 SID를 별도로 차단하지 않으며 범위 밖과 free만 건너뛴다.
test('원격 삭제 53h: 현재 Pool과 flags 0', () => {
    const pool = new SidPool(24000, { server: true }), sid = pool.allocate(), packet = new Uint8Array(6), view = new DataView(packet.buffer), calls = [];
    view.setUint16(4, sid, true); receiveRemoteDeletedSquid(pool, packet, (...args) => calls.push(args)); assert.deepEqual(calls, [[sid, 0]]);
    pool.physical(sid)[11] |= 1; receiveRemoteDeletedSquid(pool, packet, (...args) => calls.push(args));
    pool.physical(1)[11] &= 0xfe; view.setUint16(4, 1, true); receiveRemoteDeletedSquid(pool, packet, (...args) => calls.push(args)); assert.deepEqual(calls[1], [1, 0]);
    view.setUint16(4, 0, true); receiveRemoteDeletedSquid(pool, packet); view.setUint16(4, 24000, true); receiveRemoteDeletedSquid(pool, packet);
    assert.equal(calls.length, 2); assert.throws(() => receiveRemoteDeletedSquid(pool, packet.subarray(0, 5)), /WORD/);
});

// 삭제 callback 뒤 바뀐 다음 SID와 count를 다시 읽고 뒤 필드 부족은 앞 삭제를 보존한다.
test('원격 삭제 62h: callback 뒤 현재 패킷과 부분 효과', () => {
    const pool = new SidPool(24000, { server: true }), first = pool.allocate(), second = pool.allocate(), third = pool.allocate(), packet = new Uint8Array(12), view = new DataView(packet.buffer), calls = [];
    view.setInt32(4, 2, true); view.setUint16(8, first, true); view.setUint16(10, second, true);
    receiveRemoteDeletedSquids(pool, packet, (sid, flags) => { calls.push([sid, flags]); if (sid === first) view.setUint16(10, third, true); });
    assert.deepEqual(calls, [[first, 0], [third, 0]]); calls.length = 0; view.setInt32(4, 3, true);
    assert.throws(() => receiveRemoteDeletedSquids(pool, packet, (sid) => calls.push(sid)), /WORD/); assert.deepEqual(calls, [first, third]);
    view.setInt32(4, -2, true); receiveRemoteDeletedSquids(pool, packet); assert.equal(calls.length, 2);
    view.setInt32(4, 2, true); calls.length = 0;
    receiveRemoteDeletedSquids(pool, packet, (sid) => { calls.push(sid); view.setInt32(4, 1, true); }); assert.deepEqual(calls, [first]);
});

// 송신만 count -1을 진단하고 callback 뒤의 현재 하위 WORD로 길이를 갱신한다.
test('원격 삭제 62h: 송신 관찰기의 현재 count와 WORD 길이', () => {
    const packet = new Uint8Array(8), view = new DataView(packet.buffer), calls = []; view.setInt32(4, -1, true);
    observeRemoteDeletedSquids(packet, false); assert.equal(view.getUint16(2, true), 0);
    observeRemoteDeletedSquids(packet, true, (...args) => { calls.push(args); view.setInt32(4, 3, true); });
    assert.deepEqual(calls, [['count != (int)-1', '\\Ns\\O\\_p_packets.cpp', 0x9ac]]); assert.equal(view.getUint16(2, true), 14);
});
