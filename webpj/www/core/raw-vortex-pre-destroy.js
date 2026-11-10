// 신전(Vortex)의 실제 가상 PreDestroy(004dca50)와 현재 Player/두 목록/지역 효과 순서를 옮긴 모듈이다(cpppj `o/RawVortexPreDestroy.cpp/.h`, 10.78 전용).
// 순서: extra 비트 0·3(9)이 없으면 신전 목록(temples)과 거주 목록(residences)에서 이 SID 의 모든 중복을 제거 → 현재 좌표의 섬 번호 → 섬이 없으면 진단(0x2ab)·섬의 현재 신전 SID 가 이 SID 가 아니면 진단(0x2ac)
//   (둘 다 정상 복귀하면 계속) → 섬의 신전 비움(clearIslandTemple) → 섬 소유자 0 → 효과 뒤 현재 좌표로 지역 소유 요청 → 추가 목록(outposts)을 현재 개수·항목으로 순회하며 이 신전과 같은 소유자인 항목의 지역 소유 요청(항목이 풀 밖이면 거부)
//   → 공통 Damageable PreDestroy(damageablePre) → 로딩 깊이 0 이고 extra&9 가 없으면 생산 갱신 → 현재 소유자(+34)가 있으면 그 Player 의 신전 DWORD(+0x7c)가 이 SID 일 때 0 으로 쓰고 읽은 값을 복사본(+0x80)에 쓰며,
//   두 번째 신전 DWORD(+0x84, 복사본 +0x88)도 같게 처리한다. 진입 extra 검사와 Damageable 이후 extra 검사는 서로 다른 현재 읽기이고, 소유자/좌표는 효과 뒤 위치마다 다시 읽는다.
// 같은 원본 신전/섬/지역/공통 Damageable 의 실제 소비자(훅)는 선택 위치에서만 요구하며 앞선 원문 쓰기는 보존한 채 거부한다. 훅 = { islandNumber(x, y), islandExists(섬), islandTemple(섬), clearIslandTemple(섬),
// islandOwner(섬, 소유자), regionOwnership(x, y), damageablePre(sid, flags), refreshProduction(), assertion(줄) } 이다. loading 은 현재 로딩 깊이(005c89b8)를 돌려주는 getter 이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E80: 신전 PreDestroy 이식으로 추가)

// 10.78 Player 한 칸 폭과 Player 수, 첫/둘째 신전 DWORD 위치(복사본은 +4)이다. (2026-11-08 WB04-E80)
const PLAYER_STRIDE = 0xb4;
const PLAYER_SLOTS = 9;
const TEMPLE_FIRST = 0x7c;
const TEMPLE_FIRST_COPY = 0x80;
const TEMPLE_SECOND = 0x84;
const TEMPLE_SECOND_COPY = 0x88;
// 슬롯의 소유자 BYTE(+34)·extra BYTE(+40)·좌표 float(+14/+18) 위치와 진단 줄 번호이다. (2026-11-08 WB04-E80)
const OWNER = 34;
const EXTRA = 40;
const X_AT = 14;
const Y_AT = 18;
const MISSING_ISLAND_LINE = 0x2ab;
const WRONG_TEMPLE_LINE = 0x2ac;

// 선택 위치의 미연결 효과는 앞선 원문 쓰기를 보존한 채 명시 거부한다. (2026-11-08 WB04-E80)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('신전 삭제의 실제 하위 소비자 미연결');
    return hook;
}

// 비정렬 Player/좌표의 실제 DWORD 만 읽고 쓴다(이웃 BYTE 보존). (2026-11-08 WB04-E80)
function read(bytes, offset) {
    return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
}
function put(bytes, offset, value) {
    // 원본 네 BYTE 의 이웃을 보존한다.
    for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 효과 직전의 현재 단정도 좌표를 읽는다(NaN payload 보존). (2026-11-08 WB04-E80)
const view = new DataView(new ArrayBuffer(4));
function float(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

export class RawVortexPreDestroy {
    // 삭제되는 신전과 outpost 가 공유하는 풀, 같은 Player 9기록, 두 등록 목록, 효과 뒤 현재 개수를 읽는 추가 목록, 로딩 깊이 getter, 하위 소비자이다.
    #pool;
    #players;
    #temples;
    #residences;
    #outposts;
    #loading;
    #hooks;

    // 같은 PATCH 풀/Player 9기록/로딩 getter 를 확인한다. (2026-11-08 WB04-E80)
    constructor(pool, players, temples, residences, outposts, loading, hooks) {
        if (pool?.edition !== 'patch1078' || !(players instanceof Uint8Array) || players.length !== PLAYER_SLOTS * PLAYER_STRIDE || typeof loading !== 'function') {
            throw new TypeError('신전 삭제의 실제 PATCH/Player 저장소 오류');
        }
        this.#pool = pool;
        this.#players = players;
        this.#temples = temples;
        this.#residences = residences;
        this.#outposts = outposts;
        this.#loading = loading;
        this.#hooks = hooks ?? {};
    }

    // 현재 raw 소유자의 해당 Player DWORD 주소이며 범위 밖은 호스트 오류이다. 소유자는 위치마다 다시 읽는다. (2026-11-08 WB04-E80)
    #field(sid, field) {
        const owner = this.#pool.slot(sid)[OWNER];
        if (owner >= PLAYER_SLOTS) throw new RangeError('신전 삭제의 현재 Player 범위 오류');
        return owner * PLAYER_STRIDE + field;
    }

    // 원본 목록 helper(0040ea00)와 같은 앞방향 안정 압축이며 사용하지 않는 물리 꼬리는 보존한다. (2026-11-08 WB04-E80)
    #remove(list, sid) {
        list.removeAll(sid);
    }

    // 004dca50 전체의 목록 제거→지역→공통 Damageable→현재 UI→현재 Player 네 필드 순서이다. (2026-11-08 WB04-E80)
    preDestroy(sid, flags) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        if (!(pool.slot(sid)[EXTRA] & 9)) {
            this.#remove(this.#temples, sid);
            this.#remove(this.#residences, sid);
            let raw = pool.slot(sid);
            const island = need(hooks.islandNumber)(float(raw, X_AT), float(raw, Y_AT));
            if (!need(hooks.islandExists)(island)) need(hooks.assertion)(MISSING_ISLAND_LINE);
            if (need(hooks.islandTemple)(island) !== sid) need(hooks.assertion)(WRONG_TEMPLE_LINE);
            need(hooks.clearIslandTemple)(island);
            need(hooks.islandOwner)(island, 0);
            raw = pool.slot(sid);
            need(hooks.regionOwnership)(float(raw, X_AT), float(raw, Y_AT));
            // outpost 목록 count 와 부모/후보 owner 를 효과 이후 현재 값으로 검사한다.
            //  원문 반복은 index 와 count 를 signed 로 비교한다(count 의 상위 비트가 서 있으면 반복하지 않는다). (2026-11-10 WB04-E260: cpppj 기준선 뒤 VortexPreDestroy 재반영)
            for (let index = 0; (index | 0) < (this.#outposts.readCount() | 0); index += 1) {
                const value = this.#outposts.storage()[index];
                if (value >= pool.capacity || value > 65535) throw new RangeError('신전 삭제의 outpost SID 범위 오류');
                if (pool.slot(value)[OWNER] === pool.slot(sid)[OWNER]) {
                    const outpost = pool.slot(value);
                    need(hooks.regionOwnership)(float(outpost, X_AT), float(outpost, Y_AT));
                }
            }
        }
        need(hooks.damageablePre)(sid, flags >>> 0);
        if (this.#loading() === 0 && !(pool.slot(sid)[EXTRA] & 9)) need(hooks.refreshProduction)();
        if (pool.slot(sid)[OWNER] !== 0) {
            const players = this.#players;
            if (read(players, this.#field(sid, TEMPLE_FIRST)) === sid) {
                put(players, this.#field(sid, TEMPLE_FIRST), 0);
                const value = read(players, this.#field(sid, TEMPLE_FIRST));
                put(players, this.#field(sid, TEMPLE_FIRST_COPY), value);
            }
            if (read(players, this.#field(sid, TEMPLE_SECOND)) === sid) {
                put(players, this.#field(sid, TEMPLE_SECOND), 0);
                const value = read(players, this.#field(sid, TEMPLE_SECOND));
                put(players, this.#field(sid, TEMPLE_SECOND_COPY), value);
            }
        }
    }
}
