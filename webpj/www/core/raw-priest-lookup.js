// 공유 사제 목록에서 현재 소유자의 첫 살아 있는 사제를 찾는 조회(004921f0)를 옮긴 모듈이다(cpppj `o/RawPriestLookup.cpp/.h`, 10.78 전용).
// 순서: 활성 개수를 signed 로 읽어 0 이하이면 0 → 목록을 원래 순서로 읽으며 첫 일치에서 멈춘다(뒤의 손상된 항목은 읽지 않음). 일치는 현재 소유자 BYTE 와 전체 DWORD 인자가 같고 상태 dead 비트(2)가 없는 항목이다.
//   free/void/타입/extra 는 검사하지 않는다. 범위 밖 SID(풀 용량 이상)는 WORD 로 자르지 않고 접근 때 거부한다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E66: 사제 소유자 조회 이식으로 추가)

// 슬롯의 상태/현재 소유자 BYTE 위치와 조회가 제외하는 유일한 상태 비트(dead)이다. (2026-11-08 WB04-E66)
const STATE_OFFSET = 11;
const OWNER_OFFSET = 34;
const DEAD = 2;

// pool 은 10.78 SidPool, state 는 사제 목록(`state.priests` SquidPostPopList)을 가진 PriestPostPopState 이다. 등록·제거가 즉시 보이도록 목록을 복사하지 않고 참조한다. (2026-11-08 WB04-E66)
export class RawPriestLookup {
    // 현재 소유자와 사망 상태를 매 조회마다 읽는 풀, 공유 사제 목록의 상태이다.
    #pool;
    #state;

    // 판본 불일치는 첫 조회 전에 거부한다. (2026-11-08 WB04-E66)
    constructor(pool, state) {
        if (pool?.edition !== 'patch1078' || !state?.priests) throw new TypeError('사제 조회 연결 오류');
        this.#pool = pool;
        this.#state = state;
    }

    // 건설 통지 어댑터가 같은 월드인지 검사하도록 노출하는 풀이다. (2026-11-08 WB04-E66)
    get pool() {
        return this.#pool;
    }

    // 소유자 인자는 전체 DWORD 로 비교한다(BYTE 로 자르지 않음). 없으면 0 이다. 손상된 양수 개수(용량 초과)와 범위 밖 SID 는 접근 시 거부한다. (2026-11-08 WB04-E66)
    find(player) {
        const list = this.#state.priests;
        if ((list.readCount() | 0) <= 0) return 0;
        const wanted = player >>> 0;
        // 목록을 원래 순서로 읽고 첫 일치에서 멈춘다(뒤의 항목은 읽지 않는다).
        for (const sid of list.items()) {
            if (sid >= this.#pool.capacity) throw new RangeError('소유 사제 조회 SID 범위 오류');
            const raw = this.#pool.slot(sid);
            if (raw[OWNER_OFFSET] === wanted && !(raw[STATE_OFFSET] & DEAD)) return sid;
        }
        return 0;
    }
}
