// 전투 방송(cpppj `o/RawBattleBroadcast`, 10.78 004e2e30·004e2e80)과 훨리기그 명령(0x86)·섬 정리(0x35) 패킷 작성기의 웹 대응 `core/raw-battle-broadcast.js` 를 검사한다(4-C, WB04-E311).
//  - 원본 독립 관측 `battle-bcast-x86.tsv`(10.78 originals 678행: 전체/제외 송신 × 접속 비트 × 연결 표 × 로컬 × 제외 번호 × 첫 송신 중 상태 변경)의 후보 송신 순서를 재생한다.
//  - `flyer-command-x86.tsv` 30행(훨리기그 명령 20BYTE 전체와 20BYTE 밖 canary)과 `drop-island-packet-x86.tsv` 336행(섬 정리 20BYTE 전체와 canary)을 두 정밀도 열과 함께 대조한다.
//  - 손 시험(cpppj `BattleBroadcastTests.cpp` 이식): NULL 연결·비접속 생략, 제외 송신의 server 진단(복귀하면 계속), mutable 송신의 장치 연결, 짧은 출력의 쓰기 전 거부, 원격 연결에만 전달.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E311)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { RawBattleBroadcast, writeDropIslandPacket, writeFlyerCommandPacket } from '../www/core/raw-battle-broadcast.js';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';

// 원본 관측 파일들이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-11 WB04-E311)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const BROADCAST_ROWS = readRows(path.join(FIXTURES, 'battle-bcast-x86.tsv'));
const FLYER_ROWS = readRows(path.join(FIXTURES, 'flyer-command-x86.tsv'));
const DROP_ROWS = readRows(path.join(FIXTURES, 'drop-island-packet-x86.tsv'));

// 10.78 Player9 기록 폭(180BYTE)과 접속 BYTE 위치(+0x50)이다. (2026-11-11 WB04-E311)
const STRIDE = 0xb4;
const CONNECTED = 0x50;

// 바이트열을 소문자 16진 문자열로 바꾼다. (2026-11-11 WB04-E311)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 접속 BYTE 만 가진 Player9 기록, 길이 9 연결 표, 로컬 owner 와 server 상자를 만든다. (2026-11-11 WB04-E311)
function makeScene() {
    return { players: new Uint8Array(9 * STRIDE), peers: new Array(9).fill(null), local: { value: 0 }, server: { value: true } };
}

// 원본 관측 678행: 열은 edition/mode(A=전체,E=제외)/접속 mask/연결 mask/local/제외 번호/첫 송신 중 변경/관측 순서이다. (2026-11-11 WB04-E311)
test('전투 방송: 원본 관측 678행의 후보 송신 순서', { skip: BROADCAST_ROWS === null }, () => {
    const scene = makeScene();
    const broadcast = new RawBattleBroadcast(scene.players, scene.peers, scene.local, scene.server);
    const packet = Uint8Array.from({ length: 32 }, (_, index) => index);
    let count = 0;
    // 10.78(originals) 행만 재생한다. CD 판본 행(originalCD)은 웹 범위 밖이다.
    for (const row of BROADCAST_ROWS) {
        if (row[0] !== 'originals') continue;
        const active = Number(row[2]);
        const connected = Number(row[3]);
        scene.local.value = Number(row[4]);
        const action = Number(row[6]);
        const sent = [];
        // Player 0 과 bit 0 외 비트는 원본과 같은 입력이다.
        for (let owner = 0; owner < 9; owner += 1) {
            scene.players[owner * STRIDE + CONNECTED] = 0xfe | ((active >> owner) & 1);
            scene.peers[owner] = null;
            if (!(connected & (1 << owner))) continue;
            scene.peers[owner] = (bytes) => {
                // 명시 packet 은 보내는 동안 같은 객체여야 하고 내용이 변하면 안 된다.
                assert.equal(bytes, packet);
                sent.push(owner);
                if (sent.length !== 1) return;
                // 첫 송신 안에서 로컬 번호·연결 표·접속 비트를 바꾸는 외부 상태 변경이다.
                if (action === 1) scene.local.value = 8;
                if (action === 2 && owner < 8) scene.peers[owner + 1] = null;
                if (action === 3 && owner < 8) scene.players[(owner + 1) * STRIDE + CONNECTED] = 0xfe;
            };
        }
        if (row[1] === 'A') broadcast.send(packet);
        else broadcast.sendExcept(Number(row[5]), packet);
        assert.equal(sent.length ? sent.join(',') : '-', row[7], `행 ${count}`);
        count += 1;
    }
    assert.equal(count, 678);
});

// 훨리기그 명령 30행: 입력은 sid,command,seed, 출력은 두 정밀도의 20BYTE 이다. 20BYTE 밖과 미정 필드(+8,+16)는 caller 의 기존 값(seed)을 유지한다. (2026-11-11 WB04-E311)
test('전투 방송: 훨리기그 명령 패킷 30행', { skip: FLYER_ROWS === null }, () => {
    let count = 0;
    // 모든 입력 행을 그대로 재생한다.
    for (const row of FLYER_ROWS) {
        const [sid, command, seed] = row[0].split(',').map(Number);
        const packet = new Uint8Array(24).fill(seed);
        writeFlyerCommandPacket(packet, sid, command);
        assert.equal(row[1], row[2]);
        assert.equal(hex(packet.subarray(0, 20)), row[1], `행 ${count}`);
        // 원래 20BYTE 밖은 쓰지 않는다.
        for (let index = 20; index < packet.length; index += 1) assert.equal(packet[index], seed);
        count += 1;
    }
    assert.equal(count, 30);
});

// 섬 정리 336행: 입력은 xbits,ybits,flags,owner,seed, 좌표 DWORD 는 원문 관측값(wireX/wireY)을 그대로 쓴다. (2026-11-11 WB04-E311)
test('전투 방송: 섬 정리 패킷 336행', { skip: DROP_ROWS === null }, () => {
    let count = 0;
    // 모든 입력 행을 그대로 재생한다.
    for (const row of DROP_ROWS) {
        const input = row[0].split(',').map(Number);
        const packet = new Uint8Array(24).fill(input[4]);
        writeDropIslandPacket(packet, Number(row[1]), Number(row[2]), input[2], input[3]);
        assert.equal(row[3], row[4]);
        assert.equal(hex(packet.subarray(0, 20)), row[3], `행 ${count}`);
        // 원본 20BYTE 밖은 caller 의 기존 값 그대로 남는다.
        for (let index = 20; index < packet.length; index += 1) assert.equal(packet[index], input[4]);
        count += 1;
    }
    assert.equal(count, 336);
});

// 접속 플래그가 모두 켜져 있어도 NULL 연결에는 송신하지 않고, 제외 송신은 server 가 아니면 진단 장치를 요구하며 복귀하면 서버 값을 재검사하지 않고 계속한다. (2026-11-11 WB04-E311)
test('전투 방송: NULL 연결·비접속 생략과 제외 송신의 server 진단', () => {
    const scene = makeScene();
    scene.local.value = 1;
    for (let owner = 0; owner < 9; owner += 1) scene.players[owner * STRIDE + CONNECTED] = 0xff;
    const broadcast = new RawBattleBroadcast(scene.players, scene.peers, scene.local, scene.server);
    const packet = Uint8Array.of(1, 2, 3, 4);
    // 모든 연결이 NULL 이면 어떤 송신 장치도 부르지 않는다(장치가 없어도 오류가 아니다).
    assert.doesNotThrow(() => { broadcast.send(packet); broadcast.sendExcept(2, packet); broadcast.sendMutable(packet); });
    // server 가 아니면 진단 장치가 없을 때 거부한다.
    scene.server.value = false;
    assert.throws(() => broadcast.sendExcept(2, packet), /server 진단 장치 미연결/);
    // 진단 장치가 있으면 부르고, 복귀하면 이어서 순회한다(재검사 없음).
    const sent = [];
    scene.peers[3] = () => { sent.push(3); };
    scene.peers[4] = () => { sent.push(4); };
    let diagnostics = 0;
    const checked = new RawBattleBroadcast(scene.players, scene.peers, scene.local, scene.server, () => { diagnostics += 1; });
    checked.sendExcept(4, packet);
    assert.deepEqual([diagnostics, sent], [1, [3]]);
    // 제외 번호는 int32 비트열로 비교한다: 2^32+3 은 3 번 owner 를 제외한다.
    scene.local.value = 0;
    sent.length = 0;
    scene.server.value = true;
    scene.peers[3] = () => { sent.push(3); };
    scene.peers[4] = () => { sent.push(4); };
    scene.peers[5] = null;
    scene.players[3 * STRIDE + CONNECTED] = 1;
    scene.players[4 * STRIDE + CONNECTED] = 1;
    checked.sendExcept(2 ** 32 + 3, packet);
    assert.deepEqual(sent, [4]);
    // 제외 송신은 시작 때 로컬 owner 를 보관한다: 첫 송신이 로컬 번호를 바꿔도 이번 순회는 처음 값으로 판정한다.
    scene.server.value = true;
    scene.local.value = 4;
    scene.peers[3] = () => { sent.push(3); scene.local.value = 3; };
    sent.length = 0;
    checked.sendExcept(0, packet);
    assert.deepEqual(sent, [3]);
    // 반대로 전체 송신은 후보마다 로컬을 다시 읽는다.
    scene.local.value = 7;
    scene.peers[3] = () => { sent.push(3); scene.local.value = 4; };
    sent.length = 0;
    checked.send(packet);
    assert.deepEqual(sent, [3]);
});

// mutable 송신: 선택 순서(접속·NULL 연결·현재 로컬) 뒤 target 마다 같은 packet 으로 장치를 부르고, 장치가 없으면 대상이 있을 때 도달한 위치에서 오류이다. (2026-11-11 WB04-E311)
test('전투 방송: mutable 송신의 대상 순서와 장치 연결', () => {
    const scene = makeScene();
    scene.local.value = 2;
    for (const owner of [1, 2, 3, 5]) scene.players[owner * STRIDE + CONNECTED] = 1;
    scene.peers[1] = () => {};
    scene.peers[2] = () => {};
    scene.peers[3] = () => {};
    // 4 번은 연결 객체가 있어도 접속 비트가 꺼져 있어 건너뛰고, 5 번은 접속했지만 NULL 연결이라 건너뛴다.
    scene.peers[4] = () => {};
    const broadcast = new RawBattleBroadcast(scene.players, scene.peers, scene.local, scene.server);
    const packet = Uint8Array.of(9, 9, 9);
    assert.throws(() => broadcast.sendMutable(packet), /mutable 가상 송신 미연결/);
    const calls = [];
    broadcast.bindMutableSend((owner, bytes) => {
        calls.push(owner);
        // 앞 대상이 바꾼 packet 은 다음 대상 송신에도 보존된다.
        bytes[0] += 1;
        assert.equal(bytes, packet);
    });
    broadcast.sendMutable(packet);
    assert.deepEqual([calls, packet[0]], [[1, 3], 11]);
    // 장치를 비우면 다시 미연결이고, 함수가 아닌 값은 거부한다.
    broadcast.bindMutableSend(null);
    assert.throws(() => broadcast.sendMutable(packet), /미연결/);
    assert.throws(() => broadcast.bindMutableSend(5), TypeError);
});

// 구성: Player9 크기·연결 표 길이·상자 모양·진단 장치 종류를 확인한다. (2026-11-11 WB04-E311)
test('전투 방송: 구성의 크기와 모양 검사', () => {
    const scene = makeScene();
    assert.throws(() => new RawBattleBroadcast(new Uint8Array(9 * 0xac), scene.peers, scene.local, scene.server), RangeError);
    assert.throws(() => new RawBattleBroadcast(new Uint8Array(9 * STRIDE + 1), scene.peers, scene.local, scene.server), RangeError);
    assert.throws(() => new RawBattleBroadcast([], scene.peers, scene.local, scene.server), RangeError);
    // 같은 원소 수라도 BYTE 배열이 아니면 거부한다.
    assert.throws(() => new RawBattleBroadcast(new Uint16Array(9 * STRIDE), scene.peers, scene.local, scene.server), RangeError);
    assert.throws(() => new RawBattleBroadcast(scene.players, new Array(8).fill(null), scene.local, scene.server), TypeError);
    assert.throws(() => new RawBattleBroadcast(scene.players, new Array(10).fill(null), scene.local, scene.server), TypeError);
    assert.throws(() => new RawBattleBroadcast(scene.players, null, scene.local, scene.server), TypeError);
    assert.throws(() => new RawBattleBroadcast(scene.players, scene.peers, 3, scene.server), TypeError);
    assert.throws(() => new RawBattleBroadcast(scene.players, scene.peers, scene.local, true), TypeError);
    assert.throws(() => new RawBattleBroadcast(scene.players, scene.peers, scene.local, scene.server, 5), TypeError);
    assert.doesNotThrow(() => new RawBattleBroadcast(scene.players, scene.peers, scene.local, scene.server, null));
});

// 너무 짧은 출력은 쓰기 전에 거부하고(부분 헤더 없음), 실제 방송은 로컬이 아닌 원격 연결에만 전달한다. (2026-11-11 WB04-E311)
test('전투 방송: 패킷 작성기의 용량 거부와 원격 연결 전달', () => {
    for (const write of [(packet) => writeFlyerCommandPacket(packet, 50, 2), (packet) => writeDropIslandPacket(packet, 0, 0, 0, 0)]) {
        const shortPacket = new Uint8Array(19).fill(0xa5);
        assert.throws(() => write(shortPacket), RangeError);
        assert.ok(shortPacket.every((byte) => byte === 0xa5));
    }
    const scene = makeScene();
    scene.local.value = 1;
    scene.players[STRIDE + CONNECTED] = 1;
    scene.players[2 * STRIDE + CONNECTED] = 1;
    let own = 0;
    const remote = [];
    scene.peers[1] = () => { own += 1; };
    scene.peers[2] = (bytes) => { remote.push(Array.from(bytes)); };
    const broadcast = new RawBattleBroadcast(scene.players, scene.peers, scene.local, scene.server);
    const flyer = new Uint8Array(20);
    writeFlyerCommandPacket(flyer, 50, 3);
    broadcast.send(flyer);
    const drop = new Uint8Array(20);
    writeDropIslandPacket(drop, 0x87654321, 0xffff0000, 0x1050, 8);
    broadcast.send(drop);
    assert.equal(own, 0);
    assert.equal(remote.length, 2);
    assert.deepEqual([remote[0][1], remote[0][4], remote[0][12], remote[0].length], [0x86, 50, 3, 20]);
    assert.equal(hex(remote[1]), '01351400' + '21436587' + '0000ffff' + '50100000' + '08000000');
    // 부호 있는 값과 큰 값은 unsigned 32비트로 쓴다.
    const wide = new Uint8Array(20);
    writeDropIslandPacket(wide, -1, 0x100000001, 2 ** 32 + 5, -2);
    assert.equal(hex(wide), '01351400' + 'ffffffff' + '01000000' + '05000000' + 'feffffff');
});
