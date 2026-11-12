// Commander 의 70개 시간 표본과 지연/송수신 카운터의 장기 보관 구조 `ClockSynchronizationData` 만 옮긴 모듈이다(cpppj `o/RawClockSynchronization.h`, 10.78 전용, 기준선 1f00a25a 뒤 추가된 새 파일, 3.6절 ③).
// 원문 BSS 표본과 진단값을 서버/Commander 의 서로 다른 초기화 범위로 장기 보유한다.
//  - latencyMilliseconds(005c8bf0)·sent(005c8bf8)·received(005c8bfc): 단방향 지연·송신·수신 현재 DWORD. next(005c8c08): 현재 벽시각 기한(double), 0 은 예약 없음.
//  - samples(005c8c10): 실제 70×32BYTE 이다. 헤더/미작성 4BYTE 도 각 표본 안에 보존한다.
// 아직 옮기지 않은 것: `RawClockSynchronization` 본체(서버 응답 등록 004c1e70·Commander 연결 004c1eb0·수신 004c1f60·Poll 004c2070)는 시간 동기화 단위(연결/송신 장치가 필요)에서 옮긴다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E335: 4-D ClientLocalStateGlobals 의 하위 소유자로 데이터 구조만 추가)

// 표본 개수와 표본 한 칸의 바이트 수이다. (2026-11-12 WB04-E335)
export const CLOCK_SAMPLE_COUNT = 70;
export const CLOCK_SAMPLE_BYTES = 32;

// 시간 동기화의 장기 보관 데이터이다. 사용: const data = new ClockSynchronizationData(); data.sent = 1; data.samples[0][8] = 0x40. DWORD 필드는 쓰기에서 부호 없는 32비트로 감긴다. (2026-11-12 WB04-E335)
export class ClockSynchronizationData {
    // 지연(밀리초)·송신·수신 카운터 세 DWORD 이다.
    #counters = new Uint32Array(3);
    // 다음 표본 송신 벽시각 기한(double)이다. 0 은 예약 없음이다.
    next = 0;
    // 70×32BYTE 표본 표이다(표본마다 독립 Uint8Array).
    samples = Array.from({ length: CLOCK_SAMPLE_COUNT }, () => new Uint8Array(CLOCK_SAMPLE_BYTES));

    get latencyMilliseconds() { return this.#counters[0]; }

    set latencyMilliseconds(next) { this.#counters[0] = next; }

    get sent() { return this.#counters[1]; }

    set sent(next) { this.#counters[1] = next; }

    get received() { return this.#counters[2]; }

    set received(next) { this.#counters[2] = next; }
}
