// 원본 10.78 의 두 난수 스트림(CRT rand·게임 난수)과 게임 난수 시드 초기화, 클라이언트가 장기 소유하는 난수 전역을 옮긴 모듈이다.
// 근거: cpppj `o/CrtRandom.cpp`(004e64ab), `o/SpStore.cpp` 의 GameRandom(004558c0), `o/RawGameRandomSeed.cpp`(00455970 Initialize, 004559d0 Reseed),
//       `client/ClientRandomGlobals.h`(CRT holdrand 1·게임 난수 0x37·mode·Fort 핵심 시드·Subscriber 초기화 DWORD), 원본 관측 `cpppj/tests/fixtures/campaign-seed-x86.tsv`.
// C++ 의 `uint32_t&` 공유는 JS 에서 같은 객체를 함께 빌리는 방식으로 옮겼다. 상태를 복사한 별도 스트림을 만들지 않는다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-05 WB04-C1: 세션 장기 상태의 난수 부분 이식으로 추가)

// MSVC CRT rand 의 곱·덧셈 상수이다(214013, 2531011). (2026-11-05 WB04-C1)
const CRT_MULTIPLIER = 0x343fd;
const CRT_INCREMENT = 0x269ec3;

// 게임 난수의 0 상태 대체값과 곱·덧셈 상수이다(원본 004558c0). (2026-11-05 WB04-C1)
const GAME_ZERO_REPLACEMENT = 0x0bad0bad;
const GAME_MULTIPLIER = 0x10003;
const GAME_INCREMENT = 3;

// 난수 모드 0 의 고정 CRT 시드이다(재현 가능한 미션 시작). (2026-11-05 WB04-C1)
export const FIXED_RANDOM_SEED = 0x38d535;

// 시드 초기화가 CRT 에서 버리는 값의 수이다. 이어 세 값을 더 뽑는다. (2026-11-05 WB04-C1)
export const SEED_WARMUP = 100;

// 클라이언트 생성 때 게임 난수의 초기 상태이다(보호 PE 00532710 의 data 초기값 0x37). (2026-11-05 WB04-C1)
export const INITIAL_GAME_RANDOM_STATE = 55;

// 32비트 부호 없는 정수인지 검사한다. (2026-11-05 WB04-C1)
function uint32(value, name) {
    if (!Number.isInteger(value) || value < 0 || value > 0xffffffff) throw new RangeError(`${name} 는 32비트 부호 없는 정수여야 합니다.`);
    return value;
}

// MSVC CRT rand 스트림이다(원본 004e64ab). 상태 = 상태 × 214013 + 2531011 의 하위 32비트, 반환은 상위 15비트(0~32767)이다.
// 0 도 유효한 상태이며 게임 난수의 0 대체 규칙을 쓰지 않는다. 같은 CRT 를 쓰는 소비자(시드·지형·파편 등)는 같은 객체를 빌린다.
// 사용: const crt = new CrtRandom(1); crt.next() === 41. (2026-11-02 WB03-X 지형용으로 처음 추가, 2026-11-05 WB04-C1 core 로 옮기고 setState 추가)
export class CrtRandom {
    // 시드를 32비트 부호 없는 정수로 받는다(음수 시각 시드도 같은 비트로 받는다).
    constructor(seed = 1) {
        // 현재 holdrand 상태이다. 기존 지형 코드가 직접 읽으므로 공개 속성으로 둔다.
        this.state = seed >>> 0;
    }

    // 상태를 한 번 전진시키고 0~32767 의 값을 돌려준다.
    next() {
        this.state = (Math.imul(this.state, CRT_MULTIPLIER) + CRT_INCREMENT) >>> 0;
        return (this.state >>> 16) & 0x7fff;
    }

    // 원본 srand 처럼 상태를 바꾼다. 0 도 그대로 쓴다.
    setState(state) {
        this.state = uint32(state, 'CRT 상태');
    }
}

// 원본 게임 난수이다(004558c0). 상태가 0 이면 먼저 0x0bad0bad 로 바꾸고, 상태 = 상태 × 0x10003 + 3 의 하위 32비트, 반환은 상위 16비트 % limit 이다.
// limit 0 은 원본의 0 나눗셈이므로 거부한다. SP·공격·생성·AI 등이 같은 객체를 빌린다.
// 사용: const game = new GameRandom(55); game.next(100). (2026-11-05 WB04-C1)
export class GameRandom {
    // 현재 상태를 받는다.
    #state;

    // 초기 상태로 만든다. 0 은 첫 호출에서 대체값으로 바뀐다.
    constructor(state = 0) {
        this.#state = uint32(state, '게임 난수 상태');
    }

    // 상태를 한 번 전진시키고 [0, limit) 값을 돌려준다. limit 는 1~0xffffffff 정수이다.
    next(limit) {
        if (!Number.isInteger(limit) || limit < 1 || limit > 0xffffffff) throw new RangeError('게임 난수의 limit 는 1 이상 32비트 정수여야 합니다.');
        if (this.#state === 0) this.#state = GAME_ZERO_REPLACEMENT;
        this.#state = (Math.imul(this.#state, GAME_MULTIPLIER) + GAME_INCREMENT) >>> 0;
        return (this.#state >>> 16) % limit;
    }

    // 진단/기계어 대조용 현재 상태이다(스트림을 전진시키지 않는다).
    get state() { return this.#state; }

    // 시드 초기화(00455970/004559d0)가 만든 상태를 넣는다.
    setState(state) {
        this.#state = uint32(state, '게임 난수 상태');
    }
}

// 클라이언트가 장기 소유하는 난수 전역이다(cpppj ClientRandomGlobals). 새 클라이언트에서만 CRT 1·게임 난수 55·나머지 0 으로 만들고,
// 세션/미션 생성·반납으로 다시 초기화하지 않는다. 소비자는 이 객체의 같은 crt/game 을 빌리고 DWORD 칸(gameMode 등)을 직접 읽고 쓴다.
// 사용: const globals = new ClientRandomGlobals(); new GameRandomSeed(globals, () => unixSeconds).initialize(). (2026-11-05 WB04-C1)
export class ClientRandomGlobals {
    // 같은 스레드의 CRT holdrand 스트림이다(최초 1). 별도 srand 를 실행하지 않는다.
    crt = new CrtRandom(1);
    // 메뉴 구름·현재 세션이 함께 쓰는 단일 게임 난수 객체이다(최초 0x37).
    game = new GameRandom(INITIAL_GAME_RANDOM_STATE);
    // 원본 005c89a8 난수 모드 DWORD 이다. 0 만 고정 시드를 고른다.
    gameMode = 0;
    // 원본 005c89b4 Fort 핵심 시드 DWORD 이다. 원본 정리는 하위 WORD 만 지운다(소비자 연결은 후속 단위).
    fortCoreSeed = 0;
    // 원본 005c89a4 Subscriber ID 최초 시드 여부 DWORD 이다(소비자 연결은 후속 단위).
    subscriberIdInitialized = 0;
}

// 게임 난수 시드 초기화기이다(원본 00455970 Initialize, 004559d0 Reseed). globals 의 crt·game·gameMode 를 같은 객체로 빌리고,
// unixSeconds() 는 원본 __time32(NULL) 의 signed 32비트 Unix 초를 돌려주는 함수이다(모드 0 에서는 부르지 않는다).
// 사용: const seed = new GameRandomSeed(globals, () => Math.floor(Date.now() / 1000) | 0); seed.initialize(). (2026-11-05 WB04-C1)
export class GameRandomSeed {
    // 빌린 난수 전역이다.
    #globals;
    // Unix 초 공급 함수이다.
    #unixSeconds;

    // 전역과 시각 공급자를 받는다. 시각 공급자가 없으면 실제 세션 초기화 전에 거부한다.
    constructor(globals, unixSeconds) {
        if (!(globals?.crt instanceof CrtRandom) || !(globals.game instanceof GameRandom)) throw new TypeError('GameRandomSeed 에는 같은 crt/game 을 가진 난수 전역이 필요합니다.');
        if (typeof unixSeconds !== 'function') throw new TypeError('게임 난수의 Unix 초 공급자가 없습니다.');
        this.#globals = globals;
        this.#unixSeconds = unixSeconds;
    }

    // 00455970: 모드 0 이면 고정 시드, 아니면 현재 Unix 초를 CRT 상태로 쓰고 100개를 버린 뒤 세 값의 32비트 곱을 게임 난수 상태로 넣는다. 그 값을 돌려준다.
    initialize() {
        const globals = this.#globals;
        let seed = FIXED_RANDOM_SEED;
        if (globals.gameMode !== 0) {
            const seconds = this.#unixSeconds();
            if (!Number.isInteger(seconds) || seconds < -0x80000000 || seconds > 0x7fffffff) throw new RangeError('Unix 초는 signed 32비트 정수여야 합니다.');
            seed = seconds >>> 0;
        }
        globals.crt.setState(seed);
        // 원본과 같은 100회 소비를 수행하며 반환값은 쓰지 않는다.
        for (let index = 0; index < SEED_WARMUP; index += 1) globals.crt.next();
        const first = globals.crt.next();
        const second = globals.crt.next();
        const third = globals.crt.next();
        const value = Math.imul(Math.imul(first, second), third) >>> 0;
        globals.game.setState(value);
        return value;
    }

    // 004559d0: 모드를 1 로 잠시 바꿔 시각 시드로 초기화한 뒤 원래 모드 DWORD 를 되돌린다. 공급자 예외 때도 모드를 되돌린다.
    reseed() {
        const globals = this.#globals;
        const previous = globals.gameMode;
        globals.gameMode = 1;
        try {
            return this.initialize();
        } finally {
            globals.gameMode = previous;
        }
    }
}
