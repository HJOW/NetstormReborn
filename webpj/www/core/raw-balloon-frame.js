// 벌룬 경로(Balloon42)의 실제 프레임 0048ad60 — 이륙/착륙 구간 고도 BYTE 갱신과 화염 소리·도착·공간 재등록 순서를 옮긴 모듈이다(cpppj `o/RawBalloonFrame.cpp/.h`, 10.78 전용).
//  - runBalloonFrame(pool, parent, runtime, now, balloonType, hooks, watch):
//    · pool 은 10.78 SidPool, parent 는 벌룬 SID, runtime 은 같은 경로의 RawPathRuntime(`raw-path-runtime.js`), now 는 현재 게임 시각(double), balloonType 은 화염 소리를 내는 벌룬 TYPE 번호다.
//    · hooks = { halt()→bool(00489ff0 중단 판정), step()→{ position, angle, finished }(0048a330 Step), arrive()(가상+24 의 0048b340 도착), unpop(sid, flags)(가상+48),
//      pop(sid, point, flags)(가상+90), flame(x, y)(현재 좌표 balloonFlame.wav/loop 0) }. watch 는 `{ value }` 상자이며 외부 효과가 자기 자신을 삭제하면 false 가 된다.
//    · 순서: 완료/중단이면 반환 → Step(뒤 watch 확인) → 현재 단계 1(이륙)이면 고도 = max(현재 BYTE, ftol(44 − 남은 시각×44/구간 길이)), 그 값이 0 이고 현재 TYPE 이 벌룬이면 화염(뒤 watch 확인)
//      → 마지막 구간이면 고도 = min(현재 BYTE, ftol(남은 시각×44/구간 길이)) → Step 완료면 도착만 → 좌표가 바뀌었을 때만 Unpop(0)(뒤 watch 확인)·고도 BYTE 쓰기(−1 표식이면 생략)·Pop(0x40).
//    · 남은 시각은 float 로 한 번 저장하고, 구간 길이는 0.001f 미만(NaN 은 원문 비교대로 float 변환)이면 0.001f 다. 정수화는 원본 _ftol 하위 DWORD(비유한/±2^63 밖 0)다.
//      고도 비교의 기존 BYTE 는 Step 직후 값을 그대로 쓰며(원본 ebp 보존), 좌표 비교는 화염 효과 뒤의 현재 부모 좌표를 다시 읽는다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E209: 이동 유닛 경로 프레임 3종 이식으로 추가)

// 원본 ROM 50481c 의 순항 고도 44.0f 와 509144 의 최소 구간 길이 0.001f 이다. (2026-11-09 WB04-E209)
const CRUISE_HEIGHT = 44;
const MINIMUM_DURATION = Math.fround(0.0010000000474974513);
// CRT signed64 절삭이 정의된 경계(2^63)이다. (2026-11-09 WB04-E209)
const FTOL_LIMIT = 9223372036854775808;
// 슬롯의 TYPE·좌표·고도 위치와 재등록 flags·고도 미기록 표식이다. (2026-11-09 WB04-E209)
const TYPE_AT = 10;
const X_AT = 14;
const Y_AT = 18;
const HEIGHT_AT = 30;
const POP_FLAGS = 0x40;
const NO_HEIGHT = -1;

// float 비트 변환용 공용 보기이다. (2026-11-09 WB04-E209)
const FLOAT_VIEW = new DataView(new ArrayBuffer(4));

// 원본 _ftol 의 signed64 절삭 뒤 하위 DWORD 를 signed 로 돌려준다. 비유한·범위 밖은 0 이다. (2026-11-09 WB04-E209)
function truncate(value) {
    if (!(value >= -FTOL_LIMIT && value < FTOL_LIMIT)) return 0;
    return Math.trunc(value) | 0;
}

// 현재 부모의 비정렬 float 좌표 두 개를 직접 읽는다(보관 사본이 아닌 현재 슬롯). (2026-11-09 WB04-E209)
function position(pool, parent) {
    const raw = pool.slot(parent);
    const read = (offset) => {
        FLOAT_VIEW.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
        return FLOAT_VIEW.getFloat32(0, true);
    };
    return { x: read(X_AT), y: read(Y_AT) };
}

// 0048ad60 전체이다. 화염 효과 뒤 현재 좌표는 다시 읽지만 고도 비교의 BYTE 는 Step 직후 값을 보존한다. (2026-11-09 WB04-E209)
export function runBalloonFrame(pool, parent, runtime, now, balloonType, hooks, watch) {
    if (runtime.finished || hooks.halt()) return;
    const sample = hooks.step();
    if (!watch.value) return;
    const current = runtime.current;
    // 원본 fstp DWORD 의 저장 지점이다.
    const remaining = Math.fround(runtime.physicalTime(current) - now);
    const difference = runtime.physicalTime(current) - runtime.physicalTime(current - 1);
    // NaN 비교도 원문 분기를 따른다(NaN < 0.001 은 거짓이라 float 변환한 NaN 을 쓴다).
    const duration = difference < MINIMUM_DURATION ? MINIMUM_DURATION : Math.fround(difference);
    const oldHeight = pool.slot(parent)[HEIGHT_AT];
    let height = NO_HEIGHT;
    if (current === 1) {
        height = Math.max(oldHeight, truncate(CRUISE_HEIGHT - remaining * CRUISE_HEIGHT / duration));
        if (height === 0 && pool.slot(parent)[TYPE_AT] === balloonType) {
            const point = position(pool, parent);
            hooks.flame(point.x, point.y);
        }
        if (!watch.value) return;
    }
    if (current === runtime.count - 1) height = Math.min(oldHeight, truncate(remaining * CRUISE_HEIGHT / duration));
    if (sample.finished) {
        hooks.arrive();
        return;
    }
    // 현재 좌표가 그대로면 고도 BYTE 도 쓰지 않는다.
    const point = position(pool, parent);
    if (sample.position.x !== point.x || sample.position.y !== point.y) {
        hooks.unpop(parent, 0);
        if (!watch.value) return;
        if (height !== NO_HEIGHT) pool.allocated(parent)[HEIGHT_AT] = height & 0xff;
        hooks.pop(parent, sample.position, POP_FLAGS);
    }
}
