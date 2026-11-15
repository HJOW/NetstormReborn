// cpppj CampaignMotionTests의 경로 진행 루프와 Motion/Path 조립 계약을 실제 World/Host/보호 TYPE/SHP로 검사한다. 저장 미션 전체 설치는 아직 Session 통합 몫이다. (2026-11-16 WB04-E568)
import test from 'node:test';
import assert from 'node:assert/strict';
import { CampaignMotionScene } from './campaign-motion-support.mjs';
import { RawPathRuntime } from '../www/core/raw-path-runtime.js';
import { RawPathProcess } from '../www/core/raw-path-process.js';
import { RawCampaignPathBindings } from '../www/core/raw-campaign-path-bindings.js';
import { RawPriestHalt } from '../www/core/raw-priest-halt.js';
import { RawContainedLookup } from '../www/core/raw-contained-lookup.js';
import { put, get, floatToBits } from './raw-support.mjs';

// 이동할 실제 사제와 넓은 연결 지면을 만든다. 경로 자체는 실제 BFS만 생성한다.
function scene(owner = 1) { const s = new CampaignMotionScene(); s.ground(20, 20, 36, 36); const priest = s.priest(23.5, 23.5, owner); return { s, priest }; }
// 원문 Start의 같은 명령 필드를 지정한다.
function request(parent, x = 30.5, y = 30.5) { return { parent, destination: { x, y }, helper: 0, tailWord: 0, tailByte: 0 }; }
// 실제 Path만 같은 clock으로 실행한다. 다른 전투/정기 과정은 cpppj 해당 경로 시험처럼 실행하지 않는다.
function finish(s, parent) { let frames = 0; while (s.motion.paths.currentPath(parent)) { s.core.processState.now += 1 / 60; s.motion.paths.currentPath(parent).runFrame(); if (++frames > 1200) throw new Error('실제 Path 완료 시간 초과'); } return frames; }

// Path 생성자의 같은 하위 몸체를 사용하며 한 입력만 바꾸기 위한 인수 묶음이다.
function pathInputs(s) {
    // 같은 실제 HP/Carrier/공간을 사용하는 부모 Halt이다.
    const halt = new RawPriestHalt(s.core.pool, s.core.hash, s.core.types, s.core.spots, s.priests.carrierCheck, s.priests.priestState, s.hp, s.motionState.halt, {
        unpop: (sid, flags) => s.world.unpop(sid, flags), pop: (sid, x, y, flags) => s.world.pop(sid, x, y, flags), carrierPickup: () => { throw new Error('검사 포획 미공급'); }, nuggetTransfer: () => { throw new Error('검사 너겟 전달 미공급'); },
    });
    return { world: s.world, core: s.core, priests: s.priests, halt, arrival: s.motion.priestArrival, cleanup: s.motion.cleanup, lookup: new RawContainedLookup(s.core.pool, (sid, flags) => s.world.destroy(sid, flags)), start: s.startState, process: s.processState, search: s.motionState.search, effects: s.motionEffects.path };
}
// 검증할 참조/장치만 바꿔 원문의 실제 Path 바인딩을 구성한다.
function makePaths(input, overrides = {}) { const x = { ...input, ...overrides }; return new RawCampaignPathBindings(x.world, x.core, x.priests, x.halt, x.arrival, x.cleanup, x.lookup, x.start, x.process, x.search, x.effects); }

test('CampaignMotion: cpppj 이동 루프의 owner1·2 실제 사제/BFS/도착/cleanup/Form 반납', () => {
    // 같은 보호 사제의 두 소유자에 대해 cpppj CheckCampaignMovement의 경로 부분을 실행한다.
    for (const owner of [1, 2]) {
        const { s, priest } = scene(owner); const begin = Uint8Array.from(s.core.pool.slot(priest));
        assert.equal(s.priests.priestState.immobile(priest), false);
        // 실제 Start/BFS/Prepare/Attach로 생성한 경로이다.
        const path = s.motion.paths.startPriest(request(priest)); assert.ok(path instanceof RawPathProcess); const form = path.form; const count = s.core.kernel.size; const endpoint = path.runtime.point(path.runtime.count - 1);
        assert.equal(s.motion.paths.currentPath(priest), path); assert.ok(s.pathPackets.length > 0); assert.equal(s.world.storage.pathSearchWork.initialized, true);
        // 같은 clock으로 실행한 실제 Path 프레임 수이다.
        const frames = finish(s, priest); const raw = s.core.pool.slot(priest);
        assert.ok(frames > 0 && frames < 1200); assert.notDeepEqual([...raw.subarray(14, 22)], [...begin.subarray(14, 22)]);
        assert.deepEqual([get(raw, 14), get(raw, 18)], [floatToBits(endpoint.x), floatToBits(endpoint.y)]);
        assert.ok(s.core.pool.slot(form)[11] & 1); assert.equal(s.core.kernel.size, count - 1); assert.equal(s.motion.paths.currentRuntime(priest), null); assert.equal(s.core.bookkeeping.depth, 0);
        assert.equal(s.motion.pool, s.core.pool); assert.equal(s.motion.carrierArrival.pool, s.core.pool); assert.equal(s.motion.cleanup.pool, s.core.pool); assert.equal(s.motion.priestArrival.pool, s.core.pool); assert.equal(s.motion.terrain.pool, s.core.pool);
        assert.equal(s.motion.paths.starter.pool, s.core.pool); assert.equal(s.motion.commonHalt(priest), 10);
    }
});

test('CampaignMotion: 같은 Pool/장부/시계/프레임 외의 입력은 첫 명령 전에 거부한다', () => {
    // 보호 자산/실제 모듈을 공유하는 단일 시험 월드이다.
    const s = new CampaignMotionScene(); const other = new CampaignMotionScene();
    // 실제 다른 월드의 하위 모듈을 섞지 않는다.
    for (const name of ['priests', 'islands', 'contained', 'anchor', 'hp']) assert.throws(() => s.makeMotion({ [name]: other[name] }), /불일치/);
    assert.throws(() => s.makeMotion({ frames: s.frameTypes.slice(1) }), /불일치/);
    assert.throws(() => s.makeMotion({ start: { ...s.startState, clock: other.core.processState } }), /불일치/);
    assert.throws(() => s.makeMotion({ process: other.processState }), /불일치/);
});

test('CampaignPath: 같은 현재 Core·하위·clock·실제54와 필수 효과 계약을 확인한다', () => {
    // 보호 자산/실제 모듈을 공유하는 단일 시험 월드이다.
    const s = new CampaignMotionScene(); const other = new CampaignMotionScene(); const input = pathInputs(s);
    // 같은 길이의 사본 배열도 C++ span.data가 달라지는 경우처럼 거부한다.
    for (const name of ['pool', 'hash', 'types', 'frames', 'shapes', 'processes', 'processState', 'kernel']) assert.throws(() => makePaths(input, { core: { ...s.core, [name]: other.core[name] } }), /연결 오류/);
    assert.throws(() => makePaths(input, { core: { ...s.core, spots: new Uint8Array(65536) } }), /연결 오류/);
    assert.equal(makePaths(input, { core: { ...s.core, spots: new Uint8Array(s.core.spots.buffer, s.core.spots.byteOffset, s.core.spots.byteLength) } }).pool, s.core.pool);
    assert.throws(() => makePaths(input, { start: { ...s.startState, priestType: 157 } }), /연결 오류/);
    assert.throws(() => makePaths(input, { process: { ...s.processState, priestProcess: 53 } }), /연결 오류/);
    // 필수 효과 여섯 개는 생성 시점에서 검사한다.
    for (const name of ['metadata', 'soundId', 'stepSound', 'startBlocked', 'arrivalBlocked', 'retarget']) assert.throws(() => makePaths(input, { effects: { ...input.effects, [name]: null } }), /연결 오류/);
});

test('CampaignPath: 현재 부모 검증과 정상 빈 경로는 선택하지 않은 큰 도착 효과를 요구하지 않는다', () => {
    const { s, priest } = scene();
    assert.equal(s.motion.paths.currentPath(priest), null); assert.equal(s.motion.paths.setTargets(priest, 1, 2), false); assert.equal(s.motion.paths.retargetCurrent(priest), false);
    assert.throws(() => s.motion.paths.startPriest(request(4)), /SID 범위/); assert.throws(() => s.motion.paths.currentPath(4), /SID 범위/);
    // 현재 부모/Form의 실제 슬롯 보기이다.
    const raw = s.core.pool.physical(priest); const table = get(raw, 0); put(raw, 0, 0);
    assert.throws(() => s.motion.paths.startPriest(request(priest)), /가상 표/); put(raw, 0, table);
    raw[11] |= 8; assert.throws(() => s.motion.paths.startPriest(request(priest)), /살아 있는/); raw[11] &= ~8;
    assert.throws(() => s.motion.paths.startConstruction(request(priest)), /타입\/VT/); assert.throws(() => s.motion.paths.startCarrier(request(priest)), /실제 부모/); assert.throws(() => s.motion.paths.startDude(request(priest)), /실제 부모/);
});

test('CampaignPath: 여덟 실제 cdecl 생성자와 mutable 번호는 서로 다른 종류 선택이다', () => {
    // 보호 자산/실제 모듈을 공유하는 단일 시험 월드이다.
    const s = new CampaignMotionScene();
    // 현재 번호를 바꿔도 함수 주소가 고정한 클래스의 Runtime 가상 표는 바뀌지 않는다.
    const cases = [[0x48be40, 23], [0x48be70, 42], [0x48bea0, 39], [0x48bed0, 38], [0x48bf00, 41], [0x48bf30, 24], [0x48bf60, 54], [0x48bf90, 40]];
    for (const [address, type] of cases) { const path = s.motion.paths.createByConstructor(address); assert.ok(path instanceof RawPathProcess); assert.equal(path.movementType(), type); assert.equal(path.form, 0); assert.equal(path.parent, 0); }
    // 번호 변경 전 실제 고정 생성자가 만든 Path이다.
    const original = s.motion.paths.createByConstructor(0x48bf60); const table = get(original.runtime.bytes, 0); s.processState.priestProcess = 53;
    // 같은 생성자와 바뀐 현재 번호가 만든 Path이다.
    const changed = s.motion.paths.createByConstructor(0x48bf60); assert.equal(changed.movementType(), 53); assert.equal(get(changed.runtime.bytes, 0), table);
    assert.equal(s.motion.paths.createByConstructor(0x123456), null);
});

test('CampaignPath: 실제 후보 BFS는 같은 장기 거리표/큐를 쓰며 Form/Kernel을 만들지 않는다', () => {
    const { s, priest } = scene(); const temporary = new RawPathRuntime({ priest: true }); const before = s.core.kernel.size; const shared = s.world.storage.pathSearchWork;
    assert.equal(shared.initialized, false); const work = shared.work;
    assert.equal(s.motion.paths.searchCandidate(temporary, priest, { x: 23.5, y: 23.5 }, { x: 30.5, y: 30.5 }, 1), true); assert.ok(temporary.count > 0); assert.equal(s.core.kernel.size, before); assert.equal(shared.initialized, true);
    // 비교할 다른 실제 장면 또는 반환 장치이다.
    const other = s.makeMotion(); assert.equal(s.world.storage.pathSearchWork.work, work); assert.equal(other.paths.pool, s.core.pool);
});

test('CampaignPath: 실제 기존 경로 교체·현재 PID/Runtime 조회와 두 DWORD 대상 setter', () => {
    const { s, priest } = scene(); const first = s.motion.paths.startPriest(request(priest)); const pid = first.pid;
    // 기존 경로를 교체한 실제 새 경로이다.
    const second = s.motion.paths.startPriest(request(priest, 28.5, 32.5)); assert.notEqual(first, second); assert.equal(s.core.processes.isRegistered(pid, first), false); assert.equal(s.motion.paths.currentPath(priest), second);
    assert.equal(s.motion.paths.setTargets(priest, 0x89abcdef, 0xfedcba98), true); assert.deepEqual([second.runtime.targetA, second.runtime.targetB], [0x89abcdef, 0xfedcba98]);
    second.runtime.setTargets(0, 0);
    // 현재 Form PID가 다른 실제 프로세스로 바뀌면 캐시한 Path를 돌려주지 않는다.
    const raw = s.core.pool.physical(second.form); const currentPid = get(raw, 18); put(raw, 18, 0);
    assert.throws(() => s.motion.paths.currentPath(priest), /실제Host Path/); put(raw, 18, currentPid);
    assert.equal(s.motion.paths.currentRuntime(priest), second.runtime);
});

test('CampaignMotion: caller 함수 사본을 보존하며 실제 retarget이 현재 Path를 지워도 재접근하지 않는다', () => {
    const { s, priest } = scene(); const calls = [];
    // 값 사본으로 보관한 큰 외부 효과 함수이다.
    const effects = { ...s.motionEffects, path: { ...s.motionEffects.path, retarget: (runtime, parent) => { calls.push([runtime, parent]); s.core.processes.kill(s.motion.paths.currentPath(parent), 0x40); } } };
    const motion = s.makeMotion({ effects }); const path = s.motion.paths.startPriest(request(priest));
    effects.path.retarget = () => { throw new Error('호출자 함수 재대입 누출'); };
    assert.equal(motion.paths.retargetCurrent(priest), true); assert.deepEqual(calls, [[path.runtime, priest]]); assert.equal(motion.paths.currentPath(priest), null);
});

test('CampaignPath: 실제 Path 일반/강제 writer와 같은 버퍼·외부 전송/다른 프로세스 fallback', () => {
    const { s, priest } = scene(); const path = s.motion.paths.startPriest(request(priest)); const sent = [];
    // 실제 writer가 공유할 버퍼이다.
    const bytes = new Uint8Array(16384).fill(0xcc); const header = { packetId: 0x30, packetFlags: 1, packetLength: 0 }; const fallbackBytes = Uint8Array.of(7); const calls = [];
    // 같은 실제 버퍼/현재 헤더/외부 전송으로 연결한 직렬화기이다.
    const wire = s.motion.paths.transmission(header, bytes, (n) => sent.push(n), { writeData: () => 1, writeFullData: () => 1, packet: () => fallbackBytes, transmit: (n) => calls.push(n) });
    // 실제 writer가 반환한 작성 길이이다.
    const length = wire.writeData(path, true); assert.ok(length > 24); assert.equal(wire.packet(length).buffer, bytes.buffer); assert.equal(bytes[1], 0x30); wire.transmit(length); assert.deepEqual(sent, [length]);
    header.packetId = 0x61; wire.writeFullData(path, 0x80000000); assert.equal(bytes[1], 0x61);
    assert.throws(() => wire.packet(-1), RangeError); assert.throws(() => wire.packet(bytes.length + 1), RangeError);
    // 비교할 다른 실제 장면 또는 반환 장치이다.
    const other = s.core.kernel.get(1); assert.equal(wire.writeData(other, true), 1); assert.equal(wire.packet(1), fallbackBytes); wire.transmit(1); assert.equal(wire.writeFullData(other, 1), 1); assert.deepEqual(calls, [1]);
});

test('CampaignPath: 30h 실제 경로 수신은 free Form의 생성/decoder/Prepare/Attach·부모 재배치와 현재 trace를 연결한다', () => {
    const { s, priest } = scene(); const path = s.motion.paths.startPriest(request(priest)); const form = path.form; const count = path.runtime.count;
    // 실제 writer가 반환한 작성 길이이다.
    const length = s.pathTransmission.writeData(path, true); const packet = Uint8Array.from(s.pathTransmission.packet(length));
    s.core.processes.kill(path, 0x40); s.core.processState.boss = false; const traces = [];
    s.world.storage.gameAux.pathTraceLevel = 57; s.world.storage.bindGraphTrace((level, text) => traces.push([level, text]));
    // 해제하지 않고 보존해야 하는 두 번째5105 Regular이다.
    const previous = s.core.processes.addRegular(priest, 0x5105, 0); const regular = s.core.processes.addRegular(priest, 0x5105, 0); const regularForm = regular.form;
    s.motion.paths.receivePacket(packet); const received = s.motion.paths.currentPath(priest);
    assert.ok(received instanceof RawPathProcess); assert.notEqual(received, path); assert.equal(received.form, form); assert.equal(received.runtime.count, count); assert.ok(s.core.pool.slot(regularForm)[11] & 1);
    assert.equal(s.core.processes.fromForm(previous.form), previous);
    // 실제 첫 경로 또는 decoder가 사용하는 첫점이다.
    const first = received.runtime.point(0); assert.deepEqual([get(s.core.pool.slot(priest), 14), get(s.core.pool.slot(priest), 18)], [floatToBits(first.x), floatToBits(first.y)]);
    assert.ok(traces.length > 0); assert.ok(traces.every(([level]) => level === 57));
});

test('CampaignPath: live Form 상세/진단은 현재 TYPE을 보고 경로를 갱신하지 않으며 boss 진단도 복귀한다', () => {
    const { s, priest } = scene(); const path = s.motion.paths.startPriest(request(priest)); const length = s.pathTransmission.writeData(path, true); const packet = Uint8Array.from(s.pathTransmission.packet(length));
    // live 무갱신을 확인할 수신 전 Runtime 바이트이다.
    const image = Uint8Array.from(path.runtime.bytes); const calls = []; const detail = { value: '' };
    s.world.storage.bindDisplayDiagnostics(detail, (...args) => calls.push(args)); s.core.processState.boss = false; s.motion.paths.receivePacket(packet);
    assert.equal(s.motion.paths.currentPath(priest), path); assert.deepEqual(path.runtime.bytes, image); assert.equal(calls.at(-1)[2], 0x24f); assert.match(detail.value, /Receiving an invalid update/);
    assert.ok(detail.value.includes(s.core.types[54].name)); assert.ok(Buffer.byteLength(detail.value) <= 255);
    s.core.processState.boss = true; s.motion.paths.receivePacket(packet); assert.deepEqual(calls.slice(-2).map((entry) => entry[2]), [0x224, 0x24f]);
});

test('CampaignPath: 수신의 실제 WORD/SID/생성자·payload 부족은 물리 경계에서 거부한다', () => {
    const { s, priest } = scene(); s.core.processState.boss = false;
    // 실제 송신 writer가 만든 수신 시험 바이트이다.
    const packet = new Uint8Array(24); put(packet, 4, 23000, 2); put(packet, 6, priest, 2);
    assert.throws(() => s.motion.paths.receivePacket(packet.subarray(0, 5)), /Form WORD 부족/);
    assert.throws(() => s.motion.paths.receivePacket(packet.subarray(0, 6)), /선택된 WORD 부족/);
    // 전체 WORD가 0이거나 풀 밖인 입력을 다른 유효 번호로 바꾸지 않는다.
    for (const form of [0, s.core.pool.capacity]) { const bad = Uint8Array.from(packet); put(bad, 4, form, 2); assert.throws(() => s.motion.paths.receivePacket(bad), /Form 물리 범위/); }
    // 물리 경계 오류를 지정한 패킷이다.
    const bad = Uint8Array.from(packet); put(bad, 6, s.core.pool.capacity, 2); assert.throws(() => s.motion.paths.receivePacket(bad), /부모 물리 범위/);
    // 복원할 실제 TYPE의 현재 생성자 주소이다.
    const address = s.core.types[54].constructorAddress; s.core.types[54].constructorAddress = 0; assert.throws(() => s.motion.paths.receivePacket(packet), /생성자 미연결/); s.core.types[54].constructorAddress = address;
    put(packet, 13, 1, 2); assert.throws(() => s.motion.paths.receivePacket(packet), /payload 부족/);
});

test('CampaignPath: 실제 건설·운반·Dude 부모의 TYPE/VT/이동 매트릭스와 외부 도착 필수 경계', () => {
    // 보호 자산/실제 모듈을 공유하는 단일 시험 월드이다.
    const s = new CampaignMotionScene(); s.world.load([]);
    // 살아 있는 void 부모도 현재 경로 조회는 가능하며 실제 종속 Path가 없으면 null이다.
    for (const type of [134, 105, 133, 111, 141, 146, 135, 137, 156, 70]) {
        // 원문 수신 헤더의 현재 부모 번호이다.
        const parent = s.create(type, 1); assert.equal(s.motion.paths.currentPath(parent), null);
        if (type === 134 || type === 105) assert.throws(() => s.motion.paths.startConstruction(request(parent)), /실제 도착 소비자/);
        if (type === 70) assert.throws(() => s.motion.paths.startDude(request(parent)), /실제 부모 효과/);
        // 현재 부모/Form의 실제 슬롯 보기이다.
        const raw = s.core.pool.physical(parent); const table = get(raw, 0); put(raw, 0, table ^ 0x100);
        assert.throws(() => s.motion.paths.currentPath(parent), /실제 부모|타입\/VT/); put(raw, 0, table);
    }
    // 별도 미연결42ac60을 가진 실제 empty 부모이다.
    const empty = s.create(187, 1); assert.throws(() => s.motion.paths.startCarrier(request(empty)), /empty42ac60/);
    assert.throws(() => s.motion.paths.startCarrier(request(4)), /판본\/SID/); assert.throws(() => s.motion.paths.startDude(request(4)), /판본\/SID/); assert.throws(() => s.motion.paths.startConstruction(request(4)), /판본\/SID/);
});

test('CampaignPath: Native writer 선택은 생성 시 고정하고 같은 실제 버퍼·부모 변경·DWORD force를 전달한다', () => {
    const { s, priest } = scene(); const path = s.motion.paths.startPriest(request(priest)); const calls = []; const buffer = new Uint8Array(2048);
    s.world.storage.usesNativeTransmissionDevices = () => true;
    s.world.storage.serializeProcessForm = (...args) => { calls.push(args); return 27; };
    // 같은 실제 버퍼/현재 헤더/외부 전송으로 연결한 직렬화기이다.
    const wire = s.motion.paths.transmission(s.pathHeader, buffer, (length) => calls.push(length));
    s.world.storage.usesNativeTransmissionDevices = () => false;
    assert.equal(wire.writeData(path, true), 27); assert.equal(wire.writeFullData(path, 0x80000000), 27); assert.equal(wire.packet(27).buffer, buffer.buffer); wire.transmit(27);
    assert.deepEqual(calls, [[path, buffer, true, 0], [path, buffer, false, 0x80000000], 27]);
});

test('CampaignPath: 수신 live 상세는 현재 TYPE 이름의 첫 NUL과 원문 255BYTE 한도를 따른다', () => {
    const { s, priest } = scene(); const path = s.motion.paths.startPriest(request(priest)); const packet = new Uint8Array(6); put(packet, 4, path.form, 2);
    // 원문 상세와 진단을 관찰하는 장치이다.
    const detail = { value: '' }; const calls = []; s.core.processState.boss = false; s.world.storage.bindDisplayDiagnostics(detail, (...args) => calls.push(args));
    s.core.types[54].name = 'short\0ignored'; s.motion.paths.receivePacket(packet); assert.ok(detail.value.includes('short.\n')); assert.equal(detail.value.includes('ignored'), false);
    s.core.types[54].name = 'x'.repeat(300); s.motion.paths.receivePacket(packet); assert.equal(Buffer.byteLength(detail.value), 255); assert.equal(calls.at(-1)[2], 0x24f);
});
