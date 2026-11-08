// Fierytail.cpp 의 실제 입자 프레임과 선 출력 요청을 옮긴 모듈이다(cpppj `o/RawFieryTailFrame.cpp/.h`, 10.78 전용: 004580f0 프레임·004583a0 출력). 11 불꽃과 12 건설 마나(Streak)가 같은 프레임/Draw 를 공유한다.
//  - createFieryTailFrameFields(): 효과 객체의 의미 필드 — registration(현재 Form/부모/표시 영역, `createRenderProcessFields()`), particles(용량 전체 24바이트 입자 배열, 정식 생성자는 120개를 0 으로 채움), total(+0x14 실제 사용 꼬리 수, 죽은 입자를 재사용해도 줄지 않음),
//    lastTime/elapsed(+0x30/+0x38 마지막 시각과 누적 delta), delta/scaledDelta(+0x40/+0x44 10배 시간 조각과 배율 적용 값), first(+0x48 최초 10입자 생성 완료), palette(+0x50 월드 공유 색 표 토큰, 효과 소멸 때 반납하지 않음),
//    maximumLife(+0x54 생성 입자의 수명이자 색 표 분모, 기본 4), previousX/previousY(+0x58/+0x5c 직전 부모 픽셀 좌표), offset(+0x60 후속 방출 좌표와 선 끝에 더하는 정수)이다.
//    입자는 `{ x, y, velocityX, velocityY, life, spare }` 이며 속도/마지막 DWORD 는 이 프레임이 바꾸지 않고 보존한다.
//  - new RawFieryTailFrame(pool, random, state, palette, hooks): pool 은 현재 raw 부모 좌표의 같은 PATCH SidPool, random 은 다른 게임 로직과 공유하는 게임 난수, state = { now, pixelScaleX(16), pixelScaleY(11), rate(1), cameraX, cameraY, diagnostic } 는 같은 Kernel 의 현재 전역(호출자가 현재 값으로 갱신),
//    palette = { count, readable } 는 색수와 읽을 수 있는 DWORD 목록(색수 다음 인접 DWORD 도 공급된 만큼만 읽음, 진단 2 가 count 를 바꾸므로 같은 객체를 공유), hooks = { invalidate({ left, top, right, bottom }), line({ x1, y1, x2, y2, color }) } 는 모두 필수이다.
//  - step(fields): 최초에는 영역 설정과 10입자만 만들고 dirty 도 수명 감소도 없다. 이후에는 시간 조각(최대 0.05초, 시간 후퇴/0 간격도 원문 그대로)마다 2입자 방출 → 용량 전체(사용 꼬리가 아님) 양수 수명 감소와 영역 누적(±10) → 이전/새 영역 dirty 요청을 한다.
//  - draw(fields, endpointX, endpointY): 사용 꼬리까지 양수 수명 입자마다 부모 화면 끝점+offset 까지의 선(색 = 색수 × min(수명, 최대 수명) / 최대 수명 의 색 표 DWORD)을 요청한다. 진단 2 는 색수 0x17655/최대 수명 0x315a 로 바꾼다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E187: 불꽃 꼬리 프레임 이식으로 추가)
import { createRenderProcessFields } from './raw-construction-lifetime.js';

// 실제 타이머 최대 간격(0.5 × 0.1)·시간 배율·입자 영역 여백·float 난수 단위이다. (2026-11-09 WB04-E187)
const MAXIMUM_STEP = 0.5 * 0.1;
const DELTA_SCALE = 10;
const MARGIN = 10;
const RANDOM_UNIT = 1.0 / 65536.0;
// 실제 최초/시간 조각당 방출 수와 진단 2 가 쓰는 두 상수이다. (2026-11-09 WB04-E187)
const INITIAL_PARTICLES = 10;
const STEP_PARTICLES = 2;
const DIAGNOSTIC_COLORS = 0x17655;
const DIAGNOSTIC_LIFE = 0x315a;
const TWO_31 = 2147483648;

// 새 효과 객체의 의미 필드를 만든다(입자 배열은 비어 있고 최대 수명은 불꽃 기본 4 이다). (2026-11-09 WB04-E187)
export function createFieryTailFrameFields() {
    return {
        registration: createRenderProcessFields(), particles: [], total: 0, lastTime: 0, elapsed: 0, delta: 0, scaledDelta: 0, first: false, palette: 0,
        maximumLife: 4, previousX: 0, previousY: 0, offset: 0
    };
}

// 24바이트 입자 한 개를 0 으로 만든다(정식 생성자의 120개 zero 와 시험 입력이 같이 쓴다). (2026-11-09 WB04-E187)
export function createFieryTailParticle() {
    return { x: 0, y: 0, velocityX: 0, velocityY: 0, life: 0, spare: 0 };
}

// 정상 유한 좌표만 원본 CRT 처럼 0 방향으로 정수화한다. (2026-11-09 WB04-E187)
function ftol(value) {
    if (!Number.isFinite(value) || value < -TWO_31 || value >= TWO_31) throw new RangeError('Fiery Tail 정수화 범위 오류');
    return Math.trunc(value) | 0;
}

// 비정렬 raw float 를 실제 little endian DWORD 로 읽는다. (2026-11-09 WB04-E187)
function readFloat(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return view.getFloat32(0, true);
}

// 실제 004239f0 의 배타적 오른쪽/아래쪽 경계(+1)와 ordered 비교를 보존한다. (2026-11-09 WB04-E187)
function include(x, y, box) {
    box.left = box.left < x ? box.left : x;
    box.top = box.top < y ? box.top : y;
    const nextRight = Math.fround(x + 1);
    const nextBottom = Math.fround(y + 1);
    box.right = nextRight < box.right ? box.right : nextRight;
    if (box.bottom <= nextBottom) box.bottom = nextBottom;
}

// 손상된 사용 수와 호스트 크기 넘침은 상태 변경 전에 거부한다. (2026-11-09 WB04-E187)
function validate(fields) {
    if (fields.particles.length > 0x7fffffff || fields.total < 0 || fields.total > fields.particles.length) throw new RangeError('Fiery Tail 현재 입자 수 오류');
}

// 불꽃 꼬리 프레임/출력 몸체이다. 빈 성공 경계를 허용하지 않고 현재 10.78 본체만 선택한다. (2026-11-09 WB04-E187)
export class RawFieryTailFrame {
    #pool;
    #random;
    #state;
    #palette;
    #hooks;

    // 같은 풀·게임 난수·전역·색 표와 실제 출력 경계를 연결한다(모든 참조는 이 객체보다 오래 살아야 함). (2026-11-09 WB04-E187)
    constructor(pool, random, state, palette, hooks) {
        if (pool?.edition !== 'patch1078' || typeof random?.next !== 'function' || !state || !palette
            || typeof hooks?.invalidate !== 'function' || typeof hooks.line !== 'function') throw new TypeError('Fiery Tail 판본/출력 경계 미연결');
        this.#pool = pool;
        this.#random = random;
        this.#state = state;
        this.#palette = palette;
        this.#hooks = hooks;
    }

    // PATCH 00487b40 은 꼬리를 먼저 쓰고 꽉 찬 뒤에만 죽은 슬롯을 앞에서 찾는다. NaN 과 양수 수명은 건너뛴다. (2026-11-09 WB04-E187)
    #append(fields) {
        if (fields.total < fields.particles.length) {
            const particle = fields.particles[fields.total];
            fields.total += 1;
            return particle;
        }
        // 0/음수 수명만 재사용하며 NaN 비교는 거짓이라 재사용하지 않는다.
        for (const particle of fields.particles) if (particle.life <= 0) return particle;
        return null;
    }

    // 난수는 슬롯이 없을 때도 두 번(y → x) 전진한다. 최초 방출은 offset 을 더하지 않는다. (2026-11-09 WB04-E187)
    #emit(fields, x, y, initial) {
        const noiseY = this.#random.next(65536) * RANDOM_UNIT * 6 - 3;
        const noiseX = this.#random.next(65536) * RANDOM_UNIT * 6 - 3;
        const offset = initial ? 0 : fields.offset;
        const particle = this.#append(fields);
        if (particle) {
            particle.x = Math.fround((noiseX + offset) + x);
            particle.y = Math.fround((noiseY + offset) + y);
            particle.life = Math.fround(fields.maximumLife);
        }
    }

    // 원본은 시간 차이 float 를 먼저 저장한 뒤 넓은 값과 한도를 비교한다. 마지막 시간 조각이면 true 이다. (2026-11-09 WB04-E187)
    #clock(fields) {
        const state = this.#state;
        const difference = state.now - fields.lastTime;
        fields.delta = Math.fround(difference);
        const finished = !(difference > MAXIMUM_STEP);
        if (!finished) {
            fields.delta = Math.fround(MAXIMUM_STEP);
            fields.lastTime += MAXIMUM_STEP;
        } else fields.lastTime = state.now;
        const wide = fields.delta * DELTA_SCALE;
        fields.delta = Math.fround(wide);
        fields.scaledDelta = Math.fround(wide * state.rate);
        fields.elapsed += fields.delta;
        return finished;
    }

    // 실제 00487ba0 이 00499310 으로 넘기는 다섯 인자 중 flags 는 항상 0 이다. 카메라 감산과 1픽셀 여유는 중간 float 저장 없이 정수화한다. (2026-11-09 WB04-E187)
    #invalidate(left, top, right, bottom) {
        const state = this.#state;
        this.#hooks.invalidate({ left: ftol((left - 1) - state.cameraX), top: ftol((top - 1) - state.cameraY), right: ftol((right + 1) - state.cameraX), bottom: ftol((bottom + 1) - state.cameraY) });
    }

    // 원본 00487e50 의 색수/float 역수/__ftol 조회이다. 공급된 메모리 밖 읽기는 거부한다. (2026-11-09 WB04-E187)
    #color(life, maximum) {
        const palette = this.#palette;
        if (!Number.isFinite(maximum) || maximum <= 0 || palette.count < 0) throw new RangeError('Fiery Tail 색 표 분모/개수 오류');
        const reciprocal = Math.fround(1.0 / maximum);
        // 색수는 float 로 먼저 저장되고 이후 두 곱은 넓게 계산된다.
        const index = Math.max(0, ftol(Math.fround(palette.count) * (maximum < life ? maximum : life) * reciprocal));
        if (index >= palette.readable.length) throw new RangeError('Fiery Tail 색 표 인접 DWORD 미공급');
        return palette.readable[index] >>> 0;
    }

    // 첫 프레임은 영역 설정과 10입자만 수행하며 dirty 제출과 수명 감소가 없다. 이후에는 시간 조각마다 2입자·전체 용량 감소·dirty 를 수행한다. (2026-11-09 WB04-E187)
    step(fields) {
        const state = this.#state;
        validate(fields);
        const rect = fields.registration;
        if (rect.parent < 0 || rect.parent > 65535) throw new RangeError('Fiery Tail 부모 SID 범위 오류');
        const raw = this.#pool.slot(rect.parent);
        if (!raw[6] && !raw[7]) throw new Error('Fiery Tail 부모 head 원본 assert');
        const x = Math.fround(readFloat(raw, 14) * state.pixelScaleX);
        const y = Math.fround(readFloat(raw, 18) * state.pixelScaleY);
        if (!Number.isFinite(x) || !Number.isFinite(y) || !Number.isFinite(state.now) || !Number.isFinite(fields.lastTime)) throw new RangeError('Fiery Tail 현재 좌표/시각 오류');
        if (!fields.first) {
            fields.lastTime = state.now;
            fields.elapsed = 0;
            rect.left = x;
            rect.right = x;
            rect.top = y;
            rect.bottom = y;
            fields.previousX = x;
            fields.previousY = y;
            fields.first = true;
            // 실제 최초 방출 횟수이며 capacity 가 작아도 모든 난수를 소비한다.
            for (let index = 0; index < INITIAL_PARTICLES; index += 1) this.#emit(fields, x, y, true);
            return;
        }
        const box = { left: x, top: y, right: x, bottom: y };
        let finished;
        // 표시 영역은 시간 조각 사이에 초기화하지 않고 누적한다.
        do {
            finished = this.#clock(fields);
            // 각 시간 조각에서 먼저 두 입자를 생성한다.
            for (let index = 0; index < STEP_PARTICLES; index += 1) this.#emit(fields, x, y, false);
            // 실제 사용 수가 아닌 용량 전체를 검사한다. 이번 조각에서 죽어도 영역에는 포함한다.
            for (const particle of fields.particles) {
                if (!(particle.life > 0)) continue;
                particle.life = Math.fround(particle.life - fields.delta);
                include(Math.fround(particle.x - MARGIN), Math.fround(particle.y - MARGIN), box);
                include(Math.fround(particle.x + MARGIN), Math.fround(particle.y + MARGIN), box);
            }
            this.#invalidate(rect.left, rect.top, rect.right, rect.bottom);
            this.#invalidate(box.left, box.top, box.right, box.bottom);
            rect.left = box.left;
            rect.top = box.top;
            rect.right = box.right;
            rect.bottom = box.bottom;
            fields.previousX = x;
            fields.previousY = y;
        } while (!finished);
    }

    // 실제 선은 입자 화면 좌표에서 호출자가 전달한 부모 화면 끝점 + offset 까지 그린다(offset 합은 32비트로 감김). (2026-11-09 WB04-E187)
    draw(fields, endpointX, endpointY) {
        const state = this.#state;
        const palette = this.#palette;
        validate(fields);
        const x2 = (endpointX + fields.offset) | 0;
        const y2 = (endpointY + fields.offset) | 0;
        if (state.diagnostic === 2) {
            palette.count = DIAGNOSTIC_COLORS;
            fields.maximumLife = DIAGNOSTIC_LIFE;
        }
        // 원본 Draw 는 용량 대신 사용 수까지만 순회하고 양수 수명만 선택한다.
        for (let index = 0; index < fields.total; index += 1) {
            const particle = fields.particles[index];
            if (!(particle.life > 0)) continue;
            const color = this.#color(particle.life, Math.fround(fields.maximumLife));
            this.#hooks.line({ x1: ftol(particle.x - state.cameraX), y1: ftol(particle.y - state.cameraY), x2, y2, color });
        }
    }
}
