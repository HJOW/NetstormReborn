// 일반 타입/패턴 배치(MayPlace) 후반의 현재 표면 소유 관계·특수 섬 지역·최종 거부 조건을 옮긴 모듈이다(cpppj `o/RawCanonPlacementRelations.cpp/.h`, 10.78).
//  - createCanonPlacementRelationsState(): { bypassGroundPermission(0059ab30, 지면/권한만 우회), islandCount(IslandList 00568b60 의 부호 있는 개수 DWORD) } 를 원본 초기값(0)으로 만든다.
//  - new RawCanonPlacementRelations(pool, types, surfaces, islands, state, placement, permission): surfaces 는 현재 표면 지도(65536칸 uint16 SID, 지형 후보의 고정 섬 지도와 별개),
//    islands 는 특수 지역 표({ restricted, exists } 배열), placement 는 표시 차단 전역({ blockedRelation })을 가진 객체, permission 은 E149 권한 helper(같은 풀)이다.
//  - inspect(query, terrain, priorRejection = 0): 지형 누적(E148 finish)이 끝난 뒤 호출하는 원본 후반이다.
//    ① 표시 차단 genus(0x34000, 단 0x200000 제외)이면 현재 표면 소유 관계를 검사해 내부 거부값(relationRejected)을 정하고 표시 차단 전역(blockedRelation)에 쓰되, 0x4200 이면 0 으로 되돌린다.
//    ② 10.78 은 지면이 완성이고 특수 genus(0x200)이면 지역 배열의 첫 BYTE(부호 확장)가 sentinel 이 아니고 제한된 지역이면 지면 허용을 취소한다. ③ 0x200000 은 즉시 허용한다.
//    ④ 다리 genus 가 아니고 유효 표면의 소유자가 있어 동맹이 아니고 0x20a000 genus 가 아니면 최종 소유자 거부(foreignRejected=1)이다. priorRejection 은 원본 함수 진입에서 0 인 지역 변수이며 foreignRejected 의 초기값이다.
//    ⑤ 허용 = (지면/권한 우회 또는 권한과 지면 허용) 이고 내부 거부와 소유자 거부가 없음. 반환은 { allowed, relationRejected, foreignRejected, priorRejection } 이다.
//  - surfaceAt(x, y): 0040eaf0 의 원래 좌표+0.9999 절삭 뒤 현재 표면 지도 WORD(지도 밖은 0)이다. makeCanonRelationsTerrainHooks(pool, relations, hooks): 지형 훅의 최종 관계 경계만 실제 구현으로 바꾼다.
// 10.72(CD)의 소유자 BYTE 위치·이전 거부 저장·특수 지역 분기는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E150: 일반 배치 최종 관계 이식으로 추가)

// 표면 좌표 보정(0.9999f)과 지도 한 변, 타입 표 크기, 소유자 BYTE 위치, int32 범위이다. (2026-11-09 WB04-E150)
const BIAS = Math.fround(0.9999);
const MAP_SIDE = 256;
const TYPE_COUNT = 188;
const OWNER = 34;
const INT_MIN = -2147483648;
const INT_MAX = 2147483647;

// 타입 genus 비트: 표시 차단 대상(0x34000)과 제외(0x200000), 다리 표시(0x4000)·초기화 마스크(0x4200), 특수 지역(0x200), 다리(4), 외부 소유자 면제(0x20a000)이다. (2026-11-09 WB04-E150)
const GENUS_DISPLAY_BLOCK = 0x34000;
const GENUS_ALWAYS_ALLOWED = 0x200000;
const GENUS_BRIDGE_DISPLAY = 0x4000;
const GENUS_CLEAR_BLOCK = 0x4200;
const GENUS_SPECIAL_REGION = 0x200;
const GENUS_BRIDGE = 4;
const GENUS_FOREIGN_EXEMPT = 0x20a000;

// 최종 관계 계층이 읽고 쓰는 전역 상태를 원본 초기값으로 만든다. (2026-11-09 WB04-E150)
export function createCanonPlacementRelationsState() {
    return { bypassGroundPermission: 0, islandCount: 0 };
}

// 0 방향 CRT 절삭을 재현하며 비정상 정수 변환은 진단한다. (2026-11-09 WB04-E150)
function coordinate(value) {
    const result = Math.trunc(Math.fround(value) + BIAS);
    if (!Number.isFinite(result) || result < INT_MIN || result > INT_MAX) throw new RangeError('최종 표면 좌표 절삭 오류');
    return result;
}

// 일반 타입/패턴 배치 후반의 관계 계층이다. 표들의 수명은 호출자가 보장하며 값은 호출 때마다 다시 읽는다. (2026-11-09 WB04-E150)
export class RawCanonPlacementRelations {
    #pool;
    #types;
    #surfaces;
    #islands;
    #state;
    #placement;
    #permission;

    // 표면 지도/타입과 기존 관계 helper 가 같은 풀을 사용하는지 확인한다. (2026-11-09 WB04-E150)
    constructor(pool, types, surfaces, islands, state, placement, permission) {
        if (pool?.edition !== 'patch1078') throw new TypeError('최종 배치 관계는 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !surfaces || surfaces.length !== MAP_SIDE * MAP_SIDE || !islands || !state || !placement || !permission || pool !== permission.pool) {
            throw new TypeError('최종 배치 관계 자료/풀 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#surfaces = surfaces;
        this.#islands = islands;
        this.#state = state;
        this.#placement = placement;
        this.#permission = permission;
    }

    // 같은 raw 풀을 사용하는 지형 어댑터만 연결하도록 원래 풀을 제공한다. (2026-11-09 WB04-E150)
    get pool() { return this.#pool; }

    // 지도 범위 밖이면 SID 0 으로 돌려주고 지도 안의 번호 유효성은 별도 검사한다. 비유한 좌표는 거부한다. (2026-11-09 WB04-E150)
    surfaceAt(x, y) {
        const px = coordinate(x);
        const py = coordinate(y);
        if (px < 0 || py < 0 || px >= MAP_SIDE || py >= MAP_SIDE) return 0;
        return this.#surfaces[py * MAP_SIDE + px] & 0xffff;
    }

    // 원본 isValid 는 주소 범위만 검사하므로 free/dead/void 상태를 추가하지 않는다. 번호 0 은 무효이다. (2026-11-09 WB04-E150)
    #valid(sid) { return sid !== 0 && sid < this.#pool.capacity; }

    // 0046f260: 존재하는 유효 지역 번호의 현재 제한 값을 읽는다. 원본은 없는 지역에서 assert 하므로 임의 허용/0 으로 대신하지 않는다. (2026-11-09 WB04-E150)
    #restricted(region) {
        const count = this.#state.islandCount | 0;
        if (region < 0 || region >= count || region >= this.#islands.length || !this.#islands[region].exists) throw new RangeError('최종 배치 특수 섬 지역 오류');
        return this.#islands[region].restricted >>> 0;
    }

    // 지면 누적/권한 교정은 E148 finish 가 수행하며 이 함수는 원본 후반만 처리한다. (2026-11-09 WB04-E150)
    inspect(query, terrain, priorRejection = 0) {
        if (!Number.isInteger(query.type) || query.type < 0 || query.type >= this.#types.length) throw new RangeError('최종 배치 타입 오류');
        const genus = this.#types[query.type].flags2 >>> 0;
        const owner = ((query.owner << 24) >> 24) >>> 0;
        const prior = priorRejection >>> 0;
        const result = { allowed: false, relationRejected: false, foreignRejected: prior, priorRejection: prior };
        if ((genus & GENUS_DISPLAY_BLOCK) !== 0 && (genus & GENUS_ALWAYS_ALLOWED) === 0) {
            const surface = this.surfaceAt(query.x, query.y);
            if (!this.#valid(surface)) result.relationRejected = true;
            else {
                const current = this.#pool.slot(surface)[OWNER];
                if ((genus & GENUS_BRIDGE_DISPLAY) === 0 || current !== 0) result.relationRejected = !this.#permission.related(current, owner);
            }
            this.#placement.blockedRelation = result.relationRejected ? 1 : 0;
            if ((genus & GENUS_CLEAR_BLOCK) !== 0) this.#placement.blockedRelation = 0;
        }
        if (terrain.groundComplete && (genus & GENUS_SPECIAL_REGION) !== 0) {
            const region = (terrain.regions[0] << 24) >> 24;
            if ((region >>> 0) !== (terrain.emptyRegion >>> 0) && this.#restricted(region) !== 0) terrain.canPlaceGround = false;
        }
        if ((genus & GENUS_ALWAYS_ALLOWED) !== 0) {
            result.allowed = true;
            return result;
        }
        const surface = this.surfaceAt(query.x, query.y);
        if ((genus & GENUS_BRIDGE) === 0 && this.#valid(surface)) {
            const current = this.#pool.slot(surface)[OWNER];
            if (current !== 0 && !this.#permission.alliance(current, owner) && (genus & GENUS_FOREIGN_EXEMPT) === 0) result.foreignRejected = 1;
        }
        result.allowed = (!!this.#state.bypassGroundPermission || (terrain.permission && terrain.canPlaceGround)) && !result.relationRejected && result.foreignRejected === 0;
        return result;
    }
}

// 이전 후보/주변 정책과 공통 상태를 유지하며 최종 반환만 실제 관계 구현으로 결정한다. (2026-11-09 WB04-E150)
export function makeCanonRelationsTerrainHooks(pool, relations, hooks = {}) {
    if (pool !== relations.pool) throw new TypeError('최종 배치 관계/지형 SID 풀 불일치');
    return { ...hooks, finishRelations: (query, terrain) => relations.inspect(query, terrain).allowed };
}
