// cpppj1320 MissionConnectionFlushTests.cpp의4검사와 직접 수신의 정상/예외 대여를 옮긴다(WB04-E620).
// 실제 내부 미션 전송만 사용하며 LAN·게임 실행·승리를 합성하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { CompositeMissionTransport, CompositeTransportRole } from '../www/core/composite-mission-transport.js';
import { RawConnectionRegistry } from '../www/core/raw-connection-registry.js';
import { PacketConnection } from '../www/core/raw-packet-dispatch.js';
import { receiveConnectionEnded } from '../www/core/raw-connection-ended.js';
import { LocalMissionTransport } from '../www/core/local-mission-transport.js';

// 두 미션이 실제로 공유하는 Player9·서버·시계·활동 표와 종료 통지 관측이다.
function globals() { return { players: new Uint8Array(9 * 180), server: true, now: 0, frames: new Uint32Array(22), ended: 0, events: [] }; }
// 원문 내부 bootstrap만 만들며 로그인 본문은 이 검사에서 선택되면 거부한다.
function transport(registry, state) {
    const next = CompositeMissionTransport.create(CompositeTransportRole.InternalMission, state.players, () => state.server, {
        now: () => ++state.now, record: (node, bytes, length) => state.events.push(['record', node.sender, bytes[1], length]),
        terminate: (node) => { node.flags |= 0x40; }, close: (node) => state.events.push(['close', node.sender]), release: (node) => state.events.push(['release', node.sender]),
    }, registry, state);
    next.dispatch.register(0x27, () => { throw new Error('실제 로그인 본문은 이번 검사 범위 밖'); });
    next.dispatch.register(2, (sender, node) => {
        receiveConnectionEnded(sender | 0, registry.tokenFor(node), { server: () => state.server, synchronization: registry.cleanup().synchronization, phase: 8, nextPhase: { value: 0 }, frames: state.frames, players: () => state.players }, {
            disconnect: (id) => registry.disconnect(id), broadcastPlayer: (owner) => { assert.equal(owner, 1); state.ended += 1; },
        });
    });
    return next;
}

test('MissionConnectionFlush_NewMissionAcceptsPreviousSourceNode: 이전 끝 종료 통지 뒤 같은 번호를 재사용', () => {
    const registry = new RawConnectionRegistry(); const state = globals(); let previous = transport(registry, state); let next; // 실제 서로 다른 Source다.
    try {
        previous.serve(6799); previous.connect(new Uint8Array(20), 6799); previous.poll(); previous.assign(-1, 1);
        const oldToken = registry.cleanup().transceivers[1]; assert.ok(oldToken); assert.ok(previous.connection(1));
        registry.disconnect(9); assert.equal(registry.cleanup().transceivers[1], oldToken); previous = null;
        next = transport(registry, state); next.serve(6799); next.connect(new Uint8Array(20), 6799);
        next.flush(); assert.equal(registry.cleanup().transceivers[1], oldToken); assert.ok(next.connection(9)); assert.equal(state.ended, 0);
        next.poll(); assert.equal(registry.cleanup().transceivers[1], 0); assert.equal(state.ended, 1); assert.equal(registry.owns(oldToken), false);
        next.assign(-1, 1); assert.ok(next.connection(1)); assert.notEqual(registry.cleanup().transceivers[1], oldToken);
        next.closeRoom(); assert.equal(registry.cleanup().transceivers[1], 0); assert.equal(state.ended, 1);
    } finally { previous?.dispose(); next?.closeRoom(); registry.shutdownTrackedDevices(); }
});

test('MissionConnectionFlush_DirectCallbackKeepsSelectedOwnerAndFunction: 자기 반납 중 선택 함수 보유', () => {
    const registry = new RawConnectionRegistry(); const node = new PacketConnection(); const calls = []; let token; // 실제 등록한 장치만 호출한다.
    token = registry.track(node, () => calls.push('close'), () => calls.push('release'), undefined, undefined, undefined, () => {
        registry.forget(token); registry.bindings.set(token, { flush: () => calls.push('replacement') }); calls.push('selected');
    });
    registry.register(1, token); registry.flushDirect(token); assert.deepEqual(calls, ['selected']);
    registry.forget(token); assert.equal(registry.owns(token), false);
});

test('다른 Source의 실제 buffered flush: 새 조정자의 flush/poll이 기존 끝의 누적 프레임을 전달', () => {
    const registry = new RawConnectionRegistry(); const state = globals(); const previous = transport(registry, state); let next; const received = [];
    try {
        previous.serve(6799); previous.connect(new Uint8Array(20), 6799); previous.poll(); previous.assign(-1, 1); state.players[180 + 0x50] |= 1;
        previous.dispatch.register(0x24, (sender, node, bytes) => received.push([sender, node.sender, bytes[4]]));
        assert.equal(previous.send(1, Uint8Array.of(1, 0x24, 5, 0, 0x7b)), true); assert.deepEqual(received, []);
        next = transport(registry, state); // 새 자기 끝을 열지 않고 같은 전역의 이전 두 실제 끝을 순회한다.
        next.flush(); assert.equal(next.poll(), 1); assert.deepEqual(received, [[9, 9, 0x7b]]); assert.equal(next.poll(), 0);
    } finally { next?.closeRoom(); previous.closeRoom(); registry.shutdownTrackedDevices(); }
});

test('MissionConnectionFlush_DirectReceiveKeepsOwnerAndActualResult: 자기 반납 뒤 실제 소비/sentinel 반환', () => {
    const registry = new RawConnectionRegistry(); const node = new PacketConnection(); const result = { consumed: true, again: false }; let token;
    token = registry.track(node, () => assert.fail('소멸 선택 없음'), () => assert.fail('반납 선택 없음'), undefined, undefined, undefined, undefined, () => { registry.forget(token); return result; });
    assert.deepEqual(registry.receiveDirect(token), result); assert.equal(registry.owns(token), false);
});

test('MissionConnectionFlush_MissingDeviceStillFailsExplicitly: 누락된 장치는 선택 토큰과 함께 거부', () => {
    const registry = new RawConnectionRegistry(); const node = new PacketConnection(); const token = registry.track(node, () => assert.fail('소멸 선택 없음'), () => assert.fail('반납 선택 없음'));
    assert.throws(() => registry.flushDirect(token), new RegExp(`token=${token}`)); assert.throws(() => registry.receiveDirect(token), new RegExp(`token=${token}`)); assert.equal(registry.owns(token), true);
    registry.forget(token); assert.throws(() => registry.flushDirect(token), new RegExp(`token=${token}`)); assert.throws(() => registry.receiveDirect(token), new RegExp(`token=${token}`));
});

test('직접 이전 Source 수신: callback 중 재진입 거부·예외 뒤 자기 Poll 대여 반납', () => {
    const registry = new RawConnectionRegistry(); const state = globals(); // 직접 진입할 실제 내부 전송과 외부 경계다.
    const source = new LocalMissionTransport(state.players, true, { now: () => ++state.now, record: (...args) => state.events.push(args), terminate: (node) => { node.flags |= 0x40; }, close: () => state.events.push('close'), release: () => state.events.push('release') }, registry);
    try {
        source.registerLogin(() => { throw new Error('로그인 선택 없음'); }); source.open(6799, false); source.connect(); const token = registry.cleanup().transceivers[9];
        source.dispatch.register(1, () => { assert.throws(() => source.beginPoll(), /중첩/); throw new Error('연결 통지 실패'); });
        assert.throws(() => registry.receiveDirect(token), /연결 통지 실패/);
        source.beginPoll(); source.endPoll(); // 직접 진입만 반납됐으므로 다시 실제 Poll을 빌릴 수 있다.
        assert.equal(registry.owns(token), true);
    } finally { source.closeAll(); registry.shutdownTrackedDevices(); }
});
