// 원본 표면 지도·spot·프레임 접합을 사용하는 너비 우선 탐색(BFS)과 역추적 경로 축약을 옮긴 모듈이다(cpppj `o/RawPathSearch.cpp/.h`, 10.78 전용, 기준선 1f00a25a 뒤 변경 없음).
// 길 찾기는 같은 실제 SID 풀·해시 0단계(표면 지도)·spot 바이트 지도·타입/프레임 표에서만 계산하고, 표면 사본이나 소유자 정책을 따로 만들지 않는다.
//  - searchCells(output, sx, sy, dx, dy, cardinalOnly) (00488370): (sx, sy) 에서 북/동/남/서 네 방향으로 BFS 해 signed WORD 거리표(256×256)와 두 BYTE 큐(16,384항목)를 채우고,
//    (dx, dy) 에서 거리를 거슬러 올라가며 방향이 바뀌는 지점마다 꼭짓점 두 개(이전 방향·새 방향)를 모아 경로 출력의 `setPath` 로 쓴다. 출력은 역순(목적→시작)이다.
//    cardinalOnly 가 false 이면 역추적에서 거리 한 단계 건너뛰는 대각선 축약도 허용하고 여덟 방향을 모두 본다. 기존 경로 꼭짓점·방향·시각은 보존하고 뒤에 이어 붙인다.
//  - search(output, parent, start, destination, flags) (00488af0): 목적 표면의 Graph 번호가 있고 시작 표면과 같아야 하고, 시작 칸이 걸을 수 있는 칸(spot & 6)일 때만 목적→시작 방향으로 BFS 를 부른다(flags 의 비트 0 이 cardinalOnly).
//  - surfaceAt/graphAt/connects: 0.9999 보정 칸의 표면 SID, 같은 Graph 번호 비교용 조회(번호 254 는 Graph 없음), 같은 SID 는 바로 연결 / 다른 SID 는 현재 타입·물리 프레임으로 접합(`bridgeConnects`).
//  - RawPathSearchState: Path 가 직접 읽고 바꾸는 전역(Graph 허용·kenGo 활성·역추적 flag 2 회피·정책 선택값·방향 선행 허용 비트·방향 우선순위·예외 Graph 타입). 정책 선택값이 5 이면 방향 3 의 선행 비트를 5 로 바꾼다.
//  - RawPathSearchWork/RawPathSearchStorage: 원본 거리표·큐와, Client 수명 소유자(진단이 새로 계산하지 않고 읽는 59514c, 실제 거리표 쓰기에 도달한 뒤에만 `initialized`).
// 웹 범위: 10.72(CD) 판본 분기는 옮기지 않았다. `makePathSearchHooks` 는 RawPathProcess 훅 묶음이 필요하므로 경로 프로세스 이식 단위에서 옮긴다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E334: 6-P RawPathSearch 이식으로 추가)
import { PATH_LAYOUT } from './raw-path-runtime.js';
import { pathCell, rawPathSurfaceAt } from './raw-path-surface.js';
import { bridgeConnects, TYPE_FLAG1_SURFACE } from './surface-finder.js';

// 북쪽부터 시계 방향인 원본 단위 좌표이다(방향 0..7, 큐에는 두 BYTE 로 저장). (2026-11-12 WB04-E334)
const DIRECTION_X = Object.freeze([0, 1, 1, 1, 0, -1, -1, -1]);
const DIRECTION_Y = Object.freeze([-1, -1, 0, 1, 1, 1, 0, -1]);
// 지도 한 변의 칸 수, 큐 항목 수(16,384), 큐 꼬리 감김 마스크, 지도 칸 수이다. (2026-11-12 WB04-E334)
const BOARD_SIDE = 256;
const QUEUE_ENTRIES = 16384;
const QUEUE_MASK = 0x3fff;
const BOARD_CELLS = 65536;
// 슬롯 필드 위치(타입·상태 flag·Graph 번호·프레임 DWORD)이다. 10.78 슬롯 배치이다. (2026-11-12 WB04-E334)
const TYPE_AT = 10;
const FLAG_AT = 12;
const GRAPH_AT = 30;
const FRAME_AT = 36;
// Graph 가 없다는 표면 Graph 번호이다. (2026-11-12 WB04-E334)
const NO_GRAPH = 254;
// 역추적 후보 비용의 초기값(후보 없음)이다. (2026-11-12 WB04-E334)
const NO_CANDIDATE = 9999;

// 보드 칸 범위 판정이다. 범위 밖 칸은 원본 포인터 산술 밖에서 처리한다. (2026-11-12 WB04-E334)
const inside = (x, y) => x >= 0 && y >= 0 && x < BOARD_SIDE && y < BOARD_SIDE;

// signed WORD 거리의 원본 덧셈 감김이다(32767 + 1 → -32768). (2026-11-12 WB04-E334)
const increment = (value) => ((value + 1) << 16) >> 16;

// Path 가 직접 읽고 바꾸는 전역이다. 명령을 만든 부모의 owner 를 별도 이동 허가 정책으로 바꾸지 않는다.
// sharedPolicySelector 는 실제 Client 의 005318ec `{ value }` 상자이며 독립 시험은 null 로 두어 `policySelector` 를 그대로 쓴다. (2026-11-12 WB04-E334)
export class RawPathSearchState {
    // 00540414: GraphAt 조회 허용 전역이다. 번호 254 는 Graph 없음이다.
    graphsEnabled = true;
    // 00540be4: 원본 kenGo 활성 전역이다. 같은 Graph·허용 지면에서 꺼졌으면 원본 assert 경로(예외)이다.
    searchEnabled = true;
    // 00540be8: 역추적 후보 raw+12 &6 == 2 에 30 비용을 더한다.
    avoidFlagTwo = true;
    // 패치 005318ec 의 PE 초기값 6 이다. 5 면 방향 3 의 선행 비트를 5 로 바꾼다(CD 에는 이 분기가 없고 웹은 10.78 만).
    policySelector = 6;
    // 대각선은 양옆 직선 방향의 허용 비트가 필요하다(방향별 선행 허용 비트).
    directionPrerequisite = new Uint32Array([0, 5, 0, 20, 0, 80, 0, 65]);
    // 원본 방문/동률 선택 우선순위이다(앞 네 항목이 북/동/남/서).
    directionOrder = new Int32Array([0, 2, 4, 6, 1, 3, 5, 7]);
    // surface 가 아닌 예외 Graph BYTE getter 의 현재 타입 번호(울타리 표식·바람 궁수)이다.
    fenceMarkType = 90;
    windArcherType = 140;
    // 실제 Client 의 005318ec `{ value }` 상자이다. null 이면 `policySelector` 의 signed 입력을 유지한다.
    sharedPolicySelector = null;
}

// 원본 signed WORD 거리표와 두 BYTE 큐의 미사용 흔적까지 보존하는 작업 배열이다. (2026-11-12 WB04-E334)
export class RawPathSearchWork {
    // 256×256 칸의 원본 signed WORD BFS 거리표이다.
    distance = new Int16Array(BOARD_CELLS);
    // BFS 방문 순서대로 X·Y BYTE 좌표를 담는 16,384항목 큐이다(항목 i 는 [2i], [2i+1]).
    queue = new Uint8Array(QUEUE_ENTRIES * 2);
}

// 원문 59514c 의 거리/큐 소유자는 Session 보다 오래 살며 실제 BFS 도달 여부를 구별한다. 사용: new RawPathSearchStorage() 를 Client 가 한 번 만들어 검색기들에 넘긴다. (2026-11-12 WB04-E334)
export class RawPathSearchStorage {
    // 기존 실제 거리/큐 배열을 공유하며 진단에서 새로 계산하지 않는다(처음엔 null, 첫 검색기가 만든다).
    work = null;
    // 실제 searchCells 의 거리표 쓰기에 도달한 뒤에만 원문 비 NULL 로 관측한다.
    initialized = false;
}

// 같은 실제 pool/hash0/spot/현재 타입·프레임에서만 경로를 계산하는 검색기이다. 사용: new RawPathSearch(pool, hash, spots, types, frames, state).search(path, parentSid, start, destination, flags).
// types[type] 은 { flags1, flags2 }, frames[type] 은 프레임 코드 { side, variant, number, flags } 배열이다. (2026-11-12 WB04-E334)
export class RawPathSearch {
    // 객체 슬롯의 생성·조회에 사용하는 SID 풀이다.
    #pool;
    // 좌표별 객체 후보를 찾는 공간 해시이다(0단계가 표면 지도).
    #hash;
    // 칸별 점유·통행 비트를 읽는 spot 지도(65,536 바이트)이다.
    #spots;
    // 타입 번호로 속성·플래그를 조회하는 타입 표이다.
    #types;
    // 타입별 프레임 코드 표이다.
    #frames;
    // Path 가 직접 읽고 바꾸는 전역이다.
    #state;
    // 외부 장기 소유자(실제 Client)이며 독립 검사는 null 이다.
    #sharedWork;
    // 현재 거리표·큐 작업 배열이다(sharedWork 가 있으면 그 소유자와 같은 객체).
    #work;

    // 모든 참조는 검색기보다 오래 살아야 한다. hash0 의 실제 배열을 읽으며 표면 사본을 만들지 않는다. 실제 Client 만 장기 59514c 소유자(sharedWork)를 공급한다. (2026-11-12 WB04-E334)
    constructor(pool, hash, spots, types, frames, state, sharedWork = null) {
        if (spots.length !== BOARD_CELLS || types.length !== frames.length) throw new RangeError('BFS 실제 월드 연결');
        this.#pool = pool;
        this.#hash = hash;
        this.#spots = spots;
        this.#types = types;
        this.#frames = frames;
        this.#state = state;
        this.#sharedWork = sharedWork;
        // 공유 소유자가 있으면 그 작업 배열(없으면 만들어 소유자에 등록)을, 없으면 독립 배열을 쓴다.
        this.#work = sharedWork ? (sharedWork.work ?? (sharedWork.work = new RawPathSearchWork())) : new RawPathSearchWork();
    }

    // 슬롯 원본 바이트를 사본 없이 읽기 전용으로 빌린다(BFS 가 칸마다 부르므로 slice 를 피한다). 이 모듈은 쓰지 않는다. (2026-11-12 WB04-E334)
    #raw(sid) {
        return this.#pool.physical(sid);
    }

    // 0040eaf0: 좌표 + 0.9999 를 절삭한 실제 표면 SID 이다. 보드 밖은 0 이다. 사용: search.surfaceAt({ x: 9.2, y: 9.3 }). (2026-11-12 WB04-E334)
    surfaceAt(position) {
        return rawPathSurfaceAt(this.#hash, position.x, position.y);
    }

    // 00462c30: 시작/목적 표면의 같은 Graph 번호를 비교할 실제 조회이다. Graph 비허용·표면 0·번호 254 는 null 이다. 표면 타입이 아닌 예외 타입은 진단(Error)이다. (2026-11-12 WB04-E334)
    graphAt(position) {
        if (!this.#state.graphsEnabled) return null;
        const sid = this.surfaceAt(position);
        if (sid === 0) return null;
        const raw = this.#raw(sid);
        const type = raw[TYPE_AT];
        if (type >= this.#types.length || (((this.#types[type].flags1 >>> 0) & TYPE_FLAG1_SURFACE) === 0 && type !== this.#state.fenceMarkType && type !== this.#state.windArcherType)) {
            throw new Error('BFS Graph getter 표면 타입');
        }
        const graph = raw[GRAPH_AT];
        return graph === NO_GRAPH ? null : graph;
    }

    // 00441f20: 같은 SID 는 바로 연결되며 다른 SID 는 현재 타입/물리 프레임으로 접합을 계산한다. 어느 쪽이든 0 이면 false 이다. (2026-11-12 WB04-E334)
    connects(first, second, direction) {
        if (first === 0 || second === 0) return false;
        if (first === second) return true;
        return bridgeConnects(this.#frame(first), this.#types[this.#raw(first)[TYPE_AT]].flags2, this.#frame(second), this.#types[this.#raw(second)[TYPE_AT]].flags2, direction);
    }

    // 현재 물리 프레임(슬롯 +36 의 signed DWORD)을 타입별 프레임 코드로 읽는다. 범위 밖은 진단이다. (2026-11-12 WB04-E334)
    #frame(sid) {
        const raw = this.#raw(sid);
        const type = raw[TYPE_AT];
        const frame = raw[FRAME_AT] | (raw[FRAME_AT + 1] << 8) | (raw[FRAME_AT + 2] << 16) | (raw[FRAME_AT + 3] << 24);
        if (type >= this.#frames.length || frame < 0 || frame >= this.#frames[type].length) throw new RangeError('BFS 표면 프레임');
        return this.#frames[type][frame];
    }

    // 한 방향이 바뀔 때 같은 좌표의 이전/새 방향 꼭짓점 둘을 저장하는 원본 append 이다. 최대 길이(5000)나 출력 이미지 크기가 맞지 않으면 진단이다. (2026-11-12 WB04-E334)
    #append(points, directions, point, direction, output) {
        points.push(point);
        directions.push(direction);
        if (points.length >= PATH_LAYOUT.capacity || output.bytes.length !== PATH_LAYOUT.size) throw new RangeError('BFS 최대 경로 길이');
    }

    // 00488370: (sx, sy) 에서 BFS 한 뒤 (dx, dy) 에서 역추적하고 실제 방향 변경점을 저장한다. 미사용 큐 바이트는 다시 초기화하지 않고 거리표만 매 호출에서 전부 지운다.
    // 도달하지 못하면(큐 고갈·표면 0) false 이다. 범위 밖 시작/끝 칸은 RangeError 이다. (2026-11-12 WB04-E334)
    searchCells(output, sx, sy, dx, dy, cardinalOnly) {
        if (!inside(sx, sy) || !inside(dx, dy)) throw new RangeError('BFS 시작/끝 칸');
        // 진단의 원문 59514c 비 NULL 은 실제 거리표 초기화에 도달한 뒤에만 관측한다.
        if (this.#sharedWork) this.#sharedWork.initialized = true;
        const work = this.#work;
        const state = this.#state;
        work.distance.fill(0);
        // 정책 선택값이 5 이면 방향 3 의 선행 비트를 5 로 바꾼다(공유 상자가 있으면 그 현재 값을 unsigned 로, 없으면 signed 입력을 비교).
        if ((state.sharedPolicySelector ? (state.sharedPolicySelector.value >>> 0) : (state.policySelector | 0)) === 5) state.directionPrerequisite[3] = 5;
        work.queue[0] = sx;
        work.queue[1] = sy;
        work.distance[sy * BOARD_SIDE + sx] = 1;
        let tail = 1;
        let head = 0;
        let layerEnd = 1;
        let distance = 2;
        // 원본은 cardinal 네 방향만 BFS 확장하며 대각선은 역추적 축약에서 선택한다.
        for (;;) {
            if (head >= QUEUE_ENTRIES) throw new RangeError('BFS 원본 큐 읽기 넘침');
            if (head === layerEnd) {
                distance = increment(distance);
                layerEnd = tail;
            }
            const x = work.queue[head * 2];
            const y = work.queue[head * 2 + 1];
            const current = this.#hash.cell(0, x, y);
            if (current === 0) return false;
            if (x === dx && y === dy) break;
            let admitted = 0;
            // 원본 우선순위의 앞 네 항목은 북/동/남/서이며 이미 허용된 방향 비트가 선행 비트를 채워야 한다.
            for (let i = 0; i < 4; i += 1) {
                const direction = state.directionOrder[i];
                if (direction < 0 || direction > 7) throw new RangeError('BFS 방향 전역');
                const required = state.directionPrerequisite[direction];
                if ((admitted & required) !== required) continue;
                const nx = x + DIRECTION_X[direction];
                const ny = y + DIRECTION_Y[direction];
                if (!inside(nx, ny)) continue;
                const index = ny * BOARD_SIDE + nx;
                const spot = this.#spots[index];
                if (work.distance[index] !== 0 || (spot & 6) === 0 || (spot & 8) !== 0) continue;
                const next = this.#hash.cell(0, nx, ny);
                if (next === 0) throw new Error('BFS 허용 지면의 표면0');
                if (!this.connects(current, next, direction)) continue;
                work.queue[tail * 2] = nx;
                work.queue[tail * 2 + 1] = ny;
                work.distance[index] = distance;
                tail = (tail + 1) & QUEUE_MASK;
                admitted |= 1 << direction;
            }
            head += 1;
            if (head === tail) return false;
        }
        const points = [];
        const directions = [];
        const times = [];
        // append 전 기존 경로와 시각을 보존한다. 원본 search 본체는 nVert 를 자동 0 으로 만들지 않는다.
        for (let i = 0; i < output.count; i += 1) {
            points.push(output.point(i));
            directions.push(output.direction(i));
            times.push(output.time(i));
        }
        let x = dx;
        let y = dy;
        let remaining = work.distance[y * BOARD_SIDE + x] - 1;
        let previousDirection = -1;
        if (remaining === 0) this.#append(points, directions, { x, y }, 0, output);
        // 짧은 거리 후보/현재 방향/대각선/raw flag 비용을 실제 방문 순서로 비교한다.
        while (remaining > 0) {
            const current = this.#hash.cell(0, x, y);
            let alternate = remaining;
            if (!cardinalOnly && remaining > 1) alternate -= 1;
            let bestCost = NO_CANDIDATE;
            let bestDirection = 0;
            let admitted = 0;
            // cardinalOnly 는 앞 네 항목, 일반 사제는 여덟 방향으로 역추적한다.
            for (let i = 0; i < (cardinalOnly ? 4 : 8); i += 1) {
                const direction = state.directionOrder[i];
                if (direction < 0 || direction > 7) throw new RangeError('BFS 역추적 방향');
                const nx = x + DIRECTION_X[direction];
                const ny = y + DIRECTION_Y[direction];
                if (!inside(nx, ny)) continue;
                const value = work.distance[ny * BOARD_SIDE + nx];
                if (value !== remaining && value !== alternate) continue;
                const next = this.#hash.cell(0, nx, ny);
                if (next === 0) throw new Error('BFS 역추적 표면0');
                const required = state.directionPrerequisite[direction];
                if ((admitted & required) !== required || !this.connects(current, next, direction)) continue;
                let cost = direction === previousDirection ? 10 : 15;
                if ((direction & 1) !== 0) cost -= 10;
                if (state.avoidFlagTwo && (this.#raw(next)[FLAG_AT] & 6) === 2) cost += 30;
                if (cost < bestCost) {
                    bestCost = cost;
                    bestDirection = direction;
                }
                admitted |= 1 << direction;
            }
            if (bestCost === NO_CANDIDATE) throw new Error('BFS 역추적 후보 없음');
            if (bestDirection !== previousDirection) {
                if (previousDirection !== -1) this.#append(points, directions, { x, y }, previousDirection, output);
                this.#append(points, directions, { x, y }, bestDirection, output);
                previousDirection = bestDirection;
            }
            x += DIRECTION_X[bestDirection];
            y += DIRECTION_Y[bestDirection];
            remaining = work.distance[y * BOARD_SIDE + x] - 1;
        }
        if (previousDirection !== -1) this.#append(points, directions, { x, y }, previousDirection, output);
        if (x !== sx || y !== sy) throw new Error('BFS 역추적 시작 불일치');
        // 새 꼭짓점의 시각은 0 으로 채우고(원본 resize) 전체를 출력에 쓴다.
        while (times.length < points.length) times.push(0);
        output.setPath(points, directions, times);
        return true;
    }

    // 00488af0: 현재 Graph/목적 지면을 검사하고 목적 → 시작 BFS 를 불러 최종 start → dest 배열을 만든다. 목적 표면 Graph 가 없거나 시작과 다르면, 칸이 지도 밖이거나 시작 칸이 걷지 못하면 false 이다.
    // kenGo 비활성은 진단(Error)이다. 소유자 정책은 더하지 않는다. (2026-11-12 WB04-E334)
    search(output, parent, start, destination, flags) {
        const parentType = this.#raw(parent)[TYPE_AT];
        if (parentType >= this.#types.length) throw new RangeError('BFS 부모 타입');
        const destinationGraph = this.graphAt(destination);
        if (destinationGraph === null || destinationGraph !== this.graphAt(start)) return false;
        const sx = pathCell(destination.x);
        const sy = pathCell(destination.y);
        const dx = pathCell(start.x);
        const dy = pathCell(start.y);
        if (!inside(sx, sy) || !inside(dx, dy) || (this.#spots[dy * BOARD_SIDE + dx] & 6) === 0) return false;
        if (!this.#state.searchEnabled) throw new Error('원본 kenGo 비활성 진단');
        return this.searchCells(output, sx, sy, dx, dy, (flags & 1) !== 0);
    }

    // 원본 전체 거리표/큐를 독립 native 결과와 대조할 수 있게 작업 배열을 그대로 돌려준다. (2026-11-12 WB04-E334)
    get work() { return this.#work; }

    // 실제 PathProcess/월드 연결의 풀 일치를 검사할 수 있게 같은 풀을 돌려준다. (2026-11-12 WB04-E334)
    get pool() { return this.#pool; }
}
