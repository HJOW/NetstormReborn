// 공통 객체 수신의 일반 저장·검증 분기를 일반 타입과 독립 Pool로 확인한다. 사제·공중 유닛 행동은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { receiveRawSquid } from '../www/core/raw-squid-receive.js';
import { receiveSessionSquid } from '../www/core/raw-campaign-session-squid.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { ClientLocalStateGlobals } from '../www/core/client-local-state-globals.js';
import { ClientCoreDataGlobals } from '../www/core/client-core-data-globals.js';

// 같은 SID·기본 필드의 35BYTE 패킷을 만든다.
function packetFor(sid) {
    const packet = new Uint8Array(35), view = new DataView(packet.buffer);
    packet.set([1, 0x2c, 35, 0]); view.setUint16(4, sid, true); view.setFloat32(6, 2.5, true); view.setFloat32(10, 3.25, true);
    view.setUint16(14, 0x1234, true); packet[16] = 80; packet[17] = 0x10; packet[18] = 1;
    view.setUint32(19, 0x11223344, true); view.setUint32(23, 0xaabbccdd, true); view.setInt16(27, -7, true);
    packet[29] = 0x55; view.setUint32(30, 0xdeadbeef, true); packet[34] = 0xa9; return packet;
}

// 기존 물리 객체의 수신 필드·기술 경고·첫 Pop을 원문 효과 순서로 기록한다.
test('공통 객체 수신: 현재 Pool 필드와 기술 경고 순서', () => {
    const pool = new SidPool(24000, { server: true }), sid = pool.allocate(), raw = pool.physical(sid), packet = packetFor(sid), trace = [];
    raw[10] = 80; raw[40] = 6; const state = { verifySid: 0, typeWas: 9, noMakeWarnings: new Uint32Array(255) };
    receiveRawSquid(pool, packet, state, {
        take: (type, incoming) => { trace.push(['take', type, incoming]); return sid; },
        word: (object, value) => { trace.push(['word', object, value]); new DataView(raw.buffer, raw.byteOffset, raw.byteLength).setUint16(12, value, true); },
        owner: (object, owner) => { trace.push(['owner', object, owner]); raw[34] = owner; },
        flags1: (type) => { trace.push(['flags1', type]); return 0x10; }, firstPop: (object) => { trace.push(['firstPop', object]); return 0x40; },
        hasTechnology: (owner, type) => { trace.push(['기술', owner, type]); return false; }, noMake: (object) => trace.push(['안내', object]),
        markNoMake: (object) => trace.push(['표식', object]), pop: (object, x, y, flags) => trace.push(['pop', object, x, y, flags]),
    });
    const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
    assert.equal(view.getUint16(22, true), 40); assert.equal(view.getUint16(24, true), 36); assert.equal(view.getInt32(26, true), -7);
    assert.equal(view.getUint32(36, true), 0xdeadbeef); assert.equal(raw[40], 0xaf); assert.equal(raw[11], 4);
    assert.equal(state.typeWas, 0); assert.equal(state.noMakeWarnings[80], 2);
    assert.deepEqual(trace, [['take', 80, sid], ['word', sid, 0x1234], ['owner', sid, 1], ['flags1', 80], ['firstPop', sid], ['기술', 1, 80], ['안내', sid], ['표식', sid], ['pop', sid, 2.5, 3.25, 0x48]]);
});

// 검증 로그의 첫 효과가 바꾼 현재 Pool 값을 다음 비교에서 다시 읽는다.
test('공통 객체 수신: 검증 비교의 현재 값과 원본 마스크', () => {
    const pool = new SidPool(24000, { server: true }), sid = pool.allocate(), raw = pool.physical(sid), packet = packetFor(sid), logs = [];
    raw[10] = 80; raw[34] = 2; raw[11] = 0x0a; packet[16] = 81; packet[17] = 0x0a; packet[18] = 3;
    const state = { verifySid: sid, typeWas: 44, noMakeWarnings: new Uint32Array(255) };
    receiveRawSquid(pool, packet, state, {
        typeName: (current) => { assert.equal(current, sid); return 'Ground\0ignored'; },
        log: (level, message) => { assert.equal(level, 0x1d); logs.push(message); if (logs.length === 1) raw[34] = 3; },
    });
    assert.deepEqual(logs, [`VERIFY ${sid} (client type=Ground): type 81 != 80\n`, `VERIFY ${sid} (client type=Ground): deadFlag 2 != 1\n`, `VERIFY ${sid} (client type=Ground): containedFlag 8 != 1\n`]);
    assert.equal(state.verifySid, 0); assert.equal(state.typeWas, 44); assert.equal(raw[34], 3);
});

// 필요한 실제 Take가 없으면 앞선 TYPE 저장은 보존하고 후속 쓰기는 하지 않는다.
test('공통 객체 수신: 선택 장치 누락과 물리 길이 관문', () => {
    const pool = new SidPool(24000, { server: true }), sid = pool.allocate(), packet = packetFor(sid), state = { verifySid: 0, typeWas: 9, noMakeWarnings: new Uint32Array(255) };
    const before = pool.slot(sid); assert.throws(() => receiveRawSquid(pool, packet, state, {}), /take/);
    assert.equal(state.typeWas, 80); assert.deepEqual(pool.slot(sid), before);
    assert.throws(() => receiveRawSquid(pool, packet.subarray(0, 34), state, {}), /35BYTE/);
});

// 일반 타입의 현재 기술 장부·Player 이름·경고 표식과 배치 호출을 함께 확인한다.
test('Session 객체 수신: 현재 기술 장부와 Player 경고', () => {
    const pool = new SidPool(24000, { server: true }), sid = pool.allocate(), raw = pool.physical(sid), packet = packetFor(sid), events = [];
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, techBitIndex: -1, name: 'Generic' })); types[80].techBitIndex = 3;
    const aux = new GameAuxState(), local = new ClientLocalStateGlobals(), coreData = new ClientCoreDataGlobals(), players = new Uint8Array(9 * 180);
    players.set(Buffer.from('Bob\0'), 180 + 12);
    const o = {
        core: { pool, types, bridgeState: { bridgeType: 82 }, building: { notifySurface: () => { throw new Error('예상하지 않은 다리 통지'); } },
            factory: { take: (type) => { new DataView(raw.buffer, raw.byteOffset, raw.byteLength).setUint32(0, 0x501ea0, true); raw[10] = type; return sid; }, firstPopFlags: () => 0x40 } },
        gameplayGlobals: { gameAux: () => aux }, localStateGlobals: local, coreDataGlobals: coreData, players,
        ownership: { setOwner: (_sid, owner) => { raw[34] = owner; } }, objectNames: { baseName: () => 'Unit' },
        services: { player: {}, factoryTell: (text) => events.push(['안내', text]) },
        world: { storage: { assertDisplay: () => { throw new Error('예상하지 않은 진단'); } }, pop: (...args) => events.push(['배치', ...args]) },
    };
    receiveSessionSquid(o, packet);
    assert.equal(local.playerNoMakeWarnings()[80], 2); assert.equal(new DataView(players.buffer).getUint32(180 + 0x54, true) & 0x400, 0x400);
    assert.equal(aux.supportTypeWas, 0); assert.deepEqual(events, [['안내', 'PlayerNoMake(Bob,Unit)'], ['배치', sid, 2.5, 3.25, 0x48]]);
    // 같은 기술 비트가 존재하면 새 Player 안내와 표식을 만들지 않는다.
    new DataView(coreData.records().buffer).setUint32(16 + 10, 8, true); local.playerNoMakeWarnings()[80] = 0;
    new DataView(players.buffer).setUint32(180 + 0x54, 0, true); events.length = 0;
    receiveSessionSquid(o, packet);
    assert.equal(local.playerNoMakeWarnings()[80], 0); assert.equal(new DataView(players.buffer).getUint32(180 + 0x54, true), 0);
    assert.deepEqual(events, [['배치', sid, 2.5, 3.25, 0x48]]);
});
