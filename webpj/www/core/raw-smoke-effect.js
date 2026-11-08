// Smoke.cpp 의 냉각 연기 생성·공통 입자/시간·프레임·색/선 요청 수명을 옮긴 모듈이다(cpppj `o/RawSmokeEffect.cpp/.h`, 10.78 전용: 004a8140 생성자·004a86e0 냉각 wrapper·004a8240 방출·004a82b0 프레임·004a85a0 출력).
//  - buildSmokePalette(): 00487cd0(77, 88)의 양끝 포함 오름차순 팔레트 번호 12개이다(RGB 근사 색을 만들지 않음).
//  - createSmokeFields(): 128바이트 객체의 의미 필드 — registration(공통 form/부모/영역/목록 슬롯/깊이), particles(같은 24바이트 입자 전체 용량, `{ x, y, velocityX, velocityY, life, spare }`),
//    capacity/total(+24/+28, 소멸 뒤에도 숫자는 유지), hasStorage(+2c 유효 저장소 존재), lastTime/elapsed(+30/+38), delta/scaledDelta(+40/+44, 생성자가 쓰지 않음), first(+48),
//    due/windX/windY(+50/+54/+58 다음 방출 시각과 개별 바람), hotspotX/hotspotY/remaining(+5c/+60/+64 포구 오프셋과 남은 방출 수, −1 이면 기한마다 무한 방출),
//    maximumLife(100)/velocity(−1)/spread(9)/period(5)/noise(0.5)(+68..+78 수명/속도/위치 분산/간격/속도 분산)이다.
//  - SmokeRelease: 소멸이 구별하는 입자 버퍼와 객체 자체이다(공유 색 객체는 남김).
//  - new RawSmokeLifetime(pool, registry, state, hooks): state = { palette } 는 같은 월드의 공유 색 객체 토큰(005c7aa8), hooks = { reserve(바이트)→성공 여부(입자 4800·첫 색 객체 8), createPalette()→비영 토큰, buildPalette(token) } 이다.
//    reserve 는 필수, createPalette/buildPalette 는 공유 색이 없을 때(도달 시) 요구한다.
//    initialize(fields, token, request): request = { parent, hotspotX, hotspotY, windX, windY, maximumLife, velocity, spread, period, noise }(createSmokeRequest() 기본값), 공통 등록(TYPE 21)/부착 → 입력 값 → 깊이 −1 → 200입자 → 공유 색 순서이다.
//    initializeCooling(fields, token, parent, [hotspotX, hotspotY]): 기본 생성 뒤 방출 수 10·간격 3 으로 덮어쓴다. destroy(fields, flags, release): 입자 → 등록/base → flags bit 0 객체 순서이다.
//  - new RawSmokeFrame(pool, random, state, palette, hooks): random 은 공유 게임 난수, state 는 불꽃 꼬리와 같은 `{ now, pixelScaleX, pixelScaleY, rate, cameraX, cameraY }`, palette = { count, readable }(색수와 읽을 수 있는 DWORD 목록, 같은 객체 공유),
//    hooks = { invalidate({ left, top, right, bottom }), line({ x1, y1, x2, y2, color }), kill(flags) } 은 모두 필수이다.
//    emit(fields, x, y, life): 빈 슬롯(꼬리 먼저, 가득 차면 사용 범위의 수명 ≤ 0 슬롯, NaN 은 재사용 안 함)을 얻은 경우에만 속도 난수 두 개를 쓴다.
//    step(fields): 최초 호출은 시각/영역만 맞추고 끝난다. 이후에는 전체 delta 한 번 → 방출(남은 수 > 0 이거나 무한 방출 기한) → 사용 꼬리(분열로 늘어난 새 꼬리 포함)의 수명/분열/바람 이동/±1 영역 → 활성 입자가 없으면 kill, 아니면 이전/새 dirty 이다.
//    draw(fields): 좌표를 먼저 정수화한 뒤 카메라를 빼고 색 표 DWORD 의 가로 2픽셀 선을 낸다.
//  정수화는 원본 __ftol 처럼 signed64 절삭 뒤 하위 DWORD 이고 비유한/범위 밖은 0 이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E191: 냉각 연기 이식으로 추가)
import { createRenderProcessFields } from './raw-construction-lifetime.js';
import { createFieryTailParticle } from './raw-fiery-tail-frame.js';

// 실제 005410c4 연기 TYPE·생성자 입자 수·24바이트 stride·공유 색 객체 크기이다. (2026-11-09 WB04-E191)
const SMOKE_TYPE = 21;
const PARTICLE_CAPACITY = 200;
const PARTICLE_BYTES = 24;
const PALETTE_BYTES = 8;
// 00487cd0 의 첫 팔레트 번호와 색 수·생성자 기본 방출 수·냉각 wrapper 의 방출 수/간격이다. (2026-11-09 WB04-E191)
const PALETTE_FIRST = 77;
const PALETTE_COLORS = 12;
const DEFAULT_REMAINING = 8;
const COOLING_REMAINING = 10;
const COOLING_PERIOD = 3;
const TWO_63 = 9223372036854775808;

// 원본 __ftol 하위 DWORD 이다(ToInt32 가 절삭과 2^32 나머지를 같이 수행). 비유한/64비트 넘침은 0 이다. (2026-11-09 WB04-E191)
function smokeInteger(value) {
    if (!Number.isFinite(value) || value >= TWO_63 || value < -TWO_63) return 0;
    return value | 0;
}

// 비정렬 raw float 좌표를 little endian 으로 읽는다. (2026-11-09 WB04-E191)
function readFloat(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return view.getFloat32(0, true);
}

// 실제 연속 팔레트 번호 12개이며 색수 다음 DWORD 를 임의로 덧붙이지 않는다. (2026-11-09 WB04-E191)
export function buildSmokePalette() {
    return Array.from({ length: PALETTE_COLORS }, (_, index) => PALETTE_FIRST + index);
}

// 소멸이 구별하는 입자 버퍼와 scalar deleting destructor 객체이다. (2026-11-09 WB04-E191)
export const SmokeRelease = Object.freeze({ Particles: 'Particles', Object: 'Object' });

// 새 연기 효과의 의미 필드를 만든다(생성자 기본값과 같은 수명/속도/분산/간격). (2026-11-09 WB04-E191)
export function createSmokeFields() {
    return {
        registration: createRenderProcessFields(),
        particles: [], capacity: 0, total: 0, hasStorage: false, lastTime: 0, elapsed: 0, delta: 0, scaledDelta: 0, first: false,
        due: 0, windX: 0, windY: 0, hotspotX: 0, hotspotY: 0, remaining: 0,
        maximumLife: 100, velocity: -1, spread: 9, period: 5, noise: 0.5
    };
}

// 004a8140 의 부모와 설정 가능한 생성 값의 기본 요청이다(float 인자는 같은 binary32 값). (2026-11-09 WB04-E191)
export function createSmokeRequest() {
    return { parent: 0, hotspotX: 0, hotspotY: 0, windX: 0, windY: 0, maximumLife: 100, velocity: -1, spread: 9, period: 5, noise: 0.5 };
}

// 공통 registry 생성/200입자·공유 색 초기화와 반납을 맡는다. registry/state/hooks 는 이 객체보다 오래 살아야 한다. (2026-11-09 WB04-E191)
export class RawSmokeLifetime {
    #registry;
    #state;
    #hooks;

    // 다른 판본/월드나 실제 확보 경계 누락은 등록 전에 거부한다. (2026-11-09 WB04-E191)
    constructor(pool, registry, state, hooks) {
        if (pool?.edition !== 'patch1078' || registry?.pool !== pool || !state || typeof hooks?.reserve !== 'function') throw new TypeError('냉각 연기 실제 월드/확보 경계 오류');
        this.#registry = registry;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 공통 등록/부착 뒤 입력 값·200입자·공유 표를 초기화한다. 생성자가 쓰지 않는 delta/scaledDelta 는 기존 입력을 보존한다. 원본: 004a8140. (2026-11-09 WB04-E191)
    initialize(fields, token, request) {
        const hooks = this.#hooks;
        const state = this.#state;
        this.#registry.construct(fields.registration, token, request.parent, SMOKE_TYPE);
        fields.capacity = 0;
        fields.total = 0;
        fields.hasStorage = false;
        fields.particles = [];
        fields.lastTime = 0;
        fields.elapsed = 0;
        fields.first = false;
        fields.maximumLife = Math.fround(request.maximumLife);
        fields.noise = Math.fround(request.noise);
        fields.spread = Math.fround(request.spread);
        fields.hotspotY = request.hotspotY | 0;
        fields.velocity = Math.fround(request.velocity);
        fields.hotspotX = request.hotspotX | 0;
        fields.period = Math.fround(request.period);
        fields.registration.zOrder = (fields.registration.zOrder - 1) | 0;
        if (!hooks.reserve(PARTICLE_CAPACITY * PARTICLE_BYTES)) throw new Error('냉각 연기 입자 메모리 확보 실패');
        fields.particles = Array.from({ length: PARTICLE_CAPACITY }, createFieryTailParticle);
        fields.capacity = PARTICLE_CAPACITY;
        fields.hasStorage = true;
        fields.due = 0;
        fields.windX = Math.fround(request.windX);
        fields.windY = Math.fround(request.windY);
        fields.remaining = DEFAULT_REMAINING;
        if (!state.palette) {
            if (typeof hooks.createPalette !== 'function' || typeof hooks.buildPalette !== 'function') throw new Error('냉각 연기 첫 공유 색 표 미연결');
            if (!hooks.reserve(PALETTE_BYTES)) throw new Error('냉각 연기 공유 색 객체 확보 실패');
            state.palette = hooks.createPalette() >>> 0;
            if (!state.palette) throw new Error('냉각 연기 공유 색 객체 생성 실패');
            hooks.buildPalette(state.palette);
        }
    }

    // 원본 wrapper 는 기본 생성/공유 표 초기화가 끝나야 방출 수 10·간격 3 을 덮어쓴다. 원본: 004a86e0. (2026-11-09 WB04-E191)
    initializeCooling(fields, token, parent, hotspot) {
        const request = createSmokeRequest();
        request.parent = parent;
        request.hotspotX = hotspot[0] | 0;
        request.hotspotY = hotspot[1] | 0;
        this.initialize(fields, token, request);
        fields.remaining = COOLING_REMAINING;
        fields.period = COOLING_PERIOD;
    }

    // 공통 소멸은 숫자 카운터를 다시 쓰지 않는다. 실제 배열만 반납하고 공유 색은 남긴다. (2026-11-09 WB04-E191)
    destroy(fields, flags, release) {
        if (typeof release !== 'function') throw new TypeError('냉각 연기 실제 반납 경계 미연결');
        if (fields.hasStorage) {
            release(SmokeRelease.Particles);
            fields.particles = [];
            fields.hasStorage = false;
        }
        this.#registry.destroy(fields.registration);
        if (flags & 1) release(SmokeRelease.Object);
    }
}

// 실제 단일 프레임/방출/출력 몸체이다(004a8240/004a82b0/004a85a0). (2026-11-09 WB04-E191)
export class RawSmokeFrame {
    #pool;
    #random;
    #state;
    #palette;
    #hooks;

    // 출력/삭제가 실제 같은 세계에 연결되었는지 생성 때 확인한다. (2026-11-09 WB04-E191)
    constructor(pool, random, state, palette, hooks) {
        if (pool?.edition !== 'patch1078' || typeof random?.next !== 'function' || !state || !palette
            || typeof hooks?.invalidate !== 'function' || typeof hooks.line !== 'function' || typeof hooks.kill !== 'function') throw new TypeError('냉각 연기 판본/실제 출력·삭제 미연결');
        this.#pool = pool;
        this.#random = random;
        this.#state = state;
        this.#palette = palette;
        this.#hooks = hooks;
    }

    // 손상된 숫자/소유 배열은 변경 전에 거부한다. (2026-11-09 WB04-E191)
    #validate(fields) {
        if (fields.capacity < 0 || fields.total < 0 || fields.total > fields.capacity || fields.capacity !== fields.particles.length) throw new RangeError('냉각 연기 현재 입자 용량/꼬리 오류');
    }

    // capacity 꼬리를 먼저 쓰고 가득 차면 사용한 범위의 수명 ≤ 0 슬롯만 찾는다(NaN/양수 수명은 유지). (2026-11-09 WB04-E191)
    #append(fields) {
        if (fields.total < fields.capacity) {
            const particle = fields.particles[fields.total];
            fields.total += 1;
            return particle;
        }
        // 0/음수 수명만 재사용한다.
        for (let index = 0; index < fields.total; index += 1) if (fields.particles[index].life <= 0) return fields.particles[index];
        return null;
    }

    // 원본 00455930 의 상위 16비트와 넓은 중간값이다. (2026-11-09 WB04-E191)
    #randomRange(low, high) {
        return this.#random.next(65536) * (1.0 / 65536.0) * Math.abs(high - low) + low;
    }

    // 실패한 확보는 난수/기존 입자를 바꾸지 않는다. 마지막 spare DWORD 도 유지한다. 원본: 004a8240. (2026-11-09 WB04-E191)
    emit(fields, x, y, life) {
        if (!fields.hasStorage) return;
        this.#validate(fields);
        const particle = this.#append(fields);
        if (particle) {
            particle.velocityX = Math.fround(this.#randomRange(-fields.noise, fields.noise) + fields.velocity);
            particle.velocityY = Math.fround(this.#randomRange(-fields.noise, fields.noise) + fields.velocity);
            particle.x = x;
            particle.y = y;
            particle.life = life;
        }
    }

    // 같은 공통 dirty helper 의 여유/카메라 계산이다. (2026-11-09 WB04-E191)
    #invalidate(left, top, right, bottom) {
        const state = this.#state;
        this.#hooks.invalidate({ left: smokeInteger((left - 1) - state.cameraX), top: smokeInteger((top - 1) - state.cameraY), right: smokeInteger((right + 1) - state.cameraX), bottom: smokeInteger((bottom + 1) - state.cameraY) });
    }

    // 실제 단일 프레임의 전체 delta 를 쓰며 별도 시간 조각 반복을 추가하지 않는다. 원본: 004a82b0. (2026-11-09 WB04-E191)
    step(fields) {
        if (!fields.hasStorage) return;
        this.#validate(fields);
        const pool = this.#pool;
        const state = this.#state;
        const parent = fields.registration.parent;
        if (parent < 0 || parent >= pool.capacity) throw new RangeError('냉각 연기 현재 부모 SID 오류');
        const raw = pool.slot(parent & 0xffff);
        const parentX = Math.fround(readFloat(raw, 14) * state.pixelScaleX);
        const parentY = Math.fround(readFloat(raw, 18) * state.pixelScaleY);
        if (!fields.first) {
            fields.lastTime = state.now;
            fields.elapsed = 0;
            fields.registration.left = parentX;
            fields.registration.right = parentX;
            fields.registration.top = parentY;
            fields.registration.bottom = parentY;
            fields.first = true;
            return;
        }
        // 00418454 의 FADD QWORD 는 기존 누적 시각을 float 로 줄이지 않는다.
        const difference = (state.now - fields.lastTime) * 10.0;
        fields.delta = Math.fround(difference);
        fields.scaledDelta = Math.fround(difference * state.rate);
        fields.lastTime = state.now;
        fields.elapsed += fields.delta;
        if (fields.remaining > 0 || (fields.remaining === -1 && fields.due <= fields.elapsed)) {
            // y 위치 난수를 x 보다 먼저 소비한다.
            const spread = Math.fround(fields.spread * 0.5);
            const y = Math.fround(this.#randomRange(-spread, spread) + fields.hotspotY + parentY);
            const x = Math.fround(this.#randomRange(-spread, spread) + fields.hotspotX + parentX);
            this.emit(fields, x, y, fields.maximumLife);
            const elapsed = fields.elapsed;
            const noise = this.#randomRange(0, Math.fround(fields.period * 0.25));
            fields.due = Math.fround(fields.period * 0.75 + noise + elapsed);
            if (fields.remaining !== -1) fields.remaining = (fields.remaining - 1) | 0;
        }
        let active = false;
        let left = parentX;
        let top = parentY;
        let right = parentX;
        let bottom = parentY;
        // total 은 분열 중 늘 수 있다. 새 꼬리 입자도 같은 프레임에서 순서대로 처리한다.
        for (let index = 0; index < fields.total; index += 1) {
            const particle = fields.particles[index];
            if (!(particle.life > 0)) continue;
            particle.life = Math.fround(particle.life - fields.delta);
            // 수명이 2 를 넘을 때만 분열 난수를 소비한다.
            if (particle.life > 2 && this.#randomRange(0, 3) < fields.delta) {
                this.emit(fields, particle.x, particle.y, particle.life);
                particle.life = Math.fround(particle.life * 0.8);
            }
            particle.x = Math.fround((particle.velocityX + fields.windX) * fields.scaledDelta + particle.x);
            particle.y = Math.fround((particle.velocityY + fields.windY) * fields.scaledDelta + particle.y);
            const x = smokeInteger(particle.x);
            const y = smokeInteger(particle.y);
            const lowX = Math.fround((x - 1) | 0);
            const lowY = Math.fround((y - 1) | 0);
            const highX = Math.fround((x + 1) | 0);
            const highY = Math.fround((y + 1) | 0);
            // 첫 활성 입자는 영역을 덮어쓰고 이후에는 std::min/max 비교 방향으로 넓힌다.
            if (active) {
                left = lowX < left ? lowX : left;
                top = lowY < top ? lowY : top;
                right = right < highX ? highX : right;
                bottom = bottom < highY ? highY : bottom;
            } else {
                left = lowX;
                top = lowY;
                right = highX;
                bottom = highY;
            }
            active = true;
        }
        if (!active) {
            this.#hooks.kill(0);
            return;
        }
        const rect = fields.registration;
        this.#invalidate(rect.left, rect.top, rect.right, rect.bottom);
        this.#invalidate(left, top, right, bottom);
        rect.left = left;
        rect.top = top;
        rect.right = right;
        rect.bottom = bottom;
    }

    // 원본 색수/float 역수 조회이다. 색수 다음 DWORD 도 공급된 메모리 안에서만 읽는다. (2026-11-09 WB04-E191)
    #color(life, maximum) {
        const palette = this.#palette;
        if (!Number.isFinite(maximum) || maximum <= 0 || palette.count < 0) throw new RangeError('냉각 연기 색 표 분모/개수 오류');
        const reciprocal = Math.fround(1.0 / maximum);
        const index = Math.max(0, smokeInteger(Math.fround(palette.count) * (maximum < life ? maximum : life) * reciprocal));
        if (index >= palette.readable.length) throw new RangeError('냉각 연기 색 표 인접 DWORD 미공급');
        return palette.readable[index] >>> 0;
    }

    // 원문은 좌표를 먼저 정수화한 뒤 카메라/가로 1픽셀을 적용한다. 원본: 004a85a0. (2026-11-09 WB04-E191)
    draw(fields) {
        if (!fields.hasStorage) return;
        this.#validate(fields);
        const state = this.#state;
        // 사용한 꼬리 범위의 현재 양수 수명만 제출한다.
        for (let index = 0; index < fields.total; index += 1) {
            const particle = fields.particles[index];
            if (!(particle.life > 0)) continue;
            const x = Math.fround(smokeInteger(particle.x) - state.cameraX);
            const y = smokeInteger(smokeInteger(particle.y) - state.cameraY);
            const color = this.#color(particle.life, fields.maximumLife);
            this.#hooks.line({ x1: smokeInteger(x), y1: y, x2: smokeInteger(x + 1), y2: y, color });
        }
    }
}
