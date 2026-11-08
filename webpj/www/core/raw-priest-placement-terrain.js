// 일반 사제 배치의 후보별 다리·섬·지역 배열과 모양 종료 효과를 옮긴 모듈이다(cpppj `o/RawPriestPlacementTerrain.cpp/.h`, 10.78). 원본 0049bb1a 이후의 사제 지형 효과이며 일반 타입 지형(E148)과 달리 권한 훅이 없다.
//  - createPriestPlacementTerrainState(): { regions(144바이트, 열 간격 12, 후보 쓰기는 8×8), emptyRegion(현재 지역 sentinel DWORD, 기본 127), noIslandType(noIsland 타입 전역), bridgeOverlap, noIslandOnly(참), groundComplete(참),
//    permission(참 — 일반 사제는 처음부터 허용), canPlaceGround } 를 원본 초기값으로 만든다.
//  - new RawPriestPlacementTerrain(pool, types, frames, islands, state): islands 는 지형 SID 지도(65536칸 uint16, 미리보기의 표면 SID 지도와 구분). 사제 genus(0x200000)이고 다리(4)가 아니며 10.78 의 특수 지역(0x200)이 아닌 타입, 발자국 1..12 만 받는다.
//  - begin(query): 누적 상태를 초기화한다(지역 배열은 유지). beginShape(query, x, y): 절삭한 (x, y)−발자국+1 을 배열 원점으로 잡고 지면 마스크(flags1 & 6)를 고정하며 배열을 sentinel 하위 BYTE 로 채운다.
//    candidate(query, sid): 충돌을 통과/무시한 모든 후보에 다리 겹침과 섬 지역 쓰기를 적용한다. endShape(query): 첫 signed BYTE 와 sentinel 비교 및 발자국 전체 동일성으로 지면 완성 여부를 누적한다(후속 모양이 참으로 되돌리지 않음).
//    finish(query): 지면 허용(받침 전용·지면 완성·다리 겹침)을 계산하고, 받침 전용 타입이 섬과 겹치면서 지면이 불완전하면 권한을 철회한다. 반환은 지면/권한 결과와 무관하게 참이다(사제의 최종 지면 우회). regionAt(x, y): 지도 칸의 지역 번호이다.
//  - makePriestTerrainGeometryHooks(pool, terrain): 실제 지역 초기화/모양 시작/모양 종료/최종 계산을 사제 기하에 연결한다. makePriestTerrainCollisionHooks(pool, terrain, hooks): 후보 지형 효과(inspectCandidateTerrain)만 연결한다.
// 10.72(CD)의 BYTE 프레임과 지역 getter 차이는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E158: 사제 배치 지형 이식으로 추가)

// 지역 배열의 열 간격, 후보 쓰기의 제한 폭(8×8), 배열 크기, 지도 한 변, 지도 밖/빈 칸의 지역 번호, 슬롯 타입 위치, 타입 표 크기, int32 범위이다. (2026-11-09 WB04-E158)
const PITCH = 12;
const PAINT_LIMIT = 8;
const REGION_BYTES = 144;
const MAP_SIDE = 256;
const OUTSIDE_REGION = 127;
const TYPE = 10;
const TYPE_COUNT = 188;
const INT_MIN = -2147483648;
const INT_MAX = 2147483647;

// 타입 genus/flags 비트: 사제(0x200000)·다리(4)·섬(2)·지역 지도 허용(0x1000006)·10.78 특수 지역(0x200)·다리 위 배치(0x208100), 지면 마스크(6)·받침 전용(0x400)이다. (2026-11-09 WB04-E158)
const GENUS_PRIEST = 0x200000;
const GENUS_BRIDGE = 4;
const GENUS_ISLAND = 2;
const GENUS_REGION = 0x1000006;
const GENUS_SPECIAL_REGION = 0x200;
const GENUS_OVER_BRIDGE = 0x208100;
const FLAG_GROUND_MASK = 6;
const FLAG_NO_ISLAND_ONLY = 0x400;

// 사제 지형 처리가 읽고 쓰는 상태를 원본 초기값으로 만든다. (2026-11-09 WB04-E158)
export function createPriestPlacementTerrainState() {
    return { regions: new Uint8Array(REGION_BYTES), emptyRegion: OUTSIDE_REGION, noIslandType: 0, bridgeOverlap: false, noIslandOnly: true, groundComplete: true, permission: true, canPlaceGround: false };
}

// 부호 있는 CRT 절삭을 안전한 정수 범위에서 수행한다. (2026-11-09 WB04-E158)
function truncate(value) {
    const result = Math.trunc(Math.fround(value));
    if (!Number.isFinite(result) || result < INT_MIN || result > INT_MAX) throw new RangeError('사제 지형 좌표 절삭 오류');
    return result;
}

// 슬롯의 리틀엔디언 값을 읽는다(width 바이트, 기본 DWORD). (2026-11-09 WB04-E158)
function read(raw, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 합친다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// DWORD 비트를 단정도 float 로 해석한다. (2026-11-09 WB04-E158)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 원본 movsx BYTE 와 DWORD 비교의 비트값(부호 확장 뒤 uint32)을 보존한다. (2026-11-09 WB04-E158)
function signedByte(value) { return ((value << 24) >> 24) >>> 0; }

// 일반 사제 배치의 지형 계층이다. 표들의 수명은 호출자가 보장하며 값은 호출 때마다 다시 읽는다. (2026-11-09 WB04-E158)
export class RawPriestPlacementTerrain {
    #pool;
    #types;
    #frames;
    #islands;
    #state;
    #originX = 0;
    #originY = 0;
    #groundMask = 0;
    #activeType = 0;
    #active = false;
    #shape = false;

    // 타입 표와 독립 지역 SID 지도의 물리 크기를 확인한다. (2026-11-09 WB04-E158)
    constructor(pool, types, frames, islands, state) {
        if (pool?.edition !== 'patch1078') throw new TypeError('사제 지형은 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(frames) || frames.length !== types.length || !islands || islands.length !== MAP_SIDE * MAP_SIDE
            || !state || !(state.regions instanceof Uint8Array) || state.regions.length !== REGION_BYTES) {
            throw new TypeError('사제 지형 자료 크기 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#islands = islands;
        this.#state = state;
    }

    // 같은 풀을 사용하는 모양/충돌 어댑터만 연결하도록 원래 풀을 제공한다. (2026-11-09 WB04-E158)
    get pool() { return this.#pool; }

    // 일반 사제는 처음부터 permission=참이다. 다리 및 10.78 특수 지역 helper 조합과 배열 밖 발자국은 지원 범위 밖으로 진단한다. (2026-11-09 WB04-E158)
    #placing(query) {
        if (!Number.isInteger(query.type) || query.type < 0 || query.type >= this.#types.length) throw new RangeError('사제 지형 타입 오류');
        const type = this.#types[query.type];
        if ((type.flags2 & GENUS_PRIEST) === 0 || (type.flags2 & GENUS_BRIDGE) !== 0 || (type.flags2 & GENUS_SPECIAL_REGION) !== 0) throw new TypeError('일반 사제 지형 타입 계약 오류');
        if (type.footX < 1 || type.footX > PITCH || type.footY < 1 || type.footY > PITCH) throw new RangeError('사제 지형 발자국 배열 범위 오류');
        return type;
    }

    // 동일 순회의 타입과 모양 준비 상태를 확인한다. (2026-11-09 WB04-E158)
    #require(query, shape) {
        if (!this.#active || query.type !== this.#activeType || (shape && !this.#shape)) throw new Error('사제 지형 순회 순서 오류');
    }

    // 일반 사제의 초기 permission 과 모양 사이에 유지하는 누적 상태를 초기화한다. 유효 모양이 없으면 원본처럼 이전 지역 배열을 지우지 않는다. (2026-11-09 WB04-E158)
    begin(query) {
        this.#placing(query);
        this.#active = true;
        this.#shape = false;
        this.#activeType = query.type;
        const state = this.#state;
        state.bridgeOverlap = false;
        state.noIslandOnly = true;
        state.groundComplete = true;
        state.permission = true;
        state.canPlaceGround = false;
    }

    // finder 탐색 직전에 배열을 sentinel 하위 BYTE 로 채우고 발자국 원점/지면 마스크를 캡처한다. 지면 종류는 후보 순회 동안 고정한다. (2026-11-09 WB04-E158)
    beginShape(query, x, y) {
        this.#require(query, false);
        const type = this.#placing(query);
        this.#originX = truncate(x) - type.footX + 1;
        this.#originY = truncate(y) - type.footY + 1;
        this.#groundMask = type.flags1 & FLAG_GROUND_MASK;
        this.#state.regions.fill(this.#state.emptyRegion & 0xff);
        this.#shape = true;
    }

    // 지도 번호는 uint16 SID 이다. 지도 밖/빈 칸은 127, 다리 genus 는 현재 sentinel, 그 밖의 섬은 raw +8 WORD 를 돌려준다. 지역 genus 가 아닌 대상은 10.78 에서 거부한다. (2026-11-09 WB04-E158)
    regionAt(x, y) {
        if (x < 0 || y < 0 || x >= MAP_SIDE || y >= MAP_SIDE) return OUTSIDE_REGION;
        const sid = this.#islands[y * MAP_SIDE + x] & 0xffff;
        if (sid === 0) return OUTSIDE_REGION;
        const raw = this.#pool.slot(sid);
        if (raw[TYPE] >= this.#types.length) throw new RangeError('지역 지도 타입 오류');
        const genus = this.#types[raw[TYPE]].flags2;
        if ((genus & GENUS_REGION) === 0) throw new TypeError('지역 지도 자산 genus 오류');
        return (genus & GENUS_BRIDGE) !== 0 ? this.#state.emptyRegion >>> 0 : read(raw, 8, 2);
    }

    // 후보가 충돌에서 무시되었더라도 다리와 섬의 효과를 순서대로 적용한다. (2026-11-09 WB04-E158)
    candidate(query, sid) {
        this.#require(query, true);
        const placing = this.#placing(query);
        const raw = this.#pool.slot(sid);
        if (raw[TYPE] >= this.#types.length) throw new RangeError('사제 지형 후보 타입 오류');
        const other = this.#types[raw[TYPE]];
        const state = this.#state;
        if ((other.flags2 & GENUS_BRIDGE) !== 0 && (placing.flags2 & GENUS_OVER_BRIDGE) !== 0 && (raw[40] & 1) === 0) {
            if (placing.footX !== placing.footY) throw new RangeError('다리 위 사제 발자국 폭/높이 불일치');
            state.bridgeOverlap = true;
        }
        if ((other.flags2 & GENUS_ISLAND) === 0) return;
        if (raw[TYPE] !== (state.noIslandType >>> 0)) state.noIslandOnly = false;
        const frame = read(raw, 36) | 0;
        const codes = this.#frames[raw[TYPE]].frames;
        if (frame < 0 || frame >= codes.length) throw new RangeError('사제 지형 후보 프레임 오류');
        const surface = (codes[frame].flags & 4) !== 0 ? 4 : 2;
        if ((this.#groundMask & surface) === 0) return;
        const x = truncate(floatOf(read(raw, 14)));
        const y = truncate(floatOf(read(raw, 18)));
        const region = this.regionAt(x, y) & 0xff;
        // 배열 밖의 반복은 쓰기 효과가 없으므로 8×8 교집합만 순서대로 방문한다.
        const left = Math.max(0, x - other.footX - this.#originX + 1);
        const top = Math.max(0, y - other.footY - this.#originY + 1);
        const right = Math.min(PAINT_LIMIT - 1, x - this.#originX);
        const bottom = Math.min(PAINT_LIMIT - 1, y - this.#originY);
        // 원본 x 바깥/y 안쪽 순서와 열 간격 12를 보존한다.
        for (let cx = left; cx <= right; cx += 1) {
            // 해당 열의 지역 BYTE 만 덮어쓴다.
            for (let cy = top; cy <= bottom; cy += 1) state.regions[cx * PITCH + cy] = region;
        }
    }

    // 한 모양에서 거짓이 된 지면 완성 여부는 후속 모양에서 참으로 되돌리지 않는다. 배치 발자국 전체를 검사하며 후보의 8칸 쓰기 제한과 구분한다. (2026-11-09 WB04-E158)
    endShape(query) {
        this.#require(query, true);
        const type = this.#placing(query);
        const state = this.#state;
        const first = state.regions[0];
        if (signedByte(first) === (state.emptyRegion >>> 0)) state.groundComplete = false;
        else {
            for (let x = 0; x < type.footX; x += 1) {
                // 각 열의 BYTE 지역 번호가 첫 번호와 같은지 검사한다.
                for (let y = 0; y < type.footY; y += 1) if (state.regions[x * PITCH + y] !== first) state.groundComplete = false;
            }
        }
        this.#shape = false;
        return true;
    }

    // 일반 사제의 마지막 genus 분기는 지면/권한 결과에 관계없이 참을 반환하되 지면 허용/권한 철회 계산 효과는 보존한다. (2026-11-09 WB04-E158)
    finish(query) {
        this.#require(query, false);
        const type = this.#placing(query);
        const state = this.#state;
        const noIsland = (type.flags1 & FLAG_NO_ISLAND_ONLY) !== 0;
        state.canPlaceGround = (noIsland && state.noIslandOnly) || state.groundComplete || state.bridgeOverlap;
        if (noIsland && !state.noIslandOnly && !state.groundComplete) state.permission = false;
        this.#active = false;
        return true;
    }
}

// 모양 시작은 finder/후보 순회 전에 호출되어야 하므로 실제 지역 초기화/모양 시작/모양 종료/최종 계산을 사제 기하의 네 경계에 연결한다. (2026-11-09 WB04-E158)
export function makePriestTerrainGeometryHooks(pool, terrain) {
    if (pool !== terrain.pool) throw new TypeError('사제 지형/모양 SID 풀 불일치');
    return {
        beginRegions: (query) => { terrain.begin(query); },
        endShape: (query) => terrain.endShape(query),
        finishRegions: (query) => terrain.finish(query),
        beginShape: (query, _frame, x, y) => { terrain.beginShape(query, x, y); }
    };
}

// 후보 지형 효과(inspectCandidateTerrain)만 대체하고 기존 모양 순회를 유지한다. (2026-11-09 WB04-E158)
export function makePriestTerrainCollisionHooks(pool, terrain, hooks = {}) {
    if (pool !== terrain.pool) throw new TypeError('사제 지형/충돌 SID 풀 불일치');
    return { ...hooks, inspectCandidateTerrain: (query, sid) => { terrain.candidate(query, sid); } };
}
