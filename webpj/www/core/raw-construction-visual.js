// 건설 표시(Build.cpp 의 건설 중 건물 입자 효과) 생성·버퍼 준비·좌표 동기화·초기 입자 선택을 옮긴 모듈이다(cpppj `o/RawConstructionVisual.cpp/.h`, 10.78 전용).
//  - RawConstructionVisual.create(parent, instant): 부모 유효성 검사(패치 004afd00→004ac040→004ad730) → 0x88 객체 확보 → 부착 경계 → 공유 색 표(최초 한 번 8색 파랑→흰색·3색 흰색→회색 띠)
//    → 수명 범위(일반 1.3~2, 즉시 2~9) → SHP 캡처 → 출력 버퍼를 투명(0xff)으로 채움 → synchronize.
//  - synchronize(fields): 부모 raw 의 zOrder BYTE(부호 확장)와 월드 좌표×배율로 표시 사각형 두 모서리를 맞추고 seed 를 다시 실행한다.
//  - seed(fields): 불투명 수·최소 500칸 확보·마지막 픽셀 복사/제거·중앙 열부터 교차하는 열 순서의 하단 첫 불투명 픽셀 선택(수명은 공유 게임 난수)·열 수만큼 두 난수로 섞기. selected 는 초기화하지 않는다.
//  - makeConstructionVisualHooks(pool, visual, store, hooks): 건설 시작의 표시 효과 경계를 create 에 연결하고 만든 초기 상태를 store 에 넘긴다.
// 원본이 new 실패 뒤 null 을 계속 쓰는 경로는 옮기지 않고 거부(예외)한다. 이 모듈은 DOM 을 쓰지 않는다. 부착(RenderProcess 등록)은 RawConstructionLifetime 의 레지스트리 경계가 맡는다.
// 실제 월드/클라이언트가 소유하는 전역(공유 색 표·타입·배율)은 호출자가 가진 state 객체를 빌린다. (2026-11-08 WB04-E128: RawConstructionVisual 이식으로 추가)

// 원본 객체/색 표/입자 확보 크기와 최소 입자 수·불투명 비율의 분모이다. (2026-11-08 WB04-E128)
const OBJECT_BYTES = 0x88;
const PALETTE_BYTES = 8;
const PARTICLE_BYTES = 12;
const MIN_PARTICLES = 500;
const PIXEL_RATIO = 6;

// 투명 팔레트 항목과 상태 비트이다(free=1·dead=2·void=4). (2026-11-08 WB04-E128)
const TRANSPARENT = 0xff;
const FREE = 1;
const DEAD = 2;
const VOID = 4;

// 부모 raw 슬롯의 위치이다: next=+4(WORD)·head=+6(WORD)·type=+10·state=+11·x=+14(float)·y=+18(float)·owner=+34·zOrder=+35. (2026-11-08 WB04-E128)
const NEXT = 4;
const HEAD = 6;
const TYPE = 10;
const STATE = 11;
const X = 14;
const Y = 18;
const OWNER = 34;
const Z = 35;

// 실제 호출의 첫 색 띠(파랑→흰색)와 두 번째 띠(흰색→128/255 회색), 공통 보정값이다. (2026-11-08 WB04-E128)
const BLUE = Object.freeze([0, 0, 1]);
const WHITE = Object.freeze([1, 1, 1]);
const GRAY = Object.freeze([128 / 255, 128 / 255, 128 / 255]);
const NOISE = 2.4;

// 일반 건설의 최소 수명은 원본 float32 0x3fa66666(약 1.3)이다. 즉시 건설은 2~9초이다. (2026-11-08 WB04-E128)
const NORMAL_MIN = Math.fround(1.2999999523162842);
const NORMAL_MAX = 2;
const INSTANT_MIN = 2;
const INSTANT_MAX = 9;

// int32 상한이다. (2026-11-08 WB04-E128)
const INT32_MAX = 2147483647;

// 필수 경계 이름이다. 하나라도 없으면 첫 효과 전에 거부한다. (2026-11-08 WB04-E128)
const HOOK_NAMES = ['reserve', 'attach', 'createPalette', 'gradient', 'buildPalette', 'capture', 'releaseParticles'];

// 공유 색 표/현재 타입/월드→화면 좌표 배율이다(패치 005410b4·005453cc·0059a924/28). 사용: const state = createConstructionVisualState(). (2026-11-08 WB04-E128)
export function createConstructionVisualState() {
    return { processType: 17, palette: 0, scaleX: 16, scaleY: 11 };
}

// 0x88 바이트 건설 표시 객체의 복원한 필드이다(미초기화 영역은 가상의 의미로 채우지 않는다). 사용: const fields = createConstructionVisualFields(). (2026-11-08 WB04-E128)
export function createConstructionVisualFields() {
    return {
        parent: 0, // +8: 표시할 부모 SID 이다. 부착 경계가 바꿀 수 있다.
        instant: false, // 파생 가상 표: 즉시 건설은 2~9, 일반 건설은 1.3~2 수명 범위이다.
        zOrder: 0, // +0x20: 부모 zOrder BYTE 를 signed 로 확장한 값이다.
        left: 0, top: 0, right: 0, bottom: 0, // +0xc..+0x18: 표시 갱신 사각형(float)이다.
        lastTime: 0, elapsed: 0, // +0x30/+0x38: 프레임 시각/누적 시각이다.
        palette: 0, // +0x48: 공유 입자 색 표 호스트 항목이다.
        image: { width: 0, height: 0, hotspotX: 0, hotspotY: 0, pixels: new Uint8Array(0) }, // +0x4c/+0x54..+0x60: 캡처 픽셀·크기·hotspot 이다.
        output: new Uint8Array(0), // +0x50: 투명으로 채운 표시 버퍼이다.
        lastRow: 0, // +0x64: (height-1)*width 이다.
        minimum: 0, maximum: 0, // +0x6c/+0x70: 입자 수명 범위(float)이다.
        selected: 0, opaque: 0, active: 0, // +0x74/+0x78/+0x80: 누적 선택·불투명 픽셀·활성 입자 수이다.
        firstDraw: false, // +0x7c: 첫 RunFrame 의 시각 helper 초기화 여부이다.
        capacity: 0, total: 0, // +0x24/+0x28: 확보한 입자 수와 현재 초기 입자 수이다.
        particles: [], // +0x2c: capacity 개의 { active, color, life, pixel } 이다. 쓰지 않은 입자는 모두 0 이다.
    };
}

// 입자 한 칸의 빈 값이다. (2026-11-08 WB04-E128)
function emptyParticle() {
    return { active: 0, color: 0, life: 0, pixel: 0 };
}

// 정렬과 무관하게 리틀엔디언 정수(폭 1~4)를 읽는다. (2026-11-08 WB04-E128)
function readUnsigned(raw, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 지정 폭만 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// raw 의 단정도 float 를 읽는다. (2026-11-08 WB04-E128)
function readFloat(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, readUnsigned(raw, offset), true);
    return view.getFloat32(0, true);
}

// 부호 있는 BYTE 로 확장한다. (2026-11-08 WB04-E128)
function signedByte(value) {
    return (value << 24) >> 24;
}

// 패치 004afd00→004ac040(1)→004ad730: owner<10·유효한 살아 있는 자산·공간/종속 form 상태를 확인한다. (2026-11-08 WB04-E128)
function validatePatch(pool, parent) {
    const raw = pool.slot(parent);
    const vtable = readUnsigned(raw, 0);
    if (!parent || raw[OWNER] >= 10 || !vtable || vtable === 0xdddddddd || vtable === 0xcdcdcdcd || raw[TYPE] < 70 || (raw[STATE] & (FREE | DEAD | VOID))) {
        throw new Error('건설 표시 부모 유효성 오류');
    }
    let current = readUnsigned(raw, HEAD, 2);
    const visited = new Set();
    // 종속은 free/void 가 아닌 form 이어야 한다. dead 는 별도 제외하지 않으며 순환 링크는 호스트에서 거부한다.
    while (current) {
        if (current >= pool.capacity || visited.has(current)) throw new Error('건설 표시 종속 체인 오류');
        visited.add(current);
        const child = pool.slot(current);
        if (child[TYPE] >= 70 || (child[STATE] & (FREE | VOID))) throw new Error('건설 표시 종속 상태 오류');
        current = readUnsigned(child, NEXT, 2);
    }
}

// 건설 표시 생성/초기화 몸체이다. 사용: const visual = new RawConstructionVisual(pool, state, random, hooks); const fields = visual.create(parentSid, instant).
// pool·state·random 은 이 객체보다 오래 살아야 하며 random 은 다른 게임 기능과 공유하는 GameRandom(next(limit))이다. (2026-11-08 WB04-E128)
export class RawConstructionVisual {
    // 읽기 전용으로 노출하는 부모 raw 풀이다.
    #pool;
    // 공유 색 표/현재 타입/좌표 배율이다.
    #state;
    // 공유 게임 난수이다.
    #random;
    // 부착·캡처·팔레트·확보 경계이다.
    #hooks;

    // 미연결 표시 경계는 확보나 부모 변경 전에 거부한다. (2026-11-08 WB04-E128)
    constructor(pool, state, random, hooks) {
        for (const name of HOOK_NAMES) {
            if (typeof hooks?.[name] !== 'function') throw new TypeError('건설 표시 경계 누락');
        }
        this.#pool = pool;
        this.#state = state;
        this.#random = random;
        this.#hooks = { ...hooks };
    }

    // 어댑터가 같은 월드를 쓰는지 확인할 때 쓰는 풀이다. (2026-11-08 WB04-E128)
    get pool() { return this.#pool; }

    // 원본이 실패한 포인터를 계속 사용하는 경로는 안전한 예외로 치환한다. (2026-11-08 WB04-E128)
    #reserve(bytes) {
        if (!this.#hooks.reserve(bytes >>> 0)) throw new Error('건설 표시 메모리 확보 실패');
    }

    // 원본 두 파생 생성자의 수명 지정 이후에 캡처와 좌표/입자 초기화를 수행한다. (2026-11-08 WB04-E128)
    create(parent, instant) {
        validatePatch(this.#pool, parent);
        this.#reserve(OBJECT_BYTES);
        const fields = createConstructionVisualFields();
        fields.parent = parent;
        this.#hooks.attach(fields, this.#state.processType);
        // 공통 생성자의 필드는 RenderProcess 부착이 끝난 뒤 초기화한다. 사각형/부모는 상위 부착의 값이다.
        fields.capacity = 0;
        fields.total = 0;
        fields.particles = [];
        fields.lastTime = 0;
        fields.elapsed = 0;
        fields.image.pixels = new Uint8Array(0);
        fields.selected = 0;
        fields.opaque = 0;
        fields.active = 0;
        fields.firstDraw = false;
        if (!this.#state.palette) {
            this.#reserve(PALETTE_BYTES);
            this.#state.palette = this.#hooks.createPalette() >>> 0;
            if (!this.#state.palette) throw new Error('건설 표시 공유 색 표 생성 실패');
            this.#hooks.gradient(8, [...BLUE], [...WHITE], NOISE);
            this.#hooks.gradient(3, [...WHITE], [...GRAY], NOISE);
            this.#hooks.buildPalette(this.#state.palette);
        }
        fields.palette = this.#state.palette;
        fields.instant = Boolean(instant);
        fields.minimum = instant ? INSTANT_MIN : NORMAL_MIN;
        fields.maximum = instant ? INSTANT_MAX : NORMAL_MAX;
        // 버퍼 캡처의 부모 인자는 생성 진입 인자이다. 동기화는 부착 경계가 바꿀 수 있는 현재 fields.parent 를 읽는다.
        const captured = this.#hooks.capture(parent);
        fields.image = {
            width: captured.width | 0,
            height: captured.height | 0,
            hotspotX: captured.hotspotX | 0,
            hotspotY: captured.hotspotY | 0,
            pixels: Uint8Array.from(captured.pixels),
        };
        const count = this.#pixelCount(fields);
        this.#reserve(count);
        fields.lastRow = Math.imul(fields.image.height - 1, fields.image.width);
        fields.output = new Uint8Array(count).fill(TRANSPARENT);
        this.synchronize(fields);
        return fields;
    }

    // 잘못된 크기를 곱하거나 버퍼를 쓰기 전에 호스트의 폭/용량 계약을 검사한다. (2026-11-08 WB04-E128)
    #pixelCount(fields) {
        const { width, height, pixels } = fields.image;
        if (width <= 0 || height <= 0 || width * height > INT32_MAX || width * height !== pixels.length) throw new RangeError('건설 표시 캡처 크기 오류');
        return pixels.length;
    }

    // 현재 부모 zOrder/좌표로 표시 사각형의 두 모서리를 맞춘 뒤 입자를 다시 선택한다. x87 결과가 float 필드에 저장되는 경계에서 단정도 반올림을 보존한다. (2026-11-08 WB04-E128)
    synchronize(fields) {
        const raw = this.#pool.slot(fields.parent);
        fields.zOrder = signedByte(raw[Z]);
        fields.left = Math.fround(Math.fround(this.#state.scaleX) * readFloat(raw, X));
        fields.right = fields.left;
        fields.top = Math.fround(Math.fround(this.#state.scaleY) * readFloat(raw, Y));
        fields.bottom = fields.top;
        this.seed(fields);
    }

    // 원본의 열/행 선택과 순열에 사용되는 모든 난수를 같은 공유 난수에서 차례로 소비한다. selected 는 초기화하지 않는다. (2026-11-08 WB04-E128)
    seed(fields) {
        const count = this.#pixelCount(fields);
        if (fields.output.length !== count) throw new RangeError('건설 표시 출력 크기 오류');
        const pixels = fields.image.pixels;
        let opaque = 0;
        // 투명(0xff)이 아닌 픽셀을 센다.
        for (const pixel of pixels) if (pixel !== TRANSPARENT) opaque += 1;
        fields.opaque = opaque;
        fields.capacity = Math.max(MIN_PARTICLES, Math.floor(opaque / PIXEL_RATIO));
        if (fields.particles.length) this.#hooks.releaseParticles();
        this.#reserve(fields.capacity * PARTICLE_BYTES);
        fields.particles = Array.from({ length: fields.capacity }, emptyParticle);
        fields.total = 0;
        fields.output[count - 1] = pixels[count - 1];
        pixels[count - 1] = TRANSPARENT;
        const { width, height } = fields.image;
        let column = (width - 1) >> 1;
        // 중앙 열부터 0,+1,-2,+3 처럼 교차하며 하단의 첫 불투명 픽셀 하나만 고른다.
        for (let step = 0; step < width && fields.total <= width * 2; step += 1) {
            column += (step & 1) ? step : -step;
            if (column < 0 || column >= width) throw new RangeError('건설 표시 입자 열 범위 오류');
            // height/3 보다 아래인 행만 검사한다. 그 행 자체는 포함하지 않는다.
            for (let row = height - 1; row > (height / 3 | 0); row -= 1) {
                const pixel = row * width + column;
                if (pixels[pixel] === TRANSPARENT) continue;
                if (fields.total < fields.capacity) {
                    const particle = fields.particles[fields.total];
                    fields.total += 1;
                    const fraction = this.#random.next(65536) / 65536;
                    particle.life = Math.fround(fraction * Math.abs(fields.maximum - fields.minimum) + fields.minimum);
                    particle.pixel = pixel;
                    particle.color = pixels[pixel];
                    pixels[pixel] = TRANSPARENT;
                    fields.selected = (fields.selected + 1) >>> 0;
                }
                break;
            }
        }
        if (fields.total) {
            // 열 수만큼 두 난수를 소비하고 입자 12바이트에 해당하는 값 전체를 교환한다.
            for (let index = 0; index < width; index += 1) {
                const first = this.#random.next(fields.total);
                const second = this.#random.next(fields.total);
                const temp = fields.particles[first];
                fields.particles[first] = fields.particles[second];
                fields.particles[second] = temp;
            }
        }
    }
}

// 건설 시작의 표시 효과 경계를 create 에 연결하고 초기 상태의 소유권을 store 에 넘긴다.
// 사용: const connected = makeConstructionVisualHooks(pool, visual, (fields) => visuals.push(fields), hooks). 같은 풀이 아니거나 store 가 없으면 거부한다. (2026-11-08 WB04-E128)
export function makeConstructionVisualHooks(pool, visual, store, hooks) {
    if (pool !== visual.pool || typeof store !== 'function') throw new TypeError('건설 표시 어댑터 연결 오류');
    return { ...hooks, effect: (parent, instant) => { store(visual.create(parent, instant)); } };
}
