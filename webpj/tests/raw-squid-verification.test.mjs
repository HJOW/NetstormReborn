// 일반 Pool의 검증 간격·진단 복귀·현재 바이트 재조회만 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { advanceSquidVerification, beginSquidVerification, receiveFreeSquidVerification } from '../www/core/raw-squid-verification.js';

// 4bh는 선행 진단이 바꾼 패킷을 읽고 최초 이전 SID를 같은 DWORD에 보관한다.
test('객체 검증 4bh: 진단 복귀와 최초 간격', () => {
    const pool = new SidPool(24000, { server: true }), state = { sid: 5, previous: 0 }, packet = Uint8Array.of(1, 0x4b, 6, 0, 0x98, 0x3a), calls = [];
    beginSquidVerification(pool, packet, state, { assertion: (...args) => { calls.push(args); new DataView(packet.buffer).setUint16(4, 15001, true); } });
    assert.deepEqual(calls, [['!sidToVerify', '.\\Clientboard.cpp', 0x4c]]); assert.deepEqual(state, { sid: 15001, previous: 15001 });
});

// 각 로그 효과 뒤 같은 슬롯의 현재 TYPE을 다시 읽고 끝 SID는 건너뛴다.
test('객체 검증 간격: 현재 슬롯과 wrap', () => {
    const pool = new SidPool(24000, { server: true }), raw = pool.physical(15001), state = { sid: 15003, previous: 15000 }, logs = [];
    raw[10] = 80; raw[11] = 0; const next = pool.physical(15002); next[10] = 0; next[11] = 1;
    advanceSquidVerification(pool, state, { typeName: () => 'Ground', log: (level, text) => { logs.push([level, text]); if (logs.length === 1) raw[10] = 81; } });
    assert.deepEqual(logs, [[0x1d, 'VERIFY 15001 (client type=Ground): on server is free||form, but not on client 1 != 0\n'], [0x1d, 'VERIFY 15001 (client type=Ground): server type != client type 0 != 81\n']]);
    assert.equal(state.previous, 15003); state.previous = 65534; state.sid = 15000; advanceSquidVerification(pool, state, {}); assert.equal(state.previous, 15000);
});

// 4ah의 첫 로그 뒤 현재 패킷과 슬롯을 읽고 간격을 반영한 뒤 sid만 비운다.
test('객체 검증 4ah: 현재 값과 SID 정리', () => {
    const pool = new SidPool(24000, { server: true }), raw = pool.physical(15004), state = { sid: 0, previous: 0 }, packet = Uint8Array.of(1, 0x4a, 7, 0, 0x9c, 0x3a, 81), logs = [];
    raw[10] = 80; raw[11] = 0;
    receiveFreeSquidVerification(pool, packet, state, { typeName: () => 'Ground', log: (_level, text) => { logs.push(text); if (logs.length === 1) { raw[10] = 81; packet[6] = 82; } } });
    assert.deepEqual(logs, ['VERIFY 15004 (client type=Ground): isFree 1 != 0\n', 'VERIFY 15004 (client type=Ground): freeType 82 != 81\n']);
    assert.deepEqual(state, { sid: 0, previous: 15004 });
});
