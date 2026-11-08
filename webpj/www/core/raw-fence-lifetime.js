// 썬 바리케이드의 연결 정리(4577f0)·이웃 갱신(4578f0)·가상 preDestroy(457990)를 같은 SID 풀에 적용해 옮긴 모듈이다(cpppj `o/RawFenceLifetime.cpp/.h`, 10.78 전용).
//  - clear(부모): extra 의 비트 9 가 켜져 있으면 끝. 아니면 동맹 이웃을 방향 0/2/4/6 순서로 찾아 두 끝점 표식을 지우고 효과 뒤의 현재 좌표/소유자로 연결선을 지운다.
//  - refresh(부모): 동맹 이웃 네 개를 먼저 모두 저장 → 자기 정리 → 자기가 dead 가 아니면 Unpop(0) → 저장한 이웃마다 현재 dead 가 아니고 현재 타입이 부모와 같으면 정리 후 재생성 → 자기가 dead 가 아니면 Repop(0x40).
//  - preDestroy(부모, flags): refresh → 비동맹 제한 없는 일반 이웃 네 개를 저장 → 첫 두 칸에만 예약(new 0x28) 성공 시 이웃 부모에 event 2/payload 0.1 Regular 를 부착 → Damageable 전처리를 같은 flags 로 부른다.
// 이웃 유효성/dead/타입과 좌표/소유자는 효과 뒤 현재 raw 를 다시 읽는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E74: 썬 바리케이드 수명 이식으로 추가)

// 슬롯의 TYPE/상태/소유자/extra/좌표 위치, 상태 dead 비트, extra 의 정리 생략 비트, 이웃 방향 수, 지연 Regular 를 붙이는 이웃 칸 수와 사건/payload, Repop flags 이다. (2026-11-08 WB04-E74)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const OWNER_OFFSET = 34;
const EXTRA_OFFSET = 40;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const DEAD = 2;
const SKIP_EXTRA = 9;
const DIRECTIONS = [0, 2, 4, 6];
const REGULAR_NEIGHBORS = 2;
const REGULAR_EVENT = 2;
const REGULAR_PAYLOAD = Math.fround(0.10000000149011612);
const REPOP_FLAGS = 0x40;

// 실제 선택된 효과가 빠졌을 때 빈 처리로 넘어가지 않는다. (2026-11-08 WB04-E74)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('실제 펜스 수명 효과 미연결');
    return hook;
}

// 비정렬 float 좌표의 비트를 그대로 읽는다. (2026-11-08 WB04-E74)
function coordinate(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    // 네 바이트를 리틀엔디언으로 옮긴다.
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// pool 은 10.78 SidPool, hooks = { neighbor(sid, 방향, 동맹)→SID, clearMarkers(sid, 이웃), clearLine([x1, y1, x2, y2], 소유자1, 소유자2, sid, 이웃), unpop(sid, flags), repop(sid, flags), rebuild(sid),
//   reserveRegular()→bool, addRegular(부모, 사건, payload), damageable(sid, flags) } 이며 도달한 것만 필요하다. (2026-11-08 WB04-E74)
export class RawFenceLifetime {
    // 삭제/재생성이 변경하는 단일 raw 풀, 계산을 대신하지 않는 실제 월드 효과이다.
    #pool;
    #hooks;

    // 다른 판본의 raw 오프셋을 적용하지 않는다. (2026-11-08 WB04-E74)
    constructor(pool, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('펜스 수명10.78 풀 계약 오류');
        this.#pool = pool;
        this.#hooks = hooks ?? {};
    }

    // 다른 월드의 객체 효과를 혼용하지 않게 하는 풀이다. (2026-11-08 WB04-E74)
    get pool() {
        return this.#pool;
    }

    // Refresh 의 포인터 유효성 검사에서 SID 1 도 허용하던 원본 범위를 유지한다(0 과 용량 이상은 거부). (2026-11-08 WB04-E74)
    #raw(sid) {
        if (!sid || sid >= this.#pool.capacity) throw new RangeError('펜스 수명 SID 범위 오류');
        return this.#pool.slot(sid);
    }

    // 앞선 표식 삭제/재생성이 좌표를 바꿀 수 있으므로 호출마다 현재 슬롯을 읽는다. (2026-11-08 WB04-E74)
    #endpoints(first, second) {
        const a = this.#raw(first);
        const b = this.#raw(second);
        return [coordinate(a, X_OFFSET), coordinate(a, Y_OFFSET), coordinate(b, X_OFFSET), coordinate(b, Y_OFFSET)];
    }

    // 최초 extra 검사는 이 함수 한 번의 진입에서만 수행한다. (2026-11-08 WB04-E74)
    clear(parent) {
        const hooks = this.#hooks;
        if (this.#raw(parent)[EXTRA_OFFSET] & SKIP_EXTRA) return;
        // 현재 동맹 이웃을 0/2/4/6 순서로 찾고 효과 후 현재 인수를 다시 읽는다.
        for (const direction of DIRECTIONS) {
            const other = need(hooks.neighbor)(parent, direction, true);
            if (!other) continue;
            need(hooks.clearMarkers)(parent, other);
            const points = this.#endpoints(parent, other);
            const a = this.#raw(parent);
            const b = this.#raw(other);
            need(hooks.clearLine)(points, a[OWNER_OFFSET], b[OWNER_OFFSET], parent, other);
        }
    }

    // 네 이웃 스냅샷은 자기 Unpop 전에 만들고 각 이웃의 유효성/dead/타입은 처리 시 다시 읽는다. (2026-11-08 WB04-E74)
    refresh(parent) {
        const hooks = this.#hooks;
        this.#raw(parent);
        const neighbors = [0, 0, 0, 0];
        // 원본은 네 방향을 모두 저장한 후에야 정리 효과를 시작한다.
        for (const direction of DIRECTIONS) neighbors[direction / 2] = need(hooks.neighbor)(parent, direction, true);
        this.clear(parent);
        if (!(this.#raw(parent)[STATE_OFFSET] & DEAD)) need(hooks.unpop)(parent, 0);
        // 재생성이 다음 이웃 또는 부모의 타입/상태를 바꾸면 원본처럼 다음 처리에 반영한다.
        for (const other of neighbors) {
            if (!other || other >= this.#pool.capacity) continue;
            const raw = this.#raw(other);
            if (!(raw[STATE_OFFSET] & DEAD) && raw[TYPE_OFFSET] === this.#raw(parent)[TYPE_OFFSET]) {
                this.clear(other);
                need(hooks.rebuild)(other);
            }
        }
        if (!(this.#raw(parent)[STATE_OFFSET] & DEAD)) need(hooks.repop)(parent, REPOP_FLAGS);
    }

    // 마지막 일반 이웃은 네 개를 읽되 원본은 첫 두 칸에만 지연 Regular 를 붙인다. (2026-11-08 WB04-E74)
    preDestroy(parent, flags) {
        const hooks = this.#hooks;
        this.refresh(parent);
        const neighbors = [0, 0, 0, 0];
        // 실제 456220 의 비동맹 제한 없는 현재 이웃을 따로 저장한다.
        for (const direction of DIRECTIONS) neighbors[direction / 2] = need(hooks.neighbor)(parent, direction, false);
        // 이 원본의 2회 조건을 네 방향으로 고쳐 쓰지 않는다.
        for (let index = 0; index < REGULAR_NEIGHBORS; index += 1) {
            if (neighbors[index] && need(hooks.reserveRegular)()) need(hooks.addRegular)(neighbors[index], REGULAR_EVENT, REGULAR_PAYLOAD);
        }
        need(hooks.damageable)(parent, flags);
    }
}
