// 벌프 경로(Bulf41)의 실제 프레임 0048b760 과 회전 프레임 0048ba30 — 회전 구간 E/F 와 보행 구간 A~D 프레임 선택·발소리를 같은 raw 부모에 적용하는 모듈이다(cpppj `o/RawBulfFrame.cpp/.h`, 10.78 전용).
//  - bulfTurningFrame(frames, angle, counterClockwise): frames 는 FrameCodeTable(`assets/terrain-tiles.js`), counterClockwise 는 0/1 이다(그 밖은 오류).
//    E(시계)/F(반시계) 구간에서 angle×(1/360 float)×(구간 수−1) 을 float 로 저장한 뒤 +0.5 를 더해 원본 _ftol 하위 DWORD 로 절삭하고 구간 첫 번호를 더한 signed 프레임이다.
//  - runBulfFrame(pool, parent, runtime, frames, shapes, hooks, watch):
//    · pool 은 10.78 SidPool, runtime 은 같은 경로의 RawPathRuntime, frames 는 TYPE 별 FrameCodeTable 배열, shapes 는 TYPE 별 `{ frames: [{ walkStepX, walkStepY }] }` SHP 자료(`squid-renderer.js` 형식)다.
//    · hooks = { immobile()→bool(004287a0 Fall 사건 조회), halt()→bool(00489ff0), step()→{ position, angle, finished }(0048a330), arrive()(0048b340), unpop(sid, flags), pop(sid, point, flags),
//      randomInt(low, high)→int(공용 004558f0 반열린 범위), sound(x, y, name)(현재 부모 좌표 004a9d70/loop 0) }. randomInt·sound 는 발소리 때만 필요하다. watch 는 `{ value }` 상자다.
//    · 순서: immobile 이면 반환 → Step 전 각도 보관 → 완료/중단/삭제면 반환 → Step(뒤 watch) → 완료면 도착만 → 현재 TYPE/프레임 범위 확인 →
//      각도가 바뀌었으면 회전 프레임(작아졌거나 180도 넘게 커지면 F), 아니고 90도 배수면 발자국(현재 프레임 walkStep) 거리가 이동 거리 이하일 때 A~D 보행 다음 프레임(E/F 에서 돌아오면 3/9/17/30 중간 프레임부터)
//      → 프레임이 같으면 위치도 갱신하지 않음 → Unpop(0)(뒤 watch)·프레임 DWORD 쓰기·Pop(0x40)(뒤 watch) → (프레임+1)%4==0 이면 randomInt(1000,6000)/1000 번 bulfWalk 발소리.
// CD 10.72 는 지원하지 않는다(풀 판본 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E209: 이동 유닛 경로 프레임 3종 이식으로 추가)
import { pathDirection } from './raw-path-animation.js';

// 원본 0050b534 의 float 1/360 이다. (2026-11-09 WB04-E209)
const ANGLE_SCALE = Math.fround(0.0027777778450399637);
// CRT signed64 절삭이 정의된 경계(2^63)이다. (2026-11-09 WB04-E209)
const FTOL_LIMIT = 9223372036854775808;
// E/F 에서 각 A~D 로 돌아올 때 쓰는 원본 중간 프레임이다. (2026-11-09 WB04-E209)
const AFTER_TURNING = Object.freeze([3, 9, 17, 30]);
// 슬롯의 TYPE·좌표·프레임 위치와 재등록 flags, 글자 E/F/A 코드이다. (2026-11-09 WB04-E209)
const TYPE_AT = 10;
const X_AT = 14;
const Y_AT = 18;
const FRAME_AT = 36;
const POP_FLAGS = 0x40;
const LETTER_A = 0x41;
const LETTER_E = 0x45;
const LETTER_F = 0x46;

// float 비트 변환용 공용 보기이다. (2026-11-09 WB04-E209)
const FLOAT_VIEW = new DataView(new ArrayBuffer(4));

// 비정렬 DWORD 를 부호 없이 읽는다. 부호는 호출자가 정한다. (2026-11-09 WB04-E209)
function read(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// sound 와 공간 효과 뒤에도 보관 사본 대신 현재 부모의 두 float 를 읽는다. (2026-11-09 WB04-E209)
function position(pool, parent) {
    const raw = pool.slot(parent);
    FLOAT_VIEW.setUint32(0, read(raw, X_AT), true);
    const x = FLOAT_VIEW.getFloat32(0, true);
    FLOAT_VIEW.setUint32(0, read(raw, Y_AT), true);
    return { x, y: FLOAT_VIEW.getFloat32(0, true) };
}

// 원본 프레임 DWORD(패치판 +24) 네 바이트만 쓰며 다른 필드는 보존한다. (2026-11-09 WB04-E209)
function writeFrame(pool, parent, frame) {
    const raw = pool.allocated(parent);
    const value = frame >>> 0;
    // 낮은 바이트부터 네 바이트를 쓴다.
    for (let index = 0; index < 4; index += 1) raw[FRAME_AT + index] = (value >>> (index * 8)) & 0xff;
}

// 0048ba30: round 가 아니라 원본 43fa50 의 +0.5 뒤 signed64 절삭·하위 DWORD 감김이다. (2026-11-09 WB04-E209)
export function bulfTurningFrame(frames, angle, counterClockwise) {
    if (counterClockwise !== 0 && counterClockwise !== 1) throw new RangeError('Bulf 회전 방향은 0/1 이어야 한다');
    const run = frames.run(counterClockwise ? LETTER_F : LETTER_E);
    const scaled = Math.fround(angle * ANGLE_SCALE * (run.count - 1));
    const rounded = scaled + 0.5;
    const integer = rounded >= -FTOL_LIMIT && rounded < FTOL_LIMIT ? Math.trunc(rounded) | 0 : 0;
    return (integer + run.first) | 0;
}

// 0048b760 전체 분기이다. Finished 보다 먼저 immobile 을 조회하고, 다음 프레임이 같으면 위치도 갱신하지 않는다. (2026-11-09 WB04-E209)
export function runBulfFrame(pool, parent, runtime, frames, shapes, hooks, watch) {
    if (pool?.edition !== 'patch1078') throw new TypeError('Bulf41 은 10.78 전용이다');
    if (typeof hooks?.immobile !== 'function' || typeof hooks.halt !== 'function' || typeof hooks.step !== 'function' || typeof hooks.arrive !== 'function' ||
        typeof hooks.unpop !== 'function' || typeof hooks.pop !== 'function' || !watch) throw new TypeError('Bulf41 프레임의 필수 연결 누락');
    if (hooks.immobile()) return;
    const previous = runtime.angle;
    if (runtime.finished || hooks.halt() || !watch.value) return;
    const sample = hooks.step();
    if (!watch.value) return;
    if (sample.finished) {
        hooks.arrive();
        return;
    }
    const raw = pool.slot(parent);
    const type = raw[TYPE_AT];
    const current = read(raw, FRAME_AT) | 0;
    if (type >= frames.length || type >= shapes.length || current < 0 || current >= shapes[type].frames.length) throw new RangeError('Bulf41 현재 TYPE/SHP 프레임 범위');
    const point = position(pool, parent);
    const shape = shapes[type].frames[current];
    const footX = Math.fround(point.x + shape.walkStepX);
    const footY = Math.fround(point.y + shape.walkStepY);
    // C++ 정수 나눗셈처럼 0 방향으로 자른다.
    const direction = (runtime.direction(runtime.current) / 2) | 0;
    let next = current;
    if (previous !== sample.angle) {
        const counterClockwise = sample.angle < previous || sample.angle - previous > 180;
        next = bulfTurningFrame(frames[type], sample.angle, counterClockwise ? 1 : 0);
        if (next >= frames[type].codes.length) throw new RangeError('Bulf 회전 프레임 원본 진단 5bc');
    } else if (sample.angle % 90 === 0) {
        const dx = sample.position.x - point.x;
        const dy = sample.position.y - point.y;
        const fx = footX - point.x;
        const fy = footY - point.y;
        if (fx * fx + fy * fy <= dx * dx + dy * dy) {
            if (direction < 0 || direction >= 4) throw new RangeError('Bulf41 원본 A~D 방향 범위');
            const side = pathDirection(frames[type], current);
            if (side === 4 || side === 5) next = AFTER_TURNING[direction];
            const run = frames[type].run(LETTER_A + direction);
            if (!run.count) throw new RangeError('Bulf41 보행 구간 0');
            next = (((next + 1) | 0) % run.count + run.first) | 0;
            if (next >= frames[type].codes.length) throw new RangeError('Bulf 보행 프레임 원본 진단 5cd');
        }
    }
    if (next === current) return;
    hooks.unpop(parent, 0);
    if (!watch.value) return;
    writeFrame(pool, parent, next);
    hooks.pop(parent, sample.position, POP_FLAGS);
    if (!watch.value) return;
    if (((next + 1) >>> 0) % 4 === 0) {
        if (typeof hooks.randomInt !== 'function' || typeof hooks.sound !== 'function') throw new Error('Bulf41 공용 난수/발소리 미연결');
        const variant = (hooks.randomInt(1000, 6000) / 1000) | 0;
        const spot = position(pool, parent);
        hooks.sound(spot.x, spot.y, `bulfWalk${variant}.wav`);
    }
}
