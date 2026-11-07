// 원본 Squid 의 hasLevel/getLevel/setLevel(패치 004ad0a0/004ad0f0/004ad110)을 실제 슬롯 상태 BYTE 에 연결한 모듈이다(cpppj `o/RawSquidLevel.cpp/.h`).
//  - 레벨은 상태 BYTE(+11)의 상위 3비트이다. 하위 5비트(free·dead·void·contained·전송)는 보존한다.
//  - has: 원본 hasLevel 의 OR 조건 때문에 정상 TYPE 에서는 항상 true 이다(intended AND 로 고치지 않음). 읽기에는 free/dead/contained 관문을 추가하지 않고 현재 TYPE 의 물리 범위만 확인한다.
//  - set: signed 범위 진단(0 <= n && n < 8, Squid.cpp 0x579) 뒤에도 저장을 계속한다. 진단 뒤 현재 BYTE 의 하위 5비트와 n 의 하위 3비트를 저장하며 풀 할당/반납은 하지 않는다.
// 근거: cpppj 기준선 1f00a25a. 원본 관측 squid-level-x86.tsv(10.78 행, tests/squid-level.test.mjs). 10.78 전용이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E2: 레벨 상태 이식으로 추가)

// 레벨 진단이 보고하는 원문 파일 이름과 행(10.78)이다. (2026-11-08 WB04-E2)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';
const OUT_OF_RANGE_LINE = 0x579;

// 같은 Client 진단 장치를 원문 식/파일/행으로 호출하는 범위 진단 어댑터를 만든다. assertion 이 없으면 호출 시점에 오류이다.
// 사용: new RawSquidLevel(pool, types, makeSquidLevelAssertion((expression, file, line) => …)). (2026-11-08 WB04-E2)
export function makeSquidLevelAssertion(assertion) {
    return () => {
        if (typeof assertion !== 'function') throw new Error('Squid level 실제 범위 진단 장치 미연결');
        assertion('0 <= n && n < 8', SQUID_CPP, OUT_OF_RANGE_LINE);
    };
}

// 레벨 상태 접근자이다. Pool/TYPE/진단 경계는 객체보다 오래 살아야 한다.
export class RawSquidLevel {
    // 원문 예약/반납 영역까지 포함하는 실제 물리 슬롯 저장소, 현재 raw type 의 TYPE 조회 범위, 범위 진단 함수이다.
    #pool;
    #types;
    #assertion;

    // 판본의 전체 TYPE 표와 원본 진단 경계를 처음 쓰기 전에 확인한다.
    constructor(pool, types, assertion) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== 188 || typeof assertion !== 'function') {
            throw new TypeError('Squid level 판본 TYPE/진단 누락');
        }
        this.#pool = pool;
        this.#types = types;
        this.#assertion = assertion;
    }

    // 같은 풀인지 확인하기 위한 실제 풀이다.
    get pool() { return this.#pool; }

    // 원본 hasLevel: 현재 TYPE 의 물리 범위만 확인하고 항상 true 이다.
    has(sid) {
        if (this.#pool.slot(sid)[10] >= this.#types.length) throw new RangeError('Squid level 현재 TYPE 범위 오류');
        return true;
    }

    // 현재 상태 BYTE 의 상위 3비트를 zero-extend 한 레벨이다.
    get(sid) {
        return this.has(sid) ? this.#pool.slot(sid)[11] >>> 5 : 0;
    }

    // signed 범위 진단 뒤 현재 BYTE 의 하위 5비트와 n 의 하위 3비트를 저장한다. 진단이 상태 BYTE 를 바꿔도 마지막 저장 직전의 현재 BYTE 를 읽는다.
    set(sid, value) {
        this.has(sid);
        const n = value >>> 0;
        if ((n | 0) < 0 || n >= 8) this.#assertion();
        const raw = this.#pool.physical(sid);
        raw[11] = (raw[11] & 31) | ((n & 7) << 5);
    }
}
