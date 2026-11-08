// Carrier 의 가상 +0xcc 지면 조회(00426fc0)를 옮기고 실제 사제 +0xc8 분배와 합성하는 모듈이다(cpppj `o/RawCarrierCheck.cpp/.h`, 10.78 전용).
// 순서: 현재 TYPE 의 genus 에 0x320000 비트가 있으면 즉시 거짓(사제의 실제 genus 0x210000 은 좌표/낙하를 읽지 않고 제외), 아니면 좌표에 0.9999 를 더해 절삭한 칸(y 먼저)의 spot & 6(지면)이 있으면 거짓,
//   그 밖에는 가상 +0xc8(walker 낙하)의 0 아님을 반환한다. 상태/HP/free/dead/void/extra 비트는 조회 조건이 아니다. 표는 참조하며 생성 뒤에도 현재 genus/spot 을 읽는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E65: Carrier 지면 조회 이식으로 추가)

// 슬롯의 TYPE/좌표 위치, 제외 genus, 지면 비트, 좌표 거의 올림 배율(float32 0.9999), 지도 크기, 타입 표 크기이다. (2026-11-08 WB04-E65)
const TYPE_OFFSET = 10;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const EXCLUDED_GENUS = 0x320000;
const GROUND = 6;
const COORDINATE_BIAS = Math.fround(0.9999);
const WORLD_CELLS = 256;
const TYPE_COUNT = 188;

// 정렬되지 않은 좌표를 원본 비트 그대로 읽는다. (2026-11-08 WB04-E65)
function readFloat(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 원본 x87 합을 float 으로 좁히지 않고 절삭한다. 범위 밖 spot 읽기는 진단한다. (2026-11-08 WB04-E65)
function cell(coordinate) {
    const value = Math.trunc(coordinate + COORDINATE_BIAS);
    if (!Number.isFinite(value) || value < 0 || value >= WORLD_CELLS) throw new RangeError('Carrier 지면 좌표 오류');
    return value;
}

// 타입/지면이 허용할 때만 walker 낙하를 요청하는 조회 객체이다. pool 은 10.78 SidPool, types 는 188칸 TYPE 표(참조), spots 는 65536칸 spot 지도, fallWalker(sid) → bool 는 가상 +0xc8 이다. (2026-11-08 WB04-E65)
export class RawCarrierCheck {
    // 풀, 현재 타입 표, spot 지도, 낙하 walker 이다.
    #pool;
    #types;
    #spots;
    #fallWalker;

    // 표 크기와 필수 가상 경계를 검증한다. 특정 자산 지원 여부는 낙하 분배기가 결정한다. (2026-11-08 WB04-E65)
    constructor(pool, types, spots, fallWalker) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT || !(spots instanceof Uint8Array) || spots.length !== WORLD_CELLS * WORLD_CELLS || typeof fallWalker !== 'function') {
            throw new TypeError('Carrier 지면 조회 연결 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#spots = spots;
        this.#fallWalker = fallWalker;
    }

    // 다른 풀 연결을 효과 발생 전에 거부할 때 쓰는 풀이다. (2026-11-08 WB04-E65)
    get pool() {
        return this.#pool;
    }

    // 원본은 free/dead/void/extra 비트를 조회 조건으로 추가하지 않는다. (2026-11-08 WB04-E65)
    check(sid) {
        const raw = this.#pool.slot(sid);
        if (raw[TYPE_OFFSET] >= this.#types.length) throw new RangeError('Carrier 지면 조회 타입 오류');
        if (this.#types[raw[TYPE_OFFSET]].flags2 & EXCLUDED_GENUS) return false;
        const y = cell(readFloat(raw, Y_OFFSET));
        const x = cell(readFloat(raw, X_OFFSET));
        if (this.#spots[y * WORLD_CELLS + x] & GROUND) return false;
        return Boolean(this.#fallWalker(sid));
    }
}

// postPop 의 +0xcc 만 실제 조회로 바꾸며 지면 소유/공통 장부 경계는 유지한다. postPop 은 원본처럼 +0xcc 반환값을 버리고 같은 flags 로 부모 처리를 계속한다. (2026-11-08 WB04-E65)
export function makeCarrierCheckPostPopHooks(pool, check, hooks) {
    if (check.pool !== pool) throw new TypeError('Carrier postPop/조회 풀 불일치');
    return { ...hooks, carrierCheck: (sid) => { check.check(sid); } };
}

// 사제 나선 생성의 genus 래퍼 다음 +0xcc 만 같은 실제 조회로 연결한다. (2026-11-08 WB04-E65)
export function makeCarrierCheckSpawnHooks(pool, check, hooks) {
    if (check.pool !== pool) throw new TypeError('사제 생성/Carrier 조회 풀 불일치');
    return { ...hooks, checkCarrier: (sid) => { check.check(sid); } };
}
