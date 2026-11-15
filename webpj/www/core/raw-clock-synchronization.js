// Commander 의 70개 시간 표본과 지연/송수신 카운터의 장기 보관 구조 `ClockSynchronizationData` 만 옮긴 모듈이다(cpppj `o/RawClockSynchronization.h`, 10.78 전용, 기준선 1f00a25a 뒤 추가된 새 파일, 3.6절 ③).
// 원문 BSS 표본과 진단값을 서버/Commander 의 서로 다른 초기화 범위로 장기 보유한다.
//  - latencyMilliseconds(005c8bf0)·sent(005c8bf8)·received(005c8bfc): 단방향 지연·송신·수신 현재 DWORD. next(005c8c08): 현재 벽시각 기한(double), 0 은 예약 없음.
//  - samples(005c8c10): 실제 70×32BYTE 이다. 헤더/미작성 4BYTE 도 각 표본 안에 보존한다.
// 일반 등록·서버 초기화·Commander 연결·수신은 RawClockSynchronization 이 같은 데이터와 분배기를 빌려 처리한다.
// 아직 옮기지 않은 것: Poll 004c2070 과 장기 고정 Native 수신기 baseline 승격은 4-D 후속이다.
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

// 시간 패킷과 표본은 32바이트이고 세 시각은 비정렬 little-endian double 이다. (2026-11-15 WB04-E515)
const CLOCK_PACKET_BYTES = 32;

// 선택된 시계·응답 장치만 사용 시점에 요구한다. (2026-11-15 WB04-E515)
function needClockHook(hook) {
    if (typeof hook !== 'function') throw new Error('시간 동기화의 실제 장치 미연결');
    return hook;
}

// 시간 패킷의 물리 범위를 확인하고 같은 바이트의 DataView 를 돌려준다. (2026-11-15 WB04-E515)
function clockView(packet) {
    if (!(packet instanceof Uint8Array) || packet.length < CLOCK_PACKET_BYTES) throw new RangeError('시간 수신의 실제32BYTE 범위');
    return new DataView(packet.buffer, packet.byteOffset, packet.byteLength);
}

// 같은 Client 전역·표본·패킷 분배기에 시간 응답을 연결한다. Poll 과 고정 수신기 승격은 제공하지 않는다. (2026-11-15 WB04-E515)
export class RawClockSynchronization {
    // Client 의 연결·응답 모드·시계 보정을 빌린다.
    #globals;
    // Client 수명의 70표본과 카운터를 빌린다.
    #data;
    // 선택된 시각과 실제 응답 송신 장치를 빌린다.
    #hooks;

    // 생성은 등록이나 현재 저장소의 초기화를 수행하지 않는다. (2026-11-15 WB04-E515)
    constructor(globals, data, hooks = {}) {
        this.#globals = globals;
        this.#data = data;
        this.#hooks = hooks;
    }

    // 일반 분배의 5번 빈 관찰기와 수신기를 등록한다. 수신 바이트는 원문 어댑터처럼 32바이트를 복사한다. (2026-11-15 WB04-E515)
    #register(dispatch) {
        dispatch.observe(5, () => {});
        dispatch.register(5, (_sender, connection, packet) => {
            clockView(packet);
            this.receive(connection, Uint8Array.from(packet.subarray(0, CLOCK_PACKET_BYTES)));
        });
    }

    // 기존 표본·기한·연결은 보존하고 등록 뒤 송수신 수와 응답 모드만 갱신한다. (2026-11-15 WB04-E515)
    initializeServer(dispatch) {
        this.#register(dispatch);
        this.#data.sent = 0;
        this.#data.received = 0;
        this.#globals.replyMode = 1;
    }

    // 등록 뒤 70표본 전체를 지우고 현재 벽시각을 읽어 측정 연결을 준비한다. (2026-11-15 WB04-E515)
    attach(dispatch, connection) {
        this.#register(dispatch);
        // 각 32바이트에는 헤더와 사용하지 않는 자리도 포함된다.
        for (const sample of this.#data.samples) sample.fill(0);
        const now = needClockHook(this.#hooks.wallSeconds)();
        this.#data.sent = 0;
        this.#data.received = 0;
        this.#globals.replyMode = 0;
        this.#data.next = now;
        this.#globals.connection = connection >>> 0;
    }

    // 응답 모드에서는 게임 시각을 돌려주고, 측정 모드에서는 첫 요청 시각 일치 표본을 갱신한다. (2026-11-15 WB04-E515)
    receive(connection, packet) {
        const view = clockView(packet);
        const address = new Uint8Array(20);
        address[0] = 2;
        if (connection.flags & 0x220) address.set(connection.address);
        if (this.#globals.replyMode) {
            const now = needClockHook(this.#hooks.gameSeconds)();
            view.setFloat64(16, now, true);
            needClockHook(this.#hooks.reply)(connection, packet.subarray(0, CLOCK_PACKET_BYTES), (connection.flags & 0x220) ? address : null);
            return;
        }
        const now = needClockHook(this.#hooks.wallSeconds)();
        view.setFloat64(24, now, true);
        // 중복 요청 시각도 첫 일치 표본만 바꾸고 빈 시각도 원문처럼 비교한다.
        for (const sample of this.#data.samples) {
            if (clockView(sample).getFloat64(8, true) === view.getFloat64(8, true)) {
                this.#data.received = (this.#data.received + 1) >>> 0;
                sample.set(packet.subarray(0, CLOCK_PACKET_BYTES));
                return;
            }
        }
    }
}
