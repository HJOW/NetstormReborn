// 연결 등록/재배치(cpppj `o/RawConnectionRegistration`, 원문 004e2870·004df4f0·004e27b0)의 웹 대응 `core/raw-connection-registration.js` 를 검사한다(4-C, WB04-E307).
//  - 원본 독립 관측 `connection-registration-x86.tsv` 288행(두 FPU 환경 × 진단/버퍼 callback 중단 × 호출 도중 변경)의 전체 표(22 연결·32 pending)·개수·노드 flags/sender·효과열·중단 prefix 를 재생해 대조한다.
//  - 손 시험: 호출 도중 바뀐 현재 칸/개수를 다시 읽음, 누락 장치가 도달한 위치에서만 요구됨, 앞선 store 를 실패 시에도 보존함, 대상 표 범위 부족 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E307)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { PacketConnection } from '../www/core/raw-packet-dispatch.js';
import { createConnectionCleanupGlobals } from '../www/core/raw-connection-cleanup.js';
import { assignPendingConnection, registerTransceiver, removePendingConnection } from '../www/core/raw-connection-registration.js';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';

// 원본 관측 파일이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-11 WB04-E307)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'connection-registration-x86.tsv'));

// Native 격리 입력의 첫 연결 토큰과 노드 한 칸 폭, 입력 바이트 안의 노드 시작 위치이다. (2026-11-11 WB04-E307)
const NODES = 0x12000100;
const NODE_STRIDE = 192;
const NODE_OFFSET = 224;

// 외부 장치의 명시 중단이며 정상 반환과 구별한다. (2026-11-11 WB04-E307)
class RegistrationStop extends Error {}

// 16진 문자열을 BYTE 배열로 복원한다. (2026-11-11 WB04-E307)
const hexBytes = (hex) => Uint8Array.from(hex.match(/../g) ?? [], (pair) => Number.parseInt(pair, 16));

// little endian 정확히 4BYTE 를 읽고 쓴다(이웃 필드는 건드리지 않음). (2026-11-11 WB04-E307)
const read = (bytes, at) => (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
function write(bytes, at, value) {
    // 낮은 BYTE 부터 네 개를 쓴다.
    for (let byte = 0; byte < 4; byte += 1) bytes[at + byte] = (value >>> (8 * byte)) & 0xff;
}

// 예외가 나는지 본다. (2026-11-11 WB04-E307)
function rejects(action) {
    try {
        action();
        return false;
    } catch {
        return true;
    }
}

// 288행 재생: 열은 operation/case/change/stop/control/pending/target/input/output/calls/stopped 이다. (2026-11-11 WB04-E307)
test('연결 등록: 원본 288행의 전체 표·노드·효과열·중단 prefix', { skip: ROWS === null }, () => {
    let rows = 0;
    let stopped = 0;
    // 등록·pending 제거·재배치 세 연산의 모든 행을 재생한다.
    for (const row of ROWS) {
        assert.equal(row.length, 11);
        const bytes = hexBytes(row[7]);
        assert.equal(bytes.length, 800);
        const globals = createConnectionCleanupGlobals();
        const nodes = Array.from({ length: 3 }, () => new PacketConnection());
        // 입력 표: 22 연결 칸, 0 칸도 보존하는 32 pending 칸, 개수, server 순이다.
        for (let index = 0; index < 22; index += 1) globals.transceivers[index] = read(bytes, index * 4);
        for (let index = 0; index < 32; index += 1) globals.pending[index] = read(bytes, 88 + index * 4);
        nodes.forEach((node, index) => {
            const at = NODE_OFFSET + NODE_STRIDE * index;
            Object.assign(node, { flags: read(bytes, at + 0x34), sender: read(bytes, at + 0x40) });
        });
        globals.pendingCount.value = read(bytes, 216);
        const server = read(bytes, 220);
        const source = Number(row[5]) | 0;
        const target = Number(row[6]) | 0;
        let calls = '';
        let interrupted = false;
        const hooks = {
            node: (token) => {
                if (token < NODES || (token - NODES) % NODE_STRIDE || (token - NODES) / NODE_STRIDE >= nodes.length) throw new RangeError('최초 연결 등록 노드 범위');
                return nodes[(token - NODES) / NODE_STRIDE];
            },
        };
        const effect = (kind, value) => {
            if (calls) calls += ';';
            calls += `${kind}:${value}`;
            // buffered 는 같은 노드에 flags 2 를 켜고, 변이 행에서는 개수/대상 칸/pending 칸을 바꾼다.
            if (kind === 'B') {
                hooks.node(value).flags = (hooks.node(value).flags | 2) >>> 0;
                if (Number(row[2])) {
                    globals.pendingCount.value = 0;
                    globals.transceivers[target] = NODES + 384;
                    globals.pending[-source] = NODES + 384;
                }
            }
            if (row[3] === kind) throw new RegistrationStop();
        };
        hooks.buffered = (token) => { effect('B', token); };
        hooks.assertion = (value) => { effect('A', value); };
        try {
            switch (Number(row[0])) {
            case 0: registerTransceiver(target, NODES, globals, hooks); break;
            case 1: removePendingConnection(source, globals); break;
            case 2: assert.equal(assignPendingConnection(source, target, server, globals, hooks), target); break;
            default: assert.fail('알 수 없는 연산');
            }
        } catch (error) {
            if (!(error instanceof RegistrationStop)) throw error;
            interrupted = true;
            stopped += 1;
        }
        // 현재 표를 입력 바이트 위에 되써 기대 출력과 전체 대조한다.
        for (let index = 0; index < 22; index += 1) write(bytes, index * 4, globals.transceivers[index]);
        for (let index = 0; index < 32; index += 1) write(bytes, 88 + index * 4, globals.pending[index]);
        write(bytes, 216, globals.pendingCount.value);
        write(bytes, 220, server);
        nodes.forEach((node, index) => {
            const at = NODE_OFFSET + NODE_STRIDE * index;
            write(bytes, at + 0x34, node.flags);
            write(bytes, at + 0x40, node.sender);
            // 호스트 표현 필드는 등록이 바꾸지 않는다.
            assert.deepEqual([node.received, node.heartbeat, node.depth, node.delayed], [0, 0, 0, false]);
        });
        assert.deepEqual(Array.from(bytes), Array.from(hexBytes(row[8])), `행 ${rows} 출력`);
        assert.equal(calls || '-', row[9], `행 ${rows} 효과열`);
        assert.equal(interrupted, row[10] === '1', `행 ${rows} 중단`);
        assert.equal(globals.synchronization.value, 0);
        rows += 1;
    }
    assert.deepEqual([rows, stopped], [288, 60]);
});

// 등록: 빈 칸이면 진단 없이 store, 차 있으면 줄 c2 진단 뒤(반환하면) 덮어쓰며 sender 는 unsigned id 이다. (2026-11-11 WB04-E307)
test('연결 등록: 차 있는 칸의 진단과 sender 기록', () => {
    const globals = createConnectionCleanupGlobals();
    const node = new PacketConnection();
    const hooks = { node: (token) => { assert.equal(token, 77); return node; } };
    // 빈 칸은 진단 장치를 요구하지 않는다.
    registerTransceiver(5, 77, globals, hooks);
    assert.deepEqual([globals.transceivers[5], node.sender], [77, 5]);
    // 차 있는 칸은 줄 c2 진단을 부르고 반환하면 덮어쓴다.
    let line = 0;
    globals.transceivers[6] = 99;
    registerTransceiver(6, 77, globals, { ...hooks, assertion: (value) => { line = value; } });
    assert.deepEqual([line, globals.transceivers[6], node.sender], [0xc2, 77, 6]);
    // 진단 장치가 없으면 그 위치에서 중단하고 칸을 바꾸지 않는다.
    globals.transceivers[7] = 99;
    assert.ok(rejects(() => registerTransceiver(7, 77, globals, hooks)));
    assert.equal(globals.transceivers[7], 99);
    // 표 밖 id 는 실제 표 범위 부족으로 거부하고 이름 있는 칸을 만들지 않는다.
    for (const id of [-1, 22, 2147483647, -2147483648]) assert.ok(rejects(() => registerTransceiver(id, 77, globals, hooks)), `id ${id}`);
});

// pending 제거: 칸 0 쓰기 후 개수를 unsigned 로 감소하며 입력 검증을 앞당기지 않는다. (2026-11-11 WB04-E307)
test('연결 등록: pending 제거의 개수 감김과 범위', () => {
    const globals = createConnectionCleanupGlobals();
    globals.pending[3] = 123;
    globals.pendingCount.value = 1;
    removePendingConnection(-3, globals);
    assert.deepEqual([globals.pending[3], globals.pendingCount.value], [0, 0]);
    // 개수가 0 이면 감김으로 0xffffffff 가 된다.
    removePendingConnection(-3, globals);
    assert.equal(globals.pendingCount.value, 0xffffffff);
    // 양수 id 는 pending 표 밖이면 개수를 건드리기 전에 거부한다(칸 쓰기가 실제 표 범위에서 실패).
    globals.pendingCount.value = 5;
    for (const id of [1, 32, 2147483647, -2147483648]) assert.ok(rejects(() => removePendingConnection(id, globals)), `id ${id}`);
    assert.equal(globals.pendingCount.value, 5);
    // id 0 은 0 번 칸을 비우고 개수를 줄인다.
    globals.pending[0] = 9;
    removePendingConnection(0, globals);
    assert.deepEqual([globals.pending[0], globals.pendingCount.value], [0, 4]);
});

// 재배치: 순서(진단 → store → sender → buffered → bb 진단 → pending 제거)와 buffered 뒤 현재 개수에서 감소함을 확인한다. (2026-11-11 WB04-E307)
test('연결 등록: pending 재배치 순서와 buffered 뒤 현재 개수', () => {
    const globals = createConnectionCleanupGlobals();
    const node = new PacketConnection();
    const order = [];
    globals.pending[1] = NODES;
    globals.pendingCount.value = 2;
    const hooks = {
        node: (token) => { assert.equal(token, NODES); return node; },
        buffered: (token) => {
            // buffered 시점에 양수 칸과 sender 는 이미 쓰였고 pending 칸/개수는 아직 남아 있다.
            order.push(['buffered', token, globals.transceivers[3], node.sender, globals.pending[1], globals.pendingCount.value]);
            globals.pendingCount.value = 10;
        },
    };
    assert.equal(assignPendingConnection(-1, 3, 1, globals, hooks), 3);
    assert.deepEqual(order, [['buffered', NODES, NODES, 3, NODES, 2]]);
    // callback 이 바꾼 현재 개수 10 에서 감소한다.
    assert.deepEqual([globals.pending[1], globals.pendingCount.value], [0, 9]);
    // 클라이언트 쪽(server=0)은 대상 9 만 정상이며 아니면 줄 b3, 서버 쪽 범위 밖은 줄 b0 진단을 부른다.
    const lines = [];
    const diagnose = { ...hooks, buffered: () => {}, assertion: (line) => { lines.push(line); } };
    globals.pending[2] = NODES;
    assignPendingConnection(-2, 9, 0, globals, diagnose);
    globals.pending[2] = NODES;
    assignPendingConnection(-2, 4, 0, globals, diagnose);
    globals.pending[2] = NODES;
    assignPendingConnection(-2, 9, 1, globals, diagnose);
    // 클라이언트 쪽 대상 4(b3) → 서버 쪽 대상 9(b0)와 이미 차 있는 칸(b6) 순서로 정확히 불린다. 첫 호출은 진단이 없다.
    assert.deepEqual(lines, [0xb3, 0xb0, 0xb6]);
    // 비음수 pending 은 buffered 뒤에 줄 bb 진단을 부르고 반환하면 0 번 칸을 제거한다.
    globals.pending[0] = NODES;
    globals.pendingCount.value = 3;
    lines.length = 0;
    assert.equal(assignPendingConnection(0, 5, 1, globals, diagnose), 5);
    assert.deepEqual([lines, globals.pending[0], globals.pendingCount.value, globals.transceivers[5]], [[0xbb], 0, 2, NODES]);
});

// 재배치의 앞선 쓰기는 buffered/진단 중단에도 되돌리지 않고, 장치 누락은 도달한 위치에서만 요구한다. (2026-11-11 WB04-E307)
test('연결 등록: 재배치 중단 시 앞선 store 보존과 장치 누락 위치', () => {
    const globals = createConnectionCleanupGlobals();
    const node = new PacketConnection();
    globals.pending[1] = NODES;
    globals.pendingCount.value = 1;
    const hooks = { node: () => node };
    // buffered 장치가 없으면 양수 칸과 sender 는 쓰이고 pending 제거는 일어나지 않는다.
    assert.ok(rejects(() => assignPendingConnection(-1, 3, 1, globals, hooks)));
    assert.deepEqual([globals.transceivers[3], node.sender, globals.pending[1], globals.pendingCount.value], [NODES, 3, NODES, 1]);
    // 대상이 1..8 안이고 비어 있는 정상 경로는 진단 장치를 전혀 요구하지 않는다.
    globals.transceivers[3] = 0;
    assert.equal(assignPendingConnection(-1, 3, 1, globals, { ...hooks, buffered: () => {} }), 3);
    // 범위 밖 서버 대상은 진단 장치가 없으면 그 위치(저장 전)에서 중단한다.
    globals.pending[2] = NODES;
    assert.ok(rejects(() => assignPendingConnection(-2, 0, 1, globals, { ...hooks, buffered: () => {} })));
    assert.equal(globals.transceivers[0], 0);
    // 차 있는 대상의 줄 b6 진단은 범위 밖 pending 의 표 거부보다 먼저 불린다(원문 순서).
    const seen = [];
    globals.transceivers[4] = 5;
    assert.ok(rejects(() => assignPendingConnection(-32, 4, 1, globals, { ...hooks, assertion: (line) => { seen.push(line); } })));
    assert.deepEqual(seen, [0xb6]);
    // 표 밖 대상/pending 은 실제 표 범위 부족으로 거부하고 어떤 칸도 쓰지 않는다.
    for (const [pending, target] of [[-2, 22], [-2, -1], [-32, 3], [-2147483648, 3]]) {
        assert.ok(rejects(() => assignPendingConnection(pending, target, 1, globals, { ...hooks, buffered: () => {}, assertion: () => {} })), `${pending}/${target}`);
    }
});
