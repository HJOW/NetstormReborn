// 연결 정리(cpppj `o/RawConnectionCleanup`, 원문 004df440·004df520·004e2940·004e29c0·004e2a10·004df4a0)의 웹 대응 `core/raw-connection-cleanup.js` 를 검사한다(4-C, WB04-E307).
//  - 원본 독립 관측 `connection-cleanup-x86.tsv` 580행(originals 10.78 PE 여섯 몸체 × 두 FPU 환경 × 호출 도중 변경 포함)의 전체 표(22 연결·22 버퍼·32 pending)·노드 필드·동기화/개수/server·효과열·중단 prefix 를 재생해 대조한다.
//  - 손 시험(cpppj 시험 이식): 누락 장치가 선행 검사로 가려지지 않고 flags/sync 와 소멸 효과의 완료 prefix 를 남김, 음수 경계의 자연 반환과 양수 진단이 선택 위치에서만 장치를 요구함.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E307)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { PacketConnection } from '../www/core/raw-packet-dispatch.js';
import { closeAllPendingConnections, closeAllTransceivers, closePendingConnection, createConnectionCleanupGlobals, deferConnectionClose, disconnectTransceiver, endPlayerConnections } from '../www/core/raw-connection-cleanup.js';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';

// 원본 관측 파일이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-11 WB04-E307)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'connection-cleanup-x86.tsv'));

// Native 격리 입력의 첫 연결 토큰, 입력 바이트 안의 노드 입력 시작(양수 176 + pending 128 + 세 DWORD 뒤)과 노드 한 칸 폭이다. (2026-11-11 WB04-E307)
const NODES = 0x12000100;
const NODE_OFFSET = 316;
const NODE_STRIDE = 192;

// 외부 장치의 명시 중단이며 정상 반환과 구별한다. (2026-11-11 WB04-E307)
class ConnectionStop extends Error {}

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

// 580행 재생: 열은 operation/case/change/stop/control/argument/input/output/calls/stopped/instructions 이다. (2026-11-11 WB04-E307)
test('연결 정리: 원본 580행의 전체 표·노드·효과열·중단 prefix', { skip: ROWS === null }, () => {
    let rows = 0;
    let stopped = 0;
    // 두 FPU 환경과 callback 의 뒤 슬롯/깊이/개수 변경까지 모든 행을 재생한다.
    for (const row of ROWS) {
        assert.equal(row.length, 11);
        const bytes = hexBytes(row[6]);
        assert.equal(bytes.length, 892);
        const globals = createConnectionCleanupGlobals();
        const nodes = Array.from({ length: 3 }, () => new PacketConnection());
        // 입력 표: 원문 양수 연결과 별도 버퍼표, 전체 정리의 0 칸도 보존하는 pending 이다.
        for (let index = 0; index < 22; index += 1) {
            globals.transceivers[index] = read(bytes, index * 4);
            globals.buffers[index] = read(bytes, 88 + index * 4);
        }
        for (let index = 0; index < 32; index += 1) globals.pending[index] = read(bytes, 176 + index * 4);
        // 정리 대상과 보존해야 할 현재 sender/두 시각을 같은 실제 필드에 입력한다.
        nodes.forEach((node, index) => {
            const at = NODE_OFFSET + NODE_STRIDE * index;
            Object.assign(node, { flags: read(bytes, at + 0x34), sender: read(bytes, at + 0x40), received: read(bytes, at + 0x44), heartbeat: read(bytes, at + 0x48), depth: read(bytes, at + 0x8c) });
        });
        globals.synchronization.value = read(bytes, 304);
        globals.pendingCount.value = read(bytes, 308);
        let server = read(bytes, 312);
        let calls = '';
        let changed = false;
        let interrupted = false;
        const hooks = {
            node: (token) => {
                if (token < NODES || (token - NODES) % NODE_STRIDE || (token - NODES) / NODE_STRIDE >= nodes.length) throw new RangeError('원문 연결 토큰 범위');
                return nodes[(token - NODES) / NODE_STRIDE];
            },
        };
        const effect = (kind, token) => {
            if (calls) calls += ';';
            calls += `${kind}:${token}`;
            // 소멸 callback 이 뒤 슬롯/깊이/개수/server 를 바꾸는 변이 행이다.
            if (Number(row[2]) && !changed && kind === 'D') {
                changed = true;
                globals.transceivers[21] = NODES + 384;
                globals.pending[31] = NODES + 384;
                globals.synchronization.value = NODES + 384;
                globals.pendingCount.value = 0xffffffff;
                server = 0;
                hooks.node(token).depth = 7;
            }
            if (row[3] === kind) throw new ConnectionStop();
        };
        hooks.destroy = (token) => { effect('D', token); };
        hooks.release = (token) => { effect('F', token); };
        hooks.assertion = (line) => { effect('A', line); };
        const argument = Number(row[5]) | 0;
        try {
            switch (Number(row[0])) {
            case 0: deferConnectionClose(nodes[0], () => { effect('D', NODES); }, () => { effect('F', NODES); }); break;
            case 1: closePendingConnection(argument, globals, hooks); break;
            case 2: disconnectTransceiver(argument, globals, hooks); break;
            case 3: closeAllTransceivers(globals, hooks); break;
            case 4: endPlayerConnections(server, globals, hooks); break;
            case 5: closeAllPendingConnections(globals, hooks); break;
            default: assert.fail('알 수 없는 연산');
            }
        } catch (error) {
            if (!(error instanceof ConnectionStop)) throw error;
            interrupted = true;
            stopped += 1;
        }
        // 모든 표를 기대 출력과 독립적으로 직렬화해 입력 바이트 위에 쓴다.
        for (let index = 0; index < 22; index += 1) {
            write(bytes, index * 4, globals.transceivers[index]);
            write(bytes, 88 + index * 4, globals.buffers[index]);
        }
        for (let index = 0; index < 32; index += 1) write(bytes, 176 + index * 4, globals.pending[index]);
        write(bytes, 304, globals.synchronization.value);
        write(bytes, 308, globals.pendingCount.value);
        write(bytes, 312, server);
        nodes.forEach((node, index) => {
            const at = NODE_OFFSET + NODE_STRIDE * index;
            write(bytes, at + 0x34, node.flags);
            write(bytes, at + 0x40, node.sender);
            write(bytes, at + 0x44, node.received);
            write(bytes, at + 0x48, node.heartbeat);
            write(bytes, at + 0x8c, node.depth);
            assert.equal(node.delayed, false);
        });
        assert.deepEqual(Array.from(bytes), Array.from(hexBytes(row[7])), `행 ${rows} 출력`);
        assert.equal(calls || '-', row[8], `행 ${rows} 효과열`);
        assert.equal(interrupted, row[9] === '1', `행 ${rows} 중단`);
        rows += 1;
    }
    assert.deepEqual([rows, stopped], [580, 268]);
});

// 누락 장치를 선행 검사로 덮지 않으며 flags/sync 와 실제 소멸 효과의 완료 prefix 를 남긴다. (2026-11-11 WB04-E307)
test('연결 정리: 늦게 드러난 누락 장치는 원문 prefix 를 남김', () => {
    const globals = createConnectionCleanupGlobals();
    globals.synchronization.value = 71;
    globals.pendingCount.value = 1;
    const node = new PacketConnection();
    globals.transceivers[9] = 71;
    globals.buffers[9] = 72;
    const hooks = { node: (token) => { assert.equal(token, 71); return node; } };
    assert.ok(rejects(() => disconnectTransceiver(9, globals, hooks)));
    assert.deepEqual([globals.synchronization.value, globals.transceivers[9], globals.buffers[9], (node.flags & 0x40) !== 0], [0, 71, 72, true]);
    // 소멸 장치만 있고 free 장치가 없으면 소멸 효과가 완료된 prefix 를 남긴다.
    let destroyed = 0;
    hooks.destroy = (token) => { assert.equal(token, 71); destroyed += 1; node.depth = 7; };
    node.depth = 0;
    assert.ok(rejects(() => disconnectTransceiver(9, globals, hooks)));
    assert.deepEqual([destroyed, node.depth, globals.transceivers[9], globals.buffers[9]], [1, 7, 71, 72]);
    // 수신 중(깊이 > 0) 연결은 소멸/free 를 부르지 않고 칸을 분리하지만 뒤 버퍼 free 장치의 부재는 남긴다.
    node.depth = 1;
    assert.ok(rejects(() => disconnectTransceiver(9, globals, hooks)));
    assert.deepEqual([globals.transceivers[9], globals.buffers[9], destroyed], [0, 72, 1]);
});

// 장기 표 묶음의 칸 수와 초기 0 상태(독립 전송의 첫 BSS)를 확인한다. (2026-11-11 WB04-E307)
test('연결 정리: 장기 표 묶음의 칸 수와 초기값', () => {
    const globals = createConnectionCleanupGlobals();
    assert.deepEqual([globals.transceivers.length, globals.buffers.length, globals.pending.length], [22, 22, 32]);
    assert.deepEqual([globals.synchronization.value, globals.pendingCount.value], [0, 0]);
    for (const table of [globals.transceivers, globals.buffers, globals.pending]) assert.ok(table.every((value) => value === 0));
});

// Native 음수 경계의 자연 반환과 양수 진단이 실제 선택 위치에서만 장치를 요구한다. (2026-11-11 WB04-E307)
test('연결 정리: 빈 표와 범위 밖 음수는 장치를 요구하지 않음', () => {
    const globals = createConnectionCleanupGlobals();
    globals.synchronization.value = 19;
    globals.pendingCount.value = 7;
    // 없는 대상과 범위 밖 음수는 종료 장치를 요구하지 않는다.
    for (const id of [-2147483648, -33, -32, 0, 1, 21]) assert.doesNotThrow(() => disconnectTransceiver(id, globals, {}), `id ${id}`);
    closeAllTransceivers(globals, {});
    endPlayerConnections(1, globals, {});
    closeAllPendingConnections(globals, {});
    assert.deepEqual([globals.synchronization.value, globals.pendingCount.value], [19, 7]);
    // 22 이상은 줄 250 진단 뒤 부족한 실제 표를 거부한다(진단 장치가 없으면 그 위치에서 중단).
    assert.ok(rejects(() => disconnectTransceiver(22, globals, {})));
    let line = 0;
    assert.ok(rejects(() => disconnectTransceiver(22, globals, { assertion: (value) => { line = value; } })));
    assert.equal(line, 250);
    // server 가 0 이면 owner 종료는 아무 것도 하지 않는다.
    globals.transceivers[3] = NODES;
    assert.doesNotThrow(() => endPlayerConnections(0, globals, {}));
    assert.equal(globals.transceivers[3], NODES);
});

// 지연 닫힘: 깊이를 먼저 보관하고 flags 를 켠 뒤 깊이 0 일 때만 소멸/반납하며, 켜진 flags 는 unsigned 이다. (2026-11-11 WB04-E307)
test('연결 정리: 지연 닫힘의 깊이 보관과 flags', () => {
    const order = [];
    const idle = new PacketConnection({ flags: 0x80000000 });
    deferConnectionClose(idle, () => { order.push('destroy'); }, () => { order.push('release'); });
    assert.deepEqual(order, ['destroy', 'release']);
    assert.equal(idle.flags, 0x80000040);
    const busy = new PacketConnection({ depth: 2 });
    deferConnectionClose(busy, () => { order.push('x'); }, () => { order.push('y'); });
    assert.deepEqual(order, ['destroy', 'release']);
    assert.equal(busy.flags & 0x40, 0x40);
    // 소멸 뒤 깊이를 다시 읽지 않는다: 소멸이 깊이를 올려도 반납은 계속한다.
    const odd = new PacketConnection();
    deferConnectionClose(odd, () => { odd.depth = 5; order.push('d2'); }, () => { order.push('r2'); });
    assert.deepEqual(order.slice(-2), ['d2', 'r2']);
});

// pending 정리: 개수는 소멸 callback 뒤 현재값에서 감소하며(감김), 칸은 소멸 뒤 0 이고, 전체 정리는 개수/동기화를 쓰지 않는다. (2026-11-11 WB04-E307)
test('연결 정리: pending 개별·전체 정리의 개수 규칙', () => {
    const globals = createConnectionCleanupGlobals();
    const nodes = new Map([[NODES, new PacketConnection()], [NODES + 192, new PacketConnection()]]);
    const hooks = { node: (token) => nodes.get(token), destroy: () => {}, release: () => {} };
    globals.pending[2] = NODES;
    globals.pending[5] = NODES + 192;
    globals.pendingCount.value = 2;
    globals.synchronization.value = NODES;
    closePendingConnection(-2, globals, hooks);
    assert.deepEqual([globals.pending[2], globals.pending[5], globals.pendingCount.value, globals.synchronization.value], [0, NODES + 192, 1, NODES]);
    // 개수가 0 이면 unsigned 로 감긴다.
    globals.pendingCount.value = 0;
    closePendingConnection(-5, globals, hooks);
    assert.equal(globals.pendingCount.value, 0xffffffff);
    // 전체 pending 정리는 칸만 비우고 개수/동기화를 건드리지 않는다.
    globals.pending[7] = NODES;
    globals.pending[0] = NODES + 192;
    globals.pendingCount.value = 9;
    closeAllPendingConnections(globals, hooks);
    assert.deepEqual([Array.from(globals.pending).every((value) => value === 0), globals.pendingCount.value, globals.synchronization.value], [true, 9, NODES]);
    // 범위 밖 음수(0·-32 이하)는 아무 일도 없다.
    globals.pending[1] = NODES;
    closePendingConnection(0, globals, hooks);
    closePendingConnection(-32, globals, hooks);
    assert.equal(globals.pending[1], NODES);
});
