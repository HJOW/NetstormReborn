// 패킷 분배기(cpppj `o/RawPacketDispatch`)의 웹 대응 `core/raw-packet-dispatch.js` 를 검사한다(4-C, WB04-E306).
// 원본 관측 파일이 없는 단위 계약 검사이다(cpppj 시험은 분배기를 쓰는 건설/연결/로그인 시험 안에서 간접 확인): 등록 API 의 범위·touch·교체 규칙, 헤더/타입/선언 길이 검증, 닫힘·무시·종료를 가르는 버전 정책,
// 시각 기록과 타입 10 이상 기록 순서, 즉시 콜백의 연결 깊이(정상·예외 복구)와 닫힘 후처리, 등록 핀, 지연/집합 거부, 직접 호출, 건설 수신 등록, 현재 Player9 활성 조회. 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E306)
import assert from 'node:assert/strict';
import test from 'node:test';
import { PacketRegistrationTable } from '../www/core/packet-registration-table.js';
import { PacketConnection, RawPacketDispatch, packetPlayerActive, registerConstructionData } from '../www/core/raw-packet-dispatch.js';

// 헤더(버전, 타입, 선언 길이 WORD)와 뒤따르는 내용으로 패킷을 만든다. (2026-11-11 WB04-E306)
function packet(version, type, length, ...payload) {
    return Uint8Array.of(version, type, length & 0xff, length >> 8, ...payload);
}

// 효과(시계·기록·종료·소멸·해제)의 호출을 순서대로 남기는 분배기 장면이다. (2026-11-11 WB04-E306)
function scene(table = null) {
    const log = [];
    const state = { clock: 42 };
    const hooks = {
        now: () => state.clock,
        record: (connection, bytes, length) => { log.push(['record', bytes[1], length]); },
        terminate: () => { log.push(['terminate']); },
        close: () => { log.push(['close']); },
        release: () => { log.push(['release']); },
    };
    const dispatch = new RawPacketDispatch(hooks, table);
    return { dispatch, log, state, hooks, connection: new PacketConnection({ sender: 17 }) };
}

// 구성: 필수 효과 다섯과 같은 판본 표가 필요하고, 표를 주면 공유하며 안 주면 새로 만든다. (2026-11-11 WB04-E306)
test('분배기: 구성과 필수 효과', () => {
    const { hooks } = scene();
    // 필수 효과를 하나씩 빼면 거부한다.
    for (const name of ['now', 'record', 'terminate', 'close', 'release']) {
        assert.throws(() => new RawPacketDispatch({ ...hooks, [name]: undefined }), TypeError, name);
    }
    assert.throws(() => new RawPacketDispatch(null), TypeError);
    assert.throws(() => new RawPacketDispatch({ ...hooks, sent: 1 }), TypeError);
    assert.throws(() => new RawPacketDispatch(hooks, {}), TypeError);
    // 같은 표를 여러 분배기가 공유한다.
    const table = new PacketRegistrationTable();
    const first = new RawPacketDispatch(hooks, table);
    const second = new RawPacketDispatch(hooks, table);
    assert.equal(first.registrations, table);
    assert.equal(second.registrations, table);
    assert.ok(new RawPacketDispatch(hooks).registrations instanceof PacketRegistrationTable);
    assert.equal(first.hooks, hooks);
    // 연결 객체의 기본값: 주소는 20BYTE 이고 나머지는 0/거짓이다.
    const blank = new PacketConnection();
    assert.deepEqual([blank.address.length, blank.flags, blank.sender, blank.received, blank.heartbeat, blank.depth, blank.delayed], [20, 0, 0, 0, 0, 0, false]);
});

// 등록 API: 타입 0/지원 밖/비정수는 변경 전에 거부하고, 각 API 는 자기 표만 바꾸며 관찰기 두 종류는 서로 교체한다. (2026-11-11 WB04-E306)
test('분배기: 등록 API 의 범위와 교체 규칙', () => {
    const { dispatch } = scene();
    const calls = [
        (type) => dispatch.register(type, () => {}), (type) => dispatch.registerDirect(type, () => {}), (type) => dispatch.observe(type, () => {}), (type) => dispatch.observePacket(type, () => {}),
        (type) => dispatch.version(type, 1, 0), (type) => dispatch.requireLength(type, 5), (type) => dispatch.registration(type),
    ];
    // 모든 등록 API 가 같은 범위 검사를 한다.
    for (const call of calls) {
        // 타입 0·지원 밖·음수·비정수는 거부한다.
        for (const type of [0, 138, -1, 1.5]) assert.throws(() => call(type), RangeError, `${call} ${type}`);
        assert.doesNotThrow(() => call(137));
        assert.doesNotThrow(() => call(1));
    }
    // 관찰기 두 종류는 같은 원문 슬롯이므로 마지막 등록이 이전 장치를 완전히 교체한다.
    const direction = () => {};
    const withPacket = () => {};
    dispatch.observe(20, direction);
    assert.deepEqual([dispatch.registration(20).observer, dispatch.registration(20).packetObserver], [direction, null]);
    dispatch.observePacket(20, withPacket);
    assert.deepEqual([dispatch.registration(20).observer, dispatch.registration(20).packetObserver], [null, withPacket]);
    dispatch.observe(20, null);
    assert.deepEqual([dispatch.registration(20).observer, dispatch.registration(20).packetObserver], [null, null]);
    // 버전/길이는 BYTE/WORD 로 자른다.
    dispatch.version(21, 0x1ff, 0x1ff);
    dispatch.requireLength(21, 0x1ffff);
    assert.deepEqual([dispatch.registration(21).version, dispatch.registration(21).mismatch, dispatch.registration(21).length], [0xff, 0xff, 0xffff]);
    // 직접 등록은 일반 프레임 입구를 같이 설치하고 일반 등록은 직접 입구를 지운다.
    dispatch.registerDirect(22, () => {});
    assert.ok(dispatch.registration(22).receive && dispatch.registration(22).directReceive);
    dispatch.register(22, () => {});
    assert.equal(dispatch.registration(22).directReceive, null);
    dispatch.registerDirect(22, () => {});
    dispatch.registerDirect(22, null);
    assert.deepEqual([dispatch.registration(22).receive, dispatch.registration(22).directReceive], [null, null]);
    // 대여가 있으면 모든 등록 API 가 변경 칸을 기록하고 반납이 복원한다.
    const lease = dispatch.registrations.bindSession({});
    const apis = [
        (type) => dispatch.register(type, () => {}), (type) => dispatch.registerDirect(type, () => {}), (type) => dispatch.observe(type, () => {}),
        (type) => dispatch.observePacket(type, () => {}), (type) => dispatch.version(type, 7, 3), (type) => dispatch.requireLength(type, 9),
    ];
    apis.forEach((call, index) => {
        call(40 + index);
        assert.equal(dispatch.registration(40 + index).leased, true, `api ${index}`);
    });
    lease.detach();
    // 반납 뒤 각 칸이 대여 시작 값으로 돌아왔는지 본다.
    for (let index = 0; index < apis.length; index += 1) {
        const restored = dispatch.registration(40 + index);
        assert.deepEqual([restored.leased, restored.receive, restored.observer, restored.packetObserver, restored.version, restored.mismatch], [false, null, null, null, 0, 0], `restored ${index}`);
    }
});

// 검증: 헤더 4바이트, 지원 타입, 타입 10 이상은 처리기, 선언 길이는 표와 정확히 같아야 한다(0xffff 가변). (2026-11-11 WB04-E306)
test('분배기: 패킷 검증', () => {
    const { dispatch } = scene();
    assert.throws(() => dispatch.validate(Uint8Array.of(1, 2, 3)), RangeError);
    // 타입 0 과 지원 밖 타입은 거부한다.
    assert.equal(dispatch.validate(packet(0, 0, 4)), false);
    assert.equal(dispatch.validate(packet(0, 138, 4)), false);
    // 타입 10 이상은 처리기가 있어야 한다. 타입 9 이하는 처리기 없이 통과한다(선언 길이는 표의 값).
    dispatch.requireLength(20, 0xffff);
    assert.equal(dispatch.validate(packet(0, 20, 4)), false);
    dispatch.register(20, () => {});
    assert.equal(dispatch.validate(packet(0, 20, 4)), true);
    dispatch.requireLength(8, 0xffff);
    assert.equal(dispatch.validate(packet(0, 8, 4)), true);
    // 경계: 타입 9 는 처리기 없이 통과, 타입 10 은 처리기가 필요하다.
    dispatch.requireLength(9, 0xffff);
    dispatch.requireLength(10, 0xffff);
    assert.deepEqual([dispatch.validate(packet(0, 9, 4)), dispatch.validate(packet(0, 10, 4))], [true, false]);
    dispatch.register(10, () => {});
    assert.equal(dispatch.validate(packet(0, 10, 4)), true);
    // 고정 길이는 헤더 WORD 와 정확히 같아야 한다(little endian).
    dispatch.requireLength(20, 0x0105);
    assert.equal(dispatch.validate(packet(0, 20, 0x0105)), true);
    assert.equal(dispatch.validate(packet(0, 20, 0x0501)), false);
    assert.equal(dispatch.validate(packet(0, 20, 0x0104)), false);
});

// 닫힘/무효/집합/버전 정책: 닫힌 연결은 false, 무효 패킷은 무시(true), 집합은 거부, 버전은 방향별 종료(false)/무시(true)/계속을 가른다. incoming 의 0x80 비트는 가린다. (2026-11-11 WB04-E306)
test('분배기: 닫힘·무효·집합·버전 정책', () => {
    const { dispatch, log, connection } = scene();
    dispatch.requireLength(20, 0xffff);
    let received = 0;
    dispatch.register(20, () => { received += 1; });
    // 닫힌 연결은 진입하지 않는다.
    connection.flags = 0x40;
    assert.equal(dispatch.dispatch(connection, packet(0, 20, 4)), false);
    assert.deepEqual([received, log.length], [0, 0]);
    connection.flags = 0;
    // 무효 패킷(처리기 없는 타입 10 이상·지원 밖)은 무시한다.
    assert.equal(dispatch.dispatch(connection, packet(0, 21, 4)), true);
    assert.equal(dispatch.dispatch(connection, packet(0, 0, 4)), true);
    assert.deepEqual([received, log.length], [0, 0]);
    // 집합 패킷(타입 4)은 공통 프레임 부모가 필요해 거부한다.
    assert.throws(() => dispatch.dispatch(connection, packet(0, 4, 4)), /집합 패킷/);
    // 버전 정책: 등록 버전 5, 정책 별로 높은/낮은 버전을 가른다.
    dispatch.version(20, 5, 0);
    assert.equal(dispatch.dispatch(connection, packet(9, 20, 4)), true);
    assert.equal(received, 1);
    // 정책 BYTE 와 방향 조합마다 종료/무시/계속을 확인한다.
    for (const [mismatch, incoming, terminated, result] of [[4, 9, true, false], [1, 9, false, true], [8, 1, true, false], [2, 1, false, true], [4, 1, false, true], [8, 9, false, true], [0, 1, false, true]]) {
        const before = received;
        log.length = 0;
        dispatch.version(20, 5, mismatch);
        assert.equal(dispatch.dispatch(connection, packet(incoming, 20, 4)), result, `mismatch ${mismatch} incoming ${incoming}`);
        assert.equal(log.some((item) => item[0] === 'terminate'), terminated);
        // 종료/무시는 시각 기록·기록·수신기를 건너뛰고, 계속(정책이 해당 방향이 아님)이면 처리한다.
        assert.equal(received, before + (result && !terminated && !(mismatch & (incoming < 5 ? 2 : 1)) ? 1 : 0), `receive ${mismatch}/${incoming}`);
    }
    // incoming 의 0x80 비트는 버전 비교에서 가린다.
    dispatch.version(20, 5, 4);
    assert.equal(dispatch.dispatch(connection, packet(0x85, 20, 4)), true);
});

// 시각 기록: 타입 6 은 heartbeat, 그 밖은 received 에 현재 시계를 쓰고 타입 10 이상만 선언 길이와 함께 기록한다. 순서는 시각 → 기록 → 수신기이다. (2026-11-11 WB04-E306)
test('분배기: 시각 기록과 기록 순서', () => {
    const { dispatch, log, state, connection } = scene();
    const order = [];
    dispatch.requireLength(6, 0xffff);
    dispatch.requireLength(20, 0xffff);
    dispatch.requireLength(9, 0xffff);
    dispatch.register(6, () => { order.push('receive6'); });
    dispatch.register(9, () => { order.push('receive9'); });
    dispatch.register(20, () => { order.push('receive20'); });
    state.clock = 100;
    assert.equal(dispatch.dispatch(connection, packet(0, 6, 4)), true);
    assert.deepEqual([connection.heartbeat, connection.received], [100, 0]);
    state.clock = 101;
    dispatch.dispatch(connection, packet(0, 9, 4));
    assert.deepEqual([connection.heartbeat, connection.received, log.length], [100, 101, 0]);
    state.clock = 102;
    const bytes = packet(0, 20, 0x0123, 9, 9);
    dispatch.requireLength(20, 0x0123);
    dispatch.dispatch(connection, bytes);
    assert.deepEqual(log, [['record', 20, 0x0123]]);
    // 기록 경계: 타입 10 은 기록하고 타입 9 는 기록하지 않는다.
    log.length = 0;
    dispatch.requireLength(10, 0xffff);
    dispatch.register(10, () => {});
    dispatch.dispatch(connection, packet(0, 10, 4));
    dispatch.dispatch(connection, packet(0, 9, 4));
    assert.deepEqual(log, [['record', 10, 4]]);
    log.length = 0;
    assert.equal(connection.received, 102);
    assert.deepEqual(order, ['receive6', 'receive9', 'receive20', 'receive9']);
    // 기록 효과가 등록을 바꾸면 그 슬롯의 현재 수신기를 한 번 선택한다.
    order.length = 0;
    dispatch.requireLength(20, 0xffff);
    const original = scene();
    original.dispatch.requireLength(20, 0xffff);
    original.dispatch.register(20, () => { order.push('old'); });
    original.hooks.record = () => { original.dispatch.register(20, () => { order.push('new'); }); };
    const swapped = new RawPacketDispatch(original.hooks, original.dispatch.registrations);
    swapped.dispatch(new PacketConnection(), packet(0, 20, 4));
    assert.deepEqual(order, ['new']);
});

// 즉시 콜백: sender 와 연결을 넘기고 깊이를 올렸다가 정상/예외 종료에서 복구하며, 깊이 0 에서 닫힘이 켜지면 소멸/해제 뒤 false 이다. (2026-11-11 WB04-E306)
test('분배기: 콜백 깊이와 닫힘 후처리', () => {
    const { dispatch, log, connection } = scene();
    dispatch.requireLength(20, 0xffff);
    const seen = [];
    dispatch.register(20, (sender, current, bytes) => { seen.push([sender, current === connection, current.depth, Array.from(bytes)]); });
    assert.equal(dispatch.dispatch(connection, packet(0, 20, 4, 7)), true);
    assert.deepEqual(seen, [[17, true, 1, [0, 20, 4, 0, 7]]]);
    assert.equal(connection.depth, 0);
    // 예외가 나도 깊이는 복구되고 예외가 전파된다.
    dispatch.register(20, () => { throw new Error('수신 오류'); });
    assert.throws(() => dispatch.dispatch(connection, packet(0, 20, 4)), /수신 오류/);
    assert.equal(connection.depth, 0);
    // 콜백이 연결을 닫으면 깊이 0 에서 소멸/해제를 순서대로 실행하고 false 를 돌려준다.
    dispatch.register(20, (sender, current) => { current.flags |= 0x40; });
    log.length = 0;
    assert.equal(dispatch.dispatch(connection, packet(0, 20, 4)), false);
    assert.deepEqual(log, [['record', 20, 4], ['close'], ['release']]);
    // 중첩 콜백(깊이가 남아 있을 때)에서는 닫혀도 소멸하지 않는다.
    const nested = new PacketConnection({ depth: 1 });
    dispatch.register(20, (sender, current) => { current.flags |= 0x40; });
    log.length = 0;
    assert.equal(dispatch.dispatch(nested, packet(0, 20, 4)), true);
    assert.deepEqual(log, [['record', 20, 4]]);
    assert.equal(nested.depth, 1);
    // 수신기가 없는 타입 9 이하는 깊이를 올리지 않고 닫힘만 확인한다.
    const closed = new PacketConnection({ flags: 0 });
    dispatch.requireLength(8, 0xffff);
    assert.equal(dispatch.dispatch(closed, packet(0, 8, 4)), true);
    assert.equal(closed.depth, 0);
});

// 등록 핀: 분배 동안 대여의 활동 수가 올라 있고(기록 효과·수신기 모두 안에서) 끝나면 내려가며, 지연 표시 연결은 수신기가 있을 때만 거부한다. (2026-11-11 WB04-E306)
test('분배기: 등록 핀과 지연 연결', () => {
    const { dispatch, hooks, connection } = scene();
    const lease = dispatch.registrations.bindSession({});
    dispatch.requireLength(20, 0xffff);
    const counts = [];
    hooks.record = () => { counts.push(['record', lease.calls]); };
    const traced = new RawPacketDispatch(hooks, dispatch.registrations);
    traced.register(20, () => { counts.push(['receive', lease.calls]); });
    traced.dispatch(connection, packet(0, 20, 4));
    assert.deepEqual(counts, [['record', 1], ['receive', 2]]);
    assert.equal(lease.calls, 0);
    // 예외에도 핀을 반납한다.
    traced.register(20, () => { throw new Error('오류'); });
    assert.throws(() => traced.dispatch(connection, packet(0, 20, 4)), /오류/);
    assert.equal(lease.calls, 0);
    // 지연 표시가 있는 연결: 수신기가 있으면 거부하고 깊이를 건드리지 않으며, 수신기가 없는 타입 9 이하는 통과한다.
    const delayed = new PacketConnection({ delayed: true });
    assert.throws(() => traced.dispatch(delayed, packet(0, 20, 4)), /지연 표시/);
    assert.equal(delayed.depth, 0);
    traced.requireLength(8, 0xffff);
    assert.equal(traced.dispatch(delayed, packet(0, 8, 4)), true);
    lease.detach();
});

// 직접 호출: 직접 수신기는 NULL 연결도 받고 검증/기록/시각/깊이를 추가하지 않으며, 일반 수신기는 실제 연결이 있어야 한다. 두 입구는 같은 함수를 쓴다. (2026-11-11 WB04-E306)
test('분배기: 직접 수신기와 직접 호출', () => {
    const { dispatch, log, connection } = scene();
    const calls = [];
    dispatch.registerDirect(30, (sender, current, bytes) => { calls.push([sender, current, bytes.length]); });
    dispatch.invokeDirect(30, 5, null, Uint8Array.of(1, 2));
    dispatch.invokeDirect(30, 6, connection, Uint8Array.of(1));
    assert.deepEqual(calls, [[5, null, 2], [6, connection, 1]]);
    // 직접 호출은 시각·기록·깊이를 건드리지 않는다.
    assert.deepEqual([log.length, connection.depth, connection.received], [0, 0, 0]);
    // 프레임 분배 입구도 같은 함수를 실제 연결로 호출한다.
    dispatch.requireLength(30, 0xffff);
    dispatch.dispatch(connection, packet(0, 30, 4));
    assert.deepEqual(calls[2].slice(0, 2), [17, connection]);
    // 일반 수신기는 NULL 연결 직접 호출을 거부하고 수신기가 없으면 아무 일도 없다.
    dispatch.register(31, () => {});
    assert.throws(() => dispatch.invokeDirect(31, 1, null, Uint8Array.of(1)), TypeError);
    assert.doesNotThrow(() => dispatch.invokeDirect(32, 1, null, Uint8Array.of(1)));
    assert.throws(() => dispatch.invokeDirect(0, 1, null, Uint8Array.of(1)), RangeError);
    // 직접 호출 동안에도 등록 핀을 유지하고 예외에도 반납한다.
    const lease = dispatch.registrations.bindSession({});
    dispatch.registerDirect(33, () => { calls.push(['pin', lease.calls]); throw new Error('직접 오류'); });
    assert.throws(() => dispatch.invokeDirect(33, 1, null, Uint8Array.of(1)), /직접 오류/);
    assert.deepEqual(calls.at(-1), ['pin', 1]);
    assert.equal(lease.calls, 0);
    lease.detach();
});

// 건설 수신 등록: 0x37 의 observer 를 비우고 버전 1/정책 0 을 쓰며 수신기는 패킷만 data.receive 로 넘긴다(sender/연결은 사용하지 않음). (2026-11-11 WB04-E306)
test('분배기: 건설 수신 등록', () => {
    const { dispatch, connection } = scene();
    dispatch.observe(0x37, () => {});
    const received = [];
    registerConstructionData(dispatch, { receive: (bytes) => { received.push(Array.from(bytes)); } });
    const entry = dispatch.registration(0x37);
    assert.deepEqual([entry.observer, entry.packetObserver, entry.version, entry.mismatch], [null, null, 1, 0]);
    dispatch.requireLength(0x37, 0xffff);
    assert.equal(dispatch.dispatch(connection, packet(1, 0x37, 5, 9)), true);
    assert.deepEqual(received, [[1, 0x37, 5, 0, 9]]);
    // 버전이 다르면 정책 0 이므로 계속 분배한다.
    assert.equal(dispatch.dispatch(connection, packet(2, 0x37, 5, 9)), true);
    assert.equal(received.length, 2);
});

// 현재 Player9 활성 조회: 선택한 owner 기록의 +0x50 낮은 BYTE 만 읽고, current 가 있으면 매 호출 현재 base 를 쓴다. (2026-11-11 WB04-E306)
test('분배기: 현재 Player9 활성 조회', () => {
    const players = new Uint8Array(9 * 180);
    players[3 * 180 + 0x50] = 0x03;
    players[4 * 180 + 0x50] = 0x02;
    assert.equal(packetPlayerActive(players, null, 3), true);
    assert.equal(packetPlayerActive(players, null, 4), false);
    assert.equal(packetPlayerActive(players, null, 0), false);
    // current 가 있으면 fallback 대신 현재 base 를 매번 조회한다.
    const other = new Uint8Array(9 * 180);
    other[0 * 180 + 0x50] = 1;
    let reads = 0;
    const current = () => { reads += 1; return other; };
    assert.equal(packetPlayerActive(players, current, 0), true);
    assert.equal(packetPlayerActive(players, current, 3), false);
    assert.equal(reads, 2);
    // 크기가 다르거나 owner 가 밖이면 거부한다.
    assert.throws(() => packetPlayerActive(new Uint8Array(8 * 180), null, 0), RangeError);
    assert.throws(() => packetPlayerActive(players, null, 9), RangeError);
    assert.throws(() => packetPlayerActive(players, null, -1), RangeError);
    assert.throws(() => packetPlayerActive(players, null, 1.5), RangeError);
});
