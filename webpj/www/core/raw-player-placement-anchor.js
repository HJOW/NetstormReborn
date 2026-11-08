// Player 배치 기준점(0048fdb0)의 현재 작업장/추가 목록·그래프·종속 조건과 거리 선택을 옮긴 모듈이다(cpppj `o/RawPlayerPlacementAnchor.cpp/.h`, 10.78 전용).
//  - inspect(owner, type, x, y): 좌표 그래프(graphAt)가 무효 번호면 바로 끝 → owner 작업장 목록을 매번 다시 읽으며(중복·순서 유지) 요청 TYPE 의 genus(flags2 & 6) 또는
//    작업장의 종속 내용물(현재 contained 타입·mask 0x10·kind == type) 또는 현재 FullDeck(종속 조회 뒤 다시 읽음)으로 허용한 후보 중 그래프가 같은 것을 40개까지 모은다.
//    후보가 있거나 종속으로 허용된 작업장이 있었으면 추가 목록에서 현재 owner 가 같고 그래프가 같은 것을 더한다. 하나면 그것, 여럿이면 거리 최소(nearest)를 고른다.
//  - graphAt(x, y): 그래프 비활성이면 254. 좌표 + float 0.9999 를 double 로 더해 0 쪽 절삭한 칸의 표면 지도 SID 의 raw graph BYTE(없으면 254).
//  - graphFor(sid): SID 0 은 10.78 checking 이면 Graph.cpp 0x93 진단 뒤 0, 표면 타입은 자기 raw graph, 그 밖은 자기 좌표의 graphAt 이다.
//  - nearest: owner BYTE 가 같은 후보의 double 거리와 float 최소값(초기 9999)을 비교하고 채택한 최소값만 float 로 저장한다(10.78 은 NaN 을 채택하지 않음).
// 임시 목록은 중복을 지우지 않으며 가득 찬 뒤에도 원본의 종속/그래프 조회 순서를 유지한다. TYPE 표·표면 지도·장부·목록·상태는 복사하지 않고 빌린다.
// 이 모듈은 DOM 을 쓰지 않는다. CD 10.72 분기(BYTE graph +28/owner +32·NaN 채택)는 옮기지 않았다. (2026-11-08 WB04-E57: Player 배치 기준점 이식으로 추가)
import { ContainedFinderState, RawContainedFinder } from './raw-contained-finder.js';

// 원본 임시 목록 상한, 거리 선택의 float 초기 최소값, 디컴파일에서 생략된 fadd 상수(00500edc 의 float 0.9999)이다. (2026-11-08 WB04-E57)
export const PLAYER_ANCHOR_LIMIT = 40;
const INITIAL_DISTANCE = Math.fround(9999.0);
const COORDINATE_BIAS = Math.fround(0.9999);

// 공통 raw 타입·좌표·종속 kind/mask·graph·owner 위치와 표면/종속 비트, 무효 그래프 기본값, TYPE 수·표면 지도 크기이다. (2026-11-08 WB04-E57)
const TYPE_OFFSET = 10;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const KIND_OFFSET = 18;
const MASK_OFFSET = 20;
const GRAPH_OFFSET = 30;
const OWNER_OFFSET = 34;
const SURFACE = 0x800;
const DEPENDENT_MASK = 0x10;
const GENUS_ALLOWED = 6;
const INVALID_GRAPH = 254;
const TYPE_COUNT = 188;
const MAP_SIZE = 256;

// 원본 SID 0 그래프 진단의 식·파일·줄이다. (2026-11-08 WB04-E57)
export const PLAYER_ANCHOR_GRAPH_ASSERTION = Object.freeze({ expression: 'mySid', file: '\\Ns\\O\\Graph.cpp', line: 0x93 });

// 비정렬 little endian WORD 를 읽는다. (2026-11-08 WB04-E57)
function word(raw, offset) {
    return raw[offset] | (raw[offset + 1] << 8);
}

// 비정렬 raw 좌표의 float 비트를 읽는다(NaN/부호 있는 0 도 그대로). (2026-11-08 WB04-E57)
function coordinate(raw, offset) {
    return new DataView(raw.buffer, raw.byteOffset + offset, 4).getFloat32(0, true);
}

// x87 의 좌표 + float 상수는 double 로 계산한 뒤 float 로 좁히지 않고 CRT 절삭한다. 정수 범위 밖은 오류이다. (2026-11-08 WB04-E57)
function truncate(value) {
    const result = Math.trunc(value + COORDINATE_BIAS);
    if (!Number.isFinite(result) || result < -2147483648 || result > 2147483647) throw new RangeError('Player 그래프 좌표 절삭 오류');
    return result;
}

// 배치 기준점이 읽는 현재 전역이다(10.78 주소). 사용: const state = new PlayerPlacementAnchorState(); state.graphReady = 1; state.bindFullDeck(() => fullDeck). (2026-11-08 WB04-E57)
export class PlayerPlacementAnchorState {
    // 그래프 활성 여부(DWORD)이다.
    graphReady = 0;
    // 무효 그래프 번호(invalidGraphNum)이다.
    invalidGraph = INVALID_GRAPH;
    // 독립 입력의 종속 조건 우회 fallback DWORD(0054db14)이다.
    ignoreRestrictions = 0;
    // 10.78 raw graph getter 가 표면 밖에서도 허용하는 두 현재 타입 번호(fence·windArcher)이다.
    fenceType = 0;
    windArcherType = 0;
    // 10.78 SID 0 그래프 진단 장치 (식, 파일, 줄)이다. 복귀하면 원문대로 0 을 돌려준다.
    graphAssertion = null;
    // 실제 0054db14 FullDeck DWORD 의 현재 getter 이다(없으면 null 이고 fallback 을 쓴다).
    #readFullDeck = null;

    // 실제 FullDeck getter(() => DWORD)를 빌린다. null 이면 fallback 으로 돌아간다.
    bindFullDeck(getter) {
        if (getter !== null && typeof getter !== 'function') throw new TypeError('FullDeck 은 getter 여야 합니다.');
        this.#readFullDeck = getter;
    }

    // 후보별 종속 조회 뒤 DWORD 전체를 정규화 없이 다시 읽는다.
    get currentIgnoreRestrictions() { return this.#readFullDeck ? this.#readFullDeck() >>> 0 : this.ignoreRestrictions >>> 0; }
}

// Player 배치 기준점 처리기이다. 원본의 별도 추가 목록을 공통 공장 목록과 구분하며 모든 자료는 이 객체보다 오래 살아야 한다.
// 사용: const anchor = new RawPlayerPlacementAnchor(pool, types, surfaces, bookkeeping, additional, contained, state); anchor.locate(1, 83, 20, 21). (2026-11-08 WB04-E57)
export class RawPlayerPlacementAnchor {
    // 같은 실제 SID 풀이다.
    #pool;
    // 번호로 속성을 조회하는 현재 TYPE 표(188칸, 빌림)이다.
    #types;
    // 256×256 표면 SID 지도(Uint16Array, 빌림)이다.
    #surfaces;
    // 소유자별 작업장 목록을 가진 공통 장부(SquidPostPopState)이다.
    #bookkeeping;
    // 원본 별도 추가 목록(SquidPostPopList)이다.
    #additional;
    // 종속 조회가 쓰는 현재 contained 타입·권한·checking 상태이다.
    #contained;
    // 그래프 활성·무효 번호·FullDeck·진단의 현재 상태이다.
    #state;

    // 판본 크기의 TYPE 표와 65536칸 표면 지도를 확인한다.
    constructor(pool, types, surfaces, bookkeeping, additional, contained, state) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Player 배치 기준점은 10.78 SID 풀만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || surfaces?.length !== MAP_SIZE * MAP_SIZE) throw new RangeError('Player 배치 기준점 자료 크기 오류');
        if (!Array.isArray(bookkeeping?.ownerFactories) || typeof additional?.items !== 'function') throw new TypeError('Player 배치 기준점 장부/추가 목록이 필요합니다.');
        if (!(contained instanceof ContainedFinderState) || !(state instanceof PlayerPlacementAnchorState)) throw new TypeError('Player 배치 기준점 상태가 필요합니다.');
        this.#pool = pool;
        this.#types = types;
        this.#surfaces = surfaces;
        this.#bookkeeping = bookkeeping;
        this.#additional = additional;
        this.#contained = contained;
        this.#state = state;
    }

    // 권한 함수가 같은 raw 풀만 연결하도록 제공한다.
    get pool() { return this.#pool; }

    // WORD SID 로 좁히기 전에 실제 풀과 번호의 물리 범위를 검사한다(손상된 DWORD 를 다른 SID 로 감지 않음).
    #raw(sid) {
        if (!Number.isInteger(sid) || sid < 0 || sid > 0xffff || sid >= this.#pool.capacity) throw new RangeError('Player 기준점 SID 오류');
        return this.#pool.slot(sid);
    }

    // 10.78 의 표면/특수 타입 assert 계약을 지키고 raw graph BYTE 를 읽는다.
    #rawGraphByte(sid) {
        const raw = this.#raw(sid);
        const type = raw[TYPE_OFFSET];
        const state = this.#state;
        if (type >= this.#types.length || (!((this.#types[type].flags1 >>> 0) & SURFACE) && type !== state.fenceType && type !== state.windArcherType)) throw new Error('Player 표면 graph getter 타입 오류');
        return raw[GRAPH_OFFSET];
    }

    // 00462b80: 그래프 비활성은 좌표를 읽기 전에 고정 무효 값 254 이다. 0.9999 를 더해 절삭한 칸 밖이나 빈 칸도 254 이다.
    graphAt(x, y) {
        if (!(this.#state.graphReady >>> 0)) return INVALID_GRAPH;
        const px = truncate(x);
        const py = truncate(y);
        if (px < 0 || py < 0 || px > 255 || py > 255) return INVALID_GRAPH;
        const sid = this.#surfaces[py * MAP_SIZE + px];
        return sid ? this.#rawGraphByte(sid) : INVALID_GRAPH;
    }

    // 00462bc0: SID 0 → 진단 뒤 0, 표면 타입은 그래프 활성과 무관하게 자기 raw 번호, 그 밖은 자기 좌표의 그래프이다.
    graphFor(sid) {
        if (!sid) {
            if (this.#contained.currentChecking) {
                const assertion = this.#state.graphAssertion;
                if (typeof assertion !== 'function') throw new Error('Player graph SID 0 실제 진단 미연결');
                assertion(PLAYER_ANCHOR_GRAPH_ASSERTION.expression, PLAYER_ANCHOR_GRAPH_ASSERTION.file, PLAYER_ANCHOR_GRAPH_ASSERTION.line);
            }
            return 0;
        }
        const raw = this.#raw(sid);
        if (raw[TYPE_OFFSET] >= this.#types.length) throw new RangeError('Player graph 타입 오류');
        return (this.#types[raw[TYPE_OFFSET]].flags1 >>> 0) & SURFACE ? this.#rawGraphByte(sid) : this.graphAt(coordinate(raw, X_OFFSET), coordinate(raw, Y_OFFSET));
    }

    // 기존 contained 커서로 현재 타입 후보 중 요청 kind 와 mask 0x10 이 맞는 종속이 있는지 본다(인덱스 0).
    #hasContained(parent, kind) {
        this.#raw(parent);
        const finder = new RawContainedFinder(this.#pool, this.#contained);
        // 일치 타입 후보만 요청 kind 와 mask 로 검사한다.
        for (let sid = finder.begin(parent); sid; sid = finder.next()) {
            const raw = this.#pool.slot(sid);
            if ((!kind || word(raw, KIND_OFFSET) === kind) && (word(raw, MASK_OFFSET) & DEPENDENT_MASK)) return true;
        }
        return false;
    }

    // 0048fd30: owner BYTE 가 같은 후보의 double 거리를 float 최소값(초기 9999)과 비교한다. 동률은 앞의 것을 유지한다.
    nearest(candidates, owner, x, y) {
        let best = INITIAL_DISTANCE;
        let selected = 0;
        const wanted = owner >>> 0;
        // 현재 raw 소유자와 좌표를 목록 순서대로 읽는다.
        for (const sid of candidates) {
            const raw = this.#raw(sid);
            if (raw[OWNER_OFFSET] !== wanted) continue;
            const dx = x - coordinate(raw, X_OFFSET);
            const dy = y - coordinate(raw, Y_OFFSET);
            const distance = Math.sqrt(dx * dx + dy * dy);
            // 10.78 은 C0/C2 를 함께 검사해 비순서(NaN)를 채택하지 않는다.
            if (distance < best) {
                best = Math.fround(distance);
                selected = sid;
            }
        }
        return selected;
    }

    // 0048fdb0 의 몸체와 임시 후보 목록 관찰이다: { sid, candidates }. type 과 owner 는 실제로 읽는 시점에 검사해 무효 그래프의 조기 반환을 보존한다.
    inspect(owner, type, x, y) {
        const result = { sid: 0, candidates: [] };
        const graph = this.graphAt(x, y);
        if (graph === (this.#state.invalidGraph >>> 0)) return result;
        const ownerValue = owner >>> 0;
        const typeValue = type >>> 0;
        if (ownerValue >= this.#bookkeeping.ownerFactories.length) throw new RangeError('Player 기준점 소유자 오류');
        const workshops = this.#bookkeeping.ownerFactories[ownerValue];
        let linked = false;
        // 목록을 매번 다시 읽고 중복·원래 순서·상한 이후의 조회까지 유지한다.
        for (let index = 0; index < workshops.items().length; index += 1) {
            const sid = workshops.items()[index];
            this.#raw(sid);
            if (typeValue >= this.#types.length) throw new RangeError('Player 기준점 요청 타입 오류');
            let allowed = ((this.#types[typeValue].flags2 >>> 0) & GENUS_ALLOWED) !== 0;
            if (!allowed) {
                allowed = this.#hasContained(sid, typeValue);
                linked = linked || allowed;
                if (!allowed) allowed = this.#state.currentIgnoreRestrictions !== 0;
            }
            if (allowed && this.graphFor(sid) === graph && result.candidates.length < PLAYER_ANCHOR_LIMIT) result.candidates.push(sid);
        }
        if (result.candidates.length || linked) {
            // 추가 목록은 현재 raw owner 가 같은 것만 받고 그래프 조회 뒤 40개 상한을 적용한다.
            for (let index = 0; index < this.#additional.items().length; index += 1) {
                const sid = this.#additional.items()[index];
                const raw = this.#raw(sid);
                if (raw[OWNER_OFFSET] === ownerValue && this.graphFor(sid) === graph && result.candidates.length < PLAYER_ANCHOR_LIMIT) result.candidates.push(sid);
            }
        }
        if (result.candidates.length === 1) result.sid = result.candidates[0];
        else if (result.candidates.length) result.sid = this.nearest(result.candidates, ownerValue, x, y);
        return result;
    }

    // 0048fdb0: 임시 목록은 호출마다 새로 만들고 원본 DWORD 반환만 권한 함수에 넘긴다.
    locate(owner, type, x, y) { return this.inspect(owner, type, x, y).sid; }
}

// 범위와 같은 풀을 확인한 실제 조회 함수를 후보 권한의 필수 경계에 연결한다. (2026-11-08 WB04-E57)
export function makePlayerAnchorQuery(pool, anchor) {
    if (anchor?.pool !== pool) throw new TypeError('Player 기준점/권한 SID 풀 불일치');
    return (owner, type, x, y) => anchor.locate(owner, type, x, y);
}
