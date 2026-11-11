// 로그인 서버가 보낸 실제 8종 패킷의 수신 처리(cpppj `o/RawMissionClientPackets`와 대응표 밖 신규 `RawStateBattleOptions`)의 웹 대응 `core/raw-mission-client-packets.js` 를 검사한다(4-C, WB04-E314).
//  - 원본 독립 관측 `mission-client-packets-x86.tsv` 157행(8종 수신 × 서버/boss/미션 모드 × 알려진 Player × 변형 × 호출 도중 변경)의 전체 Player 1620바이트·방향별 동맹 81DWORD·현재 전역 20개·옵션/zone/cache/TYPE 쓰기·SP 9칸·외부 효과 순서를 재생한다.
//    Ready 의 boss 서버 경로 6행은 실제 `RawMissionLogin.sendReady` 몸체(E319)를 같은 장면(mission-client-scene.mjs)에 연결해 함께 재생한다(E314 에서는 로그인 몸체가 없어 보류했던 행).
//  - 손 시험(cpppj `MissionClientPacketsTests.cpp` 이식): 미연결 소비자는 앞선 옵션 복사를 남긴 채 거부, 8종 등록(관찰기·버전·정책), 이름 정제·송신 observer·시간 응답 등 계약.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E314)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { PacketConnection, RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { RawMissionClientPackets, observeMissionClientPacket, receiveStateBattleOptions, registerMissionClientPackets, sanitizeMissionPlayerName } from '../www/core/raw-mission-client-packets.js';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { ClientPacketScene, floatBits, hex, hexBytes, put } from './mission-client-scene.mjs';

// 원본 관측 파일이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-11 WB04-E314)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'mission-client-packets-x86.tsv'));

// 현재 저장소 장면은 로그인 시험과 공유하는 도우미(mission-client-scene.mjs)를 쓴다. (2026-11-11 WB04-E319)
// SP 하위의 논리 쓰기 호출(`Set,`)은 전체 9 SP 바이트로 대조하고 나머지 외부 효과는 모두 순서대로 대조한다. (2026-11-11 WB04-E314)
function effectTrace(text) {
    const result = text.split(';').filter((effect) => effect && !effect.startsWith('Set,')).join(';');
    return result || '-';
}

// 157행: 열은 input/packet/players/allies/state/storage/sp/trace 이다. Ready boss 경로 6행도 실제 로그인 몸체의 sendReady 로 재생한다. (2026-11-11 WB04-E314, E319 보강)
test('미션 클라이언트 패킷: 원본 관측 157행의 전체 Player·동맹·전역·SP·효과', { skip: ROWS === null }, () => {
    const seen = new Set();
    let replayed = 0;
    // 모든 기대값은 독립 기계어 fixture 의 원문 전체 출력이다.
    for (const row of ROWS) {
        if (row[0] === 'input') continue;
        assert.equal(row.length, 8);
        assert.ok(!seen.has(row[0]), `중복 입력 ${row[0]}`);
        seen.add(row[0]);
        const values = row[0].split(',');
        const scene = new ClientPacketScene(values, hexBytes(row[1]));
        scene.packets.receive(Number(values[8]), new PacketConnection(), scene.packet);
        assert.equal(hex(scene.players), row[2], `${row[0]} Player`);
        assert.equal(scene.alliesText(), row[3], `${row[0]} 동맹`);
        assert.equal(scene.stateText(), row[4], `${row[0]} 전역`);
        assert.equal(scene.storageText(), row[5], `${row[0]} 저장`);
        assert.equal(scene.spText(), row[6], `${row[0]} SP`);
        assert.equal(scene.traceText(), effectTrace(row[7]), `${row[0]} 효과`);
        replayed += 1;
    }
    assert.equal(replayed, 157);
});

// 이름 정제: 금지 문자(0xa0 과 % . ? = ( ) [ ] ~ " ` ; : 공백)를 NUL 까지 당겨 지우고 연속 금지문자도 제거하며, 비면 Cheat<번호>로 대체한다. NUL 뒤 stale 바이트는 지우지 않는다. (2026-11-11 WB04-E314)
test('미션 클라이언트 패킷: 이름 정제의 삭제·대체 규칙', () => {
    const name = (text, size = 16) => { const bytes = new Uint8Array(size).fill(0x7a); bytes.set(Array.from(text, (char) => char.charCodeAt(0))); bytes[text.length] = 0; return bytes; };
    const run = (text, subscriber = 1, size = 16) => { const bytes = name(text, size); sanitizeMissionPlayerName(subscriber, bytes); return bytes; };
    const view = (bytes) => hex(bytes.subarray(0, bytes.indexOf(0) + 1));
    assert.equal(view(run('Good')), '476f6f6400'.slice(0, 10));
    assert.equal(view(run('a b.c')), '61626300');
    assert.equal(view(run('..a..b..')), '616200');
    assert.equal(view(run('x%?=()[]~"`;:y')), '787900');
    // 0xa0 도 금지 문자이다.
    const nbsp = name('ab');
    nbsp[1] = 0xa0;
    nbsp[2] = 0x63;
    nbsp[3] = 0;
    sanitizeMissionPlayerName(1, nbsp);
    assert.equal(view(nbsp), '616300');
    // 당겨 쓴 뒤 원래 NUL 위치는 NUL 로 남고(NUL 도 한 칸 당겨지고 이름 길이가 줄어든 새 위치에 NUL), 그 뒤 stale 바이트는 지우지 않는다.
    const stale = run('a.b');
    assert.deepEqual(Array.from(stale.subarray(0, 5)), [0x61, 0x62, 0, 0, 0x7a]);
    // 이름이 비면 Cheat<구독자 & 0xff> 로 대체한다.
    assert.equal(view(run('...', 7)), hex(Array.from('Cheat7', (char) => char.charCodeAt(0))) + '00');
    assert.equal(view(run('', 0x1ff)), hex(Array.from('Cheat255', (char) => char.charCodeAt(0))) + '00');
    // 대체 이름이 저장소보다 길면 거부하고, NUL 없는 이름도 거부한다.
    assert.throws(() => run('', 123, 6), RangeError);
    assert.throws(() => sanitizeMissionPlayerName(1, new Uint8Array(4).fill(0x61)), RangeError);
});

// 송신 observer: 수신 방향(0)은 읽지 않고, 송신은 현재 필드로 선언 WORD 를 다시 쓴다. 진단이 복귀하면 현재 packet 을 다시 읽어 길이를 쓴다. (2026-11-11 WB04-E314)
test('미션 클라이언트 패킷: 송신 observer 의 길이 확정과 진단', () => {
    const seen = [];
    const assertion = (expression, file, line) => { seen.push([expression, file, line]); };
    const wire = (packet) => packet[2] | (packet[3] << 8);
    // 수신 방향은 아무것도 하지 않는다(장치가 없어도 된다).
    const untouched = new Uint8Array(64).fill(9);
    observeMissionClientPacket(0x2b, untouched, 0, undefined);
    assert.ok(untouched.every((byte) => byte === 9));
    // Fort(0x2a): 선언 길이 = Fort 크기 WORD + 0x11, 크기 DWORD 가 -1 이면 진단.
    const fort = new Uint8Array(32);
    put(fort, 0xc, 0x100);
    observeMissionClientPacket(0x2a, fort, 1, assertion);
    assert.equal(wire(fort), 0x111);
    put(fort, 0xc, 0xffffffff);
    observeMissionClientPacket(0x2a, fort, 1, assertion);
    assert.deepEqual(seen.pop(), ['fortFileSize != (int)-1', '\\Ns\\O\\_p_packets.cpp', 0x672]);
    assert.equal(wire(fort), (0xffff + 0x11) & 0xffff);
    // Player(0x2b): 선언 길이 = 이름 길이 BYTE + 0x3b, 0xff 이면 진단.
    const player = new Uint8Array(64);
    player[0x33] = 8;
    observeMissionClientPacket(0x2b, player, 1, assertion);
    assert.equal(wire(player), 0x43);
    player[0x33] = 0xff;
    observeMissionClientPacket(0x2b, player, 1, assertion);
    assert.deepEqual(seen.pop(), ['nameLen != (unsigned char)-1', '\\Ns\\O\\_p_packets.cpp', 0x695]);
    // Options(0x67): 선언 길이 = dataLen WORD + 7, 0xffff 이면 진단.
    const options = new Uint8Array(16);
    options[5] = 0x57;
    observeMissionClientPacket(0x67, options, 1, assertion);
    assert.equal(wire(options), 0x5e);
    options[5] = 0xff;
    options[6] = 0xff;
    observeMissionClientPacket(0x67, options, 1, assertion);
    assert.deepEqual(seen.pop(), ['dataLen != (unsigned short)-1', '\\Ns\\O\\_p_packets.cpp', 0xa07]);
    // SP(0x81): 선언 길이 = ((count WORD + 2) & 0xffff) * 4 의 낮은 WORD, -1 이면 진단.
    const spPacket = new Uint8Array(16);
    spPacket[4] = 9;
    observeMissionClientPacket(0x81, spPacket, 1, assertion);
    assert.equal(wire(spPacket), 44);
    put(spPacket, 4, 0xffffffff);
    observeMissionClientPacket(0x81, spPacket, 1, assertion);
    assert.deepEqual(seen.pop(), ['numPlayers != (int)-1', '\\Ns\\O\\_p_packets.cpp', 0xc3d]);
    assert.equal(wire(spPacket), ((0xffff + 2) & 0xffff) * 4 & 0xffff);
    // 나머지 종류는 무효과이고, 진단 장치가 없으면 도달한 위치에서 거부한다.
    const other = new Uint8Array(16).fill(5);
    observeMissionClientPacket(0x28, other, 1, assertion);
    assert.ok(other.every((byte) => byte === 5));
    const bad = new Uint8Array(16);
    put(bad, 0xc, 0xffffffff);
    assert.throws(() => observeMissionClientPacket(0x2a, bad, 1, undefined), /소비자 미연결/);
    assert.throws(() => observeMissionClientPacket(0x2a, new Uint8Array(8), 1, assertion), RangeError);
});

// 67h 공통 부모: 길이 진단 복귀 뒤 현재 길이를 다시 읽어 DWORD 단위→나머지 BYTE 복사, 선택 CS → 범위 → 비용(float) → 비율 → 선택 재접속 순서이며 87바이트 밖 복사는 앞선 쓰기를 남기고 거부한다. (2026-11-11 WB04-E314)
test('미션 클라이언트 패킷: 옵션 수신의 복사·순서·미연결 소비자', () => {
    const log = [];
    const options = new Uint8Array(87).fill(0xa5);
    const globals = { options, challenge: 0, battle: true, mission: false, saved: true, setValue: (value) => { log.push(`value,${value}`); } };
    const hooks = {
        assertion: (text) => { log.push(`assert,${text}`); },
        challengeOptions: (sender) => { log.push(`challenge,${sender}`); },
        range: () => { log.push('range'); },
        cost: () => { log.push('cost'); return 1251; },
        storeCost: (value) => { log.push(`store,${value}`); },
        currentValue: () => { log.push('current'); return 17; },
        reconnect: () => { log.push('reconnect'); },
    };
    const packet = (length, body = 0x70) => { const bytes = new Uint8Array(7 + 100).fill(body); bytes[5] = length & 0xff; bytes[6] = length >> 8; return bytes; };
    // 길이 7: DWORD 한 번 + 나머지 3 BYTE 를 복사하고 미복사 꼬리(0xa5)는 보존한다.
    receiveStateBattleOptions(3, packet(7), globals, hooks);
    assert.deepEqual(Array.from(options.subarray(0, 9)), [0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0xa5, 0xa5]);
    assert.deepEqual(log, ['range', 'cost', 'store,1251', 'current', 'value,17', 'reconnect']);
    // CS 가 있으면 range 앞에서 자식을 부르고, 전투 아님/미션/미저장이면 재접속이 없다.
    log.length = 0;
    globals.challenge = 1;
    globals.mission = true;
    receiveStateBattleOptions(9, packet(0), globals, hooks);
    assert.deepEqual(log, ['challenge,9', 'range', 'cost', 'store,1251', 'current', 'value,17']);
    // 길이 88 은 진단이 복귀해도 87바이트 밖 쓰기를 거부하고, 앞선 DWORD 복사는 남긴다.
    log.length = 0;
    options.fill(0xa5);
    assert.throws(() => receiveStateBattleOptions(1, packet(88, 0x31), globals, hooks), RangeError);
    assert.deepEqual(log, ['assert,z->dataLen <= sizeof(globalBattleOptions)']);
    assert.ok(options.subarray(0, 84).every((byte) => byte === 0x31) && options.subarray(84).every((byte) => byte === 0xa5));
    // 미연결 소비자는 도달한 위치에서 거부한다: 앞선 복사는 남는다.
    options.fill(0xa5);
    assert.throws(() => receiveStateBattleOptions(1, packet(4, 0x42), globals, { ...hooks, range: undefined }), /소비자 미연결/);
    assert.deepEqual(Array.from(options.subarray(0, 5)), [0x42, 0x42, 0x42, 0x42, 0xa5]);
});

// cpppj `MissionClientPackets_RequiredEffectsAndRegistration`: 미등록 소비자와 잘린 입력을 수신 완료로 바꾸지 않고 앞서 쓴 옵션을 보존하며, 8종 모두 같은 등록을 가진다. (2026-11-11 WB04-E314)
test('미션 클라이언트 패킷: 필수 효과 누락과 8종 등록', () => {
    const packet = hexBytes('0167080000010077');
    const scene = new ClientPacketScene('103,1,1,1,1,1,0,0,9'.split(','), packet);
    const missing = new RawMissionClientPackets(scene.players, scene.participation, scene.sp, scene.state(), { ...scene.hooks(), updateRange: undefined });
    assert.throws(() => missing.receive(9, new PacketConnection(), packet), /소비자 미연결/);
    assert.deepEqual([scene.options[0], scene.option], [0x77, 10]);
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    registerMissionClientPackets(dispatch, scene.packets);
    // 8종 모두 실제 수신기가 있으며 수신 observer 의 0 인자는 원본처럼 자연 반환한다.
    for (const type of [0x29, 0x67, 0x2b, 0x2a, 0x28, 0x81, 0x84, 0x6e]) {
        const entry = dispatch.registration(type);
        assert.deepEqual([entry.version, entry.mismatch, typeof entry.receive, typeof (entry.observer ?? entry.packetObserver)], [1, 0, 'function', 'function'], `타입 ${type}`);
    }
    // 가변 길이 4종만 mutable 송신 observer(packetObserver)이고 나머지는 빈 observer 이다.
    for (const type of [0x2a, 0x2b, 0x67, 0x81]) assert.equal(typeof dispatch.registration(type).packetObserver, 'function', `packetObserver ${type}`);
    for (const type of [0x29, 0x28, 0x84, 0x6e]) assert.equal(typeof dispatch.registration(type).observer, 'function', `observer ${type}`);
    // 잘린 Fort 입력은 수신 완료로 바꾸지 않고 거부한다.
    assert.throws(() => scene.packets.receive(9, new PacketConnection(), Uint8Array.of(1, 0x29, 34, 0)), RangeError);
    // 알 수 없는 종류와 고정 진입값은 거부한다.
    assert.throws(() => scene.packets.receive(9, new PacketConnection(), Uint8Array.of(1, 0x55, 4, 0)), TypeError);
    assert.throws(() => scene.packets.receiveFixed(0x12345678, 9, packet), TypeError);
});

// 구성: 물리 크기와 hidden 이 옵션 87바이트의 +0x3e 인 같은 메모리여야 한다. (2026-11-11 WB04-E314)
test('미션 클라이언트 패킷: 구성의 물리 크기 검사', () => {
    const scene = new ClientPacketScene('1,0,0,0,0,1,0,0,9'.split(','), Uint8Array.of(1, 0x28, 4, 0));
    const state = scene.state();
    const make = (players, overrides) => new RawMissionClientPackets(players, scene.participation, scene.sp, { ...state, ...overrides }, {});
    assert.doesNotThrow(() => make(scene.players, {}));
    assert.throws(() => make(new Uint8Array(1619), {}), RangeError);
    assert.throws(() => make(scene.players, { cachedCoreData: new Uint8Array(9) }), RangeError);
    assert.throws(() => make(scene.players, { zoneAddress: new Uint8Array(5) }), RangeError);
    assert.throws(() => make(scene.players, { battleOptions: new Uint8Array(86) }), RangeError);
    assert.throws(() => make(scene.players, { hidden: new Uint8Array(9) }), RangeError);
    assert.throws(() => make(scene.players, { hidden: scene.options.subarray(0x3d, 0x3d + 9) }), RangeError);
});

// 원본 관측 fixture 가 닿지 않는 경계(변형 확인에서 확인): 비용 float 반올림, LoginBegin/Fort/SP 범위, 공유 seed 상자, 로컬 Fort 설치, 짧은 Fort 이미지, flags 0x40 보존, CoreData 복원 조건, Reply 기본 경로, signed SP count. (2026-11-11 WB04-E314)
test('미션 클라이언트 패킷: fixture 밖 경계와 분기', () => {
    const packetOf = (type, size, set = {}) => {
        const bytes = new Uint8Array(size);
        bytes[0] = 1;
        bytes[1] = type;
        bytes[2] = size & 0xff;
        bytes[3] = size >> 8;
        for (const [offset, value] of Object.entries(set)) put(bytes, Number(offset), value);
        return bytes;
    };
    const make = (values = '1,0,0,0,1,1,0,0,9', packet = packetOf(0x28, 20)) => new ClientPacketScene(values.split(','), packet);
    // 옵션 비용은 float 으로 반올림해 저장한다: 16777217 → 16777216.
    const options = make();
    const stored = [];
    options.options.fill(0);
    receiveStateBattleOptions(1, packetOf(0x67, 7), { options: options.options, challenge: 0, battle: false, mission: false, saved: false, setValue: () => {} }, {
        assertion: () => {}, challengeOptions: () => {}, range: () => {}, cost: () => 16777217, storeCost: (value) => { stored.push(value); }, currentValue: () => 0, reconnect: () => {},
    });
    assert.deepEqual(stored, [16777216]);
    // LoginBegin: Player 번호 9 이상은 거부하고, 공유 전투 seed 상자를 빌리면 그곳에 int32 로 쓴다(자기 필드는 그대로).
    const begin = make('1,0,0,0,1,1,0,0,9', packetOf(0x29, 40, { 4: 9 }));
    assert.throws(() => begin.packets.receive(9, new PacketConnection(), begin.packet), RangeError);
    const shared = make('1,0,0,0,1,1,0,0,9', packetOf(0x29, 40, { 4: 2, 30: 0xfffffff0 }));
    const box = { value: 0 };
    const sharedState = shared.state();
    sharedState.sharedBattleSeed = box;
    new RawMissionClientPackets(shared.players, shared.participation, shared.sp, sharedState, shared.hooks()).receive(9, new PacketConnection(), shared.packet);
    assert.deepEqual([box.value, shared.battleSeed], [-16, 8]);
    // Fort: Player 번호 9 이상 거부, 로컬 Fort(미션 후처리가 아닐 때)는 설치 장치를 부르고, 잘린 이미지는 읽기/배치 전에 거부한다.
    const badFort = make('1,0,0,0,1,1,0,0,9', packetOf(0x2a, 40, { 4: 9 }));
    assert.throws(() => badFort.packets.receive(9, new PacketConnection(), badFort.packet), RangeError);
    const local = make('1,1,0,0,1,1,0,0,9', packetOf(0x2a, 40, { 4: 1 }));
    local.packets.hooks.localFort = (owner) => { local.trace.push(`Local,${owner}`); };
    local.packets.receive(9, new PacketConnection(), local.packet);
    assert.deepEqual(local.trace, ['Local,1']);
    const mission = make('1,1,0,1,1,1,0,0,9', packetOf(0x2a, 40, { 4: 1 }));
    mission.packets.hooks.localFort = () => { mission.trace.push('Local'); };
    mission.packets.receive(9, new PacketConnection(), mission.packet);
    assert.deepEqual(mission.trace, []);
    const shortImage = make('1,0,0,0,1,1,0,0,9', packetOf(0x2a, 20, { 4: 2, 12: 4 }));
    shortImage.hidden.fill(0);
    assert.throws(() => shortImage.packets.receive(9, new PacketConnection(), shortImage.packet), RangeError);
    assert.ok(!shortImage.trace.includes('Build'));
    const shortHeader = make('1,0,0,0,1,1,0,0,9', Uint8Array.of(1, 0x2a, 16, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
    shortHeader.hidden.fill(0);
    assert.throws(() => shortHeader.packets.receive(9, new PacketConnection(), shortHeader.packet), RangeError);
    // Player: 이미 있는 flags 의 0x40 은 보존하고 패킷 flags 를 OR 한다. 구독자가 0 이면 CoreData 복원 장치를 부르지 않는다.
    const playerPacket = packetOf(0x2b, 0x50, { 0x26: 0, 0x2a: 0x101, 0x22: 3 });
    playerPacket[0x32] = 2;
    playerPacket[0x33] = 3;
    playerPacket.set([0x41, 0x42, 0], 0x3b);
    const player = make('1,0,0,0,1,1,0,0,9', playerPacket);
    put(player.players, 2 * 180 + 0x50, 0x240);
    player.packets.receive(9, new PacketConnection(), player.packet);
    assert.equal(player.players[2 * 180 + 0x50], 0x41);
    assert.equal(player.players[2 * 180 + 0x51], 0x01);
    assert.ok(!player.trace.some((entry) => entry.startsWith('Restore')));
    // 구독자가 0 이 아니면 (미션 후처리가 아닐 때) 복원한다.
    const restored = make('1,0,0,0,1,1,0,0,9', packetOf(0x2b, 0x50, { 0x26: 5 }));
    restored.packet[0x32] = 2;
    restored.packet.set([0x41, 0], 0x3b);
    restored.packets.receive(9, new PacketConnection(), restored.packet);
    assert.ok(restored.trace.includes('Restore,2'));
    // Reply: 정상 버전이고 phase 가 10/16 이 아니면 State 를 요청하지 않고 틱을 0, 다음 상태를 3 으로 둔다.
    const reply = make('1,0,0,0,1,1,4,0,9', packetOf(0x28, 20, { 4: 0x4e000a }));
    assert.equal(reply.phase, 9);
    reply.packets.receive(9, new PacketConnection(), reply.packet);
    assert.deepEqual([reply.ticks, reply.pending, reply.trace.includes('State,13')], [0, 3, false]);
    // SP: 개수가 음수(signed)면 아무것도 하지 않고, 소유자 0 은 로컬이 아니면 저장하지 않는다.
    const spNegative = make('1,0,0,0,1,1,0,0,9', packetOf(0x81, 20, { 4: 0xffffffff }));
    const before = spNegative.spText();
    spNegative.packets.receive(9, new PacketConnection(), spNegative.packet);
    assert.equal(spNegative.spText(), before);
    const spZero = make('1,0,0,0,0,3,0,0,9', packetOf(0x81, 24, { 4: 2, 8: 5, 12: 6 }));
    spZero.packets.receive(9, new PacketConnection(), spZero.packet);
    assert.deepEqual([spZero.sp.get(0), spZero.sp.get(1)], [floatBits(900), floatBits(6)]);
});

// Ready 의 boss 서버 경로와 시간 응답의 signed 64비트 차, 분배기를 통한 sender 전달(원본 관측 밖 계약). (2026-11-11 WB04-E314)
test('미션 클라이언트 패킷: Ready boss 경로·시간 응답·분배기 sender', () => {
    const bossScene = (loginEnd, sender) => {
        const scene = new ClientPacketScene(['1', '1', '1', '0', '1', '1', '0', '0', String(sender)], Uint8Array.of(1, 0x6e, 8, 0, 0, 0, 0, 0));
        scene.loginEnd = loginEnd;
        const hooks = { ...scene.hooks(), sendReady: (target) => { scene.trace.push(`Ready,${target}`); } };
        scene.bossPackets = new RawMissionClientPackets(scene.players, scene.participation, scene.sp, scene.state(), hooks);
        return scene;
    };
    // boss: 보낸 owner 에 0x40 을 켜고 전체 분배를 요청하며, 로그인 타이머가 0 이면 시작 응답(0x6e, 8, 0x80000000)을 보낸다. sender 9 는 자연 반환이다.
    const started = bossScene(0, 3);
    started.bossPackets.receive(3, new PacketConnection(), started.packet);
    assert.equal(started.players[3 * 180 + 0x50] & 0x40, 0x40);
    assert.deepEqual(started.trace, ['Ready,0', 'Send,3,016e080000000080']);
    const running = bossScene(30, 4);
    running.bossPackets.receive(4, new PacketConnection(), running.packet);
    assert.deepEqual(running.trace, ['Ready,0']);
    const natural = bossScene(0, 9);
    natural.bossPackets.receive(9, new PacketConnection(), natural.packet);
    assert.deepEqual(natural.trace, []);
    // 시간 응답: 기준 시계 = (clock(signed) − 틱(signed)*1000) 의 낮은 DWORD 이다.
    const timeScene = (clock, ticks) => {
        const scene = new ClientPacketScene('1,0,0,0,1,1,0,0,9'.split(','), Uint8Array.of(1, 0x84, 8, 0, ticks & 0xff, (ticks >>> 8) & 0xff, (ticks >>> 16) & 0xff, (ticks >>> 24) & 0xff));
        const hooks = { ...scene.hooks(), clock: () => clock };
        new RawMissionClientPackets(scene.players, scene.participation, scene.sp, scene.state(), hooks).receive(9, new PacketConnection(), scene.packet);
        return scene;
    };
    assert.deepEqual([timeScene(7501, 5).clockStamp, timeScene(7501, 5).baseClock], [7501, 2501]);
    assert.equal(timeScene(0x80000001, 3).baseClock, Number(BigInt.asUintN(32, BigInt(-0x7fffffff) - 3000n)));
    assert.equal(timeScene(100, 0xfffffff0).baseClock, 100 + 16000);
    // 분배기를 통한 수신: 연결의 sender 가 그대로 전달된다(0x67 옵션의 CS 자식 호출로 확인).
    const scene = new ClientPacketScene('1,0,0,0,1,1,1,0,9'.split(','), Uint8Array.of(1, 0x67, 7, 0, 0, 0, 0));
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    registerMissionClientPackets(dispatch, scene.packets);
    assert.equal(dispatch.dispatch(new PacketConnection({ sender: 4 }), Uint8Array.of(1, 0x67, 7, 0, 0, 0, 0)), true);
    assert.ok(scene.trace.includes('Outpost,4'));
});
