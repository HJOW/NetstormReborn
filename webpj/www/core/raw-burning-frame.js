// Burning(process 14)의 캡처와 실제 프레임을 기존 생성자·연소점·Append 몸체에 연결하는 모듈이다(cpppj `o/RawBurningFrame.cpp/.h`, 10.78 전용: 패치 00423d50 캡처·00424530 프레임).
// 원본 x87 저장 위치(float 저장 시점)와 단계별 순서(캡처 → 첫 좌표 → visible·timer·연소점·입자·dirty)를 보존한다.
//  - createBurningFrameFields(): 생성자가 초기화하지 않는 원본 +40/+44 의 { delta, scaledDelta } 이다. 프레임이 처음 쓸 때만 갱신한다.
//  - createBurningFrameState(): 프레임이 매 호출 읽는 현재 전역이다. { now(게임 시각 0055b4d0), pixelScaleX/Y(월드→픽셀 0059a924/28), cameraX/Y(0059a938/3c), wind·deltaMultiplier(입자 바람·timer +44 배율),
//    viewport({ left, top, right, bottom }), insetViewport(visible 경계를 양쪽 100 픽셀 줄이는 모드), victim({ checking(), robust?, assertion? }, `raw-burning-victim.js` 의 진단 상태), sharedWind(null 또는 현재 Client 바람을 읽는 getter) } 이다.
//    float 전역 값은 호출자가 Math.fround 한 값을 넣는다. sharedWind 가 있으면 각 입자의 가속 지점에서 그것을 다시 읽는다.
//  - new RawBurningFrame(pool, types, visual, sources, random, state, sink, hooks): visual(`RawBurningVisual`)·sources(`RawBurningSources`)는 같은 풀이어야 하고,
//    random 은 연소점 정수 난수와 같은 게임 난수(`next(limit)`), sink 는 Squid 표시 출력 대상({ invalidate(rect, flags) }, 예: `render/squid-renderer.js`), hooks = { capture(sid) → 그림, stopSound?(handle) } 이다.
//    capture 는 실제 client ShapeToBuffer(`client/BurningFrameCapture` = `makeBurningFrameCapture`)로 연결한다. 0 이 아닌 소리 정지는 도달할 때만 요구한다.
//  - capture(fields): 423d50 — 이전 이미지/행 반납 → 입자 전체 zero → 피해 대상 앞단(진단 포함) → 현재 그림 캡처 → (폭+높이)/5 개 행 표식 zero.
//  - step(fields, frame): 424530 — capacity 0 반환, 단계 0(캡처)·1(첫 좌표) 처리, visible 프레임에서만 시각/연소점/입자/dirty 를 갱신한다(보이지 않으면 소리 정지와 off).
//  - pool: 같은 풀 확인용.
// 10.72(CD)의 무진단 인라인·signed DWORD life·저장 vx 재읽기 분기는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E200: Burning 프레임 이식으로 추가)
import { BurningVictimEntry, makeBurningVictimHooks, runBurningVictimParent } from './raw-burning-victim.js';

// 원본 timer 10배·float 난수 단위·visible 여백·행 표식 나눗수의 실제 상수이다. (2026-11-09 WB04-E200)
const TICK_SCALE = 10.0;
const RANDOM_UNIT = 1.0 / 65536.0;
const INSET = 100;
const ROWS_DIVISOR = 5;
// 방출 확률 상한·새 입자 수명 범위·상승 속도의 원본 float 상수이다. (2026-11-09 WB04-E200)
const EMIT_MAXIMUM = Math.fround(1.4);
const LIFE_MINIMUM = 4;
const LIFE_MAXIMUM = 9;
const RISE = -2;
// 원본 signed DWORD 범위이다. (2026-11-09 WB04-E200)
const TWO_31 = 2147483648;
const INT32_MAX = 2147483647;

// float 저장(Math.fround)의 짧은 이름이다. (2026-11-09 WB04-E200)
const f = Math.fround;

// 원본 ftol 의 정상 유한 범위를 확인한 뒤 0 방향으로 정수화한다(범위 밖은 원본 미정의 결과 대신 거부). (2026-11-09 WB04-E200)
function frameInteger(value) {
    if (!Number.isFinite(value) || value < -TWO_31 || value >= TWO_31) throw new RangeError('Burning frame 원본 정수화 범위 오류');
    return Math.trunc(value) | 0;
}

// raw 의 비정렬 little endian float 를 읽는다. (2026-11-09 WB04-E200)
function readFloat(raw, offset) {
    return new DataView(raw.buffer, raw.byteOffset + offset, 4).getFloat32(0, true);
}

// 패치 helper 의 ordered 비교로 점 하나를 dirty 상자에 넣는다(NaN 비교는 새 값 쪽을 고른다). (2026-11-09 WB04-E200)
function includePoint(box, x, y) {
    box.left = box.left < x ? box.left : x;
    box.top = box.top < y ? box.top : y;
    const nextRight = f(x + 1);
    const nextBottom = f(y + 1);
    box.right = nextRight < box.right ? box.right : nextRight;
    if (box.bottom <= nextBottom) box.bottom = nextBottom;
}

// 생성자가 초기화하지 않는 두 float 프레임 칸이다. (2026-11-09 WB04-E200)
export function createBurningFrameFields() {
    return { delta: 0, scaledDelta: 0 };
}

// 프레임이 매 호출 읽는 현재 전역의 기본값이다(cpppj BurningFrameState). (2026-11-09 WB04-E200)
export function createBurningFrameState() {
    return {
        now: 0, pixelScaleX: 1, pixelScaleY: 1, cameraX: 0, cameraY: 0, wind: 0, deltaMultiplier: 0,
        viewport: { left: 0, top: 0, right: 0, bottom: 0 }, insetViewport: false, victim: { checking: () => 0 }, sharedWind: null,
    };
}

// 00423d50/00424530 의 Burning 캡처·프레임 몸체이다. 모든 처리기는 같은 SID 풀을 쓰며 state/hooks/sink 는 이 객체보다 오래 살아야 한다. (2026-11-09 WB04-E200)
export class RawBurningFrame {
    #pool;
    #types;
    #visual;
    #sources;
    #random;
    #state;
    #sink;
    #hooks;

    // 기존 생성자·연소점·Append 를 같은 풀에 연결하고 현재 캡처 소비자를 요구한다. (2026-11-09 WB04-E200)
    constructor(pool, types, visual, sources, random, state, sink, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Burning frame 은 10.78 SidPool 만 받습니다.');
        if (visual?.pool !== pool || sources?.pool !== pool) throw new TypeError('Burning frame/생성/연소점 SID 풀 불일치');
        if (typeof hooks?.capture !== 'function') throw new TypeError('Burning frame 실제 ShapeToBuffer 캡처 미연결');
        if (typeof random?.next !== 'function' || !state || typeof sink?.invalidate !== 'function') throw new TypeError('Burning frame 난수/상태/출력 대상 누락');
        this.#pool = pool;
        this.#types = types;
        this.#visual = visual;
        this.#sources = sources;
        this.#random = random;
        this.#state = state;
        this.#sink = sink;
        this.#hooks = hooks;
    }

    // 현재 피해·캡처·실제 효과 소비자가 같은 풀을 쓰는지 확인하는 풀이다. (2026-11-09 WB04-E200)
    get pool() { return this.#pool; }

    // 같은 현재 바람 owner 를 각 입자의 가속 지점에서 다시 읽는다. (2026-11-09 WB04-E200)
    #wind() {
        const state = this.#state;
        return typeof state.sharedWind === 'function' ? state.sharedWind() : state.wind;
    }

    // float 난수는 상위 16비트·1/65536·넓은 x87 반환값을 쓰며 같은 게임 난수를 한 번 전진한다. (2026-11-09 WB04-E200)
    #randomFloat(minimum, maximum) {
        return this.#random.next(65536) * RANDOM_UNIT * Math.abs(maximum - minimum) + minimum;
    }

    // 세 부모의 같은 피해 대상 앞단이다. 현재 SID 를 getter 로 다시 읽는다. (2026-11-09 WB04-E200)
    #victim(fields, entry) {
        const victim = this.#state.victim;
        return runBurningVictimParent(() => fields.victim, entry, victim.checking ?? (() => 0), makeBurningVictimHooks(this.#pool, this.#types, victim));
    }

    // 423d50: 이전 버퍼 해제 뒤 원본은 일부 메타데이터를 유지하며, 정상 캡처만 새 버퍼로 바꾼다. (2026-11-09 WB04-E200)
    capture(fields) {
        if (fields.total > fields.capacity || fields.particles.length !== fields.capacity) throw new RangeError('Burning capture 현재 입자 배열 오류');
        fields.image.pixels = new Uint8Array(0);
        fields.rows = new Uint8Array(0);
        fields.total = 0;
        // 실제 particle container clear 는 용량만큼 모든 24바이트 의미 필드를 0 으로 만든다.
        for (let index = 0; index < fields.particles.length; index += 1) fields.particles[index] = { x: 0, y: 0, velocityX: 0, velocityY: 0, life: 0, source: false };
        if (this.#victim(fields, BurningVictimEntry.Capture) === null) return;
        // 원문은 앞단이 고정한 번호가 아니라 현재 victim 필드로 캡처한다.
        const image = this.#hooks.capture(fields.victim);
        const sum = image.width + image.height;
        if (image.width < 0 || image.height < 0 || sum > INT32_MAX) throw new RangeError('Burning capture 행 표식 크기 오류');
        fields.image = image;
        fields.rows = new Uint8Array(Math.trunc(sum / ROWS_DIVISOR));
        fields.selected = 0;
    }

    // 이전 범위를 먼저 정수화하고 카메라를 빼서 다시 원본 ftol 을 수행한 뒤 현재 viewport 와 겹치는지 본다. (2026-11-09 WB04-E200)
    #visible(registration) {
        const state = this.#state;
        const left = frameInteger(f(f(frameInteger(registration.left)) - state.cameraX));
        const top = frameInteger(f(f(frameInteger(registration.top)) - state.cameraY));
        const right = frameInteger(f(f(frameInteger(registration.right)) - state.cameraX));
        const bottom = frameInteger(f(f(frameInteger(registration.bottom)) - state.cameraY));
        const margin = state.insetViewport ? INSET : 0;
        const view = state.viewport;
        const viewLeft = view.left + margin;
        const viewTop = view.top + margin;
        const viewRight = view.right - margin;
        const viewBottom = view.bottom - margin;
        return viewLeft <= viewRight && viewTop <= viewBottom && left <= right && top <= bottom && viewLeft < right && left < viewRight && viewTop < bottom && top < viewBottom;
    }

    // SetRect 는 양 끝을 1 픽셀 넓히고(패치: 여백 → 카메라 순서) 넓은 값을 정수화한 뒤 viewport 로 제한한다. 빈 영역은 넘기지 않는다. (2026-11-09 WB04-E200)
    #invalidate(left, top, right, bottom) {
        const state = this.#state;
        const view = state.viewport;
        const clamp = (value, low, high) => (value < low ? low : high < value ? high : value);
        const coordinate = (value, camera, edge) => frameInteger((value + edge) - camera);
        const rect = {
            left: clamp(coordinate(left, state.cameraX, -1), view.left, view.right),
            top: clamp(coordinate(top, state.cameraY, -1), view.top, view.bottom),
            right: clamp(coordinate(right, state.cameraX, 1), view.left, view.right),
            bottom: clamp(coordinate(bottom, state.cameraY, 1), view.top, view.bottom),
        };
        if (rect.left < rect.right && rect.top < rect.bottom) this.#sink.invalidate(rect, 0);
    }

    // capacity 0·초기 캡처·첫 좌표·visible timer·연소점과 모든 capacity 입자의 실제 순서이다. (2026-11-09 WB04-E200)
    step(fields, frame) {
        if (!fields.capacity) return;
        if (fields.particles.length !== fields.capacity || fields.total > fields.capacity) throw new RangeError('Burning frame 현재 입자 배열 오류');
        const state = this.#state;
        if (fields.stage === 0) {
            fields.lastTime = state.now;
            fields.elapsed = 0;
            this.capture(fields);
            fields.stage += 1;
            return;
        }
        const selected = this.#victim(fields, BurningVictimEntry.Frame);
        if (selected === null) return;
        // 진단 뒤 현재 필드가 바뀌어도 원문이 보관한 선택 슬롯의 좌표/깊이를 읽는다.
        const raw = this.#pool.slot(selected);
        const centerX = f(readFloat(raw, 14) * state.pixelScaleX);
        const centerY = f(readFloat(raw, 18) * state.pixelScaleY);
        const registration = fields.registration;
        registration.zOrder = ((raw[35] << 24) >> 24) - 1;
        if (fields.stage === 1) {
            registration.left = centerX;
            registration.top = centerY;
            registration.right = centerX;
            registration.bottom = centerY;
            fields.off = true;
            fields.stage = 2;
            return;
        }
        if (!this.#visible(registration)) {
            if (!fields.off && fields.sound) {
                if (typeof this.#hooks.stopSound !== 'function') throw new Error('Burning frame 실제 nonzero 소리 정지 미연결');
                this.#hooks.stopSound(fields.sound);
            }
            fields.off = true;
            return;
        }
        if (fields.off) {
            fields.lastTime = state.now;
            fields.off = false;
        }
        const wide = (state.now - fields.lastTime) * TICK_SCALE;
        frame.delta = f(wide);
        frame.scaledDelta = f(wide * state.deltaMultiplier);
        fields.lastTime = state.now;
        fields.elapsed += frame.delta;
        this.#sources.select(fields);
        const box = { left: centerX, top: centerY, right: centerX, bottom: centerY };
        // 원본은 total 이 아니라 capacity 전체를 검사하므로 새 후방 입자를 같은 프레임에 처리할 수 있다.
        for (let index = 0; index < fields.capacity; index += 1) {
            const particle = fields.particles[index];
            if (!(particle.life > 0)) continue;
            if (particle.source) {
                if (!(this.#randomFloat(0, EMIT_MAXIMUM) < frame.delta)) continue;
                const emitted = {
                    x: f((centerX - fields.image.hotspotX) + particle.x),
                    y: f((centerY - fields.image.hotspotY) + particle.y),
                    velocityX: 0,
                    velocityY: RISE,
                    life: f(this.#randomFloat(LIFE_MINIMUM, LIFE_MAXIMUM)),
                    source: false,
                };
                if (this.#visual.append(fields, emitted)) includePoint(box, emitted.x, emitted.y);
            } else {
                particle.life = f(particle.life - frame.delta);
                // 패치는 저장 전 넓은 속도로 위치를 계산한다.
                const velocity = particle.velocityX + this.#wind();
                particle.velocityX = f(velocity);
                particle.x = f(velocity * frame.delta + particle.x);
                particle.y = f(particle.velocityY * frame.delta + particle.y);
                includePoint(box, particle.x, particle.y);
            }
        }
        const right = f(box.right + 2);
        const bottom = f(box.bottom + 1);
        const minimum = f(right - 17);
        const left = minimum <= box.left ? minimum : box.left;
        const top = f(box.top - 2);
        this.#invalidate(registration.left, registration.top, registration.right, registration.bottom);
        this.#invalidate(left, top, right, bottom);
        registration.left = left;
        registration.top = top;
        registration.right = right;
        registration.bottom = bottom;
    }
}

// client/BurningFrameCapture: 같은 SID 풀의 실제 ShapeToBuffer 캡처만 설치하고 호출자가 준 다른 소비자(소리 정지)는 유지한다.
// capture 는 `render/raw-shape-to-buffer.js` 의 RawShapeToBuffer 이다. (2026-11-09 WB04-E200)
export function makeBurningFrameCapture(capture, pool, hooks = {}) {
    if (capture?.pool !== pool) throw new TypeError('Burning frame/ShapeToBuffer SID 풀 불일치');
    return { ...hooks, capture: (sid) => capture.capture(sid) };
}
