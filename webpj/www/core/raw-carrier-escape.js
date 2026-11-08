// 운반체(Carrier)의 네 방향 회피와 주변 surface 후보 경로 시간 비교(429010)를 실제 월드에 연결하는 모듈이다(cpppj `o/RawCarrierEscape.cpp/.h`, 10.78 전용).
// 탐색 실패를 성공 경로로 바꾸지 않고 원본 1001 반환과 깊이·시각 저장 순서를 보존한다. 결과 선택은 이 모듈이 수행하고, 현재 경로 조회·난수·새 경로 시작·후보 운반체 조회·임시 BFS·이동 속도·좌표 유효성·진단만 훅이다.
//  - run(parent, distance, event): 현재 이동 회피 깊이(path 가 있으면 +1)가 3 을 넘거나 거리 1 인 비행체(TYPE flags2 0x20000)이면 1000 이다(깊이 진단은 검사 전역이 켜졌을 때 0x87b[음수]·0x87c[3 초과]).
//    event 0x1da1a 이면 주변 surface 후보 탐색, 아니면 event/2 번 방향(event 가 -1 이면 난수(0,5000)/1000)부터 4 번 회전하며 방향표(첫 난수 방향 4 의 중복 서쪽도 원본 그대로)로 `방향×거리+출발점` 목적지를 만들고 spot bit4(0x10)로 막히지 않으며
//    새 경로 시작이 성공하면 그 경로에 깊이를 써서 1001 을 돌려준다. 어떤 경우든 실패 뒤에도 1001 이다(다음 방향 시도는 이전 경로가 지워진 뒤에도 반환된 새 경로 이외에는 접근하지 않는다).
//    주변 탐색은 출발점 정수 중심(41d7a0 snap 뒤 다시 ftol)의 ±5 영역을 실제 일반 finder 로 훑어 surface 후보(flags1 0x800) 중 막히지 않고 운반체가 없는 후보마다 임시 BFS(cardinal TYPE 이면 flags 1)를 돌려
//    경로가 있으면 현재 이동 속도로 시각을 준비(Prepare)해 `마지막 시각 − 현재 시각`이 지금까지의 최소(초기 10000)보다 작으면 선택한다. 선택점이 유효하면 새 경로 시작(깊이 기록)을 하고 1001 이다.
//  - Blocked: 좌표에 0.9999 를 더해 ftol(signed64 하위 DWORD)한 두 정수로 감기는 선형 spot 주소(y×256+x)의 bit4 이며 보드로 clamp 하지 않고 65536 밖은 오류이다.
// state = CarrierEscapeState({ directions[17] }), now 는 현재 시각 getter 또는 `{ value }`, checking 은 현재 5e4794 진단 DWORD getter(비영이면 켜짐), cardinalTypes 는 mutable 488fb0 타입 4개, hooks = { path(sid)→RawPathRuntime|null, random(lo, hi),
// start(sid, point)→RawPathRuntime|null, blocker(point)→sid, search(path, sid, source, destination, flags), movement(sid)→[speed, turning], valid(point), assertion(line), scan?(area) }. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E101: 운반체 회피 이식으로 추가)
import { RawPathRuntime } from './raw-path-runtime.js';
import { RawSquidFinder } from './raw-squid-finder.js';

// 두 실제 PE 상수의 서로 다른 좌표 편향(단정도)과 ftol 의 signed64 불확정값 경계, 반환 코드, 최대 깊이, 슬롯의 위치이다. (2026-11-08 WB04-E101)
const SPOT_BIAS = Math.fround(0.9999);
const SNAP_BIAS = Math.fround(0.99999);
const INT64_BOUNDARY = 2 ** 63;
const GATED = 1000;
const FINISHED = 1001;
const MAX_DEPTH = 3;
const AROUND_EVENT = 0x1da1a;
const TYPE_AT = 10;
const X_AT = 14;
const Y_AT = 18;
const TYPE_COUNT = 188;
const SPOT_COUNT = 65536;
const CARDINAL_COUNT = 4;
const SURFACE = 0x800;
const FLYER = 0x20000;
const BLOCKED_SPOT = 0x10;
const fround = Math.fround;

// 도달한 큰 하위의 누락은 원문 부작용을 보존한 채 거부한다. (2026-11-08 WB04-E101)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('캐리어 회피 실제 하위 미연결');
    return hook;
}

// signed64 절삭 뒤 EAX 하위 signed32 만 사용하는 실제 변환이다(비유한·범위 밖은 0). (2026-11-08 WB04-E101)
function ftol(value) {
    if (!Number.isFinite(value) || value >= INT64_BOUNDARY || value < -INT64_BOUNDARY) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 유효한 signed 좌표 계산도 DWORD 감김으로 수행한다. (2026-11-08 WB04-E101)
const add = (first, second) => (first + second) | 0;

// 422230 은 41d7a0 의 정수→float 저장 뒤 다시 ftol 하여 중심을 만든다. (2026-11-08 WB04-E101)
const center = (value) => ftol(fround(ftol(value + SNAP_BIAS)));

// 52f83c 부터 연속한 17 DWORD 방향표이다. x 의 9번째와 y 의 첫 번째는 같은 물리 항목이다. (2026-11-08 WB04-E101)
export class CarrierEscapeState {
    // 첫 난수 방향 4 의 중복 서쪽도 원본 그대로이다.
    directions = [0, 1, 1, 1, 0, -1, -1, -1, -1, -1, 0, 1, 1, 1, 0, -1, 0];
}

// 운반체 회피 처리기이다. (2026-11-08 WB04-E101)
export class RawCarrierEscape {
    // 같은 실제 부모/후보 raw 저장소이다. (2026-11-08 WB04-E101)
    #pool;
    // 실제 5×5 일반 탐색기의 네 단계 해시이다. (2026-11-08 WB04-E101)
    #hash;
    // 현재 flags1 surface/flags2 비행 부류를 제공하는 TYPE 표이다. (2026-11-08 WB04-E101)
    #types;
    // 65536 칸의 현재 통행/점유 비트이다. (2026-11-08 WB04-E101)
    #spots;
    // 후보 탐색 뒤 현재 55b4d0 시각의 소유자(getter 또는 { value })이다. (2026-11-08 WB04-E101)
    #now;
    // 현재 5e4794 깊이 진단 전역 getter 이다. (2026-11-08 WB04-E101)
    #checking;
    // 기존 선택기와 같은 mutable 488fb0 타입 4종이다. (2026-11-08 WB04-E101)
    #cardinal;
    // 현재 물리 방향표 상태이다. (2026-11-08 WB04-E101)
    #state;
    // 실제 큰 하위의 외부 연결이다(도달할 때만 요구). (2026-11-08 WB04-E101)
    #hooks;

    // PATCH 의 실제 물리 자료만 받으며 CD 의 다른 회피를 이 몸체로 대신하지 않는다. (2026-11-08 WB04-E101)
    constructor(pool, hash, types, spots, now, checking, cardinalTypes, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT || !spots || spots.length !== SPOT_COUNT || !cardinalTypes || cardinalTypes.length !== CARDINAL_COUNT) {
            throw new Error('캐리어 회피 판본/물리 자료 오류');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#spots = spots;
        this.#now = now;
        this.#checking = checking;
        this.#cardinal = cardinalTypes;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 다른 실제 월드의 하위 연결을 확인할 때 쓰는 같은 풀이다. (2026-11-08 WB04-E101)
    get pool() { return this.#pool; }

    // 현재 시각이다. (2026-11-08 WB04-E101)
    #time() { return typeof this.#now === 'function' ? this.#now() : this.#now.value; }

    // 현재 진단 전역이 켜졌는지(비영) 읽는다. (2026-11-08 WB04-E101)
    #isChecking() { return Number(this.#checking()) !== 0; }

    // 최초 보관한 깊이는 진단 두 조건에 사용하고 최종 반환은 현재 같은 Runtime 에서 다시 읽는다. (2026-11-08 WB04-E101)
    #depth(parent) {
        const path = need(this.#hooks.path)(parent);
        if (!path) return 0;
        if (this.#isChecking()) {
            const depth = path.moveAwayDepth;
            if (depth < 0) need(this.#hooks.assertion)(0x87b);
            if (this.#isChecking() && depth > MAX_DEPTH) need(this.#hooks.assertion)(0x87c);
        }
        return path.moveAwayDepth;
    }

    // 초기 부모 위치와 후보의 효과 전후 위치를 별개로 조회한다. (2026-11-08 WB04-E101)
    #position(sid) {
        const raw = this.#pool.slot(sid);
        const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        return { x: view.getFloat32(X_AT, true), y: view.getFloat32(Y_AT, true) };
    }

    // 현재 BYTE 타입을 같은 실제 표의 물리 범위에서 읽는다. (2026-11-08 WB04-E101)
    #type(sid) {
        const kind = this.#pool.slot(sid)[TYPE_AT];
        if (kind >= this.#types.length) throw new RangeError('캐리어 회피 TYPE 범위 오류');
        return this.#types[kind];
    }

    // x/y 를 각각 보드로 clamp 하지 않고 원래 선형 DWORD 주소를 계산한다. (2026-11-08 WB04-E101)
    #blocked(point) {
        const x = ftol(point.x + SPOT_BIAS);
        const y = ftol(point.y + SPOT_BIAS);
        const index = (Math.imul(y, 256) + x) >>> 0;
        if (index >= this.#spots.length) throw new RangeError('캐리어 회피 spot 주소 범위 오류');
        return (this.#spots[index] & BLOCKED_SPOT) !== 0;
    }

    // 반환한 새 경로에만 깊이를 쓴다(이전 경로가 지워져도 그 이외에는 접근하지 않는다). (2026-11-08 WB04-E101)
    #start(parent, destination, depth) {
        const path = need(this.#hooks.start)(parent, destination);
        if (path) {
            path.setMoveAwayDepth(depth);
            return true;
        }
        return false;
    }

    // 깊이 관문은 1000, 그 외 실패를 포함한 회피 분기는 1001 이다. event 0x1da1a 는 네 방향을 건너뛰지만 난수는 먼저 소비한다. (2026-11-08 WB04-E101)
    run(parent, distance, event) {
        const hooks = this.#hooks;
        const nextDepth = add(this.#depth(parent), 1);
        if (nextDepth > MAX_DEPTH || (distance === 1 && (this.#type(parent).flags2 & FLYER))) return GATED;
        const origin = this.#position(parent);
        const searchAround = event === AROUND_EVENT;
        let direction = Math.trunc((event | 0) / 2);
        if (searchAround || event === 0xffffffff) direction = Math.trunc(need(hooks.random)(0, 5000) / 1000);
        if (!searchAround) {
            // 첫 방향 4 를 그대로 읽은 다음 signed 나머지로 회전한다.
            for (let attempt = 0; attempt < 4; attempt += 1) {
                if (direction < 0 || direction > 4) throw new RangeError('캐리어 회피 물리 방향 범위 오류');
                const index = direction * 2;
                const destination = {
                    x: fround(this.#state.directions[index] * distance + origin.x),
                    y: fround(this.#state.directions[index + 8] * distance + origin.y)
                };
                if (!this.#blocked(destination) && this.#start(parent, destination, nextDepth)) return FINISHED;
                direction = add(direction, 1) % 4;
            }
            return FINISHED;
        }
        const x = center(origin.x);
        const y = center(origin.y);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        let best = 10000;
        let chosen = { x: -10000, y: -10000 };
        const area = { left: add(x, -5), top: add(y, -5), right: add(x, 5), bottom: add(y, 5) };
        if (hooks.scan) hooks.scan(area);
        // 실제 일반 finder 의 현재 체인과 표면 후보를 원래 순서대로 평가한다.
        for (let candidate = finder.begin(area, 0); candidate; candidate = finder.next()) {
            if (!(this.#type(candidate).flags1 & SURFACE)) continue;
            const point = this.#position(candidate);
            if (this.#blocked(point) || need(hooks.blocker)(point)) continue;
            const temporary = new RawPathRuntime();
            const destination = this.#position(candidate);
            const kind = this.#pool.slot(parent)[TYPE_AT];
            const cardinal = this.#cardinal.includes(kind);
            need(hooks.search)(temporary, parent, origin, destination, cardinal ? 1 : 0);
            if (temporary.count) {
                const movement = need(hooks.movement)(parent);
                const now = this.#time();
                temporary.prepare(now, now, movement[0], movement[1], 0);
                const metric = temporary.time(temporary.count - 1) - this.#time();
                if (metric < best) {
                    best = fround(metric);
                    chosen = this.#position(candidate);
                }
            }
        }
        if (need(hooks.valid)(chosen)) this.#start(parent, chosen, nextDepth);
        return FINISHED;
    }
}
