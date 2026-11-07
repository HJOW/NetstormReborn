// 다리 수명·한 칸 붕괴(004227e0/00421c30)와 프레임 스캔(00422bc0)을 실제 raw SID 풀 위에서 옮긴 모듈이다(cpppj `o/RawBridgeDecay.cpp/.h` + `o/RawBridgeDecayScan.cpp/.h`, 10.78 전용).
// 순수 논리 `bridge-decay.js`가 정수 스냅샷에서 하던 일을 같은 원문 순서로 현재 raw 슬롯에 직접 적용한다: 슬롯/표면 지도/Graph 표는 호출 중에도 현재값을 읽고, 각 방문의 삭제·WORD·프레임 쓰기가 만든 변화를
// 다음 방문에서 다시 읽으며, 진단(assertion)이 돌려준 뒤에도 같은 슬롯에서 계속한다. 외부 효과는 훅으로 받는다.
//  - RawBridgeDecay(pool, types, frames, surface, spots, graphs, mode, hooks, { authority, checking })
//      pool: 10.78 SidPool, types: 188 타입 {flags1, flags2, footX, footY}, frames: 타입별 프레임 코드 배열, surface: 표면 지도로 쓰는 SquidHash(0단계 머리, 이웃 순회와 열린 방향 검사가 같이 읽음),
//      spots: 256×256 spot, graphs: 255행 Graph 표({surfaces} 현재값), mode: { server, authority, editor, debugRedraw, debugKeep }(현재 객체를 그대로 읽음),
//      hooks: { destroy(sid, flags), notifySurface(sid), refresh(sid), carriers(sid), crackSound(sid), assertion(식, 파일, 줄) }, authority: { server } 현재 서버 전역(없으면 mode.server), checking: () => 현재 DWORD.
//      cell(root): 작은 Graph → 단단한 프레임 → 이웃/열린 끝 → 방문 목록 → 최소 수명 → 방문별 수명(현재 dead 가 아닌 칸) → 금 소리. applyLife(sid, reduction): 정상 7 초과를 미리 거부하지 않고 원문의 signed 산술/4비트 쓰기를 보존한다.
//  - RawBridgeDecayScan(pool, types, state, period, mode, cell): 정지·편집기·권한 관문 뒤 기한/주기에 맞춘 부분 방문과 기한 재설정. period 는 현재 float 를 돌려주는 함수, state 는 { first, last, cursor, next } 를 직접 바꾼다.
// 이 모듈은 DOM 을 쓰지 않는다. CD 10.72 분기는 옮기지 않았다. (2026-11-08 WB04-E45: RawBridgeDecay/RawBridgeDecayScan 이식으로 추가)
import { BRIDGE_CARRIER_LIFE, BRIDGE_CRACK_LIFE, BRIDGE_HARD_FRAME, BRIDGE_LIFE_MASK, BRIDGE_MINIMUM_GRAPH_SURFACES, BRIDGE_PLANK_REMOVAL, bridgeCrackedFrame, bridgeOpenDirection } from './bridge-decay.js';
import { NEIGHBOR_FLAG, RawSquidNeighborWalk, RawSquidNeighbors } from './raw-squid-neighbors.js';

// 슬롯 필드 위치(타입·상태·수명 단어·좌표·그래프 번호·프레임·extra)와 타입 수, 지도 한 변이다. (2026-11-08 WB04-E45)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const WORD_OFFSET = 12;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const GRAPH_OFFSET = 30;
const FRAME_OFFSET = 36;
const EXTRA_OFFSET = 40;
const TYPE_COUNT = 188;
const GRAPH_COUNT = 255;
const INVALID_GRAPH = 254;

// 타입 genus(flags2): island(2)·bridge(4), 방문 목록 용량(0x64), 열린 방향 이웃 목록 용량(10), 안전 재귀 깊이(256, 원본 상수 아님)이다. (2026-11-08 WB04-E45)
const GENUS_ISLAND = 2;
const GENUS_BRIDGE = 4;
const VISITED_CAPACITY = 100;
const NEIGHBOR_LIST_CAPACITY = 10;
const SAFE_RECURSION_DEPTH = 256;
const SURFACE_SIDE = 256;

// 정렬되지 않은 리틀엔디언 정수 읽기와 쓰기, float 비트 읽기이다. (2026-11-08 WB04-E45)
const floatView = new DataView(new ArrayBuffer(4));
function read(raw, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 합친다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}
function write(raw, offset, value, width = 4) {
    // 원문 지정 폭의 낮은 바이트부터 쓴다.
    for (let index = 0; index < width; index += 1) raw[offset + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}
function coordinate(raw, offset) {
    floatView.setUint32(0, read(raw, offset, 4), true);
    return floatView.getFloat32(0, true);
}

// 선택된 현재 효과가 빠졌으면 앞선 WORD/프레임 쓰기 상태에서 오류를 전달한다. (2026-11-08 WB04-E45)
function need(device) {
    if (typeof device !== 'function') throw new Error('다리 붕괴의 실제 효과 미연결');
    return device;
}

// 다리 한 칸 붕괴 실행기이다.
export class RawBridgeDecay {
    #pool;
    #types;
    #frames;
    #surface;
    #spots;
    #graphs;
    #mode;
    #hooks;
    #authority;
    #checking;
    #neighbors;

    // 초기화/구성은 저장된 수명·스캔·Graph 를 수정하지 않는다.
    constructor(pool, types, frames, surface, spots, graphs, mode, hooks, { authority = null, checking = () => 0 } = {}) {
        if (pool?.edition !== 'patch1078' || types.length !== TYPE_COUNT || frames.length !== types.length || spots.length !== SURFACE_SIDE * SURFACE_SIDE || graphs.length !== GRAPH_COUNT) {
            throw new TypeError('다리 붕괴의 PATCH 풀/현재 표면/Graph 표 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#surface = surface;
        this.#spots = spots;
        this.#graphs = graphs;
        this.#mode = mode;
        this.#hooks = hooks;
        this.#authority = authority;
        this.#checking = checking;
        this.#neighbors = new RawSquidNeighbors(pool, surface, spots, types, frames);
    }

    // 프레임 코드를 현재 TYPE 과 DWORD 프레임에서 읽는다. 잘못된 프레임은 해당 칸의 원문 예외 경계로 올린다.
    #code(sid) {
        const raw = this.#pool.physical(sid);
        if (raw[TYPE_OFFSET] >= this.#frames.length) throw new RangeError('다리 붕괴의 현재 TYPE 범위');
        const codes = this.#frames[raw[TYPE_OFFSET]];
        const number = read(raw, FRAME_OFFSET);
        if (number >= codes.length) throw new RangeError('다리 붕괴의 현재 프레임 범위');
        return codes[number];
    }

    // 생성된 원천 기하만 고정하고 후보/지도는 이미 복원된 flag 8 순회에서 읽는다. 원문 y/x·중복 제거 순서를 보존한다.
    #neighborList(sid) {
        const result = [];
        const walk = new RawSquidNeighborWalk(this.#neighbors, sid, NEIGHBOR_FLAG.surfaceMap);
        for (let neighbor = walk.current; neighbor !== 0; neighbor = walk.next()) result.push(neighbor);
        return result;
    }

    // 원문은 방문 집합 대신 부모 하나만 제외한다. 고리/과도한 재귀는 호스트의 안전 중단(complete=false)으로 표시한다.
    #collect(root) {
        const result = { visited: [root], shortJunction: false, canDecay: false, complete: true };
        const active = [];
        // 원문 004218b0 의 각 재귀 호출마다 현재 이웃 목록을 읽는다.
        const visit = (sid, depth, parent) => {
            if (depth >= SAFE_RECURSION_DEPTH || active.includes(sid)) {
                result.complete = false;
                result.canDecay = false;
                return;
            }
            active.push(sid);
            // 다리/섬만 처리하며 부모 외의 중복은 원문처럼 목록에 남긴다.
            for (const neighbor of this.#neighborList(sid)) {
                const raw = this.#pool.slot(neighbor);
                const flags = this.#types[raw[TYPE_OFFSET]].flags2;
                const bridge = (flags & GENUS_BRIDGE) !== 0;
                const island = (flags & GENUS_ISLAND) !== 0;
                if ((!bridge && !island) || neighbor === parent) continue;
                if (bridge && result.visited.length < VISITED_CAPACITY) result.visited.push(neighbor);
                if (this.#code(neighbor).side < 0x4a && !island) {
                    if (depth === 0 && this.#neighborList(neighbor).length < 3) result.shortJunction = true;
                    visit(neighbor, depth + 1, sid);
                    if (!result.complete) break;
                } else if (depth === 0 || (depth === 1 && result.shortJunction)) {
                    result.canDecay = true;
                    result.visited = result.visited.filter((value) => value !== neighbor);
                } else if (!bridge || this.#neighborList(neighbor).length < 2) result.canDecay = true;
                else result.visited = result.visited.filter((value) => value !== neighbor);
            }
            active.pop();
        };
        visit(root, 0, 0);
        return result;
    }

    // 수명은 signed DWORD 에서 빼고 음수면 0 으로 제한한다. 제거 분기는 WORD 를 쓰지 않는다.
    applyLife(sid, reduction) {
        const hooks = this.#hooks;
        const previous = (read(this.#pool.slot(sid), WORD_OFFSET, 2) & BRIDGE_LIFE_MASK) >> 3;
        let remaining = (previous - reduction) | 0;
        if (remaining < 0) remaining = 0;
        if (!remaining && (this.#authority ? this.#authority.server : this.#mode.server)) {
            const side = this.#code(sid).side;
            need(hooks.destroy)(sid, side === 0x4a || side === 0x4b ? BRIDGE_PLANK_REMOVAL : 0);
            return;
        }
        const raw = this.#pool.physical(sid);
        write(raw, WORD_OFFSET, ((read(raw, WORD_OFFSET, 2) & ~BRIDGE_LIFE_MASK) | ((remaining * 8) & BRIDGE_LIFE_MASK)) & 0xffff, 2);
        need(hooks.notifySurface)(sid);
        if (remaining < BRIDGE_CRACK_LIFE && previous >= BRIDGE_CRACK_LIFE) {
            const current = this.#pool.slot(sid);
            if (current[TYPE_OFFSET] >= this.#frames.length) throw new RangeError('다리 수명 통지 뒤의 현재 TYPE 범위');
            const frame = bridgeCrackedFrame(this.#frames[current[TYPE_OFFSET]], read(current, FRAME_OFFSET) | 0, false);
            if (frame !== -1) {
                write(this.#pool.physical(sid), FRAME_OFFSET, frame >>> 0, 4);
                need(hooks.refresh)(sid);
            }
        } else if (this.#mode.debugRedraw) need(hooks.refresh)(sid);
        if (remaining <= BRIDGE_CARRIER_LIFE) need(hooks.carriers)(sid);
    }

    // 각 방문의 삭제가 바꾼 dead/WORD 는 다음 방문에서 다시 읽으며 전체를 사본으로 되돌리지 않는다.
    cell(root) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const graph = pool.slot(root)[GRAPH_OFFSET];
        if (graph === INVALID_GRAPH || graph >= this.#graphs.length) throw new RangeError('다리 붕괴의 무효 Graph 원문 예외');
        if (this.#graphs[graph].surfaces < BRIDGE_MINIMUM_GRAPH_SURFACES) {
            need(hooks.destroy)(root, 0);
            return;
        }
        const code = this.#code(root);
        if (code.flags & BRIDGE_HARD_FRAME) return;
        const neighbors = this.#neighborList(root);
        if (neighbors.length === 0) {
            this.applyLife(root, 1);
            return;
        }
        // 원문의 첫 10개 목록만 열린 방향 판정에 쓴다.
        const listed = neighbors.slice(0, NEIGHBOR_LIST_CAPACITY);
        const raw = pool.slot(root);
        if (bridgeOpenDirection(code.side, coordinate(raw, X_OFFSET), coordinate(raw, Y_OFFSET), this.#surface.entries(0), listed) === -1) return;
        const walk = this.#collect(root);
        if (!walk.complete) throw new Error('원문 다리 재귀 고리의 호스트 안전 중단');
        if (!walk.canDecay) return;
        let lowest = 2147483647;
        // 방문 목록의 현재 비영 수명만 최소값에 반영한다.
        for (const number of walk.visited) {
            const life = (read(pool.slot(number), WORD_OFFSET, 2) & BRIDGE_LIFE_MASK) >> 3;
            if (life && life <= lowest) lowest = life;
        }
        const none = lowest === 2147483647;
        // 진단 복귀 뒤 같은 최소값을 제한하며 실제 슬롯은 아래 각 방문에서 다시 읽는다.
        if (!none && this.#checking() && lowest > 7) need(hooks.assertion)('0 <= lowestTimeLeft && lowestTimeLeft <= brMAX_TIME_LEFT', '\\Ns\\O\\Bridge.cpp', 0x268);
        if (!none && lowest > 7) lowest = 7;
        const target = none ? 7 : lowest - 1;
        // 앞선 실제 삭제/반납의 결과를 이후 방문에서 다시 읽는다.
        for (const number of walk.visited) {
            const current = pool.slot(number);
            if (!(current[STATE_OFFSET] & 2)) this.applyLife(number, ((read(current, WORD_OFFSET, 2) & BRIDGE_LIFE_MASK) >> 3) - target);
        }
        if (target < BRIDGE_CRACK_LIFE && lowest >= BRIDGE_CRACK_LIFE) need(hooks.crackSound)(root);
    }
}

// 비유한/64비트 밖은 indefinite 의 하위 0 이다. 004e49c0 은 signed64 로 절삭한 뒤 EAX 만 소비한다. (2026-11-08 WB04-E45)
function budget(value) {
    if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0;
    return Math.trunc(value) | 0;
}

// 프레임 스캔 실행기이다. 모든 참조(state/period/mode)와 칸 처리기는 이 실행기보다 오래 살아야 한다.
export class RawBridgeDecayScan {
    #pool;
    #types;
    #state;
    #period;
    #mode;
    #cell;

    // 구성은 현재 스캔을 쓰지 않으며 원본의 최초 BSS/이전 World 반납 상태도 보존한다.
    constructor(pool, types, state, period, mode, cell) {
        if (pool?.edition !== 'patch1078' || types.length !== TYPE_COUNT || typeof cell !== 'function' || typeof period !== 'function') throw new TypeError('실제 다리 붕괴 스캔의 PATCH 풀/TYPE/칸 처리 연결 오류');
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#period = period;
        this.#mode = mode;
        this.#cell = cell;
    }

    // 기한이 지난 프레임은 range 개만 방문하며 끝 슬롯을 포함한다. 재설정은 모든 칸 처리 뒤이다. time = { game, delta }.
    frame(time, paused) {
        const state = this.#state;
        if (paused || this.#mode.editor || !this.#mode.authority) return;
        const range = (state.last - state.first) | 0;
        let remaining = range;
        // 기한을 넘겼으면 delta/주기를 읽지 않는 원문 분기이다.
        if (state.next - time.game > 0) remaining = budget((time.delta / Math.fround(this.#period())) * range);
        // 매 칸 효과가 바꾼 현재 커서/끝 번호를 다음 방문에 다시 읽는다.
        while (remaining > 0 && state.cursor <= state.last) {
            remaining -= 1;
            try {
                if (state.cursor < 0 || state.cursor >= this.#pool.capacity) throw new RangeError('다리 붕괴 스캔의 실제 물리 SID 범위');
                const sid = state.cursor;
                const raw = this.#pool.slot(sid);
                if ((raw[STATE_OFFSET] & 7) === 0) {
                    if (raw[TYPE_OFFSET] >= this.#types.length) throw new RangeError('다리 붕괴 스캔의 현재 TYPE 범위');
                    if ((this.#types[raw[TYPE_OFFSET]].flags2 & GENUS_BRIDGE) && !(raw[EXTRA_OFFSET] & 1)) this.#cell(sid);
                }
            } catch {
                // 00422c93 은 이 슬롯의 예외만 삼키고 이미 발생한 효과를 되돌리지 않는다.
            }
            state.cursor = (state.cursor + 1) | 0;
        }
        // 기한과 현재 주기도 칸 효과 뒤 다시 읽으며 밀린 옛 기한에 누적하지 않는다.
        if (time.game >= state.next) {
            const next = Math.fround(this.#period()) + time.game;
            state.cursor = state.first;
            state.next = next;
        }
    }
}
