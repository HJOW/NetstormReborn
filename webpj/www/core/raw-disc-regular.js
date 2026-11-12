// 썬 디스크(TYPE 83)의 가상 Handle(방향별 modulo 프레임 진행)을 옮긴 모듈이다(cpppj `o/RawDiscRegular.cpp/.h`, 10.78 전용: 00469540 Handle, 가상 표 0x50b570).
//  - new RawDiscRegular(pool, types, setter): types[n] = { directions(TYPE+110 방향 수), frameCount(TYPE+114 물리 프레임 수) }(`readRawArcherTypeData` 결과와 같은 모양, 188칸), setter 는 같은 풀의 SquidFrame 이다.
//  - static nextFrame(current, frameCount, directions): 현재 프레임이 음수이거나 int32 최댓값이거나 방향 수가 1 미만이거나 프레임 수가 방향 수보다 작으면 거부하고 그 밖에는 nextFrameNative 이다.
//  - static nextFrameNative(current, frameCount, directions): 원문의 signed IDIV 세 번(프레임 수÷방향 수 = 방향당 프레임, 다음 프레임 % 방향당, 현재 프레임 % 방향당)과 DWORD 감김을 그대로 한다:
//    결과 = (다음 % 방향당) − (현재 % 방향당) + 현재(int32 로 감김). 0 분모와 INT_MIN ÷ −1 은 하드웨어 나눗셈 오류라 `RangeError('Disc 원본 IDIV 오류')` 로 표현한다. 나머지의 부호는 피제수를 따른다(−0 은 0 으로 정규화).
//  - handles(sid): 현재 raw 가상 표 0x50b570 인지 확인한다(타입 번호로 추측하지 않음). handle(sid, event, count, payload): event/count 는 원본도 쓰지 않으며 현재 프레임을 진행한 뒤 payload 를 그대로 돌려준다.
//  - static handleIn(pool, setter, type, sid, payload, nativeArithmetic = false): 같은 본문으로 현재 TYPE getter `type(번호) → { directions, frameCount }` 를 먼저 읽고(getter 가 슬롯을 바꾸면 현재 프레임은 그 뒤 값),
//    프레임 DWORD(+36, signed)를 읽어 다음 프레임을 `setter.set(sid, 다음, 0)` 로 지정한다. Native 장기 소비자는 nativeArithmetic = true 로 원래 정수 산술을 쓴다.
//  - handler(fallback): 썬 디스크의 가상 Handle 만 선택하고 다른 부모는 기존 처리기로 넘긴다(없으면 거부).
// 10.72(CD)의 BYTE 프레임은 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E364: 7-P2 RawDiscRegular 이식으로 추가)

// 썬 디스크 가상 표(10.78), 프레임 DWORD·TYPE 슬롯 위치, 188칸 TYPE 표, int32 한계이다. (2026-11-13 WB04-E364)
const DISC_TABLE = 0x50b570;
const FRAME_AT = 36;
const TYPE_AT = 10;
const TYPE_COUNT = 188;
const INT32_MIN = -2147483648;
const INT32_MAX = 2147483647;

// 비정렬 리틀엔디언 DWORD 를 읽는다. (2026-11-13 WB04-E364)
function readDword(raw, at) {
    let value = 0;
    // 지정된 네 바이트만 낮은 자리부터 조합한다.
    for (let index = 0; index < 4; index += 1) value += raw[at + index] * 2 ** (8 * index);
    return value;
}

// x86 signed IDIV 의 0 분모와 몫 overflow(INT_MIN ÷ −1)만 오류로 하고 음수 피연산자는 그대로 허용한다(몫은 0 쪽으로 절삭). (2026-11-13 WB04-E364)
function divide(value, divisor) {
    if (!divisor || (value === INT32_MIN && divisor === -1)) throw new RangeError('Disc 원본 IDIV 오류');
    return Math.trunc(value / divisor) | 0;
}

// 나머지도 같은 IDIV 오류 조건을 먼저 확인한다(부호는 피제수를 따르며 −0 은 0). (2026-11-13 WB04-E364)
function remainder(value, divisor) {
    divide(value, divisor);
    return (value % divisor) | 0;
}

// 썬 디스크 가상 Handle 의 방향별 반복 처리기이다(글자별 프레임 구간 처리기와 구별). (2026-11-13 WB04-E364)
export class RawDiscRegular {
    #pool;
    #types;
    #setter;

    // PATCH 풀·같은 풀의 지정기·188칸 TYPE 표를 확인한다. (2026-11-13 WB04-E364)
    constructor(pool, types, setter) {
        if (pool?.edition !== 'patch1078') throw new TypeError('썬 디스크 Regular 는 PATCH 만 복원했습니다');
        if (setter?.pool !== pool) throw new TypeError('Disc Regular 지정기 SID 풀 불일치');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT) throw new RangeError('Disc Regular TYPE 표 크기 오류');
        this.#pool = pool;
        this.#types = types;
        this.#setter = setter;
    }

    // 정상 signed32 입력에서 방향당 프레임 수의 modulo 를 정확히 적용한다. (2026-11-13 WB04-E364)
    static nextFrame(current, frameCount, directions) {
        if (current < 0 || current === INT32_MAX || directions <= 0 || frameCount < directions) throw new RangeError('Disc Regular 현재 프레임/TYPE 나눗셈 범위 오류');
        return RawDiscRegular.nextFrameNative(current, frameCount, directions);
    }

    // 원문 LEA/SUB/ADD 는 DWORD 로 감기며 세 나눗셈의 signed 나머지를 그대로 쓴다. (2026-11-13 WB04-E364)
    static nextFrameNative(current, frameCount, directions) {
        const stride = divide(frameCount, directions);
        const next = (current + 1) | 0;
        return (remainder(next, stride) - remainder(current, stride) + current) | 0;
    }

    // 원본 가상 표 기록만 확인하며 보호 PE 주소를 호스트 함수로 실행하지 않는다. (2026-11-13 WB04-E364)
    handles(sid) {
        return readDword(this.#pool.slot(sid), 0) === DISC_TABLE;
    }

    // 00469540: event/count 는 원본도 쓰지 않는다. 프레임 지정 뒤 payload 전체 float 를 유지한다. (2026-11-13 WB04-E364)
    handle(sid, event, count, payload) {
        if (!this.handles(sid)) throw new TypeError('Disc Regular 현재 가상 표 미지원');
        return RawDiscRegular.handleIn(this.#pool, this.#setter, (type) => {
            if (type >= this.#types.length) throw new RangeError('Disc Regular TYPE 오류');
            return this.#types[type];
        }, sid, payload);
    }

    // 현재 TYPE 을 먼저 선택하고 같은 Frame.Set 뒤 payload 를 돌려준다. 슬롯은 살아 있는 보기로 읽어 TYPE getter 가 바꾼 현재 프레임을 본다. (2026-11-13 WB04-E364)
    static handleIn(pool, setter, type, sid, payload, nativeArithmetic = false) {
        if (typeof type !== 'function') throw new Error('Disc Regular 현재 TYPE getter 미연결');
        const raw = pool.physical(sid);
        const data = type(raw[TYPE_AT]);
        const current = readDword(raw, FRAME_AT) | 0;
        const next = nativeArithmetic ? RawDiscRegular.nextFrameNative(current, data.frameCount, data.directions) : RawDiscRegular.nextFrame(current, data.frameCount, data.directions);
        setter.set(sid, next, 0);
        return payload;
    }

    // 다른 부모 처리기는 같은 입력을 유지한다. 미지정 fallback 은 명시 오류이다. (2026-11-13 WB04-E364)
    handler(fallback = null) {
        return (sid, event, count, payload) => {
            if (this.handles(sid)) return this.handle(sid, event, count, payload);
            if (!fallback) throw new Error('Disc 외부 Regular 부모 미연결');
            return fallback(sid, event, count, payload);
        };
    }
}
