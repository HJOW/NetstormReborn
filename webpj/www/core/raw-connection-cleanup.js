// 실제 연결표와 수신 깊이의 원문 정리 순서를 옮긴 모듈이다(cpppj `o/RawConnectionCleanup.cpp/.h`, 10.78: 004df440·004df520·004e2940·004e29c0·004e2a10·004df4a0/004e2a00).
// 사용: const globals = createConnectionCleanupGlobals(); closeAllTransceivers(globals, { node, destroy, release, assertion }) …
//   globals 는 같은 장기 표 세 개(현재 22 연결·22 송신버퍼·32 pending)와 현재 동기화 토큰/pending 개수 상자(`{ value }`)를 **빌린다**(새 0 표로 대체하지 않음).
//   hooks.node(token) → 같은 실제 연결 객체, destroy(token) = 004e03d0 고정 연결 소멸, release(token) = 004e4354 연결 또는 별도 송신버퍼 free, assertion(line) = 004e0620 줄 250 진단이다.
// 연결 숫자 토큰은 호출자가 같은 실제 연결 저장소에서 해석한다. 각 슬롯은 callback 뒤의 현재 값을 다시 읽고(앞 칸을 채워도 같은 호출에서 되돌아가지 않음), 소멸이 연결을 반납한 뒤 노드를 다시 읽지 않는다.
// 선택한 장치는 도달한 위치에서만 요구하고 빠졌으면 앞선 쓰기를 남긴 채 중단한다. 게임 모드/Player/로그인 성공은 만들지 않는다. 기준선 뒤 cpppj 변경은 없다(3.6절 ①). CD(10.72)는 옮기지 않았다.
// 원본 독립 관측 연결 정리 580행을 시험이 재생한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E307: 4-C RawConnectionCleanup 이식)

// 005e4a18..005e4a6c 의 현재 연결 수(22)와 005e46f0..005e476c 의 pending 칸 수(32, 0 칸도 전체 정리에 포함)이다. (2026-11-11 WB04-E307)
export const TRANSCEIVER_SLOTS = 22;
export const PENDING_CONNECTION_SLOTS = 32;

// 닫는 중임을 나타내는 연결 flags 비트이다. (2026-11-11 WB04-E307)
const CLOSING = 0x40;

// 새 장기 표 묶음을 만든다(독립 전송의 첫 BSS 0). 실제 Client 는 같은 표/상자를 계속 빌린다. (2026-11-11 WB04-E307)
export function createConnectionCleanupGlobals() {
    return {
        transceivers: new Uint32Array(TRANSCEIVER_SLOTS),
        buffers: new Uint32Array(TRANSCEIVER_SLOTS),
        pending: new Uint32Array(PENDING_CONNECTION_SLOTS),
        synchronization: { value: 0 },
        pendingCount: { value: 0 },
    };
}

// 선택한 외부 장치가 함수가 아니면 도달한 위치에서 오류를 낸다(앞선 실제 쓰기는 그대로 남음). (2026-11-11 WB04-E307)
function need(device) {
    if (typeof device !== 'function') throw new Error('연결 정리의 선택 장치 누락');
    return device;
}

// 표의 index 가 정수이고 범위 안임을 읽기/쓰기 전에 확인한다(호스트 span 의 물리 부족은 해당 슬롯 위치에서 거부). (2026-11-11 WB04-E307)
function checked(entries, index) {
    if (!Number.isInteger(index) || index < 0 || index >= entries.length) throw new RangeError('연결 정리의 실제 표 범위 부족');
    return index;
}

// 동일 토큰의 실제 연결에만 지연 삭제 부모를 적용하고 반납 뒤 노드를 읽지 않는다. (2026-11-11 WB04-E307)
function closeToken(token, hooks) {
    const connection = need(hooks.node)(token);
    deferConnectionClose(connection, () => { need(hooks.destroy)(token); }, () => { need(hooks.release)(token); });
}

// 실제 양수 연결 한 칸의 현재 동기화 해제와 종료 후 0 쓰기를 공유한다. (2026-11-11 WB04-E307)
function closeMainSlot(globals, index, hooks) {
    const token = globals.transceivers[checked(globals.transceivers, index)];
    if (!token) return;
    if (token === globals.synchronization.value) globals.synchronization.value = 0;
    closeToken(token, hooks);
    globals.transceivers[index] = 0;
}

// 004df440: 진입 깊이 보관 → flags |= 0x40 → 깊이 0 에서만 소멸 → 반납이다. 소멸 뒤 깊이를 다시 읽지 않는다(수신 중이면 호출 쪽 분배기가 닫힘을 처리). (2026-11-11 WB04-E307)
export function deferConnectionClose(connection, destroy, release) {
    const depth = connection.depth;
    connection.flags = (connection.flags | CLOSING) >>> 0;
    if (depth === 0) {
        need(destroy)();
        need(release)();
    }
}

// 004df520: -31..-1 의 실제 pending 만 종료하고 현재 개수 감소 → 선택 칸 0 → 개수 store 를 수행한다(없는 칸/범위 밖은 아무 일도 없음). (2026-11-11 WB04-E307)
export function closePendingConnection(id, globals, hooks) {
    if (id >= 0 || id <= -32) return;
    const index = -id;
    const token = globals.pending[checked(globals.pending, index)];
    if (!token) return;
    closeToken(token, hooks);
    // 소멸 callback 이 개수를 바꿨어도 현재 값에서 감소하며 unsigned 감김을 유지한다.
    const count = (globals.pendingCount.value - 1) >>> 0;
    globals.pending[index] = 0;
    globals.pendingCount.value = count;
}

// 004e2940: 음수 pending 위임 또는 양수 연결과 현재 송신버퍼의 개별 종료이다. 22 이상은 줄 250 진단 뒤(반환하면) 부족한 실제 표를 거부한다. (2026-11-11 WB04-E307)
export function disconnectTransceiver(id, globals, hooks) {
    if (id < 0) {
        closePendingConnection(id, globals, hooks);
        return;
    }
    if (id >= TRANSCEIVER_SLOTS) need(hooks.assertion)(250);
    closeMainSlot(globals, id, hooks);
    const buffer = globals.buffers[checked(globals.buffers, id)];
    if (buffer) {
        need(hooks.release)(buffer);
        globals.buffers[id] = 0;
    }
}

// 004e29c0: 현재 0..21 을 한 번 순회하며 동기화/연결만 정리하고 버퍼표는 보존한다. 방문 뒤 callback 이 앞 칸을 채워도 같은 호출에서 되돌아가지 않는다. (2026-11-11 WB04-E307)
export function closeAllTransceivers(globals, hooks) {
    // 22 연결 칸을 앞에서부터 한 번씩 정리한다.
    for (let index = 0; index < TRANSCEIVER_SLOTS; index += 1) closeMainSlot(globals, index, hooks);
}

// 004e2a10: 진입 server 가 비영일 때만 1..8 연결을 종료하며 각 칸의 현재값을 다시 읽는다. (2026-11-11 WB04-E307)
export function endPlayerConnections(server, globals, hooks) {
    if (!server) return;
    // owner 1..8 의 연결 칸을 차례로 정리한다.
    for (let index = 1; index < 9; index += 1) closeMainSlot(globals, index, hooks);
}

// 004df4a0/004e2a00: 현재 0..31 pending 을 종료하고 개수/동기화/양수표는 쓰지 않는다. (2026-11-11 WB04-E307)
export function closeAllPendingConnections(globals, hooks) {
    // 32 pending 칸을 앞에서부터 한 번씩 정리하고 칸을 0 으로 만든다(개수는 이 부모가 갱신하지 않음).
    for (let index = 0; index < PENDING_CONNECTION_SLOTS; index += 1) {
        const token = globals.pending[index];
        if (token) {
            closeToken(token, hooks);
            globals.pending[index] = 0;
        }
    }
}
