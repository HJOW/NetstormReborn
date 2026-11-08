// 사제 postPop 전체 wrapper(004950f0)의 후반을 옮긴 모듈이다(cpppj `o/RawPriestPostPopTail.cpp/.h`, 10.78 전용).
// 순서: 목록/회복 접두(RawPriestPostPop.prefix) → Carrier 본체(00427720) → Carrier 뒤 현재 extra 의 abstract/buried 비트(9)가 있으면 끝 → 현재 이동 불가 조회(00427030, RawPriestState.immobile)가 거짓이면
//   현재 보호막을 조회/삭제(clearShield: 00491980) 하고 끝 → 이동 불가이고 낙하 재등록 flag(0x800)가 없으면 현재 좌표를 +0.9999 해 절삭한 칸의 spot & 6(지면)이 없을 때 현재 프레임의 방향(signed side − 'A')이 낙하 방향 9 가 아니면
//   낙하 요청(fall: 004941f0) → 마지막으로 보호막 확보(ensureShield: 00493d30). 낙하 효과로 extra/지면이 바뀌어도 원본이 선택한 이동 불가 분기의 보호막 요청을 유지한다.
// 장치는 도달한 것만 그 시점에 요구하고(PATCH), 풀/표 크기 오류는 첫 접두 효과 전에 거부한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E64: 사제 후반 후처리 이식으로 추가)

import { pathDirection } from './raw-path-animation.js';

// 슬롯의 좌표/프레임/TYPE/extra 위치, 낙하 재등록 flag, 지면 지지 비트, 낙하 중 방향 번호, 거의 올림 배율(float32 0.9999), 지도 크기, 타입 표 크기이다. (2026-11-08 WB04-E64)
const X_OFFSET = 14;
const Y_OFFSET = 18;
const FRAME_OFFSET = 36;
const TYPE_OFFSET = 10;
const EXTRA_OFFSET = 40;
const FALLING_POP = 0x800;
const GROUND_MASK = 6;
const FALLING_DIRECTION = 9;
const COORDINATE_BIAS = Math.fround(0.9999);
const WORLD_CELLS = 256;
const TYPE_COUNT = 188;
const SKIP_EXTRA = 9;

// 도달한 낙하/보호막 효과만 요구하고 거부한다. (2026-11-08 WB04-E64)
function need(device) {
    if (typeof device !== 'function') throw new Error('사제 postPop 후반 선택된 실제 하위 미연결');
    return device;
}

// 리틀엔디언 DWORD 를 읽는다. (2026-11-08 WB04-E64)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 단정도 float 로 해석한다. (2026-11-08 WB04-E64)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    return view.getFloat32(0, true);
}

// 원본 x87 의 float+float 합을 좁히지 않고 0 방향 절삭한다. 지도 밖/비유한 좌표는 접근 전에 거부한다. (2026-11-08 WB04-E64)
function cell(value) {
    const shifted = value + COORDINATE_BIAS;
    if (!Number.isFinite(value) || shifted < 0 || shifted >= WORLD_CELLS) throw new RangeError('사제 postPop 지면 좌표 오류');
    return Math.trunc(shifted);
}

// pool 은 10.78 SidPool, prefix 는 RawPriestPostPop, carrier 는 RawCarrierPostPop, state 는 RawPriestState(모두 같은 풀), frames 는 188칸 FrameCodeTable 표(복사), spots 는 65536칸 spot 지도,
//   hooks = { fall(sid), ensureShield(sid), clearShield(sid) } 이다. 참조 대상은 이 객체보다 오래 살아야 한다. (2026-11-08 WB04-E64)
export class RawPriestPostPopTail {
    // 객체 슬롯 풀, 접두/Carrier/상태 몸체, 복사한 프레임 표, spot 지도, 낙하/보호막 효과이다.
    #pool;
    #prefix;
    #carrier;
    #state;
    #frames;
    #spots;
    #hooks;

    // 풀/표의 연결 오류는 첫 목록/회복 효과 전에 거부한다. 효과 훅은 도달할 때만 요구한다. (2026-11-08 WB04-E64)
    constructor(pool, prefix, carrier, state, frames, spots, hooks) {
        if (pool?.edition !== 'patch1078' || prefix?.pool !== pool || carrier?.pool !== pool || state?.pool !== pool
            || !Array.isArray(frames) || frames.length !== TYPE_COUNT || !(spots instanceof Uint8Array) || spots.length !== WORLD_CELLS * WORLD_CELLS) {
            throw new TypeError('사제 postPop 후반 연결 오류');
        }
        this.#pool = pool;
        this.#prefix = prefix;
        this.#carrier = carrier;
        this.#state = state;
        this.#frames = [...frames];
        this.#spots = spots;
        this.#hooks = hooks ?? {};
    }

    // Pop/Activate 분배가 같은 raw 풀을 사용하는지 확인하는 풀이다. (2026-11-08 WB04-E64)
    get pool() {
        return this.#pool;
    }

    // 공간 변경 전에는 현재 위치의 이동 불가/지면을 읽지 않고 접두 계약만 확인한다. (2026-11-08 WB04-E64)
    validate(sid) {
        this.#prefix.validate(sid);
    }

    // 004950f0 전체 wrapper: 접두 → Carrier → 현재 extra → 이동 불가 조회 → (낙하 요청) → 보호막. (2026-11-08 WB04-E64)
    postPop(sid, flags) {
        const pool = this.#pool;
        this.#prefix.prefix(sid, flags);
        this.#carrier.postPop(sid, flags);
        if (pool.slot(sid)[EXTRA_OFFSET] & SKIP_EXTRA) return;
        if (!this.#state.immobile(sid)) {
            need(this.#hooks.clearShield)(sid);
            return;
        }
        if (!(flags & FALLING_POP)) {
            const raw = pool.slot(sid);
            const x = cell(floatOf(readDword(raw, X_OFFSET)));
            const y = cell(floatOf(readDword(raw, Y_OFFSET)));
            if (!(this.#spots[y * WORLD_CELLS + x] & GROUND_MASK)) {
                const frame = readDword(raw, FRAME_OFFSET) | 0;
                const table = this.#frames[raw[TYPE_OFFSET]];
                if (table === undefined) throw new RangeError('사제 postPop 프레임 표 물리 범위 오류');
                if (pathDirection(table, frame) !== FALLING_DIRECTION) need(this.#hooks.fall)(sid);
            }
        }
        need(this.#hooks.ensureShield)(sid);
    }
}

// 같은 풀의 실제 보호막 생성/조회에 연결하는 훅 묶음이다: 이동 가능한 현재 좌표의 조회 결과만 삭제(destroy, 가상 +0x10 flags 0)하고 확보는 기존 보호막 생성을 그대로 잇는다. fall 은 전체 낙하 경계이다. (2026-11-08 WB04-E64)
export function makePriestPostPopTailShieldHooks(pool, shield, lookup, destroy, fall) {
    if (shield.pool !== pool || lookup.pool !== pool || typeof destroy !== 'function' || typeof fall !== 'function') throw new TypeError('사제 postPop 보호막 연결 오류');
    return {
        fall,
        ensureShield: (sid) => shield.ensure(sid),
        clearShield: (sid) => {
            const found = lookup.find(sid);
            if (found > 0 && found < pool.capacity) destroy(found, 0);
        }
    };
}
