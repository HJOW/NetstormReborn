// 표면 WORD 지도의 한 칸(또는 float 좌표 한 점)이 가리키는 표면 객체의 소유자를 읽는 모듈이다(cpppj `o/RawRegionOwner.cpp/.h`, 10.78 전용, 원문 0046f710/0046f770).
// 256×256 표면 WORD 지도(hash0)에서 칸 번호의 SID 를 고르고, 그 SID 슬롯의 TYPE(+10)이 현재 섬(isle) 타입 전역과 같을 때만 소유자 BYTE(+34)를 돌려준다. 그 밖(범위 밖 칸·표면 없음·다른 TYPE)은 0 이다.
// 영역 번호/섬 목록을 거치지 않고, dead/void/free 필터도 더하지 않는다. 풀·지도·섬 타입은 복사하지 않고 매 조회마다 현재 값을 읽는다(섬 타입은 getter).
// float 좌표는 원본 순서대로 '+0.99999f → 64비트 절삭(하위 32비트) → float 저장 → 다시 절삭' 으로 칸을 고른 뒤 정수 조회를 실행한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E26: 영역 소유자 이식으로 추가)

// 원본 snap 편향(float 0.99999f)과 표면 지도 한 변의 칸 수, 소유자 BYTE 위치(10.78)이다. (2026-11-08 WB04-E26)
const SNAP_BIAS = Math.fround(0.99999);
const SIDE = 256;
const OWNER_OFFSET = 34;

// 원본 _ftol 의 signed 64비트 절삭과 하위 DWORD 감김(signed)이다. 유한하지 않거나 64비트 범위 밖이면 0 이다. (2026-11-08 WB04-E26)
function truncate(value) {
    if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 첫 합계를 float 로 줄이지 않고 절삭한 뒤, 정수를 float 로 저장하고 다시 절삭하는 원본 snap helper 이다. (2026-11-08 WB04-E26)
function snap(value) {
    return truncate(Math.fround(truncate(value + SNAP_BIAS)));
}

// 사용: new RegionOwner(pool, surfaces(Uint16Array 65536), () => 현재 섬 타입). 처리기보다 오래 살아야 하는 자료를 빌린다. (2026-11-08 WB04-E26)
export class RegionOwner {
    // 소유자 BYTE 를 현재 raw 슬롯에서 직접 읽는 동일 객체 풀, 같은 hash0 WORD 배열, 현재 섬 타입 getter 이다.
    #pool;
    #surfaces;
    #terrainType;

    // 표면 지도 크기가 65536 이 아니거나 getter 가 없으면 거부한다.
    constructor(pool, surfaces, terrainType) {
        if (!surfaces || surfaces.length !== SIDE * SIDE) throw new TypeError('영역 소유자 실제 표면 WORD 지도 크기 오류');
        if (typeof terrainType !== 'function') throw new TypeError('영역 소유자 현재 섬 타입 getter 누락');
        this.#pool = pool;
        this.#surfaces = surfaces;
        this.#terrainType = terrainType;
    }

    // 같은 실제 월드와 동일 풀인지 연결 시 확인하는 풀이다.
    get pool() { return this.#pool; }

    // 정수 칸 (x, y) 의 소유자이다. 256×256 밖이거나 표면이 없거나 현재 섬 타입이 아니면 0, 슬롯 번호가 풀 용량 이상이면 RangeError 이다.
    atCell(x, y) {
        if (x < 0 || y < 0 || x >= SIDE || y >= SIDE) return 0;
        const sid = this.#surfaces[y * SIDE + x];
        if (!sid) return 0;
        if (sid >= this.#pool.capacity) throw new RangeError('영역 소유자 현재 표면 SID 범위 오류');
        const raw = this.#pool.slot(sid);
        return raw[10] === (this.#terrainType() >>> 0) ? raw[OWNER_OFFSET] : 0;
    }

    // float 좌표 (x, y) 를 원본 helper 와 같은 독립 순서로 snap 한 칸의 소유자이다.
    atPoint(x, y) {
        const cellX = snap(x);
        const cellY = snap(y);
        return this.atCell(cellX, cellY);
    }
}
