// Carrier 의 실제 가상 c8 인 4287d0(낙하 시작)과 복귀 위치 거의 올림 41d7a0 을 옮긴 모듈이다(cpppj `o/RawCarrierFallStart.cpp/.h`, 10.78 전용).
// 순서: 이미 5105 낙하 Regular 가 있으면 즉시 참 → 권한자는 현재 이동 프로세스 종류를 조회해 이동을 제거 → extra(+40)의 비트 4(선택)가 켜져 있으면 선택 해제 → 포획 사제 Form 을 찾아 있으면 부모 좌표를 거의 올림(0.99999)해
//   그 Form 의 이전 소유자(원래 소유자 WORD 하위 7비트)의 Player 기록 +0xa8/+0xac 에 복귀 좌표로 쓰고 권한자는 사제를 해방 → 마지막으로 5105 낙하 task 를 지연 0 으로 예약(실제 RawCarrierTask)하고 참을 반환한다.
// 각 권한 분기에서 현재 값을 다시 읽으며 큰 하위(이동 제거·선택 해제·내용물 검색·사제 해방)는 명시 경계이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E72: Carrier 낙하 시작 이식으로 추가)

// 낙하 task 번호, 슬롯의 TYPE/extra/원래 소유자 위치와 선택 비트, Player 기록 폭과 복귀 좌표 위치, 거의 올림 bias(float32 0.99999)이다. (2026-11-08 WB04-E72)
const FALL_TASK = 0x5105;
const TYPE_OFFSET = 10;
const EXTRA_OFFSET = 40;
const SELECTED_BIT = 4;
const ORIGINAL_OWNER_OFFSET = 12;
const OWNER_MASK = 0x7f;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const PLAYER_STRIDE = 0xb4;
const RETURN_X = 0xa8;
const RETURN_Y = 0xac;
const ALMOST_CEIL_BIAS = Math.fround(0.99999);

// 현재 raw 의 정렬되지 않은 DWORD 를 읽는다. (2026-11-08 WB04-E72)
function dwordAt(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 비트를 단정도 float 로 해석한다. (2026-11-08 WB04-E72)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    return view.getFloat32(0, true);
}

// Player 의 한 좌표를 원본 DWORD 폭으로 저장하며 인접 필드를 보존한다. (2026-11-08 WB04-E72)
function writeFloat(raw, offset, value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    // 낮은 바이트부터 쓴다.
    for (let index = 0; index < 4; index += 1) raw[offset + index] = view.getUint8(index);
}

// 41d7a0 은 0.99999F 이며 ftol64 하위 signed DWORD 를 다시 float 에 저장한다. 비유한/범위 밖은 0 이다. (2026-11-08 WB04-E72)
function almostCeil(value) {
    const adjusted = value + ALMOST_CEIL_BIAS;
    if (!Number.isFinite(adjusted) || adjusted < -9223372036854775808 || adjusted >= 9223372036854775808) return 0;
    return Math.fround(Number(BigInt.asIntN(32, BigInt(Math.trunc(adjusted)))));
}

// 선택된 호출이 필요해질 때만 미연결을 거부한다. (2026-11-08 WB04-E72)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('캐리어 낙하 시작 실제 하위 미연결');
    return hook;
}

// pool 은 10.78 SidPool, players 는 Player 기록 바이트(폭 0xb4 의 배수; 현재 9 기록도 허용), boss 는 현재 권한 getter, priestType 은 현재 사제 종류 getter,
//   hooks = { hasRegular(부모, 사건), movementType(타입), removeMovement(부모, 이동 종류), clearSelection(), findKind(부모, 종류)→SID, releasePriest(부모), task(부모, 사건, 지연) } 이다. (2026-11-08 WB04-E72)
export class RawCarrierFallStart {
    // 부모와 포획 사제 Form 의 풀, 이전 소유자의 복귀 좌표를 기록하는 Player 기록, 현재 권한/사제 종류 getter, 큰 하위 경계이다.
    #pool;
    #players;
    #boss;
    #priestType;
    #hooks;

    // 잘못된 이전 소유자는 쓰기 전에 거부하도록 기록 폭을 구성 때 확인한다. (2026-11-08 WB04-E72)
    constructor(pool, players, boss, priestType, hooks) {
        if (pool?.edition !== 'patch1078' || !(players instanceof Uint8Array) || players.length % PLAYER_STRIDE !== 0 || typeof boss !== 'function' || typeof priestType !== 'function') {
            throw new TypeError('캐리어 낙하 시작 PATCH Player 자료 오류');
        }
        this.#pool = pool;
        this.#players = players;
        this.#boss = boss;
        this.#priestType = priestType;
        this.#hooks = hooks ?? {};
    }

    // 다른 낙하/생성 모듈의 풀 혼합을 검사하는 풀이다. (2026-11-08 WB04-E72)
    get pool() {
        return this.#pool;
    }

    // 기존 낙하 중이면 즉시 참, 아니면 전체 시작 순서 뒤 참이다. (2026-11-08 WB04-E72)
    begin(parent) {
        const hooks = this.#hooks;
        if (need(hooks.hasRegular)(parent, FALL_TASK)) return true;
        if (this.#boss()) {
            const movement = need(hooks.movementType)(this.#pool.slot(parent)[TYPE_OFFSET]);
            need(hooks.removeMovement)(parent, movement);
        }
        if (this.#pool.slot(parent)[EXTRA_OFFSET] & SELECTED_BIT) need(hooks.clearSelection)();
        const form = need(hooks.findKind)(parent, this.#priestType() >>> 0);
        if (form) {
            const raw = this.#pool.slot(parent);
            const x = almostCeil(floatOf(dwordAt(raw, X_OFFSET)));
            const y = almostCeil(floatOf(dwordAt(raw, Y_OFFSET)));
            const at = (this.#pool.slot(form)[ORIGINAL_OWNER_OFFSET] & OWNER_MASK) * PLAYER_STRIDE;
            if (at > this.#players.length || this.#players.length - at < PLAYER_STRIDE) throw new RangeError('캐리어 사제 복귀 Player 범위 오류');
            writeFloat(this.#players, at + RETURN_X, x);
            writeFloat(this.#players, at + RETURN_Y, y);
            if (this.#boss()) need(hooks.releasePriest)(parent);
        }
        need(hooks.task)(parent, FALL_TASK, 0);
        return true;
    }
}
