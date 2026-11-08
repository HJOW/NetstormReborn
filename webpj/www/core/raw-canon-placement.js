// 일반 타입/패턴 배치(MayPlace 0049b510)의 초기화·즉시 허용·픽셀 여백·지도 경계 접두를 옮긴 모듈이다(cpppj `o/RawCanonPlacement.cpp/.h`, 10.78).
//  - createPlacementState(): 배치 검사가 공유하는 전역 { forcePlacement(0055a488, 모든 배치를 즉시 허용하는 DWORD), localPlayer(00540c70, 부호 있는 소유자와 비교하는 값), blockedRelation(0059ab2c, 검사 진입마다 0 으로 지움),
//    sharedForcePlacement(실제 Client 의 55a488 을 빌리는 getter 또는 null) } 를 원본 초기값으로 만든다. currentForcePlacement(state): 즉시 허용 관문에서 현재 전체 DWORD 를 읽는다(공유 getter 가 있으면 그 값).
//  - new RawCanonPlacement(pool, types, state, hooks).mayPlace(query): query = { type, argument, x, y, flags, owner, mode }(요청은 값으로 복사). 타입 70 이상이어야 하고 좌표가 유한해야 한다.
//    blockedRelation 을 먼저 0 으로 지우고, 강제 허용 전역이거나 타입 genus 에 즉시 허용 비트(0x02000000)가 있으면 참이다. 아니면 요청 소유자의 하위 BYTE 를 부호 확장해 localPlayer 와 비교(모양 조회 전에 값으로 캡처)하고,
//    hooks.shape(type, argument, flags)가 주는 사각형으로 지도 여백을 구한다: 가로 (오른쪽−왼쪽)×0.0625, 세로 (아래−위)×0.09090909361839294(단정도 저장)의 큰 값을 정수로 절삭한 값이다.
//    x 나 y 가 여백보다 작거나 지도 한 변(256)에서 뺀 값이 여백보다 작으면 거짓이고, 아니면 hooks.inspectGeometry(query, localOwner)가 결정한다.
//  - makeCanonShapePlacementHooks(pool, shape, hooks): 공통 모양 경계만 E147 픽셀 getter(전체 패턴, 명시 프레임 없음)로 교체한다.
// 10.72(CD)의 중간 단정도 저장 여백은 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E152: 일반 배치 접두 이식으로 추가)

// 즉시 허용 genus 비트, 가로/세로 픽셀 환산 계수(단정도), 지도 한 변, 첫 자산 타입 번호, 타입 표 크기, int32 범위이다. (2026-11-09 WB04-E152)
const ANYWHERE = 0x02000000;
const HORIZONTAL_SCALE = Math.fround(0.0625);
const VERTICAL_SCALE = Math.fround(0.09090909361839294);
const MAP_SIDE = 256;
const FIRST_ASSET_TYPE = 70;
const TYPE_COUNT = 188;
const INT_MIN = -2147483648;
const INT_MAX = 2147483647;

// 배치 검사가 공유하는 전역을 원본 초기값으로 만든다(일반 배치와 사제 배치가 같은 구조를 쓴다). (2026-11-09 WB04-E152)
export function createPlacementState() {
    return { forcePlacement: 0, localPlayer: 0, blockedRelation: 0, sharedForcePlacement: null };
}

// 즉시 허용 관문에서 현재 전체 DWORD 를 읽는다. 공유 getter 가 있으면 그 값을, 없으면 독립 입력의 값을 쓴다. (2026-11-09 WB04-E152)
export function currentForcePlacement(state) {
    return (typeof state.sharedForcePlacement === 'function' ? state.sharedForcePlacement() : state.forcePlacement) >>> 0;
}

// 원본 CRT 가 정의하지 못하는 입력을 정수 변환 전에 진단한다. (2026-11-09 WB04-E152)
function truncate(value) {
    if (!Number.isFinite(value) || value < INT_MIN || value > INT_MAX) throw new RangeError('일반 배치 여백의 정수 범위 오류');
    return Math.trunc(value);
}

// 10.78 은 가로 차의 정밀도를 유지하고 세로 차만 단정도로 저장한다. 큰 값을 정수로 절삭해 단정도 여백으로 돌려준다. (2026-11-09 WB04-E152)
function margin(rect) {
    if (!Number.isFinite(rect.left) || !Number.isFinite(rect.top) || !Number.isFinite(rect.right) || !Number.isFinite(rect.bottom)) throw new TypeError('일반 배치 모양의 비유한 좌표');
    const horizontal = (Math.fround(rect.right) - Math.fround(rect.left)) * HORIZONTAL_SCALE;
    const vertical = Math.fround((Math.fround(rect.bottom) - Math.fround(rect.top)) * VERTICAL_SCALE);
    return Math.fround(truncate(horizontal > vertical ? horizontal : vertical));
}

// 일반 타입/패턴 배치 접두이다. 자료/상태/경계의 수명은 호출자가 보장한다. (2026-11-09 WB04-E152)
export class RawCanonPlacement {
    #pool;
    #types;
    #state;
    #hooks;

    // 타입 표 크기와 필수 모양/후반 판정 경계를 확인한다. (2026-11-09 WB04-E152)
    constructor(pool, types, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('일반 배치는 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !state || typeof hooks?.shape !== 'function' || typeof hooks.inspectGeometry !== 'function') throw new TypeError('일반 배치 자료/하위 경계 누락');
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#hooks = { shape: hooks.shape, inspectGeometry: hooks.inspectGeometry };
    }

    // 같은 풀에 실제 픽셀 getter 를 연결하는지 검사할 수 있도록 원래 풀을 제공한다. (2026-11-09 WB04-E152)
    get pool() { return this.#pool; }

    // 전역 차단값을 먼저 지우고 현재 허용 조건 → 캡처한 소유자 비교 → 모양 여백 → 지도 경계 순서로 처리한다. 요청은 값으로 복사한다. (2026-11-09 WB04-E152)
    mayPlace(query) {
        query = { ...query, x: Math.fround(query.x), y: Math.fround(query.y) };
        if (!Number.isInteger(query.type) || query.type < FIRST_ASSET_TYPE || query.type >= this.#types.length) throw new RangeError('일반 배치 타입 범위 오류');
        if (!Number.isFinite(query.x) || !Number.isFinite(query.y)) throw new TypeError('일반 배치의 비유한 지도 좌표');
        const state = this.#state;
        state.blockedRelation = 0;
        if (currentForcePlacement(state) !== 0 || (this.#types[query.type].flags2 & ANYWHERE) !== 0) return true;
        const owner = ((query.owner << 24) >> 24) | 0;
        const localOwner = owner === (state.localPlayer | 0);
        const edge = margin(this.#hooks.shape(query.type, query.argument, query.flags));
        if (query.x < edge || MAP_SIDE - query.x < edge || query.y < edge || MAP_SIDE - query.y < edge) return false;
        return !!this.#hooks.inspectGeometry({ ...query }, localOwner);
    }
}

// 공통 모양 경계만 실제 전체 픽셀 getter 로 교체한다. 패턴 argument 와 방향은 원본 값을 그대로 넘기고 명시 프레임은 켜지 않으며, 후반 정책은 호출자가 공급한다. (2026-11-09 WB04-E152)
export function makeCanonShapePlacementHooks(pool, shape, hooks = {}) {
    if (pool !== shape.pool) throw new TypeError('일반 픽셀 모양/배치의 SID 풀이 다릅니다');
    return { ...hooks, shape: (type, argument, direction) => shape.measure(type, argument, direction).bounds };
}
