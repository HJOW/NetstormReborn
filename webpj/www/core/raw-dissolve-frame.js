// 애시드 포획의 120바이트 Dissolve(분해) 효과를 옮긴 모듈이다(cpppj `o/RawDissolveFrame.cpp/.h`, 10.78 전용: 00450460 캡처 뒤 초기화·004500a0 준비·0044fe40 프레임·0044ffc0 출력).
//  입자/캡처/공통 등록의 배치는 픽셀 파편(`raw-shrapnel-frame.js`)과 같고 Dissolve 고유 꼬리(불투명 수·시계 시작·입자 확보 여부)를 따로 둔다.
//  - createDissolveFrameFields(): `{ common, opaqueCount, clockStarted, particlesAllocated }`. common 은 파편과 같은 의미 필드(+0x00..+0x60, 중력은 쓰지 않음),
//    opaqueCount(+0x70 투명색 255 를 뺀 원본 픽셀 수), clockStarted(+0x74 첫 프레임에서만 시계를 현재 시각으로 맞추는 상태),
//    particlesAllocated(new 0 도 비영 포인터를 반납하므로 용량 0 과 실제 확보 여부를 구분)이다.
//  - new RawDissolveFrame(pool, random, state, hooks): pool 은 현재 부모 더미와 두 폴이 공유하는 같은 PATCH SidPool, random 은 다른 생산/공격과 공유하는 게임 난수,
//    state = { now, pixelScaleX(16), pixelScaleY(11), rate(1), cameraX, cameraY } 는 같은 Kernel 의 현재 전역, hooks = { reserve, release, invalidate, pixel, removeForm } 은 파편과 같은 실제 경계로 모두 필수이다.
//  - initialize(fields, first, second, xOriented): 캡처 뒤 현재 부모(더미)의 좌표/깊이로 영역을 정하고 캡처 기준점을 뺀 원점으로 prepare 를 부른다.
//  - prepare(fields, originX, originY, first, second, xOriented): 두 폴 위치에서 양쪽 간격·가로 오프셋을 구하고, 불투명 픽셀마다 열 바깥/행 안쪽 순서로 게임 난수 두 번(속도 잡음 → 방향 흔들림)을 써서 속도/수명을 만든다.
//  - step(fields): 최초 호출에서만 시계를 현재 시각으로 맞춰 delta 0 을 적용하고, 용량 전체의 진입 양수 수명 입자를 무중력으로 이동해 영역(+1)을 모은다. 진입 양수 입자가 없으면 removeForm 만 부른다.
//  - draw(fields): 사용 꼬리의 양수 수명 입자만 캡처 색으로 픽셀 요청을 낸다.
//  정수화는 원본 CRT 처럼 signed64 절삭 뒤 하위 DWORD 를 쓰고, 유한하지 않거나 ±2^63 밖이면 0(원본 _ftol 의 불확정 반환 경계)이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E189: Dissolve 프레임 이식으로 추가)
import { createShrapnelFrameFields, shrapnelFloatFromBits } from './raw-shrapnel-frame.js';

// 원본 게임 난수 단위·폴 y 보정(508c40)·10배 시간(418430)이다. (2026-11-09 WB04-E189)
const RANDOM_UNIT = 1.0 / 65536.0;
const POLE_Y = 36.3;
const TIME_SCALE = 10.0;
// 508c48 폴 x 보정·004500a0 의 난수 상한/속도 float 상수이다(binary32 값의 넓은 표현). (2026-11-09 WB04-E189)
const POLE_X = 20.799999237060547;
const MAXIMUM_RANDOM = 1.2999999523162842;
const VELOCITY = 10.0;
// 투명 표시 byte·입자 stride·signed DWORD 한계·_ftol 불확정 경계이다. (2026-11-09 WB04-E189)
const TRANSPARENT = 255;
const PARTICLE_BYTES = 24;
const INT32_MAX = 2147483647;
const TWO_63 = 9223372036854775808;

// 새 Dissolve 효과의 의미 필드를 만든다. (2026-11-09 WB04-E189)
export function createDissolveFrameFields() {
    return { common: createShrapnelFrameFields(), opaqueCount: 0, clockStarted: false, particlesAllocated: false };
}

// 원본 CRT 의 signed64 절삭 뒤 하위 DWORD 이다(ToInt32 가 절삭과 2^32 나머지를 같이 수행). 범위 밖/비유한 값은 0 이다. (2026-11-09 WB04-E189)
function truncate(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return value | 0;
}

// 비정렬 raw float 를 실제 little endian DWORD 로 읽는다. (2026-11-09 WB04-E189)
function readFloat(raw, offset) {
    return shrapnelFloatFromBits(raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24));
}

// 메모리/픽셀을 만지기 전에 같은 원본 이미지와 실제 사용 꼬리 범위를 확인한다. (2026-11-09 WB04-E189)
function validate(fields) {
    const value = fields.common;
    const { width, height, pixels } = value.image;
    if (!Number.isInteger(width) || !Number.isInteger(height) || width < 1 || height < 1 || width * height > INT32_MAX || width * height !== pixels.length
        || !Number.isInteger(value.total) || value.total < 0 || value.total > value.particles.length) throw new RangeError('Dissolve 캡처/입자 범위 오류');
}

// Dissolve 준비/프레임/출력 몸체이다. 빈 출력/삭제 효과로 정상 소진을 대신하지 않는다. (2026-11-09 WB04-E189)
export class RawDissolveFrame {
    #pool;
    #random;
    #state;
    #hooks;

    // 같은 풀·게임 난수·전역과 실제 출력 경계를 연결한다(모든 참조는 이 객체보다 오래 살아야 함). (2026-11-09 WB04-E189)
    constructor(pool, random, state, hooks) {
        if (pool?.edition !== 'patch1078' || typeof random?.next !== 'function' || !state
            || typeof hooks?.reserve !== 'function' || typeof hooks.release !== 'function' || typeof hooks.invalidate !== 'function'
            || typeof hooks.pixel !== 'function' || typeof hooks.removeForm !== 'function') throw new TypeError('Dissolve 판본/실제 출력 경계 미연결');
        this.#pool = pool;
        this.#random = random;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 더미/폴의 현재 raw 좌표다(하위 생성 뒤 현재 슬롯을 다시 읽는다). 원본 SID 는 WORD 이므로 하위 16비트만 쓴다. (2026-11-09 WB04-E189)
    #position(sid) {
        const raw = this.#pool.slot(sid & 0xffff);
        return [readFloat(raw, 14), readFloat(raw, 18)];
    }

    // 00455930(1, 1.3)의 넓은 반환값이며 게임 난수를 한 번 소비한다. 중간 float 저장은 호출자가 한다. (2026-11-09 WB04-E189)
    #noise() {
        return this.#random.next(65536) * RANDOM_UNIT * (MAXIMUM_RANDOM - 1.0) + 1.0;
    }

    // 004558f0(−width, width)의 반열린 범위다. 최소 폭은 원본 max(1, half/2)가 보장한다. (2026-11-09 WB04-E189)
    #jitter(width) {
        return this.#random.next((width * 2) >>> 0) - width;
    }

    // 공통 487ba0 의 1픽셀 여유/카메라다(이전 영역 → 새 영역 순서는 호출자가 지킴). (2026-11-09 WB04-E189)
    #invalidate(left, top, right, bottom) {
        const state = this.#state;
        this.#hooks.invalidate({ left: truncate((left - 1) - state.cameraX), top: truncate((top - 1) - state.cameraY), right: truncate((right + 1) - state.cameraX), bottom: truncate((bottom + 1) - state.cameraY) });
    }

    // 원점은 저장한 사각형 float 가 아닌 원래 배율 곱에서 hotspot 을 뺀다. 원본: 00450460(캡처 뒤 부분). (2026-11-09 WB04-E189)
    initialize(fields, first, second, xOriented) {
        validate(fields);
        const value = fields.common;
        const state = this.#state;
        const parent = value.registration.parent & 0xffff;
        const point = this.#position(parent);
        const x = point[0] * state.pixelScaleX;
        const y = point[1] * state.pixelScaleY;
        value.registration.zOrder = (this.#pool.slot(parent)[35] << 24) >> 24;
        value.registration.left = Math.fround(x);
        value.registration.right = value.registration.left;
        value.registration.top = Math.fround(y);
        value.registration.bottom = value.registration.top;
        this.prepare(fields, truncate(x - value.image.hotspotX), truncate(y - value.image.hotspotY), first, second, xOriented);
    }

    // 같은 캡처의 불투명 픽셀마다 한 입자이며 열 바깥/행 안쪽 순서를 바꾸지 않는다. xOriented 가 0 이 아니면 가로 분해다. 원본: 004500a0. (2026-11-09 WB04-E189)
    prepare(fields, originX, originY, first, second, xOriented) {
        validate(fields);
        const value = fields.common;
        const state = this.#state;
        const a = this.#position(first);
        const b = this.#position(second);
        const ax = Math.fround(a[0] * state.pixelScaleX - POLE_X);
        const ay = a[1] * state.pixelScaleY - POLE_Y;
        const bx = Math.fround(b[0] * state.pixelScaleX) - POLE_X;
        const by = Math.fround(Math.fround(b[1] * state.pixelScaleY) - POLE_Y);
        const gapA = Math.fround(xOriented ? originX - ax : originY - ay);
        const gapB = Math.fround(xOriented ? bx - originX : by - originY);
        const transverse = truncate(xOriented ? ay - originY : ax - originX);
        const { width, height, pixels } = value.image;
        let opaque = 0;
        // 투명색 255 를 뺀 실제 원본 픽셀 수가 입자 용량이다(0 도 허용).
        for (let index = 0; index < pixels.length; index += 1) if (pixels[index] !== TRANSPARENT) opaque += 1;
        fields.opaqueCount = opaque;
        if (fields.particlesAllocated) this.#hooks.release(false);
        if (!this.#hooks.reserve((opaque * PARTICLE_BYTES) >>> 0)) throw new Error('Dissolve 입자 메모리 확보 실패');
        value.particles = Array.from({ length: opaque }, () => ({ x: 0, y: 0, velocityX: 0, velocityY: 0, life: 0, pixel: 0 }));
        value.total = 0;
        fields.particlesAllocated = true;
        const halfX = (width / 2) | 0;
        const halfY = (height / 2) | 0;
        const jitterX = Math.max(1, (halfX / 2) | 0);
        const jitterY = Math.max(1, (halfY / 2) | 0);
        // 원본은 열 단위로 게임 난수를 소비한다.
        for (let x = 0; x < width; x += 1) {
            // 투명색은 난수를 소비하지 않는다.
            for (let y = 0; y < height; y += 1) {
                const pixel = y * width + x;
                if (pixels[pixel] === TRANSPARENT) continue;
                const noise = this.#noise();
                let vx;
                let vy;
                let life;
                if (xOriented) {
                    const speed = Math.fround((Math.abs(x - halfX) / halfX + noise) * VELOCITY);
                    if (this.#jitter(jitterX) + x < halfX) {
                        vx = -speed;
                        life = Math.fround((x + gapA) / speed);
                    } else {
                        vx = speed;
                        life = Math.fround(((width - x) + gapB - width) / speed);
                    }
                    vy = Math.fround(((transverse - y) | 0) / life);
                } else {
                    const speed = Math.fround((Math.abs(y - halfY) / halfY + noise) * VELOCITY);
                    if (this.#jitter(jitterY) + y < halfY) {
                        vy = -speed;
                        life = Math.fround((y + gapA) / speed);
                    } else {
                        vy = speed;
                        life = Math.fround(((height - y) + gapB - height) / speed);
                    }
                    vx = Math.fround(((transverse - x) | 0) / life);
                }
                const particle = value.particles[value.total];
                value.total += 1;
                particle.x = Math.fround((originX + x) | 0);
                particle.y = Math.fround((originY + y) | 0);
                particle.velocityX = vx;
                particle.velocityY = vy;
                particle.life = life;
                particle.pixel = pixel + 1;
            }
        }
    }

    // 최초 프레임에서는 생성 시각과 무관하게 시계를 현재 시각으로 맞춰 delta 0 을 적용한다. 원본: 0044fe40. (2026-11-09 WB04-E189)
    step(fields) {
        validate(fields);
        const value = fields.common;
        const state = this.#state;
        const parent = this.#position(value.registration.parent);
        let left = Math.fround(parent[0] * state.pixelScaleX);
        let top = Math.fround(parent[1] * state.pixelScaleY);
        let right = left;
        let bottom = top;
        if (!fields.clockStarted) {
            value.lastTime = state.now;
            value.elapsed = 0;
            fields.clockStarted = true;
        }
        const wide = (state.now - value.lastTime) * TIME_SCALE;
        value.delta = Math.fround(wide);
        value.scaledDelta = Math.fround(wide * state.rate);
        value.lastTime = state.now;
        value.elapsed += value.delta;
        let alive = false;
        // 원본 프레임은 사용 꼬리가 아닌 용량 전체를 검사한다.
        for (const particle of value.particles) {
            if (!(particle.life > 0)) continue;
            alive = true;
            particle.life = Math.fround(particle.life - value.delta);
            particle.x = Math.fround(particle.velocityX * value.scaledDelta + particle.x);
            const nextY = particle.velocityY * value.scaledDelta + particle.y;
            particle.y = Math.fround(nextY);
            const x = Math.fround(truncate(particle.x));
            const y = Math.fround(truncate(nextY));
            const nextRight = Math.fround(x + 1);
            const nextBottom = Math.fround(y + 1);
            // std::min/max 의 비교 방향을 그대로 쓴다.
            left = x < left ? x : left;
            top = y < top ? y : top;
            right = right < nextRight ? nextRight : right;
            bottom = bottom < nextBottom ? nextBottom : bottom;
        }
        if (!alive) {
            this.#hooks.removeForm();
            return;
        }
        const rect = value.registration;
        this.#invalidate(rect.left, rect.top, rect.right, rect.bottom);
        this.#invalidate(left, top, right, bottom);
        rect.left = left;
        rect.top = top;
        rect.right = right;
        rect.bottom = bottom;
    }

    // 원본 현재 수명/현재 캡처 색과 x 만 중간 float 에 저장하는 순서를 유지한다. 원본: 0044ffc0. (2026-11-09 WB04-E189)
    draw(fields) {
        validate(fields);
        const value = fields.common;
        const state = this.#state;
        const pixels = value.image.pixels;
        // 사용 꼬리의 살아 있는 원본 픽셀마다 정수 요청을 낸다.
        for (let index = 0; index < value.total; index += 1) {
            const particle = value.particles[index];
            if (!(particle.life > 0)) continue;
            if (!particle.pixel || particle.pixel > pixels.length) throw new RangeError('Dissolve 색 주소 토큰 오류');
            const x = Math.fround(truncate(particle.x) - state.cameraX);
            const y = truncate(particle.y) - state.cameraY;
            this.#hooks.pixel({ x: truncate(x), y: truncate(y), color: pixels[particle.pixel - 1] });
        }
    }
}
