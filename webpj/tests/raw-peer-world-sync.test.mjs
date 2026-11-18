// 실제 SID 풀과 송신 전역으로 월드 동기화의 순서·현재 슬롯 재조회·실패 접두를 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { SQUID_BASE_VTABLE } from '../www/core/squid-vtables.js';
import { RawSquidTransmission, createSquidTransmissionState } from '../www/core/raw-squid-transmission.js';
import { RawPeerWorldSync } from '../www/core/raw-peer-world-sync.js';

// 현재 슬롯의 타입·상태·종속 포인터와 실제 전송 가상 표를 설정한다.
function slot(pool, sid, type, child = 0, sibling = 0) {
    const raw = pool.physical(sid); raw.fill(0); const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
    view.setUint32(0, SQUID_BASE_VTABLE, true); view.setUint16(4, sibling, true); view.setUint16(6, child, true); raw[10] = type;
}

// 송신 전역과 실제 풀은 공유하고 전송 효과는 호출 순서를 관찰한다.
function scene() {
    const pool = new SidPool(23003), types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0 }));
    const state = createSquidTransmissionState(), wire = new RawSquidTransmission(pool, state), events = [];
    return { pool, types, state, wire, events };
}

// 지형 우선·자식 깊이 우선·이동 재등록과 콜백 뒤 현재 링크를 확인한다.
test('월드 동기화: 현재 종속과 두 서버 단계·이동 객체', () => {
    const { pool, types, state, wire, events } = scene();
    types[70].flags1 = 0x800; types[70].flags2 = 0x10000; types[71].flags2 = 0x10000;
    slot(pool, 15000, 71); slot(pool, 15001, 70, 15002); slot(pool, 15002, 4, 7); slot(pool, 7, 9);
    wire.sendTo = (sid, target, argument) => {
        events.push(['send', sid, target, argument, state.sameOwnerEnabled, state.clientSidAllowed]);
        if (sid === 15002) { state.syncTarget = 4; pool.physical(15002)[4] = 8; pool.physical(15002)[5] = 0; slot(pool, 8, 9); }
    };
    const sync = new RawPeerWorldSync(pool, types, state, wire, { sharedRegularType: () => 9,
        unpop: (sid, flags) => events.push(['unpop', sid, flags]), repop: (sid, flags) => events.push(['repop', sid, flags]) });
    sync.synchronize(3);
    assert.deepEqual(events, [
        ['send', 15001, 3, 1, 1, 1], ['send', 15002, 3, 1, 1, 1], ['send', 7, 4, 1, 1, 1], ['send', 8, 4, 1, 1, 1],
        ['send', 15000, 4, 1, 1, 1], ['unpop', 15000, 0], ['repop', 15000, 0], ['unpop', 15001, 0], ['repop', 15001, 0],
    ]);
    assert.equal(state.syncTarget, 4); assert.equal(state.sameOwnerEnabled, 0); assert.equal(state.clientSidAllowed, 0);
});

// 실제 전송 writer를 통해 서버 객체를 보내고 실패 시 이미 쓴 전역을 유지한다.
test('월드 동기화: 실제 전송 및 선택 장치 실패 접두', () => {
    const { pool, types, state, events } = scene(); slot(pool, 15000, 70); state.compress = 0;
    const wire = new RawSquidTransmission(pool, state, { connected: (target) => target === 3, send: (target, bytes) => events.push([target, bytes[1], bytes[4] | (bytes[5] << 8)]) });
    new RawPeerWorldSync(pool, types, state, wire).synchronize(3);
    assert.deepEqual(events, [[3, 0x2c, 15000]]); assert.equal(state.clientSidAllowed, 0);
    types[70].flags2 = 0x10000;
    assert.throws(() => new RawPeerWorldSync(pool, types, state, wire).synchronize(3), /선택된 실제 장치 미연결/);
    assert.equal(state.clientSidAllowed, 1); assert.equal(state.sameOwnerEnabled, 0);
    assert.throws(() => new RawPeerWorldSync({ edition: 'cd1072' }, types, state, wire), /10.78/);
});

// 손상된 sibling 순환은 현재 Pool 크기까지 방문한 뒤 중단하고 원문 앞선 전역을 보존한다.
test('월드 동기화: 종속 sibling 순환 진단', () => {
    const { pool, types, state, wire } = scene(); slot(pool, 15000, 70, 7); slot(pool, 7, 9, 0, 7);
    let probes = 0; wire.sendTo = () => {};
    const sync = new RawPeerWorldSync(pool, types, state, wire, { sharedRegularType: () => { probes += 1; return 0; } });
    assert.throws(() => sync.synchronize(2), /sibling 순환/);
    assert.equal(probes, pool.capacity); assert.equal(state.clientSidAllowed, 1); assert.equal(state.sameOwnerEnabled, 1); assert.equal(state.syncTarget, 2);
});
