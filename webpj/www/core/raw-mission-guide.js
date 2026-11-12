// PATCH Guide 의 현재 미션 포인터·pending/실제 창 관문·단계 반복을 원본 순서로 옮긴 모듈이다(cpppj `o/RawMissionGuide.cpp/.h`, 10.78 전용: 00463bb0 Run, 00482c80 Advance).
//  - run()(00463bb0): 현재 미션이 없으면 아무것도 하지 않는다. 있으면 단계 변경 전 `sequence` 를 기억하고 ① flags 의 비트 3(8)이 서 있으면 현재 미션이 아직 있을 때만 `release()` 를 부르고 끝낸다 ②
//    `pending() == 0` 이고 `dialogsClear()` 일 때(짧은 평가: pending 이 비영이면 창 관문은 읽지 않는다) 현재 미션을 다시 얻어 flags 비트 0 이 서 있으면 `advance` → 현재 단계를 다시 읽어 0..6 이면 `stage(현재 미션, 단계)` 를
//    부르고 단계 6 이면 이어서 `release()`, 범위 밖이면 `invalidStage()` ③ 막혔으면 `postpone(현재 미션)` → 마지막으로 현재 미션을 다시 얻어 없거나 단계가 6 이거나 단계가 바뀌지 않았으면 끝내고, 바뀌었으면
//    같은 호출 안에서 처음부터 반복한다(do 루프). 원본이 현재 포인터를 역참조하는 위치(전환 확인·가상 호출 대상)마다 현재 값을 다시 읽으며, 그 위치에서 NULL 이면 손상 범위를 호스트 예외(`Error`)로 거부한다.
//  - RawMissionGuide.advance(state)(00482c80, 정적): `sequence` 를 1 올리고(32비트 감김) flags 비트 0 을 지운다.
//  - hooks = { current() → 현재 미션 상태 또는 null(Guide+800), pending() → 정수(004b32e0 의 현재 pending State, 0 이어야 창 관문을 읽음), dialogsClear() → boolean(004c8e90),
//    stage(state, sequence)(현재 객체의 가상 단계 0..6), postpone(state)(가상+20 재지연), release()(00463ab0 현재 미션 소멸), invalidStage()(범위 밖 단계의 원본 진단) } 모두 필수이며 하나라도 없으면 생성 때 거부한다.
//  미션 상태는 `createMissionStartState()` 모양(`flags`/`sequence`/…)이다. 웹은 10.78 만 옮겨 판본 인자가 없고(CD 10.72 거부 시험은 옮기지 않음) 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E340: 4-D B 구획 RawMissionGuide 이식 — MissionLifecycle 시험의 선행)

// 생성 때 확인하는 필수 경계 이름이다. 아직 없는 UI/가상 몸체를 빈 성공으로 바꾸지 않는다. (2026-11-12 WB04-E340)
const REQUIRED_HOOKS = Object.freeze(['current', 'pending', 'dialogsClear', 'stage', 'postpone', 'release', 'invalidStage']);

// Guide 한 개의 Run/Advance 이다. (2026-11-12 WB04-E340)
export class RawMissionGuide {
    // 실제 현재 포인터/상태/UI·가상 몸체를 읽고 호출하는 경계이다. 현재 객체를 복제하지 않는다.
    #hooks;

    // 모든 경계가 함수여야 한다. 콜백 대상은 처리기보다 오래 살아야 한다. (2026-11-12 WB04-E340)
    constructor(hooks) {
        // 필수 경계 하나라도 함수가 아니면 거부한다.
        for (const name of REQUIRED_HOOKS) {
            if (typeof hooks?.[name] !== 'function') throw new TypeError(`Guide 의 PATCH 현재 미션/상태/UI/가상 단계 경계 누락: ${name}`);
        }
        this.#hooks = hooks;
    }

    // 원본이 현재 미션을 역참조하는 위치에서만 NULL 의 손상 범위를 명시적으로 거부한다. (2026-11-12 WB04-E340)
    #requireCurrent() {
        const current = this.#hooks.current();
        if (!current) throw new Error('Guide 가상 단계의 현재 미션 NULL');
        return current;
    }

    // 00482c80: 같은 미션의 DWORD 단계 증가(32비트 감김)와 flags 비트 0 해제이다. (2026-11-12 WB04-E340)
    static advance(state) {
        state.sequence = (state.sequence + 1) >>> 0;
        state.flags = (state.flags & ~1) >>> 0;
    }

    // 00463bb0: 취소→pending/창→advance→가상 단계/재지연→현재 포인터/단계 재조회 순서이다. 상위 State/창이 막히면 가상+20 만 실행하고 시작 가상 함수를 앞당기지 않는다. (2026-11-12 WB04-E340)
    run() {
        const hooks = this.#hooks;
        // 원본 do 루프는 현재 단계가 바뀐 경우에만 다음 가상 단계까지 이어 간다.
        for (;;) {
            let current = hooks.current();
            if (!current) return;
            const before = current.sequence; // 외부 효과 전 현재 단계의 비교용 DWORD 이다.
            if (current.flags & 8) {
                if (hooks.current()) hooks.release();
                return;
            }
            if (!hooks.pending() && hooks.dialogsClear()) {
                const selected = this.#requireCurrent();
                if (selected.flags & 1) RawMissionGuide.advance(selected);
                const sequence = this.#requireCurrent().sequence; // advance 이후 실제 switch 가 읽는 현재 단계 DWORD 이다.
                // 원본은 switch 의 단계 번호 조회와 가상 호출 대상 조회 사이에도 현재 포인터를 다시 읽는다.
                if (sequence <= 6) {
                    hooks.stage(this.#requireCurrent(), sequence);
                    if (sequence === 6) hooks.release();
                } else hooks.invalidStage();
            } else hooks.postpone(this.#requireCurrent());
            current = hooks.current();
            if (!current || current.sequence === 6 || current.sequence === before) return;
        }
    }
}
