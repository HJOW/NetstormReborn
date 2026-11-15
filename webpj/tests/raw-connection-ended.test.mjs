// 연결 종료 통지 raw-connection-ended.js(RawConnectionEnded, 원본 004b6660)를 cpppj 소스 계약으로 대조한다(WB04-E559).
// 원본 관측 fixture 가 없는 소단위이므로 동기화 정리·서버 실패 저장·연결 종료 선택·Player flags 세 번 store·방송·활동 프레임·다음 상태 예약과 호출 순서·선택 장치 요구를 직접 단언한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E559)
import test from 'node:test';
import assert from 'node:assert/strict';
import { receiveConnectionEnded } from '../www/core/raw-connection-ended.js';

// 서로 구별되는 9행(180바이트) Player 기록이다(flags DWORD 위치 +0x50 은 owner 별로 0xa5a5_0041 에 owner 를 섞은 값). (2026-11-15 WB04-E559)
function makePlayers() {
    const players = Uint8Array.from({ length: 9 * 180 }, (_, index) => (index * 13 + 5) & 0xff);
    for (let owner = 0; owner < 9; owner += 1) new DataView(players.buffer).setUint32(owner * 180 + 0x50, 0xa5a50041 + owner, true);
    return players;
}
// 완전한 전역·장치 한 벌이다. trace 에 호출 순서를 남긴다. (2026-11-15 WB04-E559)
function makeScene(overrides = {}) {
    const trace = [];
    const players = makePlayers();
    const globals = {
        server: { value: false },
        synchronization: { value: 0x1234 },
        phase: { value: 23 },
        nextPhase: { value: 5 },
        frames: Uint32Array.from({ length: 22 }, (_, index) => 100 + index),
        players: () => { trace.push('players'); return players; },
        ...overrides.globals
    };
    const hooks = {
        saveServerFailure: () => trace.push('save'),
        disconnect: (sender) => trace.push(`disconnect,${sender}`),
        broadcastPlayer: (owner) => trace.push(`broadcast,${owner}`),
        ...overrides.hooks
    };
    return { trace, players, globals, hooks };
}
// owner 의 flags DWORD 를 읽는다. (2026-11-15 WB04-E559)
const flagsOf = (players, owner) => new DataView(players.buffer).getUint32(owner * 180 + 0x50, true);

test('동기화: 같은 연결이 끝나면 동기화 DWORD 를 0 으로 만들고 다른 연결은 건드리지 않는다', () => {
    const same = makeScene();
    receiveConnectionEnded(4, 0x1234, same.globals, same.hooks);
    assert.equal(same.globals.synchronization.value, 0);
    const other = makeScene();
    receiveConnectionEnded(4, 0x1235, other.globals, other.hooks);
    assert.equal(other.globals.synchronization.value, 0x1234);
    // 동기화 0 인 상태에서 연결 0 도 같은 연결이며 값은 그대로 0 이다. token 은 unsigned 로 비교한다.
    const zero = makeScene({ globals: { synchronization: { value: 0 } } });
    receiveConnectionEnded(4, 0, zero.globals, zero.hooks);
    assert.equal(zero.globals.synchronization.value, 0);
    const wide = makeScene({ globals: { synchronization: { value: 0xfffffff0 } } });
    receiveConnectionEnded(4, -16, wide.globals, wide.hooks);
    assert.equal(wide.globals.synchronization.value, 0);
});

test('클라이언트: 종료를 요청하고 서버(9번) 끝은 먼저 서버 실패를 저장한다', () => {
    const scene = makeScene();
    receiveConnectionEnded(3, 7, scene.globals, scene.hooks);
    assert.deepEqual(scene.trace, ['disconnect,3']);
    // flags/활동 프레임/다음 상태는 서버가 아니면 쓰지 않는다.
    assert.deepEqual(scene.players, makePlayers());
    assert.equal(scene.globals.frames[3], 103);
    assert.equal(scene.globals.nextPhase.value, 5);
    // 9번: 서버 실패 저장 → 연결 종료.
    const server9 = makeScene();
    receiveConnectionEnded(9, 7, server9.globals, server9.hooks);
    assert.deepEqual(server9.trace, ['save', 'disconnect,9']);
    // 저장 중 서버가 되었다면 연결 종료·flags·다음 상태 없이 즉시 반환한다.
    const promoted = makeScene({ hooks: { saveServerFailure: undefined } });
    promoted.hooks.saveServerFailure = () => { promoted.trace.push('save'); promoted.globals.server.value = true; };
    receiveConnectionEnded(9, 7, promoted.globals, promoted.hooks);
    assert.deepEqual(promoted.trace, ['save']);
    assert.deepEqual(promoted.players, makePlayers());
    assert.equal(promoted.globals.nextPhase.value, 5);
});

test('서버: 9번 자기 끝은 닫지 않고 1..8 번만 flags 세 번 store·방송·활동 프레임 0 으로 처리한다', () => {
    const scene = makeScene({ globals: { server: { value: true } } });
    receiveConnectionEnded(4, 7, scene.globals, scene.hooks);
    // 종료 → 현재 Player base 를 세 번 빌려 flags 를 쓰고 → 방송 → 활동 프레임 0.
    assert.deepEqual(scene.trace, ['disconnect,4', 'players', 'players', 'players', 'broadcast,4']);
    const before = 0xa5a50041 + 4;
    assert.equal(flagsOf(scene.players, 4), ((before | 0x100) & ~1 & ~0x40) >>> 0);
    assert.equal(scene.globals.frames[4], 0);
    // 다른 Player 의 flags 와 인접 바이트·다른 활동 프레임은 그대로이다.
    const expected = makePlayers();
    new DataView(expected.buffer).setUint32(4 * 180 + 0x50, flagsOf(scene.players, 4), true);
    assert.deepEqual(scene.players, expected);
    assert.equal(scene.globals.frames[3], 103);
    assert.equal(scene.globals.frames[5], 105);
    // 서버 자신의 끝(9)은 이 부모에서 닫지 않는다.
    const self = makeScene({ globals: { server: { value: true } } });
    receiveConnectionEnded(9, 7, self.globals, self.hooks);
    assert.deepEqual(self.trace, []);
    assert.deepEqual(self.players, makePlayers());
    // 서버의 0번과 음수 sender 는 종료만 하고 flags 는 쓰지 않는다. 8번은 마지막 처리 대상이다.
    for (const [sender, handled] of [[0, false], [-1, false], [1, true], [8, true], [10, false]]) {
        const edge = makeScene({ globals: { server: { value: true } } });
        receiveConnectionEnded(sender, 7, edge.globals, edge.hooks);
        assert.equal(edge.trace.includes(`broadcast,${sender}`), handled, `sender ${sender}`);
        assert.equal(edge.trace[0], `disconnect,${sender}`);
    }
});

test('서버 flags: 세 store 는 각각 현재 Player base 를 다시 빌리고 비정렬 DWORD 만 바꾼다', () => {
    const arrays = [makePlayers(), makePlayers(), makePlayers()];
    for (const [index, array] of arrays.entries()) new DataView(array.buffer).setUint32(2 * 180 + 0x50, [0x00000041, 0x00000100 | 1, 0xffffffff][index], true);
    let call = 0;
    const scene = makeScene({ globals: { server: { value: true }, players: () => arrays[Math.min(call++, 2)] } });
    receiveConnectionEnded(2, 7, scene.globals, scene.hooks);
    // 첫 base 에는 0x100 설정만, 둘째에는 비트 0 해제만, 셋째에는 비트 0x40 해제만 반영된다(서로 다른 현재 base).
    assert.equal(flagsOf(arrays[0], 2), 0x00000141);
    assert.equal(flagsOf(arrays[1], 2), 0x00000100);
    assert.equal(flagsOf(arrays[2], 2), 0xffffffbf);
    // 전체 DWORD 가 아닌 인접 바이트는 어느 base 에서도 보존된다.
    for (const array of arrays) assert.equal(array[2 * 180 + 0x4f], makePlayers()[2 * 180 + 0x4f]);
    // 같은 Player 의 바이트 오프셋이 비정렬이어도(180 은 4 의 배수가 아님) 해당 4바이트만 바뀐다.
    assert.equal(arrays[0][2 * 180 + 0x54], makePlayers()[2 * 180 + 0x54]);
});

test('서버 방송이 실패하면 앞선 flags 는 남고 활동 프레임은 아직 지우지 않는다', () => {
    const scene = makeScene({ globals: { server: { value: true } }, hooks: { broadcastPlayer: () => { throw new Error('송신 실패'); } } });
    assert.throws(() => receiveConnectionEnded(6, 7, scene.globals, scene.hooks), /송신 실패/);
    assert.equal(flagsOf(scene.players, 6), (((0xa5a50041 + 6) | 0x100) & ~1 & ~0x40) >>> 0);
    assert.equal(scene.globals.frames[6], 106);
});

test('다음 상태: 메타(10) 연결이 대기(23)에서 끝나고 아직 24/12 가 아니면 20 으로 예약만 한다', () => {
    const base = (next, phase, sender) => {
        const scene = makeScene({ globals: { phase: { value: phase }, nextPhase: { value: next } } });
        receiveConnectionEnded(sender, 7, scene.globals, scene.hooks);
        return scene.globals.nextPhase.value;
    };
    assert.equal(base(5, 23, 10), 20);
    assert.equal(base(0, 23, 10), 20);
    assert.equal(base(23, 23, 10), 20);
    assert.equal(base(24, 23, 10), 24);
    assert.equal(base(12, 23, 10), 12);
    assert.equal(base(5, 22, 10), 5);
    assert.equal(base(5, 24, 10), 5);
    assert.equal(base(5, 23, 9), 5);
    assert.equal(base(5, 23, 11), 5);
    // 서버 권한과 무관하게 같은 규칙이다(서버에서도 메타 10 은 종료만 하고 예약한다).
    const server = makeScene({ globals: { server: { value: true }, phase: { value: 23 } } });
    receiveConnectionEnded(10, 7, server.globals, server.hooks);
    assert.equal(server.globals.nextPhase.value, 20);
    assert.deepEqual(server.trace, ['disconnect,10']);
});

test('현재값 재조회: 장치가 서버·단계를 바꾸면 각 원문 위치에서 바뀐 값을 읽는다', () => {
    // 연결 종료 장치가 서버를 켜면 같은 호출의 flags 처리가 서버 경로로 이어진다.
    const flip = makeScene({ hooks: {} });
    flip.hooks.disconnect = (sender) => { flip.trace.push(`disconnect,${sender}`); flip.globals.server.value = true; };
    receiveConnectionEnded(3, 7, flip.globals, flip.hooks);
    assert.deepEqual(flip.trace, ['disconnect,3', 'players', 'players', 'players', 'broadcast,3']);
    // 종료 장치가 단계를 바꾸면 마지막 예약은 바뀐 값으로 판정한다.
    const phase = makeScene({ globals: { server: { value: true }, phase: { value: 1 } } });
    phase.hooks.disconnect = () => { phase.globals.phase.value = 23; };
    receiveConnectionEnded(10, 7, phase.globals, phase.hooks);
    assert.equal(phase.globals.nextPhase.value, 20);
    // 읽기 전용 전역은 함수·단순 값으로도 받는다.
    const byFunction = makeScene({ globals: { server: () => true, phase: 23 } });
    receiveConnectionEnded(10, 7, byFunction.globals, byFunction.hooks);
    assert.equal(byFunction.globals.nextPhase.value, 20);
    assert.deepEqual(byFunction.trace, ['disconnect,10']);
});

test('선택된 장치만 요구하고 누락은 앞선 효과를 유지한 채 명시 오류이다', () => {
    // 클라이언트 비9 는 종료 장치만, 서버 자기 끝은 장치 없이도 끝난다.
    const clientOnly = makeScene({ hooks: { saveServerFailure: undefined, broadcastPlayer: undefined } });
    assert.doesNotThrow(() => receiveConnectionEnded(3, 7, clientOnly.globals, clientOnly.hooks));
    const serverSelf = makeScene({ globals: { server: { value: true }, players: undefined }, hooks: { saveServerFailure: undefined, disconnect: undefined, broadcastPlayer: undefined } });
    assert.doesNotThrow(() => receiveConnectionEnded(9, 7, serverSelf.globals, serverSelf.hooks));
    // 종료 장치가 없으면 동기화 정리 뒤 명시 오류이다.
    const noDisconnect = makeScene({ hooks: { disconnect: undefined } });
    assert.throws(() => receiveConnectionEnded(3, 0x1234, noDisconnect.globals, noDisconnect.hooks), /미연결/);
    assert.equal(noDisconnect.globals.synchronization.value, 0);
    // 서버 9번 저장 장치 누락은 종료 전에 거부한다.
    const noSave = makeScene({ hooks: { saveServerFailure: undefined } });
    assert.throws(() => receiveConnectionEnded(9, 7, noSave.globals, noSave.hooks), /미연결/);
    assert.deepEqual(noSave.trace, []);
    // 서버 1..8 에서 방송 장치가 없으면 flags 세 store 는 이미 쓰였고 활동 프레임은 남는다.
    const noBroadcast = makeScene({ globals: { server: { value: true } }, hooks: { broadcastPlayer: undefined } });
    assert.throws(() => receiveConnectionEnded(5, 7, noBroadcast.globals, noBroadcast.hooks), /미연결/);
    assert.equal(flagsOf(noBroadcast.players, 5), (((0xa5a50041 + 5) | 0x100) & ~1 & ~0x40) >>> 0);
    assert.equal(noBroadcast.globals.frames[5], 105);
    // Player base 장치가 없으면 첫 store 에서 거부한다.
    const noPlayers = makeScene({ globals: { server: { value: true }, players: undefined } });
    assert.throws(() => receiveConnectionEnded(5, 7, noPlayers.globals, noPlayers.hooks), /미연결/);
});

test('Player flags 물리 범위: 한 DWORD 가 들어가는 길이까지만 허용한다', () => {
    const exact = makeScene({ globals: { server: { value: true } } });
    const small = new Uint8Array(8 * 180 + 0x50 + 4);
    exact.globals.players = () => small;
    assert.doesNotThrow(() => receiveConnectionEnded(8, 7, exact.globals, exact.hooks));
    assert.equal(new DataView(small.buffer).getUint32(8 * 180 + 0x50, true), 0x100);
    const short = makeScene({ globals: { server: { value: true } } });
    short.globals.players = () => new Uint8Array(8 * 180 + 0x50 + 3);
    assert.throws(() => receiveConnectionEnded(8, 7, short.globals, short.hooks), RangeError);
    // 물리 오류 때 방송과 활동 프레임은 건드리지 않는다.
    assert.deepEqual(short.trace, ['disconnect,8']);
    assert.equal(short.globals.frames[8], 108);
});

// 읽기 전용 전역을 함수로 줄 때 반환값(거짓 포함)을 그대로 쓰며 함수 객체 자체를 값으로 보지 않는다. (2026-11-15 WB04-E559 변형 확인에서 추가)
test('읽기 전용 전역의 함수 형태는 반환값을 읽는다', () => {
    const client = makeScene({ globals: { server: () => false, phase: () => 22 } });
    receiveConnectionEnded(10, 7, client.globals, client.hooks);
    // 서버가 아니므로 종료만 하고, 단계가 22 이므로 다음 상태는 그대로이다.
    assert.deepEqual(client.trace, ['disconnect,10']);
    assert.equal(client.globals.nextPhase.value, 5);
    const waiting = makeScene({ globals: { server: () => false, phase: () => 23 } });
    receiveConnectionEnded(10, 7, waiting.globals, waiting.hooks);
    assert.equal(waiting.globals.nextPhase.value, 20);
    // 상자 형태의 거짓 값도 마찬가지이다.
    const boxed = makeScene({ globals: { server: { value: false }, phase: { value: 0 } } });
    receiveConnectionEnded(4, 7, boxed.globals, boxed.hooks);
    assert.deepEqual(boxed.trace, ['disconnect,4']);
});
