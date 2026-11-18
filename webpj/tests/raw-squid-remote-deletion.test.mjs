// 일반 Pool의 원격 삭제 범위·현재 패킷 재조회·부분 효과만 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { receiveRemoteDeletedSquid, receiveRemoteDeletedSquids, observeRemoteDeletedSquids, receiveContentSquid, receiveDependencySquid } from '../www/core/raw-squid-remote-deletion.js';

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

// 일반 Form 자료의 현재 Pool 필드와 첫 Pop·부착 순서를 확인한다.
test('내용물 2dh: 현재 필드·첫 Pop·부착', () => {
    const pool = new SidPool(24000, { server: true }), sid = pool.allocate(), raw = pool.physical(sid), packet = new Uint8Array(18), view = new DataView(packet.buffer), calls = [];
    packet.set([1, 0x2d, 18, 0]); view.setUint16(4, sid, true); view.setUint16(6, 0x1234, true);
    view.setUint16(8, 11, true); view.setUint16(10, 22, true); view.setUint16(12, 33, true); view.setUint16(14, 44, true); packet[16] = 0x7a; packet[17] = 0x10;
    receiveContentSquid(pool, packet, {
        contentType: () => 6, take: (type, selected) => { calls.push(['take', type, selected]); return sid; },
        word: (object, value) => { calls.push(['word', object, value]); }, firstPop: (object) => { calls.push(['firstPop', object]); return 0x40; },
        attach: (object, parent, flags) => calls.push(['attach', object, parent, flags]),
    });
    const result = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
    assert.deepEqual([result.getUint16(18, true), result.getUint16(20, true), result.getUint16(24, true), raw[23], raw[11]], [11, 22, 33, 0x7a, 4]);
    assert.deepEqual(calls, [['take', 6, sid], ['word', sid, 0x1234], ['firstPop', sid], ['attach', sid, 44, 0x48]]);
});

// 패킷에 첫 Pop 표식이 없어도 Take 전의 슬롯이 비어 있었다면 초기화한다.
test('내용물 2dh: Take 전의 빈 상태에 따른 첫 Pop', () => {
    const pool = new SidPool(24000, { server: true });
    const sid = pool.layout.serverFirst;
    const packet = Uint8Array.of(1, 0x2d, 18, 0, sid & 0xff, sid >>> 8, 0, 0, 0, 0, 0, 0, 0, 0, 44, 0, 0, 0);
    let attachment;
    receiveContentSquid(pool, packet, {
        contentType: () => 6,
        take: () => pool.allocate(),
        word: () => {},
        firstPop: (object) => {
            assert.equal(object, sid);
            assert.equal(pool.slot(object)[11], 4);
            return 0x40;
        },
        attach: (...args) => { attachment = args; },
    });
    assert.deepEqual(attachment, [sid, 44, 0x48]);
});

// 선택한 실제 부착이 없으면 그 전의 필드 쓰기는 되돌리지 않는다.
test('내용물 2dh: 부착 누락의 부분 효과', () => {
    const pool = new SidPool(24000, { server: true }), sid = pool.allocate(), packet = new Uint8Array(18), view = new DataView(packet.buffer);
    view.setUint16(4, sid, true); view.setUint16(8, 77, true); packet[17] = 0;
    assert.throws(() => receiveContentSquid(pool, packet, {
        contentType: () => 6, take: () => sid, word: () => {},
        firstPop: () => assert.fail('이미 사용 중인 슬롯은 첫 Pop을 호출하지 않는다.'),
    }), /선택된 실제 장치/);
    assert.equal(new DataView(pool.physical(sid).buffer, pool.physical(sid).byteOffset).getUint16(18, true), 77);
});

// 감시 수신은 첫 Pop 없이 필드를 쓴 뒤 가상 부착을 호출하며 실패 시 쓴 값을 유지한다.
test('감시 65h: 필드와 부착 순서 및 부분 효과', () => {
    const pool = new SidPool(24000, { server: true }), sid = pool.allocate(), packet = new Uint8Array(13), view = new DataView(packet.buffer), calls = [];
    view.setUint16(4, sid, true); view.setUint16(6, 42, true); packet[8] = 0x20; view.setUint16(9, 123, true); view.setUint16(11, 456, true);
    const hooks = { dependType: () => 1, take: (type, selected) => { calls.push(['take', type, selected]); return sid; }, dependencyAttach: (...args) => calls.push(['attach', ...args]) };
    receiveDependencySquid(pool, packet, hooks);
    const raw = pool.slot(sid), fields = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
    assert.deepEqual([raw[11], fields.getUint16(18, true), fields.getUint16(20, true)], [0x24, 123, 456]);
    assert.deepEqual(calls, [['take', 1, sid], ['attach', sid, 42, 0]]);
    delete hooks.dependencyAttach; packet[8] = 0x40;
    assert.throws(() => receiveDependencySquid(pool, packet, hooks), /선택된 실제 장치/);
    assert.equal(pool.slot(sid)[11], 0x44);
    assert.throws(() => receiveDependencySquid(pool, packet.subarray(0, 12), hooks), /PATCH13BYTE/);
});
