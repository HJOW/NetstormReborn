// 로그인 서버의 Player(0x2b)/중앙섬 metadata(0x72) 작성과 송신 순서(cpppj `o/RawMissionServerPackets`)의 웹 대응 `core/raw-mission-server-packets.js` 를 검사한다(4-C, WB04-E315).
//  - 원본 독립 관측 `mission-server-packets-x86.tsv` 187행(Player 송신 P·metadata 송신 M·metadata 수신 R × 서버/metamap/생성 표시/Zone bit/군도 × 이름·flags·대상)의 전체 Player 1620바이트·현재 전역 5개·외부 효과 순서와 송신된 wire 바이트를 재생한다.
//    송신 장치는 실제 observer 후 wire 를 캡처한다(0x72 는 서버 observer, 0x2b 는 클라이언트 패킷 observer).
//  - 손 시험(cpppj `MissionServerPacketsTests.cpp` 이식 + 경계): 도달한 위치에서만 미연결 하위를 거부하고 이름 정제는 주소 경계 누락 전에 이미 수행됨, 이미 생성한 자기 metadata 는 소비자를 부르지 않음, 중앙섬 생성 경로와 군도 배치 2, observer, 등록, 구성 검사.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E315)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { PacketConnection, RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { observeMissionClientPacket } from '../www/core/raw-mission-client-packets.js';
import { RawMissionServerPackets, observeMissionMetadataPacket, registerMissionMetadata } from '../www/core/raw-mission-server-packets.js';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';

// 원본 관측 파일이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-11 WB04-E315)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'mission-server-packets-x86.tsv'));

// 바이트를 소문자 16진 문자열로, 16진 문자열을 바이트열로 바꾼다. (2026-11-11 WB04-E315)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
const unhex = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => Number.parseInt(pair, 16));
const textHex = (text) => hex(Array.from(text, (char) => char.charCodeAt(0)));

// little endian DWORD 를 읽고 쓴다. (2026-11-11 WB04-E315)
const get = (bytes, offset, width = 4) => { let value = 0; for (let index = 0; index < width; index += 1) value |= bytes[offset + index] << (8 * index); return value >>> 0; };
function put(bytes, offset, value) {
    for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 원본에 공급한 명시 Player9 입력만 만들며 송신/정제 결과를 계산하지 않는다: 전체 stale 바이트를 native 와 같은 패턴으로 채운다. (2026-11-11 WB04-E315)
function serverInput(owner, name, flags) {
    const bytes = new Uint8Array(1620);
    for (let index = 0; index < bytes.length; index += 1) bytes[index] = (index * 37 + 11) & 0xff;
    const offset = owner * 180;
    bytes.fill(0, offset + 12, offset + 12 + 32);
    bytes.set(name, offset + 12);
    bytes[offset + 12 + name.length] = 0;
    put(bytes, offset + 0x2c, (0x12345680 + owner) >>> 0);
    put(bytes, offset + 0x50, flags);
    return bytes;
}

// 내부 실제 이름 정제 진입은 제품의 외부 효과가 아니므로 외부 trace 비교에서만 제외한다(`N,` 항목). (2026-11-11 WB04-E315)
function externalTrace(native) {
    const result = native.split(';').filter((event) => event && !event.startsWith('N,')).join(';');
    return result || '-';
}

// 187행 재생: 열은 input/players/globals/trace 이다. 입력 열: kind,owner,nameHex,flags,server,metamap,target,created,zone,archipelago. (2026-11-11 WB04-E315)
test('미션 서버 패킷: 원본 관측 187행의 전체 Player·전역·wire·효과', { skip: ROWS === null }, () => {
    let count = 0;
    // 각 행에서 새 실제 저장소를 빌려 이름 정제/송신 효과가 다음 행에 누적되지 않게 한다.
    for (const row of ROWS) {
        if (row[0] === 'input') continue;
        assert.equal(row.length, 4);
        const fields = row[0].split(',');
        assert.equal(fields.length, 10);
        const owner = Number(fields[1]);
        const flags = Number(fields[3]);
        const name = fields[2] === '-' ? new Uint8Array(0) : unhex(fields[2]);
        const players = serverInput(owner, name, flags);
        const globals = { server: Number(fields[4]) !== 0, metamap: Number(fields[5]) !== 0, localOwner: 1, centralCreated: Number(fields[7]), archipelago: Number(fields[9]), zoneAddress: new Uint8Array(6) };
        put(globals.zoneAddress, 0, Number(fields[8]));
        const trace = [];
        const effect = (event) => { trace.push(event); };
        let current = null;
        const hooks = {
            assertion: (text) => { effect(`A,${textHex(text)}`); },
            log: (text) => { effect(`L,${textHex(text)}`); },
            peerAddress: (player) => { effect(`IP,${player}`); return '127.0.0.1'; },
            // 송신 장치는 실제 observer 후 wire 를 캡처한다(0x72 는 서버 observer, 그 밖은 클라이언트 패킷 observer).
            send: (target, bytes) => {
                if (bytes[1] === 0x72) current.observeMetadata(bytes, 1);
                else observeMissionClientPacket(bytes[1], bytes, 1, (text) => { effect(`A,${textHex(text)}`); });
                effect(`S,${target >>> 0},${hex(bytes.subarray(0, get(bytes, 2, 2)))}`);
            },
        };
        const packets = new RawMissionServerPackets(players, globals, hooks);
        current = packets;
        const target = Number(fields[6]) | 0;
        if (fields[0] === 'P') packets.sendPlayer(owner, target);
        else if (fields[0] === 'M') packets.sendMetadata(target);
        else {
            const bytes = new Uint8Array(32).fill(0xff);
            bytes.set(name, 10);
            bytes[10 + name.length] = 0;
            packets.receiveMetadata(bytes);
        }
        assert.equal(hex(players), row[1], `행 ${count} Player`);
        assert.equal(`${globals.server ? 1 : 0},${globals.localOwner},${globals.centralCreated},${get(globals.zoneAddress, 0)},${globals.archipelago}`, row[2], `행 ${count} 전역`);
        assert.equal(trace.length ? trace.join(';') : '-', externalTrace(row[3]), `행 ${count} 효과`);
        count += 1;
    }
    assert.equal(count, 187);
});

// 원본이 실제 도달한 누락과 미복원 중앙섬 생성은 빈 성공으로 바꾸지 않는다. 등록은 0x72 의 observer·버전 1/정책 0·수신기이다. (2026-11-11 WB04-E315)
test('미션 서버 패킷: 선택된 경계의 누락과 등록', () => {
    const players = serverInput(1, new Uint8Array(0), 1);
    const globals = { server: true, metamap: false, localOwner: 1, centralCreated: 1, archipelago: 0, zoneAddress: new Uint8Array(6) };
    const packets = new RawMissionServerPackets(players, globals, {});
    // 주소 장치가 없어 Player 송신은 거부하지만, 실제 이름 정제('Cheat1')는 도달한 주소 경계 누락 이전에 이미 수행된다.
    assert.throws(() => packets.sendPlayer(1, 2), /실제 하위 미연결/);
    assert.equal(players[180 + 12], 0x43);
    // 이미 생성한 자기 metadata 는 어떤 소비자도 호출하지 않는다.
    assert.doesNotThrow(() => packets.sendMetadata(1));
    // 중앙섬 생성 소비자가 없으면 거부한다.
    put(globals.zoneAddress, 0, 0x100);
    const central = new Uint8Array(11);
    central[10] = 0x24;
    assert.throws(() => packets.receiveMetadata(central), /실제 하위 미연결/);
    const dispatch = new RawPacketDispatch({ now: () => 0, record: () => {}, terminate: () => { throw new Error('등록만 검사'); }, close: () => { throw new Error('등록만 검사'); }, release: () => { throw new Error('등록만 검사'); } });
    registerMissionMetadata(dispatch, packets);
    const entry = dispatch.registration(0x72);
    assert.deepEqual([entry.version, entry.mismatch, typeof entry.receive, typeof entry.packetObserver], [1, 0, 'function', 'function']);
    // 송신 observer 는 진단 장치(없음)가 필요한 센티넬에서 거부한다.
    assert.throws(() => entry.packetObserver(Uint8Array.of(1, 0x72, 4, 0, 0, 0, 0xff, 0xff, 0xff, 0xff, 0), 1), /실제 하위 미연결/);
    // 수신 방향 observer 는 읽지 않는다.
    assert.doesNotThrow(() => entry.packetObserver(new Uint8Array(0), 0));
});

// 중앙섬 생성 경로: Zone bit 0x100 이 없으면 로그만 남기고 반환, `$` 가 아니면 반환, `$` 이면 안내→로그→군도 기록→배치 선택→(특수 2 는 반환)→Zone 6바이트 중앙섬 생성→생성 표시→예측수 로그이다. (2026-11-11 WB04-E315)
test('미션 서버 패킷: 중앙섬 metadata 수신의 분기와 순서', () => {
    const players = serverInput(1, new Uint8Array(0), 1);
    const globals = { server: true, metamap: false, localOwner: 1, centralCreated: 0, archipelago: 0, zoneAddress: Uint8Array.of(0, 1, 2, 3, 4, 5) };
    const log = [];
    let layout = 0;
    const hooks = {
        assertion: (text) => { log.push(`assert:${text}`); },
        log: (text) => { log.push(`log:${text}`); },
        creatingIslands: () => { log.push('creating'); },
        setArchipelago: (value) => { log.push(`archipelago:${value}`); globals.archipelago = value; },
        setCentralCreated: (value) => { log.push(`created:${value}`); globals.centralCreated = value; },
        layout: () => { log.push('layout'); return layout; },
        randomCentral: (zone) => { log.push(`random:${hex(zone)}`); },
        surroundedCentral: () => { log.push('surrounded'); },
        predictableCount: () => { log.push('count'); return 0xfffffffd; },
        peerAddress: () => '10.0.0.1',
        send: () => { log.push('send'); },
    };
    const packets = new RawMissionServerPackets(players, globals, hooks);
    const central = (extra = 0x24, archipelago = 3) => { const bytes = new Uint8Array(32).fill(0xff); bytes[10] = extra; bytes[5] = archipelago; return bytes; };
    // Zone bit 0x100 이 없으면 로그만 남긴다. (zone 앞 DWORD 0x03020100 에는 bit 8 이 켜져 있음 → bit 8 을 지워 확인)
    globals.zoneAddress[1] = 0;
    packets.receiveMetadata(central());
    assert.deepEqual(log.splice(0), ['log:No Central Island.\n']);
    // bit 0x100 이 있고 `$` 가 아니면 반환한다.
    globals.zoneAddress[1] = 1;
    packets.receiveMetadata(central(0x41));
    assert.deepEqual(log.splice(0), []);
    // `$` 이고 일반 배치면 중앙섬을 만들고 생성 표시를 켜며 예측수(signed)를 로그에 남긴다. zone 6바이트를 그대로 넘긴다.
    packets.receiveMetadata(central());
    assert.deepEqual(log.splice(0), ['creating', 'log:createArchipelago... ', 'archipelago:3', 'layout', 'random:000102030405', 'created:1', 'count', 'log:predictableCount = -3\n']);
    assert.equal(globals.centralCreated, 1);
    // 특수 배치 2 는 생성 표시/예측수 로그 없이 반환한다.
    layout = 2;
    globals.centralCreated = 0;
    packets.receiveMetadata(central());
    assert.deepEqual(log.splice(0), ['creating', 'log:createArchipelago... ', 'archipelago:3', 'layout', 'surrounded']);
    assert.equal(globals.centralCreated, 0);
    // 짧은 원문은 거부한다.
    assert.throws(() => packets.receiveMetadata(new Uint8Array(10)), RangeError);
    // 자기 metadata 송신: 아직 중앙섬이 없으면 같은 수신 몸체를 먼저 실행하고(서버가 아니면 진단), 대상이 로컬이면 송신하지 않는다.
    layout = 0;
    globals.centralCreated = 0;
    globals.archipelago = 7;
    packets.sendMetadata(1);
    assert.ok(log.includes('archipelago:0') === false && log.includes('creating'));
    assert.ok(!log.includes('send'));
    log.length = 0;
    globals.centralCreated = 0;
    globals.server = false;
    packets.sendMetadata(5);
    assert.deepEqual(log.slice(0, 1), ['assert:server']);
    assert.equal(log[log.length - 1], 'send');
});

// 0x72 observer: 송신은 실제 길이 DWORD 가 -1 이면 진단 뒤 선언 WORD = 낮은 WORD + 10, 수신은 무동작. 센티넬 뒤 현재 값을 다시 읽는다. (2026-11-11 WB04-E315)
test('미션 서버 패킷: 0x72 observer 의 길이 확정', () => {
    const seen = [];
    const packet = new Uint8Array(24);
    put(packet, 6, 8);
    observeMissionMetadataPacket(packet, 1, (text) => { seen.push(text); });
    assert.equal(packet[2] | (packet[3] << 8), 18);
    assert.deepEqual(seen, []);
    const before = Array.from(packet);
    observeMissionMetadataPacket(packet, 0, undefined);
    assert.deepEqual(Array.from(packet), before);
    put(packet, 6, 0xffffffff);
    observeMissionMetadataPacket(packet, 1, (text) => { seen.push(text); });
    assert.deepEqual(seen, ['len != (int)-1']);
    assert.equal(packet[2] | (packet[3] << 8), (0xffff + 10) & 0xffff);
    // 진단 장치 없이 센티넬이면 거부한다. 진단이 packet 을 바꾸면 복귀 뒤 현재 값으로 쓴다.
    assert.throws(() => observeMissionMetadataPacket(packet, 1, undefined), /실제 하위 미연결/);
    observeMissionMetadataPacket(packet, 1, () => { put(packet, 6, 100); });
    assert.equal(packet[2] | (packet[3] << 8), 110);
});

// Player 송신(0x2b)의 필드 배치와 연결 gate: 비접속은 주소 0, 접속은 주소 장치 문자열, 번호 9 이상/metamap 진단/서버 아님 진단. (2026-11-11 WB04-E315)
test('미션 서버 패킷: Player 송신의 필드·주소·진단', () => {
    const players = serverInput(2, Uint8Array.from('Bob', (char) => char.charCodeAt(0)), 0x1);
    put(players, 2 * 180 + 0x54, 0x0a0b0c0d);
    players[2 * 180 + 0x5c] = 0x44;
    players[2 * 180 + 0x60] = 0x55;
    players[2 * 180 + 0x61] = 0x66;
    players[2 * 180 + 0xa0] = 0x77;
    players[2 * 180 + 0xa4] = 0x88;
    players[2 * 180 + 0xa5] = 0x99;
    players[2 * 180 + 0x71] = 0xaa;
    const globals = { server: true, metamap: true, localOwner: 1, centralCreated: 1, archipelago: 0, zoneAddress: new Uint8Array(6) };
    const log = [];
    let sent = null;
    const packets = new RawMissionServerPackets(players, globals, {
        assertion: (text) => { log.push(`assert:${text}`); },
        peerAddress: (player) => { log.push(`ip:${player}`); return '192.168.0.7'; },
        send: (target, bytes) => { log.push(`send:${target}`); sent = Uint8Array.from(bytes); },
    });
    packets.sendPlayer(2, -3);
    // metamap 진단은 번호를 조회할 때마다 나온다(번호 쓰기 + 주소 조회 = 두 번).
    assert.deepEqual(log, ['assert:!inMetamapMode', 'assert:!inMetamapMode', 'ip:2', 'send:-3']);
    assert.equal(sent.length, 100);
    assert.deepEqual(Array.from(sent.subarray(0, 4)), [1, 0x2b, 4, 0]);
    assert.equal(sent[0x32], 2);
    assert.equal(sent[0x33], 4);
    assert.equal(hex(sent.subarray(0x3b, 0x3f)), '426f6200');
    assert.equal(hex(sent.subarray(4, 16)), '3139322e3136382e302e3700'.slice(0, 24));
    assert.equal(get(sent, 0x26), 0x12345682);
    assert.equal(get(sent, 0x2a), 1);
    assert.equal(get(sent, 0x22), 0x0a0b0c0d);
    assert.deepEqual([sent[0x34], sent[0x35], sent[0x36], sent[0x37], sent[0x38], sent[0x39], sent[0x3a]], [0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa]);
    // 접속하지 않은 Player 는 주소를 쓰지 않고(첫 BYTE 0) 주소 장치를 부르지 않는다. 나머지는 0xff 예약이 남는다.
    put(players, 2 * 180 + 0x50, 0x200);
    log.length = 0;
    globals.metamap = false;
    packets.sendPlayer(2, 5);
    assert.deepEqual(log, ['send:5']);
    assert.equal(sent[4], 0);
    assert.equal(sent[5], 0xff);
    // 서버가 아니면 진단 뒤에도 작성은 계속하고, 번호 9 이상은 거부한다.
    globals.server = false;
    log.length = 0;
    packets.sendPlayer(2, 5);
    assert.deepEqual(log, ['assert:server', 'send:5']);
    assert.throws(() => packets.sendPlayer(9, 5), RangeError);
    assert.throws(() => packets.sendPlayer(-1, 5), RangeError);
});

// 구성: 같은 Player9 와 6BYTE Zone 이 필요하다. (2026-11-11 WB04-E315)
test('미션 서버 패킷: 구성의 물리 크기 검사', () => {
    const state = { server: true, metamap: false, localOwner: 1, centralCreated: 0, archipelago: 0, zoneAddress: new Uint8Array(6) };
    assert.doesNotThrow(() => new RawMissionServerPackets(new Uint8Array(1620), state, {}));
    assert.throws(() => new RawMissionServerPackets(new Uint8Array(1619), state, {}), RangeError);
    assert.throws(() => new RawMissionServerPackets([], state, {}), RangeError);
    assert.throws(() => new RawMissionServerPackets(new Uint8Array(1620), { ...state, zoneAddress: new Uint8Array(5) }, {}), RangeError);
    assert.throws(() => new RawMissionServerPackets(new Uint8Array(1620), null, {}), RangeError);
});

// fixture 밖 경계(변형 확인에서 확인): 짧은 observer 입력의 물리 범위 거부, Player 이름/주소 복사의 예약 폭 경계(이름 40자 통과·41자 거부, 주소 95자 통과·96자 거부), 송신 직전 metadata 바이트, 로컬 번호의 unsigned 비교. (2026-11-11 WB04-E315)
test('미션 서버 패킷: 복사 폭 경계와 metadata 바이트·unsigned 대상 비교', () => {
    // 길이 DWORD 를 읽을 수 없는 짧은 입력은 거부한다.
    assert.throws(() => observeMissionMetadataPacket(new Uint8Array(9), 1, () => {}), RangeError);
    // 이름 복사: 송신 예약 100BYTE 의 +0x3b 이후 41칸 안에 NUL 까지 들어가야 하므로 이름은 40자까지이다.
    const withName = (length) => {
        const players = serverInput(1, new Uint8Array(length).fill(0x61), 0x200);
        const globals = { server: true, metamap: false, localOwner: 0, centralCreated: 1, archipelago: 0, zoneAddress: new Uint8Array(6) };
        let sent = null;
        const packets = new RawMissionServerPackets(players, globals, { send: (target, bytes) => { sent = Uint8Array.from(bytes); }, peerAddress: () => 'x' });
        return { call: () => packets.sendPlayer(1, 2), sent: () => sent };
    };
    const ok = withName(40);
    ok.call();
    assert.equal(ok.sent()[0x33], 41);
    assert.equal(ok.sent()[0x3b + 40], 0);
    assert.throws(() => withName(41).call(), RangeError);
    // 주소 복사: 예약의 +4 이후 96칸 안에 NUL 까지 들어가야 하므로 주소는 95자까지이다.
    const withAddress = (length) => {
        const players = serverInput(1, Uint8Array.from('A', (char) => char.charCodeAt(0)), 0x1);
        const globals = { server: true, metamap: false, localOwner: 0, centralCreated: 1, archipelago: 0, zoneAddress: new Uint8Array(6) };
        const packets = new RawMissionServerPackets(players, globals, { send: () => {}, peerAddress: () => 'z'.repeat(length) });
        return () => packets.sendPlayer(1, 2);
    };
    assert.doesNotThrow(withAddress(95));
    assert.throws(withAddress(96), RangeError);
    // 송신 직전 metadata: 선언 길이는 4(observer 가 나중에 확정), +4 는 0, +5 는 군도 낮은 BYTE, +6 은 이름 길이 DWORD(8), 이름은 `$Random` 이다.
    const globals = { server: true, metamap: false, localOwner: 1, centralCreated: 1, archipelago: 0x107, zoneAddress: new Uint8Array(6) };
    let raw = null;
    const packets = new RawMissionServerPackets(new Uint8Array(1620), globals, { send: (target, bytes) => { raw = hex(bytes); } });
    packets.sendMetadata(5);
    assert.equal(raw, '01720400' + '00' + '07' + '08000000' + '2452616e646f6d00');
    // 로컬 번호는 unsigned 로 비교한다: 대상 -1 과 로컬 0xffffffff 는 같은 번호이다.
    raw = null;
    globals.localOwner = 0xffffffff;
    packets.sendMetadata(-1);
    assert.equal(raw, null);
    packets.sendMetadata(0);
    assert.notEqual(raw, null);
});
