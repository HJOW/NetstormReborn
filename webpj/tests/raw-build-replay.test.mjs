// RawBuildReplay의 실행/라벨 물리와 Client 현재 객체 수명을 C++ 소스 계약으로 대조한다(WB04-E601).
// FILE 판독/Host 설치·실제 게임 실행 또는 새 원본 관측이 아니다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { BuildReplayState, BUILD_REPLAY_STATE_BYTES, BUILD_REPLAY_EVENT_LIMIT, runBuildReplayFrame, isBuildReplayLabel, notifyBuildReplayLabel } from '../www/core/raw-build-replay.js';
import { ClientBuildReplayGlobals, ClientBuildReplayContext, currentBuildReplayIsLabel, notifyCurrentBuildReplay } from '../www/core/client-build-replay.js';

// 미선택 장치가 실행되면 실패한다. 성공 기본값으로 쓰지 않는다.
function forbidden() { throw new Error('선택하지 않은 실제 하위'); }
// 현재 원문 행의 이름만 NUL 포함20BYTE에 쓴다. UTF-8 변환을 하지 않는 바이트 문자열 입력이다.
function name(event, text) { event.name.fill(0); event.name.set(Array.from(text, (char) => char.charCodeAt(0))); }
// 선택된 실행 입력만 만드는 실제 상태다. 이벤트 행/패킷은 제품의 같은 물리 구현이다.
function scene() { const state = new BuildReplayState(); state.count = 1; const event = state.events[0]; name(event, 'testType'); event.x = 1.25; event.y = 2.5; event.delay = 0.5; return { state, event, playing: { value: 7 }, packet: new Uint8Array(32).fill(0xa5) }; }
// 같은 바이트의 현재 DWORD를 읽거나 쓴다.
function read(raw, offset) { return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(offset, true); }
function put(raw, offset, value) { new DataView(raw.buffer, raw.byteOffset, raw.byteLength).setUint32(offset, value, true); }

test('BuildReplay: 실제200행/40BYTE와 double·두DWORD는 같은8016BYTE이고 미판독 tail을 보존한다', () => {
    const bytes = new Uint8Array(BUILD_REPLAY_STATE_BYTES + 7).fill(0xa5); const raw = bytes.subarray(3); const state = new BuildReplayState(raw); assert.equal(state.events.length, BUILD_REPLAY_EVENT_LIMIT); assert.equal(state.events[1].raw.byteOffset - state.events[0].raw.byteOffset, 40); assert.equal(state.raw.buffer, bytes.buffer); assert.equal(state.raw.byteOffset, 3); assert.equal(state.events[199].name[19], 0xa5); state.events[0].x = 1.25; assert.equal(read(raw, 20), 0x3fa00000); state.events[0].argument = 0xffffffff; assert.equal(state.events[0].argument, -1); state.previousTime = 123.5; state.current = 0x100000001; state.count = 3; assert.equal(new DataView(raw.buffer, raw.byteOffset).getFloat64(8000, true), 123.5); assert.equal(read(raw, 8008), 1); assert.equal(read(raw, 8012), 3); assert.equal(bytes.at(-1), 0xa5); assert.throws(() => new BuildReplayState(new Uint8Array(8015)), /8016BYTE/);
});

test('BuildReplay: 호스트 초기0은 상태/행 전체의 명시0이며 단정도 저장을 사용한다', () => {
    const state = new BuildReplayState(); assert.equal(state.raw.some((byte) => byte !== 0), false); state.events[0].delay = 0.1; assert.equal(state.events[0].delay, Math.fround(0.1)); assert.throws(() => { state.events = []; }, TypeError); assert.throws(() => { state.events[0].raw = new Uint8Array(40); }, TypeError);
});

test('BuildReplay Run: 종료는 playing0을 Kill보다 먼저 쓰며 물리 밖 current도 종료 비교가 먼저다', () => {
    const s = scene(); s.state.current = 200; s.state.count = 199; const stop = new Error('실제 Kill 경계'); assert.throws(() => runBuildReplayFrame(s.state, s.playing, s.packet, { now: forbidden, kill: () => { assert.equal(s.playing.value, 0); throw stop; } }), (error) => error === stop); assert.equal(s.state.current, 200);
});

test('BuildReplay Run: current/count는 원문의 signed32 비교이며 실제200행 범위는 소비 때 검사한다', () => {
    const s = scene(); s.state.count = 0xffffffff; let killed = 0; runBuildReplayFrame(s.state, s.playing, s.packet, { kill: () => { killed += 1; } }); assert.equal(killed, 1); s.state.current = 0xffffffff; s.state.count = 0; s.playing.value = 8; assert.throws(() => runBuildReplayFrame(s.state, s.playing, s.packet, { now: forbidden }), /현재 이벤트 물리 범위/); assert.equal(s.playing.value, 8);
});

test('BuildReplay Run: equality/NaN 기한에서는 소비하지 않고 now를 추가 조회하지 않는다', () => {
    const s = scene(); let reads = 0; const hooks = { now: () => { reads += 1; return 0.5; }, validPoint: forbidden }; runBuildReplayFrame(s.state, s.playing, s.packet, hooks); assert.equal(reads, 1); assert.equal(s.state.current, 0); hooks.now = () => Number.NaN; runBuildReplayFrame(s.state, s.playing, s.packet, hooks); s.event.delay = Number.NaN; hooks.now = () => 100; runBuildReplayFrame(s.state, s.playing, s.packet, hooks); assert.equal(s.state.current, 0); assert.equal(s.state.previousTime, 0);
});

test('BuildReplay Run: 한 이벤트/25BYTE만 송신하며 low폭과 미정 품질/패킷 꼬리를 보존한다', () => {
    const s = scene(); s.state.count = 2; s.event.argument = 0x12345678; s.event.rotation = -2; const calls = []; runBuildReplayFrame(s.state, s.playing, s.packet, { now: () => 100, validPoint: (x, y) => { calls.push(['point', x, y]); return true; }, constructPacket: (packet) => { packet.set([1, 0x3e, 25, 0]); packet.fill(0, 17, 25); }, findType: (value) => { calls.push(['name', value]); return -1; }, send: (target, packet) => { assert.equal(packet.length, 25); assert.equal(packet.buffer, s.packet.buffer); calls.push(['send', target]); } }); assert.equal(s.state.current, 1); assert.equal(s.state.previousTime, 100); assert.equal(s.playing.value, 7); assert.equal(s.packet[4], 255); assert.equal(read(s.packet, 5), 0x3fa00000); assert.equal(read(s.packet, 9), 0x40200000); assert.deepEqual([...s.packet.subarray(13, 17)], [0x78, 0x56, 0xfe, 0xa5]); assert.equal(s.packet[25], 0xa5); assert.deepEqual(calls, [['point', 1.25, 2.5], ['name', 'testType'], ['send', 9]]);
});

test('BuildReplay Run: 라벨도 시각/인덱스를 먼저 소비하고 패킷/이름은 조회하지 않는다', () => {
    const s = scene(); s.event.name.fill(0x41); const times = [2, 3]; runBuildReplayFrame(s.state, s.playing, new Uint8Array(0), { now: () => times.shift(), validPoint: () => false, constructPacket: forbidden, findType: forbidden }); assert.equal(s.state.previousTime, 3); assert.equal(s.state.current, 1); assert.equal(s.playing.value, 7); assert.equal(times.length, 0);
});

test('BuildReplay Run: 짧은 패킷/미연결 constructor 오류도 앞선 시각/소비 효과를 되돌리지 않는다', () => {
    const s = scene(); assert.throws(() => runBuildReplayFrame(s.state, s.playing, new Uint8Array(24), { now: () => 2, validPoint: () => true, constructPacket: forbidden }), /25BYTE 부족/); assert.equal(s.state.current, 1); assert.equal(s.state.previousTime, 2); const second = scene(); assert.throws(() => runBuildReplayFrame(second.state, second.playing, second.packet, { now: () => 3, validPoint: () => true }), /실제 하위 미연결/); assert.equal(second.state.current, 1); assert.equal(second.state.previousTime, 3);
});

test('BuildReplay Run: 같은 행 span을 효과 뒤 재조회하고 Y/type/X/WORD/회전 비트를 직접 쓴다', () => {
    const s = scene(); let timeReads = 0; const calls = []; runBuildReplayFrame(s.state, s.playing, s.packet, { now: () => { timeReads += 1; if (timeReads === 2) s.event.x = 3; return timeReads; }, validPoint: (x, y) => { calls.push([x, y]); s.event.x = 4; return true; }, constructPacket: () => { name(s.event, 'afterHeader'); s.event.y = 5; s.state.current = 19; }, findType: (text) => { assert.equal(text, 'afterHeader'); put(s.event.raw, 24, 0x7fa12345); put(s.event.raw, 20, 0x80000000); s.event.argument = 0xabcdef01; return 0x123; }, send: (target, packet) => { assert.equal(target, 9); assert.equal(read(packet, 9), 0x7fa12345); assert.equal(packet[4], 0x23); assert.equal(read(packet, 5), 0x80000000); assert.deepEqual([...packet.subarray(13, 15)], [1, 0xef]); } }); assert.deepEqual(calls, [[3, 2.5]]); assert.equal(s.state.current, 19); assert.equal(s.state.previousTime, 2);
});

test('BuildReplay Run: 이름 NUL 오류는 실제 header 실행 뒤이며 TYPE/송신은 실행하지 않는다', () => {
    const s = scene(); s.event.name.fill(0x41); let headers = 0; assert.throws(() => runBuildReplayFrame(s.state, s.playing, s.packet, { now: () => 2, validPoint: () => true, constructPacket: () => { headers += 1; }, findType: forbidden, send: forbidden }), /이름 NUL 누락/); assert.equal(headers, 1); assert.equal(s.state.current, 1);
});

test('BuildReplay Label: count 뒤 실제 tail을 읽으며 RL 접두/이름 대신 현재 보드 판정만 사용한다', () => {
    const s = scene(); s.state.count = 0; s.state.current = 4; s.state.events[4].x = 7; s.state.events[4].y = 8; s.state.events[4].name.fill(0x41); const calls = []; assert.equal(isBuildReplayLabel(s.state, { validPoint: (x, y) => { calls.push([x, y]); return false; } }), true); assert.deepEqual(calls, [[7, 8]]); s.state.current = 200; assert.throws(() => isBuildReplayLabel(s.state, { validPoint: forbidden }), /물리 범위/);
});

test('BuildReplay Notify: NULL callback은 실제 current/좌표 조회보다 먼저 생략한다', () => {
    const state = new BuildReplayState(); state.current = 999; notifyBuildReplayLabel(state, 3, { validPoint: forbidden }); assert.equal(state.current, 999);
});

test('BuildReplay Notify: 현재 보드 밖이면 접두 검사 없이2BYTE 뒤 이름과 unsigned SID를 전달한다', () => {
    const s = scene(); name(s.event, 'XXlabel'); const calls = []; notifyBuildReplayLabel(s.state, 0x100000005, { validPoint: () => false, label: (...args) => calls.push(args) }); notifyBuildReplayLabel(s.state, 6, { validPoint: () => true, label: (...args) => calls.push(args) }); assert.deepEqual(calls, [['label', 5], ['', 6]]); s.event.name.fill(0x41); s.event.name[0] = 0; assert.throws(() => notifyBuildReplayLabel(s.state, 1, { validPoint: () => false, label: forbidden }), /NUL 누락/);
});

test('BuildReplay Notify: 좌표 효과 뒤 같은 행의 현재 이름/현재 callback을 선택한다', () => {
    const s = scene(); const calls = []; const hooks = { label: forbidden, validPoint: () => { name(s.event, 'RLchanged'); s.state.current = 3; hooks.label = (...args) => calls.push(args); return false; } }; notifyBuildReplayLabel(s.state, 9, hooks); assert.deepEqual(calls, [['changed', 9]]); assert.equal(s.state.current, 3);
});

test('Client BuildReplay: 최초 BSS/빈 weak와 명시 context0은 효과 없이 생성한다', () => {
    const globals = new ClientBuildReplayGlobals(); const context = new ClientBuildReplayContext(); assert.equal(globals.playing, 0); assert.equal(globals.current, null); assert.equal(context.state.raw.some((byte) => byte !== 0), false); assert.equal(context.disposed, false); globals.playing = 0x100000001; assert.equal(globals.playing, 1);
});

test('Client BuildReplay: NULL/만료 current는 빈 이벤트가 아니며 반납은 playing을 지우지 않는다', () => {
    assert.throws(() => currentBuildReplayIsLabel(null), /전역 미연결/); const globals = new ClientBuildReplayGlobals(); assert.throws(() => currentBuildReplayIsLabel(globals), /호스트 수명 종료/); const context = new ClientBuildReplayContext(); globals.current = new WeakRef(context); globals.playing = 1; context.dispose(); assert.throws(() => notifyCurrentBuildReplay(globals, 1), /호스트 수명 종료/); assert.equal(globals.playing, 1);
});

test('Client BuildReplay: 매 호출 현재 context를 고르고 호출 중 교체/반납은 현재 stack pin을 끊지 않는다', () => {
    const globals = new ClientBuildReplayGlobals(); const first = new ClientBuildReplayContext(); const second = new ClientBuildReplayContext(); const calls = []; first.hooks.validPoint = () => { calls.push('first'); globals.current = new WeakRef(second); first.dispose(); return false; }; second.hooks.validPoint = () => { calls.push('second'); return true; }; globals.current = new WeakRef(first); assert.equal(currentBuildReplayIsLabel(globals), true); assert.equal(currentBuildReplayIsLabel(globals), false); assert.deepEqual(calls, ['first', 'second']);
});

test('Client BuildReplay: Notify도 현재 context를 먼저 요구한 뒤 실제 라벨/NULL callback 몸체를 실행한다', () => {
    const globals = new ClientBuildReplayGlobals(); assert.throws(() => notifyCurrentBuildReplay(globals, 1), /수명 종료/); const context = new ClientBuildReplayContext(); globals.current = new WeakRef(context); context.state.current = 999; notifyCurrentBuildReplay(globals, 1); context.state.current = 0; name(context.state.events[0], 'RLactual'); const calls = []; context.hooks = { validPoint: () => false, label: (...args) => calls.push(args) }; notifyCurrentBuildReplay(globals, 7); assert.deepEqual(calls, [['actual', 7]]);
});
