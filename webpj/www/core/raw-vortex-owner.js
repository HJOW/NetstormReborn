// 신전(Vortex)의 실제 가상 소유자 변경(004dc4e0, 가상 +0x74)을 옮긴 모듈이다(cpppj `o/RawVortexOwner.cpp/.h`, 10.78 전용). 같은 Player 기록·섬·지역·공통 Owner 에 연결한다.
// 순서: 이전 소유자 해제 → 섬/지역/패치 추가 목록 → 공통 Owner → 현재 소유자 신전 기록 → 생산 갱신 → revision 증가.
//  1) 외부 효과 전의 원본 초기 조건으로 이전 소유자(+34)와 extra(+40)를 읽는다. extra 비트 3(8)이 없고 이전 소유자가 있으며 그 Player 의 신전 DWORD(Player 한 칸 0xb4 의 +0x7c)가 이 SID 이면 신전 두 DWORD(+0x7c, 복사본 +0x80)를 0 으로 지운다.
//  2) 슬롯 상태 비트 2(4)가 없고 extra 비트 0·3(9)이 없으면: 현재 좌표의 섬 번호 조회 → 섬이 없으면 진단(missingIsland, 정상 복귀 시 계속) → 섬 소유자 변경 → 같은 좌표의 지역 소유(regionOwnership) 요청 →
//     추가 목록(outposts)을 매 반복의 현재 개수(signed)와 항목으로 순회하며(free/dead 필터 없음) 이 신전과 같은 소유자인 항목의 지역 소유 요청. 항목이 목록 범위나 풀 밖이면 거부한다.
//  3) 공통 Owner(base.set)를 실행하고 변경 후의 소유자(+34)로, extra&9 가 없고 그 Player 의 신전 DWORD 가 0 이면 두 DWORD 에 이 SID 를 쓴다.
//  4) 생산 갱신(refreshProduction)을 부르고 revision 을 1 증가한다(DWORD 감김).
// 가상 표는 신전 네 개(0x5196f0·0x519860·0x5197a8·0x519918)만 처리한다(타입 이름으로 다른 몸체를 대체하지 않음). 섬/지역/생산 효과는 명시 외부 경계(훅)이며 효과 뒤 현재 raw 소유자·좌표·목록을 다시 읽는다.
// 훅 = { islandNumber(x, y), islandExists(섬), setIslandOwner(섬, 소유자), regionOwnership(x, y), refreshProduction(), missingIsland(sid, 섬, 새 소유자) } 이고 revision 은 `{ value }` 보유 객체이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E79: 신전 소유자 이식으로 추가)

// 10.78 Player 한 칸 폭, 초기 신전 DWORD(+0x7c)와 그 복사본(+0x80) 위치, Player 번호 상한(0..8)이다. (2026-11-08 WB04-E79)
const PLAYER_STRIDE = 0xb4;
const TEMPLE_AT = 0x7c;
const PLAYER_LAST = 8;
// 슬롯의 상태 BYTE(+11)·소유자 BYTE(+34)·extra BYTE(+40)·좌표 float(+14/+18) 위치이다. (2026-11-08 WB04-E79)
const STATE = 11;
const OWNER = 34;
const EXTRA = 40;
const X_AT = 14;
const Y_AT = 18;
// 신전 네 개의 가상 표 주소이다. (2026-11-08 WB04-E79)
const VORTEX_TABLES = Object.freeze([0x5196f0, 0x519860, 0x5197a8, 0x519918]);

// 비정렬 little endian DWORD 를 읽고 쓴다(주변 바이트는 보존). (2026-11-08 WB04-E79)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}
function writeDword(raw, offset, value) {
    // 지정한 네 BYTE 만 낮은 비트부터 기록한다.
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 외부 효과 뒤의 현재 단정도 좌표를 읽는다(NaN payload 보존). (2026-11-08 WB04-E79)
const view = new DataView(new ArrayBuffer(4));
function position(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

export class RawVortexOwner {
    // 같은 풀, 공통 Owner(SquidOwner), 공유 Player 9기록, 별도 추가 목록, revision 보유 객체, 외부 효과 훅이다.
    #pool;
    #base;
    #players;
    #outposts;
    #revision;
    #hooks;

    // 같은 풀/실제 9기록/필수 효과를 최초 쓰기 전에 확인한다. revision 은 `{ value }` 이다. (2026-11-08 WB04-E79)
    constructor(pool, base, players, outposts, revision, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('신전 Owner 는 PATCH 전용입니다');
        const required = ['islandNumber', 'islandExists', 'setIslandOwner', 'regionOwnership', 'refreshProduction', 'missingIsland'];
        if (base?.pool !== pool || !(players instanceof Uint8Array) || players.length !== (PLAYER_LAST + 1) * PLAYER_STRIDE ||
            typeof revision !== 'object' || revision === null || required.some((name) => typeof hooks?.[name] !== 'function')) {
            throw new TypeError('신전 Owner의 실제 Pool/9기록/필수 효과 누락');
        }
        this.#pool = pool;
        this.#base = base;
        this.#players = players;
        this.#outposts = outposts;
        this.#revision = revision;
        this.#hooks = hooks;
    }

    // 다른 월드의 분배기를 연결하지 않도록 같은 풀과 9기록 저장소를 제공한다. (2026-11-08 WB04-E79)
    get pool() { return this.#pool; }
    get records() { return this.#players; }

    // 현재 원본 가상 표를 직접 조회한다. 타입 이름으로 다른 몸체를 대체하지 않는다. (2026-11-08 WB04-E79)
    handles(sid) {
        return VORTEX_TABLES.includes(readDword(this.#pool.slot(sid), 0));
    }

    // 초기 신전 DWORD(복사본이면 +4)의 현재 Player 저장 주소이다. (2026-11-08 WB04-E79)
    #templeField(owner, copy = false) {
        if (!Number.isInteger(owner) || owner < 0 || owner > PLAYER_LAST) throw new RangeError('신전 Owner 실제9 Player 범위 오류');
        return owner * PLAYER_STRIDE + TEMPLE_AT + (copy ? 4 : 0);
    }

    // 공통 Owner 를 마지막 지역 갱신 뒤에 실행하며 이전 소유자 기준의 패치 순회를 유지한다. (2026-11-08 WB04-E79)
    set(sid, owner) {
        if (!this.handles(sid)) throw new Error('미복원 신전 가상 Owner 표');
        const pool = this.#pool;
        const players = this.#players;
        const hooks = this.#hooks;
        // 외부 호출 전 원본 초기 조건이다.
        const previous = pool.slot(sid)[OWNER];
        const extra = pool.slot(sid)[EXTRA];
        if (!(extra & 8) && previous && readDword(players, this.#templeField(previous)) === sid) {
            writeDword(players, this.#templeField(previous), 0);
            writeDword(players, this.#templeField(previous, true), 0);
        }
        if (!(pool.slot(sid)[STATE] & 4) && !(extra & 9)) {
            // 현재 지면 지역이다.
            const island = hooks.islandNumber(position(pool.slot(sid), X_AT), position(pool.slot(sid), Y_AT));
            if (!hooks.islandExists(island)) hooks.missingIsland(sid, island, owner >>> 0);
            hooks.setIslandOwner(island, owner >>> 0);
            hooks.regionOwnership(position(pool.slot(sid), X_AT), position(pool.slot(sid), Y_AT));
            // 현재 count 와 각 SID·소유자·좌표를 효과 뒤에 다시 읽는다. free/dead 필터를 추가하지 않는다.
            for (let index = 0; index < (this.#outposts.readCount() | 0); index += 1) {
                const entries = this.#outposts.storage();
                if (index >= entries.length || entries[index] >= pool.capacity) throw new RangeError('신전 Owner 현재 추가 목록 범위 오류');
                const other = entries[index];
                if (pool.slot(other)[OWNER] === pool.slot(sid)[OWNER]) hooks.regionOwnership(position(pool.slot(other), X_AT), position(pool.slot(other), Y_AT));
            }
        }
        this.#base.set(sid, owner);
        // 공통 몸체 이후의 실제 소유자 BYTE 이다.
        const current = pool.slot(sid)[OWNER];
        if (!(pool.slot(sid)[EXTRA] & 9) && current && !readDword(players, this.#templeField(current))) {
            writeDword(players, this.#templeField(current), sid);
            writeDword(players, this.#templeField(current, true), sid);
        }
        hooks.refreshProduction();
        this.#revision.value = (this.#revision.value + 1) >>> 0;
    }
}
