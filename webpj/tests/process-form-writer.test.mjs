// 실제 Host의 WriteFormPacket 부모/PID/Regular·파생 writer 계약을 대조한다(225553df, WB04-E591).
// 이 부모의 직접 cpppj 검사/원본 fixture는 없다. 실제 보호 자산/Host와 독립 패킷 기대값을 쓰며 게임을 실행하지 않는다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { GameAssets } from '../www/assets/game-assets.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RegularProcess, SquidProcess, SquidProcessHost, createProcessHostRuntime } from '../www/core/squid-process.js';
import { createSquidTransmissionState } from '../www/core/raw-squid-transmission.js';
import { put, get, bitsToFloat } from './raw-support.mjs';

// 같은 실제 보호 TYPE/SHP의 Pool/Kernel/Host를 빌린다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database);
// 이번 순수 writer에서 선택하지 않은 효과를 빈 성공으로 바꾸지 않는다.
function forbidden() { throw new Error('선택되지 않은 외부 효과'); }
// 실제 기반 owner와 필수 시각 입력만 만든다. 전체 Session/미션 Load는 하지 않는다.
function scene() { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList() }); }
// 서버 SID를 실제 공장에서 만들고 현재 Host의 같은 부모로 사용한다.
function parent(o, flags = 0) { return o.world.create(75, flags); }
// 같은 실제 Kernel에 일반 SquidProcess를 붙인다. 외부 파생 writer의 계약만 입력으로 공급한다.
function derived(o, owner) { const process = new SquidProcess(); o.core.processes.attach(process, 14, owner, 0x50); return process; }
// writer가 실제 목적지를 빌렸는지 확인하는 고정 초기 바이트다.
function packet(length = 40) { return Uint8Array.from({ length }, (_, n) => 0x80 + n); }

test('ProcessForm writer: 일반 Regular와 허용되지 않은 공유 Regular는 버퍼/파생 장치를 읽지 않고0이다', () => {
    const o = scene();
    try { const sid = parent(o); const regular = o.core.processes.addRegular(sid, 7, 1.25); const shared = o.core.processes.addSharedRegular(sid, 8, -1.25); const wire = createSquidTransmissionState(); const bytes = packet(); const before = bytes.slice(); o.core.processes.setFormTransmit({ writeFullData: forbidden, packet: forbidden }); assert.equal(regular.shared, false); assert.equal(shared.shared, true); assert.equal(o.core.processes.writeFormPacket(regular.form, 0, bytes, wire), 0); assert.equal(o.core.processes.writeFormPacket(shared.form, 0, bytes, wire), 0); assert.deepEqual(bytes, before); wire.clientSidAllowed = 1; assert.equal(o.core.processes.writeFormPacket(regular.form, 0, new Uint8Array(0), wire), 0); } finally { o.dispose(); }
});

test('ProcessForm writer: 공유28BYTE의 현재 헤더/부모/double/사건/payload/count와 미작성4·5/꼬리를 보존한다', () => {
    const o = scene();
    try { const sid = parent(o); const process = o.core.processes.addSharedRegular(sid, 0xffffffff, -1.25); process.resetTask(0x89abcdef, 100.25); process.setInitialCount(0xfedcba98); const wire = createSquidTransmissionState(); wire.clientSidAllowed = 0x80000000; wire.sharedRegular = { protocol: 0xff, message: 0x72, length: 0x10001 }; const bytes = packet(); const expected = bytes.slice(); const view = new DataView(expected.buffer); expected[0] = 0x7f; expected[1] = 0x72; view.setUint16(2, 4, true); view.setUint16(6, sid, true); view.setFloat64(8, 100.25, true); view.setUint32(16, 0x89abcdef, true); view.setUint32(20, 0xbfa00000, true); view.setUint32(24, 0xfedcba98, true); assert.equal(o.core.processes.writeFormPacket(process.form, 0x12345678, bytes, wire), 28); assert.deepEqual(bytes, expected); wire.sharedRegular.length = 0; process.resetTask(3, -0); assert.equal(o.core.processes.writeFormPacket(process.form, 0, bytes, wire), 28); assert.equal(get(bytes, 2, 2), 4); assert.equal(new DataView(bytes.buffer).getBigUint64(8, true), 0x8000000000000000n); } finally { o.dispose(); }
});

test('ProcessForm writer: 공유 여부는 현재 Form TYPE가 아니라 실제 생성자 writer 플래그다', () => {
    const o = scene();
    try { const sid = parent(o); const regular = o.core.processes.addRegular(sid, 2, 0); const shared = o.core.processes.addSharedRegular(sid, 3, 0); const wire = createSquidTransmissionState(); wire.clientSidAllowed = 1; o.core.pool.allocated(regular.form)[10] = 61; o.core.pool.allocated(shared.form)[10] = 62; assert.equal(o.core.processes.writeFormPacket(regular.form, 0, packet(), wire), 0); assert.equal(o.core.processes.writeFormPacket(shared.form, 0, packet(), wire), 28); } finally { o.dispose(); }
});

test('ProcessForm writer: 공유 payload의 quiet NaN/무한대/음수0와 double NaN 비트를 보존한다', () => {
    const o = scene();
    try {
        const sid = parent(o); const wire = createSquidTransmissionState(); wire.clientSidAllowed = 1; const clock = new DataView(new ArrayBuffer(8)); clock.setBigUint64(0, 0x7ff8123456789abcn, true);
        // 원문의 float/double 저장 비트만 입력으로 공급하며 산술/실제 게임 시계는 진행하지 않는다.
        for (const bits of [0x7fc12345, 0xffc54321, 0x7f800000, 0x80000000]) { const process = o.core.processes.addSharedRegular(sid, 7, bitsToFloat(bits)); process.resetTask(7, clock.getFloat64(0, true)); const bytes = packet(); o.core.processes.writeFormPacket(process.form, 0, bytes, wire); const view = new DataView(bytes.buffer); assert.equal(view.getUint32(20, true), bits); assert.equal(view.getBigUint64(8, true), 0x7ff8123456789abcn); }
    } finally { o.dispose(); }
});

test('ProcessForm writer: 공유 버퍼 부족은 저장 전에 거부하고 비허용 관문은 물리 크기를 먼저 읽지 않는다', () => {
    const o = scene();
    try { const process = o.core.processes.addSharedRegular(parent(o), 7, 1); const wire = createSquidTransmissionState(); const bytes = packet(27); const before = bytes.slice(); wire.clientSidAllowed = 1; assert.throws(() => o.core.processes.writeFormPacket(process.form, 0, bytes, wire), /실제28BYTE/); assert.deepEqual(bytes, before); wire.clientSidAllowed = 0; assert.equal(o.core.processes.writeFormPacket(process.form, 0, bytes, wire), 0); } finally { o.dispose(); }
});

test('ProcessForm writer: 현재 부모 진단이 PID를 바꾸면 복귀 뒤의 실제 Kernel 객체를 선택한다', () => {
    const o = scene();
    try { const client = parent(o, 2); const first = o.core.processes.addRegular(client, 1, 0); const second = o.core.processes.addSharedRegular(parent(o), 0x12345678, 1.75); const wire = createSquidTransmissionState(); wire.clientSidAllowed = 1; const calls = []; o.world.storage.bindDisplayDiagnostics({ value: '' }, (...args) => { calls.push(args); put(o.core.pool.allocated(first.form), 18, second.pid); }); const bytes = packet(); assert.equal(o.core.processes.writeFormPacket(first.form, 0, bytes, wire), 28); assert.deepEqual(calls, [['!getParent().isClientSid()', '\\Ns\\O\\Squid.cpp', 0xac2]]); assert.equal(get(bytes, 6, 2), second.parent); assert.equal(get(bytes, 16), 0x12345678); put(o.core.pool.allocated(first.form), 18, first.pid); } finally { o.dispose(); }
});

test('ProcessForm writer: 부모 진단 중단/누락과 실제 PID 누락은 앞선 물리와 버퍼를 보존한다', () => {
    const o = scene();
    try { const process = o.core.processes.addRegular(parent(o, 2), 1, 0); const bytes = packet(); const before = bytes.slice(); const wire = createSquidTransmissionState(); const host = SquidProcessHost.borrowing(o.core.pool, o.core.types, o.core.kernel, o.core.processState, () => null); o.core.processState.assertion = null; assert.throws(() => host.writeFormPacket(process.form, 0, bytes, wire), /부모 SID 진단 미연결/); o.core.processState.assertion = () => { throw new Error('명시 진단 중단'); }; assert.throws(() => host.writeFormPacket(process.form, 0, bytes, wire), /명시 진단 중단/); o.core.processState.assertion = () => { put(o.core.pool.allocated(process.form), 18, 0); }; assert.throws(() => host.writeFormPacket(process.form, 0, bytes, wire), /실제 PID 객체 누락/); assert.deepEqual(bytes, before); put(o.core.pool.allocated(process.form), 18, process.pid); } finally { o.dispose(); }
});

test('ProcessForm writer: 파생 writer는 현재 Process/전체 DWORD 인자와 함수 값 사본을 사용한다', () => {
    const o = scene();
    try { const process = derived(o, parent(o)); const calls = []; const bytes = packet(); const source = Uint8Array.of(1, 2, 3, 4, 5); const device = { writeFullData: (selected, argument) => { assert.equal(selected, process); calls.push(['write', argument]); device.packet = forbidden; return 4; }, packet: (size) => { calls.push(['packet', size]); return source; } }; o.core.processes.setFormTransmit(device); device.packet = forbidden; assert.equal(o.core.processes.writeFormPacket(process.form, 0x100000001, bytes, createSquidTransmissionState()), 4); assert.deepEqual(calls, [['write', 1], ['packet', 4]]); assert.deepEqual([...bytes.subarray(0, 6)], [1, 2, 3, 4, 0x84, 0x85]); } finally { o.dispose(); }
});

test('ProcessForm writer: 파생0/음수 반환은 packet을 조회하지 않으며 양수만 현재 물리 범위를 검사한다', () => {
    const o = scene();
    try { const process = derived(o, parent(o)); const bytes = packet(4); const before = bytes.slice(); const wire = createSquidTransmissionState(); o.core.processes.setFormTransmit({ writeFullData: () => 0, packet: forbidden }); assert.equal(o.core.processes.writeFormPacket(process.form, 0, bytes, wire), 0); o.core.processes.setFormTransmit({ writeFullData: () => 0xffffffff, packet: forbidden }); assert.equal(o.core.processes.writeFormPacket(process.form, 0, bytes, wire), -1); o.core.processes.setFormTransmit({ writeFullData: () => 5, packet: () => new Uint8Array(8) }); assert.throws(() => o.core.processes.writeFormPacket(process.form, 0, bytes, wire), /실제 패킷 복사 범위/); o.core.processes.setFormTransmit({ writeFullData: () => 4, packet: () => new Uint8Array(3) }); assert.throws(() => o.core.processes.writeFormPacket(process.form, 0, bytes, wire), /실제 패킷 복사 범위/); assert.deepEqual(bytes, before); } finally { o.dispose(); }
});

test('ProcessForm writer: 선택된 누락은 writer/양수 반환/작성 버퍼 순서에서만 거부한다', () => {
    const o = scene();
    try { const process = derived(o, parent(o)); const bytes = packet(); const wire = createSquidTransmissionState(); o.core.processes.setFormTransmit({}); assert.throws(() => o.core.processes.writeFormPacket(process.form, 0, bytes, wire), /전체 파생 writer 미연결/); o.core.processes.setFormTransmit({ writeFullData: () => 1 }); assert.throws(() => o.core.processes.writeFormPacket(process.form, 0, bytes, wire), /작성 버퍼 미연결/); } finally { o.dispose(); }
});

test('ProcessForm writer: 같은 시작 주소는 복사를 생략하고 다른 겹친 주소는 실제 바이트만 복사한다', () => {
    const o = scene();
    try { const process = derived(o, parent(o)); const bytes = packet(12); const wire = createSquidTransmissionState(); const original = bytes.set; bytes.set = forbidden; o.core.processes.setFormTransmit({ writeFullData: () => 4, packet: () => bytes.subarray(0, 8) }); assert.equal(o.core.processes.writeFormPacket(process.form, 0, bytes, wire), 4); bytes.set = original; o.core.processes.setFormTransmit({ writeFullData: () => 4, packet: () => bytes.subarray(2, 7) }); assert.equal(o.core.processes.writeFormPacket(process.form, 0, bytes, wire), 4); assert.deepEqual([...bytes.subarray(0, 7)], [0x82, 0x83, 0x84, 0x85, 0x84, 0x85, 0x86]); } finally { o.dispose(); }
});

test('ProcessForm writer: 현재 Runtime pin/활성 호출은 진단보다 먼저이며 정상/예외에 같은 호출만 반납한다', () => {
    const o = scene();
    try { const process = derived(o, parent(o, 2)); const calls = []; const runtime = createProcessHostRuntime({ destroy: o.core.destroy, pin: () => { calls.push(['pin']); return o; }, assertion: () => { calls.push(['assert', runtime.activeCalls]); runtime.transmit = { writeFullData: () => { calls.push(['write', runtime.activeCalls]); return 0; } }; } }); const host = SquidProcessHost.borrowing(o.core.pool, o.core.types, o.core.kernel, o.core.processState, () => runtime); assert.equal(host.writeFormPacket(process.form, 0, packet(), createSquidTransmissionState()), 0); assert.deepEqual(calls, [['pin'], ['assert', 1], ['write', 1]]); assert.equal(runtime.activeCalls, 0); runtime.assertion = () => { assert.equal(runtime.activeCalls, 1); throw new Error('진단 중단'); }; assert.throws(() => host.writeFormPacket(process.form, 0, packet(), createSquidTransmissionState()), /진단 중단/); assert.equal(runtime.activeCalls, 0); runtime.pin = () => { throw new Error('pin 중단'); }; assert.throws(() => host.writeFormPacket(process.form, 0, packet(), createSquidTransmissionState()), /pin 중단/); assert.equal(runtime.activeCalls, 0); } finally { o.dispose(); }
});

test('Regular writer 종류: 부착 전 pin 실패에도 공유 선택을 보존하고 예약 재생성은 일반 생성자로 되돌린다', () => {
    const o = scene();
    try { const process = new RegularProcess(7, 1); const runtime = createProcessHostRuntime({ destroy: o.core.destroy, pin: () => { throw new Error('pin 중단'); } }); const host = SquidProcessHost.borrowing(o.core.pool, o.core.types, o.core.kernel, o.core.processState, () => runtime); assert.throws(() => host.attachRegular(process, parent(o), true), /pin 중단/); assert.equal(process.shared, true); assert.equal(process.form, 0); process.initializeDetached(8, 2); assert.equal(process.shared, false); } finally { o.dispose(); }
});

test('ProcessForm writer: World 없음/빈 현재 표는 같은 장기 기본 writer와 진단을 사용한다', () => {
    const o = scene();
    try { const process = derived(o, parent(o, 2)); const calls = []; let current = null; const host = SquidProcessHost.borrowing(o.core.pool, o.core.types, o.core.kernel, o.core.processState, () => current); const input = { writeData: forbidden, writeFullData: (selected, argument) => { assert.equal(selected, process); calls.push(['write', argument]); return 3; }, packet: () => Uint8Array.of(5, 6, 7), transmit: forbidden }; host.bindDefaultFormTransmission(input, (...args) => calls.push(['assert', ...args])); input.writeFullData = forbidden; const bytes = packet(); const wire = createSquidTransmissionState(); assert.equal(host.writeFormPacket(process.form, 1, bytes, wire), 3); assert.deepEqual([...bytes.subarray(0, 4)], [5, 6, 7, 0x83]); assert.deepEqual(calls, [['assert', '!getParent().isClientSid()', '\\Ns\\O\\Squid.cpp', 0xac2], ['write', 1]]); current = createProcessHostRuntime({ transmit: {} }); assert.equal(host.writeFormPacket(process.form, 2, bytes, wire), 3); current.transmit = { writeFullData: () => 0 }; assert.equal(host.writeFormPacket(process.form, 3, bytes, wire), 0); current.transmit = { packet: forbidden }; assert.throws(() => host.writeFormPacket(process.form, 4, bytes, wire), /전체 파생 writer 미연결/); current = null; assert.equal(host.writeFormPacket(process.form, 5, bytes, wire), 3); } finally { o.dispose(); }
});

test('ProcessForm 기본 장치: 누락/직접 Host/중복 설치를 거부하고 실패 전에는 입력 함수를 실행하지 않는다', () => {
    const o = scene();
    try { const host = SquidProcessHost.borrowing(o.core.pool, o.core.types, o.core.kernel, o.core.processState, () => null); const input = { writeData: forbidden, writeFullData: forbidden, packet: forbidden, transmit: forbidden }; assert.throws(() => host.bindDefaultFormTransmission({ ...input, packet: null }, forbidden), /기본직렬화장치계약/); assert.throws(() => host.bindDefaultFormTransmission(input, null), /기본직렬화장치계약/); host.bindDefaultFormTransmission(input, forbidden); assert.throws(() => host.bindDefaultFormTransmission(input, forbidden), /기본직렬화장치계약/); const direct = new SquidProcessHost(o.core.pool, o.core.types, o.core.kernel, o.core.destroy, o.core.processState); assert.throws(() => direct.bindDefaultFormTransmission(input, forbidden), /기본직렬화장치계약/); } finally { o.dispose(); }
});
