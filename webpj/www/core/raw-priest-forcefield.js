// 사제 좌표 한 점의 보호막(forcefield)을 실제 일반 finder 의 타입/소유자 순회로 조회하는 모듈이다(cpppj `o/RawPriestForcefield.cpp/.h`, 10.78 전용: 004918e0).
// 사제 좌표를 0 방향 절삭한 한 칸에서 일반 탐색 순서(단계→y→x→버킷 next)로 같은 타입(005412f4 의 현재 DWORD, BYTE 로 줄이지 않음)이고 같은 소유자 BYTE 인 첫 SID 를 돌려준다(없으면 0).
// 기본 true 필터와 flags 0 을 쓰며 후보의 free/dead/void 는 추가로 제외하지 않는다(매장 후보는 finder 가 제외). dead/void/추상/매장은 사제 자체의 조회 생략 조건이 아니다.
// 손상된 사제(TYPE/가상 표/free·contained 비트)와 유한 signed int 범위를 넘는 좌표는 임의 포인터/정수 감김으로 복제하지 않고 거부한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E63: 사제 보호막 조회 이식으로 추가)

import { RawSquidFinder } from './raw-squid-finder.js';
import { PRIEST_TYPE, PRIEST_VTABLE } from './raw-priest-owner.js';

// 슬롯의 좌표/상태/소유자 위치, 무효 상태 비트(free+contained), 타입 표 크기이다. (2026-11-08 WB04-E63)
const X_OFFSET = 14;
const Y_OFFSET = 18;
const STATE_OFFSET = 11;
const OWNER_OFFSET = 34;
const INVALID_STATE = 9;
const TYPE_COUNT = 188;

// 사제 포스필드 처리 중 조회하는 상태이다: 보호막 TYPE 번호(005412f4, 기본 167). 생성과 조회가 같은 현재 값을 공유한다. (2026-11-08 WB04-E63)
export class PriestForcefieldState {
    type = 167;
}

// raw DWORD 를 호스트 정렬과 무관하게 읽는다. (2026-11-08 WB04-E63)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// float→signed int 의 정상 범위에서는 처음 0 방향 절삭한 값이 유지된다. 비유한/범위 밖은 거부한다. (2026-11-08 WB04-E63)
function cell(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    const value = view.getFloat32(0, true);
    if (!Number.isFinite(value) || value < -2147483648 || value > 2147483647) throw new RangeError('사제 보호막 조회 좌표 절삭 범위 오류');
    return Math.trunc(value);
}

// pool 은 10.78 SidPool, hash 는 SquidHash, types 는 188칸 TYPE 표(footX·footY; 복사), state 는 PriestForcefieldState 이다. 참조 대상은 조회 객체보다 오래 살아야 한다. (2026-11-08 WB04-E63)
export class RawPriestForcefield {
    // 풀/공간 해시/복사한 타입 표/보호막 TYPE 상태이다.
    #pool;
    #hash;
    #types;
    #state;

    // 판본별 타입 수를 확인한다. 공간 자료를 미복원 표시/Pop 몸체와 연결하지 않는다. (2026-11-08 WB04-E63)
    constructor(pool, hash, types, state) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT) throw new TypeError('사제 보호막 조회 타입 판본 오류');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types.map((record) => ({ ...record }));
        this.#state = state;
    }

    // 삭제 준비/보호막 생성 연결 시 다른 월드의 풀 혼합을 거부하기 위한 풀이다. (2026-11-08 WB04-E63)
    get pool() {
        return this.#pool;
    }

    // 조회 뒤 생성에 사용할 현재 DWORD 타입 전역을 다시 읽는다. (2026-11-08 WB04-E63)
    type() {
        return this.#state.type;
    }

    // 004918e0: 좌표를 0 방향 절삭하고 같은 타입·owner BYTE 의 첫 SID 를 돌려준다(없으면 0). 매 조회마다 독립 커서를 만든다. (2026-11-08 WB04-E63)
    find(priest) {
        if (priest < 5 || priest >= this.#pool.capacity) throw new RangeError('사제 보호막 조회 SID 오류');
        const raw = this.#pool.slot(priest);
        if (raw[10] !== PRIEST_TYPE || readDword(raw, 0) !== PRIEST_VTABLE || (raw[STATE_OFFSET] & INVALID_STATE)) throw new TypeError('사제 보호막 조회 raw 상태 오류');
        const x = cell(readDword(raw, X_OFFSET));
        const y = cell(readDword(raw, Y_OFFSET));
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 원본 기본 true 필터와 flags=0 을 사용한다. 후보 state/free/dead/void 를 추가로 제외하지 않는다.
        for (let current = finder.begin({ left: x, top: y, right: x, bottom: y }); current !== 0; current = finder.next()) {
            const candidate = this.#pool.slot(current);
            if (candidate[10] === this.#state.type && candidate[OWNER_OFFSET] === this.#pool.slot(priest)[OWNER_OFFSET]) return current;
        }
        return 0;
    }
}

// 같은 풀의 findForcefield 만 실제 조회로 바꾼다. 나머지 삭제/Carrier 경계는 호출자가 공급한다. (2026-11-08 WB04-E63)
export function makePriestForcefieldHooks(pool, lookup, hooks) {
    if (lookup.pool !== pool) throw new TypeError('사제 보호막 조회/삭제 준비 풀이 다릅니다');
    return { ...hooks, findForcefield: (sid) => lookup.find(sid) };
}
