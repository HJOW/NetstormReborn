// Player 의 실제 내용물 통지(00490750 → 004d55e0)와 현재 GumpForm 소비자 순회를 옮긴 모듈이다(cpppj `o/RawPlayerContentNotice.cpp/.h`, 10.78 전용).
// 순서: ① 진단 접두 — 첫 비교(cursorA!=cursorB)에서 problemCode 에 fallbackCode 를 쓰고, moneyChecks 가 비0 이면 PlayerKnown(battleShown)일 때 현재 SP(money(localPlayer))를 조회해
//   fallbackCode 를 다시 읽고 7000 초과면 problemCode 에 쓰며 moneyChecks 를 하나 줄인다(32비트 감김). 첫 SP 분산배열 주소가 0 이거나 첫 순열이 -1 이면 problemCode 에 fallback 을 쓴다.
//   ② 부모 SID(+6 WORD)에서 시작해 종속 체인을 순회한다. 각 Form 의 next(+4 WORD)는 등록 조회/가상 처리보다 먼저 읽어 보존하고, Form 타입(+10)이 현재 gumpFormType 과 같으면
//   등록 ID(+18 DWORD)로 Gump 토큰을 조회해 비0 일 때 handle(token, contentEvent, form, reason) 을 부른다. 순환/범위 밖 SID 는 거부한다. 부모/자식 상태를 임의로 필터하지 않는다.
// 전역은 호출 위치에서 한 번 현재 값을 읽는 getter 로만 받고(초기값/프레임 사본 재사용 금지) problemCode·moneyChecks 만 쓴다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E9: Player 내용물 통지 이식으로 추가)

// 원본 ROM 00505514 의 SP 이상 조건 상수이다. (2026-11-08 WB04-E9)
const MONEY_THRESHOLD = 7000;

// 정렬되지 않은 원본 필드의 정확한 BYTE 폭을 읽는다(WORD 뒤의 stale 값을 섞지 않음). (2026-11-08 WB04-E9)
function word(raw, offset, width) {
    let result = 0;
    // 낮은 BYTE 부터 조합한다.
    for (let index = 0; index < width; index += 1) result += raw[offset + index] * 2 ** (8 * index);
    return result;
}

// Player 내용물 통지 처리기이다.
//  - pool: 현재 head/next/type/Gump 등록 ID 를 읽는 유일한 저장소(SidPool).
//  - state: { cursorA(), cursorB(), problemCode(읽기/쓰기 속성), fallbackCode(), moneyChecks(읽기/쓰기 속성), battleShown(), localPlayer(), firstSpPool(), firstPermutation()(signed 16), gumpFormType(), contentEvent() }
//  - hooks: { money(owner) → float, gump(id) → 토큰/0, handle(token, event, form, reason) }
// (2026-11-08 WB04-E9)
export class RawPlayerContentNotice {
    #pool;
    #state;
    #hooks;

    // 현재 전역/실제 UI 소비자가 하나라도 없으면 통지를 성공으로 대신하지 않고 첫 쓰기 전에 거부한다.
    constructor(pool, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Player 내용물은 10.78 SID 풀만 받습니다.');
        const getters = ['cursorA', 'cursorB', 'fallbackCode', 'battleShown', 'localPlayer', 'firstSpPool', 'firstPermutation', 'gumpFormType', 'contentEvent'];
        for (const name of getters) if (typeof state?.[name] !== 'function') throw new TypeError(`Player 내용물의 현재 전역 조회기 누락: ${name}`);
        if (typeof hooks?.money !== 'function' || typeof hooks.gump !== 'function' || typeof hooks.handle !== 'function') {
            throw new TypeError('Player 내용물의 현재 실제 SP/등록표/UI 경계 누락');
        }
        this.#pool = pool;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 상위가 현재 PlayerVT 를 분배할 때 같은 풀인지 검증하도록 풀을 돌려준다.
    get pool() { return this.#pool; }

    // 원본 Player 의 부모 내용물 알림과 직접 004d55e0 호출이 쓰는 같은 몸체이다. parent 는 Player SID, form 은 알림 대상 Form, reason 은 사유이다.
    notify(parent, form, reason) {
        const state = this.#state;
        // 원본 EAX 는 첫 비교 직후 읽고 SP 조회가 없으면 뒤 조건에서도 보존한다.
        let fallback = state.fallbackCode() >>> 0;
        if ((state.cursorA() >>> 0) !== (state.cursorB() >>> 0)) state.problemCode = fallback;
        if (state.moneyChecks) {
            if (state.battleShown()) {
                // 출력 float 는 조건부 조회에서 한 번 저장한 현재 SP 이다.
                const money = this.#hooks.money(state.localPlayer() >>> 0);
                fallback = state.fallbackCode() >>> 0;
                if (money > MONEY_THRESHOLD) state.problemCode = fallback;
            }
            state.moneyChecks = (state.moneyChecks - 1) >>> 0;
        }
        if (!state.firstSpPool()) state.problemCode = fallback;
        if (state.firstPermutation() === -1) state.problemCode = fallback;
        let current = word(this.#pool.slot(parent), 6, 2);
        const visited = new Set();
        // 현재 head 는 진단/SP 효과가 끝난 뒤 읽으며 각 next 는 UI 조회/처리 전에 보존한다.
        while (current) {
            if (current >= this.#pool.capacity || visited.has(current)) throw new Error('Player 내용물 UI 종속 체인 물리 범위/순환 오류');
            visited.add(current);
            const raw = this.#pool.slot(current);
            const type = raw[10];
            const gumpType = state.gumpFormType() >>> 0;
            // 원본은 다음 Form 을 lookup/가상 handle 보다 먼저 읽는다.
            const next = word(raw, 4, 2);
            if (type === gumpType) {
                // 등록 ID 는 WORD 로 줄이지 않는 현재 DWORD 이다.
                const token = this.#hooks.gump(word(raw, 18, 4));
                if (token) this.#hooks.handle(token, state.contentEvent() >>> 0, form, reason);
            }
            current = next;
        }
    }
}
