// Rune166 후처리의 304 예약·신전 조회·건설 시작과 공통 postPop 호출을 옮긴 모듈이다(cpppj `o/RawRunePostPop.cpp/.h`, 10.78 전용).
// 순서: 최초 flag(비트 1)에서 예약(new(0x28))이 성공하면 Host.AddRegular(Rune, 0x304, 0.01)을 등록한다. flags 비트 2 에서는 Rune 위치(float 좌표에 0.9999899864196777 을 더해 0 방향 절삭) ±1 칸의 일반 finder 로
//   현재 Dais TYPE 이고 현재 소유자(+34)가 Rune 과 같은 첫 후보(Dais)를 찾아, 그 Dais 위치의 첫 Altar(현재 Altar TYPE, genus & 6 에 따른 시작 단계)에서 건설 시간(7/10/25초)을 구해 기존 Construction.Start(Rune, 초)를 호출한다.
//   마지막으로 항상 공통 postPop 의 비가상 몸체(PostPopBase)를 같은 flags 로 한 번 실행한다.
// 현재 Rune/Dais/Altar TYPE 번호와 시간 억제 전역은 소비 위치마다 다시 읽는 getter 로 받고, 선택한 장치만 요구한다(없으면 앞선 효과를 보존한 채 거부). 10.78 만 옮겼다(CD 의 전수 장치 사전 계약은 제외). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E61: Rune 후처리 이식으로 추가)

import { RawSquidFinder } from './raw-squid-finder.js';

// Rune166 생성자가 기록하는 10.78 가상 표, 최초 Rune Regular 사건 번호, 예약 payload 0.01 과 finder 좌표 보정(0.9999899864196777)의 float32 값이다. (2026-11-08 WB04-E61)
export const RUNE_VTABLE = 0x005075d8;
export const RUNE_REGULAR_EVENT = 0x304;
const RUNE_PAYLOAD = Math.fround(0.01);
const FINDER_BIAS = Math.fround(0.9999899864196777);
const TYPE_OFFSET = 10;
const OWNER_OFFSET = 34;
const ORIGINAL_OWNER_OFFSET = 12;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const GENUS_SURFACE_MASK = 6;
const PLAYER_COUNT = 8;
const TYPE_COUNT = 188;

// 선택된 실제 하위가 없으면 그 위치에서 거부한다. (2026-11-08 WB04-E61)
function need(device) {
    if (typeof device !== 'function') throw new Error('Rune 후처리의 선택된 실제 하위 미연결');
    return device;
}

// 리틀엔디언 값을 지정 폭만 읽는다(ownerWORD 와 vtableDWORD 를 구별). (2026-11-08 WB04-E61)
function read(raw, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 단정도 float 로 해석한다. (2026-11-08 WB04-E61)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 유한 지도 좌표를 원본 CRT 처럼 0 방향으로 절삭한다. 범위 밖/비유한은 거부한다. (2026-11-08 WB04-E61)
function cell(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('Rune 후처리 절삭 좌표 범위');
    return Math.trunc(value);
}

// Rune 조회/건설시간 helper 가 효과 뒤에도 읽는 현재 원본 전역이다: 현재 Rune·Dais·Altar TYPE 번호, 시간 억제(00594fa4), Player 플래그(Player+50 의 bit1)이다.
//   shared* 는 실제 Client 의 현재 값을 돌려주는 getter 이며 있으면 독립 값 대신 매번 그것을 읽는다. (2026-11-08 WB04-E61)
export class RunePostPopState {
    runeType = 166;
    daisType = 165;
    altarType = 164;
    suppressDuration = false;
    playerFlags = new Array(PLAYER_COUNT + 1).fill(0);
    sharedRuneType = null;
    sharedDaisType = null;
    sharedAltarType = null;
    sharedSuppressedDuration = null;

    // 여러 의식 소비자가 원래 owner 의 같은 번호를 계속 읽는다. (2026-11-08 WB04-E61)
    readRuneType() { return this.sharedRuneType ? this.sharedRuneType() >>> 0 : this.runeType; }

    // 현재 Dais TYPE 번호이다. (2026-11-08 WB04-E61)
    readDaisType() { return this.sharedDaisType ? this.sharedDaisType() >>> 0 : this.daisType; }

    // 현재 Altar TYPE 번호이다. (2026-11-08 WB04-E61)
    readAltarType() { return this.sharedAltarType ? this.sharedAltarType() >>> 0 : this.altarType; }

    // 표시/소리/Player 갱신 콜백 뒤 현재 미션 상태를 다시 읽는 시간 억제이다. (2026-11-08 WB04-E61)
    readSuppressDuration() { return this.sharedSuppressedDuration ? Boolean(this.sharedSuppressedDuration()) : this.suppressDuration; }
}

// pool 은 10.78 SidPool, hash 는 SquidHash, types 는 188칸 TYPE 표(footX·footY·flags2), state 는 RunePostPopState, base 는 같은 풀의 SquidPostPop,
//   hooks = { reserveRegular() → bool, addRegular(sid, 사건, payload), startConstruction(sid, 초), suppressDuration?() → bool, playerFlags?(owner) → DWORD }. (2026-11-08 WB04-E61)
export class RawRunePostPop {
    // 풀/해시/현재 TYPE 표/상태/공통 몸체/효과 경계이다.
    #pool;
    #hash;
    #types;
    #state;
    #base;
    #hooks;

    // 같은 풀과 188칸 TYPE 표를 확인한다. (2026-11-08 WB04-E61)
    constructor(pool, hash, types, state, base, hooks) {
        if (pool?.edition !== 'patch1078' || base?.pool !== pool || !Array.isArray(types) || types.length !== TYPE_COUNT) throw new TypeError('Rune 후처리 실제몸체/판본 연결 누락');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#state = state;
        this.#base = base;
        this.#hooks = hooks ?? {};
    }

    // 실제 Pop/Host/Construction 어댑터의 같은 풀 연결을 확인하는 풀이다. (2026-11-08 WB04-E61)
    get pool() {
        return this.#pool;
    }

    // 10.78 가상 진입은 현재 TYPE 와 무관하게 실제 가상 표만 본다. free/void 는 지원 주소 판단과 구별한다. (2026-11-08 WB04-E61)
    handles(sid) {
        return read(this.#pool.slot(sid), 0) === RUNE_VTABLE;
    }

    // Pop 의 공간 쓰기 전에 Rune 타입/가상 표/공통 장부 조건을 검사한다(명시 호출). (2026-11-08 WB04-E61)
    validate(sid, flags, projected = null) {
        if (!this.handles(sid) || this.#pool.slot(sid)[TYPE_OFFSET] !== this.#state.readRuneType()) throw new Error('Rune 후처리 타입/가상표 오류');
        this.#base.validateBase(sid, flags, projected);
    }

    // 현재 raw 좌표를 효과 뒤에도 각 조회마다 다시 읽는다. (2026-11-08 WB04-E61)
    #position(sid) {
        const raw = this.#pool.slot(sid);
        return [floatOf(read(raw, X_OFFSET)), floatOf(read(raw, Y_OFFSET))];
    }

    // 소유자는 원래 owner WORD 가 아닌 현재 +34 BYTE 이며 조회할 때마다 읽는다. (2026-11-08 WB04-E61)
    #owner(sid) {
        return this.#pool.slot(sid)[OWNER_OFFSET];
    }

    // 반경 1 의 일반 finder 를 같은 해시의 현재 후보에 적용해 현재 Dais TYPE 이고 같은 소유자인 첫 후보를 돌려준다(없으면 0). free/dead/abstract 는 추가 필터가 아니다. (2026-11-08 WB04-E61)
    findOwnedDais(rune) {
        const point = this.#position(rune);
        const x = cell(point[0] + FINDER_BIAS);
        const y = cell(point[1] + FINDER_BIAS);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 후보를 원본 순서로 훑는다.
        for (let sid = finder.begin({ left: x - 1, top: y - 1, right: x + 1, bottom: y + 1 }); sid !== 0; sid = finder.next()) {
            if (this.#pool.slot(sid)[TYPE_OFFSET] === this.#state.readDaisType() && this.#owner(sid) === this.#owner(rune)) return sid;
        }
        return 0;
    }

    // 전달한 객체 기준점의 첫 Altar 를 같은 typed finder 에서 읽는다(현재 Altar TYPE 의 genus & 6 에 따른 시작 단계). Rune 시간/실제 Dais 의식이 공유한다. (2026-11-08 WB04-E61)
    altarAt(rune) {
        const type = this.#state.readAltarType();
        if (type >= this.#types.length) throw new RangeError('Rune 현재altar 타입범위');
        const point = this.#position(rune);
        const x = cell(point[0]);
        const y = cell(point[1]);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 원본 첫 타입 일치를 재사용한다.
        for (let sid = finder.begin({ left: x, top: y, right: x, bottom: y }, (this.#types[type].flags2 & GENUS_SURFACE_MASK) === 0 ? 1 : 0); sid !== 0; sid = finder.next()) {
            if (this.#pool.slot(sid)[TYPE_OFFSET] === type) return sid;
        }
        return 0;
    }

    // 10.78 의 char 7/10/25 를 같은 정수로 반환한다: Altar 가 없거나 원래 owner WORD 가 0 이거나 시간 억제면 10, 아니면 원래 owner 의 Player 플래그 bit1 이 있으면 25, 없으면 7. (2026-11-08 WB04-E61)
    constructionSeconds(rune) {
        const altar = this.altarAt(rune);
        if (!altar) return 10;
        const originalOwner = read(this.#pool.slot(altar), ORIGINAL_OWNER_OFFSET, 2);
        if (!originalOwner || (this.#hooks.suppressDuration ? need(this.#hooks.suppressDuration)() : this.#state.readSuppressDuration())) return 10;
        if (this.#hooks.playerFlags) return (need(this.#hooks.playerFlags)(originalOwner) & 1) ? 25 : 7;
        if (originalOwner >= this.#state.playerFlags.length) throw new RangeError('Rune altar 원래ownerWORD/player범위');
        return (this.#state.playerFlags[originalOwner] & 1) ? 25 : 7;
    }

    // 예약 확보가 실패해도 신전/건설/공통 후처리를 계속한다. 발견한 Dais 가 시간 helper 의 this 이다(Rune 위치로 대신 계산하지 않는다). (2026-11-08 WB04-E61)
    postPop(rune, flags) {
        const hooks = this.#hooks;
        if ((flags & 1) && need(hooks.reserveRegular)()) need(hooks.addRegular)(rune, RUNE_REGULAR_EVENT, RUNE_PAYLOAD);
        if (flags & 2) {
            const dais = this.findOwnedDais(rune);
            if (dais && dais < this.#pool.capacity) {
                const seconds = Math.fround(this.constructionSeconds(dais));
                need(hooks.startConstruction)(rune, seconds);
            }
        }
        this.#base.postPopBase(rune, flags);
    }
}

// 기존 local Regular 와 실제 건설 프로세스를 직접 재사용하는 훅 묶음을 만든다. host 는 { pool, addRegular(sid, 사건, payload) }, construction 은 { pool, start(sid, 초) } 형태이다. (2026-11-08 WB04-E61)
export function makeRunePostPopHooks(pool, host, construction, hooks) {
    if (pool !== host.pool || pool !== construction.pool) throw new TypeError('Rune Host/Construction SID 풀 불일치');
    return {
        ...hooks,
        addRegular: (sid, event, payload) => host.addRegular(sid, event, payload),
        startConstruction: (sid, seconds) => construction.start(sid, seconds)
    };
}
