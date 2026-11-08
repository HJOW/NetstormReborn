// 배치 디코더의 현재 모양에서 flag 8 표면 지도를 동적으로 순회하고 주변 권한을 누적하는 모듈이다(cpppj `o/RawCanonPlacementSurrounding.cpp/.h`, 10.78).
//  - RawCanonSurfaceWalk: 004b23e0 모양 탐색 생성자와 flag 8 지도 반복/필터를 옮긴 순회기이다. 모양 원천은 두 가지이며 같은 발자국/중심/최초 방문을 만든다.
//    · RawCanonSurfaceWalk.forPlacement(pool, types, frames, islands, spots, query, cell): 배치 질의 타입과 디코더의 현재 칸(프레임·원래 좌표). frames 는 타입별 { frames: 코드 배열 }.
//    · RawCanonSurfaceWalk.forSquid(pool, types, codes, islands, spots, sid): 실제 Squid 슬롯의 타입·프레임 DWORD·float 좌표. codes 는 타입별 프레임 코드 배열의 배열.
//    생성 직후 current 가 첫 결과이며 next() 가 지도를 그 시점에 다시 읽어 다음 결과를 돌려준다(0 이면 종료 상태 유지). state 는 원본 커서(x·y·left·top·right·bottom)와 signed WORD 중복 캐시(returned)이다.
//    발자국은 1..255 로 자른 뒤 내부 spot 비트(8)가 모두 켜져 있으면 곧바로 종료한다. 확장 사각형의 네 모서리는 건너뛰고, 모양 안쪽 후보와 이미 반환한 후보, 표면 타입이 아니거나 접합/죽음/내부 비트에 걸리는 후보는 거른다.
//  - new RawCanonPlacementSurrounding(pool, types, frames, islands, spots, permission): inspect(query, cell) 은 현재 모양의 표면 후보 모두에 바깥 관계 검사와 실제 후보 권한(permission.surrounding)을 적용해
//    하나라도 권한을 얻으면 참이다. 권한을 얻은 뒤에도 원본처럼 남은 후보를 계속 읽는다. 요청 소유자는 하위 BYTE 만 부호 확장한다.
//  - makeCanonSurroundingTerrainHooks(pool, surrounding, hooks): 모양 종료의 주변 권한 경계(shapePermission)만 실제 순회로 바꾼다.
// 10.72(CD)의 BYTE 프레임/소유자 위치/루프 차이는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E151: 일반 배치 주변 표면 순회 이식으로 추가)
import { TYPE_FLAG1_SURFACE, bridgeConnects } from './surface-finder.js';
import { RawSquidNeighbors } from './raw-squid-neighbors.js';

// 원본 모서리 clamp 범위와 기준점 spot 절삭 보정(float 0.9999), 중복 캐시 용량과 정상 비교 SID 상한, 지도 크기와 타입 표 크기이다. (2026-11-09 WB04-E151)
const FIRST = 1;
const LAST = 255;
const BIAS = Math.fround(0.9999);
const CACHE_SIZE = 64;
const LARGEST_SID = 32767;
const MAP_SIDE = 256;
const TYPE_COUNT = 188;

// 슬롯 필드 위치(10.78): 타입(+10)·상태(+11, bit1 죽음)·x(+14)·y(+18)·프레임 DWORD(+36)이다. (2026-11-09 WB04-E151)
const TYPE = 10;
const STATE = 11;
const X = 14;
const Y = 18;
const FRAME = 36;

// 슬롯의 비정렬 DWORD 를 리틀엔디언으로 읽는다. (2026-11-09 WB04-E151)
function dword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// DWORD 비트를 단정도 float 로 해석한다. (2026-11-09 WB04-E151)
function floatAt(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, dword(raw, offset), true);
    return view.getFloat32(0, true);
}

// 지도 안의 유한 float 계약을 확인하고 원본 CRT 처럼 0 방향으로 절삭한다. (2026-11-09 WB04-E151)
function cell(value) {
    if (!Number.isFinite(value) || value < 0 || value >= MAP_SIDE) throw new RangeError('주변 표면 좌표 오류');
    return Math.trunc(value);
}

// std::clamp(value, low, high)와 같은 비교 순서이다(NaN 은 그대로). (2026-11-09 WB04-E151)
function clamp(value, low, high) { return value < low ? low : (high < value ? high : value); }

// 현재 모양에서 flag 8 표면 지도를 동적으로 순회하는 커서이다. 모양의 float 발자국/중심/프레임 번호는 생성 때 고정하고 지도/raw/코드 표는 next 마다 현재 값을 읽는다. (2026-11-09 WB04-E151)
export class RawCanonSurfaceWalk {
    #pool;
    #types;
    #codes;
    #islands;
    #spots;
    #sourceType = 0;
    #sourceFrame = 0;
    #left = 0;
    #top = 0;
    #right = 0;
    #bottom = 0;
    #centerX = 0;
    #centerY = 0;
    #ended = false;
    #state = { x: 0, y: 0, left: 0, top: 0, right: 0, bottom: 0, current: 0, returned: [] };

    // 배치 질의의 타입과 디코더 칸에서 모양을 만든다. 지형용 snapped 좌표나 충돌 사각형이 아니라 디코더의 현재 x/y 를 쓴다. (2026-11-09 WB04-E151)
    static forPlacement(pool, types, frames, islands, spots, query, cell) {
        if (!Array.isArray(frames)) throw new TypeError('주변 표면 자료 크기 오류');
        return new RawCanonSurfaceWalk(pool, types, (type) => frames[type].frames, frames.length, islands, spots, query.type, cell);
    }

    // 실제 004b23e0(sid, 8)의 생성 시점 타입/프레임/float 기준점을 같은 순회에 공급한다. (2026-11-09 WB04-E151)
    static forSquid(pool, types, codes, islands, spots, sid) {
        if (!Array.isArray(codes)) throw new TypeError('주변 표면 자료 크기 오류');
        const raw = pool.slot(sid);
        return new RawCanonSurfaceWalk(pool, types, (type) => codes[type], codes.length, islands, spots, raw[TYPE], { frame: dword(raw, FRAME) | 0, x: floatAt(raw, X), y: floatAt(raw, Y) });
    }

    // 두 원천의 같은 발자국/중심/최초 방문을 구성한다. codesOf(type)는 타입의 현재 프레임 코드 배열을 매번 읽는 함수이다. (2026-11-09 WB04-E151)
    constructor(pool, types, codesOf, frameTableSize, islands, spots, sourceType, origin) {
        if (pool?.edition !== 'patch1078') throw new TypeError('주변 표면은 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || frameTableSize !== types.length || !islands || islands.length !== MAP_SIDE * MAP_SIDE || !spots || spots.length !== MAP_SIDE * MAP_SIDE) {
            throw new TypeError('주변 표면 자료 크기 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#codes = codesOf;
        this.#islands = islands;
        this.#spots = spots;
        this.#sourceType = sourceType;
        if (!Number.isInteger(sourceType) || sourceType < 0 || sourceType >= types.length) throw new RangeError('주변 표면 배치 타입 오류');
        const type = types[sourceType];
        const codes = codesOf(sourceType);
        if (type.footX < 1 || type.footY < 1 || type.footX > 12 || type.footY > 12 || origin.frame < 0 || origin.frame >= codes.length) throw new RangeError('주변 표면 발자국/프레임 오류');
        cell(origin.x);
        cell(origin.y);
        this.#sourceFrame = origin.frame;
        this.#left = clamp(Math.fround(origin.x - (type.footX - 1)), FIRST, LAST);
        this.#top = clamp(Math.fround(origin.y - (type.footY - 1)), FIRST, LAST);
        this.#right = origin.x < this.#left ? this.#left : origin.x;
        this.#bottom = origin.y < this.#top ? this.#top : origin.y;
        this.#centerX = Math.fround(origin.x - type.footX * 0.5);
        this.#centerY = Math.fround(origin.y - type.footY * 0.5);
        let bits = 0xff;
        // 원본은 발자국을 1..255 로 자른 뒤 y/x 순서로 spot 을 AND 한다.
        for (let y = cell(this.#top); y <= Math.min(255, cell(this.#bottom)); y += 1) {
            // 발자국 안 모든 칸의 공통 내부 비트만 사용한다.
            for (let x = cell(this.#left); x <= Math.min(255, cell(this.#right)); x += 1) bits &= spots[y * MAP_SIDE + x];
        }
        if ((bits & 8) !== 0) {
            this.#ended = true;
            return;
        }
        const state = this.#state;
        state.left = Math.trunc(Math.fround(this.#left - 1));
        state.top = Math.trunc(Math.fround(this.#top - 1));
        state.right = Math.trunc(Math.fround(this.#right + 1));
        state.bottom = Math.trunc(Math.fround(this.#bottom + 1));
        state.x = state.left;
        state.y = state.top;
        this.next();
    }

    // 생성 직후 첫 결과와 이후 결과를 원본처럼 노출한다. 0 반환 뒤에는 종료 상태를 유지한다. (2026-11-09 WB04-E151)
    get current() { return this.#state.current; }

    // 기계어 관찰과 지도 변경 검증에 사용할 실제 커서(물리 포인터 제외)이다. (2026-11-09 WB04-E151)
    get state() { return this.#state; }

    // 표면 타입 → 접합 → 죽음/내부 비트 순서로 flag 8 가상 필터를 적용한다. 필터는 원본처럼 후보 spot 부터 읽는다. (2026-11-09 WB04-E151)
    #accept(candidate) {
        const raw = this.#pool.slot(candidate);
        const x = floatAt(raw, X);
        const y = floatAt(raw, Y);
        cell(x);
        cell(y);
        const spot = Math.trunc(y + BIAS) * MAP_SIDE + Math.trunc(x + BIAS);
        if (spot >= this.#spots.length) throw new RangeError('주변 표면 기준점 spot 오류');
        const bits = this.#spots[spot];
        if (raw[TYPE] >= this.#types.length) throw new RangeError('주변 표면 후보 타입 오류');
        const type = this.#types[raw[TYPE]];
        if ((type.flags1 & TYPE_FLAG1_SURFACE) === 0) return false;
        const frame = dword(raw, FRAME);
        const codes = this.#codes(raw[TYPE]);
        if (frame >= codes.length) throw new RangeError('주변 표면 후보 프레임 오류');
        const cx = Math.fround(x - type.footX * 0.5);
        const cy = Math.fround(y - type.footY * 0.5);
        const direction = RawSquidNeighbors.direction(cx, cy, this.#centerX, this.#centerY);
        const sourceCodes = this.#codes(this.#sourceType);
        if (this.#sourceFrame >= sourceCodes.length) throw new RangeError('주변 표면 현재 배치 프레임 오류');
        if (!bridgeConnects(codes[frame], type.flags2, sourceCodes[this.#sourceFrame], this.#types[this.#sourceType].flags2, direction)) return false;
        return (raw[STATE] & 2) === 0 && (bits & 8) === 0;
    }

    // 확장 사각형의 네 모서리를 제외하고 다음 지도 WORD 를 그 시점에 읽는다. 새 후보가 나오거나 마지막 행을 소진할 때까지 원본 커서를 진행한다. (2026-11-09 WB04-E151)
    next() {
        const state = this.#state;
        state.current = 0;
        if (this.#ended) return 0;
        for (;;) {
            state.x += 1;
            if (state.y === state.top || state.y === state.bottom) {
                if (state.x >= state.right) {
                    state.y += 1;
                    if (state.y > state.bottom) {
                        this.#ended = true;
                        return 0;
                    }
                    state.x = state.left;
                }
            } else if (state.x > state.right) {
                state.y += 1;
                state.x = state.left + (state.y === state.bottom ? 1 : 0);
            }
            if (state.x < 0 || state.y < 0 || state.x >= MAP_SIDE || state.y >= MAP_SIDE) continue;
            const candidate = this.#islands[state.y * MAP_SIDE + state.x] & 0xffff;
            if (candidate === 0) continue;
            if (candidate > LARGEST_SID) throw new RangeError('주변 표면 signed SID 범위 오류');
            const raw = this.#pool.slot(candidate);
            const x = floatAt(raw, X);
            const y = floatAt(raw, Y);
            if (this.#left <= x && x <= this.#right && this.#top <= y && y <= this.#bottom) continue;
            if (!this.#accept(candidate) || state.returned.includes(candidate)) continue;
            if (state.returned.length === CACHE_SIZE) throw new RangeError('주변 표면 중복 캐시 용량 오류');
            state.returned.push(candidate);
            state.current = candidate;
            return candidate;
        }
    }
}

// 배치 디코더의 현재 모양 주변 권한 계층이다. 지도/타입/프레임 자료는 호출자 소유이며 후보 권한 helper 와 같은 풀을 요구한다. (2026-11-09 WB04-E151)
export class RawCanonPlacementSurrounding {
    #pool;
    #types;
    #frames;
    #islands;
    #spots;
    #permission;

    // 같은 자료로 만든 권한 helper 를 확인한다. (2026-11-09 WB04-E151)
    constructor(pool, types, frames, islands, spots, permission) {
        if (pool !== permission?.pool) throw new TypeError('주변 표면/권한 SID 풀 불일치');
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#islands = islands;
        this.#spots = spots;
        this.#permission = permission;
    }

    // 지형 어댑터가 동일 풀을 확인할 수 있게 한다. (2026-11-09 WB04-E151)
    get pool() { return this.#pool; }

    // 바깥 관계 검사와 실제 권한 helper 를 모든 반환 표면에 적용하며 참을 누적한다. 처음 권한을 얻은 뒤에도 현재 모양의 표면 순회를 계속한다. (2026-11-09 WB04-E151)
    inspect(query, cell) {
        const walk = RawCanonSurfaceWalk.forPlacement(this.#pool, this.#types, this.#frames, this.#islands, this.#spots, query, cell);
        let result = false;
        const owner = ((query.owner << 24) >> 24) >>> 0;
        for (let candidate = walk.current; candidate !== 0; candidate = walk.next()) {
            if (this.#permission.surrounding(candidate, owner) !== 0) result = true;
        }
        return result;
    }
}

// 모양 종료에서 초기/후보 권한이 거짓일 때만 실제 주변 순회에 들어가도록 주변 권한 경계(shapePermission)만 바꾼다. 다른 훅은 유지한다. (2026-11-09 WB04-E151)
export function makeCanonSurroundingTerrainHooks(pool, surrounding, hooks = {}) {
    if (pool !== surrounding.pool) throw new TypeError('주변 표면/지형 SID 풀 불일치');
    return { ...hooks, shapePermission: (query, cell) => surrounding.inspect(query, cell) };
}
