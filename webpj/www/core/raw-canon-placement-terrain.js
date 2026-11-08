// 일반 타입/패턴 배치의 후보 지형·지역 배열·모양 종료·최종 지면 누적을 옮긴 모듈이다(cpppj `o/RawCanonPlacementTerrain.cpp/.h`, 10.78).
//  - createCanonPlacementTerrainState(): 지형 처리가 읽고 쓰는 상태 { regions(144바이트, 열 간격 12), emptyRegion(현재 sentinel DWORD, 기본 127), noIslandType(받침 타입 전역), bridgeOverlap,
//    noIslandOnly, groundComplete, permission, canPlaceGround } 를 원본 초기값으로 만든다.
//  - new RawCanonPlacementTerrain(pool, types, frames, islands, state, hooks): islands 는 지도 칸마다 지역 SID(uint16)를 담은 65536칸 표이다. hooks = { candidatePermission(query, sid),
//    shapePermission(query, cell), finishRelations(query, state) } 셋 모두 필수이다(권한/최종 관계의 실제 정책은 후속 단위가 공급한다).
//  - begin(query): 그룹 10·소유 genus 비트에 따른 초기 권한을 만들고 누적 상태를 초기화한다. beginShape(query, cell): 절삭 좌표−발자국+1 을 원점으로 잡고 지면 종류 마스크를 고정하며 배열을 sentinel 하위 BYTE 로 채운다.
//  - candidate(query, sid): 충돌에서 무시한 후보도 받는다. 다리 겹침과 섬 지역 쓰기(8×8 안, 열 바깥/행 안쪽 순서)와 조건부 후보 권한을 처리한다. endShape(query): 부호 있는 BYTE 와 sentinel 을 비교하고 현재 발자국을 검사한 뒤
//    필요하면 주변 권한을 조회한다. finish(query): 지면 허용·권한 철회를 계산하고 최종 관계 경계의 판정을 돌려준다. regionAt(x, y): 지도 칸의 지역 번호(섬 SID 의 WORD)이다.
//  - makeCanonTerrainGeometryHooks(pool, terrain, query): E146 기하 계층에 연결하는 훅 묶음이다(소유자/모드 등 원래 질의는 값으로 캡처). makeCanonTerrainCollisionHooks(pool, terrain, hooks): 후보 지형 효과만 덧붙인다.
// 10.72(CD)의 BYTE 프레임과 지역 getter 차이는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E148: 일반 배치 지형 이식으로 추가)

// 지역 배열의 열 간격, 후보 쓰기의 제한 폭(8×8), 배열 크기, 지도 한 변, 지도 밖/빈 칸의 지역 번호, 타입 raw 위치, 타입 표 크기이다. (2026-11-09 WB04-E148)
const PITCH = 12;
const PAINT_LIMIT = 8;
const REGION_BYTES = 144;
const MAP_SIDE = 256;
const OUTSIDE_REGION = 127;
const TYPE = 10;
const TYPE_COUNT = 188;
const INT_MIN = -2147483648;
const INT_MAX = 2147483647;

// 지역 처리 플래그: 다리 genus(4), 섬 genus(2), 지역 지도에 올 수 있는 genus(0x1000006), 배치 대상의 권한/다리 겹침 genus 비트이다. (2026-11-09 WB04-E148)
const GENUS_BRIDGE = 4;
const GENUS_ISLAND = 2;
const GENUS_REGION = 0x1000006;
const GENUS_PERMISSION = 0x10202000;
const GENUS_OVER_BRIDGE = 0x208100;
const FLAG_GROUND_MASK = 6;
const FLAG_NO_ISLAND_ONLY = 0x400;

// 지형 처리가 읽고 쓰는 상태를 원본 초기값으로 만든다. (2026-11-09 WB04-E148)
export function createCanonPlacementTerrainState() {
    return { regions: new Uint8Array(REGION_BYTES), emptyRegion: OUTSIDE_REGION, noIslandType: 0, bridgeOverlap: false, noIslandOnly: true, groundComplete: true, permission: false, canPlaceGround: false };
}

// 부호 있는 CRT 절삭을 안전한 정수 범위에서 수행한다. (2026-11-09 WB04-E148)
function truncate(value) {
    const result = Math.trunc(Math.fround(value));
    if (!Number.isFinite(result) || result < INT_MIN || result > INT_MAX) throw new RangeError('일반 지형 좌표 절삭 오류');
    return result;
}

// 슬롯의 리틀엔디언 값을 읽는다(width 바이트, 기본 DWORD). (2026-11-09 WB04-E148)
function read(raw, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 합친다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// DWORD 비트를 단정도 float 로 해석한다. (2026-11-09 WB04-E148)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 원본 movsx BYTE 와 DWORD 비교의 비트값(부호 확장 뒤 uint32)을 보존한다. (2026-11-09 WB04-E148)
function signedByte(value) { return ((value << 24) >> 24) >>> 0; }

// 일반 타입/패턴의 후보 지형·지역 배열·모양 종료와 최종 지면 누적 계층이다. 표들의 수명은 호출자가 보장하며 값은 호출 때마다 다시 읽는다. (2026-11-09 WB04-E148)
export class RawCanonPlacementTerrain {
    #pool;
    #types;
    #frames;
    #islands;
    #state;
    #hooks;
    #cell = null;
    #originX = 0;
    #originY = 0;
    #groundMask = 0;
    #activeType = 0;
    #active = false;
    #shape = false;

    // 타입 표와 독립 지역 SID 지도의 크기를 확인하고 세 훅을 필수로 요구한다. (2026-11-09 WB04-E148)
    constructor(pool, types, frames, islands, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('일반 지형은 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(frames) || frames.length !== types.length || !islands || islands.length !== MAP_SIDE * MAP_SIDE
            || !state || !(state.regions instanceof Uint8Array) || state.regions.length !== REGION_BYTES
            || typeof hooks?.candidatePermission !== 'function' || typeof hooks.shapePermission !== 'function' || typeof hooks.finishRelations !== 'function') {
            throw new TypeError('일반 지형 자료 크기 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#islands = islands;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 같은 풀을 사용하는 어댑터만 연결하도록 원래 풀을 제공한다. (2026-11-09 WB04-E148)
    get pool() { return this.#pool; }

    // 0 발자국도 허용하되 원본 배열의 물리 범위를 넘는 입력은 거부한다. genus 는 제한하지 않는다. (2026-11-09 WB04-E148)
    #placing(query) {
        if (!Number.isInteger(query.type) || query.type < 0 || query.type >= this.#types.length) throw new RangeError('일반 지형 타입 오류');
        const type = this.#types[query.type];
        if (type.footX < 0 || type.footX > PITCH || type.footY < 0 || type.footY > PITCH) throw new RangeError('일반 지형 발자국 배열 범위 오류');
        return type;
    }

    // 활성 순회의 타입/모양 준비 순서를 확인한다. (2026-11-09 WB04-E148)
    #require(query, shape) {
        if (!this.#active || query.type !== this.#activeType || (shape && !this.#shape)) throw new Error('일반 지형 순회 순서 오류');
    }

    // 유효 모양이 없으면 원본처럼 이전 지역 배열을 지우지 않는다. 권한은 그룹 10 이거나 권한 genus 가 있고 다리 genus 가 아닐 때 먼저 허용된다. (2026-11-09 WB04-E148)
    begin(query) {
        const type = this.#placing(query);
        this.#active = true;
        this.#shape = false;
        this.#activeType = query.type;
        const state = this.#state;
        state.bridgeOverlap = false;
        state.noIslandOnly = true;
        state.groundComplete = true;
        state.permission = (type.group === 10 || (type.flags2 & GENUS_PERMISSION) !== 0) && (type.flags2 & GENUS_BRIDGE) === 0;
        state.canPlaceGround = false;
    }

    // 절삭 좌표를 기준으로 배열의 왼쪽 위 원점을 만들며 지면 종류(마스크)는 후보 순회 동안 고정한다. 배열은 sentinel 하위 BYTE 로 채운다. (2026-11-09 WB04-E148)
    beginShape(query, cell) {
        this.#require(query, false);
        const type = this.#placing(query);
        this.#cell = cell;
        this.#originX = truncate(cell.snappedX) - type.footX + 1;
        this.#originY = truncate(cell.snappedY) - type.footY + 1;
        this.#groundMask = type.flags1 & FLAG_GROUND_MASK;
        this.#state.regions.fill(this.#state.emptyRegion & 0xff);
        this.#shape = true;
    }

    // 지도 번호는 uint16 SID 이다. 지도 밖/빈 칸은 127, 다리 genus 는 현재 sentinel, 그 밖의 섬은 raw +8 WORD 를 돌려준다. 지역 genus 가 아닌 대상은 10.78 에서 거부한다. (2026-11-09 WB04-E148)
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

    // 충돌에서 무시한 후보도 다리 겹침과 섬 지역 쓰기, 조건부 권한 조회를 순서대로 받는다. (2026-11-09 WB04-E148)
    candidate(query, sid) {
        this.#require(query, true);
        const placing = this.#placing(query);
        const raw = this.#pool.slot(sid);
        if (raw[TYPE] >= this.#types.length) throw new RangeError('일반 지형 후보 타입 오류');
        const other = this.#types[raw[TYPE]];
        const state = this.#state;
        if ((other.flags2 & GENUS_BRIDGE) !== 0 && (placing.flags2 & GENUS_OVER_BRIDGE) !== 0 && (raw[40] & 1) === 0) {
            if (placing.footX !== placing.footY) throw new RangeError('다리 위 배치 발자국 폭/높이 불일치');
            state.bridgeOverlap = true;
        }
        if ((other.flags2 & GENUS_ISLAND) === 0) return;
        if (raw[TYPE] !== (state.noIslandType >>> 0)) state.noIslandOnly = false;
        const frame = read(raw, 36) | 0;
        const codes = this.#frames[raw[TYPE]].frames;
        if (frame < 0 || frame >= codes.length) throw new RangeError('일반 지형 후보 프레임 오류');
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
        if (!state.permission && this.#hooks.candidatePermission(query, sid)) state.permission = true;
    }

    // 한 모양에서 거짓이 된 지면 완성 여부는 후속 모양에서 참으로 되돌리지 않는다. 권한이 없으면 주변 관계 조회를 요청한다. (2026-11-09 WB04-E148)
    endShape(query) {
        this.#require(query, true);
        const type = this.#placing(query);
        const state = this.#state;
        const first = state.regions[0];
        if (signedByte(first) === (state.emptyRegion >>> 0)) state.groundComplete = false;
        else {
            // 배치 발자국 전체를 검사하며 후보의 8칸 쓰기 제한과 구분한다.
            for (let x = 0; x < type.footX; x += 1) {
                // 각 열의 BYTE 지역 번호가 첫 번호와 같은지 검사한다.
                for (let y = 0; y < type.footY; y += 1) if (state.regions[x * PITCH + y] !== first) state.groundComplete = false;
            }
        }
        if (!state.permission && this.#hooks.shapePermission(query, this.#cell)) state.permission = true;
        this.#shape = false;
        return true;
    }

    // 최종 관계 진입 전의 지면/권한 효과를 계산한다. 받침 전용 타입이 섬과 겹치면서 지면이 불완전하면 권한을 철회하고, 허용 결과는 필수 최종 관계 경계가 결정한다. (2026-11-09 WB04-E148)
    finish(query) {
        this.#require(query, false);
        const type = this.#placing(query);
        const state = this.#state;
        const noIsland = (type.flags1 & FLAG_NO_ISLAND_ONLY) !== 0;
        state.canPlaceGround = (type.flags2 & GENUS_BRIDGE) !== 0 || (noIsland && state.noIslandOnly) || state.groundComplete || state.bridgeOverlap;
        if (noIsland && !state.noIslandOnly && !state.groundComplete) state.permission = false;
        this.#active = false;
        return !!this.#hooks.finishRelations(query, state);
    }
}

// 타입/패턴 인자와 별개인 원래 소유자/모드를 모든 지형 단계에 유지하는 E146 기하 훅 묶음이다. 질의는 값으로 캡처한다. (2026-11-09 WB04-E148)
export function makeCanonTerrainGeometryHooks(pool, terrain, query) {
    if (pool !== terrain.pool) throw new TypeError('일반 지형/모양 SID 풀 불일치');
    query = { ...query };
    return {
        beginRegions: () => { terrain.begin(query); },
        beginShape: (_geometryQuery, cell) => { terrain.beginShape(query, cell); },
        endShape: () => terrain.endShape(query),
        finishRegions: () => terrain.finish(query)
    };
}

// 후보 지형 효과(inspectCandidateTerrain)만 덧붙이고 기존 실제 기하/finder 순회 훅은 유지한다. (2026-11-09 WB04-E148)
export function makeCanonTerrainCollisionHooks(pool, terrain, hooks = {}) {
    if (pool !== terrain.pool) throw new TypeError('일반 지형/충돌 SID 풀 불일치');
    return { ...hooks, inspectCandidateTerrain: (query, sid) => { terrain.candidate(query, sid); } };
}
