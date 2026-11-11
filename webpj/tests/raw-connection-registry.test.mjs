// 연결 등록소(cpppj `o/RawConnectionRegistry`)의 웹 대응 `core/raw-connection-registry.js` 를 검사한다(4-C, WB04-E308).
// 등록소 자체는 호스트 장치(토큰 발급·노드/장치 해석·수명)이고 원본 PE 관측이 없으므로(store 순서는 `raw-connection-cleanup`·`raw-connection-registration` 의 원본 관측 fixture 가 맡는다)
// cpppj `ConnectionRegistrationTests.cpp` 의 등록소 단독 시험 세 건과 각 메서드의 계약을 직접 검사한다. Editor/LocalMission/Client 연결 시험은 해당 단위(LocalMissionTransport 등)에서 옮긴다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E308)
import assert from 'node:assert/strict';
import test from 'node:test';
import { PacketConnection } from '../www/core/raw-packet-dispatch.js';
import { FIRST_CONNECTION_TOKEN, RawConnectionRegistry } from '../www/core/raw-connection-registry.js';

// Native 진단 장치가 받는 원문 파일 문자열(`\Ns\O\Ztrans.cpp`)이다. (2026-11-11 WB04-E308)
const TRANS_FILE = '\\Ns\\O\\Ztrans.cpp';

// 소멸/해제 호출을 순서대로 남기는 실제 장치 모음이다: 소멸은 닫힘 표시(0x40)와 깊이 0 을 확인한다. (2026-11-11 WB04-E308)
function makeDevices() {
    const trace = [];
    return {
        trace,
        // 노드를 등록소에 묶고 토큰을 돌려준다. extra 로 송신/준비/buffered 장치를 더한다.
        track(registry, node, extra = {}) {
            return registry.track(
                node,
                () => { assert.ok(node.flags & 0x40); assert.equal(node.depth, 0); trace.push('close'); },
                () => { trace.push('release'); },
                extra.send, extra.readiness, extra.buffered,
            );
        },
    };
}

// 예외가 나는지 본다. (2026-11-11 WB04-E308)
function rejects(action) {
    try {
        action();
        return false;
    } catch {
        return true;
    }
}

// 토큰 발급·중복·소진·종료 뒤 거부·누락 장치를 확인한다. (2026-11-11 WB04-E308)
test('연결 등록소: 토큰 발급·중복 거부·소진·필수 장치', () => {
    const registry = new RawConnectionRegistry();
    const devices = makeDevices();
    const first = new PacketConnection();
    const second = new PacketConnection();
    const a = devices.track(registry, first);
    const b = devices.track(registry, second);
    // 토큰은 0x6d000000 부터 재사용 없이 오른다.
    assert.equal(FIRST_CONNECTION_TOKEN, 0x6d000000);
    assert.deepEqual([a, b], [0x6d000000, 0x6d000001]);
    assert.ok(registry.owns(a) && registry.node(a) === first && registry.tokenFor(second) === b);
    // 같은 노드는 살아 있는 동안 중복 등록할 수 없고, 필수 소멸/해제 장치가 없으면 거부한다.
    assert.ok(rejects(() => devices.track(registry, first)));
    assert.ok(rejects(() => registry.track(new PacketConnection(), () => {}, null)));
    assert.ok(rejects(() => registry.track(new PacketConnection(), undefined, () => {})));
    assert.ok(rejects(() => registry.track(new PacketConnection(), () => {}, () => {}, 5)));
    // 해제된 토큰은 다시 읽을 수 없고, 다른 연결 객체의 tokenFor 는 오류이다.
    registry.forget(a);
    assert.ok(!registry.owns(a) && rejects(() => registry.node(a)) && rejects(() => registry.tokenFor(first)));
    // forget(0) 은 빈 칸을 별칭으로 오인해 개수를 줄이지 않는다.
    registry.cleanup().pendingCount.value = 5;
    assert.doesNotThrow(() => registry.forget(0));
    assert.equal(registry.cleanup().pendingCount.value, 5);
    registry.cleanup().pendingCount.value = 0;
    // 소진: 마지막 토큰을 발급하면 다음 발급은 거부한다(감김으로 0 이 되면 소진).
    registry.nextToken = 0xffffffff;
    assert.equal(devices.track(registry, new PacketConnection()), 0xffffffff);
    assert.ok(rejects(() => devices.track(registry, new PacketConnection())));
});

// 표 묶음은 하나이며 cleanup() 은 복사가 아니라 같은 표/상자를 보는 보기이다. 동기화 상자는 빌릴 수 있다. (2026-11-11 WB04-E308)
test('연결 등록소: 정리 표 묶음 보기와 동기화 상자 대여', () => {
    const registry = new RawConnectionRegistry();
    const view = registry.cleanup();
    assert.deepEqual([view.transceivers.length, view.buffers.length, view.pending.length], [22, 22, 32]);
    view.transceivers[3] = 77;
    view.pendingCount.value = 4;
    view.synchronization.value = 9;
    const again = registry.cleanup();
    assert.deepEqual([again.transceivers[3], again.pendingCount.value, again.synchronization.value], [77, 4, 9]);
    // 명시 상자를 주면 같은 표에 그 상자를 동기화로 빌린다(등록소의 독립 상자는 건드리지 않음).
    const borrowed = { value: 123 };
    assert.equal(registry.cleanup(borrowed).synchronization, borrowed);
    assert.equal(registry.cleanup().synchronization.value, 9);
    // 실제 상자를 빌리면 이후 보기가 그 상자를 쓰고 값은 쓰지 않는다.
    registry.bindSynchronization(borrowed);
    assert.equal(registry.cleanup().synchronization, borrowed);
    assert.equal(borrowed.value, 123);
    assert.ok(rejects(() => registry.bindSynchronization({})));
    assert.ok(rejects(() => registry.bindSynchronization(null)));
});

// cpppj `ConnectionRegistration_RegistryMovesSameLiveNodeAfterBufferedEffect`: 같은 살아 있는 노드를 pending 에서 양수 칸으로 옮기고 buffered 뒤 pending 을 제거한다. (2026-11-11 WB04-E308)
test('연결 등록소: 같은 노드를 buffered 뒤 재배치하고 owner 종료로 반납', () => {
    const registry = new RawConnectionRegistry();
    const devices = makeDevices();
    const node = new PacketConnection();
    const token = devices.track(registry, node);
    assert.equal(registry.accept(token), -1);
    assert.equal(registry.cleanup().pendingCount.value, 1);
    assert.equal(node.sender, 0xffffffff);
    let observed = false;
    registry.assign(-1, 3, 1, (current) => {
        const view = registry.cleanup();
        // buffered 시점: 같은 토큰/노드, sender 와 양수 칸은 이미 쓰였고 pending/개수는 아직 남아 있다.
        assert.deepEqual([current, registry.node(current) === node, node.sender, view.transceivers[3], view.pending[1], view.pendingCount.value], [token, true, 3, token, token, 1]);
        node.flags |= 2;
        observed = true;
    });
    const view = registry.cleanup();
    assert.ok(observed);
    assert.deepEqual([view.pending[1], view.pendingCount.value, view.transceivers[3], devices.trace.length], [0, 0, token, 0]);
    registry.endPlayers(1);
    assert.ok(!registry.owns(token));
    assert.deepEqual([registry.cleanup().transceivers[3], devices.trace], [0, ['close', 'release']]);
});

// cpppj `ConnectionRegistration_RegistryFailedFreeKeepsActualPrefix`: 해제 장치가 실패하면 먼저 쓴 동기화 해제·flags 와 현재 칸·등록을 유지한다. (2026-11-11 WB04-E308)
test('연결 등록소: 해제 실패는 동기화 해제와 flags 를 남기고 칸·등록을 유지', () => {
    const registry = new RawConnectionRegistry();
    const node = new PacketConnection();
    let destroyed = 0;
    const token = registry.track(node, () => { destroyed += 1; }, () => { throw new Error('명시 실제 free 중단'); });
    registry.register(9, token);
    registry.cleanup().synchronization.value = token;
    assert.ok(rejects(() => registry.disconnect(9)));
    assert.deepEqual([destroyed, (node.flags & 0x40) !== 0, registry.cleanup().synchronization.value, registry.cleanup().transceivers[9], registry.owns(token)], [1, true, 0, token, true]);
    registry.forget(token);
    assert.ok(!registry.owns(token));
    assert.equal(registry.cleanup().transceivers[9], 0);
});

// cpppj `ConnectionRegistration_RegistryBufferedFailureRetainsTemporaryAliases`: 중단한 buffered 는 이미 쓴 양수 칸/sender 를 되돌리지 않고 pending/개수도 앞당겨 지우지 않는다. (2026-11-11 WB04-E308)
test('연결 등록소: buffered 중단은 임시 별칭을 모두 유지', () => {
    const registry = new RawConnectionRegistry();
    const node = new PacketConnection();
    const token = makeDevices().track(registry, node);
    registry.accept(token);
    assert.ok(rejects(() => registry.assign(-1, 4, 1, () => { throw new Error('실제 buffered 중단'); })));
    const view = registry.cleanup();
    assert.deepEqual([view.transceivers[4], view.pending[1], view.pendingCount.value, node.sender, registry.owns(token)], [token, token, 1, 4, true]);
    // 함수가 아닌 buffered 는 어떤 store 도 하기 전에 거부한다.
    assert.throws(() => registry.assign(-1, 6, 1, 5), TypeError);
    assert.equal(registry.cleanup().transceivers[6], 0);
    // buffered 장치를 주지 않으면 도달한 위치에서 누락으로 중단하고 앞선 store 를 남긴다.
    const other = new PacketConnection();
    const second = makeDevices().track(registry, other);
    registry.accept(second);
    assert.ok(rejects(() => registry.assign(-2, 5, 1)));
    assert.deepEqual([registry.cleanup().transceivers[5], other.sender], [second, 5]);
});

// 새 연결 수락: 첫 빈 pending 칸·sender -slot·signed 31 관문·칸 불일치 거부를 확인한다. (2026-11-11 WB04-E308)
test('연결 등록소: pending 수락의 슬롯 선택과 signed 관문', () => {
    const registry = new RawConnectionRegistry();
    const devices = makeDevices();
    const nodes = Array.from({ length: 3 }, () => new PacketConnection());
    const tokens = nodes.map((node) => devices.track(registry, node));
    assert.deepEqual(tokens.map((token) => registry.accept(token)), [-1, -2, -3]);
    assert.deepEqual(nodes.map((node) => node.sender), [0xffffffff, 0xfffffffe, 0xfffffffd]);
    // 비워진 앞 칸을 다음 수락이 다시 쓴다(첫 빈 칸).
    registry.cleanup().pending[1] = 0;
    registry.cleanup().pendingCount.value = 2;
    const extra = new PacketConnection();
    const extraToken = devices.track(registry, extra);
    assert.equal(registry.accept(extraToken), -1);
    assert.equal(registry.cleanup().pendingCount.value, 3);
    // 개수 31 이상(signed)이면 거부하고, 개수가 음수(unsigned 감김)로 보이면 통과한다.
    registry.cleanup().pendingCount.value = 31;
    assert.ok(!registry.canAccept() && rejects(() => registry.accept(devices.track(registry, new PacketConnection()))));
    registry.cleanup().pendingCount.value = 0xffffffff;
    assert.ok(registry.canAccept());
    // 개수는 31 미만이지만 31 칸이 모두 찬 불일치는 거부한다.
    registry.cleanup().pending.fill(7, 1);
    registry.cleanup().pendingCount.value = 5;
    assert.ok(rejects(() => registry.accept(devices.track(registry, new PacketConnection()))));
    // 해제된 토큰은 수락할 수 없다.
    assert.ok(rejects(() => registry.accept(0x12345)));
});

// 일반 해제(정리 부모 밖)는 같은 토큰의 양수/pending 별칭만 지우고 새 세대의 칸은 보존한다. 정리 부모 안에서는 부모가 직접 store 한다. (2026-11-11 WB04-E308)
test('연결 등록소: 해제의 별칭 정리와 정리 부모 안 유예', () => {
    const registry = new RawConnectionRegistry();
    const devices = makeDevices();
    const node = new PacketConnection();
    const token = devices.track(registry, node);
    const view = registry.cleanup();
    view.transceivers[4] = token;
    view.transceivers[11] = 9999;
    view.pending[0] = token;
    view.pending[2] = token;
    view.pending[6] = 8888;
    view.pendingCount.value = 2;
    view.synchronization.value = token;
    registry.release(token);
    assert.deepEqual([view.transceivers[4], view.transceivers[11], view.pending[2], view.pending[6], view.pendingCount.value, view.synchronization.value, registry.owns(token)], [0, 9999, 0, 8888, 1, 0, false]);
    // pending 0 칸은 원문처럼 별칭 정리(1..31)의 대상이 아니다.
    assert.equal(view.pending[0], token);
    view.pending[0] = 0;
    // pending 별칭만 있으면 동기화 토큰은 건드리지 않는다.
    const second = new PacketConnection();
    const secondToken = devices.track(registry, second);
    view.pending[3] = secondToken;
    view.pendingCount.value = 1;
    view.synchronization.value = secondToken;
    registry.release(secondToken);
    assert.deepEqual([view.pending[3], view.pendingCount.value, view.synchronization.value], [0, 0, secondToken]);
    // 알 수 없는 토큰의 해제/소멸은 오류이다.
    assert.ok(rejects(() => registry.release(0x42)) && rejects(() => registry.destroy(0x42)));
    // 정리 부모(disconnect) 안에서는 부모가 칸 0 을 쓰므로 일반 별칭 해제가 먼저 칸을 지우지 않는다: 같은 토큰의 다른 양수 칸이 남는다.
    const third = new PacketConnection();
    const thirdToken = devices.track(registry, third);
    view.transceivers[2] = thirdToken;
    view.transceivers[5] = thirdToken;
    registry.disconnect(2);
    assert.deepEqual([view.transceivers[2], view.transceivers[5], registry.owns(thirdToken)], [0, thirdToken, false]);
});

// 송신: 슬롯 읽기 → Player 관문(1..8) → 빈 슬롯 → 가상 송신. pending 은 -32..-1, 호출 뒤 같은 노드를 다시 읽지 않는다. (2026-11-11 WB04-E308)
test('연결 등록소: 가변 송신의 슬롯·Player 관문과 범위', () => {
    const registry = new RawConnectionRegistry();
    const devices = makeDevices();
    const sent = [];
    const node = new PacketConnection();
    const token = devices.track(registry, node, { send: (packet) => { sent.push(Array.from(packet)); } });
    const players = new Uint8Array(9 * 0xb4);
    registry.cleanup().transceivers[9] = token;
    registry.cleanup().transceivers[3] = token;
    registry.cleanup().pending[2] = token;
    const packet = Uint8Array.of(1, 2, 3);
    // 9 번은 Player 관문이 없고, 3 번은 Player 필드 bit 0 이 꺼져 있으면 보내지 않는다.
    assert.equal(registry.sendMutable(9, packet, players), true);
    assert.equal(registry.sendMutable(3, packet, players), false);
    players[3 * 0xb4 + 0x50] = 1;
    assert.equal(registry.sendMutable(3, packet, players), true);
    assert.equal(registry.sendMutable(-2, packet, players), true);
    assert.equal(sent.length, 3);
    // 빈 슬롯은 false, Player 필드가 배열 밖이면 오류, 범위 밖 번호는 오류이다.
    assert.equal(registry.sendMutable(5, packet, new Uint8Array(0xb4 * 6)), false);
    assert.ok(rejects(() => registry.sendMutable(8, packet, new Uint8Array(10))));
    for (const id of [-33, 22, 100]) assert.throws(() => registry.sendMutable(id, packet, players), /번호 범위/, `id ${id}`);
    // -32 는 번호 범위는 통과하지만 pending 표 32 번 칸이 없어 표 범위 오류이다.
    assert.throws(() => registry.sendMutable(-32, packet, players), /실제 표 범위/);
    // 0·9 번과 pending 은 Player 관문이 없고, 빈 슬롯은 장치를 요구하지 않고 false 이다.
    registry.cleanup().transceivers[0] = token;
    assert.equal(registry.sendMutable(0, packet, new Uint8Array(0)), true);
    assert.deepEqual([registry.sendMutable(10, packet, players), registry.sendMutable(-5, packet, players), registry.sendMutable(21, packet, players)], [false, false, false]);
    assert.equal(sent.length, 4);
    // 송신 장치가 없는 살아 있는 연결은 도달한 위치에서 오류이다.
    const bare = new PacketConnection();
    const bareToken = devices.track(registry, bare);
    registry.cleanup().transceivers[7] = bareToken;
    players[7 * 0xb4 + 0x50] = 1;
    assert.ok(rejects(() => registry.sendMutable(7, packet, players)));
    // 직접 송신은 번호/Player 관문을 반복하지 않는다.
    registry.sendDirect(token, packet);
    assert.equal(sent.length, 5);
    assert.ok(rejects(() => registry.sendDirect(bareToken, packet)) && rejects(() => registry.sendDirect(0x77, packet)));
});

// Native 송신: 범위 밖이면 줄 1e 진단이 먼저 불리고 반환하면 실제 표 범위 오류가 이어진다. (2026-11-11 WB04-E308)
test('연결 등록소: Native 송신의 줄 1e 진단', () => {
    const registry = new RawConnectionRegistry();
    const seen = [];
    const assertion = (expression, file, line) => { seen.push([expression, file, line]); };
    const token = makeDevices().track(registry, new PacketConnection(), { send: () => {} });
    registry.cleanup().transceivers[9] = token;
    assert.equal(registry.sendMutableNative(9, Uint8Array.of(1), new Uint8Array(0), assertion), true);
    assert.deepEqual(seen, []);
    for (const id of [22, -33]) assert.ok(rejects(() => registry.sendMutableNative(id, Uint8Array.of(1), new Uint8Array(0), assertion)), `id ${id}`);
    assert.deepEqual(seen, [
        ['-zaMAX_PENDING <= id && id < NUM_TRANSCEIVERS', '\\Ns\\O\\Ztrans.cpp', 0x1e],
        ['-zaMAX_PENDING <= id && id < NUM_TRANSCEIVERS', '\\Ns\\O\\Ztrans.cpp', 0x1e],
    ]);
    // 진단 장치가 없으면 그 위치에서 중단한다.
    assert.ok(rejects(() => registry.sendMutableNative(22, Uint8Array.of(1), new Uint8Array(0))));
});

// 주소 조회와 준비 조회, 접속 공개를 확인한다. (2026-11-11 WB04-E308)
test('연결 등록소: 주소·소켓 준비·접속 공개', () => {
    const registry = new RawConnectionRegistry();
    const devices = makeDevices();
    const node = new PacketConnection();
    node.address.set([2, 0, 0x1a, 0x90, 10, 0, 0, 15]);
    const token = devices.track(registry, node, { readiness: () => -1 });
    // 연결이 없으면 0.0.0.0:0 의 같은 주소(첫 BYTE 2), 있으면 그 연결의 복사본이다.
    assert.deepEqual(Array.from(registry.address(4)), [2, ...new Array(19).fill(0)]);
    registry.register(4, token);
    registry.cleanup().pending[1] = token;
    const fromMain = registry.address(4);
    assert.deepEqual(Array.from(fromMain.slice(0, 8)), [2, 0, 0x1a, 0x90, 10, 0, 0, 15]);
    fromMain[0] = 99;
    assert.equal(node.address[0], 2);
    assert.equal(registry.address(-1)[2], 0x1a);
    for (const id of [-33, 22]) assert.throws(() => registry.address(id), /ID 범위/, `id ${id}`);
    assert.throws(() => registry.address(-32), /실제 표 범위/);
    // 소켓 준비는 묶인 장치의 값을 그대로 돌려주고 장치가 없으면 오류이다.
    assert.equal(registry.socketReadiness(token), -1);
    const bareToken = devices.track(registry, new PacketConnection());
    assert.ok(rejects(() => registry.socketReadiness(bareToken)) && rejects(() => registry.socketReadiness(0x88)));
    // 접속 공개는 칸만 쓰고(기존 칸 대체, sender 와 개수 불변) 범위와 노드 존재를 확인한다.
    const fresh = new PacketConnection();
    const freshToken = devices.track(registry, fresh);
    registry.publishConnect(4, freshToken);
    assert.deepEqual([registry.cleanup().transceivers[4], fresh.sender, registry.cleanup().pendingCount.value, devices.trace.length], [freshToken, 0, 0, 0]);
    for (const id of [-1, 22]) assert.ok(rejects(() => registry.publishConnect(id, freshToken)), `id ${id}`);
    // register 는 칸을 쓰기 전에 노드 존재를 확인한다.
    assert.ok(rejects(() => registry.register(6, 0x999)));
    assert.equal(registry.cleanup().transceivers[6], 0);
    assert.ok(rejects(() => registry.publishConnect(1, 0x99)));
});

// 재배치의 진단: 일반 경로는 진단을 오류로 보고, Native 경로는 줄별 원문 식으로 장치를 부르고 반환하면 이어간다. (2026-11-11 WB04-E308)
test('연결 등록소: 재배치 진단의 일반/Native 경로', () => {
    const registry = new RawConnectionRegistry();
    const devices = makeDevices();
    // 일반 경로: 서버 대상 범위 밖은 줄 b0 진단이 오류가 된다.
    const typed = new PacketConnection();
    const typedToken = devices.track(registry, typed);
    registry.accept(typedToken);
    assert.throws(() => registry.assign(-1, 0, 1, () => {}), /원문 연결 등록 진단:176/);
    assert.equal(registry.cleanup().transceivers[0], 0);
    // Native 경로: 장치가 줄 b0/b3/b6 과 원문 식을 받고, 반환하면 이어서 쓴다.
    const seen = [];
    const assertion = (expression, file, line) => { seen.push([line, expression, file]); };
    let bufferedCalls = 0;
    const native = new PacketConnection();
    const nativeToken = devices.track(registry, native, { buffered: () => { bufferedCalls += 1; native.flags |= 2; } });
    const pending = registry.accept(nativeToken);
    registry.assignNative(pending, 0, 1, assertion);
    assert.deepEqual(seen, [[0xb0, 'FIRST_REAL_PLAYER <= transId && transId < END_REAL_PLAYERS', '\\Ns\\O\\Ztrans.cpp']]);
    assert.deepEqual([registry.cleanup().transceivers[0], native.sender, native.flags & 2, bufferedCalls], [nativeToken, 0, 2, 1]);
    // 클라이언트(server=0)의 대상 4 는 줄 b3, 이미 찬 대상은 줄 b6 진단이다.
    seen.length = 0;
    const another = new PacketConnection();
    const anotherToken = devices.track(registry, another, { buffered: () => {} });
    const anotherPending = registry.accept(anotherToken);
    registry.assignNative(anotherPending, 4, 0, assertion);
    assert.deepEqual(seen[0], [0xb3, 'transId == GAME_SERVER_ID', TRANS_FILE]);
    const third = new PacketConnection();
    const thirdToken = devices.track(registry, third, { buffered: () => {} });
    const thirdPending = registry.accept(thirdToken);
    registry.assignNative(thirdPending, 4, 0, assertion);
    assert.deepEqual(seen.map((item) => item[0]), [0xb3, 0xb3, 0xb6]);
    assert.equal(seen[2][1], 'transceivers[transId] == NULL');
    // 비음수 pending(0)은 buffered 뒤 줄 bb 진단을 부르고 반환하면 0 번 칸을 제거한다.
    const zero = new PacketConnection();
    const zeroToken = devices.track(registry, zero, { buffered: () => {} });
    registry.cleanup().pending[0] = zeroToken;
    registry.cleanup().pendingCount.value = 3;
    seen.length = 0;
    registry.assignNative(0, 6, 1, assertion);
    assert.deepEqual([seen, registry.cleanup().pending[0], registry.cleanup().pendingCount.value, registry.cleanup().transceivers[6]], [[[0xbb, 'pendingId < 0', TRANS_FILE]], 0, 2, zeroToken]);
    // 등록되지 않은 원문 줄은 Native 진단 표에 없어 거부한다.
    assert.throws(() => registry.nativeRegistrationHooks(null, assertion).assertion(0xc2), /미등록 원문 진단/);
    // 진단 장치가 없거나 buffered 장치가 없으면 도달한 위치에서 오류이다.
    const bare = new PacketConnection();
    const bareToken = devices.track(registry, bare);
    const barePending = registry.accept(bareToken);
    assert.throws(() => registry.assignNative(barePending, 0, 1), /선택된 Client 진단 미연결/);
    assert.throws(() => registry.assignNative(barePending, 8, 1, assertion), /공통buffered장치없음/);
    assert.equal(registry.cleanup().transceivers[8], bareToken);
    // 사용자 buffered 를 주면 기본 buffered 대신 쓴다.
    const custom = [];
    registry.assignNative(barePending, 7, 1, assertion, (token) => { custom.push(token); });
    assert.deepEqual(custom, [bareToken]);
    // setBufferedNative 는 묶인 장치가 없으면 오류이다.
    assert.ok(rejects(() => registry.setBufferedNative(bareToken)) && rejects(() => registry.setBufferedNative(0x123)));
});

// 개별 종료의 일반/Native 진단: 22 이상은 줄 fa 진단이 오류가 되거나(일반) 장치로 가고 실제 표 범위 거부가 이어진다(Native). (2026-11-11 WB04-E308)
test('연결 등록소: 개별 종료의 줄 fa 진단', () => {
    const registry = new RawConnectionRegistry();
    assert.throws(() => registry.disconnect(22), /원문 연결 정리 진단:250/);
    const seen = [];
    assert.ok(rejects(() => registry.disconnectNative(22, (expression, file, line) => { seen.push([expression, file, line]); })));
    assert.deepEqual(seen, [['id < NUM_TRANSCEIVERS', '\\Ns\\O\\Ztrans.cpp', 0xfa]]);
    assert.throws(() => registry.disconnectNative(22), /선택된 Client 진단 미연결/);
    // 정리 깊이는 예외 뒤에도 되돌아온다(다음 일반 해제가 별칭을 지운다).
    assert.equal(registry.cleanupDepth, 0);
    assert.equal(registry.deviceCallDepth, 0);
});

// 전체 정리: 양수 22칸만, owner 종료는 server 관문, pending 정리는 개수를 다시 계산하지 않는다. 호출마다 소멸→해제 순서이다. (2026-11-11 WB04-E308)
test('연결 등록소: 전체·owner·pending 정리', () => {
    const registry = new RawConnectionRegistry();
    const devices = makeDevices();
    const view = registry.cleanup();
    const nodes = Array.from({ length: 4 }, () => new PacketConnection());
    const [owner, spare, pending, residual] = nodes.map((node) => devices.track(registry, node));
    view.transceivers[1] = owner;
    view.transceivers[9] = spare;
    view.pending[3] = pending;
    view.pendingCount.value = 7;
    // server 가 0 이면 owner 종료는 아무 것도 하지 않는다.
    registry.endPlayers(0);
    assert.equal(registry.owns(owner), true);
    registry.endPlayers(2);
    assert.deepEqual([registry.owns(owner), view.transceivers[1], devices.trace], [false, 0, ['close', 'release']]);
    registry.closePending();
    assert.deepEqual([registry.owns(pending), view.pending[3], view.pendingCount.value], [false, 0, 7]);
    registry.closeAll();
    assert.deepEqual([registry.owns(spare), view.transceivers[9], devices.trace.length], [false, 0, 6]);
    assert.equal(registry.owns(residual), true);
});

// 응용 최종 종료: 진행 중 부모/수신 중 노드가 있으면 거부하고, 양수 칸 순서(0..21) → pending(개수 유지) → 전역 칸에서 덮인 잔류 노드(토큰 오름차순)를 닫으며 새 노드 발급을 막는다. 초기 패킷 버퍼 칸은 접속 통지(재기준선 때 이식)가 발급하는 별도 복제 물리의 몫이라 지금은 비어 있어야 한다. (2026-11-11 WB04-E308)
test('연결 등록소: 응용 최종 종료의 닫기 순서·가드·발급 차단', () => {
    const registry = new RawConnectionRegistry();
    const closed = [];
    const make = (name) => {
        const node = new PacketConnection();
        // 소멸/해제는 지연 닫힘 부모를 거쳐야 하므로 닫힘 표시(0x40)와 깊이 0 을 함께 확인한다.
        const token = registry.track(node, () => { assert.ok(node.flags & 0x40 && !node.depth); closed.push(`close-${name}`); }, () => { closed.push(`free-${name}`); });
        return { node, token };
    };
    // 토큰 발급 순서: early < late < pend < lostA < lostB < busy.
    const early = make('early');
    const late = make('late');
    const pend = make('pend');
    const lostA = make('lostA');
    const lostB = make('lostB');
    const busy = make('busy');
    const view = registry.cleanup();
    // 양수 칸은 슬롯 순서로 닫힌다: 1 번에 늦은 토큰, 2 번에 이른 토큰. pending 은 개수를 정규화하지 않는다. lostA/lostB/busy 는 전역 칸을 덮여 잃은 잔류 노드이다.
    view.transceivers[1] = late.token;
    view.transceivers[2] = early.token;
    view.pending[3] = pend.token;
    view.pendingCount.value = 7;
    // 수신 중(깊이>0)인 노드가 있으면 거부하고 발급 차단도 걸지 않는다.
    busy.node.depth = 1;
    assert.throws(() => registry.shutdownTrackedDevices(), /진행중Node배송/);
    assert.equal(registry.deviceShutdown, false);
    busy.node.depth = 0;
    // 진행 중 Native 부모(호출 범위/정리 부모) 안에서는 거부한다.
    assert.throws(() => registry.runDeviceScope(() => { registry.shutdownTrackedDevices(); }), /진행중Native/);
    assert.throws(() => registry.runCleanup(() => { registry.shutdownTrackedDevices(); }), /진행중Native/);
    registry.shutdownTrackedDevices();
    // 닫기 순서: 양수 칸(late → early) → pending(pend) → 잔류 노드 토큰 오름차순(lostA, lostB, busy), 각각 소멸 → 해제.
    assert.deepEqual(closed, ['close-late', 'free-late', 'close-early', 'free-early', 'close-pend', 'free-pend', 'close-lostA', 'free-lostA', 'close-lostB', 'free-lostB', 'close-busy', 'free-busy']);
    assert.ok([early, late, pend, lostA, lostB, busy].every(({ token }) => !registry.owns(token)));
    // pending 개수는 정규화하지 않고, 호출 범위 깊이는 모두 되돌아온다.
    assert.deepEqual([view.transceivers[1], view.transceivers[2], view.pending[3], view.pendingCount.value, registry.deviceCallDepth, registry.cleanupDepth], [0, 0, 0, 7, 0, 0]);
    // 종료 뒤 새 노드 발급은 거부한다.
    assert.throws(() => registry.track(new PacketConnection(), () => {}, () => {}), /최종종료/);
    // 비어 있지 않은 초기 버퍼 칸은 해제 장치를 알 수 없어 거부한다(접속 통지 이식 전).
    const other = new RawConnectionRegistry();
    other.cleanup().buffers[4] = 0x4242;
    assert.throws(() => other.shutdownTrackedDevices(), /free 장치 누락/);
});
