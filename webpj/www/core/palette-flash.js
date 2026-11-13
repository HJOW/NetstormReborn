// 원본 Kencloud.cpp 의 번개 팔레트 프로세스(10.78 00470d40~00471820)를 옮긴 모듈이다(cpppj `client/PaletteFlash.cpp/.h`, 10.78 전용). 기본 팔레트의 구름 색 228~245번(18색)을 밝혔다가 단계별로 원래대로 되돌린다.
//  - 상수: 첫 색 228·색 개수 18·단계 64, thunderOffset(요청 밝기)(0 은 기본 밝기 64), thunderDuration(전체화면)(전체화면 0.5초, 창 모드는 진행만 하고 표시하지 않는 1/60초).
//  - PaletteFlash.init(): 벽시계로 마지막 시각을 정하고 간격을 지속/64 로 둔 뒤 현재 팔레트를 두 번 읽어(원래 색·변경 색) 변경 색을 만든다 — offset 0 은 목표 색(solid) 채우기, 그 밖은 각 RGB 에 offset 을 더해 255 로 제한한다
//    (signed 32비트 덧셈, 음수는 아래로 제한하지 않고 byte 로 감긴다). 예약(네 번째) 바이트는 보존한다. 초기화에서 표시하지 않으며 첫 frame 이 시간 진행 뒤 변경 색을 적용한다.
//  - frame(): 경과 시간이 간격보다 엄격히 클 때마다(여러 번 지연돼도 한꺼번에) 각 색 세 채널을 원래 값 쪽으로 1씩 접근시키고 단계를 줄이며 last 에 간격을 반복 더한다(원본 반올림 순서). 단계가 1 미만이거나 64 초과이면
//    전체화면에서만 저장 팔레트 전체를 복구하고 종료(false)한다. 전체화면이면 부분 팔레트를 적용하고 계속(true). 창 모드는 화면 팔레트를 전혀 바꾸지 않는다.
//  - cancel(): 초기화된 전체화면 효과를 중단할 때 저장 팔레트를 복구한다(중복 취소는 무시). 종료 프레임은 initialized 를 먼저 꺼서 두 번째 복구가 일어나지 않는다.
// state = 원래/변경 색 배열(각 256 색의 { red, green, blue, reserved })·간격/마지막/지속·시작/개수/offset/solid/steps/initialized, hooks = { wallSeconds(), fullScreen(), readPalette(start, count, destination), applyPalette(start, count, colors), restorePalette() }.
// 시각은 게임 정지와 무관한 프레임 실시간이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E376: 5-A 팔레트 계열 이식으로 추가, 기준선 뒤 cpppj 변경 없음)

// 번개가 바꾸는 첫 색 번호·색 개수·단계 수, 기본 밝기, 전체화면 지속과 창 모드 지속이다. (2026-11-13 WB04-E376)
export const THUNDER_FIRST_COLOR = 228;
export const THUNDER_COLOR_COUNT = 18;
export const THUNDER_STEPS = 64;
const DEFAULT_OFFSET = 64;
const FULL_SCREEN_DURATION = 0.5;
const WINDOW_DURATION = 1.0 / 60.0;
const PALETTE_SIZE = 256;

// 생성 요청의 밝기 0 은 기본 밝기 64 를 뜻한다(00470fa0 / CD 004b10d0). (2026-11-13 WB04-E376)
export const thunderOffset = (requested) => (requested === 0 ? DEFAULT_OFFSET : requested);

// 생성 당시 전체화면 여부로 수명을 정한다. 창 모드는 진행만 하고 팔레트를 표시하지 않는다. (2026-11-13 WB04-E376)
export const thunderDuration = (fullScreen) => (fullScreen ? FULL_SCREEN_DURATION : WINDOW_DURATION);

// 빈 색 한 칸(RGBQUAD 순서의 red/green/blue/reserved)을 만든다. (2026-11-13 WB04-E376)
export const emptyScreenColor = () => ({ red: 0, green: 0, blue: 0, reserved: 0 });

// 원본 +0xc/+0x40c 의 원래 색/변경 색과 +0x810 이후 진행 상태이다. 색 배열은 부분 팔레트를 앞에서부터 담는다. 사용: new PaletteFlashState({ duration: 0.5 }). (2026-11-13 WB04-E376)
export class PaletteFlashState {
    // 원래 색/변경 색(256칸), 간격·마지막 시각·지속(double), 시작 색 번호·개수·밝기 offset, offset 0 경로의 목표 색, 남은 단계, 초기화 여부이다.
    original = Array.from({ length: PALETTE_SIZE }, emptyScreenColor);
    current = Array.from({ length: PALETTE_SIZE }, emptyScreenColor);
    interval = 0;
    last = 0;
    duration = 0;
    start = THUNDER_FIRST_COLOR;
    count = THUNDER_COLOR_COUNT;
    offset = DEFAULT_OFFSET;
    solid = emptyScreenColor();
    steps = 0;
    initialized = false;

    // 초기 필드 일부를 덮어쓴다.
    constructor(init = {}) { Object.assign(this, init); }
}

// signed 32비트 덧셈·256 상한 비교·byte 저장을 유지한다. 음수 밝기는 아래로 제한하지 않고 byte 로 감긴다. (2026-11-13 WB04-E376)
function brighten(channel, offset) {
    const sum = (channel + offset) | 0;
    return sum < 256 ? sum & 0xff : 255;
}

// 원래 색보다 작으면 1 증가, 크면 1 감소한다. (2026-11-13 WB04-E376)
const approach = (channel, original) => (channel < original ? channel + 1 : channel > original ? channel - 1 : channel);

// 번개 팔레트 효과의 계산과 종료 판정을 맡는다. 소유자는 init 뒤 frame 을 호출하고 중단/파괴 전에 cancel 을 호출한다. 커널 등록·부모 월드 수명은 호출자가 관리한다. (2026-11-13 WB04-E376)
export class PaletteFlash {
    #state;
    #hooks;

    // 필수 경계와 부분 색 범위를 검사한다. OS 자원을 만들지 않는다.
    constructor(state, hooks) {
        if (!hooks || typeof hooks.wallSeconds !== 'function' || typeof hooks.fullScreen !== 'function' || typeof hooks.readPalette !== 'function'
            || typeof hooks.applyPalette !== 'function' || typeof hooks.restorePalette !== 'function') throw new TypeError('Incomplete palette flash hooks');
        if (state.start > PALETTE_SIZE || state.count > PALETTE_SIZE - state.start) throw new RangeError('Palette flash color range');
        this.#state = state;
        this.#hooks = hooks;
    }

    // 원본 00470e30 / CD 004b1310: 현재 색을 두 번 읽고 밝기 변경 또는 목표 색 채우기를 준비한다. 초기화에서 표시하지 않는다.
    init() {
        const state = this.#state;
        state.last = this.#hooks.wallSeconds();
        state.interval = state.duration / 64.0;
        this.#hooks.readPalette(state.start, state.count, state.original);
        this.#hooks.readPalette(state.start, state.count, state.current);
        state.initialized = true;
        // 부분 팔레트만 변경한다. RGBQUAD 의 네 번째 예약 바이트는 기존 값을 보존한다.
        for (let index = 0; index < state.count; index += 1) {
            const color = state.current[index];
            if (state.offset === 0) {
                color.red = state.solid.red;
                color.green = state.solid.green;
                color.blue = state.solid.blue;
                color.reserved = state.solid.reserved;
            } else {
                color.red = brighten(color.red, state.offset);
                color.green = brighten(color.green, state.offset);
                color.blue = brighten(color.blue, state.offset);
            }
        }
        state.steps = THUNDER_STEPS;
    }

    // 원본 00471650 / CD 004b1450: interval 보다 엄격히 큰 경과 시간마다 RGB 를 1씩 복구한다. 계속 진행하면 true 이다.
    // 정상 종료 때 전체화면이면 저장 팔레트를 복구하며, 창 모드에서는 화면 팔레트를 전혀 바꾸지 않는다.
    frame() {
        const state = this.#state;
        if (!state.initialized) return false;
        let elapsed = this.#hooks.wallSeconds() - state.last;
        // 정확히 interval 인 시각에서는 진행하지 않는다. 한 번에 최대 남은 단계만 실행하므로 0/음수 간격에서도 끝난다.
        while (state.interval < elapsed && state.steps > 0) {
            // 각 색의 세 채널을 독립적으로 원래 값에 접근시킨다.
            for (let index = 0; index < state.count; index += 1) {
                const color = state.current[index];
                const original = state.original[index];
                color.red = approach(color.red, original.red);
                color.green = approach(color.green, original.green);
                color.blue = approach(color.blue, original.blue);
            }
            state.steps -= 1;
            state.last += state.interval;
            elapsed -= state.interval;
        }
        if (state.steps < 1 || state.steps > THUNDER_STEPS) {
            if (this.#hooks.fullScreen()) this.#hooks.restorePalette();
            state.initialized = false;
            return false;
        }
        if (this.#hooks.fullScreen()) this.#hooks.applyPalette(state.start, state.count, state.current);
        return true;
    }

    // 원본 00471820 / CD 004b15d0: 초기화된 전체화면 효과를 중단할 때 저장 팔레트를 복구한다. 중복 취소는 무시한다.
    cancel() {
        const state = this.#state;
        if (state.initialized && this.#hooks.fullScreen()) this.#hooks.restorePalette();
        state.initialized = false;
    }
}
