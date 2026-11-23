// Client의 시각 공급원·게임 시계·공유 프레임 값을 같은 수명으로 연결한다.
// 사용: new ClientFrameClock(source, local.gameTicks()); 매 Client 프레임에서 captureFrame()을 한 번 부른다.
import { GameClock } from './game-clock.js';

// 원본 Client의 최초 시계 생성, 현재 보정 DWORD 연결 및 프레임 고정을 제공한다.
export class ClientFrameClock {
    // 같은 시각 공급원·게임 시계와 주소가 바뀌지 않는 현재 프레임이다.
    #source; #clock; #frame = { game: 0, wall: 0, delta: 0, number: 0 };

    // 시각 원점을 현재 밀리초에서 읽고 기존 Client의 보정 상자를 그대로 빌린다.
    constructor(source, synchronizationTicks) {
        if (!source || ['milliseconds', 'unixSeconds', 'advanceFixed'].some((name) => typeof source[name] !== 'function')) throw new TypeError('Client 시각 공급원 누락');
        this.#source = source;
        this.#clock = new GameClock(source.milliseconds());
        this.#clock.bindSynchronizationTicks(synchronizationTicks);
    }

    // 고정 시각 검사는 먼저 한 프레임 전진하고 현재 밀리초를 한 번 읽어 같은 프레임 객체를 갱신한다.
    captureFrame() { this.#source.advanceFixed(); Object.assign(this.#frame, this.#clock.capture(this.#source.milliseconds())); return this.#frame; }
    // 고정 시각 검사 전용: 프레임을 고정하지 않고(프레임 번호·게임/벽 시각·시간차를 갱신하지 않고) 가상 시계만 한 프레임 전진한다. 원본 시작 그림 구간은 시각만 흐르고 주 루프 프레임으로는 세지 않는 것을 맞추려는 것이다. (2026-11-23 WB04-E1271)
    advanceWithoutCapture() { this.#source.advanceFixed(); }
    // 현재 프레임은 새 시계를 읽지 않고 같은 장기 객체를 빌린다.
    time() { return this.#frame; }
    // 프레임 고정과 별도로 선택된 현재 호스트 시각을 읽는다.
    milliseconds() { return this.#source.milliseconds(); }
    // 난수 초기화 등에서 같은 공급원의 현재 유닉스 초를 읽는다.
    unixSeconds() { return this.#source.unixSeconds(); }
    // 시간 동기화 응답과 벽시계는 프레임 사본이 아닌 현재 시각으로 계산한다.
    gameSeconds() { return this.#clock.gameSeconds(this.#source.milliseconds()); }
    wallSeconds() { return this.#clock.wallSeconds(this.#source.milliseconds()); }
    // 중첩 게임 정지의 진입·복귀와 누적 정지 초기화를 같은 시계에 전달한다.
    pushGamePause() { this.#clock.pause(this.#source.milliseconds()); }
    // 마지막 중첩 정지가 해제되는 시각을 같은 공급원에서 읽는다.
    popGamePause() { this.#clock.resume(this.#source.milliseconds()); }
    // 누적 정지 시간만 초기화한다.
    resetGameClock() { this.#clock.reset(); }
    // 현재 게임 시계가 중첩 정지 중인지 돌려준다.
    paused() { return this.#clock.paused; }
}
