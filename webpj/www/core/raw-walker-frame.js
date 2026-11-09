// 보행 경로(Walker38)의 실제 프레임 0048b080 — 세일 스케이터(윈드)의 공유 counter 직각/회전 프레임과 크리스탈 크랩(레인)의 0.08초 예약 보행 프레임을 같은 부모에 적용하는 모듈이다(cpppj `o/RawWalkerFrame.cpp/.h`, 10.78 전용).
//  - runWalkerFrame(pool, parent, runtime, now, frames, windType, rainType, windCounter, hooks, watch):
//    · pool 은 10.78 SidPool, runtime 은 같은 경로의 RawPathRuntime, now 는 현재 게임 시각(double), frames 는 TYPE 별 FrameCodeTable 배열, windType/rainType 은 두 보행 TYPE 번호,
//      windCounter 는 윈드 전체가 공유하는 counter 의 `{ value }` 상자(uint32), watch 는 자기 삭제 감시 `{ value }` 상자다.
//    · hooks = { immobile()→bool(004287a0), halt()→bool(부모+38 정지 검사), step()→{ position, angle, finished }(0048a330), groundStop(point)→bool(0048a7c0, 자신을 삭제할 수 있음),
//      arrive()(0048b340), unpop(sid, flags)(부모+48), pop(sid, point, flags)(부모+90) } 이며 모두 필요하다.
//    · 순서: immobile/삭제/완료/중단이면 반환 → Step(뒤 watch) → 완료면 도착만 → groundStop 이면(또는 삭제면) 반환 →
//      윈드: counter=(counter+1)&3, 각도 90/180/270 이면 +12/+8/+4, 그 밖의 0 이 아닌 각도는 회전 프레임 ftol((1−angle/360)×19 float 저장 +0.5)+16 /
//      레인: 예약 시각 ≤ now 일 때만 0/180 도는 B, 90/270 도는 A 구간에서 다음 프레임(구간 밖이면 첫 번호)과 now+0.08 예약, 비직각이면 이동도 건너뛰고 반환 / 그 밖의 TYPE 은 원본 진단 553 오류
//      → 좌표와 프레임이 모두 같으면 반환 → Unpop(0)(뒤 watch)·프레임 DWORD 쓰기·Pop(0x40).
// CD 10.72 는 지원하지 않는다(풀 판본 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E209: 이동 유닛 경로 프레임 3종 이식으로 추가)

// 원본 0050b534 의 float 1/360 과 0050e1a0 의 회전 프레임 수 19.0f, 00509c60 의 레인 보행 주기(double)이다. (2026-11-09 WB04-E209)
const ANGLE_SCALE = Math.fround(0.0027777778450399637);
const TURNING_FRAMES = 19;
const FRAME_DELAY = 0.07999999821186066;
// CRT signed64 절삭이 정의된 경계(2^63)이다. (2026-11-09 WB04-E209)
const FTOL_LIMIT = 9223372036854775808;
// 슬롯의 TYPE·좌표·프레임 위치와 재등록 flags, 레인 A/B 글자 코드, 회전 프레임 시작 번호이다. (2026-11-09 WB04-E209)
const TYPE_AT = 10;
const X_AT = 14;
const Y_AT = 18;
const FRAME_AT = 36;
const POP_FLAGS = 0x40;
const LETTER_A = 0x41;
const LETTER_B = 0x42;
const TURNING_FIRST = 16;

// float 비트 변환용 공용 보기이다. (2026-11-09 WB04-E209)
const FLOAT_VIEW = new DataView(new ArrayBuffer(4));

// 부모의 비정렬 DWORD 만 리틀엔디언으로 부호 없이 읽는다. (2026-11-09 WB04-E209)
function read(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// DWORD 비트를 float 로 읽는다. (2026-11-09 WB04-E209)
function floatAt(raw, offset) {
    FLOAT_VIEW.setUint32(0, read(raw, offset), true);
    return FLOAT_VIEW.getFloat32(0, true);
}

// 43fa50 에 넘기기 전 float 저장과 +0.5/CRT 절삭의 하위 DWORD 에 16 을 더한 uint32 이다. (2026-11-09 WB04-E209)
function turningFrame(angle) {
    const scaled = Math.fround((1 - angle * ANGLE_SCALE) * TURNING_FRAMES);
    const rounded = scaled + 0.5;
    const result = rounded >= -FTOL_LIMIT && rounded < FTOL_LIMIT ? Math.trunc(rounded) | 0 : 0;
    return (result + TURNING_FIRST) >>> 0;
}

// 0048b080 전체이다. 프레임이 같아도 좌표가 다르면 Unpop/Pop 하며, 비직각 레인 예약 분기는 이동도 건너뛴다. (2026-11-09 WB04-E209)
export function runWalkerFrame(pool, parent, runtime, now, frames, windType, rainType, windCounter, hooks, watch) {
    if (pool?.edition !== 'patch1078' || typeof hooks?.immobile !== 'function' || typeof hooks.halt !== 'function' || typeof hooks.step !== 'function' ||
        typeof hooks.groundStop !== 'function' || typeof hooks.arrive !== 'function' || typeof hooks.unpop !== 'function' || typeof hooks.pop !== 'function' || !watch || !windCounter) {
        throw new TypeError('Walker38 판본/필수 효과 누락');
    }
    if (hooks.immobile() || !watch.value || runtime.finished || hooks.halt() || !watch.value) return;
    const sample = hooks.step();
    if (!watch.value) return;
    if (sample.finished) {
        hooks.arrive();
        return;
    }
    if (hooks.groundStop(sample.position) || !watch.value) return;
    const raw = pool.slot(parent);
    const type = raw[TYPE_AT];
    const current = read(raw, FRAME_AT);
    let next = current;
    if (type === windType) {
        windCounter.value = ((windCounter.value + 1) & 3) >>> 0;
        next = windCounter.value;
        if (sample.angle === 90) next += 12;
        else if (sample.angle === 180) next += 8;
        else if (sample.angle === 270) next += 4;
        else if (sample.angle !== 0) next = turningFrame(sample.angle);
    } else if (type === rainType) {
        if (runtime.nextFrame <= now) {
            // 레인 A 는 수직, B 는 수평 이동에 쓴다.
            let side;
            if (sample.angle === 0 || sample.angle === 180) side = LETTER_B;
            else if (sample.angle === 90 || sample.angle === 270) side = LETTER_A;
            else return;
            if (type >= frames.length) throw new RangeError('Walker38 레인 TYPE 범위');
            const run = frames[type].run(side);
            const frame = current | 0;
            if (!run.count) throw new RangeError('Walker38 레인 보행 구간 0');
            next = (frame >= run.first && frame < run.first + run.count ? (frame - run.first + 1) % run.count + run.first : run.first) >>> 0;
            runtime.setNextFrame(now + FRAME_DELAY);
        }
    } else throw new RangeError('Walker38 원본 부모 진단 553');
    if (sample.position.x === floatAt(raw, X_AT) && sample.position.y === floatAt(raw, Y_AT) && next === current) return;
    hooks.unpop(parent, 0);
    if (!watch.value) return;
    const writable = pool.allocated(parent);
    // 원본 부모+24 의 네 바이트만 바꾼다.
    for (let index = 0; index < 4; index += 1) writable[FRAME_AT + index] = (next >>> (8 * index)) & 0xff;
    hooks.pop(parent, sample.position, POP_FLAGS);
}
