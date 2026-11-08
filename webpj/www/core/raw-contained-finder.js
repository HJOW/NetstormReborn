// 내용물(contained) 검색기의 현재/다음 SID 커서와 기본 true 필터를 옮긴 모듈이다(cpppj `o/RawContainedFinder.cpp/.h`, 10.78 전용).
// begin(parent): 10.78 은 checking 이 켜졌고 부모가 dead 이며 권한(boss)이 없고 dead 허용이 아니면 원문 진단(Squidfinder.cpp 0x13c)을 먼저 부른다.
//   진단이 정상 복귀한 뒤에 부모 head WORD(+6)를 처음 읽는다. next(): 후보의 next WORD(+4)를 반환 전에 저장하고, 매 후보마다 현재 contained 타입 전역과
//   후보 TYPE BYTE(+10)를 비교한다(DWORD 그대로 비교). 후보의 free/dead/void/contained 는 거르지 않는다. 범위 밖 번호·순환은 방문 시점에 오류다.
// countContained: 기본 finder 로 kind(+18 WORD, DWORD 비교)·mask(+20 WORD) 조건에 맞는 모든 내용물을 센다(004acc60).
// 상태(ContainedFinderState)는 boss·containedType·checkingDead·진단 장치를 갖고, 실제 Client 의 공유 boss({value})와 checking getter 를 빌릴 수 있다.
// CD 10.72 의 인라인 finder(dead 진단 없음)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E56: 내용물 검색기 이식으로 추가)

// 공통 raw 종속 next/head WORD·TYPE·상태 위치와 dead 비트, 종류/조건 WORD 위치이다. (2026-11-08 WB04-E56)
const NEXT_OFFSET = 4;
const HEAD_OFFSET = 6;
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const DEAD = 2;
const KIND_OFFSET = 18;
const MASK_OFFSET = 20;

// 원본 dead 부모 조회 진단의 식·파일·줄이다. (2026-11-08 WB04-E56)
export const CONTAINED_FINDER_DEAD_ASSERTION = Object.freeze({
    expression: "!p.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy",
    file: '\\Ns\\O\\Squidfinder.cpp',
    line: 0x13c,
});

// 비정렬 little endian WORD 를 읽는다. (2026-11-08 WB04-E56)
function word(raw, offset) {
    return raw[offset] | (raw[offset + 1] << 8);
}

// 내용물 검색기가 읽는 현재 전역 묶음이다(10.78 주소). 독립 입력은 자기 값을 쓰고 실제 세션은 bind* 로 같은 장기 소유자를 빌린다.
// 사용: const state = new ContainedFinderState(); state.containedType = 6; state.bindBoss(authorityOwner). (2026-11-08 WB04-E56)
export class ContainedFinderState {
    // 00540bc4: 독립 입력의 현재 권한 값이다.
    boss = true;
    // 00541088: 후보 BYTE 와 비교하는 contained TYPE DWORD 이다.
    containedType = 6;
    // 005e4794: 독립 입력의 checking 값(boolean 또는 DWORD)이다. 0 이 아니면 dead 부모 진단을 켠다.
    checkingDead = 0;
    // 원문 진단 장치 (식, 파일, 줄)이다. 없으면 선택된 진단 위치에서 명시 거부한다.
    assertion = null;
    // Carrier·Damageable·내용물 검색이 함께 읽는 실제 Client boss 의 { value } 소유자이다(없으면 null).
    #sharedBoss = null;
    // 실제 Client checking DWORD 의 현재 getter 이다(없으면 null).
    #checking = null;

    // 실제 공유 boss 소유자({ value })를 빌린다. null 이면 자기 값으로 돌아간다.
    bindBoss(owner) {
        if (owner !== null && (typeof owner !== 'object' || !('value' in owner))) throw new TypeError('contained finder 공유 boss 는 { value } 소유자여야 합니다.');
        this.#sharedBoss = owner;
    }

    // 실제 checking DWORD getter(() => number)를 빌린다. null 이면 자기 값으로 돌아간다.
    bindChecking(getter) {
        if (getter !== null && typeof getter !== 'function') throw new TypeError('contained finder checking 은 getter 여야 합니다.');
        this.#checking = getter;
    }

    // 외부 효과 뒤 또는 dead 진단 관문에서 다시 읽는 현재 권한이다.
    get currentBoss() { return this.#sharedBoss ? Boolean(this.#sharedBoss.value) : Boolean(this.boss); }

    // 진단 소비 위치마다 다시 읽는 현재 checking DWORD 이다.
    get currentChecking() { return this.#checking ? this.#checking() >>> 0 : Number(this.checkingDead) >>> 0; }
}

// 내용물 검색 커서이다. 풀/상태는 커서보다 오래 살아야 한다.
// 사용: const finder = new RawContainedFinder(pool, state); for (let sid = finder.begin(parent); sid; sid = finder.next()) { … }. (2026-11-08 WB04-E56)
export class RawContainedFinder {
    // 같은 실제 SID 풀이다.
    #pool;
    // 현재 contained 타입·권한·checking 을 매번 다시 읽는 상태이다.
    #state;
    // 다음에 볼 후보 SID(이전 반환 전에 저장한 next)이다.
    #next = 0;
    // 이번 순회에서 방문한 SID 집합(순환 검출용)이다.
    #visited = new Set();

    // 풀과 상태를 빌린다.
    constructor(pool, state) {
        if (pool?.edition !== 'patch1078') throw new TypeError('contained finder 는 10.78 SID 풀만 받습니다.');
        if (!(state instanceof ContainedFinderState)) throw new TypeError('contained finder 상태가 필요합니다.');
        this.#pool = pool;
        this.#state = state;
    }

    // 004b2270: 부모 head WORD 에서 시작한다. allowDead 는 10.78 dead 진단만 우회하며(Damageable·Dais) 후보의 타입 조건은 바꾸지 않는다.
    begin(parent, allowDead = false) {
        const pool = this.#pool;
        const state = this.#state;
        if (!Number.isInteger(parent) || parent < 0 || parent >= pool.capacity) throw new RangeError('contained finder 부모 SID 오류');
        if (state.currentChecking && (pool.slot(parent)[STATE_OFFSET] & DEAD) && !state.currentBoss && !allowDead) {
            // 진단 중 장치가 교체돼도 선택한 호출을 끝까지 쓴다.
            const assertion = state.assertion;
            if (typeof assertion !== 'function') throw new Error('권한 없는 dead 부모 종속 조회 assert 경로');
            assertion(CONTAINED_FINDER_DEAD_ASSERTION.expression, CONTAINED_FINDER_DEAD_ASSERTION.file, CONTAINED_FINDER_DEAD_ASSERTION.line);
        }
        // 진단이 정상 복귀한 뒤 현재 부모 head 를 처음 읽는다.
        this.#next = word(pool.slot(parent), HEAD_OFFSET);
        this.#visited.clear();
        return this.next();
    }

    // 004b1b20: 저장한 next 부터 현재 타입 전역으로 진행한다. 반환할 후보의 다음 링크를 먼저 저장해 호출자의 중첩 효과에 대비한다.
    next() {
        const pool = this.#pool;
        // 타입 전역은 매 후보마다 읽으며 순환/범위 오류는 방문 시점에 거부한다.
        while (this.#next) {
            const current = this.#next;
            if (current >= pool.capacity || this.#visited.has(current)) throw new Error('contained finder 종속 체인 손상');
            this.#visited.add(current);
            const raw = pool.slot(current);
            this.#next = word(raw, NEXT_OFFSET);
            if (raw[TYPE_OFFSET] === this.#state.containedType) return current;
        }
        return 0;
    }
}

// 004acc60: 기본 finder 로 kind(DWORD 그대로 비교, 0 이면 생략)·mask(0 이면 생략) 조건에 맞는 모든 내용물을 센다. 중복/free/dead 후보를 제외하지 않는다. 반환은 signed 32비트이다. (2026-11-08 WB04-E56)
export function countContained(pool, state, parent, mask, kind) {
    let count = 0;
    const finder = new RawContainedFinder(pool, state);
    // 종류 판정 뒤에도 mask 조건을 평가하고 전체 현재 종속 체인을 순서대로 센다.
    for (let sid = finder.begin(parent); sid; sid = finder.next()) {
        const raw = pool.slot(sid);
        const matches = !(kind >>> 0) || (kind >>> 0) === word(raw, KIND_OFFSET);
        if ((!(mask >>> 0) || ((mask >>> 0) & word(raw, MASK_OFFSET))) && matches) count = (count + 1) >>> 0;
    }
    return count | 0;
}
