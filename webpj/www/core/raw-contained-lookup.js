// 타입과 순번으로 종속(contained) 객체를 찾거나 그 객체의 가상 삭제를 요청하는 조회 몸체(004b1a90)를 옮긴 모듈이다(cpppj `o/RawContainedLookup.cpp/.h`, 10.78 전용).
// 범용 contained finder(`raw-contained-finder.js`)와 다음 링크를 읽는 시점이 다르다: 삭제 효과가 슬롯을 반납하거나 next 를 바꿀 수 있으므로 다음 링크는 삭제가 끝난 **뒤** 현재 슬롯에서 읽는다.
//  - find(parent, type, ordinal = 0, remove = false): parent 의 head WORD(+6)부터 next WORD(+4) 체인을 따라 TYPE BYTE(+10)가 type(DWORD 전체와 비교)과 같고 dead 비트(상태 +11 의 2)가 없는 항목 중 ordinal 번째(0부터)를 찾는다.
//    free/void/contained 비트는 후보를 제외하지 않는다. remove=false 는 해당 SID(없으면 0)를 반환한다. remove=true 는 순번이 0 인 항목에서 서버 권한이거나 client 번호(5 이상 serverFirst 미만)일 때만 가상 삭제(flags 0)를 하고
//    **계속 진행**하여 0 을 반환한다(권한이 없어도 순번은 감소한다). 순번은 DWORD 로 감소하며 0 에서 감긴다. 범위 밖/순환 체인은 방문 시점에 거부하고 이미 실행한 삭제 효과는 되돌리지 않는다.
//  - makeContainedConstructionHooks(pool, lookup, hooks): 건설 완료의 removeContained 경계를 `find(parent, type, 0, true)` 로 연결한다(다른 풀의 lookup 은 거부, 나머지 hooks 는 유지).
// destroy(sid, flags)는 선택한 객체의 가상 삭제(vtable +0x10)이며 같은 풀의 raw 를 바꿀 수 있다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E108: 종속 조회/삭제 이식으로 추가)

// next/head WORD·타입 BYTE·상태 BYTE 위치와 dead 비트, 일반 client 번호의 첫 SID 이다. (2026-11-08 WB04-E108)
const NEXT_AT = 4;
const HEAD_AT = 6;
const TYPE_AT = 10;
const STATE_AT = 11;
const DEAD = 2;
const FIRST_CLIENT = 5;

// 정렬되지 않은 링크 WORD 를 little endian 으로 읽는다. (2026-11-08 WB04-E108)
const link = (raw, offset) => raw[offset] | (raw[offset + 1] << 8);

// 종속 조회/삭제기이다. (2026-11-08 WB04-E108)
export class RawContainedLookup {
    // 현재 raw 종속 체인과 서버 권한을 공유하는 풀이다. (2026-11-08 WB04-E108)
    #pool;
    // 선택한 객체의 가상 삭제이다. (2026-11-08 WB04-E108)
    #destroy;

    // 필수 삭제 경계를 확보하고 같은 풀을 계속 참조한다. (2026-11-08 WB04-E108)
    constructor(pool, destroy) {
        if (typeof destroy !== 'function') throw new Error('contained 삭제 경계 누락');
        this.#pool = pool;
        this.#destroy = destroy;
    }

    // 건설 어댑터가 같은 월드의 풀을 사용하는지 검사할 때 쓰는 읽기 전용 참조이다. (2026-11-08 WB04-E108)
    get pool() { return this.#pool; }

    // 타입 일치와 dead 제외를 통과한 항목만 순번을 소비한다. 선택 후에도 부모 head 에서 다시 시작하지 않는다. (2026-11-08 WB04-E108)
    find(parent, type, ordinal = 0, remove = false) {
        const pool = this.#pool;
        let current = link(pool.slot(parent), HEAD_AT);
        let remaining = ordinal >>> 0;
        const visited = new Set();
        // 삭제가 슬롯을 반납하거나 next 를 바꿀 수 있으므로 다음 링크는 효과가 끝난 뒤 읽는다.
        while (current) {
            if (current >= pool.capacity || visited.has(current)) throw new Error('contained 조회 종속 체인 손상');
            visited.add(current);
            const raw = pool.slot(current);
            if (raw[TYPE_AT] === type && !(raw[STATE_AT] & DEAD)) {
                if (remaining === 0) {
                    if (!remove) return current;
                    if (pool.isServer || (current >= FIRST_CLIENT && current < pool.layout.serverFirst)) this.#destroy(current, 0);
                }
                remaining = (remaining - 1) >>> 0;
            }
            current = link(pool.slot(current), NEXT_AT);
        }
        return 0;
    }
}

// 건설 완료 시 진행음 프로세스를 실제 종속 체인에서 찾아 지우도록 연결한다. (2026-11-08 WB04-E108)
export function makeContainedConstructionHooks(pool, lookup, hooks) {
    if (pool !== lookup.pool) throw new Error('contained 건설 어댑터 풀 불일치');
    return { ...hooks, removeContained: (parent, type) => { lookup.find(parent, type, 0, true); } };
}
