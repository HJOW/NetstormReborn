// Client 캠페인 전송 생성기와 Client 기본 패킷 등록 표(순수 부분)의 계약을 검사한다(WB04-E328, 4-C 1단계).
//  - cpppj 에는 이 파일을 직접 겨냥한 시험이 없고(Session/Meta 시험이 `MakeClientCampaignTransport` 로 간접 사용, 4-D) 기준선 소스와 ② 반영분을 단위 계약으로 옮겨 확인한다.
//  - 가변 길이 관찰기: 송신에서만 -1 센티넬 진단과 선언 WORD 쓰기, 수신 방향은 읽지 않음, 16비트 감김·signed BYTE 해석.
//  - 기본 등록 표: 열여덟 행의 version 1/policy 0·길이·빈 관찰기/C3 수신기, 0x0b 버전 조회 수신기, 0x32 현재 설정 수신기, 미션 송신 관찰기 4종, 0x3f/0x5f/0x7d 길이 관찰기.
//  - 생성기: 검증과 훅(`players`·`now`·`sent`·`record`·`terminate`·`close`·`release`)이 실제 하위 전송에서 불리는 것.
// (2026-11-12 WB04-E328: 4-C ClientCampaignTransport 1단계 이식과 함께 추가)
import assert from 'node:assert/strict';
import test from 'node:test';
import {
    CLIENT_PACKET_DEFAULTS, installClientMissionSendObservers, installClientPacketDefaults, makeClientCampaignTransport,
    observeClientPacketDefault, readClientPacketDefault, writeClientPacketDefaultLength,
} from '../www/core/client-campaign-transport.js';
import { CompositeTransportRole } from '../www/core/composite-mission-transport.js';
import { ClientConnectionGlobals } from '../www/core/client-connection-globals.js';
import { RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { observeBombEffectPacket } from '../www/core/raw-bomb-effect.js';

// 패킷 한 칸을 만든다: 크기만큼 0 으로 채우고 지정한 (오프셋, 값, 폭) 필드를 리틀엔디언으로 쓴다. (2026-11-12 WB04-E328)
function packetOf(size, fields = [], fill = 0) {
    const bytes = new Uint8Array(size).fill(fill);
    // 필드마다 낮은 BYTE 부터 쓴다.
    for (const [offset, value, width] of fields) {
        for (let byte = 0; byte < width; byte += 1) bytes[offset + byte] = Math.floor(value / 2 ** (8 * byte)) & 0xff;
    }
    return bytes;
}

// 선언 길이 WORD 를 읽는다. (2026-11-12 WB04-E328)
const declared = (packet) => packet[2] | (packet[3] << 8);

// 진단 호출을 기록하는 장치이다. (2026-11-12 WB04-E328)
function recorder() {
    const calls = [];
    return { calls, assertion: (expression, file, line) => { calls.push([expression, file, line]); } };
}

// ---------------------------------------------------------------- 가변 길이 관찰기

// 수신 방향(sending=0)은 필드를 읽기 전에 복귀하므로 진단도 길이 쓰기도 없다(범위 밖 버퍼여도 읽지 않는다).
test('Client 기본 관찰기: 수신 방향은 아무 것도 읽거나 쓰지 않는다', () => {
    const device = recorder();
    for (const type of [0x0b, 0x0c, 0x55, 0x56, 0x57, 0x16, 0x1c, 0x85, 0x7b]) {
        const packet = packetOf(4, [], 0xff);
        observeClientPacketDefault(type, packet, 0, device.assertion);
        assert.deepEqual([...packet], [0xff, 0xff, 0xff, 0xff]);
    }
    assert.deepEqual(device.calls, []);
});

// 송신: 각 종류의 길이 필드(오프셋·폭)와 상수 더하기를 원문 값으로 확인한다.
test('Client 기본 관찰기: 송신 길이 필드와 상수', () => {
    const device = recorder();
    const cases = [
        [0x0b, packetOf(16, [[8, 100, 2]]), 112],
        [0x0c, packetOf(16, [[8, 100, 2]]), 112],
        [0x55, packetOf(16, [[4, 7, 2]]), 15],
        [0x56, packetOf(16, [[8, 9, 2]]), 21],
        [0x57, packetOf(16, [[8, 3, 2]]), 15],
        [0x1c, packetOf(60, [[45, 20, 1]]), 66],
        [0x85, packetOf(16, [[5, 300, 2]]), 307],
    ];
    for (const [type, packet, expected] of cases) {
        observeClientPacketDefault(type, packet, 1, device.assertion);
        assert.equal(declared(packet), expected, `종류 0x${type.toString(16)}`);
    }
    assert.deepEqual(device.calls, []);
    // 0x16 은 signed BYTE 개수 × 4 + 14, 0x7b 는 signed BYTE 길이 + 10 이며 음수는 16비트로 감긴다.
    const friends = packetOf(16, [[5, 3, 1]]);
    observeClientPacketDefault(0x16, friends, 1, device.assertion);
    assert.equal(declared(friends), 26);
    const negativeFriends = packetOf(16, [[5, 0xfe, 1]]);
    observeClientPacketDefault(0x16, negativeFriends, 1, device.assertion);
    assert.equal(declared(negativeFriends), 6);
    const address = packetOf(16, [[9, 12, 1]]);
    observeClientPacketDefault(0x7b, address, 1, device.assertion);
    assert.equal(declared(address), 22);
    const negativeAddress = packetOf(16, [[9, 0x80, 1]]);
    observeClientPacketDefault(0x7b, negativeAddress, 1, device.assertion);
    assert.equal(declared(negativeAddress), (-128 + 10) & 0xffff);
    // 선언 WORD 밖의 바이트는 보존한다.
    assert.equal(friends[4], 0);
    assert.equal(friends[5], 3);
});

// 센티넬(-1)은 진단을 부르고, 진단이 복귀하면 현재 필드를 다시 읽어 길이를 정한다. 줄 번호와 식은 원문이다.
test('Client 기본 관찰기: -1 센티넬 진단과 복귀 뒤 현재 필드 재조회', () => {
    const expected = [
        [0x0b, [[8, 0xffffffff, 4]], 'extraDataLen != (int)-1', 0x36c, 0xffff + 12],
        [0x0c, [[8, 0xffffffff, 4]], 'extraDataLen != (int)-1', 0x38c, 0xffff + 12],
        [0x55, [[4, 0xffffffff, 4]], 'count != (int)-1', 0x8dc, 0xffff + 8],
        [0x56, [[8, 0xffffffff, 4]], 'dataLength != (int)-1', 0x8f8, 0xffff + 12],
        [0x57, [[8, 0xffffffff, 4]], 'specLength != (int)-1', 0x916, 0xffff + 12],
        [0x16, [[5, 0xff, 1]], 'numFriends != (char)-1', 0x458, -4 + 14],
        [0x1c, [[45, 0xff, 1]], 'nameLen != (unsigned char)-1', 0x4ca, 0xff + 46],
        [0x85, [[5, 0xffff, 2]], 'dataLen != (unsigned short)-1', 0xa2c, 0xffff + 7],
        [0x7b, [[9, 0xff, 1]], 'addrLen != (char)-1', 0xba8, -1 + 10],
    ];
    for (const [type, fields, expression, line, length] of expected) {
        const device = recorder();
        const packet = packetOf(64, fields);
        observeClientPacketDefault(type, packet, 1, device.assertion);
        assert.deepEqual(device.calls, [[expression, '\\Ns\\O\\_p_packets.cpp', line]], `종류 0x${type.toString(16)}`);
        assert.equal(declared(packet), length & 0xffff, `종류 0x${type.toString(16)} 길이`);
    }
    // 진단 장치가 현재 필드를 바꾸면 복귀 뒤 바뀐 값을 읽는다.
    const packet = packetOf(16, [[5, 0xff, 1]]);
    observeClientPacketDefault(0x16, packet, 1, () => { packet[5] = 2; });
    assert.equal(declared(packet), 22);
});

// 물리 버퍼가 모자라면 읽기/쓰기 전에 거부하고 미등록 종류는 형 오류이다.
test('Client 기본 관찰기: 버퍼 부족과 미등록 종류', () => {
    assert.throws(() => readClientPacketDefault(new Uint8Array(4), 2, 4), RangeError);
    assert.throws(() => readClientPacketDefault(new Uint8Array(4), 5, 1), RangeError);
    assert.equal(readClientPacketDefault(new Uint8Array([1, 2, 3, 4]), 0, 4), 0x04030201);
    assert.equal(readClientPacketDefault(new Uint8Array([0xff, 0xff, 0xff, 0xff]), 0, 4), 0xffffffff);
    assert.throws(() => writeClientPacketDefaultLength(new Uint8Array(3), 5), RangeError);
    const packet = new Uint8Array(4);
    writeClientPacketDefaultLength(packet, 0x12345);
    assert.deepEqual([...packet], [0, 0, 0x45, 0x23]);
    assert.throws(() => observeClientPacketDefault(0x30, packetOf(16), 1, () => {}), /미등록 mutable 관찰기 종류/);
    assert.throws(() => observeClientPacketDefault(0x0b, packetOf(8), 1, () => {}), RangeError);
});

// 0x5f 장기 기본 관찰기: 송신에서만 count 센티넬 진단 뒤 현재 count WORD 로 (수 + 5) × 2 를 쓰며 100 개 한도 같은 범위 검사는 없다.
test('주문 효과 기본 관찰기: observeBombEffectPacket', () => {
    const device = recorder();
    const received = packetOf(16, [[6, 3, 4]], 0xaa);
    observeBombEffectPacket(received, 0, () => { throw new Error('수신은 부르지 않음'); });
    assert.equal(declared(received), 0xaaaa);
    const sent = packetOf(16, [[6, 3, 4]]);
    observeBombEffectPacket(sent, 1, () => { throw new Error('센티넬 아님'); });
    assert.equal(declared(sent), 16);
    // 100 개를 넘어도 거부하지 않고 16비트로 감긴다.
    const many = packetOf(16, [[6, 40000, 4]]);
    observeBombEffectPacket(many, 1, () => {});
    assert.equal(declared(many), ((40000 + 5) * 2) & 0xffff);
    const sentinel = packetOf(16, [[6, 0xffffffff, 4]]);
    // 진단 장치가 count DWORD 를 1 로 바꾸면 복귀 뒤 현재 하위 WORD 로 (1 + 5) × 2 = 12 를 쓴다.
    observeBombEffectPacket(sentinel, 1, () => { device.calls.push('진단'); sentinel[6] = 1; sentinel[7] = 0; });
    assert.deepEqual(device.calls, ['진단']);
    assert.equal(declared(sentinel), 12);
    // 하위 WORD 만 읽는다: 상위 WORD 가 남아 있어도 길이에 영향이 없다.
    const high = packetOf(16, [[6, 0x00010002, 4]]);
    observeBombEffectPacket(high, 1, () => {});
    assert.equal(declared(high), 14);
    assert.throws(() => observeBombEffectPacket(packetOf(16, [[6, 0xffffffff, 4]]), 1, null), /원문 진단 장치 미연결/);
    assert.throws(() => observeBombEffectPacket(new Uint8Array(3), 1, () => {}), RangeError);
});

// ---------------------------------------------------------------- 기본 등록 표

// Client 장치 장면: 등록소·진단·GameAux·기록 장치를 가진 최소 Client 이다. (2026-11-12 WB04-E328)
function makeClient({ initialized = true } = {}) {
    const records = new Uint8Array(1620);
    const registry = new ClientConnectionGlobals();
    const log = [];
    const diagnostics = [];
    const writes = [];
    const settings = new Uint32Array(7);
    const header = [0x0c, 0x85];
    const state = { milliseconds: 5000, initialLength: 0, server: { value: true } };
    const sentMutable = [];
    const realSendMutable = registry.sendMutable.bind(registry);
    registry.sendMutable = (id, packet, players) => { sentMutable.push([id, Uint8Array.from(packet), players.length]); return realSendMutable(id, packet, players); };
    const client = {
        edition: 'patch1078',
        playerRecords: () => ({ initialized: () => initialized, records: () => records }),
        gameplayGlobals: () => ({ server: () => state.server, gameAux: () => ({ missionSettings: () => settings }) }),
        gameClockOwner: () => ({ wallSeconds: (ms) => ms / 1000 }),
        clientMilliseconds: () => state.milliseconds,
        connectionGlobals: () => registry,
        packetRecordOwner: () => ({ write: (outgoing, connection, packet, length) => { writes.push([outgoing, connection.sender, packet.length, length]); } }),
        surfaceStorage: () => ({
            assertDisplay: (expression, file, line) => { diagnostics.push([expression, file, line]); },
            gameAux: () => ({ packet0cHeader: () => header, packet0cInitialLength: () => state.initialLength }),
        }),
        log: (text) => { log.push(text); },
    };
    return { client, records, registry, log, diagnostics, writes, settings, header, state, sentMutable };
}

// 열여덟 행 표: 종류·길이·C3 수신기·mutable 관찰기 구분이 원문과 같고 서로 겹치지 않는다.
test('Client 기본 등록 표: 열여덟 행의 구성', () => {
    assert.equal(CLIENT_PACKET_DEFAULTS.length, 18);
    assert.equal(new Set(CLIENT_PACKET_DEFAULTS.map((entry) => entry.type)).size, 18);
    // 원문 C3 수신기 7종과 mutable 관찰기 7종.
    assert.deepEqual(CLIENT_PACKET_DEFAULTS.filter((entry) => entry.c3Receiver).map((entry) => entry.type), [0x0c, 0x42, 0x4d, 0x55, 0x56, 0x57, 0x5a]);
    assert.deepEqual(CLIENT_PACKET_DEFAULTS.filter((entry) => entry.mutableObserver).map((entry) => entry.type), [0x0c, 0x55, 0x56, 0x57, 0x16, 0x1c, 0x85]);
    const lengths = Object.fromEntries(CLIENT_PACKET_DEFAULTS.map((entry) => [entry.type, entry.length]));
    assert.deepEqual(lengths, { 12: 0xffff, 66: 12, 77: 10, 85: 0xffff, 86: 0xffff, 87: 0xffff, 90: 5, 16: 8, 17: 12, 18: 12, 22: 0xffff, 23: 14, 28: 0xffff, 30: 14, 31: 12, 104: 5, 133: 0xffff, 135: 274 });
    assert.throws(() => { CLIENT_PACKET_DEFAULTS.push({}); }, TypeError);
    assert.throws(() => { CLIENT_PACKET_DEFAULTS[0].type = 1; }, TypeError);
});

// 설치 결과: 열여덟 행 모두 version 1/policy 0, 표의 길이, 비NULL 관찰기; C3 7종만 빈 수신기를 가진다.
test('Client 기본 등록 표: 설치된 version/policy/길이/관찰기/수신기', () => {
    const { client } = makeClient();
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    installClientPacketDefaults(client, dispatch);
    for (const entry of CLIENT_PACKET_DEFAULTS) {
        const registration = dispatch.registration(entry.type);
        assert.deepEqual([registration.version, registration.mismatch, registration.length], [1, 0, entry.length], `종류 0x${entry.type.toString(16)}`);
        // mutable 관찰기는 packetObserver, 그 밖은 비NULL 빈 observer 이다.
        if (entry.mutableObserver) assert.deepEqual([typeof registration.packetObserver, registration.observer], ['function', null]);
        else assert.deepEqual([typeof registration.observer, registration.packetObserver], ['function', null]);
        // 빈 C3 수신기는 7종뿐이다(0x0c 의 C3 는 뒤의 0x0b 가 아니라 자기 종류 표).
        assert.equal(Boolean(registration.receive), entry.c3Receiver, `종류 0x${entry.type.toString(16)} 수신기`);
    }
    // 빈 수신기는 아무 효과 없이 반환한다.
    assert.equal(dispatch.registration(0x42).receive(1, null, new Uint8Array(12)), undefined);
    // 표 밖 종류는 건드리지 않는다.
    for (const type of [0x01, 0x20, 0x40]) assert.deepEqual([dispatch.registration(type).version, dispatch.registration(type).receive], [0, null]);
    // 0x0b·0x32·0x7b·0x3f·0x5f·0x7d·미션 송신 관찰기 4종.
    for (const [type, version, length] of [[0x0b, 1, 0xffff], [0x32, 1, 32], [0x7b, 1, 0xffff], [0x3f, 1, 0xffff], [0x5f, 1, 0xffff], [0x7d, 1, 0xffff]]) {
        assert.deepEqual([dispatch.registration(type).version, dispatch.registration(type).length], [version, length], `종류 0x${type.toString(16)}`);
    }
    for (const type of [0x2a, 0x2b, 0x67, 0x81]) assert.equal(typeof dispatch.registration(type).packetObserver, 'function');
    assert.equal(typeof dispatch.registration(0x32).observer, 'function');
    assert.equal(typeof dispatch.registration(0x32).receive, 'function');
    // 아직 옮기지 않은 묶음은 설치하지 않는다(Native 고정 수신기·채팅·연결 종료·기술 목록·경로 자료·압축 Squid).
    for (const type of [0x02, 0x30, 0x41, 0x69, 0x7a]) assert.equal(dispatch.registration(type).receive, null);
});

// 관찰기들이 장기 Client 진단(storage.assertDisplay)을 쓰고 송신에서만 길이를 확정한다.
test('Client 기본 등록 표: 설치된 관찰기의 진단 연결', () => {
    const { client, diagnostics } = makeClient();
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    installClientPacketDefaults(client, dispatch);
    // 0x55: count 센티넬 진단이 Client storage 로 간다.
    const packet = packetOf(16, [[4, 0xffffffff, 4]]);
    dispatch.registration(0x55).packetObserver(packet, 1);
    assert.deepEqual(diagnostics.pop(), ['count != (int)-1', '\\Ns\\O\\_p_packets.cpp', 0x8dc]);
    // 0x2a: fortFileSize 센티넬.
    const fort = packetOf(32, [[0xc, 0xffffffff, 4]]);
    dispatch.registration(0x2a).packetObserver(fort, 1);
    assert.deepEqual(diagnostics.pop(), ['fortFileSize != (int)-1', '\\Ns\\O\\_p_packets.cpp', 0x672]);
    // 0x3f: sidCount 255.
    const construction = packetOf(40, [[26, 255, 1]]);
    dispatch.registration(0x3f).packetObserver(construction, 1);
    assert.deepEqual(diagnostics.pop(), ['sidCount != (unsigned char)-1', '\\Ns\\O\\_p_packets.cpp', 0x7c9]);
    // 0x5f: count 센티넬은 0x96b.
    const bomb = packetOf(16, [[6, 0xffffffff, 4]]);
    dispatch.registration(0x5f).packetObserver(bomb, 1);
    assert.deepEqual(diagnostics.pop(), ['count != (int)-1', '\\Ns\\O\\_p_packets.cpp', 0x96b]);
    // 0x7b: addrLen 센티넬은 0xba8.
    const addr = packetOf(16, [[9, 0xff, 1]]);
    dispatch.registration(0x7b).packetObserver(addr, 1);
    assert.deepEqual(diagnostics.pop(), ['addrLen != (char)-1', '\\Ns\\O\\_p_packets.cpp', 0xba8]);
    // 0x7d: 대상 수 65535 진단은 오류이고 수신 방향은 읽지 않는다.
    assert.throws(() => dispatch.registration(0x7d).packetObserver(packetOf(40, [[16, 0xffff, 2]]), 1), /원본 상태 타이머 대상 수 진단/);
    dispatch.registration(0x7d).packetObserver(packetOf(40, [[16, 0xffff, 2]]), 0);
    // 0x7d 송신 길이: (count + 11) × 2.
    const timer = packetOf(40, [[16, 4, 2]]);
    dispatch.registration(0x7d).packetObserver(timer, 1);
    assert.equal(declared(timer), 30);
    assert.deepEqual(diagnostics, []);
});

// 0x32: 장기 7DWORD 에 패킷의 DWORD 7개를 순서대로 직접 쓴다(송신자·연결·Session 관문 없음).
test('Client 기본 등록 표: 0x32 현재 설정 수신기', () => {
    const { client, settings } = makeClient();
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    installClientPacketDefaults(client, dispatch);
    const packet = new Uint8Array(32);
    for (let index = 0; index < 7; index += 1) new DataView(packet.buffer).setUint32(4 + index * 4, 0x100 + index, true);
    dispatch.registration(0x32).receive(0, null, packet);
    assert.deepEqual([...settings], [0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106]);
    // 직접 호출(NULL 연결)도 같은 본문이다.
    new DataView(packet.buffer).setUint32(4, 0x777, true);
    dispatch.registration(0x32).directReceive(9, null, packet);
    assert.equal(settings[0], 0x777);
});

// 0x0b 버전 조회: 50BYTE 예약 전체 0xff 에 현재 GameAux 의 BYTE 두 개/WORD 하나와 버전 10.78 을 쓴 응답을 현재 sender 의 실제 가상 송신으로 보낸다.
test('Client 기본 등록 표: 0x0b 버전 조회 수신기의 응답 작성', () => {
    const { client, registry, header, state, sentMutable, records } = makeClient();
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    installClientPacketDefaults(client, dispatch);
    header[0] = 0x0c;
    header[1] = 0xff;
    state.initialLength = 2;
    // 연결이 없는 sender 로 호출하면 등록소 송신 관문이 연결 없음으로 자연 생략하지만 응답은 만들어 보낸다.
    dispatch.registration(0x0b).receive(5, null, new Uint8Array(12));
    assert.equal(sentMutable.length, 1);
    const [id, reply, playerBytes] = sentMutable[0];
    assert.equal(id, 5);
    assert.equal(playerBytes, records.length);
    assert.equal(reply.length, 50);
    // 헤더: 프로토콜 BYTE 의 상위 비트는 지워지고, 메시지 BYTE, 선언 길이는 최소 4 로 올라간다.
    assert.deepEqual([reply[0], reply[1], reply[2] | (reply[3] << 8)], [0x7f, 0x0c, 4]);
    // 버전 WORD 10/78 과 나머지 예약은 0xff 그대로이다.
    assert.deepEqual([...reply.subarray(4, 8)], [10, 0, 78, 0]);
    assert.ok(reply.subarray(8).every((byte) => byte === 0xff));
    // 초기 길이가 4 이상이면 그대로 쓴다.
    state.initialLength = 40;
    dispatch.registration(0x0b).directReceive(0xffffffff, null, new Uint8Array(12));
    assert.equal(sentMutable[1][0], -1);
    assert.equal(sentMutable[1][1][2] | (sentMutable[1][1][3] << 8), 40);
    assert.ok(registry);
});

// 미션 송신 관찰기 4종은 수신기·version·길이 표를 바꾸지 않는다. storage 가 없으면 거부한다.
test('Client 기본 등록 표: 미션 송신 관찰기만 설치', () => {
    const { client } = makeClient();
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    installClientMissionSendObservers(client, dispatch);
    for (const type of [0x2a, 0x2b, 0x67, 0x81]) {
        const registration = dispatch.registration(type);
        assert.equal(typeof registration.packetObserver, 'function');
        assert.deepEqual([registration.version, registration.receive, registration.observer], [0, null, null]);
    }
    assert.equal(dispatch.registration(0x32).packetObserver, null);
    // 송신 길이: 0x2b = nameLen + 0x3b, 0x81 = (numPlayers 하위 WORD + 2) × 4 (16비트 감김).
    const login = packetOf(80, [[0x33, 7, 1]]);
    dispatch.registration(0x2b).packetObserver(login, 1);
    assert.equal(declared(login), 7 + 0x3b);
    const players = packetOf(16, [[4, 0xffff, 4]]);
    dispatch.registration(0x81).packetObserver(players, 1);
    assert.equal(declared(players), (((0xffff + 2) & 0xffff) * 4) & 0xffff);
    const empty = { surfaceStorage: () => null };
    assert.throws(() => installClientMissionSendObservers(empty, dispatch), /실제 Client storage 없음/);
});

// ---------------------------------------------------------------- 생성기

// 생성기 장면: Client 와 World 를 묶고 복합 조정자를 만든다. (2026-11-12 WB04-E328)
function makeScene(options) {
    const scene = makeClient(options);
    const recorded = [];
    const world = { session: () => ({ playerRecords: () => scene.records, recordConnectionPacket: (connection, packet) => { recorded.push([connection.sender, packet.length]); } }) };
    return { ...scene, world, recorded };
}

// 검증: World 없음·판본·미초기화·다른 Player 배열은 효과 전에 거부한다.
test('Client 캠페인 전송 생성기: 수명 계약 검증', () => {
    const scene = makeScene();
    const role = CompositeTransportRole.InternalMission;
    assert.throws(() => makeClientCampaignTransport(scene.client, role, null), /수명 계약 오류/);
    assert.throws(() => makeClientCampaignTransport({ ...scene.client, edition: 'patch1071' }, role, scene.world), /수명 계약 오류/);
    const uninitialized = makeScene({ initialized: false });
    assert.throws(() => makeClientCampaignTransport(uninitialized.client, role, uninitialized.world), /수명 계약 오류/);
    const other = { session: () => ({ playerRecords: () => new Uint8Array(1620), recordConnectionPacket: () => {} }) };
    assert.throws(() => makeClientCampaignTransport(scene.client, role, other), /수명 계약 오류/);
    // 거부 뒤에는 어떤 기본 등록도 만들어지지 않았다(등록소만 있음).
    assert.deepEqual([...scene.registry.cleanup().transceivers].filter(Boolean), []);
    // 지원하지 않는 역할은 조정자의 웹 제외 오류이다.
    assert.throws(() => makeClientCampaignTransport(scene.client, CompositeTransportRole.LanHost, scene.world), /웹 범위 밖: 복합 전송 역할/);
});

// 생성: 조정자는 Client 의 등록소와 Player9 를 쓰고, 연결은 열지 않으며, 기본 등록이 대여 전에 채워져 있다.
test('Client 캠페인 전송 생성기: 조정자 구성과 기본 등록', () => {
    const scene = makeScene();
    const transport = makeClientCampaignTransport(scene.client, CompositeTransportRole.InternalMission, scene.world);
    assert.equal(transport.registry, scene.registry);
    assert.equal(transport.role, 'InternalMission');
    assert.deepEqual([...scene.registry.cleanup().transceivers].filter(Boolean), []);
    assert.deepEqual(scene.log, []);
    // 기본 등록이 대여 전에 채워져 있다.
    assert.equal(transport.dispatch.registration(0x42).version, 1);
    assert.equal(typeof transport.dispatch.registration(0x0b).receive, 'function');
    // 대여를 만들고 반납하면 기본 등록으로 돌아간다.
    transport.dispatch.register(0x42, () => 'Session');
    const lease = transport.bindSession({ name: 'Session' });
    assert.ok(lease);
    lease.detach();
    assert.equal(transport.dispatch.registration(0x42).version, 1);
});

// 훅: now 는 Client 시각 공급원의 현재 밀리초를 시계 원점 계산에 넘기고, 접속 시각에 쓰인다.
test('Client 캠페인 전송 생성기: now 훅은 현재 Client 시각을 읽는다', () => {
    const scene = makeScene();
    const transport = makeClientCampaignTransport(scene.client, CompositeTransportRole.InternalMission, scene.world);
    transport.dispatch.register(0x27, () => {});
    scene.state.milliseconds = 12345;
    transport.serve(6799);
    transport.connect(new Uint8Array(20), 6799);
    // 내부 client9 의 첫 수신/하트비트 시각이 wallSeconds(12345) = 12.345 의 DWORD 절삭이다.
    assert.deepEqual([transport.connection(9).received, transport.connection(9).heartbeat], [12, 12]);
    scene.state.milliseconds = 99999;
    transport.poll();
    assert.equal(transport.connection(-1).received, 99);
});

// 훅: sent/record 는 방향별 기록 장치로, record 는 추가로 현재 World 의 Session 에 전달되며, World 가 사라지면 Session 전달만 생략된다.
test('Client 캠페인 전송 생성기: sent/record 기록과 World 약참조', () => {
    const scene = makeScene();
    const transport = makeClientCampaignTransport(scene.client, CompositeTransportRole.InternalMission, scene.world);
    transport.dispatch.register(0x27, () => {});
    transport.dispatch.register(0x24, () => {});
    transport.serve(6799);
    transport.connect(new Uint8Array(20), 6799);
    transport.poll();
    transport.assign(-1, 1);
    const packet = Uint8Array.of(1, 0x24, 5, 0, 0);
    scene.writes.length = 0;
    // 송신은 전송 본체가 복사한 뒤 방향 1 로 기록한다.
    assert.equal(transport.send(9, packet), true);
    transport.flush();
    assert.ok(scene.writes.some(([outgoing, sender, size, length]) => outgoing === true && sender === 9 && size === 5 && length === 5));
    // 수신은 방향 0 기록이 먼저이고 Session 으로도 전달된다.
    scene.writes.length = 0;
    assert.equal(transport.poll(), 1);
    assert.ok(scene.writes.some(([outgoing, sender]) => outgoing === false && sender === 1));
    assert.ok(scene.recorded.length >= 1);
    assert.ok(scene.recorded.every(([, size]) => size > 0));
});

// 훅: terminate/close/release 는 진단 문장을 남긴다(terminate 는 닫힘 표시 0x40 도 쓴다).
test('Client 캠페인 전송 생성기: 종료/닫기/반납 진단', () => {
    const scene = makeScene();
    const transport = makeClientCampaignTransport(scene.client, CompositeTransportRole.InternalMission, scene.world);
    transport.dispatch.register(0x27, () => {});
    transport.serve(6799);
    transport.connect(new Uint8Array(20), 6799);
    transport.poll();
    transport.assign(-1, 1);
    transport.closeRoom();
    assert.ok(scene.log.includes('로컬 연결 큐/소켓 종료'));
    assert.ok(scene.log.includes('로컬 연결 반납'));
    // 로그 장치가 없으면 console.debug 로 간다.
    const { log, ...bare } = scene.client;
    const originalDebug = console.debug;
    const lines = [];
    console.debug = (text) => { lines.push(text); };
    try {
        const second = makeScene();
        const noLog = { ...second.client, log: undefined };
        const other = makeClientCampaignTransport(noLog, CompositeTransportRole.InternalMission, second.world);
        other.dispatch.register(0x27, () => {});
        other.serve(6800);
        other.connect(new Uint8Array(20), 6800);
        other.closeRoom();
    } finally {
        console.debug = originalDebug;
    }
    assert.ok(lines.includes('로컬 연결 반납'));
    assert.ok(bare);
});

// ---------------------------------------------------------------- 변형 확인으로 보강한 경계 (WB04-E328)

// 0x5f 관찰기: 센티넬은 DWORD 전체가 -1 일 때만이고(하위 WORD 만 0xffff 면 아님), 길이가 0x8000 이상이면 상위 BYTE 의 최상위 비트까지 쓴다.
test('주문 효과 기본 관찰기: 센티넬 폭과 길이 상위 BYTE', () => {
    const calls = [];
    const lowOnly = packetOf(16, [[6, 0x0000ffff, 4]]);
    observeBombEffectPacket(lowOnly, 1, () => { calls.push('진단'); });
    assert.deepEqual(calls, []);
    assert.equal(declared(lowOnly), ((0xffff + 5) * 2) & 0xffff);
    const big = packetOf(16, [[6, 20000, 4]]);
    observeBombEffectPacket(big, 1, () => {});
    assert.equal(declared(big), 40010);
    assert.equal(big[3], 0x9c);
});

// 하위 전송의 1..8 송신 관문은 훅 `players` 가 돌려주는 현재 Player9 의 활성 비트(+0x50 bit 0)를 읽는다. 생성기는 Client 의 현재 Player 배열을 넘긴다.
test('Client 캠페인 전송 생성기: players 훅은 현재 Player 배열이다', () => {
    const scene = makeScene();
    const transport = makeClientCampaignTransport(scene.client, CompositeTransportRole.InternalMission, scene.world);
    transport.dispatch.register(0x27, () => {});
    transport.serve(6799);
    transport.connect(new Uint8Array(20), 6799);
    transport.poll();
    transport.assign(-1, 1);
    const packet = Uint8Array.of(1, 0x24, 5, 0, 0);
    // Player1 이 활성이 아니면 보내지 않는다.
    assert.equal(transport.send(1, packet), false);
    // Client 의 현재 Player 배열에 활성 비트를 쓰면 같은 호출이 보내진다(매 송신마다 현재 배열을 읽는다).
    scene.records[1 * 180 + 0x50] |= 1;
    assert.equal(transport.send(1, packet), true);
    scene.records[1 * 180 + 0x50] &= ~1;
    assert.equal(transport.send(1, packet), false);
});

// 분배기의 종료 훅: 버전 불일치 정책(높은 버전이면 4=종료)에 걸린 프레임을 받으면 연결을 닫힘(0x40)으로 표시하고 진단을 남긴다.
test('Client 캠페인 전송 생성기: terminate 훅은 닫힘 표시와 진단을 쓴다', () => {
    const scene = makeScene();
    const transport = makeClientCampaignTransport(scene.client, CompositeTransportRole.InternalMission, scene.world);
    transport.dispatch.register(0x27, () => {});
    transport.serve(6799);
    transport.connect(new Uint8Array(20), 6799);
    transport.poll();
    transport.assign(-1, 1);
    // 0x70 을 버전 1/정책 4(높은 버전 종료)로 등록하고 버전 2 프레임을 클라이언트에서 보낸다.
    transport.dispatch.register(0x70, () => {});
    transport.dispatch.version(0x70, 1, 4);
    transport.dispatch.requireLength(0x70, 5);
    assert.equal(transport.send(9, Uint8Array.of(2, 0x70, 5, 0, 0)), true);
    transport.flush();
    scene.log.length = 0;
    transport.poll();
    assert.ok(scene.log.includes('로컬 미션 연결 종료'));
    assert.equal((transport.connection(1)?.flags ?? 0x40) & 0x40, 0x40);
});
