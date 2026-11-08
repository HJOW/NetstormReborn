// 일반 Dude task 설정(00450670)을 옮긴 모듈이다(cpppj `o/RawDudeTask.cpp/.h`, 10.78 전용). Dude 생성과 이동은 별도 소비자이다.
//  - set(sid, event, delay): 현재 슬롯과 예약 머리/free SID 쓰기 계약을 확인한 뒤 float 지연과 현재 시각이 유한한지 확인하고, 이전 task WORD(+12)가 낙하(0x5105)인데 새 event 가 다르면 원본 진단
//    (패치는 DudeTaskFallAssertError)을 올린다. 그 뒤 현재 Regular(마스크 fff0/값 5100)를 조회하고 없으면 실제 생성(payload 5·flags 0x50)을 요구하며, 같은 Regular 를 event/count 0/`현재 시각 + 지연`(double 합)으로 갱신하고
//    task WORD(+12)에 event 하위 16비트를 쓴 뒤 현재 TYPE 이 다리(82)이면 실제 표면 통지를 부른다. 지연은 먼저 double 로 승격하며 원본 x87 의 현재 시각 정밀도를 유지한다.
//  - makeDudeTaskHostHooks(pool, host, hooks): 기존 Regular 객체와 실제 생성/부착을 같은 Host 에서 재사용하는 조회/생성 훅을 만든다.
// 미연결 호출은 도달했을 때 거부한다(쓰기 앞서는 빈 성공 금지). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E76: 일반 Dude task 이식으로 추가)

// 낙하 중 작업 교체를 막는 원본 task 번호이다. (2026-11-08 WB04-E76)
const FALL_TASK = 0x5105;
// task WORD 위치(+12)와 현재 TYPE 위치(+10)이다. (2026-11-08 WB04-E76)
const TASK_WORD = 12;
const TYPE = 10;
// 다리 task WORD 쓰기 뒤 표면 통지를 받는 기본 타입이다. (2026-11-08 WB04-E76)
const BRIDGE_TYPE = 82;
// 일반 task Regular 조회 마스크/값과 생성 payload/flags 이다. (2026-11-08 WB04-E76)
const TASK_MASK = 0xfff0;
const TASK_VALUE = 0x5100;
const TASK_PAYLOAD = 5;
const TASK_FLAGS = 0x50;

// 낙하 task 교체를 막는 원본 진단(패치는 assert 중단)을 가리키는 오류이다. 정상 반환과 구별한다. (2026-11-08 WB04-E76)
export class DudeTaskFallAssertError extends Error {
    constructor() {
        super('Dude task FALL 교체 원본 assert');
        this.name = 'DudeTaskFallAssertError';
    }
}

// 도달한 하위 호출만 요구하고 빈 성공으로 바꾸지 않는다. (2026-11-08 WB04-E76)
function need(hook, message) {
    if (typeof hook !== 'function') throw new Error(message);
    return hook;
}

export class RawDudeTask {
    // 같은 SID 풀, 시계/다리 타입 전역 묶음, 실제 Regular 조회/생성/표면 통지 훅이다.
    #pool;
    #state;
    #hooks;

    // state = { clock: { now: 현재 double 게임 시각 }, bridgeType?: 82 }, hooks = { taskRegular(sid), addTaskRegular(sid, event), notifySurface(sid) } 이다. (2026-11-08 WB04-E76)
    constructor(pool, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Dude task 는 PATCH 전용입니다');
        if (!state || !state.clock) throw new TypeError('Dude task 의 현재 시계가 필요합니다');
        this.#pool = pool;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 실제 factory/드롭 소비자가 다른 월드 풀을 혼용하지 않도록 현재 풀을 제공한다. (2026-11-08 WB04-E76)
    get pool() {
        return this.#pool;
    }

    // event 는 Regular DWORD 와 raw 하위 WORD 에 쓰고 delay(float32)는 원본 float 폭으로 더한다. 패치 낙하 교체는 원본 진단 경계이다. (2026-11-08 WB04-E76)
    set(sid, event, delay) {
        const pool = this.#pool;
        const state = this.#state;
        const raw = pool.slot(sid);
        // 새 계약의 쓰기는 예약 머리/free SID 를 원본의 미정의 쓰기 전에 거부한다.
        pool.allocated(sid);
        const wait = Math.fround(delay);
        const now = state.clock.now;
        if (!Number.isFinite(wait) || !Number.isFinite(now)) throw new RangeError('Dude task 유한 시각 입력 오류');
        const previous = raw[TASK_WORD] | (raw[TASK_WORD + 1] << 8);
        const task = event >>> 0;
        if (previous === FALL_TASK && task !== FALL_TASK) throw new DudeTaskFallAssertError();
        let regular = need(this.#hooks.taskRegular, 'Dude task 현재 Regular 조회 미연결')(sid);
        if (!regular) {
            regular = need(this.#hooks.addTaskRegular, 'Dude task 실제 Regular 생성 미연결')(sid, task);
            if (!regular) throw new Error('Dude task 실제 Regular 생성 실패');
        }
        regular.resetTask(task, state.clock.now + wait);
        const bytes = pool.allocated(sid);
        bytes[TASK_WORD] = task & 0xff;
        bytes[TASK_WORD + 1] = (task >>> 8) & 0xff;
        if (pool.slot(sid)[TYPE] === (state.bridgeType ?? BRIDGE_TYPE)) need(this.#hooks.notifySurface, 'Dude task 실제 표면 통지 미연결')(sid);
    }
}

// 일반 task Regular 의 실제 조회/생성만 같은 Host 에 연결한다. host.pool 이 pool 과 다르면 거부한다. (2026-11-08 WB04-E76)
export function makeDudeTaskHostHooks(pool, host, hooks = {}) {
    if (pool !== host.pool) throw new TypeError('Dude task/Host SID 풀 불일치');
    return {
        ...hooks,
        taskRegular: (sid) => host.findEventMasked(sid, TASK_MASK, TASK_VALUE),
        addTaskRegular: (sid, event) => host.addRegular(sid, event, TASK_PAYLOAD, TASK_FLAGS)
    };
}
