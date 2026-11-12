// Client 가 읽는 시각(밀리초 `timeGetTime` 에 해당하는 값과 유닉스 초 `time32`)의 단일 공급원을 옮긴 모듈이다(cpppj `client/ClientTimeSource.h/.cpp`, 기준선 1f00a25a 뒤 2026-11-09 에 추가된 새 파일, 3.6절 ③).
// 기본은 호스트 시계를 그대로 읽는다. **검사 전용 고정 시각 모드**(cpppj `--fixed-frame-ms`)를 켜면 프레임마다 정해진 밀리초만큼만 전진하는 가상 시계를 돌려주어, 같은 입력 단계표의 두 실행이 같은 게임 시각·난수 재시드를 쓰게 한다
// (webpj/dotnetpj 의 L2·L3 결정론 대조용, 원본 게임에는 없는 검사 기능이다).
//  - enableFixed({ frameMilliseconds, originMilliseconds = 1000000, unixSeconds = 946684800 }): 프레임 밀리초는 1~60000, 유닉스 초는 0 이상이어야 하고 이미 켜져 있으면 거부한다.
//  - milliseconds(): 고정 모드는 `(원점 + 가상 경과)` 의 부호 없는 32비트 감김(원본 timeGetTime 과 같음), 아니면 호스트 밀리초(웹 적응: `performance.now()` 의 정수부, 32비트로 감김).
//  - unixSeconds(): 고정 모드는 `설정 초 + floor(경과/1000)`, 아니면 호스트 유닉스 초. time32 범위(0 이상 2^31-1)를 벗어나면 RangeError 이다.
//  - advanceFixed(): 고정 모드에서 가상 시계를 한 프레임만큼 전진한다(꺼져 있으면 아무것도 하지 않음). disableFixedForTests(): 고정 모드를 끄고 경과를 0 으로 되돌린다(단위 시험 정리 전용).
// 웹 적응: C++ 는 프로세스 전역이지만 웹은 Client 가 만든 인스턴스가 보유한다(호스트 시계도 주입 가능). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E336: 4-D ClientSpGlobals 의 시각 공급원으로 추가)

// 고정 모드의 프레임 밀리초 상한과 기본 원점/유닉스 초(2000-01-01 00:00:00 UTC), time32 상한이다. (2026-11-12 WB04-E336)
const MAXIMUM_FRAME_MILLISECONDS = 60000;
const DEFAULT_ORIGIN_MILLISECONDS = 1000000;
const DEFAULT_UNIX_SECONDS = 946684800;
const TIME32_MAXIMUM = 0x7fffffff;

// 호스트 시계의 기본 공급 함수이다(밀리초는 단조 시계, 유닉스 초는 벽시계). (2026-11-12 WB04-E336)
const hostMilliseconds = () => Math.floor(performance.now());
const hostUnixSeconds = () => Math.floor(Date.now() / 1000);

// Client 시각 공급원이다. 사용: const time = new ClientTimeSource(); time.unixSeconds(); 검사에서는 time.enableFixed({ frameMilliseconds: 25 }) 뒤 프레임마다 time.advanceFixed(). (2026-11-12 WB04-E336)
export class ClientTimeSource {
    // 켠 고정 시각 설정이다(frameMilliseconds 0 이면 꺼짐).
    #frameMilliseconds = 0;
    #originMilliseconds = DEFAULT_ORIGIN_MILLISECONDS;
    #fixedUnixSeconds = DEFAULT_UNIX_SECONDS;
    // 원점 뒤 가상 경과 밀리초이다.
    #elapsed = 0;
    // 호스트 시계 공급 함수(시험에서 주입)이다.
    #hostMilliseconds;
    #hostUnixSeconds;

    // 호스트 시계를 주입할 수 있다(기본은 performance.now/Date.now). (2026-11-12 WB04-E336)
    constructor({ hostMilliseconds: milliseconds = hostMilliseconds, hostUnixSeconds: seconds = hostUnixSeconds } = {}) {
        this.#hostMilliseconds = milliseconds;
        this.#hostUnixSeconds = seconds;
    }

    // 고정 시각 모드를 켠다. 범위 밖 값이나 두 번째 호출은 어떤 상태도 바꾸지 않고 거부한다. (2026-11-12 WB04-E336)
    enableFixed({ frameMilliseconds, originMilliseconds = DEFAULT_ORIGIN_MILLISECONDS, unixSeconds = DEFAULT_UNIX_SECONDS }) {
        if (!Number.isInteger(frameMilliseconds) || frameMilliseconds < 1 || frameMilliseconds > MAXIMUM_FRAME_MILLISECONDS) throw new RangeError('고정 시각 프레임 밀리초는 1~60000');
        if (!Number.isInteger(unixSeconds) || unixSeconds < 0) throw new RangeError('고정 시각 유닉스 초는 0 이상');
        if (this.fixedEnabled) throw new Error('고정 시각 모드는 한 번만 켤 수 있음');
        this.#frameMilliseconds = frameMilliseconds;
        this.#originMilliseconds = originMilliseconds >>> 0;
        this.#fixedUnixSeconds = unixSeconds;
        this.#elapsed = 0;
    }

    // 고정 시각 모드를 끄고 가상 경과를 0 으로 되돌린다. 단위 시험의 정리에만 쓴다. (2026-11-12 WB04-E336)
    disableFixedForTests() {
        this.#frameMilliseconds = 0;
        this.#originMilliseconds = DEFAULT_ORIGIN_MILLISECONDS;
        this.#fixedUnixSeconds = DEFAULT_UNIX_SECONDS;
        this.#elapsed = 0;
    }

    // 고정 시각 모드가 켜져 있는지이다(메인 루프의 프레임 상한 대기 생략 판단에 쓴다). (2026-11-12 WB04-E336)
    get fixedEnabled() {
        return this.#frameMilliseconds !== 0;
    }

    // 현재 Client 밀리초이다. 고정 모드는 원점 + 가상 경과(부호 없는 32비트 감김), 아니면 호스트 밀리초(32비트로 감김)이다. (2026-11-12 WB04-E336)
    milliseconds() {
        if (this.#frameMilliseconds) return (this.#originMilliseconds + this.#elapsed) >>> 0;
        return this.#hostMilliseconds() >>> 0;
    }

    // 현재 유닉스 초이다. 고정 모드는 설정 초 + 경과 초(내림), 아니면 호스트 초. time32 범위(0~2^31-1)를 벗어나면 거부한다. (2026-11-12 WB04-E336)
    unixSeconds() {
        const now = this.#frameMilliseconds ? this.#fixedUnixSeconds + Math.floor(this.#elapsed / 1000) : this.#hostUnixSeconds();
        if (!(now >= 0) || now > TIME32_MAXIMUM) throw new RangeError('실제 time32 지원 날짜 범위 초과');
        return now | 0;
    }

    // 고정 모드에서 가상 시계를 한 프레임만큼 전진한다. 꺼져 있으면 아무것도 하지 않는다. (2026-11-12 WB04-E336)
    advanceFixed() {
        if (this.#frameMilliseconds) this.#elapsed += this.#frameMilliseconds;
    }
}
