// 신전 의식이 쓰는 사제 K~P 프레임 선택과 604 공유 예약(00491ca0/00491cf0)을 실제 프레임 표/호스트에 연결해 옮긴 모듈이다(cpppj `o/RawPriestRitualActions.cpp/.h`, 10.78 전용).
//  - begin(sid, 단계): 유효 번호(1..capacity-1 이고 WORD 범위)만 현재 raw 타입의 `K + 단계` 글자 구간 첫 프레임으로 바꾼다(free/dead/void 를 별도 조건으로 쓰지 않음). 단계 0..5 는 실제 타입의 K..P 구간이며 그 밖의 손상 단계는 거부한다.
//  - continue(sid, 단계): 같은 범위 검사 → begin → 예약(new 0x28)이 성공하면 604(= 0x25c, payload float32 0.01) 공유 Regular 를 예약한다. 기존 같은 사건은 취소하지 않으며 프레임 효과 뒤 상태가 달라도 처음 유효했던 번호로 계속 요청한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E70: 사제 의식 동작 이식으로 추가)

// 후속 604 사건 번호(0x25c), K..P 첫 프레임 표의 마지막 단계, 후속 payload(float32 0.01), 슬롯의 TYPE 위치, 타입 표 크기, 첫 글자 K 이다. (2026-11-08 WB04-E70)
const RITUAL_FOLLOW_EVENT = 0x25c;
const LAST_RITUAL_TABLE_STAGE = 5;
const RITUAL_FOLLOW_PAYLOAD = Math.fround(0.01);
const TYPE_OFFSET = 10;
const TYPE_COUNT = 188;
const LETTER_K = 0x4b;
const MAX_WORD = 0xffff;

// pool 은 10.78 SidPool, frames 는 188칸 FrameCodeTable 표(참조), hooks = { setFrame(sid, 프레임, flags), reserveShared()→bool, addShared(sid, 사건, payload) } 이며 모두 필요하다. (2026-11-08 WB04-E70)
export class RawPriestRitualActions {
    // 현재 부모 raw 타입을 읽는 풀, 현재 타입별 첫 프레임 표, 같은 월드의 실제 frame setter 와 공유 예약이다.
    #pool;
    #frames;
    #hooks;

    // 표 폭과 세 필수 경계를 첫 raw 접근 전에 확인한다. (2026-11-08 WB04-E70)
    constructor(pool, frames, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(frames) || frames.length !== TYPE_COUNT || ['setFrame', 'reserveShared', 'addShared'].some((name) => typeof hooks?.[name] !== 'function')) {
            throw new TypeError('사제 의식 프레임/예약 연결 누락');
        }
        this.#pool = pool;
        this.#frames = frames;
        this.#hooks = hooks;
    }

    // 연결 어댑터가 같은 raw 월드를 쓰는지 검사하는 풀이다. (2026-11-08 WB04-E70)
    get pool() {
        return this.#pool;
    }

    // 원본 슬롯 주소가 pool+stride 이상이고 pool+capacity*stride 미만인 번호이다. SID 0 은 자연 반환한다. (2026-11-08 WB04-E70)
    #valid(sid) {
        return sid > 0 && sid < this.#pool.capacity && sid <= MAX_WORD;
    }

    // 타입 + 0x158 은 K 첫 프레임이며 단계마다 DWORD 하나씩 증가한다. 기존 글자 구간 결과를 다시 만들지 않는다. (2026-11-08 WB04-E70)
    begin(sid, stage) {
        if (!this.#valid(sid)) return;
        if (stage > LAST_RITUAL_TABLE_STAGE) throw new RangeError('사제 의식 단계가 first-table 밖입니다');
        const type = this.#pool.slot(sid)[TYPE_OFFSET];
        if (type >= this.#frames.length) throw new RangeError('사제 의식 현재 raw 타입 범위');
        this.#hooks.setFrame(sid, this.#frames[type].run(LETTER_K + stage).first, 0);
    }

    // frame callback 뒤 상태가 달라도 원본은 처음 유효했던 번호의 예약을 계속 요청한다. (2026-11-08 WB04-E70)
    continue(sid, stage) {
        if (!this.#valid(sid)) return;
        this.begin(sid, stage);
        if (this.#hooks.reserveShared()) this.#hooks.addShared(sid, RITUAL_FOLLOW_EVENT, RITUAL_FOLLOW_PAYLOAD);
    }
}

// 기존 프레임/공유 예약 몸체를 정확한 현재 번호와 인자로 공급한다. frame 은 SquidFrame, host 는 SquidProcessHost 이며 예약 확보(reserveShared)는 호출자가 공급한다. (2026-11-08 WB04-E70)
export function makePriestRitualActionHooks(pool, frame, host, hooks) {
    if (frame.pool !== pool || host.pool !== pool) throw new TypeError('사제 의식 하위 SID 풀 불일치');
    return {
        ...hooks,
        setFrame: (sid, index, flags) => { frame.set(sid, index, flags); },
        addShared: (sid, event, payload) => { host.addSharedRegular(sid, event, payload); }
    };
}
