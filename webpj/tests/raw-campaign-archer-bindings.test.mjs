// cpppj RawCampaignArcherBindings의 조립 계약을 보호 TYPE/SHP 자료와 실제 단일 World/Kernel로 검증한다. 전용 원본 조립 fixture는 없다. (2026-11-16 WB04-E567)
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { CampaignArcherScene } from './campaign-archer-support.mjs';
import { GunAnimationFields, GunAnimationReceiveKind, initializeReceivedGunAnimation, writeGunAnimationProcessData } from '../www/core/raw-gun-animation.js';
import { ArcherAnimationFields } from '../www/core/raw-archer-animation.js';
import { GameRandom } from '../www/core/random.js';
import { createGunProcess } from '../www/core/raw-gun-process.js';
import { GunAnimationProcess } from '../www/core/raw-gun-animation.js';
import { ArcherAnimationProcess } from '../www/core/raw-archer-animation.js';
import { CannonFireProcess } from '../www/core/raw-cannon-fire.js';
import { CrossbowAnimationProcess } from '../www/core/raw-crossbow-animation.js';
import { put, get } from './raw-support.mjs';

// 실제 지면 위 부모와 그 Gun Form을 한 월드에 설치한다. (2026-11-16 WB04-E567)
function scene(type = 128) {
    // 실제 World/Core/Host와 보호 자산을 공유하는 시험 장면이다.
    const s = new CampaignArcherScene(); s.ground(20, 20, 26, 26);
    // 원문 decoder가 반환한 실제 부모 번호이다.
    const parent = s.pop(type, 23, 23, 3, 1); const gun = s.gunFor(parent);
    return { s, parent, gun };
}

// 실제 부모 종속 체인의 첫 애니메이션을 현재 PID로 조회한다. (2026-11-16 WB04-E567)
function animation(s, parent) { return s.core.processes.fromForm(get(s.core.pool.slot(parent), 6, 2)); }

// 수신 전용 생성자와 공통 writer로 실제 전체 패킷을 만든다. (2026-11-16 WB04-E567)
function fullPacket(kind, parent, gun, form = 23000, time = 9.25) {
    // 해당 실제 파생의 원문 필드 이미지이다.
    const fields = kind === GunAnimationReceiveKind.ArcherFire || kind === GunAnimationReceiveKind.ArcherRotate ? new ArcherAnimationFields() : new GunAnimationFields();
    initializeReceivedGunAnimation(fields.bytes, kind); put(fields.bytes, 4, form); put(fields.bytes, 8, parent); put(fields.bytes, 42, gun, 2); put(fields.bytes, 44, 0, 2);
    new DataView(fields.bytes.buffer).setFloat64(48, time, true);
    // 같은 현재 버퍼의 작성 범위 보기이다.
    const packet = new Uint8Array(64).fill(0xcc);
    // 실제 발사 프로세스 또는 현재 파생 선택이다.
    const fire = kind === GunAnimationReceiveKind.SunFire || kind === GunAnimationReceiveKind.ArcherFire;
    // 실제 writer가 반환한 작성 길이이다.
    const length = writeGunAnimationProcessData(fields.bytes, packet, true, true, fire ? 0x3b : 0x3c, 1);
    return packet.subarray(0, length);
}

test('CampaignArcher: 다른 표적 Pool 거부, 현재 장기 Sun/Wind TYPE와 같은 처리기 Pool 대여', () => {
    // Pool 대여를 검사할 첫 실제 장면이다.
    const a = new CampaignArcherScene({ combat: false }); const b = new CampaignArcherScene({ combat: false });
    assert.throws(() => a.makeCombat({ target: b.gunTarget }), /Pool 불일치/);
    assert.equal(a.combat.pool, a.core.pool); assert.equal(a.combat.animation.pool, a.core.pool); assert.equal(a.combat.missile.pool, a.core.pool);
    assert.equal(a.archerState.currentType.value, 71);
    a.world.storage.gameAux.sunArcherType = 72; assert.equal(a.combat.animation.type, 72);
    assert.equal(a.world.storage.gameAux.sunCannonType, 128); assert.equal(a.world.storage.gameAux.windArcherType, 140);
});

test('CampaignArcher: 실제 부모 6종의 단계11개를 올바른 애니메이션·Form TYPE·현재 시각으로 분배한다', () => {
    // 각 파생에 실제 부모/Gun을 만들고 Kernel을 실행하기 전에 생성 계약만 대조한다.
    for (const [type, stages] of [[71, [[2, 0x50b450], [5, 0x50b1c0]]], [128, [[2, 0x50b260], [5, 0x50b318], [8, 0x50b2c8]]], [131, [[2, 0x50b260], [5, 0x50b370]]], [140, [[2, 0x50b450], [5, 0x50b120]]], [148, [[5, 0x50b170]]], [129, [[5, 0x50b3f0]]]]) {
        const { s, parent, gun } = scene(type);
        s.combat.prepareFrame({ game: 12.3456789, delta: 0.123456789 });
        // 같은 부모의 각 단계가 별도의 실제 Form을 생성한다.
        for (const [stage, table] of stages) {
            s.gunHooks.startAnimation(stage, gun.form, 0);
            // 현재 Form PID로 조회한 실제 파생 프로세스이다.
            const process = animation(s, parent); const fields = process.fields.common ?? process.fields;
            assert.deepEqual([get(fields.bytes, 0), fields.parent, fields.gunForm, fields.time, s.core.pool.slot(process.form)[10]], [table, parent, gun.form, 12.3456789, stage === 5 ? 35 : 36], `TYPE${type} stage${stage}`);
        }
        assert.equal(s.archerState.delta, 0.123456789); assert.equal(s.missileState.now, 12.3456789);
    }
});

test('CampaignArcher: PrepareFrame의 불량 시각은 앞선 상태를 바꾸지 않으며 모든 파생이 다음 현재 시각을 사용한다', () => {
    const { s, gun, parent } = scene();
    s.combat.prepareFrame({ game: 3.25, delta: 0.125 });
    // 검증 실패는 어떤 공유 시각에도 쓰지 않는다.
    for (const time of [{ game: NaN, delta: 0 }, { game: 2, delta: Infinity }, { game: 2, delta: -1 }]) assert.throws(() => s.combat.prepareFrame(time), /clock/);
    assert.deepEqual([s.archerState.now, s.archerState.delta, s.missileState.now], [3.25, 0.125, 3.25]);
    s.gunHooks.startAnimation(5, gun.form, 0); assert.equal(animation(s, parent).fields.time, 3.25);
    s.combat.prepareFrame({ game: -5.5, delta: 0 }); s.gunHooks.startAnimation(8, gun.form, 0); assert.equal(animation(s, parent).fields.time, -5.5);
});

test('CampaignArcher: 다른 부모/단계 fallback은 값 사본이며 실제 Gun 조회 실패·생성 실패는 명시 오류다', () => {
    const { s, parent, gun } = scene(); const calls = [];
    // 호출자가 제공한 참조/함수를 값 사본으로 묶은 인수이다.
    const input = { startAnimation: (...args) => calls.push(args), marker: 7 };
    // 실제 현재 TYPE의 애니메이션 공장으로 연결한 함수 묶음이다.
    const hooks = s.combat.gunHooks(input); input.startAnimation = () => { throw new Error('입력 재대입 누출'); };
    hooks.startAnimation(9, gun.form, 17); assert.deepEqual(calls, [[9, gun.form, 17]]); assert.equal(hooks.marker, 7);
    assert.throws(() => s.combat.gunHooks().startAnimation(9, gun.form, 17), /미복원 TYPE128 stage9/);
    assert.throws(() => hooks.startAnimation(2, parent, 0), /GunProcess/);
    // 실제 공장까지 초기화한 뒤의 부착 거부를 성공으로 숨기지 않는다.
    const attach = s.core.processes.attach.bind(s.core.processes); s.core.processes.attach = () => null;
    assert.throws(() => hooks.startAnimation(5, gun.form, 0), /부착 실패/); s.core.processes.attach = attach;
});

test('CampaignArcher: 네 수신 생성자는 명시 Form/부모·현재 process TYPE·패킷 시각을 보존한다', () => {
    // 원문의 네 기본 생성자만 수신으로 생성한다(ArcherRotate는 Wind 전용이다).
    for (const [type, message, kind, table] of [[71, 0x3b, GunAnimationReceiveKind.ArcherFire, 0x50b1c0], [128, 0x3b, GunAnimationReceiveKind.SunFire, 0x50b318], [128, 0x3c, GunAnimationReceiveKind.SunRotate, 0x50b260], [140, 0x3c, GunAnimationReceiveKind.ArcherRotate, 0x50b450]]) {
        const { s, parent, gun } = scene(type); const packet = fullPacket(kind, parent, gun.form);
        s.combat.prepareFrame({ game: 100, delta: 0.5 }); s.world.storage.gameAux.fireAnimationProcessType = 37; s.world.storage.gameAux.rotateAnimationProcessType = 38;
        s.combat.receiveAnimationPacket(message, packet);
        // 현재 Form PID로 조회한 실제 파생 프로세스이다.
        const process = s.core.processes.fromForm(23000);
        assert.deepEqual([process.form, process.parent, process.fields.gunForm, process.fields.time, get(process.fields.bytes, 0), s.core.pool.slot(23000)[10]], [23000, parent, gun.form, 9.25, table, message === 0x3b ? 37 : 38]);
        if (kind === GunAnimationReceiveKind.ArcherFire) assert.equal(get(process.fields.bytes, 68), 0x43340000);
    }
});

test('CampaignArcher: 수신 현재 Sun/Wind TYPE 변경을 다음 공장 선택과 일반 생성이 같이 읽는다', () => {
    const { s, parent, gun } = scene(); const aux = s.world.storage.gameAux;
    aux.sunCannonType = 127; s.core.pool.physical(parent)[10] = 127;
    s.gunHooks.startAnimation(2, gun.form, 0); assert.equal(get(animation(s, parent).fields.bytes, 0), 0x50b260);
    s.combat.receiveAnimationPacket(0x3b, fullPacket(GunAnimationReceiveKind.SunFire, parent, gun.form));
    assert.equal(get(s.core.processes.fromForm(23000).fields.bytes, 0), 0x50b318);
    aux.windArcherType = 127; aux.sunCannonType = 128;
    s.combat.receiveAnimationPacket(0x3c, fullPacket(GunAnimationReceiveKind.ArcherRotate, parent, gun.form, 23001));
    assert.equal(get(s.core.processes.fromForm(23001).fields.bytes, 0), 0x50b450);
});

test('CampaignArcher: live 수신은 같은 PID의 선택 필드만 바꾸고 생성·부모/시각 초기화를 반복하지 않는다', () => {
    const { s, parent, gun } = scene(); s.combat.receiveAnimationPacket(0x3c, fullPacket(GunAnimationReceiveKind.SunRotate, parent, gun.form));
    // 현재 Form PID로 조회한 실제 파생 프로세스이다.
    const process = s.core.processes.fromForm(23000); const fields = process.fields; const before = Uint8Array.from(fields.bytes); const count = s.core.kernel.size;
    // 선택 필드만 포함한 실제 수신 패킷이다.
    const update = new Uint8Array(11); put(update, 4, 23000); update[8] = 4; put(update, 9, 0xabcd, 2);
    s.combat.prepareFrame({ game: 80, delta: 0.5 }); s.combat.receiveAnimationPacket(0x3b, update);
    assert.equal(s.core.processes.fromForm(23000), process); assert.equal(s.core.kernel.size, count); assert.equal(fields.target, 0xabcd); assert.equal(fields.time, 9.25); assert.equal(fields.parent, parent);
    before.set(update.subarray(9), 44); assert.deepEqual(fields.bytes, before);
    // 진단 복귀 뒤 현재 Form PID를 재조회한다.
    s.combat.receiveAnimationPacket(0x3c, fullPacket(GunAnimationReceiveKind.SunRotate, parent, gun.form, 23001));
    // 진단 복귀 뒤 현재 PID로 선택할 두 번째 실제 프로세스이다.
    const second = s.core.processes.fromForm(23001); const diagnostics = [];
    s.world.storage.bindDisplayDiagnostics({ value: '' }, (...args) => { diagnostics.push(args); put(s.core.pool.physical(23000), 18, second.pid); });
    // 부모 dirty 진단을 발생시키는 현재 전체 패킷이다.
    const parentUpdate = fullPacket(GunAnimationReceiveKind.SunRotate, parent, gun.form, 23000, 22.5);
    s.combat.receiveAnimationPacket(0x3c, parentUpdate);
    assert.equal(second.fields.time, 22.5); assert.equal(process.fields.time, 9.25); assert.equal(diagnostics[0][2], 0x4d);
});

test('CampaignArcher: 수신 잘못된 머리/메시지/DWORD SID·선택 필드 부족과 원문 생성 진단을 보존한다', () => {
    const { s, parent, gun } = scene(); const packet = fullPacket(GunAnimationReceiveKind.SunFire, parent, gun.form); const calls = [];
    s.world.storage.bindDisplayDiagnostics({ value: '' }, (...args) => calls.push(args));
    assert.throws(() => s.combat.receiveAnimationPacket(0x3a, packet), /메시지/);
    assert.throws(() => s.combat.receiveAnimationPacket(0x3b, packet.subarray(0, 8)), /머리/);
    // Form과 부모 모두 DWORD 전체를 검사하고 WORD 절단으로 유효 번호를 만들지 않는다.
    for (const [offset, value] of [[4, 0], [4, 65536], [4, s.core.pool.capacity], [9, 65536]]) { const invalid = Uint8Array.from(packet); put(invalid, offset, value); assert.throws(() => s.combat.receiveAnimationPacket(0x3b, invalid), /Form0|SID 물리/); }
    assert.throws(() => s.combat.receiveAnimationPacket(0x3b, packet.subarray(0, 12)), /DWORD 부족/);
    // 복귀 가능한 dirty 진단을 선택할 패킷이다.
    const nonFull = Uint8Array.from(packet); nonFull[8] = 0xfe;
    // 원문 진단은 복귀 가능하다. 진단 장치가 보정한 현재 dirty를 다음 decoder가 읽는다.
    s.world.storage.bindDisplayDiagnostics({ value: '' }, (...args) => { calls.push(args); nonFull[8] = 0xff; });
    s.combat.receiveAnimationPacket(0x3b, nonFull); assert.equal(calls.at(-1)[2], 0x42);
    s.world.storage.bindDisplayDiagnostics({ value: '' }, (...args) => calls.push(args));
    put(packet, 4, 23001);
    s.core.pool.physical(parent)[10] = 131;
    assert.throws(() => s.combat.receiveAnimationPacket(0x3b, packet)); assert.equal(calls.at(-1)[2], 0x23b);
    assert.throws(() => s.combat.receiveAnimationPacket(0x3c, packet)); assert.equal(calls.at(-1)[2], 0x248);
});

test('CampaignArcher: 현재 VT/헤더·dirty·force writer와 packet/송신은 같은 장기 버퍼를 사용한다', () => {
    const { s, parent, gun } = scene(); s.gunHooks.startAnimation(2, gun.form, 0);
    // 현재 Form PID로 조회한 실제 파생 프로세스이다.
    const process = animation(s, parent); const aux = s.world.storage.gameAux; const sent = [];
    // 같은 버퍼/현재 선택 표식을 사용하는 전송 훅이다.
    const wire = s.combat.transmission((packet) => sent.push(packet));
    aux.rotateAnimationHeader.set([0x51, 3]);
    // 실제 writer가 반환한 작성 길이이다.
    const length = wire.writeData(process, true); const packet = wire.packet(length);
    assert.equal(packet.buffer, aux.squidTransmission.buffer.buffer); assert.deepEqual([...packet.subarray(0, 2)], [3, 0x51]); assert.equal(packet[8], 0xff); assert.equal(process.fields.bytes[40], 0);
    wire.transmit(length); assert.equal(sent[0].buffer, packet.buffer);
    assert.equal(wire.writeData(process, false), 9); assert.equal(wire.packet(9)[8], 0);
    aux.rotateAnimationHeader.set([0x52, 4]); const full = wire.writeFullData(process, 0x80000000); assert.equal(full, 25); assert.deepEqual([...wire.packet(full).subarray(0, 2)], [4, 0x52]);
    assert.throws(() => wire.packet(-1), RangeError); assert.throws(() => wire.packet(2049), RangeError); assert.throws(() => wire.transmit(-1), RangeError); assert.throws(() => wire.transmit(2049), RangeError);
    aux.squidTransmission.buffer[0] = 0x99; assert.equal(packet[0], 0x99);
    s.gunHooks.startAnimation(5, gun.form, 0); const fire = animation(s, parent); aux.fireAnimationHeader.set([0x61, 5]);
    // 발사 writer가 현재 버퍼에 작성한 길이이다.
    const fireLength = wire.writeData(fire, true); assert.deepEqual([...wire.packet(fireLength).subarray(0, 2)], [5, 0x61]);
});

test('CampaignArcher: 다섯 실제 파생의 writer 선택과 다른 프로세스 fallback/누락을 분리한다', () => {
    // 파생 공통 필드는 같은 writer가 읽고 Ice는 80BYTE 중 common64만 쓴다.
    for (const type of [71, 128, 131, 140, 148, 129]) { const { s, parent, gun } = scene(type); s.gunHooks.startAnimation(5, gun.form, 0); const wire = s.combat.transmission(() => {}); const length = wire.writeFullData(animation(s, parent), 1); assert.equal(wire.packet(length)[1], 0x3b); }
    const { s, gun, parent } = scene(); const calls = []; const bytes = Uint8Array.of(7);
    // 다른 프로세스의 원래 함수 사본이다.
    const fallback = { writeData: (p, changed) => { calls.push([p, changed]); return 1; }, writeFullData: (p, force) => { calls.push([p, force]); return 1; }, packet: () => bytes, transmit: (length) => calls.push(length) };
    // 같은 버퍼/현재 선택 표식을 사용하는 전송 훅이다.
    const wire = s.combat.transmission(null, fallback); fallback.packet = () => { throw new Error('fallback 재대입 누출'); };
    assert.equal(wire.writeData(gun, true), 1); assert.equal(wire.packet(1), bytes); wire.transmit(1); assert.equal(wire.writeFullData(gun, 17), 1); assert.deepEqual(calls, [[gun, true], 1, [gun, 17]]);
    // 누락된 선택 장치의 명시 오류를 검사할 전송 훅이다.
    const missing = s.combat.transmission(); assert.throws(() => missing.writeData(gun, true), /일반 writer/); assert.throws(() => missing.writeFullData(gun, 0), /전체 writer/); assert.throws(() => missing.packet(1), /버퍼 미연결/); assert.throws(() => missing.transmit(1), /송신 미연결/);
    s.gunHooks.startAnimation(2, gun.form, 0); missing.writeData(animation(s, parent), true); assert.throws(() => missing.transmit(25), /송신 장치/);
    // writer 선택은 인스턴스 전체 상태이며 다른 반환 훅도 직전 결과를 읽는다.
    assert.equal(wire.packet(25).buffer, s.world.storage.gameAux.squidTransmission.buffer.buffer);
});

test('CampaignArcher: 실제 Native 전송 장치 계약이 공급되면 같은 버퍼·force와 방송 길이를 넘긴다', () => {
    const { s, parent, gun } = scene(); const calls = []; s.gunHooks.startAnimation(2, gun.form, 0);
    // 현재 Form PID로 조회한 실제 파생 프로세스이다.
    const process = animation(s, parent); const wire = s.combat.transmission();
    // 실제 Storage 장기 장치 설치는 E 구획이며 여기서는 명시 공급된 경계의 인수·분배만 관찰한다.
    s.world.storage.usesNativeTransmissionDevices = () => true;
    s.world.storage.serializeProcessForm = (...args) => { calls.push(args); return 17; };
    s.world.storage.broadcastProcessForm = (length) => calls.push(length);
    assert.equal(wire.writeData(process, true), 17); assert.equal(wire.writeFullData(process, 0x80000000), 17); wire.transmit(17);
    assert.deepEqual(calls, [[process, s.world.storage.gameAux.squidTransmission.buffer, true, 0], [process, s.world.storage.gameAux.squidTransmission.buffer, false, 0x80000000], 17]);
});

test('CampaignArcher: 실제 Disc 예약과 외부 fallback을 같은 인수/반환으로 합성한다', () => {
    // 실제 World/Core/Host와 보호 자산을 공유하는 시험 장면이다.
    const s = new CampaignArcherScene(); s.world.load([]); const sid = s.create(83, 1); const calls = [];
    // Disc 처리와 외부 fallback을 합성한 실제 Regular 함수이다.
    const handler = s.combat.handler((...args) => { calls.push(args); return 37; });
    assert.equal(handler(sid, 12, 5, 9), 9); assert.equal(calls.length, 0);
    // 다른 TYPE의 실제 부모이다.
    const other = s.create(75, 1); assert.equal(handler(other, 11, 3, 8), 37); assert.deepEqual(calls, [[other, 11, 3, 8]]);
    assert.throws(() => s.combat.handler()(other, 11, 3, 8), /외부 Regular/);
});

test('CampaignArcher: 실제 Wind Archer 공유 회전은 게임 난수를 한 번 전진시키고 값으로 빌린 음향 함수를 사용한다', () => {
    const { s, parent, gun } = scene(140); const target = s.pop(75, 35, 23, 3, 2);
    // 동일 시드의 독립 원문 난수 계산과 실제 회전의 한 호출을 대조한다.
    const expected = new GameRandom(s.random.state); const roll = 1000 + expected.next(9000);
    s.combatEffects.animation.common.sound = () => { throw new Error('호출자 음향 재대입 누출'); };
    s.combat.prepareFrame({ game: 1, delta: 0 }); s.gunHooks.startAnimation(2, gun.form, target);
    // 현재 Form PID로 조회한 실제 파생 프로세스이다.
    const process = animation(s, parent); process.runFrame();
    assert.equal(s.random.state, expected.state);
    assert.ok(s.take().includes(`Archer 소리:${parent}:sunDiscThrowerRatchet0${Math.trunc(roll / 1000)}.wav`));
});

test('CampaignArcher: 실제 Sun Disc launch가 같은 World에 owner/포구/Pop·Disc Regular·Missile Form을 만든다', () => {
    const { s, parent, gun } = scene(71); const target = s.pop(75, 35, 23, 3, 2);
    s.combat.prepareFrame({ game: 2.5, delta: 0.125 }); s.gunHooks.startAnimation(5, gun.form, target);
    // 실제 발사 프로세스 또는 현재 파생 선택이다.
    const fire = animation(s, parent); const before = s.core.kernel.size;
    // 실제 추적이 저장한 원문 위치를 사용하고 동일 launch→실제 Missile 공장으로 이어 간다.
    fire.runFrame(); assert.equal(s.combat.animation.launch(fire.fields), true);
    // 실제 Kernel에 부착한 Missile 프로세스이다.
    const missile = s.core.kernel.get(s.core.kernel.size); const fields = missile.fields;
    assert.equal(fields.firing, parent); assert.equal(fields.target, target); assert.equal(s.core.pool.slot(fields.parent)[10], 83); assert.equal(s.core.pool.slot(fields.parent)[34], 1);
    assert.ok(s.core.kernel.size > before); assert.equal(missile.host, s.core.processes);
    assert.equal(s.combat.missile.pool, s.core.pool);
    // 이 보호 TYPE 둘에는 공통 firingSound가 없다. Archer 전용 발사 음향과 혼동하지 않는다.
    assert.deepEqual([s.missileTypes[71].firingSound, s.missileTypes[83].firingSound], [0, 0]);
    assert.equal(s.take().some((event) => event.startsWith('Missile 발사음:')), false);
});

test('CampaignArcher: 아이스 냉각과 다음 현재8DWORD 문맥은 실제 MayTarget·Gun 완료/삭제로 연결된다', () => {
    const { s, parent, gun } = scene(129); const target = s.pop(75, 35, 23, 3, 1); const contexts = [];
    // 호출자가 제공한 참조/함수를 값 사본으로 묶은 인수이다.
    const input = { shotContext: (sid) => { const context = s.queries.shotContext(sid); contexts.push(context); return context; } };
    // 실제 현재 TYPE의 애니메이션 공장으로 연결한 함수 묶음이다.
    const hooks = s.combat.gunHooks(input); input.shotContext = () => { throw new Error('호출자 문맥 재대입 누출'); };
    s.combat.prepareFrame({ game: 10, delta: 0 }); put(gun.fields.bytes, 28, 6); hooks.startAnimation(5, gun.form, target);
    // 실제 발사 프로세스 또는 현재 파생 선택이다.
    const fire = animation(s, parent); const form = fire.form;
    s.combat.prepareFrame({ game: 14, delta: 0.25 }); fire.runFrame(); assert.ok(s.take().includes(`아이스 냉각:${parent}`));
    s.combat.prepareFrame({ game: 14.25, delta: 0.25 }); const expected = s.queries.shotContext(parent); fire.runFrame();
    assert.deepEqual(contexts, [expected]); assert.equal(gun.fields.stage, 7); assert.equal(s.core.processes.fromForm(form), null);
});

test('CampaignArcher: 정상 SunArcher Gun은 방향0의 회전을 건너뛰고 강제 회전만 cpppj처럼 거부한다', () => {
    const { s, parent, gun } = scene(71); const target = s.pop(75, 35, 23, 3, 2);
    // 원문 group0이면서 Wind Archer가 아닌 TYPE71은 단계2에서 회전 생성 없이 단계4로 진행한다.
    assert.equal(s.animationTypes[71].directions, 0); put(gun.fields.bytes, 24, target, 2); put(gun.fields.bytes, 28, 2);
    const count = s.core.kernel.size; gun.runFrame(); assert.equal(gun.fields.stage, 4); assert.equal(s.core.kernel.size, count);
    // API로 원문 정상 경로 밖 회전을 강제했을 때도 임의 방향 수로 성공을 꾸미지 않는다.
    s.gunHooks.startAnimation(2, gun.form, target);
    assert.throws(() => animation(s, parent).runFrame(), /방향\/물리 프레임 자료 미공급/);
});

test('CampaignArcher: cpppj Sun/Thunder/Crossbow의 void 부모 공장·시계·동일 Host 정리 계약', () => {
    // CampaignFireTests의 세 factory loop를 옮긴다. 저장 미션/사제 선택 부분은 D/E Session 통합에서 재대조한다.
    for (const [type, origin, cases] of [[128, 40, [[2, GunAnimationProcess], [5, CannonFireProcess], [8, GunAnimationProcess]]], [140, 70, [[2, ArcherAnimationProcess], [5, CrossbowAnimationProcess]]], [131, 90, [[2, GunAnimationProcess], [5, CannonFireProcess]]]]) {
        // 실제 World/Core/Host와 보호 자산을 공유하는 시험 장면이다.
        const s = new CampaignArcherScene(); s.world.load([]);
        // 원문 decoder가 반환한 실제 부모 번호이다.
        const parent = s.create(type, 2); const target = s.create(158, 1);
        // 현재 Form의 실제 GunProcess이다.
        const gun = createGunProcess(s.core.processes, s.bindings.gun, parent, s.gunState.processType);
        // 실제 현재 TYPE의 애니메이션 공장으로 연결한 함수 묶음이다.
        const hooks = s.combat.gunHooks();
        // 각 공장은 같은 void 부모에 붙이고 실제 Host로 제거한 뒤 다음 시각을 시험한다.
        for (const [stage, Process] of cases) {
            // cpppj 해당 단계의 게임 시각과 실제 선택 표적이다.
            const now = origin + stage; const selected = stage === 8 ? 0 : target;
            s.combat.prepareFrame({ game: now, delta: 0.025 }); hooks.startAnimation(stage, gun.form, selected);
            // 현재 Form PID로 조회한 실제 파생 프로세스이다.
            const process = animation(s, parent); const fields = process.fields;
            assert.ok(process instanceof Process); assert.deepEqual([fields.parent, fields.form, fields.gunForm, fields.target, fields.time], [parent, process.form, gun.form, selected, now]);
            s.core.processes.kill(process, 0x10); assert.equal(s.core.kernel.size, 1);
        }
        assert.equal(s.core.bookkeeping.depth, 0);
    }
});
