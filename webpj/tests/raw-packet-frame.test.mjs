// 공통 프레임의 실제 분배 순서와 지연 연결의 선택 경계를 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { PacketConnection, RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { dispatchPacketFrame } from '../www/core/raw-packet-frame.js';

// 같은 연결에 시계·기록·종료 효과를 관찰할 장치를 제공한다.
function setup() {
    const events = [], hooks = { now: () => 12, record: (_connection, bytes) => events.push(['record', bytes[1]]),
        terminate: () => events.push('terminate'), close: () => events.push('close'), release: () => events.push('release') };
    return { events, hooks, dispatch: new RawPacketDispatch(hooks), connection: new PacketConnection({ sender: 3 }) };
}

// 기록과 관찰기가 등록을 교체해도 원래 타입을 유지하며 수신기는 최신 슬롯을 사용한다.
test('공통 프레임: 집합의 기록·현재 관찰기·현재 수신기 순서', () => {
    const { events, hooks, dispatch, connection } = setup();
    dispatch.requireLength(10, 4); dispatch.version(10, 1, 0);
    dispatch.register(10, () => events.push('old'));
    dispatch.observePacket(10, (bytes) => { events.push(['observer', bytes[1]]); bytes[1] = 11; dispatch.register(10, () => events.push('new')); });
    const frame = Uint8Array.of(1, 4, 8, 0, 1, 10, 4, 0);
    assert.equal(dispatchPacketFrame(dispatch, connection, frame, hooks), true);
    assert.deepEqual(events, [['record', 10], ['observer', 10], 'new']);
    assert.equal(frame[5], 11); assert.equal(connection.received, 12); assert.equal(connection.depth, 0);
});

// 지연 수신기 없는 집합은 허용하지만 선택된 수신기는 FIFO 없이 실행하지 않는다.
test('공통 프레임: 지연 연결에서 수신기 선택과 잘못된 길이', () => {
    const { events, hooks, dispatch, connection } = setup(); connection.delayed = true;
    assert.equal(dispatchPacketFrame(dispatch, connection, Uint8Array.of(1, 4, 8, 0, 0, 6, 4, 0), hooks), true);
    assert.equal(connection.heartbeat, 12); assert.deepEqual(events, [['record', 6]]);
    dispatch.register(6, () => {});
    assert.throws(() => dispatchPacketFrame(dispatch, connection, Uint8Array.of(1, 4, 8, 0, 0, 6, 4, 0), hooks), /FIFO 미연결/);
    assert.equal(connection.depth, 0);
    assert.equal(dispatchPacketFrame(dispatch, connection, Uint8Array.of(1, 4, 9, 0, 0, 6, 4, 0), hooks), false);
    assert.equal(events.at(-1), 'terminate');
});
