// Storage/World의 공용 Squid 송신·삭제·실제 Host 선택을 대조한다(WB04-E594).
// 보호 TYPE/실제 Pool·Kernel과 독립 패킷 기대값을 쓰며 새 원본 실행/미션 플레이는 아니다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { CampaignSessionAssets } from '../www/core/raw-campaign-session-owners.js';
import { PriestPlacementAssets } from '../www/core/priest-placement-assets.js';
import { RawSurfaceStorage } from '../www/core/raw-surface-storage.js';
import { RawSurfaceWorld } from '../www/core/raw-surface-world.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcess, RegularProcess, createProcessHostRuntime } from '../www/core/squid-process.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { get, put } from './raw-support.mjs';

// 이번 경로에서 선택하지 않을 실제 외부 효과는 명시 실패다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 보호 자산은 한 번 해석하며 실제 물리/현재 TYPE는 각 Storage가 별도로 보유한다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database);
const assets = new CampaignSessionAssets(game, { read: forbidden });
const metadata = new PriestPlacementAssets(game.typeTable, game.shapes, assets.loadedTypes);
// 새 Source 저장소이며 아직 World/Process 대여를 하지 않는다.
function storage() { return new RawSurfaceStorage(assets.types, assets.frames, metadata.shapes, { source: game }); }
// 같은 실제 저장소의 World이며 외부 Kernel 검사는 명시 인자만 다르다.
function world(s, options = {}) {
    const links = Object.assign(new BridgeConnectState(), { bridgeType: 82, islandType: 94, stalagType: 95, connectorType: 155, noIslandType: 157, battle: true, mission: true });
    return new RawSurfaceWorld(s.types, s.frames, s.shapes, links, 97, { storage: s, ...options });
}
// writer를 생략하지 않는 실제 typed Pop 대여다. 선택하지 않는 다른 경계는 오류다.
function popRuntime(changes = {}) { return { transmit: forbidden, broadcast: forbidden, connected: forbidden, send: forbidden, ...changes }; }
// World의 실제 필수 경계와 패킷/로딩 관측을 연결한다. 함수 표는 제품에서 값 복사된다.
function bindWorld(w, changes = {}) {
    const packets = []; const depths = []; const core = w.core();
    const hooks = { assets: { emit: forbidden, selected: () => 0 }, destroy: (sid, flags, effects) => core.virtualDestroy.destroy(sid, flags, effects), regular: forbidden,
        transmit: { writeData: forbidden, transmit: forbidden }, walkerFall: forbidden, loadingDepth: (value) => depths.push(value), bridgeEffect: forbidden,
        squidPacket: (packet) => packets.push(Uint8Array.from(packet)), ...changes };
    w.bindRuntime(hooks); return { packets, depths, hooks };
}
// 실제 서버 자산은 생성자/HP를 거친다. 이 단위는 최초 Pop/후처리 대신 wire의 선택 부모를 검사한다.
function asset(s) { return s.factory.create(75, 0); }
// 실제 ProcessForm을 현재 Host/Kernel에 붙이며 0x50은 원문 전송 억제다.
function process(w) { const p = new SquidProcess(); w.core().processes.attach(p, 14, w.create(75, 0), 0x50); return p; }

test('공용 송신: 최초 getter는 Acquire/초기화 없이 같은 GameAux 버퍼와 한 본문을 보유한다', () => {
    const s = storage(); const bytes = s.pool.bytes(); const state = s.gameAux.squidTransmission; state.buffer.fill(0xa5); state.compressedCount = 17;
    const wire = s.transmission; assert.equal(wire, s.transmission); assert.equal(wire.state, state); assert.equal(wire.pool, s.pool);
    assert.deepEqual([s.borrowed, s.generation, state.compressedCount, state.buffer[2047]], [false, 0, 17, 0xa5]); assert.deepEqual(s.pool.bytes(), bytes);
});

test('공용 송신: 같은 현재 대여/필수 transmit과 오래된 반납 관문은 등록 때 효과를 내지 않는다', () => {
    const s = storage(); const r = popRuntime(); const bytes = s.pool.bytes();
    // 모든 거부는 Pool/예약 쓰기와 선택된 송신보다 앞이다.
    for (const bad of [null, {}, { transmit: null }]) assert.throws(() => s.bindPopRuntime(bad), /동일 실제 Pop/);
    const first = s.bindPopRuntime(r); assert.throws(() => s.bindPopRuntime(popRuntime()), /동일 실제 Pop/);
    const next = s.bindPopRuntime(r); first.release(); let calls = 0; r.transmit = () => { calls += 1; }; s.transmitPop(15001, 0x80000009);
    assert.equal(calls, 1); assert.deepEqual(s.pool.bytes(), bytes); next.release(); next.release();
});

test('공용 송신: 현재 Pop 대여는 같은 SID/전체 DWORD flags를 pin한 호출로 받는다', () => {
    const s = storage(); const calls = []; let use;
    const r = popRuntime({ pin: () => { calls.push('pin'); use.release(); }, transmit: (sid, flags) => calls.push([sid, flags]) }); use = s.bindPopRuntime(r);
    s.transmitPop(0x7ffe, 0xfedcba98); assert.deepEqual(calls, ['pin', [0x7ffe, 0xfedcba98]]);
});

test('공용 송신: 방송 pin 중 대여 교체도 현재 함수를 보존하고 같은 예약의 길이 뷰를 전달한다', () => {
    const s = storage(); const wire = s.transmission; wire.state.compress = 0; const sid = asset(s); const calls = []; let use; let replacement;
    const r = popRuntime({ pin: () => { calls.push('pin'); use.release(); replacement = popRuntime({ broadcast: () => calls.push('next') }); s.bindPopRuntime(replacement); },
        broadcast: (packet) => { assert.equal(packet.buffer, wire.state.buffer.buffer); assert.equal(packet.byteOffset, wire.state.buffer.byteOffset); assert.equal(packet.length, 35); calls.push(['old', get(packet, 4, 2)]); } });
    use = s.bindPopRuntime(r); wire.pop(sid, 9); wire.pop(sid, 9); assert.deepEqual(calls, ['pin', ['old', sid], 'next']);
});

test('공용 송신: 방송 대여가 없어도 먼저 실제35BYTE를 쓰고 뒤에서 선택된 누락을 보고한다', () => {
    const s = storage(); const sid = asset(s); const state = s.gameAux.squidTransmission; state.compress = 0; state.buffer.fill(0xa5);
    assert.throws(() => s.transmitPop(sid, 9), /broadcast.*미연결/);
    assert.deepEqual([...state.buffer.subarray(0, 6)], [1, 0x2c, 27, 0, sid & 255, sid >>> 8]); assert.equal(state.buffer[16], 75); assert.equal(state.buffer[35], 0xa5);
});

test('공용 송신: 압축은 현재 기본 TYPE·같은 Factory의 사제1/4 HP를 매번 읽는다', () => {
    const s = storage(); s.links.bridgeType = 75; s.types[75].maxHitPoints = 101; s.types[75].flags1 |= 0x10;
    s.gameAux.priestType = 75; s.gameAux.priestQuarterHp = true; const sid = asset(s); const raw = s.pool.slot(sid); put(raw, 26, 25, 2);
    const wire = s.transmission; wire.write(sid, 0); assert.equal(get(wire.state.buffer, 6, 2) & 0x610, 0);
    s.gameAux.priestQuarterHp = false; wire.write(sid, 0); assert.equal(get(wire.state.buffer, 6, 2) & 0x600, 0x200);
    s.types[75].maxHitPoints = 25; s.links.bridgeType = 82; wire.write(sid, 0); assert.equal(get(wire.state.buffer, 6, 2) & 0x610, 0x10);
});

test('공용 송신: 대상 연결/송신은 writer 뒤 각각 현재 대여를 pin하고 다른 대상을 자르지 않는다', () => {
    const s = storage(); const sid = asset(s); s.transmission.state.compress = 0; const calls = []; let connected = false;
    const r = popRuntime({ pin: () => calls.push('pin'), connected: (target) => { calls.push(['connected', target, get(s.transmission.state.buffer, 4, 2)]); return connected; },
        send: (target, packet) => { calls.push(['send', target, packet.length]); assert.equal(packet.buffer, s.gameAux.squidTransmission.buffer.buffer); } }); s.bindPopRuntime(r);
    s.transmission.sendTo(sid, -12345, 0xfedcba98); connected = true; s.transmission.sendTo(sid, 0x12345678, 7);
    assert.deepEqual(calls, ['pin', ['connected', -12345, sid], 'pin', ['connected', 0x12345678, sid], 'pin', ['send', 0x12345678, 35]]);
});

test('공용 송신: pin 예외는 방송을 막지만 앞서 작성한 실제 패킷은 보존한다', () => {
    const s = storage(); const sid = asset(s); s.transmission.state.compress = 0; const error = new Error('방송 pin 중단');
    const r = popRuntime({ pin: () => { throw error; } }); s.bindPopRuntime(r);
    assert.throws(() => s.transmission.pop(sid, 9), (actual) => actual === error); assert.equal(get(s.gameAux.squidTransmission.buffer, 4, 2), sid);
});

test('실제 World 송신: Runtime 방송 미연결 상태도 갱신/삭제 writer와 같은 버퍼를 사용한다', () => {
    const s = storage(); const w = world(s); const sid = w.create(75, 0); const state = s.gameAux.squidTransmission; state.compress = 0; state.buffer.fill(0xa5);
    s.transmitPop(sid, 9); assert.deepEqual([...state.buffer.subarray(0, 4)], [1, 0x2c, 27, 0]);
    w.transmitDeletion(sid, 0x12340009); assert.deepEqual([...state.buffer.subarray(0, 6)], [1, 0x2e, 8, 0, sid & 255, sid >>> 8]); assert.equal(state.buffer[8], 0); w.dispose();
    assert.throws(() => s.transmission.pop(sid, 9), /broadcast.*미연결/);
});

test('실제 World 송신: 반환 크기와 삭제 표식·선언 WORD는 같은 실제 방송을 거친다', () => {
    const s = storage(); const w = world(s); const out = bindWorld(w); const sid = w.create(75, 0); const state = s.gameAux.squidTransmission; state.compress = 0;
    s.transmitPop(sid, 9); state.forms.deletion.length = 0x1234; state.deletion.sid = sid; state.deletion.token = 0x71;
    w.transmitDeletion(sid, 0xabcd0009); assert.deepEqual(out.packets.map((packet) => packet.length), [35, 8]);
    assert.deepEqual([...out.packets[1]], [1, 0x2e, 0x34, 0x12, sid & 255, sid >>> 8, 0xf1, 0xab]); assert.equal(state.deletion.sid, 0); w.dispose();
});

test('실제 World 송신: 대상 연결/송신과 함수 표의 값 복사는 현재 Runtime을 따른다', () => {
    const s = storage(); const w = world(s); const calls = []; const out = bindWorld(w, { squidConnected: (target) => { calls.push(['connected', target]); return true; }, squidSend: (target, packet) => calls.push(['send', target, packet.length]) });
    out.hooks.squidSend = forbidden; out.hooks.squidPacket = forbidden; const sid = w.create(75, 0); s.transmission.state.compress = 0;
    s.transmission.sendTo(sid, 7, 1); s.transmission.pop(sid, 9); assert.deepEqual(calls, [['connected', 7], ['send', 7, 35]]); assert.equal(out.packets.length, 1); w.dispose();
});

test('실제 World 송신: 선택된 방송/대상 함수만 요구하고 누락은 실제 writer 뒤에 실패한다', () => {
    const s = storage(); const w = world(s); bindWorld(w, { squidPacket: undefined }); const sid = w.create(75, 0); s.transmission.state.compress = 0;
    assert.throws(() => s.transmission.pop(sid, 9), /방송 장치 미연결/); assert.equal(get(s.gameAux.squidTransmission.buffer, 4, 2), sid);
    assert.throws(() => s.transmission.sendTo(sid, 1, 0), /연결 관문 미연결/); w.dispose();
});

test('실제 World 송신: 외부 Kernel의 ProcessForm은 현재 World Host writer를 사용한다', () => {
    const s = storage(); const kernel = new Kernel(); const w = world(s, { processKernel: kernel }); const calls = []; const payload = Uint8Array.of(7, 8, 9);
    bindWorld(w, { transmit: { writeData: forbidden, writeFullData: (p, argument) => { calls.push([p, argument]); return 3; }, packet: () => payload, transmit: forbidden } });
    const p = process(w); assert.equal(s.processes.host.fromForm(p.form), null);
    assert.equal(s.transmission.write(p.form, 0xfedcba98), 3); assert.equal(calls[0][0], p); assert.equal(calls[0][1], 0xfedcba98); assert.deepEqual([...s.gameAux.squidTransmission.buffer.subarray(0, 3)], [7, 8, 9]); w.dispose();
});

test('공용 ProcessForm: Pop 대여가 없어도 같은 장기 Host/현재 PID writer를 사용한다', () => {
    const s = storage(); s.bindDisplayDevices({ view: () => ({ cameraX: 0, cameraY: 0, zoom: 65536, viewport: { left: 0, top: 0, right: 640, bottom: 480 } }), suppressed: () => true, invalidate: forbidden });
    const destroy = new RawSquidDestroy(s.pool, s.unpop, s.types); const payload = Uint8Array.of(0x18, 0x29); const calls = [];
    const runtime = createProcessHostRuntime({ destroy, factory: s.factory, asset: { selected: () => 0, emit: (event) => calls.push(['delete', event.effect]) },
        transmit: { writeData: forbidden, writeFullData: (p, argument) => { calls.push(['write', p, argument]); return 2; }, packet: () => payload, transmit: forbidden } });
    const use = s.processes.bind(runtime); const host = s.processes.host; const p = new SquidProcess(); host.attach(p, 14, asset(s), 0x50);
    assert.equal(s.transmission.write(p.form, 0x12345678), 2); assert.deepEqual(calls[0], ['write', p, 0x12345678]); assert.deepEqual([...s.gameAux.squidTransmission.buffer.subarray(0, 2)], [0x18, 0x29]);
    s.processes.dispose(); use.release();
});

test('공용 ProcessForm: 명시 현재 processPacket을 pin하고 SID/예약/전체 인자를 보존한다', () => {
    const s = storage();
    // 현재 Host 대신 명시 외부 Kernel 장치가 연결된 상태의 독립 source 계약이다.
    const form = s.factory.create(2, 0); const calls = []; const r = popRuntime({ pin: () => calls.push('pin'), processPacket: (sid, packet, argument) => { calls.push([sid, packet, argument]); packet[0] = 0x39; return 1; } });
    s.bindPopRuntime(r); assert.equal(s.transmission.write(form, 0xfedcba98), 1); assert.equal(calls[1][0], form); assert.equal(calls[1][1], s.gameAux.squidTransmission.buffer); assert.equal(calls[1][2], 0xfedcba98); assert.equal(calls[0], 'pin');
});

test('실제 World 송신: 반납 뒤 다음 World도 같은 송신기·헤더·압축 상태를 사용한다', () => {
    const s = storage(); const one = world(s); const wire = s.transmission; const state = wire.state; state.compressedCount = 177; state.buffer[2047] = 0xcc; one.dispose();
    const two = world(s); assert.equal(s.transmission, wire); assert.equal(wire.state, state); assert.deepEqual([state.compressedCount, state.buffer[2047]], [177, 0xcc]); two.dispose();
});


test('공용 삭제: client SID 진단 복귀 뒤 현재 헤더를 쓰고 가상+58의 두 번째 진단으로 이어진다', () => {
    const s = storage(); const sid = s.factory.create(75, 2); const calls = []; const state = s.gameAux.squidTransmission; state.buffer.fill(0xa5);
    s.bindDisplayDiagnostics({ value: null }, (expression, file, line) => { calls.push([expression, file, line]); if (line === 0x2d8) state.forms.deletion.message = 0x17; });
    assert.throws(() => s.transmission.transmit(sid, -1, -2, 0, 0), /broadcast.*미연결/);
    assert.deepEqual(calls.map((call) => call[2]), [0x2d8, 0x2f6]); assert.equal(state.buffer[1], 0x17); assert.equal(get(state.buffer, 4, 2), sid); assert.equal(state.buffer[8], 0xa5);
});

test('공용 ProcessForm: 일반 Regular의 실제0 writer는 버퍼와 대상 연결 장치를 건드리지 않는다', () => {
    const s = storage(); const w = world(s); const regular = w.core().processes.attachRegular(new RegularProcess(7, 1), w.create(75, 0), false, 0x50); const state = s.gameAux.squidTransmission;
    const lines = []; s.bindDisplayDiagnostics({ value: null }, (expression, file, line) => lines.push(line));
    state.buffer.fill(0xa5); const before = state.buffer.slice(); s.transmission.sendTo(regular.form, 7, 0xffffffff); assert.deepEqual(state.buffer, before); assert.deepEqual(lines, [0x2d8]); w.dispose();
});

test('실제 World 송신: 구성 실패의 자기 대여 반납은 앞선 Pop 대여/다음 World를 훼손하지 않는다', () => {
    const s = storage(); const calls = []; const r = popRuntime({ transmit: (sid, flags) => calls.push([sid, flags]) }); const use = s.bindPopRuntime(r);
    assert.throws(() => world(s), /동일 실제 Pop/); assert.equal(s.borrowed, false); s.transmitPop(15123, 9); assert.deepEqual(calls, [[15123, 9]]);
    use.release(); const w = world(s); w.dispose();
});
