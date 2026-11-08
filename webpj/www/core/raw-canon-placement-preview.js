// 일반 타입/패턴 배치의 로컬 미리보기 배열·표면 조회·소유 관계 판정을 옮긴 모듈이다(cpppj `o/RawCanonPlacementPreview.cpp/.h`, 10.78).
//  - createCanonPlacementPreviewState(): { blocked(144바이트, x 인덱스가 바깥인 12×12 배열, 0059a9f0), editor(005c85a4, 표면 소유 관계를 모두 허용), useAlliances(00540cb0, 소유자×9+요청자의 관계 표 사용 조건),
//    alliances(81칸 DWORD 관계 표, 00595200, 행 소유자→열 요청자 방향) } 를 원본 초기값으로 만든다.
//  - new RawCanonPlacementPreview(pool, types, spots, surfaceMap, state, hooks): spots 는 칸별 점유 BYTE 65536칸, surfaceMap 은 표면 SID 65536칸이며 hooks = { bounds(query) → 정수 사각형, inspectCollisions(query, localOwner) → bool } 둘 다 필수이다.
//  - inspect(query, localOwner): 비로컬 요청은 배열을 지우거나 모양을 조회하지 않고 곧바로 충돌 경계로 간다. 로컬 요청은 배열을 지우고 모양 범위를 조회해 왼쪽/위를 1 로 올린 뒤, 오른쪽 아래+1 에서 왼쪽 위−1 까지 y 를 먼저 줄이고
//    각 행에서 x 를 줄이며 칸을 검사한다(10.78 은 지도 밖이거나 y 가 254 이상이면 배열을 부분만 쓴 채 즉시 거짓). 칸이 막혔으면(spot 0x10, 지도 밖, 표면 없음, 표면 genus 2 이면서 소유 관계 없음) 배열 [x 번호×12 + y 번호]=1 이다.
//    소유 관계는 편집기이거나 같은 소유자이거나 방향 동맹 표(표면 소유자×9+부호 확장한 요청자)이다. 마지막에 충돌 경계가 결정한다.
//  - makeCanonPlacementPreviewHooks(pool, preview, hooks): 배치 접두의 inspectGeometry 만 실제 미리보기로 연결한다. makeCanonGeometryPreviewHooks(pool, geometry, hooks): 모양 범위(bounds)만 E146 기하 모듈로 연결한다.
// 10.72(CD)의 선행 linear spot 읽기와 y=254 조기 거부 부재는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E152: 일반 배치 미리보기 이식으로 추가)

// 지도/미리보기 한 변, 10.78 이 y 로 거부하는 행, 표면 genus, 슬롯 타입/소유자 위치, 관계 표 칸 수와 소유자 열 폭, 타입 표 크기, int32 범위이다. (2026-11-09 WB04-E152)
const MAP_SIDE = 256;
const PREVIEW_SIDE = 12;
const PATCH_ROW_END = 254;
const GENUS_SURFACE = 2;
const TYPE = 10;
const OWNER = 34;
const RELATION_COUNT = 81;
const OWNER_COLUMNS = 9;
const TYPE_COUNT = 188;
const INT_MIN = -2147483648;
const INT_MAX = 2147483647;

// 미리보기가 읽고 쓰는 상태를 원본 초기값으로 만든다. (2026-11-09 WB04-E152)
export function createCanonPlacementPreviewState() {
    return { blocked: new Uint8Array(PREVIEW_SIDE * PREVIEW_SIDE), editor: 0, useAlliances: 0, alliances: new Uint32Array(RELATION_COUNT) };
}

// 반복 범위 +1/−1 과 미리보기 인덱스 산술을 정의할 수 없는 입력은 진단한다. (2026-11-09 WB04-E152)
function checkBounds(area) {
    if (area.right === INT_MAX || area.bottom === INT_MAX || area.right < INT_MIN + PREVIEW_SIDE || area.bottom < INT_MIN + PREVIEW_SIDE) throw new RangeError('일반 배치 미리보기 사각형 산술 범위 오류');
}

// 일반 타입/패턴 배치의 로컬 미리보기 계층이다. 고정 표면 SID 지도·공간 BYTE·타입·상태 참조는 이 객체보다 오래 살아야 한다. (2026-11-09 WB04-E152)
export class RawCanonPlacementPreview {
    #pool;
    #types;
    #spots;
    #surfaceMap;
    #state;
    #hooks;

    // 타입 표와 두 256×256 지도, 필수 하위 경계를 확인한다. (2026-11-09 WB04-E152)
    constructor(pool, types, spots, surfaceMap, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('일반 배치 미리보기는 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !spots || spots.length !== MAP_SIDE * MAP_SIDE || !surfaceMap || surfaceMap.length !== spots.length || !state
            || !(state.blocked instanceof Uint8Array) || typeof hooks?.bounds !== 'function' || typeof hooks.inspectCollisions !== 'function') {
            throw new TypeError('일반 배치 미리보기 자료/하위 경계 누락');
        }
        this.#pool = pool;
        this.#types = types;
        this.#spots = spots;
        this.#surfaceMap = surfaceMap;
        this.#state = state;
        this.#hooks = { bounds: hooks.bounds, inspectCollisions: hooks.inspectCollisions };
    }

    // 배치 접두와 같은 풀인지 검사할 수 있도록 원래 풀을 제공한다. (2026-11-09 WB04-E152)
    get pool() { return this.#pool; }

    // 원본 함수는 요청자의 BYTE 를 부호 확장하지만 표면 소유자는 부호 없이 읽는다. 요청자 −1 도 표면 소유자 1 과 조합하면 인덱스 8 이므로 실제 인덱스를 검사한다. (2026-11-09 WB04-E152)
    #related(surfaceOwner, owner) {
        const state = this.#state;
        if (state.editor || surfaceOwner === owner) return true;
        if (!state.useAlliances) return false;
        const index = surfaceOwner * OWNER_COLUMNS + owner;
        if (index < 0 || index >= state.alliances.length) throw new RangeError('일반 배치 미리보기 관계 표 범위 오류');
        return state.alliances[index] !== 0;
    }

    // 한 칸이 막혔는지 본다. 지도 선형 범위 → spot 0x10 → 지도 밖 → 표면 SID 0 → 표면 타입 genus → 소유 관계 순서이며 일반 자산의 free/dead 상태로 추가 필터링하지 않는다. (2026-11-09 WB04-E152)
    #cellBlocked(x, y, owner) {
        const index = y * MAP_SIDE + x;
        if (index < 0 || index >= this.#spots.length) throw new RangeError('일반 배치 미리보기 공간 읽기 범위 오류');
        if ((this.#spots[index] & 0x10) !== 0) return true;
        if (x < 0 || x >= MAP_SIDE || y < 0 || y >= MAP_SIDE) return true;
        const surface = this.#surfaceMap[index] & 0xffff;
        if (surface === 0) return true;
        const raw = this.#pool.slot(surface);
        const type = raw[TYPE];
        if (type >= this.#types.length) throw new RangeError('일반 배치 미리보기 표면 타입 범위 오류');
        if ((this.#types[type].flags2 & GENUS_SURFACE) === 0) return false;
        return !this.#related(raw[OWNER], owner);
    }

    // 원본 접두가 캡처한 로컬 비교를 사용한다. 비로컬 요청은 배열을 지우거나 모양을 조회하지 않는다. y 를 먼저 감소시키고 각 행에서 x 를 감소시키며 배열 인덱스는 x 쪽이 바깥 차원이다. (2026-11-09 WB04-E152)
    inspect(query, localOwner) {
        if (!localOwner) return !!this.#hooks.inspectCollisions(query, localOwner);
        const state = this.#state;
        state.blocked.fill(0);
        const area = { ...this.#hooks.bounds(query) };
        checkBounds(area);
        area.left = Math.max(area.left, 1);
        area.top = Math.max(area.top, 1);
        // 비어 있는 축에는 지도/배열 효과가 없다. 다른 축이 길어도 불필요한 빈 반복을 수행하지 않는다.
        if (area.right + 1 < area.left - 1 || area.bottom + 1 < area.top - 1) return !!this.#hooks.inspectCollisions(query, localOwner);
        const owner = ((query.owner << 24) >> 24) | 0;
        // 오른쪽 아래+1 에서 왼쪽 위−1 까지 포함한다.
        for (let y = area.bottom + 1; y >= area.top - 1; y -= 1) {
            // 각 행에서 오른쪽 주변 칸부터 왼쪽 주변 칸까지 검사한다.
            for (let x = area.right + 1; x >= area.left - 1; x -= 1) {
                if (x < 0 || y < 0 || x >= MAP_SIDE || y >= PATCH_ROW_END) return false;
                const px = area.right - x + 1;
                const py = area.bottom - y + 1;
                if (px < 0 || py < 0 || px >= PREVIEW_SIDE || py >= PREVIEW_SIDE) throw new RangeError('일반 배치 미리보기 12×12 배열 범위 오류');
                if (this.#cellBlocked(x, y, owner)) state.blocked[px * PREVIEW_SIDE + py] = 1;
            }
        }
        return !!this.#hooks.inspectCollisions(query, localOwner);
    }
}

// 배치 접두의 inspectGeometry 만 실제 미리보기로 연결하고 모양 조회 훅은 유지한다. (2026-11-09 WB04-E152)
export function makeCanonPlacementPreviewHooks(pool, preview, hooks = {}) {
    if (pool !== preview.pool) throw new TypeError('일반 배치 미리보기/배치의 SID 풀이 다릅니다');
    return { ...hooks, inspectGeometry: (query, localOwner) => preview.inspect(query, localOwner) };
}

// 실제 타입/패턴 디코더의 정수 범위만 연결하고 충돌/지형 경계는 유지한다. 별도 패턴 인자와 방향의 비트를 보존하며 명시 프레임을 사용하지 않는다. (2026-11-09 WB04-E152)
export function makeCanonGeometryPreviewHooks(pool, geometry, hooks = {}) {
    if (pool !== geometry.pool) throw new TypeError('일반 모양/미리보기의 SID 풀이 다릅니다');
    return { ...hooks, bounds: (query) => geometry.bounds({ type: query.type, argument: query.argument | 0, direction: query.flags | 0, x: query.x, y: query.y, explicitFrame: false }) };
}
