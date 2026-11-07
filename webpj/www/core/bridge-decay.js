// 다리 붕괴의 순수 논리(열린 방향·방문 목록 수집·수명 감소·금 간/보통 프레임 전환·destroy 재정의 판정·한 칸 처리·스캔 커서와 주기)를 옮긴 모듈이다(cpppj `o/Bridge.cpp/.h` 의 Bridge 붕괴 계열과 BridgeDecayScan, 10.78).
// 원본 00421770/004217f0/004218b0/00421b60/00421bd0/00421c30/00421db0/00421e60/004220f0/004227e0/00422490/00422bc0 에 대응한다. 외부 효과(실제 삭제·그리기·소리·낙하)는 실행하지 않고 사건 목록으로 알린다.
//  - 프레임 표는 코드 배열 [{ side, variant, number, flags }](number 는 signed char 로 읽음)이고 side/variant 는 문자 코드이다. 표면 탐색기는 surface-finder.js 의 SurfaceFinder(object/neighbors/map)이다.
//  - 상태: { id, word(+0xc, 수명은 0x78 비트), frame, graphSurfaces, graphValid(그래프 번호 254 면 false), extra(abstract 1·buried 8), dead }.
//  - 모드: { server, authority, editor, debugRedraw, debugKeep } (원본 00540bc0/00540bc4/005c85a4/005453ac/0054db80).
//  - 사건: { kind: 'Destroy' | 'Redraw' | 'Carriers' | 'CrackSound', id, flags }, 결과 outcome 은 BRIDGE_DECAY_OUTCOME 의 이름이다.
// 이 모듈은 DOM 을 쓰지 않는다. CD 10.72 분기(수명 함수 인라인·프레임 1바이트)는 옮기지 않았고 스캔 범위 초기화만 두 판본 상수를 가진다. (2026-11-08 WB04-E44: Bridge 붕괴 논리 이식으로 추가)
import { WORLD_CELLS, connectionMask } from '../assets/terrain-mask.js';

// 다리 수명 비트 마스크(+0xc 의 3~6비트)와 판자 제거 플래그, 금 간/이동체 확인 수명, 한 칸 처리가 곧바로 없애는 그래프 표면 수 미만, 정상 수명 최댓값이다. (2026-11-08 WB04-E44)
export const BRIDGE_LIFE_MASK = 0x78;
export const BRIDGE_PLANK_REMOVAL = 0x02000000;
export const BRIDGE_CRACK_LIFE = 5;
export const BRIDGE_CARRIER_LIFE = 5;
export const BRIDGE_MINIMUM_GRAPH_SURFACES = 5;
const MAXIMUM_LIFE = 7;

// 프레임 코드 플래그: 금 간 프레임(0x20)과 단단한 프레임(0x40)이다. (2026-11-08 WB04-E44)
export const BRIDGE_CRACKED_FRAME = 0x20;
export const BRIDGE_HARD_FRAME = 0x40;

// 타입 genus(flags2) 비트: island(2)·bridge(4) 이다. 다리 프레임 검색의 변형 글자는 'P'(0x50)이다. (2026-11-08 WB04-E44)
const GENUS_ISLAND = 2;
const GENUS_BRIDGE = 4;
const BRIDGE_VARIANT = 0x50;

// 금 간 프레임은 보통 번호에 10 을 더한 번호이고 단단한 프레임은 20 이상이다. (2026-11-08 WB04-E44)
const CRACKED_NUMBER_OFFSET = 10;
const HARD_NUMBER = 20;

// 방향(0 북부터 시계 방향 8개)별 연결 비트와 한 칸 이웃 이동량이다(0052f8dc). (2026-11-08 WB04-E44)
const DIRECTION_BITS = Object.freeze([1, 2, 2, 4, 4, 8, 8, 1]);
const NEIGHBOR_CELLS = Object.freeze([[0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]]);

// 00500edc 의 float 비트 3f7ff972(0.9998999834060669)이다. 표면 조회의 거의 올림인 칸 변환이다. (2026-11-08 WB04-E44)
const SURFACE_COORDINATE_BIAS = Math.fround(0.9999);

// 원본의 고리 재귀에는 방문 제한이 없다. 새 코드에서 스택 고갈을 막는 한계이며 원본 상수가 아니다. 방문 목록의 최대 길이(0x64)와 이웃 목록 용량(10)도 원본 값이다. (2026-11-08 WB04-E44)
const SAFE_RECURSION_DEPTH = 256;
const VISITED_CAPACITY = 100;
const NEIGHBOR_LIST_CAPACITY = 10;

// 한 칸 처리의 결과 종류이다. (2026-11-08 WB04-E44)
export const BRIDGE_DECAY_OUTCOME = Object.freeze({
    invalidGraph: 'InvalidGraph', smallGraph: 'SmallGraph', hard: 'Hard', isolated: 'Isolated', closed: 'Closed',
    blocked: 'Blocked', decayed: 'Decayed', incomplete: 'Incomplete', faulted: 'Faulted',
});

// 기본 모드: 싱글 플레이(로컬이 서버) 전투 화면이다. (2026-11-08 WB04-E44)
export const DEFAULT_BRIDGE_DECAY_MODE = Object.freeze({ server: true, authority: true, editor: false, debugRedraw: false, debugKeep: false });

// 0040eaf0: 좌표에 0.9999f 를 더하고 _ftol 한 칸이다. x87 중간값처럼 float 로 다시 좁히지 않는다. 범위 밖/비유한은 RangeError 이다. (2026-11-08 WB04-E44)
function cellCoordinate(value) {
    const shifted = value + SURFACE_COORDINATE_BIAS;
    if (!Number.isFinite(shifted) || shifted < -2147483648 || shifted > 2147483647) throw new RangeError('Bridge coordinate');
    return Math.trunc(shifted);
}

// 원본 CRT _ftol(004e49c0): 0 방향 절삭한 64비트 정수의 하위 32비트이다. 범위 밖·NaN 은 0 이다. (2026-11-08 WB04-E44)
function ftol(value) {
    if (!(value > -9223372036854775808 && value < 9223372036854775808)) return 0;
    return Math.trunc(value) | 0;
}

// 연결 마스크가 있는 방향에서 이웃 목록에 없는 표면만 열린 방향이다. 지도 밖 또는 번호 0 은 열린 것으로 남는다. 원본 00421770. (2026-11-08 WB04-E44)
export function bridgeIsOpen(side, direction, x, y, surface, neighbors) {
    if (direction < 0 || direction >= 8 || surface.length !== WORLD_CELLS * WORLD_CELLS || side < 0x41 || side > 0x50) throw new RangeError('Bridge direction/map/side');
    if ((connectionMask(String.fromCharCode(side)) & DIRECTION_BITS[direction]) === 0) return false;
    const cellX = cellCoordinate(Math.fround(x + NEIGHBOR_CELLS[direction][0]));
    const cellY = cellCoordinate(Math.fround(y + NEIGHBOR_CELLS[direction][1]));
    if (cellX < 0 || cellY < 0 || cellX >= WORLD_CELLS || cellY >= WORLD_CELLS) return true;
    const id = surface[cellY * WORLD_CELLS + cellX];
    return id === 0 || !neighbors.includes(id);
}

// 방향을 북부터 시계 방향인 0,2,4,6 순서로 검사한다: 세 갈래(B~E)는 두 번째 열린 방향, 모퉁이/판자(F~K)는 첫 열린 방향, 끝 칸 L·M·N·O 는 연결 반대 방향이다. 원본 004217f0. (2026-11-08 WB04-E44)
export function bridgeOpenDirection(side, x, y, surface, neighbors) {
    if (surface.length !== WORLD_CELLS * WORLD_CELLS || side < 0x41 || side > 0x50) throw new RangeError('Bridge map/side');
    const required = side >= 0x42 && side <= 0x45 ? 2 : side >= 0x46 && side <= 0x4b ? 1 : 0;
    if (required !== 0) {
        let found = 0;
        // 원본은 두 번째/첫 번째 열린 방향을 만난 즉시 반환한다.
        for (let direction = 0; direction < 8; direction += 2) {
            if (bridgeIsOpen(side, direction, x, y, surface, neighbors)) {
                found += 1;
                if (found === required) return direction;
            }
        }
    }
    if (side === 0x4c) return 0;
    if (side === 0x4d) return 2;
    if (side === 0x4e) return 4;
    if (side === 0x4f) return 6;
    return -1;
}

// 배열에서 값과 같은 항목을 모두 지운다(std::erase). (2026-11-08 WB04-E44)
function eraseAll(list, value) {
    let write = 0;
    // 지우지 않을 항목을 앞으로 모은다.
    for (let read = 0; read < list.length; read += 1) if (list[read] !== value) list[write++] = list[read];
    list.length = write;
}

// 004218b0: 재귀 방문 목록이다. 부모만 제외하며 경계에서는 같은 번호를 모두 지운다. 접합 고리/과도한 깊이는 complete=false 로 알린다.
// 반환 { visited, shortJunction, canDecay, complete }. 사용: bridgeCollectDecay(surfaceFinder, rootId). (2026-11-08 WB04-E44)
export function bridgeCollectDecay(surfaces, root, capacity = VISITED_CAPACITY) {
    const start = surfaces.object(root);
    if (start.dead || (start.flags2 & GENUS_BRIDGE) === 0 || capacity === 0 || capacity > VISITED_CAPACITY) throw new RangeError('Bridge decay root/capacity');
    const result = { visited: [root], shortJunction: false, canDecay: false, complete: true };
    const active = [];
    // 단단한 그림 여부는 이 함수가 거르지 않는다. 원본 호출자가 구동자와 프레임을 먼저 검사한다.
    const visit = (id, depth, parent) => {
        if (depth >= SAFE_RECURSION_DEPTH || active.includes(id)) {
            result.complete = false;
            result.canDecay = false;
            return;
        }
        active.push(id);
        // 탐색기 순서와 중복 목록 추가/전체 삭제를 그대로 보존한다.
        for (const neighbor of surfaces.neighbors(id)) {
            const object = surfaces.object(neighbor);
            const bridge = (object.flags2 & GENUS_BRIDGE) !== 0;
            const island = (object.flags2 & GENUS_ISLAND) !== 0;
            if ((!bridge && !island) || neighbor === parent) continue;
            if (bridge && result.visited.length < capacity) result.visited.push(neighbor);
            if (object.frame.side < 0x4a && !island) {
                if (depth === 0 && surfaces.neighbors(neighbor).length < 3) result.shortJunction = true;
                visit(neighbor, depth + 1, id);
                if (!result.complete) break;
            } else if (depth === 0 || (depth === 1 && result.shortJunction)) {
                result.canDecay = true;
                eraseAll(result.visited, neighbor);
            } else {
                const count = bridge ? surfaces.neighbors(neighbor).length : 0;
                if (count < 2) result.canDecay = true;
                else eraseAll(result.visited, neighbor);
            }
        }
        active.pop();
    };
    visit(root, 0, 0);
    return result;
}

// 00421c30 접두: 삭제가 예약된 분기는 상태를 저장하지 않는다. 나머지는 수명 비트만 바꾸고 뒤따르는 효과(금 간 전환·이동체 확인)의 조건을 돌려준다. 정상 범위 0~7 밖은 거부한다. (2026-11-08 WB04-E44)
export function bridgeReduceLife(word, reduction, server, side) {
    const previous = (word & BRIDGE_LIFE_MASK) >> 3;
    const remaining = Math.max(0, previous - reduction);
    if (previous > MAXIMUM_LIFE || remaining > MAXIMUM_LIFE || side < 0x41 || side > 0x50) throw new RangeError('Bridge lifetime/side');
    const remove = server && remaining === 0;
    const next = remove ? word : ((word & ~BRIDGE_LIFE_MASK) | (remaining << 3)) & 0xffff;
    return {
        word: next, previous, remaining, remove,
        removalFlags: remove && (side === 0x4a || side === 0x4b) ? BRIDGE_PLANK_REMOVAL : 0,
        // 금은 5 이상에서 5 미만으로 내려갈 때만, 이동체 확인은 새 수명이 5 이하일 때마다 한다.
        crack: !remove && remaining < BRIDGE_CRACK_LIFE && previous >= BRIDGE_CRACK_LIFE,
        carriers: !remove && remaining <= BRIDGE_CARRIER_LIFE,
    };
}

// 원본 프레임 번호는 signed char 로 읽는다(movsx). 프레임이 표 밖이면 거부한다. (2026-11-08 WB04-E44)
function frameNumber(codes, frame) {
    if (frame < 0 || frame >= codes.length) throw new RangeError('Bridge frame');
    return (codes[frame].number << 24) >> 24;
}

// 0049a9a0: 방향·변형·번호의 첫 일치 프레임이다(없으면 -1). (2026-11-08 WB04-E44)
function findNumber(codes, side, variant, number) {
    // 첫 일치를 반환하므로 중복 프레임의 원본 순서가 유지된다.
    for (let index = 0; index < codes.length; index += 1) {
        const code = codes[index];
        if (code.side === side && code.variant === variant && code.number === number) return index;
    }
    return -1;
}

// 0049a9e0: 플래그를 signed 8비트에서 32비트로 부호 확장해 마스크와 겹치는 첫 프레임이다. (2026-11-08 WB04-E44)
function findMasked(codes, side, variant, number, mask) {
    // 방향·변형·번호를 확인한 뒤 마스크를 검사한다.
    for (let index = 0; index < codes.length; index += 1) {
        const code = codes[index];
        if (code.side === side && code.variant === variant && code.number === number && (((code.flags << 24) >> 24) & mask) !== 0) return index;
    }
    return -1;
}

// 00421b60: 같은 방향·변형 P·번호+10·금 간 플래그의 첫 프레임이다. 없거나 이미 금 간/단단하면 -1. force 이면 단단한 프레임(번호 20 이상)을 번호 1 로 본다(약화 규칙). (2026-11-08 WB04-E44)
export function bridgeCrackedFrame(codes, frame, force) {
    let number = frameNumber(codes, frame);
    if (force && number >= HARD_NUMBER) number = 1;
    else if (number >= CRACKED_NUMBER_OFFSET) return -1;
    return findMasked(codes, codes[frame].side, BRIDGE_VARIANT, (number + CRACKED_NUMBER_OFFSET) & 0xff, BRIDGE_CRACKED_FRAME);
}

// 00421bd0: 단단한 번호는 1 로, 금 간 번호는 10 을 뺀 번호로 같은 방향·변형 P 의 첫 프레임을 찾는다. 없으면 -1. (2026-11-08 WB04-E44)
export function bridgeNormalFrame(codes, frame) {
    let number = frameNumber(codes, frame);
    if (number >= HARD_NUMBER) number = 1;
    else if (number >= CRACKED_NUMBER_OFFSET) number -= CRACKED_NUMBER_OFFSET;
    return findNumber(codes, codes[frame].side, BRIDGE_VARIANT, number & 0xff);
}

// 00421db0: 수명을 금 간 기준 − 1(4)로 쓴 뒤에 금 간 프레임을 force 규칙으로 찾는다. 프레임을 못 찾아도 수명은 이미 바뀌어 있다. 반환 { word, frame, changed }. (2026-11-08 WB04-E44)
export function bridgeWeaken(codes, word, frame) {
    const next = ((word & ~BRIDGE_LIFE_MASK) | ((BRIDGE_CRACK_LIFE - 1) << 3)) & 0xffff;
    const found = bridgeCrackedFrame(codes, frame, true);
    return { word: next, frame: found === -1 ? frame : found, changed: found !== -1 };
}

// 00421e60: 수명 비트를 지운 뒤 보통 프레임을 찾는다. 반환 { word, frame, changed }. (2026-11-08 WB04-E44)
export function bridgeRestore(codes, word, frame) {
    const next = word & ~BRIDGE_LIFE_MASK & 0xffff;
    const found = bridgeNormalFrame(codes, frame);
    return { word: next, frame: found === -1 ? frame : found, changed: found !== -1 };
}

// 004220f0 ↔ 다리 vtable +0x10: 기본 destroy 로 진행하면 참이다. 편집기가 아니고 권한이 있고 abstract/buried 가 아니고 그래프 표면 수가 5 이상일 때, 단단한 프레임이나 디버그 유지는 destroy 를 막는다. (2026-11-08 WB04-E44)
export function bridgeDestroyProceeds(mode, extra, graphSurfaces, frameFlags) {
    if (!mode.editor && mode.authority && (extra & 9) === 0 && graphSurfaces >= BRIDGE_MINIMUM_GRAPH_SURFACES) {
        if ((frameFlags & BRIDGE_HARD_FRAME) !== 0 || mode.debugKeep) return false;
    }
    return true;
}

// 다리 destroy 재정의: 막히지 않으면 기본 destroy 가 먼저 dead 비트를 켠다. 그래프 254 인 칸은 재정의가 NULL 레코드를 읽는 조건에서 거짓(원본 예외 지점)을 돌려준다. (2026-11-08 WB04-E44)
function destroyBridge(codes, state, flags, mode, events) {
    frameNumber(codes, state.frame);
    const frameFlags = codes[state.frame].flags;
    if (!mode.editor && mode.authority && (state.extra & 9) === 0 && !state.graphValid) return false;
    if (!bridgeDestroyProceeds(mode, state.extra, state.graphValid ? state.graphSurfaces : 0, frameFlags)) return true;
    state.dead = true;
    events.push({ kind: 'Destroy', id: state.id, flags });
    return true;
}

// 00421c30 전체를 객체 하나에 적용한다: 제거(destroy 재정의 포함) 또는 수명 비트 갱신 → 금 간 프레임 전환/디버그 갱신 → 이동체 확인. 효과는 events 끝에 원본 순서로 붙인다.
// 반환값이 거짓이면 그래프 254 인 칸의 destroy 에서 원본 예외 지점에 닿은 것이다(상태는 그대로이다). 사용: bridgeApplyLife(codes, state, 1, mode, events). (2026-11-08 WB04-E44)
export function bridgeApplyLife(codes, state, reduction, mode, events) {
    frameNumber(codes, state.frame);
    const change = bridgeReduceLife(state.word, reduction, mode.server, codes[state.frame].side);
    if (change.remove) return destroyBridge(codes, state, change.removalFlags, mode, events);
    state.word = change.word;
    if (change.crack) {
        const cracked = bridgeCrackedFrame(codes, state.frame, false);
        if (cracked !== -1) {
            state.frame = cracked;
            events.push({ kind: 'Redraw', id: state.id, flags: 0 });
        }
    } else if (mode.debugRedraw) {
        events.push({ kind: 'Redraw', id: state.id, flags: 0 });
    }
    if (change.carriers) events.push({ kind: 'Carriers', id: state.id, flags: 0 });
    return true;
}

// 004227e0: 스캔이 다리 한 칸에 하는 처리를 원본 호출 순서로 따른다. surfaces 는 처리 시작 시점의 표면 스냅샷, states 는 방문될 수 있는 모든 다리의 가변 상태(시작 칸 포함)이다.
// 실제 삭제는 하지 않고 기본 destroy 로 넘어간 칸은 dead 로 표시해 사건으로 알린다. 반환 { outcome, lowest, target, events, states }(states 는 사본). (2026-11-08 WB04-E44)
export function bridgeDecayCell(surfaces, codes, states, root, modeInput = {}) {
    const mode = { ...DEFAULT_BRIDGE_DECAY_MODE, ...modeInput };
    const result = { outcome: BRIDGE_DECAY_OUTCOME.invalidGraph, lowest: 0, target: 0, events: [], states: states.map((state) => ({ ...state })) };
    // 번호로 상태를 찾는다. 없는 번호는 호출자의 입력 누락이다.
    const find = (id) => {
        const found = result.states.find((state) => state.id === id);
        if (!found) throw new RangeError('Missing bridge decay state');
        return found;
    };
    const object = surfaces.object(root);
    if ((object.flags2 & GENUS_BRIDGE) === 0) throw new TypeError('Bridge decay root is not a bridge');
    // 프레임 번호는 표 안에 있어야 한다.
    const code = (state) => {
        if (state.frame < 0 || state.frame >= codes.length) throw new RangeError('Bridge frame');
        return codes[state.frame];
    };
    let faulted = false;
    // 시작 칸의 destroy(0)와 방문 칸의 수명 감소는 같은 보조 함수를 쓴다.
    const destroy = (state, flags) => { faulted = !destroyBridge(codes, state, flags, mode, result.events); };
    const reduce = (state, reduction) => { faulted = !bridgeApplyLife(codes, state, reduction, mode, result.events); };
    const self = find(root);
    if (!self.graphValid) return result;
    if (self.graphSurfaces < BRIDGE_MINIMUM_GRAPH_SURFACES) {
        destroy(self, 0);
        result.outcome = faulted ? BRIDGE_DECAY_OUTCOME.faulted : BRIDGE_DECAY_OUTCOME.smallGraph;
        return result;
    }
    if ((code(self).flags & BRIDGE_HARD_FRAME) !== 0) { result.outcome = BRIDGE_DECAY_OUTCOME.hard; return result; }
    // 004b2660: 다리의 flag 8 이웃. 목록에는 앞 10개만 들어가고 개수는 전체를 센다.
    const neighbors = surfaces.neighbors(root);
    if (neighbors.length === 0) {
        reduce(self, 1);
        result.outcome = faulted ? BRIDGE_DECAY_OUTCOME.faulted : BRIDGE_DECAY_OUTCOME.isolated;
        return result;
    }
    // 열린 방향 검사는 이 목록에 있는 번호만 이어진 것으로 본다.
    const listed = neighbors.slice(0, NEIGHBOR_LIST_CAPACITY);
    if (bridgeOpenDirection(code(self).side, object.x, object.y, surfaces.map(), listed) === -1) {
        result.outcome = BRIDGE_DECAY_OUTCOME.closed;
        return result;
    }
    const walk = bridgeCollectDecay(surfaces, root, VISITED_CAPACITY);
    if (!walk.complete) { result.outcome = BRIDGE_DECAY_OUTCOME.incomplete; return result; }
    if (!walk.canDecay) { result.outcome = BRIDGE_DECAY_OUTCOME.blocked; return result; }
    // 0 이 아닌 수명의 최솟값이다. 원본은 같은 값도 다시 대입한다(결과는 같다).
    let lowest = 2147483647;
    // 방문 목록의 수명 비트를 모두 본다.
    for (const id of walk.visited) {
        const life = (find(id).word & BRIDGE_LIFE_MASK) >> 3;
        if (life !== 0 && life <= lowest) lowest = life;
    }
    const none = lowest === 2147483647;
    const clamped = none ? 0 : Math.min(lowest, MAXIMUM_LIFE);
    const target = none ? MAXIMUM_LIFE : clamped - 1;
    result.lowest = clamped;
    result.target = target;
    // 방문 순서대로, 그 시점에 dead 가 아닌 칸만 목표 수명으로 맞춘다. 중복 항목은 두 번째부터 감소량이 0 이다.
    for (const id of walk.visited) {
        const state = find(id);
        if (state.dead) continue;
        reduce(state, ((state.word & BRIDGE_LIFE_MASK) >> 3) - target);
        if (faulted) { result.outcome = BRIDGE_DECAY_OUTCOME.faulted; return result; }
    }
    if (!none && target < BRIDGE_CRACK_LIFE && clamped >= BRIDGE_CRACK_LIFE) result.events.push({ kind: 'CrackSound', id: root, flags: 0 });
    result.outcome = BRIDGE_DECAY_OUTCOME.decayed;
    return result;
}

// 스캔 주기(초, 0052f968)이다. 범위는 서버 SID 영역 + 예측 머리이다(10.78 15000..23001, CD 6000..14000). (2026-11-08 WB04-E44)
export const BRIDGE_DECAY_PERIOD = 10;

// 00422490: 스캔 상태 { first, last, cursor, next } 를 초기화한다. patch 가 거짓이면 CD 범위이다. (2026-11-08 WB04-E44)
export function bridgeDecayScanReset(patch, now) {
    const first = patch ? 15000 : 6000;
    const last = patch ? 23001 : 14000;
    return { first, last, cursor: first, next: BRIDGE_DECAY_PERIOD + now };
}

// 00422bc0 의 바깥 루프: 정지·편집기·권한 없음이면 아무것도 하지 않는다. 남은 시간이 양수면 trunc(delta / 주기 × 범위)개, 아니면 범위 전체만큼 커서를 전진시키고(끝 번호까지)
// 주기가 지났으면 커서를 처음으로 되돌리고 다음 시각을 now + 주기로 잡는다(이전 예정 시각 기준이 아니다). 반환 { begin, end }. (2026-11-08 WB04-E44)
export function bridgeDecayScanAdvance(state, now, delta, paused, modeInput = {}) {
    const mode = { ...DEFAULT_BRIDGE_DECAY_MODE, ...modeInput };
    if (paused || mode.editor || !mode.authority) return { begin: state.cursor, end: state.cursor };
    const range = (state.last - state.first) | 0;
    // 남은 시간이 0 보다 클 때만 비례 개수를 쓴다. 0 이하이거나 NaN 이면 범위 전체이다.
    const count = state.next - now > 0 ? ftol((delta / BRIDGE_DECAY_PERIOD) * range) : range;
    const begin = state.cursor;
    let end = begin;
    // 개수가 남아 있고 커서가 끝 번호 이하인 동안 한 번호씩 전진한다.
    if (count > 0 && begin <= state.last) {
        const available = state.last - begin + 1;
        end = begin + Math.min(count, available);
    }
    state.cursor = end;
    if (now >= state.next) {
        state.cursor = state.first;
        state.next = BRIDGE_DECAY_PERIOD + now;
    }
    return { begin, end };
}

// 스캔이 한 칸 처리를 부르는 조건: state 의 free(1)·dead(2)·void(4)가 모두 꺼져 있고 genus 에 다리 비트가 있으며 extra 의 abstract(1)가 꺼져 있어야 한다. (2026-11-08 WB04-E44)
export function bridgeDecayScanEligible(state, flags2, extra) {
    return (state & 7) === 0 && (flags2 & GENUS_BRIDGE) !== 0 && (extra & 1) === 0;
}
