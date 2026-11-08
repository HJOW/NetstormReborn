// 건설 표시(Build.cpp)의 프레임 갱신·인접 입자 추가·팔레트 조회·시각 누적·Draw 제출을 옮긴 모듈이다(cpppj `o/RawConstructionFrame.cpp/.h`, 10.78 전용).
//  - RawConstructionFrame.run(fields): 00422f00. 부모 좌표·배율·hotspot 으로 표시 사각형 계산 → 시각 누적(clock) → 최대 HP 경계 호출 → 목표 선택 수(HP 비율 × 불투명 수)까지 활성 입자 추가(+ 인접 확장)
//    → 활성 입자의 수명 감소·만료(원래 색 복원·마지막 입자 두 번 덮기)·색 표 갱신 → 미활성 입자의 수명 재설정·색 표 순환 → 이전/새 사각형 dirty 큐 → 부모 abstract 비트가 0 이면 kill(0).
//  - draw(fields): 00423200. 입자 용량이 0 이면 생략하고, 아니면 부모 좌표−카메라−hotspot 을 CRT 정수로 변환해 출력 버퍼를 제출한다.
//  - expand(fields, pixel): 00422ea0. 선형 픽셀의 오른쪽·왼쪽·위 이웃을 불투명하면 입자로 추가한다(행 경계는 검사하지 않는다).
//  - append(fields, pixel): 00422e30. 용량 안에서 한 픽셀을 입자로 옮기고 원본 색을 투명으로 바꾼다. color(life, maximum): 00487e50. clock(fields): 00418400/30.
// 모든 float 저장 경계는 Math.fround 로 보존하고 CRT ftol 은 64비트 정수 절단의 하위 DWORD(비유한/범위 밖은 0)이다. 원본이 읽는 raw 포인터는 호스트 풀의 live 뷰를 쓴다
// (경계 호출이 부모 raw 를 바꿀 수 있으므로). 풀/표시 전역/시각/색 표/난수는 이 객체보다 오래 살아야 한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E130: RawConstructionFrame 이식으로 추가)
import { createConstructionVisualFields } from './raw-construction-visual.js';

// 부모 raw 슬롯의 위치이다: x=+14(float)·y=+18(float)·HP=+26(int32)·abstract 바이트=+40. (2026-11-08 WB04-E130)
const X = 14;
const Y = 18;
const HP = 26;
const EXTRA = 40;

// 투명 팔레트 항목, abstract 비트, 시각 helper 의 계수와 dirty 영역 여유이다. (2026-11-08 WB04-E130)
const TRANSPARENT = 255;
const ABSTRACT = 1;
const TIME_SCALE = 10;
const MARGIN = 1;

// int32 범위와 ftol 의 64비트 한계이다. (2026-11-08 WB04-E130)
const INT32_MAX = 2147483647;
const INT64_LIMIT = 9223372036854775808;

// 필수 경계 이름이다. (2026-11-08 WB04-E130)
const HOOK_NAMES = ['maxHitPoints', 'invalidate', 'draw', 'kill'];

// 생성한 표시 상태와 RunFrame 의 시각 helper 가 쓰는 두 float 이다(+0x40 delta·+0x44 scaledDelta). 사용: const fields = createConstructionFrameFields(). (2026-11-08 WB04-E130)
export function createConstructionFrameFields() {
    return { visual: createConstructionVisualFields(), delta: 0, scaledDelta: 0 };
}

// 프레임 시각과 표시 영역의 현재 전역이다: now(게임 시각 double)·rate(scaledDelta 에만 곱하는 float)·cameraX/Y(dirty/Draw 에서 빼는 화면 원점 float). (2026-11-08 WB04-E130)
export function createConstructionFrameState() {
    return { now: 0, rate: 1, cameraX: 0, cameraY: 0 };
}

// CRT ftol 의 하위 signed DWORD 이다. 비유한 값/64비트 범위 밖은 하위 DWORD 0 이다. (2026-11-08 WB04-E130)
function ftol(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= INT64_LIMIT) return 0;
    const truncated = Math.trunc(value);
    if (Math.abs(truncated) <= INT32_MAX) return truncated | 0;
    return Number(BigInt.asIntN(32, BigInt(truncated)));
}

// 정렬과 무관한 리틀엔디언 부호 없는 정수를 읽는다. (2026-11-08 WB04-E130)
function readUnsigned(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// raw 의 단정도 float 를 읽는다. (2026-11-08 WB04-E130)
function readFloat(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, readUnsigned(raw, offset), true);
    return view.getFloat32(0, true);
}

// 입자 한 칸의 값을 다른 칸에 복사한다(원본 struct 대입). (2026-11-08 WB04-E130)
function copyParticle(target, source) {
    target.active = source.active;
    target.color = source.color;
    target.life = source.life;
    target.pixel = source.pixel;
}

// 건설 표시 프레임 갱신/제출 몸체이다. 사용: const frame = new RawConstructionFrame(pool, visualState, state, palette, random, hooks); frame.run(fields); frame.draw(fields).
// palette = { token, count, entries[] }(count 는 색수 DWORD, entries 는 원본이 읽을 수 있는 DWORD 배열)이며 live 로 읽는다. 누락된 경계는 생성 때 거부한다. (2026-11-08 WB04-E130)
export class RawConstructionFrame {
    // 부모 raw 풀, 표시 전역(배율), 프레임 전역(시각/카메라/배율), 색 표, 공유 게임 난수, 경계이다.
    #pool;
    #visualState;
    #state;
    #palette;
    #random;
    #hooks;

    // 경계가 빠진 표시 갱신은 픽셀 변경 전에 거부한다. (2026-11-08 WB04-E130)
    constructor(pool, visualState, state, palette, random, hooks) {
        for (const name of HOOK_NAMES) {
            if (typeof hooks?.[name] !== 'function') throw new TypeError('건설 프레임 경계 누락');
        }
        this.#pool = pool;
        this.#visualState = visualState;
        this.#state = state;
        this.#palette = palette;
        this.#random = random;
        this.#hooks = { ...hooks };
    }

    // 원본 포인터/카운터가 의미 있는 호스트 저장소 안인지 확인한다. (2026-11-08 WB04-E130)
    #validate(fields) {
        const v = fields.visual;
        const size = v.image.pixels.length;
        if (!(v.image.width > 0) || !(v.image.height > 0) || v.image.width * v.image.height !== size || size > INT32_MAX || v.output.length !== size
            || v.lastRow !== (v.image.height - 1) * v.image.width || v.capacity > INT32_MAX || v.particles.length !== v.capacity || v.total > v.capacity || v.active > v.total) {
            throw new RangeError('건설 프레임 저장소 오류');
        }
        // 현재 사용 입자의 목적지는 모두 출력 버퍼 안이어야 한다.
        for (let index = 0; index < v.total; index += 1) {
            if (v.particles[index].pixel >= v.output.length || !Number.isFinite(v.particles[index].life)) throw new RangeError('건설 프레임 입자 오류');
        }
    }

    // 원본 float 난수는 저장 이전의 값(double)을 되돌려준다. 재시작 계산은 마지막에만 float 로 저장한다. (2026-11-08 WB04-E130)
    #life(minimum, maximum) {
        const fraction = this.#random.next(65536) / 65536;
        return fraction * Math.abs(maximum - minimum) + minimum;
    }

    // 용량 안에서 입자 하나를 추가한다. 가득 찼으면 거짓이고 범위 밖 픽셀은 거부한다(원본의 범위 밖 접근을 옮기지 않음). 원본: 00422e30. (2026-11-08 WB04-E130)
    append(fields, pixel) {
        pixel |= 0;
        const v = fields.visual;
        if (v.total >= v.capacity) return false;
        if (v.particles.length !== v.capacity) throw new RangeError('건설 프레임 입자 용량 오류');
        const valid = pixel >= 0 && pixel < v.image.pixels.length && v.output.length === v.image.pixels.length;
        if (!valid) throw new RangeError('건설 입자 픽셀 오류');
        const particle = v.particles[v.total];
        v.total += 1;
        particle.life = Math.fround(this.#life(v.minimum, v.maximum));
        particle.pixel = pixel;
        particle.active = 0;
        particle.color = v.image.pixels[pixel];
        v.image.pixels[pixel] = TRANSPARENT;
        v.selected = (v.selected + 1) >>> 0;
        return true;
    }

    // 원본은 행의 좌우 경계를 별도로 검사하지 않는다. 선형 인덱스의 오른쪽·왼쪽·위 이웃 순서를 유지하며 앞 선택이 바꾼 버퍼/용량을 다시 읽는다. 원본: 00422ea0. (2026-11-08 WB04-E130)
    expand(fields, pixel) {
        pixel >>>= 0;
        const v = fields.visual;
        this.#validate(fields);
        if (pixel <= v.image.width >>> 0 || pixel >= v.lastRow >>> 0) return;
        // 오른쪽·왼쪽·위 이웃을 차례로 추가한다(후보 번호는 먼저 계산한다).
        for (const neighbor of [(pixel + 1) >>> 0, (pixel - 1) >>> 0, (pixel - v.image.width) >>> 0]) {
            if (v.image.pixels[neighbor] !== TRANSPARENT) this.append(fields, neighbor | 0);
        }
    }

    // 원본 색수는 배열 길이와 다르다. life>=maximum 이 읽는 인접 DWORD 도 명시 자료가 있어야 한다. 패치는 역수를 float 로 먼저 저장한다. 원본: 00487e50. (2026-11-08 WB04-E130)
    color(life, maximum) {
        const palette = this.#palette;
        if (!Number.isFinite(life) || !Number.isFinite(maximum) || maximum <= 0 || palette.count < 0) throw new RangeError('건설 색 표 입력 오류');
        const value = maximum < life ? maximum : life;
        const reciprocal = Math.fround(1 / maximum);
        const position = Math.fround(palette.count) * value * reciprocal;
        const index = Math.max(0, ftol(position));
        if (index >= palette.entries.length) throw new RangeError('건설 색 표 읽기 범위 오류');
        return palette.entries[index] >>> 0;
    }

    // 반올림 전 시간 차이에 배율을 곱하고, 저장한 float delta 를 double 누적 시각에 더한다. 첫 프레임에는 시각 기준을 초기화한다. 원본: 00418400/30. (2026-11-08 WB04-E130)
    clock(fields) {
        const v = fields.visual;
        const state = this.#state;
        if (!v.firstDraw) {
            v.lastTime = state.now;
            v.elapsed = 0;
            v.firstDraw = true;
        }
        const difference = (state.now - v.lastTime) * TIME_SCALE;
        fields.delta = Math.fround(difference);
        fields.scaledDelta = Math.fround(difference * state.rate);
        v.lastTime = state.now;
        v.elapsed += fields.delta;
    }

    // dirty 큐 경계가 카메라를 바꾸면 새 영역의 요청은 바뀐 카메라를 사용한다. 이전/새 영역을 큐에 넣은 뒤 현재 표시 사각형을 갱신한다. (2026-11-08 WB04-E130)
    #rectangle(v, left, top, right, bottom) {
        const state = this.#state;
        // 카메라와 1픽셀 여유를 적용한 영역을 원본 순서대로 정수로 변환한다.
        const area = (l, t, r, b) => ({
            left: ftol(l - state.cameraX - MARGIN),
            top: ftol(t - state.cameraY - MARGIN),
            right: ftol(r - state.cameraX + MARGIN),
            bottom: ftol(b - state.cameraY + MARGIN),
        });
        this.#hooks.invalidate(area(v.left, v.top, v.right, v.bottom));
        this.#hooks.invalidate(area(left, top, right, bottom));
        v.left = left;
        v.top = top;
        v.right = right;
        v.bottom = bottom;
    }

    // HP 활성화 목표는 루트에서 한 번 계산한다. 입자 확장으로 늘어나는 selected/total 은 매 반복 다시 읽는다. 원본: 00422f00. (2026-11-08 WB04-E130)
    run(fields) {
        const v = fields.visual;
        const parent = v.parent;
        if (!parent) throw new RangeError('건설 프레임 부모 오류');
        // 경계 호출이 부모 raw 를 바꿀 수 있어 live 뷰로 읽는다(HP 는 경계 호출 전에, abstract 비트는 호출 뒤에 읽는다).
        const raw = this.#pool.physical(parent);
        this.#validate(fields);
        const lx = this.#visualState.scaleX * readFloat(raw, X) - v.image.hotspotX;
        const ty = this.#visualState.scaleY * readFloat(raw, Y) - v.image.hotspotY;
        const left = Math.fround(lx);
        const top = Math.fround(ty);
        const right = Math.fround(left + v.image.width);
        const bottom = Math.fround(ty + v.image.height);
        this.clock(fields);
        const savedHp = Math.fround(readUnsigned(raw, HP) | 0);
        const maximum = this.#hooks.maxHitPoints(parent);
        const denominator = ((maximum >>> 0) - 1) | 0;
        const target = ftol(savedHp / denominator * (v.opaque | 0));
        // 활성 앞 구간에서 아직 선택하지 않은 입자를 뽑고 이웃을 확장한 뒤 active 위치로 교환한다.
        while ((v.selected | 0) < target && v.active < v.total && v.total < v.capacity) {
            const index = v.active + this.#random.next(v.total - v.active);
            const pixel = v.particles[index].pixel;
            this.expand(fields, pixel);
            v.particles[index].active = 1;
            const swap = v.particles[v.active];
            v.particles[v.active] = v.particles[index];
            v.particles[index] = swap;
            v.active += 1;
        }
        let i = 0;
        // 만료 입자는 마지막 활성 입자/마지막 전체 입자로 두 번 덮고 같은 위치를 다시 처리한다.
        while (i < v.active) {
            const particle = v.particles[i];
            const remainder = particle.life - fields.delta;
            particle.life = Math.fround(remainder);
            if (remainder <= 0) {
                v.output[particle.pixel] = particle.color;
                v.active -= 1;
                v.total -= 1;
                copyParticle(particle, v.particles[v.active]);
                copyParticle(v.particles[v.active], v.particles[v.total]);
            } else {
                if (v.palette !== this.#palette.token) throw new RangeError('건설 색 표 연결 오류');
                v.output[particle.pixel] = this.color(particle.life, v.maximum) & 0xff;
                i += 1;
            }
        }
        // 미활성 입자는 원래 색을 확정하지 않고 난수 수명을 다시 정해 색 표 순환만 계속한다.
        for (; i < v.total; i += 1) {
            const particle = v.particles[i];
            const remainder = particle.life - fields.delta;
            particle.life = Math.fround(remainder);
            if (remainder <= 0) {
                const life = this.#life(0, v.maximum);
                particle.life = Math.fround(life + fields.delta + particle.life);
            }
            if (v.palette !== this.#palette.token) throw new RangeError('건설 색 표 연결 오류');
            v.output[particle.pixel] = this.color(particle.life, v.maximum) & 0xff;
        }
        this.#rectangle(v, left, top, right, bottom);
        if (!(raw[EXTRA] & ABSTRACT)) this.#hooks.kill(0);
    }

    // 원본은 total 이 0 이어도 capacity 가 있으면 Draw 를 제출한다(입력으로 받은 두 가상 인자는 쓰지 않는다). 원본: 00423200. (2026-11-08 WB04-E130)
    draw(fields) {
        const v = fields.visual;
        if (!v.capacity) return;
        if (v.total > v.capacity) throw new RangeError('건설 Draw 입자 수 오류');
        if (!v.parent) throw new RangeError('건설 Draw 부모 오류');
        const raw = this.#pool.slot(v.parent);
        const x = Math.fround(this.#visualState.scaleX * readFloat(raw, X) - this.#state.cameraX);
        const y = this.#visualState.scaleY * readFloat(raw, Y) - this.#state.cameraY;
        this.#hooks.draw({
            x: ftol(x - v.image.hotspotX),
            y: ftol(y - v.image.hotspotY),
            pitch: v.image.width,
            clip: { left: 0, top: 0, right: v.image.width, bottom: v.image.height },
            pixels: v.output,
        });
    }
}
