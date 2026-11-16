// cpppj RawConstructionFailurePackets의 wire/현재 후보 순서를 대조한다(WB04-E602). 전용 원본 관측/게임 실행은 아니다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { writeConstructionMoneyPacket, writeConstructionRejectPacket, receiveConstructionReject } from '../www/core/raw-construction-failure-packets.js';
import { bitsToFloat, floatToBits, put } from './raw-support.mjs';

// 실제 Pool/Hash/Canon과 같은 바이트를 쓰는 명시 입력 장면이다.
function scene() {
    const world = { pool: new SidPool(24000, { server: false }), hash: new SquidHash(), types: Array.from({ length: 188 }, () => ({ footX: 1, footY: 1 })), frames: Array.from({ length: 188 }, () => ({ frames: [{ side: 65, variant: 80, number: 0, flags: 0 }], defaultFrame: 0 })), patternTypes: [107, 82, 94, 157, 131, 129, 140, 142], noIslandType: 157, diagnostics: null };
    // 앞선 머리를 next WORD로 보존하며 같은 실제 버킷에 후보를 넣는다.
    function node(sid, type = 100, extra = 1, x = 20, y = 21) {
        const raw = world.pool.physical(sid); raw.fill(0); put(raw, 0, 0x11010000); raw[10] = type; raw[40] = extra; raw[34] = 7;
        put(raw, 14, floatToBits(x)); put(raw, 18, floatToBits(y)); put(raw, 4, world.hash.bucket(0, x, y), 2); world.hash.setBucket(0, x, y, sid);
    }
    return { world, node, packet: writeConstructionRejectPacket({ x: 20, y: 21, type: 100, argument: 0, direction: 0 }) };
}

test('환불 패킷: 비정렬 signed 금액과 세 BYTE를 패딩 없이 쓴다', () => {
    assert.deepEqual([...writeConstructionMoneyPacket({ player: 0x102, source: -1, kind: 0x103, amount: -0x1234567 })], [1, 0x6c, 11, 0, 2, 255, 3, 0x99, 0xba, 0xdc, 0xfe]);
    // signed 양끝/0도 같은7번 필드에 그대로 기록한다.
    for (const amount of [-2147483648, -1, 0, 2147483647]) assert.equal(new DataView(writeConstructionMoneyPacket({ player: 0, source: 0, kind: 0, amount }).buffer).getInt32(7, true), amount);
});

test('취소 패킷: 좌표 단정도 비트/음수0/NaN과 하위 BYTE를 보존한다', () => {
    // 실제 float 인자의 비트를 independent DWORD 기대값과 비교한다.
    for (const bits of [0, 0x80000000, 0x3fa00000, 0xbf400000, 0x7f800000, 0xff800000, 0x7fc12345]) {
        const packet = writeConstructionRejectPacket({ x: bitsToFloat(bits), y: bitsToFloat(bits), type: 0x164, argument: -1, direction: 0x102 });
        assert.equal(packet.length, 15); assert.deepEqual([...packet.subarray(0, 4)], [1, 0x2f, 15, 0]); assert.equal(new DataView(packet.buffer).getUint32(4, true), bits); assert.equal(new DataView(packet.buffer).getUint32(8, true), bits); assert.deepEqual([...packet.subarray(12)], [100, 255, 2]);
    }
});

test('배치 취소: 첫 후보에서 멈추지 않고 서버 영역/SID1/다른 owner의 모든 matching abstract를 지운다', () => {
    const s = scene(); s.node(1); s.node(15000, 157); s.node(60); s.node(61, 101); s.node(62, 100, 0); s.node(63, 100, 9); const calls = [];
    receiveConstructionReject(s.packet, s.world, (sid, flags) => { calls.push([sid, flags]); s.world.pool.physical(sid).fill(0); });
    assert.deepEqual(calls, [[60, 8], [15000, 8], [1, 8]]);
});

test('배치 취소: 앞선 가상 삭제 뒤 현재 받침 TYPE/다음 후보 바이트를 다시 읽는다', () => {
    const s = scene(); s.node(50, 101); s.node(51, 157); s.node(52); const calls = [];
    receiveConstructionReject(s.packet, s.world, (sid, flags) => { calls.push([sid, flags]); if (sid === 52) { s.world.noIslandType = 101; s.world.pool.physical(50)[40] = 0; } });
    assert.deepEqual(calls, [[52, 8]]);
    s.world.pool.physical(50)[40] = 1; receiveConstructionReject(s.packet, s.world, (sid) => calls.push([sid, 8])); assert.deepEqual(calls.slice(1), [[52, 8], [50, 8]]);
});

test('배치 취소: 헤더 검사를 더하지 않고 비정렬15BYTE span의 실제 본문만 읽는다', () => {
    const s = scene(); s.node(50); const bytes = new Uint8Array(21).fill(0xaa); bytes.set(s.packet, 3); const packet = bytes.subarray(3, 18); packet.set([9, 9, 0, 0]); const calls = [];
    receiveConstructionReject(packet, s.world, (sid, flags) => calls.push([sid, flags])); assert.deepEqual(calls, [[50, 8]]); assert.equal(bytes[2], 0xaa); assert.equal(bytes[18], 0xaa);
});

test('배치 취소: 미선택 삭제는 요구하지 않고 선택된 누락/실패만 원문 위치에서 전파한다', () => {
    const s = scene(); receiveConstructionReject(s.packet, s.world, null); s.node(50); assert.throws(() => receiveConstructionReject(s.packet, s.world, null), /삭제 가상 미연결/);
    const stop = new Error('현재 삭제 실패'); assert.throws(() => receiveConstructionReject(s.packet, s.world, () => { throw stop; }), (error) => error === stop);
});

test('배치 취소: 짧은 패킷/다른 판본/TYPE 범위는 첫 삭제 전에 거부한다', () => {
    const s = scene(); s.node(50); const fail = () => assert.fail('삭제 전 계약'); assert.throws(() => receiveConstructionReject(s.packet.subarray(0, 14), s.world, fail), /PATCH15BYTE/); assert.throws(() => receiveConstructionReject(s.packet, { ...s.world, pool: { edition: 'cd1072' } }, fail), /PATCH15BYTE/);
    s.packet[12] = 188; assert.throws(() => receiveConstructionReject(s.packet, s.world, fail), /TYPE 범위/);
});
