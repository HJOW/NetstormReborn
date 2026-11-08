// Flyingshrapnel.cpp 의 픽셀 파편 묶음 생성·중력/수명 프레임·현재 색 픽셀 출력을 옮긴 모듈이다(cpppj `o/RawShrapnelFrame.cpp/.h`, 10.78 전용: 00460000 준비·0045fc70 와 세 가상 묶음 생성기·004600a0 묶음 분할·0045fd60 프레임·0045fef0 출력).
//  - ShrapnelKind: 원본 세 가상 픽셀 생성기 번호이다(0 폭발·1 붕괴·2 셋째, 셋째의 의미 이름은 추측하지 않는다). 번호는 수명 생성자의 중력 표 첨자로도 쓴다.
//  - createShrapnelFrameFields(): 104바이트 효과 객체의 의미 필드 — registration(공통 표시 등록·부모·영역, `createRenderProcessFields()`), particles(용량 전체 24바이트 입자, 미사용 0 입자 포함),
//    total(+0x14 실제 사용 꼬리 수, 죽은 슬롯을 재사용하지 않음), lastTime/elapsed(+0x30/+0x38 마지막 시각과 누적 10배 시간), delta/scaledDelta(+0x40/+0x44 한 프레임 간격과 표시 배율 값),
//    first(+0x48 공통 생성자가 0 으로 쓰고 이 프레임은 검사하지 않음), image(+0x50..+0x60 원래 색을 유지하는 캡처 `{ width, height, hotspotX, hotspotY, pixels }`), gravity(+0x64 폭발 6·붕괴 float 0.24·셋째 float 1.2)이다.
//  - createShrapnelParticle(): 24바이트 입자 `{ x, y, velocityX, velocityY, life, pixel }`. pixel 은 0 이면 미사용, 나머지는 같은 캡처 그림의 offset+1 토큰(원본 색 주소)이다.
//  - new RawShrapnelFrame(pool, game, crt, state, hooks): pool 은 현재 부모 raw 좌표/깊이의 같은 PATCH SidPool, game 은 폭발 속도의 게임 난수, crt 는 묶음 크기/수명의 원본 CRT 난수(두 스트림을 합치지 않음),
//    state = { now, pixelScaleX(16), pixelScaleY(11), rate(1), cameraX, cameraY } 는 같은 Kernel 의 현재 전역(불꽃 꼬리와 같은 형식, 호출자가 현재 값으로 갱신),
//    hooks = { reserve(바이트)→성공 여부, release(scratch: true 작업 복사본/false 이전 입자 저장소), invalidate({ left, top, right, bottom }), pixel({ x, y, color }), removeForm() } 는 모두 필수이다.
//  - prepare(fields, kind): 부모 raw 위치 × 배율 − 캡처 기준점 절삭 → 불투명(255 가 아닌) 픽셀 수(최소 1)의 용량 확보 → 작업 복사본에서 행/열 순서로 아직 고르지 않은 픽셀마다 가상 생성기 → 작업 복사본 반납 → 타이머 초기화이다. 캡처 원본은 바꾸지 않는다.
//  - step(fields): 간격 제한 없이 한 시간 조각으로 용량 전체의 진입 양수 수명 입자를 중력/속도로 갱신하고 ±3 영역을 모은다. 진입 양수 입자가 없으면 removeForm 만 부르고, 있으면 이전/새 영역 dirty 를 요청한다.
//  - draw(fields): 사용 꼬리의 양수 수명 입자만 현재 캡처 색으로 픽셀 요청을 낸다(x 는 카메라 감산 뒤 float 저장, y 는 넓은 값 그대로 절삭).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E188: 파편 프레임 이식으로 추가)
import { createRenderProcessFields } from './raw-construction-lifetime.js';

// 원본 세 가상 픽셀 생성기 번호이다(수명 생성자의 중력 표 첨자). (2026-11-09 WB04-E188)
export const ShrapnelKind = Object.freeze({ Explosion: 0, Collapse: 1, Variant2: 2 });

// binary32 비트를 float 값으로 바꾼다(PE 상수를 계산 없이 옮길 때 쓴다). (2026-11-09 WB04-E188)
export function shrapnelFloatFromBits(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// PE 의 실제 binary32 CRT 단위(0x38000100)와 붕괴 최소 수명(0x3dcccccd)이다. (2026-11-09 WB04-E188)
const CRT_UNIT = shrapnelFloatFromBits(0x38000100);
const COLLAPSE_MINIMUM = shrapnelFloatFromBits(0x3dcccccd);
// 실제 binary64 상수: 게임 난수 단위·폭발 퍼짐·붕괴 폭(PE binary32 2.4 의 넓은 표현)·10배 시간이다. (2026-11-09 WB04-E188)
const GAME_UNIT = 1.0 / 65536.0;
const SPREAD = 0.3;
const COLLAPSE_WIDTH = 2.4000000953674316;
const TIME_SCALE = 10;
// 입자 24바이트 stride·투명 표시 byte·영역 여유·signed DWORD 한계이다. (2026-11-09 WB04-E188)
const PARTICLE_BYTES = 24;
const TRANSPARENT = 255;
const MARGIN = 3;
const INT32_MAX = 2147483647;
const TWO_31 = 2147483648;

// 새 효과 객체의 의미 필드를 만든다(중력은 폭발 기본 6, 캡처는 비어 있음). (2026-11-09 WB04-E188)
export function createShrapnelFrameFields() {
    return {
        registration: createRenderProcessFields(), particles: [], total: 0, lastTime: 0, elapsed: 0, delta: 0, scaledDelta: 0, first: false,
        image: { width: 0, height: 0, hotspotX: 0, hotspotY: 0, pixels: new Uint8Array(0) }, gravity: 6
    };
}

// 미사용 0 입자 한 개를 만든다(준비 단계의 용량 확보와 시험 입력이 같이 쓴다). (2026-11-09 WB04-E188)
export function createShrapnelParticle() {
    return { x: 0, y: 0, velocityX: 0, velocityY: 0, life: 0, pixel: 0 };
}

// 정상 유한 signed32 범위만 원본 CRT 처럼 0 방향으로 정수화한다. (2026-11-09 WB04-E188)
function ftol(value) {
    if (!Number.isFinite(value) || value < -TWO_31 || value >= TWO_31) throw new RangeError('픽셀 파편 정수화 범위 오류');
    return Math.trunc(value) | 0;
}

// 비정렬 raw float 를 실제 little endian DWORD 로 읽는다. (2026-11-09 WB04-E188)
function readFloat(raw, offset) {
    return shrapnelFloatFromBits(raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24));
}

// 손상된 캡처 치수/용량/꼬리는 상태 변경 전에 거부한다. (2026-11-09 WB04-E188)
function validate(fields) {
    const { width, height, pixels } = fields.image;
    if (!Number.isInteger(width) || !Number.isInteger(height) || width < 1 || height < 1 || width * height > INT32_MAX || width * height !== pixels.length
        || fields.particles.length > Math.floor(INT32_MAX / PARTICLE_BYTES) || !Number.isInteger(fields.total) || fields.total < 0 || fields.total > fields.particles.length) {
        throw new RangeError('픽셀 파편 이미지/용량/꼬리 오류');
    }
}

// 픽셀 파편 프레임/출력 몸체이다. 빈 성공 경계를 허용하지 않고 현재 10.78 본체만 선택한다. (2026-11-09 WB04-E188)
export class RawShrapnelFrame {
    #pool;
    #game;
    #crt;
    #state;
    #hooks;

    // 같은 풀·두 난수·전역과 실제 메모리/표시/소진 경계를 연결한다(모든 참조는 이 객체보다 오래 살아야 함). (2026-11-09 WB04-E188)
    constructor(pool, game, crt, state, hooks) {
        if (pool?.edition !== 'patch1078' || typeof game?.next !== 'function' || typeof crt?.next !== 'function' || !state
            || typeof hooks?.reserve !== 'function' || typeof hooks.release !== 'function' || typeof hooks.invalidate !== 'function'
            || typeof hooks.pixel !== 'function' || typeof hooks.removeForm !== 'function') throw new TypeError('픽셀 파편 원본/외부 경계 누락');
        this.#pool = pool;
        this.#game = game;
        this.#crt = crt;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 현재 효과 부모 번호를 확인하고 raw 사본을 읽는다(원본 Slot 의 1..capacity−1 범위). (2026-11-09 WB04-E188)
    #parent(fields) {
        const parent = fields.registration.parent;
        if (!Number.isInteger(parent) || parent < 1 || parent >= this.#pool.capacity) throw new RangeError('픽셀 파편 부모 번호 오류');
        return this.#pool.slot(parent);
    }

    // 실제 00455930 의 상위 16비트/넓은 간격이다. 끝점 역순(20/14)도 원문처럼 low 에 절댓값 간격을 더한다. (2026-11-09 WB04-E188)
    #random(low, high) {
        return this.#game.next(65536) * GAME_UNIT * Math.abs(high - low) + low;
    }

    // 실제 004600a0: 각 반복 조건마다 CRT 난수를 다시 뽑고(마지막 실패 검사에도 소비) 작업 그림에 먼저 표시해 같은 픽셀을 다음 묶음에서 재사용하지 않는다. (2026-11-09 WB04-E188)
    #split(fields, work, originX, originY, x, y, vx, vy, life) {
        const crt = this.#crt;
        const { width, height } = fields.image;
        let dx = 0;
        let columns = (crt.next() % 3) + 3;
        // 원문은 3..5 범위를 매번 다시 뽑으므로 처음 정한 크기의 사각형과 다르다.
        while (dx < columns) {
            let dy = 0;
            let rows = (crt.next() % 3) + 3;
            // 열 하나 안에서도 행 한도를 반복마다 다시 뽑는다. 첫 행/열(0)은 원문의 > 0 비교로 제외된다.
            while (dy < rows) {
                if (x + dx > 0 && x + dx < width && y + dy > 0 && y + dy < height) {
                    const pixel = (y + dy) * width + x + dx;
                    if (work[pixel] !== TRANSPARENT) {
                        work[pixel] = TRANSPARENT;
                        const remaining = Math.fround(crt.next() * CRT_UNIT + life);
                        if (fields.total >= fields.particles.length) throw new RangeError('픽셀 파편 용량 원본 assert');
                        const particle = fields.particles[fields.total];
                        fields.total += 1;
                        particle.x = Math.fround((dx + originX) + x);
                        particle.y = Math.fround((dy + originY) + y);
                        particle.velocityX = vx;
                        particle.velocityY = vy;
                        particle.life = remaining;
                        particle.pixel = pixel + 1;
                    }
                }
                dy += 1;
                rows = (crt.next() % 3) + 3;
            }
            dx += 1;
            columns = (crt.next() % 3) + 3;
        }
    }

    // 세 가상 생성기의 넓은 수학과 float 인자 저장 위치를 보존한다(폭발은 게임 난수 → CRT 순서). (2026-11-09 WB04-E188)
    #seed(fields, kind, work, originX, originY, x, y) {
        const crt = this.#crt;
        const { width, height } = fields.image;
        let vx = 0;
        let vy = 0;
        let life = 0;
        if (kind === ShrapnelKind.Explosion) {
            const vertical = (y - ((height / 2) | 0)) * SPREAD;
            vy = Math.fround(vertical - this.#random(20, 14));
            life = Math.fround((crt.next() * CRT_UNIT) * 5 + 0.5);
            vx = Math.fround((x - ((width / 2) | 0)) * SPREAD);
        } else if (kind === ShrapnelKind.Collapse) {
            const random = (crt.next() * CRT_UNIT) * COLLAPSE_WIDTH + COLLAPSE_MINIMUM;
            life = Math.fround((random * Math.sqrt((height - y + 1) / height)) * 10);
        } else {
            const ratio = y / height;
            const base = (1 - ratio) * 12 + 12.5;
            vx = Math.fround(ratio);
            life = Math.fround((crt.next() * CRT_UNIT) * Math.abs(3.5 - base) + (base < 3.5 ? base : 3.5));
        }
        this.#split(fields, work, originX, originY, x, y, vx, vy, life);
    }

    // 실제 공통 487ba0 의 1픽셀 여유/카메라 적용이다(중간 float 저장 없이 정수화). (2026-11-09 WB04-E188)
    #invalidate(left, top, right, bottom) {
        const state = this.#state;
        this.#hooks.invalidate({ left: ftol((left - 1) - state.cameraX), top: ftol((top - 1) - state.cameraY), right: ftol((right + 1) - state.cameraX), bottom: ftol((bottom + 1) - state.cameraY) });
    }

    // 캡처 원본은 유지하고 임시 작업 복사본만 0xff 로 바꾼다. kind 는 ShrapnelKind 번호이다. 원본: 00460000. (2026-11-09 WB04-E188)
    prepare(fields, kind) {
        if (kind !== ShrapnelKind.Explosion && kind !== ShrapnelKind.Collapse && kind !== ShrapnelKind.Variant2) throw new RangeError('픽셀 파편 가상 생성기 범위 오류');
        validate(fields);
        const raw = this.#parent(fields);
        const state = this.#state;
        const rect = fields.registration;
        rect.zOrder = (raw[35] << 24) >> 24;
        const x = readFloat(raw, 14) * state.pixelScaleX;
        const y = readFloat(raw, 18) * state.pixelScaleY;
        rect.left = Math.fround(x);
        rect.right = rect.left;
        rect.top = Math.fround(y);
        rect.bottom = rect.top;
        const originX = ftol(x - fields.image.hotspotX);
        const originY = ftol(y - fields.image.hotspotY);
        const pixels = fields.image.pixels;
        let opaque = 0;
        // 투명(255)이 아닌 캡처 픽셀 수가 입자 용량이다(최소 1).
        for (let index = 0; index < pixels.length; index += 1) if (pixels[index] !== TRANSPARENT) opaque += 1;
        const count = Math.max(1, opaque);
        if (fields.particles.length) this.#hooks.release(false);
        if (!this.#hooks.reserve((count * PARTICLE_BYTES) >>> 0)) throw new Error('픽셀 파편 입자 메모리 확보 실패');
        fields.particles = Array.from({ length: count }, createShrapnelParticle);
        fields.total = 0;
        if (!this.#hooks.reserve(pixels.length >>> 0)) throw new Error('픽셀 파편 작업 그림 메모리 확보 실패');
        const work = Uint8Array.from(pixels);
        const { width, height } = fields.image;
        // 원본 행/열 순서로 아직 선택되지 않은 픽셀마다 가상 생성기를 호출한다.
        for (let row = 0; row < height; row += 1) {
            // 같은 작업 그림의 현재 값을 매번 다시 읽는다(앞 묶음이 0xff 로 바꾼 픽셀은 건너뜀).
            for (let column = 0; column < width; column += 1) {
                if (work[row * width + column] !== TRANSPARENT) this.#seed(fields, kind, work, originX, originY, column, row);
            }
        }
        this.#hooks.release(true);
        fields.lastTime = state.now;
        fields.elapsed = 0;
    }

    // 한 프레임의 delta/가속도를 먼저 저장하고 y 갱신/dirty 에는 넓은 새 속도/좌표를 쓴다. 원본: 0045fd60. (2026-11-09 WB04-E188)
    step(fields) {
        validate(fields);
        const raw = this.#parent(fields);
        const state = this.#state;
        const x = Math.fround(readFloat(raw, 14) * state.pixelScaleX);
        const y = Math.fround(readFloat(raw, 18) * state.pixelScaleY);
        let left = x;
        let top = y;
        let right = x;
        let bottom = y;
        const wide = (state.now - fields.lastTime) * TIME_SCALE;
        fields.delta = Math.fround(wide);
        fields.scaledDelta = Math.fround(wide * state.rate);
        fields.lastTime = state.now;
        fields.elapsed += fields.delta;
        const acceleration = Math.fround(fields.gravity * fields.delta);
        let alive = false;
        // 사용 꼬리가 아닌 용량 전체에서 진입 양수 수명만 갱신한다(NaN 수명은 건너뜀).
        for (const particle of fields.particles) {
            if (!(particle.life > 0)) continue;
            alive = true;
            particle.life = Math.fround(particle.life - fields.delta);
            const vy = acceleration + particle.velocityY;
            particle.velocityY = Math.fround(vy);
            particle.x = Math.fround(particle.velocityX * fields.scaledDelta + particle.x);
            const nextY = vy * fields.scaledDelta + particle.y;
            particle.y = Math.fround(nextY);
            const ix = ftol(particle.x);
            const iy = ftol(nextY);
            // std::min/max 의 비교 방향(새 값 < 현재 / 현재 < 새 값)을 그대로 쓴다.
            const nextLeft = Math.fround(ix - MARGIN);
            const nextTop = Math.fround(iy - MARGIN);
            const nextRight = Math.fround(ix + MARGIN);
            const nextBottom = Math.fround(iy + MARGIN);
            left = nextLeft < left ? nextLeft : left;
            top = nextTop < top ? nextTop : top;
            right = right < nextRight ? nextRight : right;
            bottom = bottom < nextBottom ? nextBottom : bottom;
        }
        if (!alive) {
            this.#hooks.removeForm();
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

    // 사용 꼬리의 생존 픽셀만 현재 캡처 색으로 출력한다. 캡처 원본을 바꾸지 않는다. 원본: 0045fef0. (2026-11-09 WB04-E188)
    draw(fields) {
        validate(fields);
        const state = this.#state;
        const pixels = fields.image.pixels;
        // 실제 사용 꼬리까지만 순회하고 양수 수명만 고른다.
        for (let index = 0; index < fields.total; index += 1) {
            const particle = fields.particles[index];
            if (!(particle.life > 0)) continue;
            if (!particle.pixel || particle.pixel > pixels.length) throw new RangeError('픽셀 파편 색 주소 토큰 오류');
            const x = Math.fround(ftol(particle.x) - state.cameraX);
            const y = ftol(particle.y) - state.cameraY;
            this.#hooks.pixel({ x: ftol(x), y: ftol(y), color: pixels[particle.pixel - 1] });
        }
    }
}
