// 단독 미션 내부 전송 local-mission-transport.js(LocalMissionTransport·Zone/로그인 작성·Zone 설치)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: local-mission-transport-x86.tsv 66행 — Zone 2(512 예약 전체와 86바이트 선언), Login 24(이미지/이름 길이별 0x27 예약 전체와 선언 바이트), Batch 30·BatchClosed 2(집합 프레임 분배의
//    반환·flags·sender·두 시각·깊이와 now/record/terminate/receive 효과 순서), Install 6·InstallMutate 2(Zone 설치 뒤 Player9 전체·현재 전역 9값·Chat/시각 효과 순서).
//  - cpppj 손 시험(LocalMissionTransportTests 4건·ConnectionRegistrationTests 1건·ConnectionActivityTransportTests 2건): Zone 설치 → open → 로그인 0x27 → 실제 CheckLogin 재배치 → 응답 0x24 왕복,
//    선택 경계·예외 깊이, 서버 열기와 클라이언트 접속 분리, 공유 등록소의 pending/owner/9번 칸, Client 객체 존재와 flags 분리, 실패한 접속의 부분 효과.
//    cpppj 원 시험 중 셋(서버 열기 분리·공유 등록소·Client 존재 두 건)은 `open` 이 서버 끝을 바로 만들던 옛 설계의 기대값이라 cpppj 에서도 실패했다(783b12763 2026-11-06 의 poll accept 설계 이후, cpppj 1304 에서 같은 값으로 교정).
//    웹은 기준선 동작(서버 끝은 poll 의 accept 에서 생성)으로 기대값을 바꿔 옮기고 바뀐 값마다 주석에 cpppj 원 기대값을 적는다(단위 문서 3절).
//  - 웹 계약: 0x87 bootstrap 등록(비NULL 빈 관찰기)·종료된 CD 검증 0x7f/0x80 미송수신·`sent` 기록·`players` 현재 Player9 조회·가상 송신 장치의 직접 노드 송신·반대 끝 상실 통지 1회·closeAll·dispose·인계.
// 실제 게임·브라우저 실행이 아니다. (2026-11-12 WB04-E324: 4-C zacket/LocalMissionTransport 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { PacketConnection } from '../www/core/raw-packet-dispatch.js';
import { RawConnectionRegistry } from '../www/core/raw-connection-registry.js';
import { PlayerWorldState } from '../www/core/raw-player-world.js';
import { PlayerParticipationState, RawPlayerParticipation } from '../www/core/raw-player-participation.js';
import {
    LocalMissionTransport, installLocalMissionZone, makeGuideMissionZone, makeLocalMissionLogin, makeLocalMissionZone, readLocalMissionLogin,
} from '../www/core/local-mission-transport.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-12 WB04-E324)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'local-mission-transport-x86.tsv'));
// 바이트 배열의 16진 문자열이다. (2026-11-12 WB04-E324)
const hex = (bytes) => Buffer.from(bytes).toString('hex');
// 16진 문자열을 바이트 배열로 바꾼다. (2026-11-12 WB04-E324)
const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex'));

// 예외가 나는지 본다. (2026-11-12 WB04-E324)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// double 의 64비트 비트 문자열(10진)이다. (2026-11-12 WB04-E324)
function doubleBits(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true).toString();
}

// 외부 시계/기록/종료/반납의 실제 호출을 관찰하며 패킷 판단을 대신하지 않는 장치이다(cpppj TransportEffects). now 는 101 부터 하나씩 증가한다. (2026-11-12 WB04-E324)
class TransportEffects {
    // 현재 시각 입력과 각 실제 효과 호출 수, 실제 분배가 기록한 연결 번호와 메시지이다.
    now = 100;
    records = 0;
    terminations = 0;
    closes = 0;
    releases = 0;
    senders = [];
    types = [];

    // 같은 관찰 저장소를 빌리는 실제 분배 효과를 만든다. (2026-11-12 WB04-E324)
    hooks() {
        return {
            now: () => { this.now += 1; return this.now; },
            record: (connection, bytes, length) => {
                assert.equal(length, get(bytes, 2, 2));
                this.records += 1;
                this.senders.push(connection.sender >>> 0);
                this.types.push(bytes[1]);
            },
            terminate: (connection) => { this.terminations += 1; connection.flags |= 0x40; },
            close: (connection) => { this.closes += 1; assert.ok((connection.flags & 0x40) !== 0 && connection.depth === 0); },
            release: () => { this.releases += 1; },
        };
    }
}

// Zone 설치 시험의 현재 전역 묶음이다(cpppj LocalMissionZoneGlobals 의 참조 대신 live 속성). (2026-11-12 WB04-E324)
function zoneGlobals(pendingStamp) {
    return {
        server: false, boss: false, mySubscriber: 1, pendingStamp, activeStamp: 0, activeZoneWord: 0, zoneFlags: 0, earlyCaptureEnd: 0,
        adjustZoneFlags: false, missionWorld: true, zoneFlagOverride: 0, metamap: false, currentCoreData: null, cachedCoreData: new Uint8Array(10),
    };
}

test('LocalMissionTransport: 원본 10.78 66입력의 Zone/로그인 작성·집합 분배·Zone 설치의 바이트·상태·효과 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    const unique = new Set();
    // 원본 관찰 행을 순서대로 읽으며 자체 계산한 기대 패킷/Player 모델을 만들지 않는다.
    for (const row of rows) {
        if (row[0] === 'kind') continue;
        assert.equal(row.length, 6);
        assert.ok(!unique.has(`${row[0]}:${row[1]}`));
        unique.add(`${row[0]}:${row[1]}`);
        const [seed, nameLength, imageLength] = row[1].split(',').map(Number);
        const label = `${row[0]} ${row[1]}`;
        if (row[0] === 'Zone') {
            const packet = makeLocalMissionZone();
            assert.equal(hex(packet.storage), row[2], label);
            assert.equal(hex(packet.bytes()), row[3], label);
            assert.deepEqual([packet.storage.length, packet.bytes().length, packet.storage[14], packet.storage[15]], [512, 86, 1, 0]);
        } else if (row[0] === 'Login') {
            // 원본과 같은 외부 파일 이미지 입력 패턴만 만든다(패킷 헤더/배치 기대값은 계산하지 않음).
            const image = Uint8Array.from({ length: imageLength }, (_, index) => (index * 17 + seed) & 0xff);
            const name = 'N'.repeat(nameLength);
            const packet = makeLocalMissionLogin(1, name, image);
            assert.equal(hex(packet.storage), row[2], label);
            assert.equal(hex(packet.bytes()), row[3], label);
            const view = readLocalMissionLogin(packet.bytes());
            assert.ok(view.subscriber === 1 && view.handle === name, label);
            assert.deepEqual(Array.from(view.fortImage), Array.from(image), label);
        } else if (row[0] === 'Batch' || row[0] === 'BatchClosed') {
            const players = new Uint8Array(1620);
            let now = 104;
            let mutated = false;
            const trace = [];
            const hooks = {
                now: () => { now += 1; trace.push(`Now,${now}`); return now; },
                record: (connection, packet, length) => { trace.push(`Record,${packet[1]},${length},${connection.sender >>> 0}`); },
                terminate: (connection) => { trace.push('Terminate'); connection.flags |= 0x40; },
                close: () => { trace.push('Close'); },
                release: () => { trace.push('Free'); },
            };
            const transport = new LocalMissionTransport(players, true, hooks);
            // 원본 native 의 등록 receive 외부 경계와 같은 현재 sender 변경 입력이다.
            const receive = (sender, connection, packet) => {
                trace.push(`Receive,${sender >>> 0},${packet[1]},${connection.depth}`);
                if (!mutated && imageLength) {
                    connection.sender = 1;
                    mutated = true;
                }
            };
            transport.registerLogin(receive);
            transport.dispatch.version(0x27, 1, nameLength);
            transport.dispatch.version(0x24, 1, 0);
            transport.dispatch.register(0x24, receive);
            const connection = transport.server;
            connection.sender = 0xffffffff;
            connection.flags = row[0] === 'BatchClosed' ? 0x64 : 0x24;
            connection.received = 103;
            connection.heartbeat = 104;
            const result = transport.dispatchFrame(connection, unhex(row[2]));
            const state = row[4].split(',').map(Number);
            assert.equal(state.length, 6);
            assert.deepEqual([result ? 1 : 0, connection.flags, connection.sender >>> 0, connection.received, connection.heartbeat, connection.depth], state, label);
            assert.equal(trace.length === 0 ? '-' : trace.join(';'), row[5], label);
        } else {
            assert.ok(row[0] === 'Install' || row[0] === 'InstallMutate');
            const mutate = row[0] === 'InstallMutate';
            let mutated = false;
            const zone = new Uint8Array(512).fill(0xff);
            zone[0] = 1;
            zone[1] = 0x1a;
            zone[2] = 86;
            zone[3] = 0;
            put(zone, 4, 0x82000004);
            zone[8] = zone[9] = 0;
            put(zone, 10, 0);
            zone[14] = 1;
            zone[15] = 0;
            put(zone, 16, 1);
            put(zone, 20, seed & 1);
            zone.set(Array.from({ length: nameLength }, () => 0x4e), 24);
            zone[24 + nameLength] = 0;
            zone[56] = 0;
            if (mutate) {
                zone[86] = 1;
                put(zone, 87, 42);
                put(zone, 91, 0);
                zone.set(Array.from('Second', (char) => char.charCodeAt(0)), 95);
                zone[101] = zone[127] = 0;
            }
            const players = new Uint8Array(1620);
            put(players, 180 + 0x50, (seed << 12) >>> 0);
            const player = new PlayerWorldState();
            player.battleOwner = player.templateOwner = 8;
            const globals = zoneGlobals(seed + 3);
            let trace = '';
            const hooks = {
                loginTimeout: () => { trace = 'Timeout'; return 30; },
                seconds: () => { trace += ';Time'; return 100.5; },
                storeName: (sub, current) => {
                    trace += `;Name,${sub},${Buffer.from(current, 'latin1').toString('hex')}`;
                    if (mutate && !mutated) {
                        put(zone, 16, 9);
                        zone[14] = 2;
                        mutated = true;
                    }
                },
                serve: (port) => { trace += `;Serve,${port}`; },
            };
            installLocalMissionZone(zone, players, player, globals, hooks, 6799);
            assert.equal(hex(players), row[2], label);
            assert.equal(trace, row[5], label);
            const state = row[4].split(',');
            assert.equal(state.length, 9);
            assert.deepEqual([player.battleOwner, player.templateOwner, player.playerKnown], state.slice(0, 3).map(Number), label);
            assert.deepEqual([globals.server ? 1 : 0, globals.boss ? 1 : 0, globals.activeStamp, globals.activeZoneWord, globals.zoneFlags], state.slice(3, 8).map(Number), label);
            assert.equal(doubleBits(globals.earlyCaptureEnd), state[8], label);
            // 현재 CoreData 는 같은 Player9 저장소의 Player1 +0x6c 10BYTE 보기이다.
            const core = globals.currentCoreData;
            assert.ok(core.buffer === players.buffer && core.byteOffset === players.byteOffset + 180 + 0x6c && core.length === 10, label);
        }
        count += 1;
    }
    assert.equal(count, 66);
});

// cpppj `LocalMissionTransport_ZonePendingLoginAndReply` 이식: 같은 Zone/Player 와 실제 CheckLogin 이 pending -1 을 owner 1 로 옮긴 뒤 진짜 0x27 바이트와 응답 0x24 가 왕복한다. (2026-11-12 WB04-E324)
test('LocalMissionTransport: Zone 설치 → open → 로그인 0x27 → CheckLogin 재배치 → 응답 0x24 왕복', () => {
    const players = new Uint8Array(1620);
    const allies = new Uint32Array(81);
    const hidden = new Uint8Array(9);
    const player = new PlayerWorldState();
    player.battleOwner = player.templateOwner = 8;
    const globals = zoneGlobals(7);
    const counts = { assignments: 0, logins: 0, replies: 0 };
    const effects = new TransportEffects();
    // 서버 모드는 Zone 설치가 켜는 같은 현재 전역을 읽는다.
    const transport = new LocalMissionTransport(players, () => globals.server, effects.hooks());
    const participationState = new PlayerParticipationState();
    participationState.localTransport = false;
    const participation = new RawPlayerParticipation(players, allies, hidden, participationState, {
        assignConnection: (id, owner) => {
            assert.ok(id === -1 && owner === 1);
            transport.assignConnection(id, owner);
            counts.assignments += 1;
        },
    });
    const image = Uint8Array.of(3, 1, 4, 1, 5, 9, 2);
    const login = makeLocalMissionLogin(globals.mySubscriber, 'Local', image);
    transport.registerLogin((sender, connection, bytes) => {
        assert.ok(sender === 0xffffffff && connection.sender === sender && connection.depth === 1);
        const view = readLocalMissionLogin(bytes);
        assert.ok(view.subscriber === globals.mySubscriber && view.handle === 'Local');
        assert.deepEqual(Array.from(view.fortImage), Array.from(image));
        assert.ok(participation.checkLogin(sender | 0, 1));
        assert.ok(connection.sender === 1 && participation.brain(1) === 0);
        counts.logins += 1;
        assert.ok(transport.send(1, Uint8Array.of(1, 0x24, 5, 0, 77)));
        transport.flush();
    });
    transport.dispatch.version(0x24, 1, 0);
    transport.dispatch.register(0x24, (sender, connection, packet) => {
        assert.ok(sender === 9 && connection.depth === 1 && packet[4] === 77);
        counts.replies += 1;
    });
    const hooks = {
        loginTimeout: () => 30,
        seconds: () => 100.5,
        storeName: (sub, name) => { assert.ok(sub === 1 && name === 'zsimpson'); },
        serve: (port) => { transport.open(port); },
    };
    installLocalMissionZone(makeLocalMissionZone().bytes(), players, player, globals, hooks, 6799);
    assert.ok(globals.server && globals.boss && transport.isOpen && transport.owner() === 0);
    assert.ok(player.battleOwner === 1 && player.playerKnown === 1 && get(players, 180 + 0x50) === 8);
    assert.ok(transport.send(9, login.bytes()) && transport.queuedFrames === 0 && counts.logins === 0);
    transport.flush();
    assert.equal(transport.queuedFrames, 1);
    assert.equal(transport.poll(), 2);
    assert.deepEqual([counts.assignments, counts.logins, counts.replies, transport.owner(), transport.queuedFrames], [1, 1, 1, 1, 0]);
    assert.ok(get(players, 180 + 0x50) === 11 && (get(players, 180 + 0x50) & 0x40) === 0 && transport.server.depth === 0 && transport.client.depth === 0);
    assert.deepEqual(effects.senders, [0xffffffff, 9]);
    assert.deepEqual(effects.types, [0x27, 0x24]);
    assert.deepEqual([effects.records, effects.terminations], [2, 0]);
});

// cpppj `LocalMissionTransport_SelectedBoundariesAndDepth` 이식: 미연결 등록/현재 flags 1/예외 중 깊이와 콜백 이후의 현재 sender 를 함께 검사한다. (2026-11-12 WB04-E324)
test('LocalMissionTransport: 로그인 처리기 없는 열기·빈 처리기 거부, 예외 뒤 깊이 0, 재배치 범위, Player flag 1 관문, 짧은 0x27 거부', () => {
    const players = new Uint8Array(1620);
    const effects = new TransportEffects();
    const transport = new LocalMissionTransport(players, true, effects.hooks());
    assert.ok(throws(() => transport.open(6799)) && !transport.isOpen);
    assert.ok(throws(() => transport.registerLogin(null)));
    transport.registerLogin(() => { throw new Error('실제 상위 로그인 경계 실패 입력'); });
    transport.open(6799);
    const login = makeLocalMissionLogin(1, 'A', Uint8Array.of(1));
    assert.equal(transport.send(1, login.bytes()), false);
    assert.equal(transport.send(9, login.bytes()), true);
    transport.flush();
    assert.ok(throws(() => transport.poll()));
    assert.ok(transport.server.depth === 0 && transport.owner() === 0);
    assert.ok(throws(() => transport.assignConnection(1, 1)));
    transport.assignConnection(-1, 1);
    assert.equal(transport.send(1, login.bytes()), false);
    players[180 + 0x50] = 1;
    assert.equal(transport.send(1, login.bytes()), true);
    assert.ok(throws(() => readLocalMissionLogin(login.storage.slice(0, 17))));
});

// cpppj `LocalMissionTransport_ServerOpenAndClientConnectAreSeparate` 이식(기대값 기준선 보정): 서버만 열면 클라이언트/Player 는 미접속이고, connect 뒤 실제 pending 큐로만 요청이 가며
// 서버 끝은 poll 의 accept 에서 생긴다. (2026-11-12 WB04-E324)
test('LocalMissionTransport: 서버 열기와 클라이언트 접속이 분리되고 서버 끝은 poll 의 accept 에서 생긴다', () => {
    const players = new Uint8Array(9 * 0xb4);
    const effects = new TransportEffects();
    const wire = new LocalMissionTransport(players, true, effects.hooks());
    let received = 0;
    wire.registerLogin((sender, connection, packet) => {
        assert.ok(sender === 0xffffffff && connection === wire.server && packet[1] === 0x27);
        received += 1;
    });
    wire.open(6799, false);
    // cpppj 원 기대값은 server.flags 0x24·now 102·received 101·heartbeat 102(열기에서 서버 끝 생성, 옛 설계)이다.
    assert.deepEqual([wire.client.flags, wire.server.flags, wire.owner(), effects.now, wire.registry.cleanup().pending[1]], [0, 0, 0, 100, 0]);
    const packet = makeLocalMissionLogin(1, 'You', Uint8Array.of(1, 2));
    assert.ok(!wire.send(9, packet.bytes()) && wire.queuedFrames === 0);
    wire.connect();
    // cpppj 원 기대값은 received 103·heartbeat 104 이다(앞선 서버 끝 생성의 시계 두 번).
    assert.deepEqual([wire.client.flags, wire.client.received, wire.client.heartbeat], [0x26, 101, 102]);
    assert.ok(wire.owner() === 0 && players[0xb4 + 0x50] === 0);
    assert.ok(wire.send(9, packet.bytes()));
    assert.ok(!received && !wire.queuedFrames);
    wire.flush();
    assert.equal(wire.queuedFrames, 1);
    assert.equal(wire.poll(), 1);
    assert.equal(received, 1);
    // accept(시계 103·104) → 서버 접속 통지(105) → 집합 로그인(106) → 클라이언트 접속 통지(107) 순서이다.
    assert.deepEqual([wire.server.flags, wire.server.received, wire.server.heartbeat, wire.client.received, effects.now], [0x04, 106, 104, 107, 107]);
    assert.ok(wire.server.depth === 0 && wire.owner() === 0 && players[0xb4 + 0x50] === 0);
    assert.equal(wire.registry.node(wire.registry.cleanup().pending[1]), wire.server);
    assert.ok(throws(() => wire.connect()));
});

// cpppj `ConnectionRegistration_LocalMissionSharesPendingOwnerAndClientNine` 이식(기대값 기준선 보정): 공유 등록소의 pending/owner/9번 칸과 정리 순서. (2026-11-12 WB04-E324)
test('LocalMissionTransport: 공유 등록소에서 pending 수락·owner 재배치·9번 칸이 같은 노드를 가리키고 정리가 두 끝을 한 번씩 반납한다', () => {
    const effects = new TransportEffects();
    const players = new Uint8Array(1620);
    const registry = new RawConnectionRegistry();
    const wire = new LocalMissionTransport(players, true, effects.hooks(), registry);
    wire.registerLogin(() => {});
    wire.open(6799, false);
    // cpppj 원 시험은 열기 직후 pending[1] 에 서버 끝이 있다고 본다(옛 설계). 기준선은 클라이언트 접속 뒤 poll 의 accept 에서 등록한다.
    assert.ok(!registry.cleanup().pending[1] && !registry.cleanup().transceivers[9]);
    wire.connect();
    const client = registry.cleanup().transceivers[9];
    assert.ok(client && registry.node(client) === wire.client);
    wire.poll();
    const token = registry.cleanup().pending[1];
    assert.ok(token && registry.node(token) === wire.server && wire.connection(-1) === wire.server);
    wire.assignConnection(-1, 3);
    const table = registry.cleanup();
    assert.ok(table.transceivers[3] === token && !table.pending[1] && !table.pendingCount.value && wire.owner() === 3 && (wire.server.flags & 2));
    registry.endPlayers(1);
    assert.ok(!registry.owns(token) && wire.hasClientConnection && !wire.owner() && registry.cleanup().transceivers[9] === client);
    registry.closeAll();
    assert.ok(!wire.hasClientConnection && !registry.owns(client) && effects.closes === 2 && effects.releases === 2);
});

// cpppj ConnectionActivityTransportTests 의 시계 장치이다: 지정한 호출 번호에서 한 번 예외를 던진다. (2026-11-12 WB04-E324)
class ActivityDevice {
    // 실제 시계 호출 수와 선택된 예외 위치, 현재 서버 모드이다.
    clockCalls = 0;
    stopAt = 0;
    server = true;
    players = new Uint8Array(1620);

    // 현재 실제 전송 생성자가 요구하는 다섯 장치를 만든다(시각은 100+호출 수). (2026-11-12 WB04-E324)
    hooks() {
        return {
            now: () => {
                this.clockCalls += 1;
                if (this.stopAt === this.clockCalls) throw new RangeError('선택된 시계 중단');
                return 100 + this.clockCalls;
            },
            record: () => {},
            terminate: (connection) => { connection.flags |= 0x40; },
            close: () => {},
            release: () => {},
        };
    }
}

// cpppj `ConnectionActivityTransport_ClientObjectPresenceIsSeparateFromFlags` 이식(시계 수 기준선 보정): Client 객체 존재는 flags·서버 모드와 별개이다. (2026-11-12 WB04-E324)
test('LocalMissionTransport: Client9 존재는 flags 변경·서버 모드 변경과 별개이고 즉시 접속 열기는 시계 두 번이다', () => {
    const device = new ActivityDevice();
    const transport = new LocalMissionTransport(device.players, () => device.server, device.hooks());
    assert.ok(!transport.isOpen && !transport.hasClientConnection && device.clockCalls === 0);
    transport.registerLogin(() => {});
    transport.open(6799, false);
    // cpppj 원 기대값은 clockCalls 2(열기에서 서버 끝 생성, 옛 설계)이다. 아래 접속 뒤 값도 2 씩 작다.
    assert.ok(transport.isOpen && !transport.hasClientConnection && transport.client.flags === 0 && device.clockCalls === 0);
    transport.connect();
    assert.ok(transport.hasClientConnection && transport.client.flags === 0x26 && device.clockCalls === 2);
    const { received, heartbeat } = transport.client;
    // flags 2 제거/종료 flag 0x40/전체 0 도 생성 완료한 객체를 반납하지 않는다.
    for (const flags of [0x24, 0x66, 0]) {
        transport.client.flags = flags;
        assert.ok(transport.hasClientConnection && transport.client.flags === flags && device.clockCalls === 2);
        assert.ok(transport.client.received === received && transport.client.heartbeat === heartbeat && transport.owner() === 0 && transport.queuedFrames === 0);
    }
    device.server = false;
    assert.ok(transport.hasClientConnection && device.clockCalls === 2);
    const immediate = new ActivityDevice();
    const connected = new LocalMissionTransport(immediate.players, () => immediate.server, immediate.hooks());
    assert.ok(!connected.hasClientConnection);
    connected.registerLogin(() => {});
    connected.open(6799, true);
    assert.ok(connected.isOpen && connected.hasClientConnection && connected.client.flags === 0x26 && immediate.clockCalls === 2);
});

// cpppj `ConnectionActivityTransport_FailedConnectKeepsPartialEffectsAndUnpublishedPresence` 이식(시계 수 기준선 보정): 실패한 접속의 앞선 쓰기는 남고 존재는 공개되지 않는다. (2026-11-12 WB04-E324)
test('LocalMissionTransport: 두 번째 시계 예외로 실패한 접속은 첫 수신 시각만 남기고 존재를 공개하지 않으며 재시도가 된다', () => {
    const device = new ActivityDevice();
    const transport = new LocalMissionTransport(device.players, () => device.server, device.hooks());
    assert.ok(throws(() => transport.open(6799, false)));
    assert.ok(!transport.isOpen && !transport.hasClientConnection && device.clockCalls === 0);
    transport.registerLogin(() => {});
    transport.open(6799, false);
    device.stopAt = device.clockCalls + 2;
    assert.ok(throws(() => transport.connect()));
    // cpppj 원 기대값은 clockCalls 4·received 103(열기의 서버 끝 생성 시계 두 번 포함, 옛 설계)이다. 아래 값도 2 씩 작다.
    assert.ok(transport.isOpen && !transport.hasClientConnection && transport.client.flags === 0 && device.clockCalls === 2);
    assert.ok(transport.client.received === 101 && transport.client.heartbeat === 0);
    device.stopAt = 0;
    transport.connect();
    assert.ok(transport.hasClientConnection && transport.client.flags === 0x26 && device.clockCalls === 4);
    assert.ok(transport.client.received === 103 && transport.client.heartbeat === 104);
    assert.ok(throws(() => transport.connect()));
    assert.ok(transport.hasClientConnection && transport.client.received === 103 && transport.client.heartbeat === 104 && device.clockCalls === 4);
});

// 열린 전송과 접속·수락까지 마친 장면을 만든다(서버 끝은 pending -1). (2026-11-12 WB04-E324)
function connectedScene(extraHooks = {}) {
    const players = new Uint8Array(1620);
    const effects = new TransportEffects();
    const registry = new RawConnectionRegistry();
    const transport = new LocalMissionTransport(players, true, { ...effects.hooks(), ...extraHooks }, registry);
    transport.registerLogin(() => {});
    transport.open(6799);
    transport.poll();
    return { players, effects, registry, transport };
}

// 기준선 뒤 ② 반영분과 웹 계약: 0x87 bootstrap 등록·종료된 CD 검증 미송수신·sent 기록·현재 Player9 조회. (2026-11-12 WB04-E324)
test('LocalMissionTransport: 0x87 bootstrap 은 비NULL 빈 관찰기·버전 1·길이 0x112 이고 종료된 CD 검증 0x7f/0x80 은 보내지도 분배하지도 않는다', () => {
    const { effects, transport } = connectedScene();
    const bootstrap = transport.dispatch.registration(0x87);
    assert.deepEqual([typeof bootstrap.observer, bootstrap.packetObserver, bootstrap.receive, bootstrap.version, bootstrap.mismatch, bootstrap.length], ['function', null, null, 1, 0, 0x112]);
    assert.equal(bootstrap.observer(1), undefined);
    let observed = 0;
    // 등록해도 관찰기/수신기가 불리지 않아야 한다.
    for (const type of [0x7f, 0x80]) {
        transport.dispatch.observePacket(type, () => { observed += 1; });
        transport.dispatch.register(type, () => { observed += 1; });
        assert.equal(transport.sendMutable(9, Uint8Array.of(1, type, 4, 0)), false);
        assert.equal(transport.send(-1, Uint8Array.of(1, type, 4, 0)), false);
    }
    assert.equal(transport.queuedFrames, 0);
    const before = effects.now;
    // 단일 프레임은 열린 연결이면 true, 닫힌 연결이면 false 를 돌려주고 시계/기록을 쓰지 않는다.
    assert.equal(transport.dispatchFrame(transport.server, Uint8Array.of(1, 0x7f, 4, 0)), true);
    const closed = new PacketConnection({ flags: 0x40 });
    assert.equal(transport.dispatchFrame(closed, Uint8Array.of(1, 0x80, 4, 0)), false);
    // 집합 안에서도 건너뛰고 뒤의 정상 패킷만 분배한다.
    let replies = 0;
    transport.dispatch.version(0x24, 1, 0);
    transport.dispatch.register(0x24, () => { replies += 1; });
    const batch = Uint8Array.of(0xff, 4, 13, 0, 1, 0x7f, 4, 0, 1, 0x24, 5, 0, 9);
    assert.equal(transport.dispatchFrame(transport.server, batch), true);
    assert.deepEqual([observed, replies, effects.now - before, effects.records], [0, 1, 1, 1]);
});

// 송신 기록과 현재 Player9 조회(기준선 뒤 ②)·서버 끝 미buffered 즉시 큐·observer 순서. (2026-11-12 WB04-E324)
test('LocalMissionTransport: 송신 뒤 sent 를 기록하고, 1..8 관문은 현재 players 조회를 쓰며, 재배치 전 서버 송신은 단일 프레임으로 바로 상대 큐에 들어간다', () => {
    const sent = [];
    let current = new Uint8Array(1620);
    const { players, transport } = connectedScene({ sent: (connection, packet, length) => { sent.push([connection, packet[1], length]); }, players: () => current });
    // pending -1 로의 송신은 buffered 가 아니므로 flush 없이 상대(클라이언트) 큐에 단일 프레임으로 들어간다.
    const reply = Uint8Array.of(1, 0x24, 5, 0, 7, 0xee);
    assert.equal(transport.send(-1, reply), true);
    assert.equal(transport.queuedFrames, 1);
    assert.deepEqual(sent, [[transport.server, 0x24, 5]]);
    const order = [];
    transport.dispatch.version(0x24, 1, 0);
    transport.dispatch.observePacket(0x24, (packet, sending) => { order.push(`O${sending}:${packet.length}`); });
    transport.dispatch.register(0x24, (sender, connection, packet) => { order.push(`R${sender}:${connection === transport.client}:${packet.length}`); });
    transport.poll();
    assert.deepEqual(order, ['O0:5', 'R9:true:5']);
    // 재배치 뒤 1번 관문은 생성자의 Player9 사본이 아니라 현재 조회 결과를 읽는다.
    transport.assignConnection(-1, 1);
    players[180 + 0x50] = 1;
    assert.equal(transport.send(1, reply), false);
    current = new Uint8Array(1620);
    current[180 + 0x50] = 1;
    assert.equal(transport.send(1, reply), true);
    assert.equal(sent.length, 2);
    // 원시 작성기는 연결이 선택된 뒤 observer(1) 가 길이를 확정하고 그 현재 WORD 로 복사한다.
    transport.dispatch.observePacket(0x24, (packet, sending) => { order.push(`W${sending}`); packet[2] = 6; packet[3] = 0; });
    const raw = Uint8Array.of(1, 0x24, 0xff, 0xff, 1, 2);
    assert.equal(transport.sendMutable(1, raw), true);
    assert.deepEqual([order.at(-1), sent.at(-1)[2]], ['W1', 6]);
    // 선택되지 않은 송신(빈 칸)은 observer 를 부르지 않는다.
    const calls = order.length;
    assert.equal(transport.sendMutable(5, Uint8Array.of(1, 0x24, 0xff, 0xff)), false);
    assert.equal(order.length, calls);
});

// Track 의 가상 송신·소켓 준비·공통 buffered 장치(기준선 뒤 ②)를 등록소 입구로 확인한다. (2026-11-12 WB04-E324)
test('LocalMissionTransport: 등록소의 가상 송신은 같은 노드로 직접 보내고, 소켓 준비는 반대 끝 상실 뒤 0, 공통 buffered 는 flags 2 와 서버 누적을 켠다', () => {
    const { players, registry, transport } = connectedScene();
    const serverToken = registry.cleanup().pending[1];
    const clientToken = registry.cleanup().transceivers[9];
    assert.deepEqual([registry.socketReadiness(serverToken), registry.socketReadiness(clientToken)], [1, 1]);
    // 등록소의 원문 송신(pending -1)은 같은 서버 노드의 가상 송신을 부르고, 노드는 번호 재선택 없이 상대 큐에 넣는다.
    assert.equal(registry.sendMutable(-1, Uint8Array.of(1, 0x24, 4, 0), players), true);
    assert.equal(transport.queuedFrames, 1);
    // 공통 buffered 입구는 같은 서버 노드를 buffered 로 바꾼다: 다음 송신은 flush 전까지 누적된다.
    registry.setBufferedNative(serverToken);
    assert.ok(transport.server.flags & 2);
    assert.equal(registry.sendMutable(-1, Uint8Array.of(1, 0x24, 4, 0), players), true);
    assert.equal(transport.queuedFrames, 1);
    transport.flush();
    assert.equal(transport.queuedFrames, 1);
    // flush 의 번호 순회는 pending 끝을 대상으로 하지 않는다. 재배치의 buffered 입구는 송신 누적을 비우므로(송신 커서 초기화) 앞선 누적은 나가지 않는다.
    transport.assignConnection(-1, 2);
    transport.flush();
    assert.equal(transport.queuedFrames, 1);
    // 재배치 뒤 owner 2 로의 송신은 누적됐다가 flush 에서 집합 하나로 나간다.
    players[2 * 180 + 0x50] = 1;
    assert.equal(registry.sendMutable(2, Uint8Array.of(1, 0x24, 5, 0, 3), players), true);
    assert.equal(transport.queuedFrames, 1);
    transport.flush();
    assert.equal(transport.queuedFrames, 2);
    // 서버 끝을 실제 정리 부모로 반납하면 클라이언트의 소켓 준비는 0 이 된다(반대 끝 상실).
    registry.endPlayers(1);
    assert.ok(!registry.owns(serverToken));
    assert.equal(registry.socketReadiness(clientToken), 0);
    assert.ok(!transport.connectionOpen(9) && transport.connectionStatus(9) === 0 && transport.connection(9) === transport.client);
});

// 반대 끝 상실 통지(type 2)는 한 번만, 닫힌 끝은 다음 폴링에서 반납한다. (2026-11-12 WB04-E324)
test('LocalMissionTransport: 반대 끝을 잃은 끝은 type 2 통지를 한 번만 분배하고 수신 큐를 읽지 않는다', () => {
    const { effects, registry, transport } = connectedScene();
    let lost = 0;
    transport.dispatch.register(2, (sender, connection) => { assert.equal(connection, transport.client); lost += 1; });
    // 서버 끝(pending -1)을 실제 pending 정리 부모로 반납한다.
    registry.closePending();
    transport.poll();
    transport.poll();
    assert.equal(lost, 1);
    assert.ok(transport.client.flags & 0x10);
    assert.deepEqual([effects.closes, effects.releases], [1, 1]);
});

// 수신 콜백 안의 closeAll 은 깊이 있는 끝을 표시만 하고 분배가 끝난 뒤 반납한다. dispose 는 장치 없이 별칭만 지운다. (2026-11-12 WB04-E324)
test('LocalMissionTransport: 콜백 안 closeAll 은 수신 중인 끝을 분배 뒤 반납하고, dispose 는 close/free 없이 등록만 지운다', () => {
    const players = new Uint8Array(1620);
    const effects = new TransportEffects();
    const registry = new RawConnectionRegistry();
    const transport = new LocalMissionTransport(players, true, effects.hooks(), registry);
    transport.registerLogin((sender, connection) => {
        transport.closeAll();
        // 수신 중인 서버 끝은 닫힘 표시만 되고 아직 반납되지 않는다. 깊이 0 인 클라이언트 끝은 바로 반납된다.
        assert.ok((connection.flags & 0x40) && registry.owns(registry.tokenFor(connection)));
        assert.deepEqual([effects.closes, effects.releases, transport.hasClientConnection], [1, 1, false]);
    });
    transport.open(6799);
    assert.ok(transport.send(9, makeLocalMissionLogin(1, 'X', Uint8Array.of(5)).bytes()));
    transport.flush();
    transport.poll();
    assert.deepEqual([effects.closes, effects.releases, transport.isOpen, transport.queuedFrames], [2, 2, false, 0]);
    assert.ok(!registry.cleanup().pending[1] && !registry.cleanup().transceivers[9]);
    // dispose: 새 열기 뒤의 두 끝을 장치 호출 없이 등록소에서만 지운다.
    const other = new LocalMissionTransport(players, true, effects.hooks(), registry);
    other.registerLogin(() => {});
    other.open(6800);
    other.poll();
    const tokens = [registry.cleanup().transceivers[9], registry.cleanup().pending[1]];
    assert.ok(tokens[0] && tokens[1]);
    other.dispose();
    assert.ok(!registry.owns(tokens[0]) && !registry.owns(tokens[1]) && !registry.cleanup().transceivers[9] && !registry.cleanup().pending[1]);
    assert.deepEqual([effects.closes, effects.releases], [2, 2]);
});

// 인계·소유 조회·Peer·Guide Zone·이름/구성 거부. (2026-11-12 WB04-E324)
test('LocalMissionTransport: rebindHooks·ownsConnection·peer·Guide Zone 과 구성/이름 거부', () => {
    const { effects, transport } = connectedScene();
    assert.ok(transport.ownsConnection(transport.client) && transport.ownsConnection(transport.server) && !transport.ownsConnection(new PacketConnection()));
    assert.ok(throws(() => transport.rebindHooks({ now: () => 1 })));
    let rebound = 0;
    transport.rebindHooks({ ...effects.hooks(), now: () => { rebound += 1; return 500; } });
    transport.dispatch.version(0x24, 1, 0);
    transport.dispatch.register(0x24, () => {});
    assert.equal(transport.dispatchFrame(transport.server, Uint8Array.of(1, 0x24, 5, 0, 0)), true);
    assert.deepEqual([rebound, transport.server.received], [1, 500]);
    // peer 는 같은 send 관문을 쓴다: 재배치 전 owner 2 는 연결이 없어 보내지 않는다.
    transport.peer(2)(Uint8Array.of(1, 0x24, 4, 0));
    assert.equal(transport.queuedFrames, 0);
    transport.peer(-1)(Uint8Array.of(1, 0x24, 4, 0));
    assert.equal(transport.queuedFrames, 1);
    // Guide Zone: 한 슬롯과 선언 길이 4, 나머지 예약은 0xff 이다.
    const guide = makeGuideMissionZone('Guide');
    const bytes = guide.storage;
    assert.deepEqual([bytes.length, guide.bytes().length, bytes[0], bytes[1], bytes[14], bytes[15], get(bytes, 16), get(bytes, 20), get(bytes, 4)], [512, 4, 1, 0x1a, 1, 0, 1, 0, 0x82000004]);
    assert.equal(Buffer.from(bytes.subarray(24, 30)).toString('latin1'), 'Guide\0');
    assert.ok(bytes[56] === 0 && bytes.subarray(57).every((value) => value === 0xff) && bytes.subarray(30, 56).every((value) => value === 0xff));
    // 이름은 BYTE 문자열만, 구성은 1620BYTE Player9·다섯 장치·등록소 형식만 받는다.
    assert.ok(throws(() => makeLocalMissionLogin(1, '한글', new Uint8Array(1))));
    assert.ok(throws(() => new LocalMissionTransport(new Uint8Array(10), true, effects.hooks())));
    assert.ok(throws(() => new LocalMissionTransport(new Uint8Array(1620), true, { ...effects.hooks(), release: null })));
    assert.ok(throws(() => new LocalMissionTransport(new Uint8Array(1620), true, effects.hooks(), {})));
    assert.ok(throws(() => new LocalMissionTransport(new Uint8Array(1620), 'yes', effects.hooks())));
});

// Zone 설치의 metamap CoreData·플래그 보충·진단 경로. (2026-11-12 WB04-E324)
test('LocalMissionTransport: Zone 설치는 metamap 이면 캐시 CoreData 를, 보충 관문이면 Zone 플래그 하위 6비트를 바꾸고, 진단 경로는 장치를 요구한다', () => {
    const zone = makeLocalMissionZone().storage;
    const hooks = (trace) => ({ loginTimeout: () => -200, seconds: () => 0.25, storeName: () => {}, serve: () => {}, assertion: (text) => { trace.push(text); } });
    const players = new Uint8Array(1620);
    const globals = zoneGlobals(1);
    Object.assign(globals, { metamap: true, adjustZoneFlags: () => 1, missionWorld: false, zoneFlagOverride: 0xea });
    // Zone+10 DWORD 전체와 슬롯 +41 문자열(Player +0x30 으로 복사)을 비어 있지 않게 둔다.
    put(zone, 10, 0x12345678);
    zone.set([0x4f, 0x72, 0x67, 0], 15 + 41);
    const trace = [];
    installLocalMissionZone(zone, players, new PlayerWorldState(), globals, hooks(trace), 1);
    assert.equal(globals.currentCoreData, globals.cachedCoreData);
    assert.equal(globals.activeZoneWord, 0x12345678);
    assert.deepEqual(Array.from(players.subarray(180 + 0x30, 180 + 0x34)), [0x4f, 0x72, 0x67, 0]);
    // Zone+4 0x82000004 의 하위 6비트를 보충값 0xea 의 하위 6비트(0x2a)로 바꾼다. 시각은 (timeout+120)|0 + seconds 이다.
    assert.deepEqual([globals.zoneFlags, globals.earlyCaptureEnd, trace.length], [0x8200002a, -80 + 0.25, 0]);
    // 미션 저장 맵이면 보충하지 않는다. 이미 예약 기한이 있으면 진단 뒤 계속한다.
    Object.assign(globals, { missionWorld: true, metamap: false });
    installLocalMissionZone(zone, players, new PlayerWorldState(), globals, hooks(trace), 1);
    assert.deepEqual([globals.zoneFlags, trace], [0x82000004, ['!earlyCaptureTimer.isRunning()']]);
    // client 0 개는 진단 뒤 설치 없이 serve 까지 간다. 진단 장치가 없으면 그 위치에서 거부한다.
    const empty = Uint8Array.from(zone);
    empty[14] = 0;
    const served = [];
    installLocalMissionZone(empty, new Uint8Array(1620), new PlayerWorldState(), zoneGlobals(1), { ...hooks(served), serve: (port) => { served.push(port); } }, 9);
    assert.deepEqual(served, ['numClients > 0 && numClients < 9', 9]);
    assert.ok(throws(() => installLocalMissionZone(empty, new Uint8Array(1620), new PlayerWorldState(), zoneGlobals(1), { ...hooks([]), assertion: undefined }, 9)));
    // metamap 캐시가 10BYTE 가 아니면 거부한다.
    const bad = zoneGlobals(1);
    Object.assign(bad, { metamap: true, cachedCoreData: new Uint8Array(9) });
    assert.ok(throws(() => installLocalMissionZone(zone, new Uint8Array(1620), new PlayerWorldState(), bad, hooks([]), 1)));
});

// 변형 확인 보강: 이름 NUL 절단·strcpy 물리 경계·nameLen/크기 DWORD 범위. (2026-11-12 WB04-E324)
test('LocalMissionTransport: 이름은 첫 NUL 에서 자르고, strcpy 의 NUL 이 저장소를 넘거나 nameLen·이미지 크기 DWORD 가 범위를 넘으면 거부한다', () => {
    const cut = makeLocalMissionLogin(1, 'AB\0CD', Uint8Array.of(7));
    assert.equal(cut.storage[12], 3);
    assert.equal(readLocalMissionLogin(cut.bytes()).handle, 'AB');
    // Guide Zone 은 +24 부터 512 끝까지: 487 글자는 NUL 이 마지막 칸에 들어가고 488 글자는 NUL 자리가 없다.
    assert.equal(makeGuideMissionZone('N'.repeat(487)).storage[511], 0);
    assert.ok(throws(() => makeGuideMissionZone('N'.repeat(488))));
    // 255 글자 이름은 nameLen BYTE 가 0 으로 감기지만 이름 strcpy 의 NUL 이 이미지 205 예약 끝을 넘으므로 먼저 거부한다.
    assert.ok(throws(() => makeLocalMissionLogin(1, 'N'.repeat(255), new Uint8Array(205))));
    const base = makeLocalMissionLogin(1, 'AB', Uint8Array.of(9, 8)).bytes();
    // nameLen 이 NUL 을 포함하지 못하면(2) 거부한다.
    const shortName = Uint8Array.from(base);
    shortName[12] = 2;
    assert.ok(throws(() => readLocalMissionLogin(shortName)));
    // 이미지 크기는 DWORD 전체로 읽는다: 상위 WORD 가 있으면 물리 범위를 넘는다.
    const wide = Uint8Array.from(base);
    wide[15] = 1;
    assert.ok(throws(() => readLocalMissionLogin(wide)));
});

// 변형 확인 보강: 열기 주소·sender, 로그인 등록 버전, 접속 관문(9번 칸 선점·flags 비영). (2026-11-12 WB04-E324)
test('LocalMissionTransport: 열기는 클라이언트 sender 9·포트 주소를 쓰고, 9번 칸이 이미 있거나 flags 가 남아 있으면 시계 전에 접속을 거부한다', () => {
    const effects = new TransportEffects();
    const registry = new RawConnectionRegistry();
    const transport = new LocalMissionTransport(new Uint8Array(1620), true, effects.hooks(), registry);
    assert.equal(transport.dispatch.registration(0x27).version, 0);
    transport.registerLogin(() => {});
    assert.deepEqual([transport.dispatch.registration(0x27).version, transport.dispatch.registration(0x27).mismatch], [1, 0]);
    transport.open(6799, false);
    const address = new Uint8Array(20);
    address.set([20, 0, 0x1a, 0x8f]);
    address.set([0x0d, 0x01, 0, 0], 16);
    assert.deepEqual([transport.client.sender, Array.from(transport.client.address), Array.from(transport.server.address)], [9, Array.from(address), Array.from(address)]);
    assert.notEqual(transport.server.address, transport.client.address);
    // 남은 flags 가 있으면 시계를 부르기 전에 거부한다.
    transport.client.flags = 2;
    assert.ok(throws(() => transport.connect()));
    assert.deepEqual([transport.client.flags, effects.now, transport.hasClientConnection], [2, 100, false]);
    transport.client.flags = 0;
    // 다른 backend 의 노드가 9번 칸에 있으면 시계를 부르기 전에 거부한다.
    const foreign = new PacketConnection();
    const token = registry.track(foreign, () => {}, () => {});
    registry.register(9, token);
    assert.ok(throws(() => transport.connect()));
    assert.deepEqual([effects.now, transport.hasClientConnection], [100, false]);
});

// 변형 확인 보강: 두 프레임 again 반복·가상 송신의 Player 관문 우회와 observer(1)·observer 안 재배치·클라이언트 flags 2 관문·buffered 상한. (2026-11-12 WB04-E324)
test('LocalMissionTransport: 한 ID 의 큐는 again 동안 모두 읽고, 가상 송신은 현재 players 관문을 다시 보지 않으며, observer 안의 재배치나 buffered 상한은 송신을 막는다', () => {
    let current = new Uint8Array(1620);
    const { registry, transport } = connectedScene({ players: () => current });
    let replies = 0;
    transport.dispatch.version(0x24, 1, 0);
    transport.dispatch.register(0x24, () => { replies += 1; });
    assert.ok(transport.send(-1, Uint8Array.of(1, 0x24, 5, 0, 1)) && transport.send(-1, Uint8Array.of(1, 0x24, 5, 0, 2)));
    assert.equal(transport.queuedFrames, 2);
    assert.equal(transport.poll(), 2);
    assert.equal(replies, 2);
    // observer(1) 안에서 같은 서버 끝이 재배치되면 이전 선택(pending -1)으로 보내지 않는다.
    const sending = [];
    transport.dispatch.observePacket(0x24, (packet, direction) => {
        sending.push(direction);
        if (direction === 1 && transport.owner() === 0) transport.assignConnection(-1, 2);
    });
    assert.equal(transport.sendMutable(-1, Uint8Array.of(1, 0x24, 5, 0, 0)), false);
    assert.deepEqual([transport.queuedFrames, transport.owner(), sending], [0, 2, [1]]);
    // 등록소의 원문 송신은 자기 관문(인자 Player9)을 거친 뒤 가상 송신을 부르며, 가상 송신은 현재 players 조회(비어 있음)를 다시 보지 않는다.
    const gate = new Uint8Array(1620);
    gate[2 * 180 + 0x50] = 1;
    assert.equal(registry.sendMutable(2, Uint8Array.of(1, 0x24, 5, 0, 3), gate), true);
    assert.deepEqual(sending, [1, 1]);
    transport.flush();
    assert.equal(transport.queuedFrames, 1);
    // 같은 번호 송신은 현재 players 조회로 막힌다.
    assert.equal(transport.send(2, Uint8Array.of(1, 0x24, 5, 0, 4)), false);
    // buffered 누적은 65531 바이트까지이고 넘으면 거부한다.
    current = gate;
    const big = new Uint8Array(65531);
    big.set([1, 0x24]);
    put(big, 2, 65531, 2);
    assert.equal(transport.send(2, big), true);
    assert.ok(throws(() => transport.send(2, Uint8Array.of(1, 0x24, 4, 0))));
    // 닫힘 표시가 있는 끝은 flush 하지 않는다.
    transport.server.flags |= 0x40;
    transport.flush();
    assert.equal(transport.queuedFrames, 1);
    // 클라이언트 끝은 flags 2 가 없으면 보내지 않는다.
    transport.client.flags &= ~2;
    assert.equal(transport.send(9, Uint8Array.of(1, 0x24, 5, 0, 5)), false);
});

// 변형 확인 보강: 클라이언트 끝 상실의 대칭 통지·큐 비움·마지막 수신 version 사용·접속 통지 type 1. (2026-11-12 WB04-E324)
test('LocalMissionTransport: 끝의 소멸은 두 큐를 비우고 반대 끝에 마지막 수신 version 으로 type 2 를 한 번 통지하며, 첫 폴링은 두 끝에 type 1 을 한 번씩 통지한다', () => {
    const effects = new TransportEffects();
    const registry = new RawConnectionRegistry();
    const transport = new LocalMissionTransport(new Uint8Array(1620), true, effects.hooks(), registry);
    transport.registerLogin(() => {});
    const connected = [];
    transport.dispatch.register(1, (sender, connection) => { connected.push(connection === transport.server ? 'S' : 'C'); });
    transport.open(6799);
    transport.poll();
    transport.poll();
    assert.deepEqual(connected, ['S', 'C']);
    // 서버가 version 5 단일 프레임을 클라이언트로 보내 클라이언트 쪽 마지막 수신 version 을 5 로 만든다.
    transport.dispatch.version(0x24, 1, 0);
    transport.dispatch.register(0x24, () => {});
    assert.ok(transport.send(-1, Uint8Array.of(5, 0x24, 5, 0, 0)));
    transport.poll();
    // type 2 는 version 5 와 일치할 때만 받도록(다르면 무시) 등록한다.
    const lost = [];
    transport.dispatch.version(2, 5, 3);
    transport.dispatch.register(2, (sender, connection) => { lost.push(connection === transport.server ? 'S' : 'C'); });
    // 아직 받지 않은 프레임이 있는 상태에서 서버 끝을 실제 정리 부모로 없애면 두 큐가 모두 비워진다.
    assert.ok(transport.send(-1, Uint8Array.of(5, 0x24, 5, 0, 1)));
    assert.equal(transport.queuedFrames, 1);
    registry.closePending();
    assert.equal(transport.queuedFrames, 0);
    transport.poll();
    transport.poll();
    assert.deepEqual(lost, ['C']);
    // 대칭: 새 전송에서 클라이언트 끝을 없애면 서버 끝이 상실을 통지받는다(마지막 수신 version 이 없으면 0xff 라 version 5 등록과 달라 무시된다).
    const other = new LocalMissionTransport(new Uint8Array(1620), true, effects.hooks(), new RawConnectionRegistry());
    other.registerLogin(() => {});
    other.open(6800);
    other.poll();
    const notices = [];
    other.dispatch.register(2, (sender, connection) => { notices.push(connection === other.server ? 'S' : 'C'); });
    other.registry.disconnect(9);
    assert.ok(!other.connectionOpen(-1) && other.registry.socketReadiness(other.registry.cleanup().pending[1]) === 0);
    other.poll();
    other.poll();
    assert.deepEqual(notices, ['S']);
});

// 변형 확인 보강: 수신 중인 끝의 retire 보류·두 번째 pending 수락·외부 노드가 있는 칸의 owner 조회. (2026-11-12 WB04-E324)
test('LocalMissionTransport: 콜백 안 receiveOne 은 수신 중인 닫힌 끝을 반납하지 않고, pending -2 서버도 읽으며, owner() 는 현재 칸의 토큰이 자기 끝일 때만 번호를 준다', () => {
    const effects = new TransportEffects();
    const registry = new RawConnectionRegistry();
    // 다른 backend 의 노드가 pending -1 을 먼저 차지한다.
    const foreign = new PacketConnection();
    const foreignToken = registry.track(foreign, () => {}, () => {});
    registry.accept(foreignToken);
    const transport = new LocalMissionTransport(new Uint8Array(1620), true, effects.hooks(), registry);
    let logins = 0;
    transport.registerLogin((sender, connection) => {
        logins += 1;
        connection.flags |= 0x40;
        transport.receiveOne(-2);
        assert.ok(registry.owns(registry.tokenFor(connection)));
        assert.equal(effects.closes, 0);
    });
    transport.open(6799);
    transport.poll();
    assert.equal(transport.connection(-2), transport.server);
    assert.ok(transport.send(9, makeLocalMissionLogin(1, 'P', Uint8Array.of(1)).bytes()));
    transport.flush();
    transport.poll();
    // 분배가 끝난 뒤 깊이 0 에서 닫힌 서버 끝을 반납한다.
    assert.deepEqual([logins, effects.closes, effects.releases, registry.cleanup().pending[2]], [1, 1, 1, 0]);
    // owner(): 다른 노드가 4번 칸에 있을 때 서버 sender 가 4 여도 자기 끝이 아니므로 0 이다.
    const scene = connectedScene();
    scene.transport.assignConnection(-1, 3);
    const other = scene.registry.track(new PacketConnection(), () => {}, () => {});
    scene.registry.register(4, other);
    scene.transport.server.sender = 4;
    assert.equal(scene.transport.owner(), 0);
    scene.transport.server.sender = 3;
    assert.equal(scene.transport.owner(), 3);
});

// 변형 확인 보강: 집합 프레임의 길이 오류 세 갈래·type 6 heartbeat·기록 → observer(0) → receive 순서·집합 프레임에는 단일 observer 를 부르지 않음. (2026-11-12 WB04-E324)
test('LocalMissionTransport: 집합 프레임의 전체/꼬리/0 길이 오류는 예외 없이 terminate 뒤 false 이고, 내부 type 6 은 heartbeat 만, 기록 → observer(0) → receive 순서이다', () => {
    const order = [];
    const { effects, players, transport } = connectedScene({ record: (connection, bytes) => { order.push(`R${bytes[1]}`); } });
    transport.dispatch.version(0x24, 1, 0);
    transport.dispatch.register(0x24, () => { order.push('V'); });
    transport.dispatch.observePacket(0x24, (packet, direction) => { order.push(`O${direction}`); });
    const server = transport.server;
    // 서버 끝을 열린 상태(0x24)로 되돌린 뒤 프레임 하나를 분배하고 [반환, 이번 terminate 수] 를 돌려준다.
    const run = (frame) => {
        server.flags = 0x24;
        const before = effects.terminations;
        const result = transport.dispatchFrame(server, Uint8Array.from(frame));
        return [result, effects.terminations - before];
    };
    // 선언 전체가 실제보다 길다.
    assert.deepEqual(run([0xff, 4, 9, 0, 1, 0x24, 5, 0]), [false, 1]);
    // 첫 패킷 뒤 꼬리 3바이트는 헤더가 되지 못한다(첫 패킷은 이미 분배됨).
    order.length = 0;
    assert.deepEqual(run([0xff, 4, 12, 0, 1, 0x24, 5, 0, 9, 1, 0x24, 0]), [false, 1]);
    assert.deepEqual(order, ['R36', 'O0', 'V']);
    // 내부 길이 0 은 종료한다.
    assert.deepEqual(run([0xff, 4, 8, 0, 1, 0x24, 0, 0]), [false, 1]);
    // type 6 은 heartbeat 만 갱신한다.
    server.received = 1;
    server.heartbeat = 1;
    assert.deepEqual(run([0xff, 4, 8, 0, 1, 6, 4, 0]), [true, 0]);
    assert.ok(server.received === 1 && server.heartbeat === effects.now);
    // 집합 프레임 자체(type 4)의 관찰기는 수신 끝에서 부르지 않는다.
    let batchObserved = 0;
    transport.dispatch.observePacket(4, () => { batchObserved += 1; });
    transport.assignConnection(-1, 2);
    players[2 * 180 + 0x50] = 1;
    assert.ok(transport.send(2, Uint8Array.of(1, 0x24, 5, 0, 0)));
    transport.flush();
    transport.poll();
    assert.equal(batchObserved, 0);
});
