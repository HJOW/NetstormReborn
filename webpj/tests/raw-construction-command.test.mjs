// 로컬 건설 요청 0x3e 의 25바이트 직렬화와 서버 수신 444af0(cpppj `o/RawConstructionCommand`)의 웹 대응 `core/raw-construction-command.js` 를 검사한다(4-C, WB04-E322).
//  - 원본 독립 관측 `construction-command-write-x86.tsv` 64행: 전송 버퍼 25바이트와 호출자 뒤 버퍼(0xa5 채움) 보존을 대조한다.
//  - 원본 독립 관측 `construction-command-receive-x86.tsv` 144행: 차단/로컬/부족/NaN/Request 반환값과 경계 뒤 패킷·비용 변경(변이 1·2)의 하위 호출 인자와 순서를 대조한다.
//  - 손 시험(cpppj `ConstructionCommandTests.cpp` 이식 + 경계): 실제 분배기 등록과 길이/버전/준비 순서, 현재 비용 관문 재조회와 음수 sender, 헤더·프로토콜 마스크·인자 WORD·시각 필드, DWORD 비교, 구성 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E322)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { PacketConnection, RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { CONSTRUCTION_COMMAND_BYTES, RawConstructionCommand, registerConstructionCommand, writeConstructionCommand } from '../www/core/raw-construction-command.js';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';

// 원본 관측 파일이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-11 WB04-E322)
const WRITE_ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'construction-command-write-x86.tsv'));
const RECEIVE_ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'construction-command-receive-x86.tsv'));

// 바이트를 소문자 16진 문자열로 바꾸고 그 반대로 복원한다. (2026-11-11 WB04-E322)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
const unhex = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => Number.parseInt(pair, 16));

// 비트(uint32)와 단정도 float 를 서로 바꾼다. (2026-11-11 WB04-E322)
function bitsToFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, Number(bits) >>> 0, true);
    return view.getFloat32(0, true);
}
function floatToBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// little endian 값을 지정 폭만 쓴다. (2026-11-11 WB04-E322)
function put(bytes, offset, value, width = 4) {
    for (let index = 0; index < width; index += 1) bytes[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 타입 레코드 188개(비용만 사용)이다. (2026-11-11 WB04-E322)
const makeTypes = () => Array.from({ length: 188 }, () => ({ cost: 0 }));

// 하위 호출 인자를 원본 경계와 같은 형식으로 기록한다: 부족/취소는 (타입, 소유자, x, y, 인자, 방향), 전체 요청은 (타입, x, y, 소유자, 인자, 방향, 플래그, 시각 하위, 시각 상위, 품질)이다. (2026-11-11 WB04-E322)
function commandEvent(events, label, value, whole) {
    const parts = [label, value.type];
    if (!whole) parts.push(value.player);
    parts.push(floatToBits(value.x), floatToBits(value.y));
    if (whole) parts.push(value.player);
    parts.push(value.argument, value.direction);
    if (whole) parts.push(value.flags, value.timeLow, value.timeHigh, value.quality);
    events.push(parts.join(':'));
}

// 64행 재생: 두 정밀도에서 같았던 모든 원본 송신 버퍼를 직접 대조한다. (2026-11-11 WB04-E322)
test('건설 요청: 원본 관측 64행의 25바이트 송신 버퍼', { skip: WRITE_ROWS === null }, () => {
    let count = 0;
    for (const row of WRITE_ROWS) {
        // 원본 관측의 열 이름은 입력 행과 구별한다.
        if (row[0] === 'type') continue;
        assert.equal(row.length, 7);
        const message = { type: Number(row[0]), x: bitsToFloat(row[1]), y: bitsToFloat(row[2]), argument: Number(row[3]), direction: Number(row[4]), quality: Number(row[5]), timeLow: 0, timeHigh: 0 };
        const output = new Uint8Array(32).fill(0xa5);
        assert.equal(writeConstructionCommand(message, output), 25);
        assert.equal(hex(output.subarray(0, 25)), row[6]);
        assert.ok(output.subarray(25).every((byte) => byte === 0xa5));
        count += 1;
    }
    assert.equal(count, 64);
});

// 144행 재생: 원본은 Money/Request 뒤 같은 패킷 필드를 다시 읽는다. (2026-11-11 WB04-E322)
test('건설 요청: 원본 관측 144행의 수신 비용 관문·Request·환불', { skip: RECEIVE_ROWS === null }, () => {
    let count = 0;
    for (const row of RECEIVE_ROWS) {
        if (row[0] === 'blocked') continue;
        assert.equal(row.length, 9);
        const types = makeTypes();
        const state = { blocked: Number(row[0]), chargeBlocked: Number(row[1]) !== 0, localPlayer: Number(row[3]) };
        types[100].cost = bitsToFloat(row[5]);
        const packet = unhex('013e1900640000a4410000aa41efbefa80efcdab890000f87f');
        const events = [];
        const mutation = Number(row[7]);
        const hooks = {
            money: (owner) => {
                events.push(`M:${owner}`);
                if (mutation === 1) { types[100].cost = 1; packet[4] = 101; put(packet, 5, 0xc0500000); }
                return bitsToFloat(row[4]);
            },
            shortfall: (value) => { commandEvent(events, 'S', value, false); },
            request: (value) => {
                commandEvent(events, 'Q', value, true);
                if (mutation === 2) { packet[4] = 101; put(packet, 9, 0xc0200000); put(packet, 13, 0x1234, 2); packet[15] = 9; }
                return Number(row[6]) !== 0;
            },
            reject: (value) => { commandEvent(events, 'R', value, false); },
        };
        new RawConstructionCommand(types, state, hooks).receive(Number(row[2]), packet);
        // C++ 의 직접 TYPE 조회에는 별도 외부 호출이 없으므로 그 경계 표식(T:)만 제외한다.
        const expected = row[8].split(';').filter((event) => event !== '-' && !event.startsWith('T:'));
        assert.deepEqual(events, expected, `${row.slice(0, 8).join(',')}`);
        count += 1;
    }
    assert.equal(count, 144);
});

// 현재 패킷 분배기의 등록·길이·버전·sender DWORD 와 준비 순서를 같은 수신 몸체로 확인한다. (2026-11-11 WB04-E322)
test('건설 요청: 실제 분배기 등록·길이 25·준비 순서·음수 sender', () => {
    const types = makeTypes();
    let prepared = 0;
    let requests = 0;
    const device = { now: () => 123, record: () => {}, terminate: () => { throw new Error('정책0은 종료하지 않는다'); }, close: () => {}, release: () => {} };
    const hooks = {
        money: () => 0,
        shortfall: () => { throw new Error('비용0은 부족하지 않다'); },
        reject: () => { throw new Error('성공 요청의 취소 금지'); },
        request: (value) => {
            assert.deepEqual([value.player, value.type, value.flags, value.timeLow, value.timeHigh], [0xffffffff, 100, 0, 0, 0]);
            requests += 1;
            return true;
        },
    };
    const body = new RawConstructionCommand(types, { blocked: 0, chargeBlocked: false, localPlayer: 1 }, hooks);
    const dispatch = new RawPacketDispatch(device);
    registerConstructionCommand(dispatch, body, () => { prepared += 1; });
    const registration = dispatch.registration(0x3e);
    assert.deepEqual([registration.version, registration.mismatch, registration.length, typeof registration.observer], [1, 0, 25, 'function']);
    // 빈 observer 이다.
    assert.equal(registration.observer(1), undefined);
    const packet = new Uint8Array(25);
    writeConstructionCommand({ type: 100, x: 0, y: 0, argument: 0, direction: 0, quality: 0 }, packet);
    const connection = new PacketConnection();
    connection.sender = 0xffffffff;
    // 선언 길이 24 는 분배하지 않고, 25 와 protocol 상위 비트(0x81)는 분배한다.
    packet[2] = 24;
    assert.equal(dispatch.dispatch(connection, packet), true);
    assert.deepEqual([prepared, requests], [0, 0]);
    packet[2] = 25;
    packet[0] = 0x81;
    assert.equal(dispatch.dispatch(connection, packet), true);
    assert.deepEqual([prepared, requests, connection.received, connection.depth], [1, 1, 123, 0]);
    // 준비가 없어도 수신은 한다.
    const bare = new RawPacketDispatch(device);
    registerConstructionCommand(bare, body);
    packet[0] = 1;
    assert.equal(bare.dispatch(connection, packet), true);
    assert.equal(requests, 2);
});

// 준비 시점 사본 대신 실제 비용 관문에서 현재 전역을 읽고 음수 sender 와 선행 TYPE/차단 순서를 보존한다. (2026-11-11 WB04-E322)
test('건설 요청: 현재 비용 관문 재조회와 차단·TYPE 범위 순서', () => {
    const types = makeTypes();
    types[100].cost = 100;
    let current = true;
    const state = { blocked: 0, chargeBlocked: false, localPlayer: 1, chargeBlockedRead: () => { counts.reads += 1; return current; } };
    const counts = { reads: 0, money: 0, requests: 0, shortfalls: 0 };
    const hooks = {
        money: (sender) => { assert.equal(sender, 0xffffffff); counts.money += 1; return 0; },
        shortfall: (request) => { assert.equal(request.player, 0xffffffff); counts.shortfalls += 1; },
        request: (request) => { assert.ok(request.player === 0xffffffff || request.player === 1); counts.requests += 1; return true; },
        reject: () => { throw new Error('성공 요청의 예상 밖 환불'); },
    };
    const body = new RawConstructionCommand(types, state, hooks);
    const packet = new Uint8Array(25);
    writeConstructionCommand({ type: 100, x: 0, y: 0, argument: 0, direction: 0, quality: 0 }, packet);
    body.receive(0xffffffff, packet);
    assert.deepEqual({ ...counts }, { reads: 1, money: 0, requests: 1, shortfalls: 0 });
    current = false;
    body.receive(0xffffffff, packet);
    assert.deepEqual({ ...counts }, { reads: 2, money: 1, requests: 1, shortfalls: 1 });
    // 명령 수신은 로컬 sender 비교보다 먼저 전역을 읽는다.
    body.receive(1, packet);
    assert.deepEqual({ ...counts }, { reads: 3, money: 1, requests: 2, shortfalls: 1 });
    // 현재 타입 번호가 표 밖이면 비용 관문 전역을 읽기 전에 거부한다.
    packet[4] = 255;
    assert.throws(() => body.receive(0xffffffff, packet), /TYPE 범위/);
    assert.equal(counts.reads, 3);
    packet[4] = 188;
    assert.throws(() => body.receive(0xffffffff, packet), /TYPE 범위/);
    packet[4] = 187;
    body.receive(1, packet);
    assert.equal(counts.reads, 4);
    // 첫 차단은 본문과 현재 전역 조회 모두를 자연 생략한다.
    state.blocked = 1;
    body.receive(0xffffffff, new Uint8Array(0));
    assert.equal(counts.reads, 4);
});

// 헤더·프로토콜 마스크·WORD 인자·시각 필드와 저장소 거부, 호출자 뒤 버퍼 보존. (2026-11-11 WB04-E322)
test('건설 요청: 송신 버퍼의 헤더·필드 폭·시각·저장소 크기', () => {
    const message = { type: 0x1ff, x: 20.5, y: -3.25, argument: 0x12345, direction: 0x1ab, quality: 0x2cd, timeLow: 0x11223344, timeHigh: 0x55667788 };
    const packet = new Uint8Array(30).fill(0x77);
    assert.equal(writeConstructionCommand(message, packet), CONSTRUCTION_COMMAND_BYTES);
    // 값은 필드 폭만 쓴다: 타입 BYTE, 인자 WORD, 방향/품질 BYTE, 시각은 두 DWORD.
    assert.equal(hex(packet.subarray(0, 25)), '013e1900ff' + '0000a441' + '000050c0' + '4523' + 'ab' + 'cd' + '44332211' + '88776655');
    assert.ok(packet.subarray(25).every((byte) => byte === 0x77));
    // 시각이 없으면 0 이다(메시지 생성자의 0 초기화).
    const zero = new Uint8Array(25).fill(0xee);
    writeConstructionCommand({ type: 1, x: 0, y: 0, argument: 0, direction: 0, quality: 0 }, zero);
    assert.equal(hex(zero.subarray(17)), '0000000000000000');
    // 헤더: 프로토콜은 상위 비트를 제거하고 메시지/선언 길이는 그대로 쓴다.
    const header = new Uint8Array(25);
    writeConstructionCommand(message, header, { protocol: 0xff, message: 0x41, length: 0x1234 });
    assert.equal(hex(header.subarray(0, 4)), '7f413412');
    // 저장소가 25바이트보다 작으면 거부하고 아무것도 쓰지 않는다.
    const small = new Uint8Array(24).fill(0x55);
    assert.throws(() => writeConstructionCommand(message, small), /25바이트/);
    assert.ok(small.every((byte) => byte === 0x55));
    assert.throws(() => writeConstructionCommand(message, Array.from({ length: 25 }, () => 0)), /25바이트/);
    // float 는 단정도로 반올림해 쓴다(0.1 → 0x3dcccccd).
    const rounded = new Uint8Array(25);
    writeConstructionCommand({ type: 1, x: 0.1, y: 0, argument: 0, direction: 0, quality: 0 }, rounded);
    assert.equal(hex(rounded.subarray(5, 9)), 'cdcccc3d');
});

// 수신 세부: 차단은 DWORD 비영, 로컬 sender 비교는 전체 DWORD, 하위 호출마다 현재 필드 재조회, 본문 길이 부족, 잔액은 단정도, 시각은 읽지 않음. (2026-11-11 WB04-E322)
test('건설 요청: 수신 세부 경계', () => {
    const base = () => {
        const packet = new Uint8Array(25);
        writeConstructionCommand({ type: 100, x: 1.5, y: 2.5, argument: 0xbeef, direction: 7, quality: 9, timeLow: 0xdeadbeef, timeHigh: 0xcafef00d }, packet);
        return packet;
    };
    const run = (state, mutate = {}, packet = base(), cost = 10) => {
        const types = makeTypes();
        types[100].cost = cost;
        const events = [];
        const hooks = {
            money: (owner) => { events.push(['money', owner]); return mutate.money ?? 0; },
            shortfall: (value) => { events.push(['shortfall', value]); },
            request: (value) => { events.push(['request', value]); return mutate.accept ?? true; },
            reject: (value) => { events.push(['reject', value]); },
        };
        new RawConstructionCommand(types, state, hooks).receive(mutate.sender ?? 2, packet);
        return events;
    };
    const live = { blocked: 0, chargeBlocked: false, localPlayer: 1 };
    // 요청 본문: 시각은 읽지 않고 0/0, 플래그 0, 인자는 WORD, sender 는 그대로 소유자이다.
    const [, [, request]] = run(live);
    assert.deepEqual(request, { type: 100, x: 1.5, y: 2.5, player: 2, argument: 0xbeef, direction: 7, flags: 0, timeLow: 0, timeHigh: 0, quality: 9 });
    // 차단은 상위 비트만 있는 DWORD 도 차단이다.
    assert.deepEqual(run({ ...live, blocked: 0x80000000 }), []);
    assert.deepEqual(run({ ...live, blocked: 0x100000000 }).map(([name]) => name), ['money', 'shortfall']);
    // 로컬 번호 비교는 전체 DWORD 이다(BYTE 로 줄이지 않음): 하위 바이트가 같아도 DWORD 가 다르면 다른 sender 라 비용을 검사한다. 32비트를 넘는 값은 DWORD 로 감긴다.
    assert.deepEqual(run(live, { sender: 1, money: 0 }).map(([name]) => name), ['request']);
    assert.deepEqual(run({ ...live, localPlayer: 0x101 }, { sender: 1 }).map(([name]) => name), ['money', 'shortfall']);
    assert.deepEqual(run({ ...live, localPlayer: 1 }, { sender: 0x101 }).map(([name]) => name), ['money', 'shortfall']);
    assert.deepEqual(run({ ...live, localPlayer: 0x100000001 }, { sender: 1 }).map(([name]) => name), ['request']);
    assert.deepEqual(run({ ...live, localPlayer: 1 }, { sender: 0x100000001 }).map(([name]) => name), ['request']);
    // 비용 선검사 차단(속성 값)이면 잔액을 부르지 않는다.
    assert.deepEqual(run({ ...live, chargeBlocked: true }).map(([name]) => name), ['request']);
    // 잔액 == 비용은 부족이 아니고(ordered less), 단정도 반올림 뒤 비교한다: 10.00000001 → 10.
    assert.deepEqual(run(live, { money: 10 }).map(([name]) => name), ['money', 'request']);
    assert.deepEqual(run(live, { money: 10.00000001 }).map(([name]) => name), ['money', 'request']);
    assert.deepEqual(run(live, { money: 9.999999 }).map(([name]) => name), ['money', 'shortfall']);
    assert.deepEqual(run(live, { money: Number.NaN }).map(([name]) => name), ['money', 'request']);
    assert.deepEqual(run(live, { money: -Infinity }).map(([name]) => name), ['money', 'shortfall']);
    assert.deepEqual(run(live, { money: 0 }, base(), Number.NaN).map(([name]) => name), ['money', 'request']);
    // Request 가 거짓이면 같은 현재 필드로 환불/취소를 부른다.
    assert.deepEqual(run(live, { money: 10, accept: false }).map(([name]) => name), ['money', 'request', 'reject']);
    // 부족 요청과 환불 요청에도 현재 패킷 필드가 같은 형식으로 전달된다(소유자·좌표·인자·방향).
    const [, [, shortfall]] = run(live);
    assert.deepEqual([shortfall.type, shortfall.player, shortfall.x, shortfall.y, shortfall.argument, shortfall.direction], [100, 2, 1.5, 2.5, 0xbeef, 7]);
    // 본문이 17바이트 미만이면 비용 관문 전에 거부하고 17바이트면 읽는다(시각 자리는 필요 없음).
    assert.throws(() => run(live, {}, new Uint8Array(16)), /본문 부족/);
    assert.deepEqual(run(live, { money: 10 }, base().slice(0, 17)).map(([name]) => name), ['money', 'request']);
    // sender 는 부호 없는 DWORD 로 전달한다(음수 입력도 같은 값).
    assert.equal(run(live, { sender: -1 })[0][1], 0xffffffff);
    // 잔액은 단정도 반환값이라 반올림 뒤 비교한다(9.99999999 → 10 이라 부족이 아님).
    assert.deepEqual(run(live, { money: 9.99999999 }).map(([name]) => name), ['money', 'request']);
    // 비용은 패킷의 현재 타입 번호의 것을 읽는다(타입 101 만 비용이 있음).
    const other = base();
    other[4] = 101;
    const types = makeTypes();
    types[101].cost = 50;
    const events = [];
    new RawConstructionCommand(types, live, {
        money: () => 20, shortfall: (value) => { events.push(['shortfall', value.type]); }, request: (value) => { events.push(['request', value.type]); return true; }, reject: () => {},
    }).receive(2, other);
    assert.deepEqual(events, [['shortfall', 101]]);
});

// 구성 거부와 등록의 선택 인자. (2026-11-11 WB04-E322)
test('건설 요청: 구성 검사', () => {
    const hooks = () => ({ money: () => 0, shortfall: () => {}, request: () => true, reject: () => {} });
    const state = { blocked: 0, chargeBlocked: false, localPlayer: 1 };
    assert.doesNotThrow(() => new RawConstructionCommand(makeTypes(), state, hooks()));
    // 타입 표는 정확히 188개여야 한다.
    assert.throws(() => new RawConstructionCommand(makeTypes().slice(0, 187), state, hooks()), TypeError);
    assert.throws(() => new RawConstructionCommand([...makeTypes(), { cost: 0 }], state, hooks()), TypeError);
    assert.throws(() => new RawConstructionCommand(null, state, hooks()), TypeError);
    assert.throws(() => new RawConstructionCommand(makeTypes(), null, hooks()), TypeError);
    // 네 하위가 모두 필수이다.
    for (const name of ['money', 'shortfall', 'request', 'reject']) {
        const broken = hooks();
        delete broken[name];
        assert.throws(() => new RawConstructionCommand(makeTypes(), state, broken), /수신 연결 오류/, name);
    }
    assert.throws(() => new RawConstructionCommand(makeTypes(), state, undefined), TypeError);
    // 거부 사유는 같은 연결 오류이다(타입 표/상태가 객체가 아닌 경우 포함).
    const invalid = /수신 연결 오류/;
    assert.throws(() => new RawConstructionCommand(null, state, hooks()), invalid);
    assert.throws(() => new RawConstructionCommand(makeTypes(), 5, hooks()), invalid);
    assert.throws(() => new RawConstructionCommand(makeTypes(), null, hooks()), invalid);
    // 수신 입력은 실제 바이트 배열이어야 한다(일반 배열은 길이가 충분해도 거부).
    const body = new RawConstructionCommand(makeTypes(), state, hooks());
    assert.throws(() => body.receive(2, Array.from({ length: 25 }, () => 0)), /본문 부족/);
});

// 등록된 수신기는 준비를 먼저 호출한 뒤 수신하고, 분배기가 넘긴 sender 를 그대로(연결 값이 아니라) 쓴다. (2026-11-11 WB04-E322)
test('건설 요청: 등록 수신기의 준비 순서와 sender 인자', () => {
    const events = [];
    const hooks = { money: () => 0, shortfall: () => {}, request: (value) => { events.push(`request:${value.player}`); return true; }, reject: () => {} };
    const body = new RawConstructionCommand(makeTypes(), { blocked: 0, chargeBlocked: false, localPlayer: 1 }, hooks);
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    registerConstructionCommand(dispatch, body, () => { events.push('prepare'); });
    const packet = new Uint8Array(25);
    writeConstructionCommand({ type: 1, x: 0, y: 0, argument: 0, direction: 0, quality: 0 }, packet);
    dispatch.registration(0x3e).receive(5, new PacketConnection({ sender: 9 }), packet);
    assert.deepEqual(events, ['prepare', 'request:5']);
});
