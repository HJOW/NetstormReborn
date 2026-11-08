// 오벨리스크(buried)의 실제 Regular Handle(004622f0)을 같은 풀/프레임/공유 난수에 연결해 옮긴 모듈이다(cpppj `o/RawBuriedRegular.cpp/.h`, 10.78 전용).
//  - handles(sid): 부모의 가상 표 기록값이 0x50a748(오벨리스크)이면 이 분배 대상이다. TYPE 번호만 같은 다른 부모를 분배하지 않는다.
//  - handle(sid, 사건, 개수, payload): 사건 0 만 애니메이션/지연을 바꾼다. 현재 TYPE 의 B 구간 수+1 로 signed 개수의 나머지를 구해, 0 이 아니면 B 구간 첫 프레임 + 나머지 − 1 을 프레임으로 지정하고 0.1초를 반환하고,
//    0 이면 A 구간 첫 프레임을 지정하고 게임 난수(6)+2 초(2..7)를 반환한다. 다른 사건은 payload 를 그대로 반환한다(사건 1 의 QA 주소 계산 반환값은 버려지며 추가 효과 없음).
//  - handler(fallback): 다른 부모의 사건은 기존 실제 처리기로 전달하며 없으면 거부한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E75: 오벨리스크 Regular 이식으로 추가)

// 오벨리스크 가상 표 기록값, 슬롯의 TYPE 위치, 프레임 구간 글자, 반환 지연, 난수 한도와 오프셋, 타입 표 크기이다. (2026-11-08 WB04-E75)
const BURIED_TABLE = 0x50a748;
const TYPE_OFFSET = 10;
const BASE_LETTER = 'A';
const WAIT_LETTER = 'B';
const STEP_DELAY = Math.fround(0.1);
const RANDOM_LIMIT = 6;
const RANDOM_OFFSET = 2;
const TYPE_COUNT = 188;
const INT_MIN = -2147483648;

// pool 은 10.78 SidPool, frames 는 188칸 FrameCodeTable 표(참조), setter 는 같은 풀의 SquidFrame, random 은 월드 공유 GameRandom 이다. (2026-11-08 WB04-E75)
export class RawBuriedRegular {
    // 현재 QA/TYPE/프레임을 읽고 쓰는 풀, 현재 TYPE 의 A/B 구간 표, 같은 풀의 프레임 지정기, 공유 난수이다.
    #pool;
    #frames;
    #setter;
    #random;

    // 다른 판본의 가상 표/프레임 배치를 PATCH 와 동일하다고 가정하지 않는다. (2026-11-08 WB04-E75)
    constructor(pool, frames, setter, random) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(frames) || frames.length !== TYPE_COUNT || setter?.pool !== pool) throw new TypeError('오벨리스크 Handle의 판본/TYPE/프레임 풀 불일치');
        this.#pool = pool;
        this.#frames = frames;
        this.#setter = setter;
        this.#random = random;
    }

    // 실제 0050a748 가상 표를 확인하며 호스트 주소로 실행하지 않는다. (2026-11-08 WB04-E75)
    handles(sid) {
        const raw = this.#pool.slot(sid);
        return ((raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0) === BURIED_TABLE;
    }

    // signed 나머지와 원본 프레임/지연을 적용한다. (2026-11-08 WB04-E75)
    handle(sid, event, count, payload) {
        if (!this.handles(sid)) throw new TypeError('오벨리스크 Handle 실제 가상 표 불일치');
        if (event !== 0) return payload;
        const type = this.#pool.slot(sid)[TYPE_OFFSET];
        if (type >= this.#frames.length) throw new RangeError('오벨리스크 Handle TYPE 범위 오류');
        const run = this.#frames[type].run(WAIT_LETTER);
        const divisor = ((run.count >>> 0) + 1) | 0;
        const signedCount = count | 0;
        if (!divisor || (signedCount === INT_MIN && divisor === -1)) throw new RangeError('오벨리스크 Handle 원본 idiv 범위 오류');
        const remainder = signedCount % divisor;
        if (remainder) {
            this.#setter.set(sid, (run.first + remainder - 1) | 0, 0);
            return STEP_DELAY;
        }
        this.#setter.set(sid, this.#frames[type].run(BASE_LETTER).first, 0);
        // 004558f0(2, 8)은 난수를 한 번 소비하며 8 은 포함하지 않는다.
        return this.#random.next(RANDOM_LIMIT) + RANDOM_OFFSET;
    }

    // 다른 부모의 사건은 기존 실제 처리기로 전달한다. 미등록 부모를 빈 성공으로 바꾸지 않는다. (2026-11-08 WB04-E75)
    handler(fallback = null) {
        return (sid, event, count, payload) => {
            if (this.handles(sid)) return this.handle(sid, event, count, payload);
            if (!fallback) throw new Error('오벨리스크 외 Regular 부모 미연결');
            return fallback(sid, event, count, payload);
        };
    }
}
