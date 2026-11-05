// 원본 10.78 게임 시계(00460cd0~00460e90)를 옮긴 모듈이다. 32비트 밀리초 입력으로 벽 시각·게임 시각을 double 초로 계산하고,
// 중첩 정지 동안의 시간을 게임 시각에서 빼며, 프레임마다 게임 시각·벽 시각·시간차·프레임 번호를 한 번 고정한다.
// 표시용 FrameClock(netstorm.js, requestAnimationFrame 간격·fps)과는 별개이며 게임 규칙만 이 시계를 쓴다. OS timeGetTime 은 호출자가 넘기는 밀리초로 대신한다.
// 근거: cpppj `o/GameClock.cpp/.h`, 원본 관측 `cpppj/tests/fixtures/game-clock-reset-x86.tsv`(공개 호출 114행). 초기 시간 오프셋과 FPS 통계는 cpppj 에서도 미복원이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-05 WB04-C1: 세션 장기 상태의 게임 시계 이식으로 추가)

// 밀리초 나머지를 초로 바꾸는 배율이다(원본 DAT_0050a668). (2026-11-05 WB04-C1)
const MILLISECOND_SECONDS = 0.001;

// 32비트 부호 없는 밀리초를 검사한다. (2026-11-05 WB04-C1)
function milliseconds(value) {
    if (!Number.isInteger(value) || value < 0 || value > 0xffffffff) throw new RangeError('게임 시계의 밀리초는 32비트 부호 없는 정수여야 합니다.');
    return value;
}

// 원본 게임 시계이다. 모든 계산은 원본과 같은 순서의 double 연산이며 밀리초 차이는 unsigned 32비트로 감긴다.
// 사용: const clock = new GameClock(startMs); const frame = clock.capture(nowMs); clock.pause(nowMs); clock.resume(laterMs). (2026-11-05 WB04-C1)
export class GameClock {
    // 시작 밀리초 원점이다.
    #origin;
    // 중첩 정지 깊이이다.
    #pauseDepth = 0;
    // 첫 정지 때 저장한 게임 시각(초)이다.
    #pausedGame = 0;
    // 게임 시각에서 빼는 누적 정지 시간(초)이다.
    #pausedTotal = 0;
    // 마지막으로 고정한 프레임(게임 시각·벽 시각·시간차·번호)이다. 원본 번호는 DWORD 이지만 cpppj 처럼 넓게 센다.
    #frame = { game: 0, wall: 0, delta: 0, number: 0 };

    // 원점 밀리초로 만든다.
    constructor(originMilliseconds = 0) {
        this.#origin = milliseconds(originMilliseconds);
    }

    // 00460d70: 원점부터의 경과를 unsigned 32비트로 구해 (몫 초) + (나머지 × 0.001) 의 double 로 돌려준다.
    wallSeconds(now) {
        const elapsed = (milliseconds(now) - this.#origin) >>> 0;
        return Math.floor(elapsed / 1000) + (elapsed % 1000) * MILLISECOND_SECONDS;
    }

    // 00460cd0: 정지 중이면 저장한 게임 시각, 아니면 벽 시각에서 누적 정지 시간을 뺀 값이다.
    gameSeconds(now) {
        return this.#pauseDepth !== 0 ? this.#pausedGame : this.wallSeconds(now) - this.#pausedTotal;
    }

    // 00460df0: 첫 정지에서만 게임 시각을 저장하고 깊이를 늘린다.
    pause(now) {
        if (this.#pauseDepth === 0xffffffff) throw new RangeError('게임 시계 정지 깊이가 넘쳤습니다.');
        if (this.#pauseDepth === 0) this.#pausedGame = this.gameSeconds(now);
        this.#pauseDepth += 1;
    }

    // 00460e30: 정지 중이 아니면 아무것도 하지 않고, 마지막 정지가 풀릴 때만 (현재 게임 시각 - 저장 게임 시각) + 누적 정지 시간을 누적값으로 쓴다.
    resume(now) {
        if (this.#pauseDepth === 0) return;
        this.#pauseDepth -= 1;
        if (this.#pauseDepth === 0) this.#pausedTotal = (this.gameSeconds(now) - this.#pausedGame) + this.#pausedTotal;
    }

    // 00460e70: 누적 정지 시간만 +0.0 으로 지운다. 원점·정지 깊이·저장 게임 시각·마지막 프레임은 그대로이고 현재 시각을 읽지 않는다.
    reset() {
        this.#pausedTotal = 0;
    }

    // 00460de0: 정지 깊이가 양수인지 돌려준다.
    get paused() { return this.#pauseDepth !== 0; }

    // 00460e90 의 시각 고정 부분: 현재 게임 시각으로 시간차를 구하고 게임/벽 시각을 고정한 뒤 프레임 번호를 늘린다. 고정한 프레임의 사본을 돌려준다.
    capture(now) {
        const game = this.gameSeconds(now);
        this.#frame.delta = game - this.#frame.game;
        this.#frame.game = game;
        this.#frame.wall = this.wallSeconds(now);
        this.#frame.number += 1;
        return { ...this.#frame };
    }

    // 마지막으로 고정한 프레임의 사본이다(다시 계산하지 않는다).
    get frame() { return { ...this.#frame }; }
}
