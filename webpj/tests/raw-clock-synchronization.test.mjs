// 시간 동기화의 등록·초기화·연결·수신을 cpppj 225553df의 순서와 기존 패킷 분배기에 대조한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClockSynchronizationData, RawClockSynchronization } from '../www/core/raw-clock-synchronization.js';
import { PacketConnection, RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';

// 실제 웹 분배기와 Client 시간 전역을 공유하는 검사 장면을 만든다. (2026-11-15 WB04-E515)
function scene(hooks = {}) {
    const data = new ClockSynchronizationData();
    const globals = { connection: 71, replyMode: 0, gameTicks: 0x12345678 };
    const effects = { now: () => 9, record() {}, terminate() {}, close() {}, release() {} };
    const dispatch = new RawPacketDispatch(effects);
    const clock = new RawClockSynchronization(globals, data, hooks);
    return { data, globals, dispatch, clock };
}

// 32바이트 시간 패킷의 요청·서버·수신 시각을 만든다. (2026-11-15 WB04-E515)
function packet(request = 0, server = 0, received = 0) {
    const result = new Uint8Array(32);
    result[1] = 5;
    result[2] = 32;
    const view = new DataView(result.buffer);
    view.setFloat64(8, request, true);
    view.setFloat64(16, server, true);
    view.setFloat64(24, received, true);
    return result;
}

// 등록 뒤 바뀌는 값과 유지되는 표본·기한·정책을 구분한다.
test('시간 동기화: 서버 초기화는 실제 등록 후 카운터와 응답 모드만 바꾼다', () => {
    const { data, globals, dispatch, clock } = scene();
    data.sent = 5; data.received = 6; data.next = 17; data.samples[0][3] = 0xa5;
    dispatch.version(5, 7, 3); dispatch.requireLength(5, 32);
    const previous = () => {};
    dispatch.observe(5, previous);
    clock.initializeServer(dispatch);
    const entry = dispatch.registration(5);
    assert.deepEqual([data.sent, data.received, data.next, data.samples[0][3], globals.connection, globals.replyMode, globals.gameTicks], [0, 0, 17, 0xa5, 71, 1, 0x12345678]);
    assert.deepEqual([entry.version, entry.mismatch, entry.length], [7, 3, 32]);
    assert.equal(typeof entry.receive, 'function');
    assert.equal(typeof entry.observer, 'function');
    assert.notEqual(entry.observer, previous);
});

// 시계 장치 실패 시 이미 지운 표본은 유지하며 뒤쪽 전역 변경은 하지 않는다.
test('시간 동기화: 연결은 표본 전체를 지운 뒤 현재 벽시각을 조회한다', () => {
    const { data, globals, dispatch, clock } = scene();
    data.sent = 8; data.received = 9; data.next = 10;
    // 마지막 표본과 헤더까지 전부 지워지는지 서로 다른 위치를 채운다.
    for (const sample of data.samples) sample.fill(0xa5);
    assert.throws(() => clock.attach(dispatch, 55), /실제 장치 미연결/);
    assert.ok(data.samples.every((sample) => sample.every((byte) => byte === 0)));
    assert.deepEqual([data.sent, data.received, data.next, globals.replyMode, globals.connection], [8, 9, 10, 0, 71]);
    assert.equal(typeof dispatch.registration(5).receive, 'function');
    const ready = new RawClockSynchronization(globals, data, { wallSeconds: () => 23.5 });
    ready.attach(dispatch, 0xffffffff);
    assert.deepEqual([data.sent, data.received, data.next, globals.replyMode, globals.connection], [0, 0, 23.5, 0, 0xffffffff]);
});

// 송신 주소는 시각 호출 전에 복사하고 실제 전달 여부는 그 뒤 현재 flags 로 고른다.
test('시간 동기화: 서버 응답은 현재 게임 시각과 선택한 주소를 전달한다', () => {
    const sent = [];
    const connection = new PacketConnection({ flags: 0x220, address: Uint8Array.from({ length: 20 }, (_, i) => i + 1) });
    const { clock, globals } = scene({
        gameSeconds: () => { connection.address.fill(0xee); return 12.25; },
        reply: (target, bytes, address) => sent.push([target, bytes.slice(), address?.slice() ?? null]),
    });
    globals.replyMode = 1;
    const bytes = packet(4, 5, 6);
    clock.receive(connection, bytes);
    assert.equal(new DataView(bytes.buffer).getFloat64(16, true), 12.25);
    assert.equal(sent[0][0], connection);
    assert.deepEqual([...sent[0][2]], Array.from({ length: 20 }, (_, i) => i + 1));
    assert.deepEqual(sent[0][1], bytes);
    connection.flags = 0;
    clock.receive(connection, packet());
    assert.equal(sent[1][2], null);
});

// 주소 복사 후 시계 조회가 flags 를 바꿔도 원문처럼 저장한 주소와 현재 전달 관문을 쓴다.
test('시간 동기화: 시계 조회 중 바뀐 주소 전달 관문을 다시 읽는다', () => {
    const connection = new PacketConnection({ flags: 0, address: new Uint8Array(20).fill(0xa5) });
    let sent;
    const { clock, globals } = scene({
        gameSeconds: () => { connection.flags = 0x20; return 1; },
        reply: (_target, _packet, address) => { sent = address.slice(); },
    });
    globals.replyMode = 1;
    clock.receive(connection, packet());
    assert.deepEqual([...sent], [2, ...new Array(19).fill(0)]);
});

// 첫 일치만 갱신하고 수신 카운터의 DWORD 감김을 보존한다.
test('시간 동기화: 중복 요청 시각은 첫 표본만 채우고 수신 수가 감긴다', () => {
    const { data, clock } = scene({ wallSeconds: () => 21.75 });
    data.samples[0].set(packet(4, 1, 0));
    data.samples[1].set(packet(4, 2, 0));
    data.received = 0xffffffff;
    const bytes = packet(4, 9, 0);
    clock.receive(new PacketConnection(), bytes);
    assert.equal(data.received, 0);
    assert.deepEqual(data.samples[0], bytes);
    assert.equal(new DataView(data.samples[0].buffer).getFloat64(24, true), 21.75);
    assert.equal(new DataView(data.samples[1].buffer).getFloat64(16, true), 2);
});

// 일치하지 않거나 NaN 인 요청은 수신 바이트만 바꾸고 장부는 바꾸지 않는다.
test('시간 동기화: 미일치와 NaN 요청은 어떤 표본도 갱신하지 않는다', () => {
    const { data, clock } = scene({ wallSeconds: () => 33 });
    data.received = 4;
    const before = data.samples.map((sample) => sample.slice());
    // 서로 다른 두 미일치 입력을 같은 원래 표본에 전달한다.
    for (const request of [7, Number.NaN]) {
        const bytes = packet(request);
        clock.receive(new PacketConnection(), bytes);
        assert.equal(new DataView(bytes.buffer).getFloat64(24, true), 33);
        assert.equal(data.received, 4);
        assert.deepEqual(data.samples, before);
    }
});

// 실제 분배기는 수신 프레임을 보존하고 복사한 패킷을 응답 장치에 넘긴다.
test('시간 동기화: 실제 패킷 분배 등록은 복사본으로 응답한다', () => {
    const sent = [];
    const { dispatch, clock } = scene({ gameSeconds: () => 42, reply: (_connection, bytes) => sent.push(bytes.slice()) });
    dispatch.requireLength(5, 32);
    clock.initializeServer(dispatch);
    const bytes = Buffer.from(packet(3, 5, 7));
    const before = Buffer.from(bytes);
    const connection = new PacketConnection();
    assert.equal(dispatch.dispatch(connection, bytes), true);
    assert.deepEqual(bytes, before);
    assert.equal(new DataView(sent[0].buffer).getFloat64(16, true), 42);
    assert.equal(connection.depth, 0);
});

// 하위 장치가 없으면 선택된 시점에서만 중단하고 이전 쓰기는 되돌리지 않는다.
test('시간 동기화: 물리 부족과 응답 장치 누락의 순서를 보존한다', () => {
    const { clock, globals } = scene({ gameSeconds: () => 17 });
    globals.replyMode = 1;
    assert.throws(() => clock.receive(new PacketConnection(), new Uint8Array(31)), RangeError);
    const bytes = packet();
    assert.throws(() => clock.receive(new PacketConnection(), bytes), /실제 장치 미연결/);
    assert.equal(new DataView(bytes.buffer).getFloat64(16, true), 17);
});
