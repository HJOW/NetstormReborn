// Lightning.cpp 의 실제 시간 조각·흔들림·굵기 선과 소진 요청을 옮긴 모듈이다(cpppj `o/RawLightningFrame.cpp/.h`, 10.78 전용: 00472310 프레임·004724f0 출력). 피해는 발사 시점의 별도 로직이다.
//  - createLightningFrameFields(): 빔 객체의 의미 필드 — registration(현재 Form/부모와 이전 표시 영역, `createRenderProcessFields()`), particles(`LightningParticles`: 용량 전체와 사용 꼬리, 같은 소유 빔 토큰), lastTime/elapsed(+0x30/+0x38 마지막 시각과 10배 시간 누적),
//    delta/scaledDelta(+0x40/+0x44 현재 조각과 표시 속도 배율을 곱한 조각), first(+0x48 최초 Seed/파일명 소리 처리 완료), palette(+0x50 월드 공유 색 객체 토큰, Draw 는 조회하지 않음), firstLive(+0x54 용량 전체에서 처음 살아 있는 입자 위치),
//    path(+0x58 이 가리키는 실제 64바이트 `LightningPathFields`), pathAllocated(경로 포인터의 비영 여부), jitter(+0x5c 흔들림 범위, 생성 시 거리의 0.025배), sound(+0x60 최초 한 번 재생할 파일명, null 은 무음)이다.
//  - new RawLightningFrame(pool, random, path, state, hooks): pool 은 현재 raw 부모 좌표의 같은 PATCH SidPool, random 은 경로와 공유하는 게임 난수, path 는 같은 월드의 `RawLightningPath`, state = { now, pixelScaleX, pixelScaleY, rate, cameraX, cameraY } 는 같은 Kernel 의 시각/배율/카메라 전역(호출자가 현재 값으로 갱신),
//    hooks = { invalidate({ left, top, right, bottom })(매 시간 조각마다 이전/새 영역 순서), line({ x1, y1, x2, y2, color })(이전 점 → 현재 점, 중심선 → 보조선 순서), sound(x, y, filename)(raw 부모 좌표와 파일명), removeForm()(원본 41bf10(this, 0) 소유 월드 삭제 경계) } 는 모두 필수이다.
//  - step(fields): 최초에는 delta/흔들림/dirty 없이 Seed 와 선택 파일명 소리만 수행하고, 그 뒤에는 시간 조각(최대 0.05초, 시간 후퇴/0 간격도 원문 그대로)마다 경로 진행 → 용량 전체(사용 꼬리가 아님) 수명 감소·흔들림(난수 y → x 순)·영역 누적 → 모든 입자가 죽으면 removeForm 뒤 즉시 반환, 아니면 이전/새 영역 dirty 요청을 한다.
//  - draw(fields): 첫 생존 입자 다음부터 사용 꼬리까지 살아 있는 점을 이전 점(생존 여부는 검사하지 않음)에 연결해 중심선(색 195)과 굵기에 따라 보조선(색 85: 굵기 ≥ 0.25 면 x+1, > 0.5 면 y−1)을 요청한다. 카메라 차이는 float 로 먼저 저장하고 보조선의 ±1 은 그 값에서 넓게 계산한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E186: 번개 프레임 이식으로 추가)
import { LightningParticles, LightningPathFields } from './raw-lightning-path.js';
import { createRenderProcessFields } from './raw-construction-lifetime.js';

// 실제 타이머 최대 조각(0.5 × 0.1)·시간 배율·공통 난수의 간격이다. (2026-11-09 WB04-E186)
const MAXIMUM_STEP = 0.5 * 0.1;
const DELTA_SCALE = 10;
const RANDOM_UNIT = 1.0 / 65536.0;
// 원본 Draw 가 직접 전달하는 중심/보조선 색이다(공유 팔레트를 대신 조회하지 않음). (2026-11-09 WB04-E186)
const MAIN_COLOR = 195;
const SIDE_COLOR = 85;
const TWO_31 = 2147483648;

// 새 빔 객체의 의미 필드를 만든다(용량 0 의 빈 입자 저장소). (2026-11-09 WB04-E186)
export function createLightningFrameFields() {
    return {
        registration: createRenderProcessFields(), particles: new LightningParticles(0, 0), lastTime: 0, elapsed: 0, delta: 0, scaledDelta: 0, first: false, palette: 0, firstLive: 0,
        path: new LightningPathFields(), pathAllocated: false, jitter: 0, sound: null
    };
}

// 원본 CRT 처럼 유한한 signed32 범위만 0 방향으로 절삭한다. (2026-11-09 WB04-E186)
function ftol(value) {
    if (!Number.isFinite(value) || value < -TWO_31 || value >= TWO_31) throw new RangeError('빔 표시 정수화 범위 오류');
    return Math.trunc(value) | 0;
}

// 비정렬 raw float 를 실제 little endian DWORD 로 읽는다. (2026-11-09 WB04-E186)
function readFloat(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return view.getFloat32(0, true);
}

// 실제 004239f0 의 오른쪽/아래쪽 1픽셀과 ordered 비교를 보존한다. (2026-11-09 WB04-E186)
function include(x, y, box) {
    box.left = box.left < x ? box.left : x;
    box.top = box.top < y ? box.top : y;
    const nextRight = Math.fround(x + 1);
    const nextBottom = Math.fround(y + 1);
    box.right = nextRight < box.right ? box.right : nextRight;
    if (box.bottom <= nextBottom) box.bottom = nextBottom;
}

// 잘못된 저장소/꼬리/첫 위치는 상태나 출력을 바꾸기 전에 거부한다. (2026-11-09 WB04-E186)
function validate(fields) {
    const { values, total } = fields.particles;
    if (values.length > 0x7fffffff || total < 0 || total > values.length || fields.firstLive < 0 || fields.firstLive > values.length) throw new RangeError('빔 입자 저장소/꼬리 오류');
}

// 번개 프레임/출력 몸체이다. 빈 성공 경계를 허용하지 않고 현재 10.78 본체만 선택한다. (2026-11-09 WB04-E186)
export class RawLightningFrame {
    #pool;
    #random;
    #path;
    #state;
    #hooks;

    // 같은 풀·경로·게임 난수·전역과 실제 출력 경계를 연결한다(모든 참조는 이 객체보다 오래 살아야 함). (2026-11-09 WB04-E186)
    constructor(pool, random, path, state, hooks) {
        if (pool?.edition !== 'patch1078' || typeof random?.next !== 'function' || !path || !state
            || typeof hooks?.invalidate !== 'function' || typeof hooks.line !== 'function' || typeof hooks.sound !== 'function' || typeof hooks.removeForm !== 'function') throw new TypeError('빔 원본/출력 경계 미연결');
        this.#pool = pool;
        this.#random = random;
        this.#path = path;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 검사에는 넓은 시간 차이, 다음 연산에는 먼저 저장한 float delta 를 사용한다. 한 조각으로 끝나면 true 이다. (2026-11-09 WB04-E186)
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

    // 실제 00455930 의 상위 16비트와 넓은 간격 연산으로 현재 흔들림 난수를 반환한다. (2026-11-09 WB04-E186)
    #randomBetween(low, high) {
        return this.#random.next(65536) * RANDOM_UNIT * Math.abs(high - low) + low;
    }

    // 실제 487ba0 의 카메라/여유는 중간 float 저장 없이 __ftol 에 들어간다. (2026-11-09 WB04-E186)
    #invalidate(left, top, right, bottom) {
        const state = this.#state;
        this.#hooks.invalidate({ left: ftol((left - 1) - state.cameraX), top: ftol((top - 1) - state.cameraY), right: ftol((right + 1) - state.cameraX), bottom: ftol((bottom + 1) - state.cameraY) });
    }

    // 최초 Seed/소리 또는 후속 시간 조각·경로·전체 용량 수명·흔들림·영역/소진을 수행한다. (2026-11-09 WB04-E186)
    step(fields) {
        const state = this.#state;
        validate(fields);
        const rect = fields.registration;
        if (rect.parent < 0 || rect.parent > 65535) throw new RangeError('빔 부모 SID 범위 오류');
        const raw = this.#pool.slot(rect.parent);
        if (!raw[6] && !raw[7]) throw new Error('빔 부모 head 원본 assert');
        const rawX = readFloat(raw, 14);
        const rawY = readFloat(raw, 18);
        const x = Math.fround(rawX * state.pixelScaleX);
        const y = Math.fround(rawY * state.pixelScaleY);
        if (!Number.isFinite(x) || !Number.isFinite(y) || !Number.isFinite(state.now) || !Number.isFinite(fields.lastTime) || !Number.isFinite(fields.jitter)) throw new RangeError('빔 좌표/시각/흔들림 오류');
        if (!fields.first) {
            fields.lastTime = state.now;
            fields.elapsed = 0;
            rect.left = x;
            rect.right = x;
            rect.top = y;
            rect.bottom = y;
            fields.first = true;
            this.#path.seed(fields.path, fields.particles);
            if (fields.sound) this.#hooks.sound(rawX, rawY, fields.sound);
            return;
        }
        const box = { left: x, top: y, right: x, bottom: y };
        let finished;
        // 영역은 모든 시간 조각에 걸쳐 누적한다. 시간 후퇴와 0 간격도 원문 그대로 처리한다.
        do {
            finished = this.#clock(fields);
            this.#path.step(fields.path, fields.particles, Math.fround(fields.elapsed), fields.delta);
            const { values } = fields.particles;
            fields.firstLive = values.length;
            // 사용 꼬리가 아닌 용량 전체에서 죽은/미사용 입자의 수명까지 줄인다.
            for (let index = 0; index < values.length; index += 1) {
                const particle = values[index];
                const remaining = particle.life - fields.delta;
                particle.life = Math.fround(remaining);
                if (!(remaining > 0)) continue;
                particle.x = Math.fround(((this.#randomBetween(-fields.jitter, fields.jitter) * particle.jitterWeight) * fields.scaledDelta) + particle.x);
                particle.y = Math.fround(((this.#randomBetween(-fields.jitter, fields.jitter) * particle.jitterWeight) * fields.scaledDelta) + particle.y);
                include(particle.x, particle.y, box);
                fields.firstLive = Math.min(fields.firstLive, index);
            }
            if (fields.firstLive >= values.length) {
                this.#hooks.removeForm();
                return;
            }
            this.#invalidate(rect.left, rect.top, rect.right, rect.bottom);
            this.#invalidate(box.left, box.top, box.right, box.bottom);
            rect.left = box.left;
            rect.top = box.top;
            rect.right = box.right;
            rect.bottom = box.bottom;
        } while (!finished);
    }

    // 카메라 차이를 float 로 먼저 저장하며 보조선의 +1/-1 은 그 값에서 넓게 계산한다. (2026-11-09 WB04-E186)
    draw(fields) {
        const state = this.#state;
        const hooks = this.#hooks;
        validate(fields);
        const { values, total } = fields.particles;
        if (fields.firstLive >= values.length) return;
        // 첫 생존점 다음부터 실제 사용 꼬리까지만 선을 만든다. 이전 점의 생존 여부는 검사하지 않는다.
        for (let index = fields.firstLive + 1; index < total; index += 1) {
            const particle = values[index];
            if (!(particle.life > 0)) continue;
            if (!particle.previous || particle.previous > values.length) throw new RangeError('빔 이전 점 토큰 오류');
            const previous = values[particle.previous - 1];
            const x = Math.fround(particle.x - state.cameraX);
            const y = Math.fround(particle.y - state.cameraY);
            const px = Math.fround(previous.x - state.cameraX);
            const py = Math.fround(previous.y - state.cameraY);
            const ix = ftol(x);
            const iy = ftol(y);
            const ipx = ftol(px);
            const ipy = ftol(py);
            hooks.line({ x1: ipx, y1: ipy, x2: ix, y2: iy, color: MAIN_COLOR });
            if (particle.strength >= 0.25) {
                hooks.line({ x1: ftol(px + 1), y1: ipy, x2: ftol(x + 1), y2: iy, color: SIDE_COLOR });
                if (particle.strength > 0.5) hooks.line({ x1: ipx, y1: ftol(py - 1), x2: ix, y2: ftol(y - 1), color: SIDE_COLOR });
            }
        }
    }
}
