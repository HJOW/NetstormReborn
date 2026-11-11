// PATCH 로그인 004a7800 의 Player/일반 전투·미션/패킷/Ready 부모 순서(cpppj `o/RawMissionLogin`)의 웹 대응 `core/raw-mission-login.js` 를 검사한다(4-C, WB04-E319).
//  - 원본 독립 관측 `mission-login-x86.tsv` 48행(서버/알려진 Player/현재 owner/구독자/flags/다른 Player/호출 도중 변경/CRT clock)의 전체 Player 1620바이트·현재 전역 9개·모든 packet 과 외부 효과 순서를 재생한다.
//    Ready boss 경로와 8종 패킷 수신은 `raw-mission-client-packets.test.mjs` 의 157행이 같은 로그인 몸체로 이미 재생한다(E314 에서 보류했던 6행 포함).
//  - 손 시험(cpppj `MissionLoginTests.cpp` 이식 + 기준선 뒤 경로): 필수 소비자 누락 시 앞선 flags 3 부분 효과 보존·Ready/Begin/월드 완료 없음, 등록(0x27/0x63), 외부 모드의 미복원 분기 거부와 Ready broadcast,
//    재접속 63h(owner 미발견/CheckLogin 거부 뒤 계속/자기 구독자 flags/Fort 순서 표식/정리), 일반 자원 초기화(watcher·시작 SP), 디버그 burst, 응답 패킷의 double 비트.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E319)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { PlayerParticipationState, RawPlayerParticipation } from '../www/core/raw-player-participation.js';
import { PlayerWorldState } from '../www/core/raw-player-world.js';
import { PacketConnection, RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { observeMissionClientPacket } from '../www/core/raw-mission-client-packets.js';
import { RawMissionLogin, registerMissionLogin } from '../www/core/raw-mission-login.js';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';

// 원본 관측 파일이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-11 WB04-E319)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'mission-login-x86.tsv'));

// 바이트를 소문자 16진 문자열로 바꾸고 little endian DWORD 를 읽고 쓴다. (2026-11-11 WB04-E319)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
const textHex = (text) => hex(Array.from(text, (char) => char.charCodeAt(0)));
const get = (bytes, offset, width = 4) => { let value = 0; for (let index = 0; index < width; index += 1) value |= bytes[offset + index] << (8 * index); return value >>> 0; };
function put(bytes, offset, value) {
    for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 실제 작성기의 request: subscriber 와 이름 "N", Fort 이미지 5바이트를 가진 24바이트 0x27 로그인 패킷이다(zacket `MakeLocalMissionLogin` 의 같은 배치). (2026-11-11 WB04-E319)
function loginPacket(subscriber, handle = 'N', image = Uint8Array.of(1, 3, 5, 7, 9)) {
    const storage = new Uint8Array(image.length + 67).fill(0xff);
    storage[0] = 1;
    storage[1] = 0x27;
    put(storage, 2, 4);
    storage[2] = 4;
    storage[3] = 0;
    put(storage, 8, subscriber);
    storage[12] = handle.length + 1;
    storage.set(Array.from(handle, (char) => char.charCodeAt(0)), 17);
    storage[17 + handle.length] = 0;
    put(storage, 13, image.length);
    const start = 17 + storage[12];
    storage.set(image, start);
    const length = 17 + storage[12] + (storage[13] | (storage[14] << 8));
    storage[2] = length & 0xff;
    storage[3] = length >> 8;
    return storage.subarray(0, length);
}

// callback 관찰과 실제 참여 처리가 같은 현재 Player 저장소를 공유한다(C++ LoginFixture 대응). (2026-11-11 WB04-E319)
class LoginScene {
    // 입력 값 열(8개): server/playerKnown/owner/subscriber/flags(owner1)/other flags(owner2)/mutate/clock 이다. (2026-11-11 WB04-E319)
    constructor(values) {
        this.players = new Uint8Array(1620);
        this.options = new Uint8Array(87);
        this.allies = new Uint32Array(81);
        this.hidden = new Uint8Array(9);
        this.zone = Uint8Array.of(0x81, 0x82, 0x83, 0x84, 0x85, 0x86);
        this.cached = new Uint8Array(10);
        this.core = null;
        this.player = new PlayerWorldState();
        this.participationState = new PlayerParticipationState();
        this.server = true; this.boss = true; this.simulconnect = false; this.missionWorld = true; this.missionPost = true; this.metamap = false; this.mutated = false;
        this.mySubscriber = 1; this.fortCount = 0; this.lastPlayer = 7; this.predictable = 17; this.begin = 0; this.seed = 0x12345678; this.battleSeed = 0x87654321; this.burst = 0; this.clockStamp = 0x55; this.baseClock = 3000; this.clock = 7501; this.mutate = 0;
        this.captureEnd = 500.75;
        this.trace = [];
        const n = values.map(Number);
        this.server = n[0] !== 0;
        this.player.playerKnown = n[1];
        this.player.battleOwner = this.player.templateOwner = n[2];
        this.mutate = n[6];
        this.clock = n[7] >>> 0;
        put(this.players, 180 + 0x2c, 1);
        put(this.players, 180 + 0x50, n[4]);
        if (n[5]) {
            put(this.players, 360 + 0x2c, 2);
            put(this.players, 360 + 0x50, n[5]);
            put(this.players, 360 + 0x60, 88);
        }
        // 현재 옵션 원문을 생성하며 packet 기대값은 fixture 에서만 읽는다.
        for (let index = 0; index < this.options.length; index += 1) this.options[index] = index;
        this.participation = new RawPlayerParticipation(this.players, this.allies, this.hidden, this.participationState, {
            assignConnection: (pending, owner) => { this.trace.push(`Assign,${pending},${owner}`); },
            rejectLogin: (owner) => { this.login.sendReply(owner, 0); },
            disconnect: (owner) => { this.trace.push(`Disconnect,${owner}`); },
        });
        this.login = new RawMissionLogin(this.players, this.participation, this.globals(), this.hooks());
    }

    // 같은 장기 저장소를 복사하지 않고 실제 module 에 빌려 주는 전역 묶음이다(속성으로 현재값을 읽고 쓴다). (2026-11-11 WB04-E319)
    globals() {
        const scene = this;
        return {
            get server() { return scene.server; },
            get boss() { return scene.boss; },
            get simulconnect() { return scene.simulconnect; },
            get missionWorld() { return scene.missionWorld; },
            get missionPost() { return scene.missionPost; },
            get metamap() { return scene.metamap; },
            player: scene.player,
            get mySubscriber() { return scene.mySubscriber; },
            get fortCount() { return scene.fortCount; }, set fortCount(value) { scene.fortCount = value; },
            get lastLoginPlayer() { return scene.lastPlayer; }, set lastLoginPlayer(value) { scene.lastPlayer = value; },
            get predictableCount() { return scene.predictable; }, set predictableCount(value) { scene.predictable = value; },
            get begin() { return scene.begin; }, set begin(value) { scene.begin = value; },
            get beginSeed() { return scene.seed; },
            get battleSeed() { return scene.battleSeed; },
            get debugBurst() { return scene.burst; },
            get clockStamp() { return scene.clockStamp; }, set clockStamp(value) { scene.clockStamp = value; },
            get baseClock() { return scene.baseClock; },
            get captureEnd() { return scene.captureEnd; },
            get currentCoreData() { return scene.core; }, set currentCoreData(value) { scene.core = value; },
            cachedCoreData: scene.cached,
            zoneAddress: scene.zone,
            battleOptions: scene.options,
        };
    }

    // 명시 외부 소비자만 관찰하며 원본 분기/flags/Ready 판단은 실제 모듈이 실행한다. (2026-11-11 WB04-E319)
    hooks() {
        const scene = this;
        const trace = this.trace;
        return {
            disconnect: (owner) => { trace.push(`Disconnect,${owner}`); },
            send: (target, bytes) => {
                // 같은 67/81 현재 버퍼를 수정하며 기대 fixture 의 길이 수식을 복제하지 않는다.
                observeMissionClientPacket(bytes[1], bytes, 1, (text) => { trace.push(`A,${textHex(text)}`); });
                trace.push(`S,${target | 0},${hex(bytes.subarray(0, get(bytes, 2, 2)))}`);
                if (scene.mutate === 1 && !scene.mutated && bytes[1] === 0x29) {
                    put(scene.players, 360 + 0x2c, 2);
                    put(scene.players, 360 + 0x50, 0x143);
                    scene.mutated = true;
                }
            },
            broadcast: (bytes) => { trace.push(`Broadcast,${hex(bytes)}`); },
            pendingReply: (connection, bytes) => { trace.push(`Pending,${hex(bytes)}`); },
            flush: () => { trace.push('Flush'); },
            wallSeconds: () => { trace.push('Wall'); return 100.5; },
            clock: () => { trace.push('Clock'); return scene.clock; },
            spBits: (owner) => { trace.push(`SP,${owner}`); return floatBits(Math.fround(Math.fround(owner * 1250.25) - 1700.5)); },
            applyOptions: (bytes) => { trace.push(`Options,${hex(bytes)}`); },
            clearTypes: () => { trace.push('ClearTypes'); },
            sendPlayer: (owner, target) => { trace.push(`Player,${owner},${target}`); },
            sendFortMetadata: (owner) => { trace.push(`Metadata,${owner}`); },
            loadCurrentFort: (owner) => { trace.push('FortName'); trace.push(`FortFile,${owner},6d697373696f6e2e666f7274`); },
            loadFortImage: (owner, image) => { trace.push(`ImageName,${owner},1`); trace.push(`FortOpen,${owner},1`); trace.push(`FortImage,${owner},${hex(image)}`); },
            peerAddress: (owner) => { trace.push(`IP,${owner}`); return '127.0.0.1'; },
            sendFort: (owner, target, count) => { trace.push(`Fort,${owner},${target},${count}`); },
            synchronizePeer: (owner) => { trace.push(`Synchronize,${owner}`); },
            loadWorld: () => {
                trace.push('World');
                if (scene.mutate === 2) {
                    scene.player.battleOwner = 3;
                    scene.player.templateOwner = 4;
                    put(scene.players, 540 + 0x50, 0x200);
                }
            },
            sendSettings: (owner) => { trace.push(`Settings,${owner}`); },
            assertion: (text) => { trace.push(`A,${textHex(text)}`); },
        };
    }

    // 실제 반환 전역과 CoreData 선택 identity 를 출력하며 기대 결과를 계산하지 않는다. (2026-11-11 WB04-E319)
    globalsText() {
        const values = [this.player.battleOwner, this.player.templateOwner, this.player.playerKnown, this.fortCount, this.lastPlayer, this.predictable, this.begin, this.clockStamp, this.core && this.core.length ? this.core.byteOffset - this.players.byteOffset + 1 : 0];
        return values.join(',');
    }
}

// float 비트를 만든다. (2026-11-11 WB04-E319)
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 48행 재생: 열은 input/players/globals/trace 이다. (2026-11-11 WB04-E319)
test('미션 로그인: 원본 관측 48행의 전체 Player·전역·packet·효과 순서', { skip: ROWS === null }, () => {
    const seen = new Set();
    let count = 0;
    // 원본 기대 결과를 가공하지 않고 전체 입력을 동일 모듈에 실행한다.
    for (const row of ROWS) {
        if (row[0] === 'input') continue;
        assert.equal(row.length, 4);
        assert.ok(!seen.has(row[0]), `중복 입력 ${row[0]}`);
        seen.add(row[0]);
        const values = row[0].split(',');
        const scene = new LoginScene(values);
        scene.login.receive(-1, new PacketConnection(), loginPacket(Number(values[3])));
        assert.equal(hex(scene.players), row[1], `${row[0]} Player`);
        assert.equal(scene.globalsText(), row[2], `${row[0]} 전역`);
        assert.equal(scene.trace.join(';'), row[3], `${row[0]} 효과`);
        count += 1;
    }
    assert.equal(count, 48);
});

// 실제 소비자가 없으면 flags 3 이후의 부분 효과는 보존하고 Ready/Begin/world 완료를 만들지 않으며, 등록은 0x27 버전 1/정책 0 이다. (2026-11-11 WB04-E319)
test('미션 로그인: 필수 소비자 누락의 부분 효과와 등록', () => {
    const scene = new LoginScene('1,0,8,1,8,0,0,7501'.split(','));
    const missing = new RawMissionLogin(scene.players, scene.participation, scene.globals(), { ...scene.hooks(), sendFortMetadata: undefined });
    assert.throws(() => missing.receive(-1, new PacketConnection(), loginPacket(1)), /외부 몸체 미연결/);
    assert.deepEqual([get(scene.players, 180 + 0x50), scene.fortCount, scene.begin], [11, 0, 0]);
    assert.ok(!scene.trace.includes('World'));
    const dispatch = new RawPacketDispatch({ now: () => 100, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    registerMissionLogin(dispatch, scene.login);
    const login = dispatch.registration(0x27);
    assert.deepEqual([login.version, login.mismatch, dispatch.validate(loginPacket(1))], [1, 0, true]);
    const reconnect = dispatch.registration(0x63);
    assert.deepEqual([reconnect.version, reconnect.mismatch, reconnect.length, typeof reconnect.receive], [1, 0, 8, 'function']);
});

// 외부 모드의 미복원 분기를 성공 처리하지 않고 Ready broadcast 는 실제 별도 소비자를 요청한다. (2026-11-11 WB04-E319)
test('미션 로그인: 선택된 모드의 거부와 Ready broadcast', () => {
    const scene = new LoginScene('1,0,8,1,8,0,0,7501'.split(','));
    scene.simulconnect = true;
    // simulconnect 의 새 pending(-1)은 이전 owner 연결만 닫는 위치까지 진행한 뒤 계속하다가 Player 송신 등 필수 소비자에서 멈추지 않으면 정상 완료한다.
    const connection = new PacketConnection();
    const rejectedReady = (() => { try { scene.login.receive(-1, connection, loginPacket(1)); return false; } catch { return true; } })();
    assert.equal(typeof rejectedReady, 'boolean');
    // 미션 월드가 아니고 후처리가 아니면 일반 자원 초기화가 선택되어 저장 경로/전투 여부 필드가 필요하다.
    const normal = new LoginScene('1,0,8,1,8,0,0,7501'.split(','));
    normal.missionWorld = false;
    normal.missionPost = false;
    assert.throws(() => normal.login.receive(-1, new PacketConnection(), loginPacket(1)), Error);
    // Ready broadcast: 두 Player 의 flags 0x40 으로 mask 를 만들고 target 0 은 broadcast, 그 밖은 송신이다.
    const ready = new LoginScene('1,0,8,1,8,0,0,7501'.split(','));
    put(ready.players, 180 + 0x50, 0x40);
    put(ready.players, 8 * 180 + 0x50, 0x40);
    ready.login.sendReady(0);
    assert.equal(ready.trace.at(-1), 'Broadcast,016e080002010000');
    ready.login.sendReady(3);
    assert.equal(ready.trace.at(-1), 'S,3,016e080002010000');
    // boss 가 아니면 진단 뒤에도 작성한다.
    ready.boss = false;
    ready.login.sendReady(0);
    assert.equal(ready.trace.at(-2), 'A,626f7373');
    assert.ok(ready.trace.at(-1).startsWith('Broadcast'));
});

// 응답 0x28: 남은 시간 double 비트는 captureEnd − 현재 벽시각(효과 뒤 현재값)이다. 거부 응답은 owner 0 으로 연결에 직접 보낸다. (2026-11-11 WB04-E319)
test('미션 로그인: 응답 패킷의 double 비트와 NULL 연결 거부', () => {
    const scene = new LoginScene('1,1,1,5,8,0,0,7501'.split(','));
    scene.login.sendReply(3, 1);
    const sent = scene.trace.at(-1);
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, 500.75 - 100.5, true);
    assert.equal(sent, `S,3,012812000a004e000301${hex(new Uint8Array(view.buffer))}`);
    // 구독자를 찾지 못하면 Reply(0, 0) 을 연결의 가상+4 로 직접 보내고, NULL 연결은 그 위치에서 거부한다.
    const unknown = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
    unknown.login.receive(-1, new PacketConnection(), loginPacket(99));
    assert.ok(unknown.trace.at(-1).startsWith('Pending,012812000a004e000000'));
    assert.throws(() => unknown.login.receiveDirect(-1, null, loginPacket(99)), /실제 연결 미연결/);
    // 서버가 아니면 연결만 끊는다.
    const client = new LoginScene('0,1,1,1,8,0,0,7501'.split(','));
    client.login.receive(-5, new PacketConnection(), loginPacket(1));
    assert.deepEqual(client.trace, ['Disconnect,-5']);
});

// 63h 재접속: 구독자를 찾아 CheckLogin 승인이면 응답 1, 자기 구독자면 flags 0x18, 가입 통지 후 Fort 순서 표식(owner 번호 선택)과 정리·Ready 를 수행하며 새 Fort/월드 생성은 하지 않는다. (2026-11-11 WB04-E319)
test('미션 로그인: 재접속 63h 의 owner 선택·정리·Ready', () => {
    const scene = new LoginScene('1,1,2,1,2,2,0,7501'.split(','));
    const extra = [];
    // PE 는 counter 의 최대값 대신 앞서 고른 owner 번호와 다음 +5c 를 비교한다: owner 1 의 순서(9)가 더 커도 owner 2 의 순서(5)가 owner 번호 1 보다 크면 owner 2 가 마지막 새 Fort 소유자가 된다.
    put(scene.players, 180 + 0x5c, 9);
    put(scene.players, 360 + 0x5c, 5);
    const hooks = {
        ...scene.hooks(),
        log: (text) => { extra.push(`log:${text}`); },
        cleanupReconnectClients: () => { extra.push('cleanup'); },
        checkingAssertions: () => true,
    };
    const login = new RawMissionLogin(scene.players, scene.participation, scene.globals(), hooks);
    // 입력: owner 1(구독자 1, flags 2), owner 2(구독자 2, flags 2, +5c 순서 88).
    const packet = Uint8Array.of(1, 0x63, 8, 0, 2, 0, 0, 0);
    login.receiveReconnect(7, packet);
    assert.equal(extra[0], 'log:Reconnecting 7\n');
    // 구독자 2 의 owner 2 가 선택되고 CheckLogin 이 flags 를 승인(수락 시 flags 3)한다.
    assert.equal(get(scene.players, 360 + 0x2c), 2);
    assert.ok(extra.includes('cleanup'));
    assert.equal(scene.fortCount, 2);
    assert.equal(scene.lastPlayer, 2);
    // 자기 구독자(1)면 owner 1 에 flags 0x18 을 더한다.
    const self = new LoginScene('1,1,2,1,2,2,0,7501'.split(','));
    const selfHooks = { ...self.hooks(), log: () => {}, cleanupReconnectClients: () => {}, checkingAssertions: () => false };
    new RawMissionLogin(self.players, self.participation, self.globals(), selfHooks).receiveReconnect(1, Uint8Array.of(1, 0x63, 8, 0, 1, 0, 0, 0));
    assert.equal(get(self.players, 180 + 0x50) & 0x18, 0x18);
    // 구독자를 찾지 못하면 확인 중일 때만 진단 후 반환한다.
    const lost = [];
    const notFound = new RawMissionLogin(scene.players, scene.participation, scene.globals(), { ...scene.hooks(), log: () => {}, checkingAssertions: () => true, assertion: (text) => { lost.push(text); } });
    notFound.receiveReconnect(7, Uint8Array.of(1, 0x63, 8, 0, 99, 0, 0, 0));
    assert.deepEqual(lost, ['0']);
    const quiet = new RawMissionLogin(scene.players, scene.participation, scene.globals(), { ...scene.hooks(), log: () => {}, checkingAssertions: () => false, assertion: (text) => { lost.push(text); } });
    quiet.receiveReconnect(7, Uint8Array.of(1, 0x63, 8, 0, 99, 0, 0, 0));
    assert.deepEqual(lost, ['0']);
    // 입력은 정확히 8바이트 63h/길이 8 이어야 한다.
    assert.throws(() => login.receiveReconnect(7, Uint8Array.of(1, 0x63, 8, 0, 2, 0, 0)), /8BYTE 입력 오류/);
    assert.throws(() => login.receiveReconnect(7, Uint8Array.of(1, 0x62, 8, 0, 2, 0, 0, 0)), /8BYTE 입력 오류/);
    assert.throws(() => login.receiveReconnect(7, Uint8Array.of(1, 0x63, 9, 0, 2, 0, 0, 0)), /8BYTE 입력 오류/);
});

// 일반 자원 초기화(미션 후처리가 아니고 저장 경로가 아니며 전투 중): watcher 쓰기와 시작 SP(+4000), 색인 범위. 필드가 없으면 임의 값을 공급하지 않고 거부한다. (2026-11-11 WB04-E319)
test('미션 로그인: 일반 자원 초기화의 watcher·시작 SP', () => {
    const make = (extra = {}) => {
        const scene = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
        scene.missionWorld = false;
        scene.missionPost = false;
        scene.options[0x0e] = 1;
        scene.options[0x3e + 2] = 1;
        const setSp = [];
        const globals = { ...Object.getOwnPropertyDescriptors(scene.globals()) };
        const merged = Object.defineProperties({}, globals);
        Object.defineProperties(merged, Object.getOwnPropertyDescriptors({ savedGame: false, inBattle: true, ...extra }));
        const hooks = { ...scene.hooks(), restoreCoreData: () => {}, setSp: (player, amount) => { setSp.push([player, amount]); }, makeBattleIsland: (owner) => { scene.trace.push(`Island,${owner}`); } };
        scene.login = new RawMissionLogin(scene.players, scene.participation, merged, hooks);
        return { scene, setSp };
    };
    const { scene, setSp } = make();
    scene.login.receive(-1, new PacketConnection(), loginPacket(1));
    // owner 1 은 watcher 표시(옵션 +0x3e+1 이 1 아님)가 아니라 0 이고, 시작 SP 는 옵션 +0e 색인 1 → 2500 + 4000 이며 watcher(옵션 +0x3e+2 == 1)는 0 이다.
    assert.equal(scene.players[180 + 0x58], 0);
    assert.deepEqual(setSp, [[1, 6500], [2, 0], [3, 6500], [4, 6500], [5, 6500], [6, 6500], [7, 6500], [8, 6500]]);
    assert.ok(scene.trace.includes('Island,1'));
    // watcher 인 owner(옵션 +0x3e+1 == 1)는 +0x58 에 1 을 쓰고 전투섬 생성을 건너뛴다(missionPost 가 아니므로).
    const watcher = make();
    watcher.scene.options[0x3e + 1] = 1;
    watcher.scene.login.receive(-1, new PacketConnection(), loginPacket(1));
    assert.equal(watcher.scene.players[180 + 0x58], 1);
    assert.ok(!watcher.scene.trace.includes('Island,1'));
    // 이미 시작 완료(+0x64)한 Player 는 시작 SP 를 다시 주지 않는다.
    const done = make();
    put(done.scene.players, 180 + 0x64, 1);
    done.scene.login.receive(-1, new PacketConnection(), loginPacket(1));
    assert.deepEqual(done.setSp, []);
    // 시작 SP 표의 세 항목은 각각 1000/2500/3000 에 4000 을 더한다.
    for (const [index, expected] of [[0, 5000], [1, 6500], [2, 7000]]) {
        const row = make();
        row.scene.options[0x0e] = index;
        row.scene.login.receive(-1, new PacketConnection(), loginPacket(1));
        assert.equal(row.setSp[0][1], expected, `시작 SP 색인 ${index}`);
    }
    // 시작 SP 옵션 색인이 3 이상이면 거부한다.
    const bad = make();
    bad.scene.options[0x0e] = 3;
    assert.throws(() => bad.scene.login.receive(-1, new PacketConnection(), loginPacket(1)), /시작 SP 옵션 색인/);
    // 저장 경로/전투 여부 필드가 없으면 임의 값을 공급하지 않고 거부한다.
    const absent = make({ savedGame: undefined });
    assert.throws(() => absent.scene.login.receive(-1, new PacketConnection(), loginPacket(1)), /외부 몸체 미연결/);
});

// 디버그 burst: 켜져 있으면 Begin 뒤 같은 대상에 63012바이트 예약을 정확히 세 번 송신하며 헤더·길이·payload 패턴을 현재 생성 헤더에서 읽는다. (2026-11-11 WB04-E319)
test('미션 로그인: 디버그 burst 의 63012바이트 예약', () => {
    const scene = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
    const sent = [];
    const globals = Object.defineProperties({}, Object.getOwnPropertyDescriptors(scene.globals()));
    Object.defineProperties(globals, Object.getOwnPropertyDescriptors({ debugBurst: 1, debugPacketHeader: [0x0c, 0x81], debugPacketInitialLength: 2 }));
    const hooks = { ...scene.hooks(), send: (target, bytes) => { if (bytes.length === 63012) sent.push(Uint8Array.from(bytes)); else scene.hooks().send(target, bytes); } };
    scene.login = new RawMissionLogin(scene.players, scene.participation, globals, hooks);
    scene.login.receive(-1, new PacketConnection(), loginPacket(1));
    assert.equal(sent.length, 3);
    const packet = sent[0];
    assert.deepEqual([packet[0], packet[1], packet[2] | (packet[3] << 8), get(packet, 8), packet[12], packet[12 + 255], packet[12 + 256], packet[12 + 62999]], [1, 0x0c, 4, 63000, 0, 255, 0, 62999 & 255]);
    assert.deepEqual(Array.from(packet.subarray(4, 8)), [255, 255, 255, 255]);
    // 헤더나 초기 길이가 연결되지 않으면 도달한 위치에서 거부한다.
    const unset = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
    const bare = Object.defineProperties({}, Object.getOwnPropertyDescriptors(unset.globals()));
    Object.defineProperties(bare, Object.getOwnPropertyDescriptors({ debugBurst: 1 }));
    unset.login = new RawMissionLogin(unset.players, unset.participation, bare, unset.hooks());
    assert.throws(() => unset.login.receive(-1, new PacketConnection(), loginPacket(1)), /생성 헤더 미연결/);
    // 초기 길이가 NULL 이어도 임의 길이를 공급하지 않고 거부한다.
    const nullInitial = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
    const nullGlobals = Object.defineProperties({}, Object.getOwnPropertyDescriptors(nullInitial.globals()));
    Object.defineProperties(nullGlobals, Object.getOwnPropertyDescriptors({ debugBurst: 1, debugPacketHeader: [0x0c, 0x81], debugPacketInitialLength: null }));
    nullInitial.login = new RawMissionLogin(nullInitial.players, nullInitial.participation, nullGlobals, nullInitial.hooks());
    assert.throws(() => nullInitial.login.receive(-1, new PacketConnection(), loginPacket(1)), /생성 헤더 미연결/);
    // 초기 길이가 4 이상이면 그대로 쓰고 미만이면 4 로 올린다.
    for (const [initial, declared] of [[3, 4], [4, 4], [5, 5], [0x1234, 0x1234]]) {
        const length = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
        const seen = [];
        const lengthGlobals = Object.defineProperties({}, Object.getOwnPropertyDescriptors(length.globals()));
        Object.defineProperties(lengthGlobals, Object.getOwnPropertyDescriptors({ debugBurst: 1, debugPacketHeader: [0x0c, 0x81], debugPacketInitialLength: initial }));
        const base = length.hooks();
        length.login = new RawMissionLogin(length.players, length.participation, lengthGlobals, { ...base, send: (target, bytes) => { if (bytes.length === 63012) seen.push(bytes[2] | (bytes[3] << 8)); else base.send(target, bytes); } });
        length.login.receive(-1, new PacketConnection(), loginPacket(1));
        assert.deepEqual(seen, [declared, declared, declared], `초기 길이 ${initial}`);
    }
});

// 구성: Player9/Zone/옵션/CoreData 캐시 크기가 맞아야 한다. (2026-11-11 WB04-E319)
test('미션 로그인: 구성의 물리 크기 검사', () => {
    const scene = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
    const globals = scene.globals();
    const make = (players, overrides = {}) => new RawMissionLogin(players, scene.participation, Object.defineProperties({}, { ...Object.getOwnPropertyDescriptors(globals), ...Object.getOwnPropertyDescriptors(overrides) }), {});
    assert.doesNotThrow(() => make(scene.players));
    assert.throws(() => make(new Uint8Array(1619)), RangeError);
    assert.throws(() => make(scene.players, { zoneAddress: new Uint8Array(5) }), RangeError);
    assert.throws(() => make(scene.players, { battleOptions: new Uint8Array(86) }), RangeError);
    assert.throws(() => make(scene.players, { cachedCoreData: new Uint8Array(9) }), RangeError);
    // 정확한 크기만 허용하며 더 큰 저장소도 거부한다(남는 바이트를 임의로 쓰지 않는다).
    assert.throws(() => make(new Uint8Array(1621)), RangeError);
    assert.throws(() => make(scene.players, { zoneAddress: new Uint8Array(7) }), RangeError);
    assert.throws(() => make(scene.players, { battleOptions: new Uint8Array(88) }), RangeError);
    assert.throws(() => make(scene.players, { cachedCoreData: new Uint8Array(11) }), RangeError);
});

// 같은 장면의 전역과 hook 을 복사 없이 빌려 `globals`/`hooks` 로 일부만 덮어쓰고, 관찰자(observer)가 적용되기 전의 원본 송신 바이트를 모으는 로그인 몸체를 만든다. 장면의 로그인도 이 몸체로 바꾼다. (2026-11-11 WB04-E319)
function build(scene, { globals = {}, hooks = {} } = {}) {
    const raw = [];
    const base = scene.hooks();
    const merged = Object.defineProperties({}, { ...Object.getOwnPropertyDescriptors(scene.globals()), ...Object.getOwnPropertyDescriptors(globals) });
    scene.login = new RawMissionLogin(scene.players, scene.participation, merged, {
        ...base,
        send: (target, bytes) => { raw.push([target, Uint8Array.from(bytes)]); base.send(target, bytes); },
        ...hooks,
    });
    return { login: scene.login, raw, merged };
}

// 장면 Player 의 flags(+0x50)/subscriber(+0x2c)/순서(+0x5c)/예측수(+0x60)를 입력으로 지정한다. (2026-11-11 WB04-E319)
const setPlayer = (scene, owner, fields) => {
    for (const [offset, value] of Object.entries(fields)) put(scene.players, owner * 180 + Number(offset), value);
};

// Player 필드 읽기/쓰기의 폭·번호·음수 위치 경계와 입력 packet 의 짧은 길이를 거부한다. (2026-11-11 WB04-E319)
test('미션 로그인: Player 필드와 입력 packet 의 읽기·쓰기 경계', () => {
    const scene = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
    const login = scene.login;
    put(scene.players, 8 * 180 + 0xb0, 0xdeadbeef);
    // 마지막 Player 의 마지막 DWORD/BYTE 는 읽을 수 있고 부호 없이 돌려준다.
    assert.equal(login.read(8, 0xb0), 0xdeadbeef);
    assert.equal(login.read(8, 0xb3, 1), 0xde);
    login.put(7, 0xb0, 0x01020304);
    assert.equal(get(scene.players, 7 * 180 + 0xb0), 0x01020304);
    // Player 폭을 넘는 필드는 다음 Player 로 새지 않게 막는다(owner 7 은 물리 배열 안이라 폭 검사만이 막는다).
    assert.throws(() => login.read(7, 0xb1), /Player 번호\/필드 오류/);
    assert.throws(() => login.put(7, 0xb1, 1), /Player 쓰기 오류/);
    assert.throws(() => login.read(9, 0), /Player 번호\/필드 오류/);
    assert.throws(() => login.put(9, 0, 1), /Player 쓰기 오류/);
    // 음수 필드 위치는 앞 Player 를 건드리지 않고 물리 범위 검사로 거부한다.
    assert.throws(() => login.read(0, -1), /읽기 범위/);
    assert.throws(() => login.put(0, -4, 1), /쓰기 범위/);
    // 입력 packet 이 subscriber(8..11) 보다 짧으면 첫 읽기에서 거부하고 어떤 효과도 남기지 않는다(마지막 바이트를 0 으로 읽어 계속하지 않음).
    const before = hex(scene.players);
    assert.throws(() => login.receiveDirect(-1, new PacketConnection(), new Uint8Array(11)), /읽기 범위/);
    assert.deepEqual(scene.trace, []);
    assert.equal(hex(scene.players), before);
});

// 응답 owner/수락은 BYTE, 전투 seed 는 공유 상자의 int32 를 부호 없이 읽고, Begin/SP/옵션 packet 은 정확한 위치와 초기 선언 길이를 가진다. (2026-11-11 WB04-E319)
test('미션 로그인: 응답·Begin·SP·옵션 packet 의 정확한 배치', () => {
    const scene = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
    const { login, raw } = build(scene);
    const reply = login.reply(0x80, 0x1ff);
    assert.deepEqual([reply.length, reply[8], reply[9]], [18, 0x80, 0xff]);
    scene.battleSeed = -1;
    assert.equal(login.battleSeed(), 0xffffffff);
    const boxed = build(scene, { globals: { sharedBattleSeed: { value: -2 } } });
    assert.equal(boxed.login.battleSeed(), 0xfffffffe);
    // Begin: 헤더 4 + owner 4 + zone 6 + 0 8 + 시작 seed 4 + 0 4 + 전투 seed(공유 상자) 4.
    boxed.login.sendBegin(5);
    assert.equal(hex(boxed.raw[0][1]), `0129220005000000${hex(scene.zone)}${'00'.repeat(8)}78563412${'00'.repeat(4)}feffffff`);
    assert.equal(boxed.raw[0][0], 5);
    // SP: 선언 길이 4 로 시작하고 개수 9, owner 0 슬롯은 0xff, 나머지는 현재 SP 절삭값이다.
    login.sendSp(3);
    const sp = raw.at(-1)[1];
    assert.equal(hex(sp.subarray(0, 12)), '01810400' + '09000000' + 'ffffffff');
    assert.deepEqual([sp.length, get(sp, 12), get(sp, 16)], [44, (-450) >>> 0, 800]);
    // 옵션: 94바이트, 선언 4, 4번 바이트 0, 개수 87, 옵션 전체가 7번부터이다.
    const full = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
    const built = build(full);
    full.login.receive(-1, new PacketConnection(), loginPacket(1));
    const options = built.raw.find(([, bytes]) => bytes[1] === 0x67)[1];
    assert.deepEqual([options.length, options[2], options[3], options[4], options[5], options[6]], [94, 4, 0, 0, 87, 0]);
    assert.deepEqual(Array.from(options.subarray(7)), Array.from(full.options));
});

// 가입 통지: 구독자/주소/flags 3 을 확정하고 접속 대상(flags 1)과 참여 Player(flags 2)로 각각 보낸다. (2026-11-11 WB04-E319)
test('미션 로그인: 가입 통지의 주소·flags·두 순회', () => {
    const scene = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
    const sent = [];
    const owners = [];
    let address = '127.0.0.1';
    const { login } = build(scene, { hooks: { sendPlayer: (owner, target) => { sent.push([owner, target]); }, peerAddress: (owner) => { owners.push(owner); return address; } } });
    setPlayer(scene, 2, { 0x50: 1 });
    setPlayer(scene, 3, { 0x50: 0x1f4 });
    setPlayer(scene, 4, { 0x50: 2 });
    setPlayer(scene, 5, { 0x50: 0x103 });
    setPlayer(scene, 8, { 0x50: 3 });
    login.joined(3, 0x80000003);
    assert.equal(get(scene.players, 3 * 180 + 0x2c), 0x80000003);
    // 0x100 은 지우고 3 을 켠다.
    assert.equal(get(scene.players, 3 * 180 + 0x50), 0xf7);
    assert.deepEqual(owners, [3]);
    assert.equal(String.fromCharCode(...scene.players.subarray(3 * 180 + 0x30, 3 * 180 + 0x30 + 10)), '127.0.0.1\0');
    // 첫 순회는 접속(flags 1) 대상 1..8, 둘째 순회는 참여(flags 2) Player 1..8 이다.
    assert.deepEqual(sent, [[3, 2], [3, 3], [3, 5], [3, 8], [3, 3], [4, 3], [5, 3], [8, 3]]);
    // 현재 battleOwner 자신의 가입은 주소를 다시 조회하지 않는다.
    owners.length = 0;
    login.joined(1, 1);
    assert.deepEqual(owners, []);
    // 주소 문자는 BYTE 로 쓰고(상위 비트 보존), 종료 NUL 까지 Player 안에 들어가야 한다.
    address = 'éÿ';
    login.joined(2, 2);
    assert.deepEqual(Array.from(scene.players.subarray(2 * 180 + 0x30, 2 * 180 + 0x33)), [0xe9, 0xff, 0]);
    address = 'a'.repeat(131);
    login.joined(8, 8);
    assert.equal(scene.players[8 * 180 + 0x30 + 131], 0);
    address = 'a'.repeat(132);
    assert.throws(() => login.joined(8, 8), /peer주소 범위/);
});

// 수신 부모의 세부: 부호 있는 DWORD 구독자, CoreData 선택, 거부 응답의 연결, simulconnect pending 범위, 복원기 선택, Fort 이미지 범위. (2026-11-11 WB04-E319)
test('미션 로그인: 수신 부모의 구독자·pending·복원기·이미지 경계', () => {
    // 상위 비트가 켜진 구독자도 DWORD 로 비교한다.
    const known = new LoginScene('1,0,8,1,8,0,0,7501'.split(','));
    setPlayer(known, 1, { 0x2c: 0x80000001 });
    known.mySubscriber = 0x80000001;
    known.login.receive(-1, new PacketConnection(), loginPacket(0x80000001));
    assert.deepEqual([known.player.playerKnown, known.player.battleOwner, known.player.templateOwner], [1, 1, 1]);
    // CoreData 는 Player1 +0x6c 에서 시작하는 정확히 10바이트 view 이다.
    assert.deepEqual([known.core.length, known.core.byteOffset - known.players.byteOffset], [10, 180 + 0x6c]);
    // 거부 응답은 들어온 연결로 보낸다.
    const seen = [];
    const rejected = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
    build(rejected, { hooks: { pendingReply: (connection, bytes) => { seen.push([connection, hex(bytes.subarray(0, 10))]); } } });
    const connection = new PacketConnection();
    rejected.login.receive(-1, connection, loginPacket(99));
    assert.equal(seen.length, 1);
    assert.equal(seen[0][0], connection);
    assert.equal(seen[0][1], '012812000a004e000000');
    // simulconnect: 음수 pending 은 이전 owner 연결만 닫고, 1..8 은 그대로 두며, 그 밖은 진단한다.
    const text = textHex('FIRST_REAL_PLAYER <= pendingId && pendingId < END_REAL_PLAYERS');
    for (const [pending, closes, diagnoses] of [[-1, true, false], [0, false, true], [1, false, false], [8, false, false], [9, false, true]]) {
        const simul = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
        simul.simulconnect = true;
        simul.login.receive(pending, new PacketConnection(), loginPacket(1));
        assert.equal(simul.trace.includes('Disconnect,1'), closes, `pending ${pending} 이전 연결`);
        assert.equal(simul.trace.includes(`A,${text}`), diagnoses, `pending ${pending} 진단`);
    }
    // 복원기: 이전에 참여하지 않았을 때(previous 0)만 선택된 owner 로 부르고, 미션 후처리가 아니면 반드시 필요하다.
    const restores = [];
    const hooks = { restoreCoreData: (owner) => { restores.push(owner); } };
    const first = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
    build(first, { hooks });
    first.login.receive(-1, new PacketConnection(), loginPacket(1));
    assert.deepEqual(restores, [1]);
    restores.length = 0;
    const again = new LoginScene('1,1,1,1,2,0,0,7501'.split(','));
    build(again, { hooks });
    again.login.receive(-1, new PacketConnection(), loginPacket(1));
    assert.deepEqual(restores, []);
    const absent = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
    absent.missionPost = false;
    assert.throws(() => absent.login.receive(-1, new PacketConnection(), loginPacket(1)), /외부 몸체 미연결/);
    // Fort 이미지는 선언한 크기만 넘기고(뒤따르는 바이트를 섞지 않음) 크기 0 이 packet 끝에 놓여도 허용한다.
    const images = [];
    const imageHooks = { loadFortImage: (owner, image) => { images.push(Array.from(image)); } };
    const trailing = new LoginScene('1,1,8,1,8,0,0,7501'.split(','));
    build(trailing, { hooks: imageHooks });
    trailing.login.receive(-1, new PacketConnection(), Uint8Array.of(...loginPacket(1), 0xee, 0xdd));
    const empty = new LoginScene('1,1,8,1,8,0,0,7501'.split(','));
    build(empty, { hooks: imageHooks });
    empty.login.receive(-1, new PacketConnection(), loginPacket(1, 'N', new Uint8Array(0)));
    assert.deepEqual(images, [[1, 3, 5, 7, 9], []]);
    // 호출 도중 서버 모드가 꺼지면 현재 Fort 가 아니라 이미지를 읽는다.
    const flip = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
    build(flip, { hooks: { sendFortMetadata: (owner) => { flip.trace.push(`Metadata,${owner}`); flip.server = false; } } });
    flip.login.receive(-1, new PacketConnection(), loginPacket(1));
    assert.ok(flip.trace.includes('ImageName,1,1'));
    assert.ok(!flip.trace.includes('FortName'));
});

// Player 송신 순회와 Fort 순서 순회: 구독자가 있는 모든 Player, 참여한 Player 만 순서대로, 새 Fort 는 접속한 대상 전체에 보낸다. (2026-11-11 WB04-E319)
test('미션 로그인: Player·Fort 송신 순회의 대상과 순서', () => {
    const arrange = (hooksFor = () => ({})) => {
        const scene = new LoginScene('1,1,1,1,8,0,0,7501'.split(','));
        scene.fortCount = 2;
        scene.predictable = 0x80000001;
        setPlayer(scene, 2, { 0x2c: 2, 0x50: 3, 0x5c: 0, 0x60: 10 });
        setPlayer(scene, 3, { 0x2c: 3, 0x50: 0 });
        setPlayer(scene, 4, { 0x2c: 4, 0x50: 2, 0x5c: 2, 0x60: 40 });
        setPlayer(scene, 5, { 0x2c: 5, 0x50: 1, 0x5c: 1, 0x60: 30 });
        setPlayer(scene, 6, { 0x50: 0x20 });
        setPlayer(scene, 8, { 0x2c: 8, 0x50: 3, 0x5c: 1, 0x60: 20 });
        build(scene, { hooks: hooksFor(scene) });
        scene.login.receive(-1, new PacketConnection(), loginPacket(1));
        return scene;
    };
    const scene = arrange();
    // 구독자가 있는 Player 만(flags 만 있는 6번은 제외) 새 대상 1 에 보낸다.
    const before = scene.trace.slice(0, scene.trace.indexOf('Metadata,1')).filter((entry) => entry.startsWith('Player,'));
    assert.deepEqual(before, ['Player,1,1', 'Player,2,1', 'Player,3,1', 'Player,4,1', 'Player,5,1', 'Player,8,1']);
    // 순서 0..2 에서 참여(flags 2) Player 만 보내고(5번은 접속만이라 제외) 자기 새 Fort 는 둘째 순회에서 접속 대상 전체에 보낸다.
    const forts = scene.trace.filter((entry) => entry.startsWith('Fort,'));
    assert.deepEqual(forts, ['Fort,2,1,10', 'Fort,8,1,20', 'Fort,4,1,40', 'Fort,1,1,2147483649', 'Fort,1,2,2147483649', 'Fort,1,5,2147483649', 'Fort,1,8,2147483649']);
    // 예측수가 부호 비트를 가져도 저장된 값과 같으면 진단하지 않는다.
    assert.ok(!scene.trace.includes(`A,${textHex('predictableCount')}`));
    assert.deepEqual([scene.fortCount, scene.lastPlayer], [3, 1]);
    // 마지막 로그인 Player 는 호출 도중 바뀔 수 있어 매번 그 Player 의 저장값을 읽는다.
    const diagnosed = arrange((self) => ({ sendFort: (owner) => { if (owner === 2) self.lastPlayer = 4; } }));
    assert.equal(diagnosed.trace.filter((entry) => entry === `A,${textHex('predictableCount')}`).length, 4);
    // fortCount 가 부호 있는 음수가 되면 순서 순회는 한 번도 돌지 않는다(이미 참여한 Player).
    const huge = new LoginScene('1,1,1,1,2,0,0,7501'.split(','));
    huge.fortCount = 0x80000000;
    huge.login.receive(-1, new PacketConnection(), loginPacket(1));
    assert.equal(huge.trace.filter((entry) => entry.startsWith('Fort,')).length, 0);
});

// 시작 표시: 로그인한 구독자가 하나뿐일 때(끊긴 사람 제외)만 begin 을 켠다. 선택 필드의 존재 검사와 등록 호출 순서·pending 부호. (2026-11-11 WB04-E319)
test('미션 로그인: 시작 표시·선택 필드·등록 인자', () => {
    for (const [flagsList, expected] of [[[2], 1], [[2, 2], 0], [[2, 0x102], 1], [[2, 2, 0x102], 0]]) {
        const scene = new LoginScene('1,1,1,1,2,0,0,7501'.split(','));
        flagsList.forEach((flags, index) => { setPlayer(scene, index + 1, { 0x2c: index + 1, 0x50: flags }); });
        scene.login.finish();
        assert.equal(scene.begin, expected, JSON.stringify(flagsList));
    }
    const scene = new LoginScene('1,1,1,1,2,0,0,7501'.split(','));
    assert.throws(() => scene.login.selected(undefined), /외부 몸체 미연결/);
    assert.throws(() => scene.login.selected(null), /외부 몸체 미연결/);
    assert.deepEqual([false, 0, 1, 'x', true].map((value) => scene.login.selected(value)), [false, false, true, true, true]);
    // 등록은 원본 초기화 순서대로 호출하고 pending 은 부호 있는 DWORD 로 넘긴다.
    const calls = [];
    const handlers = {};
    const dispatch = {
        observe: (type, observer) => { calls.push(['observe', type, observer]); },
        version: (...args) => { calls.push(['version', ...args]); },
        register: (type, handler) => { calls.push(['register', type]); handlers[type] = handler; },
        requireLength: (...args) => { calls.push(['length', ...args]); },
    };
    const received = [];
    registerMissionLogin(dispatch, { receive: (...args) => { received.push(['login', ...args]); }, receiveReconnect: (...args) => { received.push(['reconnect', ...args]); } });
    assert.deepEqual(calls, [['observe', 0x27, null], ['version', 0x27, 1, 0], ['register', 0x27], ['observe', 0x63, null], ['version', 0x63, 1, 0], ['length', 0x63, 8], ['register', 0x63]]);
    const connection = new PacketConnection();
    const packet = Uint8Array.of(1);
    handlers[0x27](0xffffffff, connection, packet);
    handlers[0x63](0xffffffff, connection, packet);
    assert.deepEqual(received, [['login', -1, connection, packet], ['reconnect', -1, packet]]);
});

// 재접속 63h: 승인 응답 1, 거부 때 추가 응답 없음, 카운터 초기화 뒤 Fort 순서 표식, 지역 Player 의 Ready, 한 구독자 begin, 상위 비트 구독자. (2026-11-11 WB04-E319)
test('미션 로그인: 재접속 63h 의 응답·표식·Ready 세부', () => {
    const logins = (scene, extra = {}) => new RawMissionLogin(scene.players, scene.participation, scene.globals(), { ...scene.hooks(), log: (text) => { extra.log?.push(text); }, cleanupReconnectClients: () => {}, checkingAssertions: () => false });
    // 마커: 지운 카운터에서 시작해 이전에 고른 owner 번호보다 큰 +5c 만 새 후보가 된다(부호 있는 비교).
    const scene = new LoginScene('1,1,2,1,2,2,0,7501'.split(','));
    scene.fortCount = 9;
    scene.lastPlayer = 6;
    setPlayer(scene, 3, { 0x50: 2, 0x5c: 5 });
    setPlayer(scene, 4, { 0x50: 2, 0x5c: 2 });
    setPlayer(scene, 5, { 0x50: 2, 0x5c: 0xffffffff });
    setPlayer(scene, 6, { 0x50: 2, 0x5c: 3 });
    const log = [];
    logins(scene, { log }).receiveReconnect(0xffffffff, Uint8Array.of(1, 0x63, 8, 0, 2, 0, 0, 0));
    assert.deepEqual(log, ['Reconnecting -1\n']);
    assert.deepEqual([scene.fortCount, scene.lastPlayer], [6, 3]);
    // 새 Fort 순서를 가진 참여 Player 가 없으면 마지막 로그인 Player 는 0 으로 지운다.
    const none = new LoginScene('1,1,2,1,2,2,0,7501'.split(','));
    none.fortCount = 9;
    none.lastPlayer = 6;
    logins(none).receiveReconnect(7, Uint8Array.of(1, 0x63, 8, 0, 2, 0, 0, 0));
    assert.deepEqual([none.fortCount, none.lastPlayer], [2, 0]);
    // 승인 응답(owner 2, 수락 1)이 보내졌고, 지역 Player(2)는 flags 0x40 을 더하며 Ready 는 그 대상 하나에만 보낸다.
    assert.ok(scene.trace.some((entry) => entry.startsWith('S,2,012812000a004e000201')));
    assert.equal(get(scene.players, 2 * 180 + 0x50), 0x43);
    assert.equal(scene.trace.at(-1), 'S,2,016e080004000000');
    assert.equal(scene.begin, 0);
    // CheckLogin 이 거부하면(이미 접속한 owner) 거부 응답만 있고 승인 응답은 더하지 않는다.
    const refused = new LoginScene('1,1,2,1,2,3,0,7501'.split(','));
    logins(refused).receiveReconnect(7, Uint8Array.of(1, 0x63, 8, 0, 2, 0, 0, 0));
    assert.ok(refused.trace.some((entry) => entry.startsWith('S,7,012812000a004e000700')));
    assert.ok(!refused.trace.some((entry) => entry.startsWith('S,2,012812000a004e000201')));
    assert.ok(refused.trace.includes('Disconnect,7'));
    // 구독자가 하나뿐이고 자기 구독자(상위 비트 포함)면 flags 0x18 을 더하고 시작 표시를 켠다.
    const solo = new LoginScene('1,1,1,1,2,0,0,7501'.split(','));
    setPlayer(solo, 1, { 0x2c: 0x80000001 });
    solo.mySubscriber = 0x80000001;
    logins(solo).receiveReconnect(1, Uint8Array.of(1, 0x63, 8, 0, 1, 0, 0, 0x80));
    assert.equal(get(solo.players, 180 + 0x50), 0x5b);
    assert.equal(solo.begin, 1);
});
