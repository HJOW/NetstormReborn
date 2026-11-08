// 사제 배치 검사(MayPlace)의 전역 초기화·강제 허용·모양 여백·지도 경계 접두를 옮긴 모듈이다(cpppj `o/RawPriestPlacement.cpp/.h`, 10.78).
//  - new RawPriestPlacement(pool, types, state, hooks).mayPlace(query): query = { type, x, y, flags, owner, mode }(사제 생성의 여섯 배치 인자). 사제 genus(0x200000)를 가진 자산 타입(70 이상)만 받고 좌표는 유한해야 한다.
//    hooks = { shape(type, argument, flags) → 사각형, inspectGeometry(query, localOwner) → bool } 둘 다 필수이다. 판정은 일반 배치 접두(E152 RawCanonPlacement)에 위임하되 첫 인자(argument)는 타입 번호 자신이고
//    후반 경계(inspectGeometry)에는 호출 때 값으로 복사한 사제 요청을 그대로 넘긴다(호출자가 요청 객체를 바꿔도 원본 스택 값 인자와 로컬 비교 결과를 유지). state 는 일반 배치와 같은 구조(createPlacementState)이다.
//  - makePriestSpawnPlacementHooks(pool, placement, hooks): 사제 생성의 mayPlace 경계만 위 접두로 연결하고 다른 생성 효과는 유지한다.
// 10.72(CD)의 중간 단정도 저장 여백은 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E155: 사제 배치 접두 이식으로 추가)
import { RawCanonPlacement } from './raw-canon-placement.js';

// 첫 자산 타입 번호, 사제 genus 비트, 타입 표 크기이다. (2026-11-09 WB04-E155)
const FIRST_ASSET_TYPE = 70;
const GENUS_PRIEST = 0x200000;
const TYPE_COUNT = 188;

// 사제 genus 전용 배치 접두이다. 풀·타입 표·전역·하위 경계의 수명은 호출자가 보장한다. (2026-11-09 WB04-E155)
export class RawPriestPlacement {
    #pool;
    #types;
    #state;
    #hooks;

    // 타입 개수와 필요한 두 경계를 확인한다. (2026-11-09 WB04-E155)
    constructor(pool, types, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('사제 배치는 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !state || typeof hooks?.shape !== 'function' || typeof hooks.inspectGeometry !== 'function') throw new TypeError('사제 배치 자료/하위 경계 누락');
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#hooks = { shape: hooks.shape, inspectGeometry: hooks.inspectGeometry };
    }

    // 사제 생성과 같은 풀에 연결하는지 확인할 수 있도록 원래 풀을 제공한다. (2026-11-09 WB04-E155)
    get pool() { return this.#pool; }

    // 진입 초기화는 즉시 허용·지도 밖 거부에도 적용한다. 모양 조회 뒤 전역을 다시 비교하지 않는다. 요청은 값으로 복사한다. (2026-11-09 WB04-E155)
    mayPlace(query) {
        query = { ...query };
        if (!Number.isInteger(query.type) || query.type < FIRST_ASSET_TYPE || query.type >= this.#types.length || (this.#types[query.type].flags2 & GENUS_PRIEST) === 0) throw new RangeError('사제 배치 타입/genus 범위 오류');
        if (!Number.isFinite(query.x) || !Number.isFinite(query.y)) throw new TypeError('사제 배치의 비유한 지도 좌표');
        const placement = new RawCanonPlacement(this.#pool, this.#types, this.#state, {
            shape: this.#hooks.shape,
            inspectGeometry: (_canonQuery, localOwner) => this.#hooks.inspectGeometry({ ...query }, localOwner)
        });
        return placement.mayPlace({ type: query.type, argument: query.type, x: query.x, y: query.y, flags: query.flags, owner: query.owner, mode: query.mode });
    }
}

// 사제 생성의 mayPlace 경계만 위 접두로 연결하고 다른 생성 효과는 유지한다. 원본 사제 생성의 여섯 배치 인자를 그대로 전달한다. (2026-11-09 WB04-E155)
export function makePriestSpawnPlacementHooks(pool, placement, hooks = {}) {
    if (pool !== placement.pool) throw new TypeError('사제 배치/생성의 SID 풀이 다릅니다');
    return { ...hooks, mayPlace: (query) => placement.mayPlace(query) };
}
