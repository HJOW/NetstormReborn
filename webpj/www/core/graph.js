// 서로 연결된 표면의 Graph 번호와 소속 관리(정수 표면 연결·flood·병합·삭제 분리·전체 재구성)를 옮긴 모듈이다(cpppj `o/Graph.cpp/.h`, 10.78 전용).
// 원본 그래프 레코드는 6 BYTE(WORD 세 개: surfaces·inUse·reserved)이고 표는 255행(정상 번호 0..250, 무효 번호 254 의 sentinel 0x7dfd, 251..253 은 틈)이며 flood 스택은 4096 DWORD 이다.
//  - add(00462d40 계열 004633c0): 활성 표면 이웃의 그래프 번호를 탐색 순서대로 중복 없이 모으고 가장 큰 그래프(동률은 첫 번째)에 붙이며 나머지는 flood 로 병합한다. 이웃이 없으면 새 번호를 확보해 flood 한다.
//  - flood(00462f10): 현재 객체의 번호를 먼저 바꾸고 y/x 순서 이웃을 push 하며 마지막 이웃부터 방문한다. 이미 목적 번호인 이웃은 확장하지 않고, 별도 visited 표 없이 중복 push 를 허용한다. WORD 증감은 16비트로 감긴다.
//  - detach/detachAt(004637b0): 죽은 원천의 이웃 무리를 탐색 순서로 분리하고 마지막 무리는 기존 번호에 남긴다. rebuild/removedSurfaces(1 또는 9)/recovery 는 분리 정책이다.
//  - rebuild(00463110): resetAll 이면 앞 251 행의 두 WORD 를 지우고 0번 예약·reserved low byte·sentinel 을 복원한 뒤, 임시 번호로 모은 표면을 SID 오름차순으로 flood 해 독립 무리마다 번호를 배정한다.
// 모든 공개 연산은 복사본에서 실행한 뒤 성공했을 때만 표/스택/번호를 반영하므로(원본의 부분 변경을 호출자에게 전파하지 않음) 오류가 나면 기존 표·번호·스택이 그대로 남는다.
// 표면 정보(위치·flags·이웃)는 SurfaceFinder 가 제공하며 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E12: Graph 정수 연산 이식으로 추가)
import { WORLD_CELLS } from '../assets/terrain-mask.js';
import { TYPE_FLAG1_SURFACE } from './surface-finder.js';

// 정상 그래프 수·무효 번호·전체 표 폭·flood 스택 크기이다. (2026-11-08 WB04-E12)
export const GRAPH_COUNT = 251;
export const GRAPH_TABLE_SIZE = 255;
export const GRAPH_FLOOD_SIZE = 4096;
export const GRAPH_INVALID = 254;

// 전역 재구성은 전체 표면/풀 계약을 보장한 입력에서만 선택한다. (2026-11-08 WB04-E12)
export const GRAPH_RECOVERY = Object.freeze({ reject: 'Reject', fullPool: 'FullPool' });

// 원본 WORD 증가/감소의 low 16비트와 signed 해석을 보존한다. (2026-11-08 WB04-E12)
const word = (value) => (((value & 0xffff) << 16) >> 16);

// sentinel 행(무효 번호)의 초기 두 WORD 값이다. (2026-11-08 WB04-E12)
const SENTINEL = 0x7dfd;

// 오류를 만들 때 쓰는 원문 진단 파일 이름이다. (2026-11-08 WB04-E12)
const GRAPH_FILE = '\\Ns\\O\\Graph.cpp';

// 번호 검사: 251..253 은 정상 그래프도 sentinel 도 아니다. (2026-11-08 WB04-E12)
function checkNumber(graph, allowInvalid) {
    if (graph >= GRAPH_COUNT && !(allowInvalid && graph === GRAPH_INVALID)) throw new RangeError('그래프 번호 오류');
}

// 00462a50: 현재 6 BYTE 행을 처리한다. 진단({ checking(), assertion(식, 파일, 줄) })이 고친 같은 행을 다시 읽고 reserved 는 보존한다. (2026-11-08 WB04-E12)
export function removeGraphSurface(record, diagnostics = {}) {
    if (diagnostics.checking?.()) {
        if (!record.inUse) {
            if (typeof diagnostics.assertion !== 'function') throw new Error('Graph inUse 실제 진단 미연결');
            diagnostics.assertion('inUse', GRAPH_FILE, 0x59);
        }
        if (diagnostics.checking() && record.surfaces < 1) {
            if (typeof diagnostics.assertion !== 'function') throw new Error('Graph surfaces 실제 진단 미연결');
            diagnostics.assertion('numSurfaces > 0', GRAPH_FILE, 0x5a);
        }
    }
    if (record.inUse && record.surfaces > 0) {
        record.surfaces = word(record.surfaces - 1);
        if (!record.surfaces) record.surfaces = record.inUse = 0;
    }
}

// 00462ac0: 현재 sentinel 두 WORD 를 검사한다. 진단 복귀 뒤 checking 과 두 번째 WORD 를 다시 읽는다. (2026-11-08 WB04-E12)
export function checkGraphSentinel(sentinel, diagnostics = {}) {
    if (!diagnostics.checking?.()) return;
    if (sentinel.surfaces !== SENTINEL) {
        if (typeof diagnostics.assertion !== 'function') throw new Error('Graph sentinel surfaces 실제 진단 미연결');
        diagnostics.assertion('graphs[INVALID_GRAPH].numSurfaces == 0x7DFD', GRAPH_FILE, 0x68);
    }
    if (diagnostics.checking() && sentinel.inUse !== SENTINEL) {
        if (typeof diagnostics.assertion !== 'function') throw new Error('Graph sentinel inUse 실제 진단 미연결');
        diagnostics.assertion('graphs[INVALID_GRAPH].inUse == 0x7DFD', GRAPH_FILE, 0x69);
    }
}

// 작업 상태(표·스택·소속)를 깊이 복사한다. 공개 연산은 복사본에서 실행하고 성공했을 때만 반영한다. (2026-11-08 WB04-E12)
function copyState(state) {
    return {
        records: state.records.map((record) => ({ ...record })),
        stack: Uint32Array.from(state.stack),
        members: new Map(Array.from(state.members, ([id, member]) => [id, { ...member }])),
    };
}

// 첫 미사용 번호만 확보한다. 지역 스냅샷의 자동 전역 재구성은 허용하지 않는다. (2026-11-08 WB04-E12)
function allocate(state) {
    for (let index = 0; index < GRAPH_COUNT; index += 1) {
        if (!state.records[index].inUse) {
            state.records[index].surfaces = 0;
            state.records[index].inUse = 1;
            return index;
        }
    }
    throw new Error('그래프가 소진되어 전체 풀 재구성이 필요합니다');
}

// 무효 번호의 sentinel 과 reserved WORD 를 보존한다. (2026-11-08 WB04-E12)
function free(state, graph) {
    checkNumber(graph, true);
    if (graph === GRAPH_INVALID) return;
    state.records[graph].surfaces = state.records[graph].inUse = 0;
}

// 소속 객체가 없는 번호는 거부한다(원본 std::map::at 의 경계 검사). (2026-11-08 WB04-E12)
function memberOf(state, id) {
    const member = state.members.get(id);
    if (!member) throw new RangeError('그래프 소속 객체 없음');
    return member;
}

// 원본처럼 먼저 현재 객체의 번호를 바꾼 뒤 y/x 순서 이웃을 push 하고 마지막 이웃부터 방문한다. (2026-11-08 WB04-E12)
function floodImpl(context, state, id, graph) {
    memberOf(state, id);
    state.stack[0] = id;
    let depth = 1;
    let changed = 0;
    // 방문 번호가 이미 목적 그래프라면 이웃을 확장하지 않는다. 별도 visited 표는 사용하지 않는다.
    while (depth) {
        depth -= 1;
        const current = state.stack[depth] & 0xffff;
        const member = memberOf(state, current);
        if (member.graph === graph) continue;
        if (member.graph !== GRAPH_INVALID) state.records[member.graph].surfaces = word(state.records[member.graph].surfaces - 1);
        if (graph !== GRAPH_INVALID) state.records[graph].surfaces = word(state.records[graph].surfaces + 1);
        member.graph = graph;
        changed += 1;
        // 동일한 목적 번호가 아닌 이웃은 중복 push 도 허용한다.
        for (const neighbor of context.surfaces.neighbors(current)) {
            if (memberOf(state, neighbor).graph === graph) continue;
            if (depth + 1 >= GRAPH_FLOOD_SIZE) throw new RangeError('그래프 flood 스택 범위 오류');
            state.stack[depth] = neighbor;
            depth += 1;
        }
    }
    return changed;
}

// 공개 함수의 성공 후 대입까지 같은 표/번호 컨테이너를 유지하는 번호 확보(소진 시 전체 재구성)이다. (2026-11-08 WB04-E12)
function allocateForOperation(context, state, recovery) {
    if (recovery === GRAPH_RECOVERY.fullPool && state.records.slice(0, GRAPH_COUNT).every((record) => record.inUse)) rebuildImpl(context, state, true);
    return allocate(state);
}

// 0번 예약·reserved low byte·sentinel 을 복원하고 SID 오름차순 두 순회를 수행한다. (2026-11-08 WB04-E12)
function rebuildImpl(context, state, resetAll) {
    if (resetAll) {
        // 앞 251개 레코드의 첫 두 WORD 만 지운다. 251..253 과 reserved WORD 는 보존한다.
        for (let index = 0; index < GRAPH_COUNT; index += 1) state.records[index].surfaces = state.records[index].inUse = 0;
        state.records[0].inUse = 1;
        state.records[0].reserved = word((state.records[0].reserved & 0xff00) | 1);
        state.records[GRAPH_INVALID].surfaces = state.records[GRAPH_INVALID].inUse = SENTINEL;
    }
    const temporary = allocate(state);
    // free 표면은 원본 전역 iterator 가 건너뛴다. dead/void/contained/buried 는 이 순회의 제외 조건이 아니다.
    for (const id of context.ids) {
        const member = state.members.get(id);
        if ((member.state & 1) || (!resetAll && member.graph !== GRAPH_INVALID) || context.surfaces.originInterior(id)) continue;
        member.graph = temporary;
        state.records[temporary].surfaces = word(state.records[temporary].surfaces + 1);
    }
    // 이전 flood 가 바꾼 번호를 매번 읽는다. 기준점 내부라도 기존 번호가 임시 번호이면 원본처럼 처리한다.
    for (const id of context.ids) {
        const member = state.members.get(id);
        if (!(member.state & 1) && member.graph === temporary) floodImpl(context, state, id, allocate(state));
    }
    free(state, temporary);
}

// 유효/활성 표면만 처리하고 가장 큰 연결 그래프의 동률에서는 첫 탐색 결과를 유지한다. (2026-11-08 WB04-E12)
function addImpl(context, state, id, recovery) {
    const source = memberOf(state, id);
    const object = context.surfaces.object(id);
    if (object.x <= 0 || object.y <= 0 || object.x >= WORLD_CELLS || object.y >= WORLD_CELLS || (source.state & 7)) return;
    const connections = [];
    let biggestSize = -2147483648;
    let biggest = 0;
    // 탐색 순서대로 그래프 번호 중복을 제거한다. 이웃 객체 번호가 다른 경우도 한 연결이다.
    for (const neighbor of context.surfaces.neighbors(id)) {
        const graph = memberOf(state, neighbor).graph;
        if (graph === GRAPH_INVALID || connections.some((other) => memberOf(state, other).graph === graph)) continue;
        if (connections.length === 64) throw new RangeError('그래프 연결 목록 범위 오류');
        connections.push(neighbor);
        if (state.records[graph].surfaces > biggestSize) {
            biggest = graph;
            biggestSize = state.records[graph].surfaces;
        }
    }
    if (!connections.length) {
        const number = allocateForOperation(context, state, recovery);
        floodImpl(context, state, id, number);
        return;
    }
    const winner = state.records[biggest];
    if (!winner.inUse) winner.inUse = 1;
    source.graph = biggest;
    winner.surfaces = word(winner.surfaces + 1);
    if (connections.length > 1) {
        // 앞서 flood 한 연결의 번호도 다시 읽으며 패배 레코드의 reserved WORD 를 보존한다.
        for (const neighbor of connections) {
            if (memberOf(state, neighbor).graph === biggest) continue;
            const loser = memberOf(state, neighbor).graph;
            floodImpl(context, state, neighbor, biggest);
            free(state, loser);
        }
    }
}

// 원본은 남은 연결의 마지막 하나를 기존 그래프에 남긴다. 다른 번호의 이웃도 남은 수에 포함한다. (2026-11-08 WB04-E12)
function detachImpl(context, state, id, number, connections, rebuild, removedSurfaces, recovery, sourceFlags2) {
    if (number === GRAPH_INVALID || !state.records[number].inUse) return;
    if (connections.length >= 64) throw new RangeError('그래프 삭제 연결 목록 범위 오류');
    let remaining = connections.length;
    let keep = !rebuild;
    // 이전 flood 가 바꾼 번호를 매번 다시 읽는다. 연결 그래프 번호를 미리 중복 제거하지 않는다.
    for (const neighbor of connections) {
        if (memberOf(state, neighbor).graph === number) {
            if (!rebuild && remaining < 2) {
                // 특수 프레임의 삭제 정보만 치환한다. 전역 재구성의 실제 객체 타입은 유지한다.
                const flags2 = sourceFlags2 ?? context.surfaces.object(id).flags2;
                if (!(flags2 & 8)) keep = true;
            } else {
                const allocated = allocateForOperation(context, state, recovery);
                floodImpl(context, state, neighbor, allocated);
            }
        }
        remaining -= 1;
    }
    // 고립된 원천은 감소하지 않는 원본 분기를 보존한다. 특수 타입은 9 를 감소시킨다.
    if (connections.length) state.records[number].surfaces = word(state.records[number].surfaces - removedSurfaces);
    if (!keep) {
        state.records[number].surfaces = 0;
        state.records[number].inUse = 0;
    }
}

// 서로 연결된 표면의 Graph 번호와 소속 목록을 관리하는 처리기이다.
//  - surfaces: SurfaceFinder(수명이 Graph 보다 길어야 한다).
//  - members: [{ id, graph(기본 254), state }] 표면 스냅샷 번호(state bit1 = free, bit2 = dead).
//  - records: 생략하면 빈 표와 sentinel, 주면 255행([{ surfaces, inUse, reserved }]).  stack: 생략하면 0, 주면 4096 DWORD.
// (2026-11-08 WB04-E12)
export class Graph {
    #context;
    #state;

    // 모든 membership 의 표면/상태/번호를 확인하며 판독한 스냅샷을 변경하지 않는다.
    constructor(surfaces, members, records = [], stack = []) {
        if (records.length !== 0 && records.length !== GRAPH_TABLE_SIZE) throw new RangeError('그래프 표 크기 오류');
        if (stack.length !== 0 && stack.length !== GRAPH_FLOOD_SIZE) throw new RangeError('그래프 스택 크기 오류');
        const table = records.length === 0 ? Array.from({ length: GRAPH_TABLE_SIZE }, () => ({ surfaces: 0, inUse: 0, reserved: 0 })) : records.map((record) => ({ surfaces: record.surfaces, inUse: record.inUse, reserved: record.reserved }));
        if (records.length === 0) table[GRAPH_INVALID] = { surfaces: SENTINEL, inUse: SENTINEL, reserved: 0 };
        const map = new Map();
        // 입력 객체는 이미 확보한 실제 표면으로 한정한다. 비표면의 위치 조회 경로는 별도다.
        for (const entry of members) {
            const member = { id: entry.id, graph: entry.graph ?? GRAPH_INVALID, state: entry.state ?? 0 };
            checkNumber(member.graph, true);
            const object = surfaces.object(member.id);
            if (!((object.flags1 >>> 0) & TYPE_FLAG1_SURFACE) || Boolean(object.dead) !== ((member.state & 2) !== 0) || map.has(member.id)) {
                throw new RangeError('그래프 표면/상태/중복 번호 오류');
            }
            map.set(member.id, member);
        }
        // std::map 순서(번호 오름차순)대로 순회하도록 번호를 정렬해 둔다.
        this.#context = { surfaces, ids: Array.from(map.keys()).sort((a, b) => a - b) };
        this.#state = { records: table, stack: stack.length ? Uint32Array.from(stack) : new Uint32Array(GRAPH_FLOOD_SIZE), members: map };
    }

    // 복사본에서 연산을 실행하고 성공했을 때만 표/스택/번호를 반영한다.
    #atomically(operation) {
        const next = copyState(this.#state);
        const result = operation(next);
        this.#state = next;
        return result;
    }

    // 첫 미사용 번호만 확보한다. 지역 스냅샷의 자동 전역 재구성은 허용하지 않는다.
    allocate() { return allocate(this.#state); }

    // 00463330: 소진 경로. 전체 표면 입력에서 재구성 뒤 첫 번호를 확보한다.
    allocateWithRecovery() { return this.#atomically((state) => allocateForOperation(this.#context, state, GRAPH_RECOVERY.fullPool)); }

    // 00463110: 전체 SID 순회·임시 번호·flood·임시 해제를 복원한다. resetAll=false 는 기존 표에 여유가 있을 때 무효 번호만 수집한다.
    rebuild(resetAll) { this.#atomically((state) => rebuildImpl(this.#context, state, resetAll)); }

    // 00462e50: 254 는 그대로 두고 유효 레코드의 앞 두 WORD 만 지운다.
    free(graph) { free(this.#state, graph); }

    // 00462a50: 활성/양수일 때 감소하고 0 이 되면 미사용으로 전환한다.
    remove(graph, diagnostics = {}) {
        checkNumber(graph, false);
        removeGraphSurface(this.#state.records[graph], diagnostics);
    }

    // 00462f10: 원본 LIFO 순서·중복 push·WORD 감김·변경 수를 유지한다.
    flood(id, graph) {
        checkNumber(graph, true);
        return this.#atomically((state) => floodImpl(this.#context, state, id, graph));
    }

    // 004633c0: 가장 큰 이웃 그래프에 붙이고 나머지를 flood 로 병합한다.
    add(id, recovery = GRAPH_RECOVERY.reject) { this.#atomically((state) => addImpl(this.#context, state, id, recovery)); }

    // 004637b0: 삭제 준비. 죽은 원천의 이웃 무리를 탐색 순서로 분리한다. rebuild 는 분할 정책이며 recovery 와 구별한다.
    detach(id, connections, rebuild = false, removedSurfaces = 1, recovery = GRAPH_RECOVERY.reject) {
        this.detachAt(id, memberOf(this.#state, id).graph, connections, rebuild, removedSurfaces, recovery);
    }

    // 위치 조회의 다른 SID 에서 고른 번호와 삭제 원천의 타입/프레임을 구별한다. sourceFlags2 는 특수 위치 정보 타입의 bit 8 판단에만 쓴다.
    detachAt(id, graph, connections, rebuild = false, removedSurfaces = 1, recovery = GRAPH_RECOVERY.reject, sourceFlags2 = null) {
        checkNumber(graph, true);
        if (!(memberOf(this.#state, id).state & 2) || (removedSurfaces !== 1 && removedSurfaces !== 9)) throw new RangeError('그래프 삭제 준비의 dead/감소 수 오류');
        this.#atomically((state) => detachImpl(this.#context, state, id, graph, connections, rebuild, removedSurfaces, recovery, sourceFlags2 ?? undefined));
    }

    // 레코드의 읽기 사본이다(미사용 흔적과 reserved 포함 255행).
    records() { return this.#state.records.map((record) => ({ ...record })); }

    // 사용 후 흔적을 포함한 전체 스택의 읽기 사본이다.
    floodStack() { return Uint32Array.from(this.#state.stack); }

    // 표면 객체의 현재 그래프 번호이다.
    number(id) { return memberOf(this.#state, id).graph; }

    // 그래프 연산이 보존한 raw 상태 입력이다.
    state(id) { return memberOf(this.#state, id).state; }

    // 앞 251개 레코드에서 inUse 가 0 이 아닌 개수이다.
    inUse() { return this.#state.records.slice(0, GRAPH_COUNT).filter((record) => record.inUse !== 0).length; }
}
