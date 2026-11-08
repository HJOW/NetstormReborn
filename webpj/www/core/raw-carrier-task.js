// Carrier 426db0 의 현재 task 조회/생성/낙하 유지/시간 재설정을 옮긴 모듈이다(cpppj `o/RawCarrierTask.cpp/.h`, 10.78 전용). Dude task 의 WORD 쓰기와 구별하며 Carrier 의 기존 Regular 자체를 갱신한다.
// 순서: 부모의 현재 Regular(마스크 fff0 / 5100 계열)를 조회 → 없으면 new(0x28) 확보(실패는 원본 NULL 쓰기 경로이므로 오류) → 확보한 메모리에 생성자(task 가 5105 가 아니면 공유 형)를 실행해 다음 기한만 쓴다(생성/부착 효과가 바꾼 event/count 는 보존)
//   → 있으면 기존 event 가 5105(낙하)일 때 새 task 가 다르면 원문 진단(Carrier.cpp 407) 뒤에도 낙하(5105)를 유지하고, 모든 경우 event 를 정한 값으로, count 를 0 으로, 기한을 `현재 시각 + 지연` 으로 쓴다(payload 는 바꾸지 않음).
// 기한은 x87 의 FLD(float 지연)/FADD(double 현재 시각) 와 같은 NaN payload/부호 선택과 quiet 변환을 보존한다. 현재 시각은 생성/진단 콜백 뒤에 읽는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E72: Carrier task 이식으로 추가)

// 낙하 task 의 event 번호와 double/float NaN 판정용 비트 마스크이다. (2026-11-08 WB04-E72)
const FALL_TASK = 0x5105;
const DOUBLE_EXPONENT = 0x7ff0000000000000n;
const DOUBLE_FRACTION = 0x000fffffffffffffn;
const DOUBLE_QUIET = 0x0008000000000000n;
const FLOAT_EXPONENT = 0x7f800000;
const FLOAT_FRACTION = 0x007fffff;
const FLOAT_SIGN = 0x80000000;

// 원본 new(0x28) 확보 실패(NULL 쓰기 경로)를 가리키는 오류이다. 정상 예약으로 바꾸지 않는다. (2026-11-08 WB04-E72)
export class CarrierTaskAllocationError extends Error {
    constructor() {
        super('캐리어 task 확보 실패(NULL 쓰기 경로)');
        this.name = 'CarrierTaskAllocationError';
    }
}

// 실제로 도달한 하위만 요구해 조기 경로에 불필요한 효과를 강제하지 않는다. (2026-11-08 WB04-E72)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('캐리어 task 실제 하위 미연결');
    return hook;
}

// double 의 64비트 패턴을 BigInt 로 읽고 되돌리는 공용 DataView 이다. (2026-11-08 WB04-E72)
const view = new DataView(new ArrayBuffer(8));
const doubleBits = (value) => { view.setFloat64(0, value, true); return view.getBigUint64(0, true); };
const bitsDouble = (bits) => { view.setBigUint64(0, bits, true); return view.getFloat64(0, true); };
const floatBits = (value) => { view.setFloat32(0, value, true); return view.getUint32(0, true); };

// 실제 FLD(float 지연)/FADD(double 현재 시각)의 NaN payload/부호 선택과 quiet 변환을 보존한다. (2026-11-08 WB04-E72)
function scheduleTime(delay, now) {
    const current = doubleBits(now);
    const rawDelay = floatBits(delay);
    const currentNaN = (current & DOUBLE_EXPONENT) === DOUBLE_EXPONENT && (current & DOUBLE_FRACTION) !== 0n;
    const delayNaN = (rawDelay & FLOAT_EXPONENT) === FLOAT_EXPONENT && (rawDelay & FLOAT_FRACTION) !== 0;
    if (!delayNaN) {
        if (currentNaN) return bitsDouble(current | DOUBLE_QUIET);
        return delay + now;
    }
    // FLD 가 먼저 float signaling NaN 을 quiet 으로 바꾸고 double 로 넓힌다.
    const expanded = (BigInt(rawDelay & FLOAT_SIGN) << 32n) | DOUBLE_EXPONENT | DOUBLE_QUIET | (BigInt(rawDelay & FLOAT_FRACTION) << 29n);
    if (!currentNaN) return bitsDouble(expanded);
    const left = expanded & DOUBLE_FRACTION;
    const right = current & DOUBLE_FRACTION;
    if (left > right) return bitsDouble(expanded);
    if (right > left) return bitsDouble(current);
    // 유효숫자가 같으면 양의 부호를 우선하며 둘 다 음수일 때만 음수다.
    return bitsDouble(expanded & current);
}

// pool 은 10.78 SidPool, now 는 현재 double 시각을 돌려주는 getter, hooks = { find(부모)→RegularProcess|null, allocate()→bool, construct(부모, task, 공유 여부)→RegularProcess, assertion() } 이다. (2026-11-08 WB04-E72)
export class RawCarrierTask {
    // 원본 부모 번호를 사용하는 풀, 생성/진단 콜백 뒤 읽는 현재 시각 getter, 기존 프로세스와 같은 확보 메모리의 실제 생성 효과이다.
    #pool;
    #now;
    #hooks;

    // 판본/현재 시각 연결을 구성 때 확인한다. (2026-11-08 WB04-E72)
    constructor(pool, now, hooks) {
        if (pool?.edition !== 'patch1078' || typeof now !== 'function') throw new TypeError('캐리어 task는 PATCH 전용입니다');
        this.#pool = pool;
        this.#now = now;
        this.#hooks = hooks ?? {};
    }

    // 다른 Host/Session 의 상태 혼합을 거부할 때 쓰는 풀이다. (2026-11-08 WB04-E72)
    get pool() {
        return this.#pool;
    }

    // 현재 낙하를 보존하고 payload 를 바꾸지 않으며 count 0/새 기한을 쓴다. delay 는 float32 지연(NaN payload 보존)이다. (2026-11-08 WB04-E72)
    set(parent, task, delay) {
        const hooks = this.#hooks;
        let process = need(hooks.find)(parent);
        let effective = task >>> 0;
        if (!process) {
            // 원본의 NULL+10 쓰기를 정상 예약으로 바꾸지 않는다.
            if (!need(hooks.allocate)()) throw new CarrierTaskAllocationError();
            process = need(hooks.construct)(parent, task >>> 0, (task >>> 0) !== FALL_TASK);
            if (!process) throw new Error('캐리어 task 생성자의 NULL 반환');
            // 생성/부착 효과가 바꾼 event/count 도 원본처럼 보존한다.
            process.setNextTime(scheduleTime(delay, this.#now()));
            return;
        }
        if (process.event === FALL_TASK) {
            if ((task >>> 0) !== FALL_TASK) need(hooks.assertion)();
            effective = FALL_TASK;
        }
        process.resetTask(effective, scheduleTime(delay, this.#now()));
    }
}
