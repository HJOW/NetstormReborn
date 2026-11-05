// 원본 10.78 일반 사각형 탐색기(finder +4..+60)를 옮긴 모듈이다. 4단계 공간 해시의 버킷 머리에서 시작해 SID 풀 슬롯의 next WORD 체인을 따라가며
// 탐색 사각형과 발자국이 겹치는 객체를 하나씩 돌려준다. flag 8 표면 이웃 탐색(SurfaceFinder)과는 다른 장치이다.
// 원본 순서를 그대로 지킨다: 단계(0 또는 1부터 3까지) → y → x 버킷, 버킷 안에서는 체인 순서. 후보의 next 는 필터·호출자 효과보다 먼저 저장하고,
// 그 뒤의 후보 좌표/타입/buried 비트와 아직 읽지 않은 버킷 머리는 읽는 시점의 현재 값을 쓴다(사본·정렬·별도 좌표 목록으로 대신하지 않는다).
// 근거: cpppj `o/RawSquidFinder.cpp/.h`(원본 004b16d0 Begin, 004b1810 Next), 읽기 전용 원본 관측 `cpppj/tests/fixtures/rawfinder-x86.tsv` 의 10.78 Find 584행.
// 다리 삭제 훅(Pre/Post)·실제 생성/Pop 등록은 WB04-D/E 범위이며 이 모듈은 탐색만 한다. CD 10.72 경로는 범위 밖이다.
// (2026-11-05 WB04-B: 실제 월드 탐색기 이식으로 추가)
import { WORLD_CELLS } from '../assets/terrain-mask.js';
import { SQUID_HASH_LEVELS, SQUID_HASH_SCALES, SquidHash } from './squid-hash.js';

// 10.78 슬롯의 next WORD·타입 BYTE·x/y float·extra BYTE 위치이다(CD 는 extra +35 로 다르다). (2026-11-05 WB04-B)
const NEXT = 4, TYPE = 10, X = 14, Y = 18, EXTRA = 40;

// extra 의 buried 비트이다. 일반 탐색은 이 비트만 제외하고 free/dead/void 를 따로 거르지 않는다(원본과 같음). (2026-11-05 WB04-B)
const BURIED = 8;

// Begin 이 받는 플래그: 1 = 표면(0단계) 건너뛰기, 4 = 타입별 높이 확장 사용. 다른 비트는 거부한다. (2026-11-05 WB04-B)
export const RAW_FINDER_FLAGS = Object.freeze({ skipSurface: 1, extendedHeight: 4 });

// 32비트 int 의 최댓값이다(확장 높이 오버플로 보호). (2026-11-05 WB04-B)
const INT_MAX = 0x7fffffff;

// 정수 입력을 검사한다. (2026-11-05 WB04-B)
function int32(value, name) {
    if (!Number.isInteger(value) || value < -0x80000000 || value > INT_MAX) throw new RangeError(`${name} 정수 범위 오류`);
    return value;
}

// 원본 finder 상태의 읽기 사본을 만든다. 호출자가 바꿔도 탐색기 상태는 바뀌지 않는다. (2026-11-05 WB04-B)
function copyState(state) {
    return { ...state, area: { ...state.area } };
}

// 원본 일반 사각형 탐색기이다. pool 은 WB04-A SidPool(같은 실제 풀), hash 는 SquidHash, types 는 타입 번호 → { footX, footY } 배열,
// heightExtensions 는 타입 번호 → 높이 확장(원본 type +1e4) 배열이다(플래그 4 를 쓸 때만 필요). 풀/해시/타입은 탐색기보다 오래 살아야 하며 복사하지 않는다.
// 사용: const finder = new RawSquidFinder(pool, hash, types); for (let sid = finder.begin({ left: 20, top: 21, right: 20, bottom: 21 }); sid !== 0; sid = finder.next()) { … }
// (2026-11-05 WB04-B)
export class RawSquidFinder {
    // 실제 SID 풀이다.
    #pool;
    // 실제 공간 해시이다.
    #hash;
    // 타입 번호 → 발자국 표이다.
    #types;
    // 타입 번호 → 높이 확장 표이다.
    #heightExtensions;
    // 원본 finder +4..+60 의 상태이다(사각형·버킷 범위·단계·커서·미리 읽은 next·현재 후보·두 플래그).
    #state = RawSquidFinder.#emptyState();
    // 현재 후보를 더 거르는 가상 필터이다. null 이면 원본 기본 필터(0044daa0)처럼 모두 허용한다.
    #filter = null;
    // 같은 버킷 체인의 순환 검출용 방문 번호이다. 다른 버킷의 중복은 원본처럼 허용한다.
    #chain = new Set();
    // Begin 을 불렀는지 여부이다.
    #started = false;

    // 탐색기를 만든다. 10.78 풀만 받는다.
    constructor(pool, hash, types, heightExtensions = []) {
        if (pool?.edition !== 'patch1078') throw new TypeError('RawSquidFinder 는 10.78 SID 풀만 받습니다.');
        if (!(hash instanceof SquidHash)) throw new TypeError('RawSquidFinder 에는 SquidHash 가 필요합니다.');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#heightExtensions = heightExtensions;
    }

    // 빈 상태이다. (2026-11-05 WB04-B)
    static #emptyState() {
        return { area: { left: 0, top: 0, right: -1, bottom: -1 }, firstX: 0, firstY: 0, lastX: 0, lastY: 0, level: 0, x: 0, y: 0, next: 0, current: 0, skipSurface: false, extendedHeight: false };
    }

    // 현재 상태의 읽기 사본이다(원본 finder 커서 대조·후속 수명 연결용).
    get state() { return copyState(this.#state); }

    // 연결된 실제 SID 풀이다. Fort 로더(fort-object-load.js)가 같은 풀인지 `finder.pool === pool` 로 확인한다(복사본이 아님).
    get pool() { return this.#pool; }

    // 연결된 실제 공간 해시이다. 후속 spot/Pop 등록이 같은 저장소에 체인을 끼워 넣을 때 쓴다.
    get hash() { return this.#hash; }

    // 원본 004b16d0: 사각형을 보드로 자르고(오른쪽/아래 -1 은 보드 끝) 첫 단계 버킷 머리를 읽은 뒤 첫 Next 를 부른다. 재시작은 이전 커서/필터를 덮어쓴다.
    // area 는 { left, top, right, bottom } 정수이며 경계 접촉을 포함한다. flags 는 RAW_FINDER_FLAGS 조합이다. 반환은 첫 후보 SID(없으면 0)이다.
    begin(area, flags = 0, filter = null) {
        if (!Number.isInteger(flags) || (flags & ~5) !== 0 || flags < 0) throw new RangeError('RawSquidFinder 플래그 오류');
        if ((flags & 4) !== 0 && this.#heightExtensions.length < this.#types.length) throw new RangeError('RawSquidFinder 확장 높이 표가 타입 표보다 짧습니다.');
        if (filter !== null && typeof filter !== 'function') throw new TypeError('RawSquidFinder 필터는 함수여야 합니다.');
        const right = int32(area?.right ?? -1, 'right');
        const bottom = int32(area?.bottom ?? -1, 'bottom');
        const clipped = {
            left: Math.max(int32(area?.left ?? 0, 'left'), 0),
            top: Math.max(int32(area?.top ?? 0, 'top'), 0),
            right: Math.min(right === -1 ? WORLD_CELLS : right, WORLD_CELLS - 1),
            bottom: Math.min(bottom === -1 ? WORLD_CELLS : bottom, WORLD_CELLS - 1),
        };
        this.#state = { ...RawSquidFinder.#emptyState(), area: clipped, skipSurface: (flags & 1) !== 0, extendedHeight: (flags & 4) !== 0 };
        this.#filter = filter;
        this.#started = true;
        this.#setLevel(this.#state.skipSurface ? 1 : 0);
        this.#state.next = this.#hash.cell(this.#state.level, this.#state.x, this.#state.y);
        return this.next();
    }

    // 원본 004b1810: 저장한 next 부터 체인을 따라가고, 체인이 끝나면 다음 x → y → 단계 버킷으로 간다. buried 후보는 건너뛰고,
    // 발자국 교차와 필터를 통과한 첫 후보를 돌려준다. 모두 소진하면 0 이다. Begin 전에 부르면 오류이다.
    next() {
        if (!this.#started) throw new Error('RawSquidFinder: begin 전에 next 를 불렀습니다.');
        const state = this.#state;
        if (state.level >= SQUID_HASH_LEVELS) return 0;
        // 단계별 제한된 버킷 범위와 원본 체인을 순서대로 방문한다.
        for (;;) {
            if (state.next === 0) {
                if (!this.#advanceBucket()) return 0;
                continue;
            }
            state.current = state.next;
            if (state.current >= this.#pool.capacity) throw new RangeError('RawSquidFinder: SID 가 풀 크기를 넘었습니다.');
            if (this.#chain.has(state.current)) throw new Error('RawSquidFinder: 같은 버킷 체인이 순환합니다.');
            this.#chain.add(state.current);
            const raw = this.#pool.slot(state.current);
            state.next = raw[NEXT] | (raw[NEXT + 1] << 8);
            if ((raw[EXTRA] & BURIED) !== 0) continue;
            if (this.#intersects(state.current) && (this.#filter === null || this.#filter(state.current))) return state.current;
        }
    }

    // 단계를 정하고 각 단계에서 오른쪽/아래 버킷을 한 칸 넓힌 뒤 배열 끝으로 자른다. 정수 나눗셈은 0 방향 절삭이다(원본 helper). (2026-11-05 WB04-B)
    #setLevel(level) {
        const state = this.#state;
        state.level = level;
        const scale = SQUID_HASH_SCALES[level];
        const last = SquidHash.side(level) - 1;
        const area = state.area;
        state.firstX = Math.min(Math.trunc(area.left / scale), last);
        state.firstY = Math.min(Math.trunc(area.top / scale), last);
        state.lastX = Math.min(Math.max(Math.min(Math.trunc(area.right / scale), last) + 1, 0), last);
        state.lastY = Math.min(Math.max(Math.min(Math.trunc(area.bottom / scale), last) + 1, 0), last);
        state.x = state.firstX;
        state.y = state.firstY;
        state.current = 0;
        this.#chain.clear();
    }

    // 다음 버킷/행/단계로 간다. 전체 소진이면 current 를 0 으로 두고 false 이다(단계는 4 로 남는다). (2026-11-05 WB04-B)
    #advanceBucket() {
        const state = this.#state;
        state.x += 1;
        if (state.x > state.lastX) {
            state.x = state.firstX;
            state.y += 1;
            if (state.y > state.lastY) {
                state.level += 1;
                if (state.level >= SQUID_HASH_LEVELS) {
                    state.current = 0;
                    return false;
                }
                this.#setLevel(state.level);
            }
        }
        this.#chain.clear();
        state.next = this.#hash.cell(state.level, state.x, state.y);
        return true;
    }

    // 후보의 현재 float 좌표를 절삭한 정수 발자국(오른쪽 아래 기준)과 탐색 사각형의 원본 OR 교차식을 검사한다.
    // 10.78 은 보드 밖·0 이하 좌표를 건너뛴다(false). 발자국/확장 높이가 음수이거나 int 를 넘으면 오류이다. (2026-11-05 WB04-B)
    #intersects(sid) {
        const raw = this.#pool.slot(sid);
        const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        const x = view.getFloat32(X, true);
        const y = view.getFloat32(Y, true);
        if (!Number.isFinite(x) || !Number.isFinite(y) || x <= 0 || y <= 0 || x >= WORLD_CELLS || y >= WORLD_CELLS) return false;
        const number = raw[TYPE];
        if (number >= this.#types.length) throw new RangeError('RawSquidFinder: 타입 번호가 타입 표 밖입니다.');
        const type = this.#types[number];
        const footX = type?.footX;
        const height = (type?.footY ?? NaN) + (this.#state.extendedHeight ? this.#heightExtensions[number] : 0);
        // Dude 70 의 0×0 발자국도 원본 OR 교차식에서 유효하다. 음수/확장 오버플로는 거부한다.
        if (!Number.isInteger(footX) || !Number.isInteger(height) || footX < 0 || height < 0 || height > INT_MAX) throw new RangeError('RawSquidFinder: 발자국 범위 오류');
        const right = Math.trunc(x);
        const bottom = Math.trunc(y);
        const left = right - footX + 1;
        const top = bottom - height + 1;
        const area = this.#state.area;
        return (area.left <= right || area.left <= left) && (right <= area.right || left <= area.right) &&
            (area.top <= bottom || area.top <= top) && (bottom <= area.bottom || top <= area.bottom);
    }
}
