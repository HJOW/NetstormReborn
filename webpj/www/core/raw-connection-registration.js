// 실제 연결 등록/재배치와 pending 제거의 원문 store 순서를 옮긴 모듈이다(cpppj `o/RawConnectionRegistration.cpp/.h`, 10.78: 004e2870·004df4f0·004e27b0).
// 사용: registerTransceiver(id, token, globals, hooks) · removePendingConnection(id, globals) · assignPendingConnection(pending, target, server, globals, hooks).
//   globals 는 연결 정리 모듈의 같은 장기 표 묶음(`createConnectionCleanupGlobals`)이고 hooks = { node(token) → 같은 실제 연결 객체, buffered(token) = 004df470(node,1) 의 현재 연결 설정, assertion(line) = 원문 줄 b0/b3/b6/bb/c2 진단 }이다.
// callback 복귀 뒤 현재 슬롯/개수를 다시 읽고 앞선 쓰기를 실패 시에도 보존한다(선택한 진단/장치는 도달한 위치에서만 요구하고 빠졌으면 그 위치에서 중단). 호스트 표 물리 부족은 도달한 읽기 위치에서만 거부한다.
// 기준선 뒤 cpppj 변경은 없다(3.6절 ①). CD(10.72)는 옮기지 않았다. 원본 독립 관측 288행을 시험이 재생한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E307: 4-C RawConnectionRegistration 이식)

// 선택한 외부 장치가 함수가 아니면 도달한 위치에서 오류를 낸다. (2026-11-11 WB04-E307)
function need(device) {
    if (typeof device !== 'function') throw new Error('연결 등록의 선택 장치 누락');
    return device;
}

// signed 인덱스가 표 안인지 확인한다(INT_MIN 도 오버플로 없이 범위 밖으로 거부). (2026-11-11 WB04-E307)
function checked(entries, index) {
    if (!Number.isInteger(index) || index < 0 || index >= entries.length) throw new RangeError('연결 등록의 실제 표 범위');
    return index;
}

// 004e2870: 현재 대상 칸이 차 있으면 줄 c2 진단 → 새 연결 store → 동일 노드 sender store 이다. sender 는 id 의 unsigned 비트이다. (2026-11-11 WB04-E307)
export function registerTransceiver(id, token, globals, hooks) {
    if (globals.transceivers[checked(globals.transceivers, id)]) need(hooks.assertion)(0xc2);
    globals.transceivers[id] = token;
    need(hooks.node)(token).sender = id >>> 0;
}

// 004df4f0: pending[-id] = 0 → 현재 개수 감소이며 입력 검증을 앞당기지 않는다. 개수 감소는 unsigned 감김을 유지한다. (2026-11-11 WB04-E307)
export function removePendingConnection(id, globals) {
    globals.pending[checked(globals.pending, -id)] = 0;
    globals.pendingCount.value = (globals.pendingCount.value - 1) >>> 0;
}

// 004e27b0: 서버/대상 진단 → 현재 pending 을 양수 표에 store → sender → buffered → (pending 이 비음수면 줄 bb 진단) → pending 제거이다. 대상을 돌려준다. (2026-11-11 WB04-E307)
export function assignPendingConnection(pending, target, server, globals, hooks) {
    // 서버면 대상이 1..8, 아니면 9 이어야 한다(아니면 줄 b0/b3 진단 뒤 계속).
    if (server) {
        if (!(target > 0 && target < 9)) need(hooks.assertion)(0xb0);
    } else if (target !== 9) need(hooks.assertion)(0xb3);
    if (globals.transceivers[checked(globals.transceivers, target)]) need(hooks.assertion)(0xb6);
    const token = globals.pending[checked(globals.pending, -pending)];
    globals.transceivers[target] = token;
    need(hooks.node)(token).sender = target >>> 0;
    need(hooks.buffered)(globals.transceivers[target]);
    if (pending >= 0) need(hooks.assertion)(0xbb);
    removePendingConnection(pending, globals);
    return target;
}
