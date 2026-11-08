// 원본 총(Gun) 공격 방향 getter(패치 004d4d00)를 현재 raw graph/frame 과 실제 프레임 문자에 연결해 옮긴 모듈이다(cpppj `o/RawGunMode.cpp/.h`, 10.78 전용).
// 순서: 현재 자산 TYPE 이 70 이상 표 안이어야 한다(아니면 거부) → 비astral 크로스보우(현재 windArcher 타입이고 extra 비트 8 이 없음)이면 현재 graph BYTE(+30) × 2 를 반환 → 그 밖에는 현재 프레임 DWORD(+36)로 코드표의
//   side 글자를 signed 로 읽어 `글자 × 2 - 152` 가 0..7 이면 그 값, 아니면 0 을 반환한다. 현재 graph/frame 을 항상 실제 슬롯에서 읽으며 타입별 기본 방향 0 으로 대신하지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E71: 총 공격 모드 이식으로 추가)

// 슬롯의 TYPE/graph/extra/프레임 위치, 첫 자산 타입, windArcher 의 astral 제외 비트, 방향 계산 상수와 허용 범위, 타입 표 크기이다. (2026-11-08 WB04-E71)
const TYPE_OFFSET = 10;
const GRAPH_OFFSET = 30;
const EXTRA_OFFSET = 40;
const FRAME_OFFSET = 36;
const FIRST_ASSET_TYPE = 70;
const ASTRAL_BIT = 8;
const SIDE_SCALE = 2;
const SIDE_BASE = 152;
const MAX_MODE = 7;
const TYPE_COUNT = 188;

// pool 은 10.78 SidPool, frames 는 188칸 FrameCodeTable 표(참조), windArcherType 은 원본의 mutable 타입 DWORD 를 매 조회 읽는 getter 이다. (2026-11-08 WB04-E71)
export class RawGunMode {
    // 현재 자산의 TYPE/extra/graph/frame 을 읽는 풀, 같은 TYPE 번호의 실제 프레임 코드, 현재 windArcher 타입 getter 이다.
    #pool;
    #frames;
    #windArcherType;

    // 판본의 실제 전체 프레임 표를 확인하고 사본을 만들지 않는다. (2026-11-08 WB04-E71)
    constructor(pool, frames, windArcherType) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(frames) || frames.length !== TYPE_COUNT || typeof windArcherType !== 'function') throw new TypeError('Gun mode 판본 프레임 표 불일치');
        this.#pool = pool;
        this.#frames = frames;
        this.#windArcherType = windArcherType;
    }

    // 다른 월드의 Gun 검색에 연결하지 않도록 같은 풀을 제공한다. (2026-11-08 WB04-E71)
    get pool() {
        return this.#pool;
    }

    // 비astral 크로스보우는 현재 graph BYTE × 2, 나머지는 signed 프레임 문자 × 2 - 152 의 0..7 만 반환한다. (2026-11-08 WB04-E71)
    get(sid) {
        const raw = this.#pool.slot(sid);
        const type = raw[TYPE_OFFSET];
        if (type < FIRST_ASSET_TYPE || type >= this.#frames.length) throw new RangeError('Gun mode 현재 자산 TYPE 오류');
        if (type === (this.#windArcherType() >>> 0) && !(raw[EXTRA_OFFSET] & ASTRAL_BIT)) return raw[GRAPH_OFFSET] * SIDE_SCALE;
        // 패치의 정렬되지 않은 DWORD 전체를 원본 리틀엔디언으로 읽는다.
        const frame = (raw[FRAME_OFFSET] | (raw[FRAME_OFFSET + 1] << 8) | (raw[FRAME_OFFSET + 2] << 16) | (raw[FRAME_OFFSET + 3] << 24)) >>> 0;
        const codes = this.#frames[type].codes;
        if (frame >= codes.length) throw new RangeError('Gun mode 현재 프레임 물리 범위 오류');
        // 원본 char 는 signed BYTE 다.
        const side = (codes[frame].side << 24) >> 24;
        const result = side * SIDE_SCALE - SIDE_BASE;
        return result < 0 || result > MAX_MODE ? 0 : result;
    }
}
