// 너겟 485230(가상 +5c)의 왕복 프레임·지면 검사·낙하 Regular 본체를 옮긴 모듈이다(cpppj `o/RawNuggetRegular.cpp/.h`, 10.78 전용).
//  - handles(sid): 현재 가상 표 기록값이 0x50dc30(너겟)이면 이 분배 대상이다(현재 TYPE/할당 상태를 새 선행 조건으로 쓰지 않음).
//  - handle(sid, 사건, 개수, payload): 개수 0 이면 payload 그대로(초기 지연). 사건 0/1 은 감소/증가 왕복 프레임(끝점이면 예약 확보 성공 때만 게임 난수 2..7 의 지연으로 후속 예약을 만들고 0, 아니면 프레임을 ±1 지정하고 증가일 때만 추가 갱신 후 0.1초),
//    사건 2 는 소멸(flags 0x100000) 후 -1, 사건 3 은 현재 좌표의 지면(spot & 6 없음)이 있으면 낙하(죽은 객체면 0, 개수 100 초과면 권한자만 삭제 후 -1, 개수 10 에서 +35 BYTE 를 20 으로, 가속 낙하 y 를 254 로 제한해 Unpop → Pop(flags 0x40) 후 0.05초),
//    그 밖은 마지막 Damageable 처리기에 넘긴다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E72: 너겟 Regular 이식으로 추가)

// 너겟 가상 표 기록값과 슬롯의 TYPE/상태/좌표/프레임/낙하 표식 위치, 지도 폭 바이트 수, 지면 확인 bias(float32 0.9999), 사건 번호, 낙하 상한 개수와 반환 지연이다. (2026-11-08 WB04-E72)
const NUGGET_VTABLE = 0x50dc30;
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const FRAME_OFFSET = 36;
const FALL_MARK_OFFSET = 35;
const TYPE_COUNT = 188;
const SPOT_COUNT = 65536;
const GROUND_BIAS = Math.fround(0.9999);
const GROUND = 6;
const FALL_EVENT = 3;
const DESTROY_EVENT = 2;
const FALL_LIMIT = 100;
const FALL_MARK_COUNT = 10;
const FALL_MARK = 20;
const SHRINK_DELAY = Math.fround(0.1);
const FALL_DELAY = Math.fround(0.05);
const FALL_FLOOR = 254.0;
const DESTROY_FLAGS = 0x100000;
const POP_FLAGS = 0x40;

// 정렬과 무관하게 원본 DWORD 를 리틀엔디언으로 읽는다. (2026-11-08 WB04-E72)
function dwordAt(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 비트를 단정도 float 로 해석한다. (2026-11-08 WB04-E72)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    return view.getFloat32(0, true);
}

// 선택된 하위만 요구하며 개수 0/죽은 객체의 조기 반환에는 효과가 필요 없다. (2026-11-08 WB04-E72)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('너겟 실제 Regular 하위 몸체 미연결');
    return hook;
}

// 실제 _ftol64 의 정수 무효값도 하위 DWORD 0 으로 관찰한다. 중간 합을 float 로 저장하지 않는다. (2026-11-08 WB04-E72)
function cell(value) {
    const adjusted = value + GROUND_BIAS;
    if (!Number.isFinite(adjusted) || adjusted < -9223372036854775808 || adjusted >= 9223372036854775808) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(adjusted))));
}

// pool 은 10.78 SidPool, aFrameCounts 는 188칸 TYPE 별 A 구간 프레임 수(signed), spots 는 65536칸 지도, boss 는 현재 권한 getter, hooks = { setFrame(sid, 프레임, flags), refresh(sid), allocate()→bool, randomInteger(낮음, 높음)→수,
//   construct(sid, 사건, payload), destroy(sid, flags), unpop(sid, flags), pop(sid, x, y, flags), damageable(Regular 처리기 (sid, 사건, 개수, payload)→수) } 이다. (2026-11-08 WB04-E72)
export class RawNuggetRegular {
    // 프레임/좌표/상태/가상 표의 단일 물리 풀, A 구간 프레임 수, 점유 지도, 현재 권한, 큰 하위 효과이다.
    #pool;
    #aFrameCounts;
    #spots;
    #boss;
    #hooks;

    // 판본/표 크기를 구성 때 확인한다. 하위 효과는 도달할 때만 요구한다. (2026-11-08 WB04-E72)
    constructor(pool, aFrameCounts, spots, boss, hooks) {
        if (pool?.edition !== 'patch1078' || !aFrameCounts || aFrameCounts.length !== TYPE_COUNT || !(spots instanceof Uint8Array) || spots.length !== SPOT_COUNT || typeof boss !== 'function') {
            throw new TypeError('너겟 Regular 는 PATCH 188TYPE/65536spot 이 필요합니다');
        }
        this.#pool = pool;
        this.#aFrameCounts = aFrameCounts;
        this.#spots = spots;
        this.#boss = boss;
        this.#hooks = hooks ?? {};
    }

    // 다른 Session 의 상태와 실제 예약을 혼합하지 않게 하는 풀이다. (2026-11-08 WB04-E72)
    get pool() {
        return this.#pool;
    }

    // 현재 가상 표 0x50dc30 의 실제 가상 5c 를 분배한다. TYPE 는 각 몸체의 읽기 위치에서 사용한다. (2026-11-08 WB04-E72)
    handles(sid) {
        return dwordAt(this.#pool.slot(sid), 0) === NUGGET_VTABLE;
    }

    // 485fe0: 실제 ftol 하위 DWORD 와 감긴 지도 색인에서 spot & 6 이 없으면 참이다. 지도 밖 색인은 거부한다. (2026-11-08 WB04-E72)
    canFall(sid) {
        const raw = this.#pool.slot(sid);
        const y = cell(floatOf(dwordAt(raw, Y_OFFSET)));
        const x = cell(floatOf(dwordAt(raw, X_OFFSET)));
        const index = (((y << 8) >>> 0) + x) >>> 0;
        if (index >= this.#spots.length) throw new RangeError('너겟 지면 조회가 원본 지도 물리 범위를 벗어났습니다');
        return !(this.#spots[index] & GROUND);
    }

    // 4850a0/485170: 증가(grow)/감소/끝점 예약이다. 끝점에서는 예약 확보가 실패하면 난수를 소비하지 않는다. (2026-11-08 WB04-E72)
    #animate(sid, grow) {
        const hooks = this.#hooks;
        const frame = dwordAt(this.#pool.slot(sid), FRAME_OFFSET);
        let endpoint = frame === 0;
        if (grow) {
            const type = this.#pool.slot(sid)[TYPE_OFFSET];
            if (type >= this.#aFrameCounts.length) throw new RangeError('너겟 프레임 TYPE 범위 오류');
            endpoint = frame === (((this.#aFrameCounts[type] >>> 0) - 1) >>> 0);
        }
        if (endpoint) {
            if (need(hooks.allocate)()) {
                const delay = Math.fround(need(hooks.randomInteger)(2, 8));
                need(hooks.construct)(sid, grow ? 0 : 1, delay);
            }
            return 0;
        }
        const next = (dwordAt(this.#pool.slot(sid), FRAME_OFFSET) + (grow ? 1 : 0xffffffff)) >>> 0;
        need(hooks.setFrame)(sid, next | 0, 0);
        if (grow) need(hooks.refresh)(sid);
        return SHRINK_DELAY;
    }

    // 484ec0: 죽음 검사/가속 낙하/상한/권한 삭제이다. 반환은 다음 예약 지연(-1 은 종료)이다. (2026-11-08 WB04-E72)
    #fall(sid, count) {
        const hooks = this.#hooks;
        if (this.#pool.slot(sid)[STATE_OFFSET] & 2) return 0;
        if (count > FALL_LIMIT) {
            if (this.#boss()) need(hooks.destroy)(sid, 0);
            return -1;
        }
        if (count === FALL_MARK_COUNT) this.#pool.allocated(sid)[FALL_MARK_OFFSET] = FALL_MARK;
        const raw = this.#pool.slot(sid);
        const x = floatOf(dwordAt(raw, X_OFFSET));
        let y = Math.fround((count * 0.5 + 1.0) * 0.0625 + floatOf(dwordAt(raw, Y_OFFSET)));
        // 실제 unordered 분기는 NaN 을 보존한다.
        if (FALL_FLOOR < y) y = FALL_FLOOR;
        need(hooks.unpop)(sid, 0);
        need(hooks.pop)(sid, x, y, POP_FLAGS);
        return FALL_DELAY;
    }

    // 개수 0 의 초기 지연 뒤 실제 0/1/2/3 사건을 처리한다. 그 밖은 마지막 Damageable 처리기이다. (2026-11-08 WB04-E72)
    handle(sid, event, count, payload) {
        if (!count) return payload;
        if (event === 0) return this.#animate(sid, false);
        if (event === 1) return this.#animate(sid, true);
        if (event === DESTROY_EVENT) {
            need(this.#hooks.destroy)(sid, DESTROY_FLAGS);
            return -1;
        }
        if (event === FALL_EVENT && this.canFall(sid)) return this.#fall(sid, count | 0);
        return need(this.#hooks.damageable)(sid, event, count, payload);
    }
}
