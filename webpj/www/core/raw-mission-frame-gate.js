// 10.78 Tutorial 의 공통 프레임·안내 정지·단계 예약(PATCH 004c34c0 외 세 몸체)을 현재 상태와 실제 UI 조회로 실행하는 모듈이다(cpppj `o/RawMissionFrameGate.cpp/.h`, 10.78 전용).
//  - runFrame(event)(004c34c0): 시작되지 않았으면(started 0) 아무 조회/효과 없이 0 을 돌려준다. 시작된 프레임은 stageDirty = 0 → 승패/사제 사건(checkOutcome) → 다음 단계 안내 기한이 지났거나 시계가 정지 중이고
//    안내 잠금이 남았으면(다이얼로그가 있으면 잠금을 10 으로 되돌리고 없으면 1 감소, 잠금이 1 미만이 되면 단계 Tell) → 상대 기한이 지난 tellAtList 실행 → 이 미션이 정지를 보유한 채 잠금이 0 이면
//    안내/다이얼로그/막는 Gump 가 모두 없는 연속 프레임을 세어 7 프레임째에 재개 → 예약된 데모 종료 기한 처리 → 잠금 0·비데모·튜토리얼 번호 1~6 이면 해당 판정기 결과를 돌려준다.
//  - advance()(004c33f0): 시계 정지 중이 아니고 잠금 < 1 일 때만 단계 char 를 1 올리고(저 8비트 부호 확장) 잠금 10, 다음 안내 기한 = float 지연 + 현재 double 게임 시각, 타이머 0 을 예약하고 true 를 돌려준다.
//  - pauseForDialog()/pauseMissionFrameForDialog(state, paused, pause)(004c2a40): 현재 시계 정지와 자기 소유 표식을 함께 읽어 중복 정지를 피한다(월드 설치 전에도 호출 가능한 독립 함수).
//  - postpone()(004c3170): 이미 비영인 다음 안내 기한만 현재 게임 시각 + float 지연으로 다시 예약한다.
// 모든 호출은 원본의 해당 위치에서 현재 상태를 조회하며 호출 중 상태가 바뀌어도 복사하지 않는다. tell/list/demo/Tutorial 본체와 시계/UI 는 호출자가 명시 경계로 연결한다(모두 필수).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E231: 미션 공통 프레임 문 이식으로 추가)

// 원본이 안내 잠금과 재개 연속 화면 수에 쓰는 signed DWORD 상수이다. (2026-11-10 WB04-E231)
const LOCK_FRAMES = 10;
const RESUME_FRAMES = 7;

// 원본 Tutorial 객체의 공통 필드와 세 단계 전역을 하나의 장기 저장소로 만든다. Tutorial1~6 판정기도 stage/timerSeconds 를 이 인스턴스에서 직접 읽어야 한다.
// 필드: stage(005ca8e8 현재 단계, 초기 65)·stageDirty(005ca8ec, 초기 1)·tutorialNumber(005ca8e4 의 1~6 분배 번호)·demo(+68)·startSeconds(+70 double)·nextStageSeconds(+78 double)·
//       delaySeconds(+80 float32, 초기 0.5)·lockFrames(+84, 초기 10)·started(+88)·demoEndSeconds(+90 double)·timerSeconds(+98 double)·tellAtSeconds(+a8 double)·scriptPaused(+b8)·clearFrames(+c8). (2026-11-10 WB04-E231)
export function createMissionFrameGateState() {
    return {
        stage: 65, stageDirty: 1, tutorialNumber: 0, demo: 0, startSeconds: 0, nextStageSeconds: 0, delaySeconds: Math.fround(0.5), lockFrames: LOCK_FRAMES, started: 0,
        demoEndSeconds: 0, timerSeconds: 0, tellAtSeconds: 0, scriptPaused: 0, clearFrames: 0,
    };
}

// World 생성과 무관한 같은 원문 자기 정지 몸체이다. 실제 도달한 장치만 요구한다(누락은 TypeError). paused: () => boolean, pause: () => void. (2026-11-10 WB04-E231)
export function pauseMissionFrameForDialog(state, paused, pause) {
    if (typeof paused !== 'function') throw new TypeError('미션 자기 정지의 현재 정지 조회 미연결');
    if (!paused() || !state.scriptPaused) {
        if (typeof pause !== 'function') throw new TypeError('미션 자기 정지 장치 미연결');
        pause();
        state.scriptPaused = 1;
    }
}

// 필수 하위 경계 이름이다. 누락은 정상 성공/빈 동작으로 처리하지 않도록 최초 실행 전에 모두 검사한다. (2026-11-10 WB04-E231)
const REQUIRED_HOOKS = Object.freeze(['gameSeconds', 'paused', 'checkOutcome', 'dialogsClear', 'knowledgeOpen', 'guideOpen', 'blockingGumpOpen', 'tellStage', 'tellAt', 'endDemo', 'tutorial', 'pause', 'resume']);

// PATCH 004c34c0 의 공통 분기를 보존한다. 초나 24Hz 틱이 아닌 실제 입력/화면 호출마다 한 번 사용한다.
//  hooks = { gameSeconds() → double(0055b4d0, 벽시각 아님), paused() → boolean(00460de0), checkOutcome()(004c36c0), dialogsClear() → boolean(004c8e90), knowledgeOpen() → boolean(00445330),
//            guideOpen() → boolean(004c79a0), blockingGumpOpen() → boolean(004641d0 의 0x4000 자식 창), tellStage()(004c2a90), tellAt()(004c3470), endDemo()(00452f90),
//            tutorial(number, event) → 정수(실제 1~6 판정), pause()(00460df0), resume()(00460e30) }. 사용: new RawMissionFrameGate(state, hooks).runFrame(event). (2026-11-10 WB04-E231)
export class RawMissionFrameGate {
    // 원본 필드/단계/타이머의 단일 저장소와 실제 시계/UI/미션 본체의 현재 조회와 효과이다.
    #state;
    #hooks;

    // 상태/훅 대상은 처리기보다 오래 살아야 한다. 선택되지 않은 큰 본체도 명시 경계로 공급한다. (2026-11-10 WB04-E231)
    constructor(state, hooks = {}) {
        if (!state || REQUIRED_HOOKS.some((name) => typeof hooks[name] !== 'function')) throw new TypeError('미션 공통 프레임의 실제 시계/UI/분배 경계 누락');
        this.#state = state;
        this.#hooks = hooks;
    }

    // started 이후 outcome → 10프레임 → tellAt → 7프레임 → demo → Tutorial 순서를 실행한다. 최초 예약과 후속 예약은 같은 카운터를 쓰고, 콜백이 쓴 현재 필드는 다음 원본 읽기에서 반영한다. (2026-11-10 WB04-E231)
    runFrame(event = 0) {
        const state = this.#state;
        const hooks = this.#hooks;
        if (!state.started) return 0;
        state.stageDirty = 0;
        hooks.checkOutcome();
        if ((state.nextStageSeconds <= hooks.gameSeconds() || hooks.paused()) && state.lockFrames > 0) {
            if (!hooks.dialogsClear()) state.lockFrames = LOCK_FRAMES;
            else state.lockFrames = (state.lockFrames - 1) | 0;
            if (state.lockFrames < 1) hooks.tellStage();
        }
        if (state.tellAtSeconds !== 0 && state.tellAtSeconds < hooks.gameSeconds() - state.startSeconds && hooks.dialogsClear() && !hooks.knowledgeOpen()) hooks.tellAt();
        if (state.scriptPaused && state.lockFrames === 0) {
            if (!hooks.guideOpen() && hooks.dialogsClear() && !hooks.blockingGumpOpen()) {
                state.clearFrames = (state.clearFrames + 1) | 0; // 원본 DWORD 덧셈은 signed overflow 에서도 감긴다.
                if (state.clearFrames >= RESUME_FRAMES) {
                    if (state.scriptPaused) { hooks.resume(); state.scriptPaused = 0; }
                    state.clearFrames = 0;
                }
            } else state.clearFrames = 0;
        }
        if (state.demo && state.demoEndSeconds !== 0 && state.demoEndSeconds <= hooks.gameSeconds()) {
            hooks.endDemo();
            state.demoEndSeconds = 0;
        }
        if (state.lockFrames === 0 && !state.demo && state.tutorialNumber >= 1 && state.tutorialNumber <= 6) return hooks.tutorial(state.tutorialNumber, event);
        return 0;
    }

    // 004c33f0: 실행 중이며 잠금 < 1 일 때만 단계 char 증가(저 8비트 부호 확장)/잠금 10/지연/타이머 0 을 예약한다. x87 은 float 지연 뒤 double 게임 시각을 더하며 현재 시각을 float 로 축소하지 않는다. (2026-11-10 WB04-E231)
    advance() {
        const state = this.#state;
        if (this.#hooks.paused() || state.lockFrames >= 1) return false;
        const next = ((state.stage >>> 0) + 1) & 0xff; // 원본 char 증가의 저 8비트이다.
        state.stage = (next << 24) >> 24;
        state.lockFrames = LOCK_FRAMES;
        state.nextStageSeconds = state.delaySeconds + this.#hooks.gameSeconds();
        state.timerSeconds = 0;
        return true;
    }

    // 004c2a40: 다른 정지 보유자가 있어도 이 미션의 첫 정지는 별도 깊이를 잡는다. 이미 자기 깊이가 있으면 재진입을 생략한다. (2026-11-10 WB04-E231)
    pauseForDialog() { pauseMissionFrameForDialog(this.#state, this.#hooks.paused, this.#hooks.pause); }

    // 004c3170: 초기 0 기한은 유지한다. 현재 0 이 아닌 경우의 재지연만 원본 가상 +20 과 같다. (2026-11-10 WB04-E231)
    postpone() {
        const state = this.#state;
        if (state.nextStageSeconds !== 0) state.nextStageSeconds = state.delaySeconds + this.#hooks.gameSeconds();
    }

    // Tutorial1~6 과 상위 스크립트가 같은 원본 저장소를 공유한다. (2026-11-10 WB04-E231)
    get state() { return this.#state; }
}
