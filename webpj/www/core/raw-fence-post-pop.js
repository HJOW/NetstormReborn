// 10.78 썬 바리케이드의 최초 후처리(00457560)를 실제 이웃/연결선/Regular 소비자와 조합하는 모듈이다(cpppj `o/RawFencePostPop.cpp/.h`, 10.78 전용).
// 순서(flags 비트 1 일 때만): 현재 TYPE 의 테마(+98) 번호가 3 이면 사건 0 예약→방향 0/2/4/6 의 최근접 펜스를 네 방향 모두 조회(local_1c 스냅샷)→현재 소유자 BYTE(+34)를 DWORD(+42)로 저장→
//   (북·남) 쌍과 (동·서) 쌍이 모두 있으면 각 이웃에 사건 3 예약 두 번 뒤 현재 두 끝점 좌표로 기존 연결선 삭제(mode 0) 요청→연결 수 조회(004573d0)를 DWORD(+46)에 저장→자신에 사건 2 예약.
//   모든 flags 에서 마지막에 Damageable 후처리(0044bf80)로 끝난다. 각 예약(new(0x28))은 독립 성공 여부를 보고 실패한 것만 생략한다. 효과가 바꾼 바이트(좌표·소유자)는 매번 다시 읽는다.
// 미연결 효과는 선택 위치에서 거부하며 앞선 쓰기와 예약을 보존한다. 실제 가상 진입은 명시 validate 의 사전 계약을 끼우지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E60: 썬 바리케이드 후처리 이식으로 추가)

// 썬 바리케이드 생성자(00457e80)의 실제 가상 표, 이웃 방향 네 개(북·동·남·서의 짝수 번호), 예약 지연 0.1 의 float32 비트, 테마 번호 3 이다. (2026-11-08 WB04-E60)
export const FENCE_VTABLE = 0x00509470;
const DIRECTIONS = Object.freeze([0, 2, 4, 6]);
const REGULAR_DELAY_BITS = 0x3dcccccd;
const SUN_THEME = 3;
const OWNER_OFFSET = 34;
const OWNER_COPY_OFFSET = 42;
const CONNECTION_OFFSET = 46;

// 선택된 하위 소비자가 없으면 그 위치에서 거부한다. (2026-11-08 WB04-E60)
function need(device) {
    if (typeof device !== 'function') throw new Error('썬 바리케이드 후처리의 실제 하위 소비자 미연결');
    return device;
}

// 비정렬 DWORD 를 리틀엔디언으로 읽는다. (2026-11-08 WB04-E60)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 원본이 쓴 DWORD 폭만 바꾸고 이웃 필드와 padding 을 보존한다. (2026-11-08 WB04-E60)
function writeDword(raw, offset, value) {
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 비트 패턴을 단정도 float 로 해석한다. (2026-11-08 WB04-E60)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// pool 은 10.78 SidPool, hooks = { theme(sid) → 번호, neighbor(sid, 방향) → 이웃 SID(0 은 없음), reserveRegular() → bool, addRegular(sid, 사건, 지연),
//   clearLine([x1, y1, x2, y2]), connections(sid) → 개수, validateBase(sid, flags, projected), damageable(sid, flags) }. 풀은 후처리 중 효과가 바꾼 바이트를 다시 읽는다. (2026-11-08 WB04-E60)
export class RawFencePostPop {
    // 현재 원본 50바이트 슬롯을 소유한 실제 월드 풀과 조회/예약/연결선/공통 후처리 소비자이다.
    #pool;
    #hooks;

    // CD 의 다른 펜스 동작을 10.78 로 간주하지 않는다. (2026-11-08 WB04-E60)
    constructor(pool, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('썬 바리케이드 후처리는 PATCH10.78만 복원했습니다');
        this.#pool = pool;
        this.#hooks = hooks ?? {};
    }

    // 상위 분배기가 같은 풀인지 확인할 때 쓰는 풀이다. (2026-11-08 WB04-E60)
    get pool() {
        return this.#pool;
    }

    // 실제 가상 표(00509470)만 분배하며 타입 번호만 같은 다른 몸체를 허용하지 않는다. (2026-11-08 WB04-E60)
    handles(sid) {
        return readDword(this.#pool.slot(sid), 0) === FENCE_VTABLE;
    }

    // 명시 사전 검사이다: projected 위치를 현재 공통 후처리의 검사에 그대로 전달한다. (2026-11-08 WB04-E60)
    validate(sid, flags, projected = null) {
        if (!this.handles(sid)) throw new Error('썬 바리케이드 실제 가상 표 불일치');
        if (this.#hooks.validateBase) this.#hooks.validateBase(sid, flags, projected);
    }

    // 매 할당의 독립 실패는 해당 생성자만 생략한다. 지연은 float 0.1 의 비트를 유지한다. (2026-11-08 WB04-E60)
    #regular(sid, event) {
        if (need(this.#hooks.reserveRegular)()) need(this.#hooks.addRegular)(sid, event, floatOf(REGULAR_DELAY_BITS));
    }

    // 확보/예약 효과가 변경한 두 슬롯의 현재 좌표를 네 float 로 복사해 연결선 삭제를 요청한다. (2026-11-08 WB04-E60)
    #clearPair(first, second) {
        const a = this.#pool.slot(first);
        const b = this.#pool.slot(second);
        need(this.#hooks.clearLine)([floatOf(readDword(a, 14)), floatOf(readDword(a, 18)), floatOf(readDword(b, 14)), floatOf(readDword(b, 18))]);
    }

    // flags 비트 1 에서만 최초 사건과 연결 수를 준비하고 모든 flags 에서 같은 Damageable 로 끝난다. (2026-11-08 WB04-E60)
    postPop(sid, flags) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        if (flags & 1) {
            if (need(hooks.theme)(sid) === SUN_THEME) this.#regular(sid, 0);
            // 원문 local_1c 는 네 방향을 모두 조회한 뒤 두 쌍에서 재사용한다.
            const neighbors = DIRECTIONS.map((direction) => need(hooks.neighbor)(sid, direction));
            writeDword(pool.physical(sid), OWNER_COPY_OFFSET, pool.slot(sid)[OWNER_OFFSET]);
            if (neighbors[0] && neighbors[2]) {
                this.#regular(neighbors[0], 3);
                this.#regular(neighbors[2], 3);
                this.#clearPair(neighbors[0], neighbors[2]);
            }
            if (neighbors[1] && neighbors[3]) {
                this.#regular(neighbors[1], 3);
                this.#regular(neighbors[3], 3);
                this.#clearPair(neighbors[1], neighbors[3]);
            }
            // 조회의 외부 효과 뒤 반환값만 원본 +0x2e 에 기록한다.
            const count = need(hooks.connections)(sid);
            writeDword(pool.physical(sid), CONNECTION_OFFSET, count >>> 0);
            this.#regular(sid, 2);
        }
        need(hooks.damageable)(sid, flags);
    }
}
