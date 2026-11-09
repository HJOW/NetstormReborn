// 보호막(forcefield)의 최초 예약과 정기 Regular 처리를 옮긴 모듈이다(cpppj `o/RawForcefieldLifecycle.cpp/.h`, 10.78 전용: 후처리 접두 00448d20, Regular 처리기 Dais.cpp 0x479 진단 포함).
// 구성: ① RawForcefieldPostPop — 보호막 가상 표(0x00507770)의 공통 후처리 직전 접두. flags 비트 1 일 때만 사건 1(0.08f)·사건 2(0.01f)를 순서대로 독립 예약한다(예약 실패는 해당 사건만 생략).
//   ② RawForcefieldRegular — 사건 1 은 프레임을 (1, 0)만큼 진행하고 0.08f 를 돌려주며, 사건 2 는 같은 칸에 사제가 있으면 0.01f, 없으면 삭제(0) 뒤 -1 을 돌려준다.
//   다른 사건은 0 을 돌려주며 checking 이 비영이면 `.\Dais.cpp` 0x479 진단을 올린 뒤 정상 복귀한다. ③ makeForcefieldPostPopProcessHooks / makeForcefieldFrameHooks — 실제 Process 호스트와 프레임 진행기로 잇는 훅 묶음.
// 10.78 은 도달한 장치만 요구한다(객체 사전 검사를 접두·Regular 에 넣지 않는다). 효과가 실행 중인 처리기를 지워도 선택한 callable 의 사본으로 호출을 마치고 이후 슬롯/상태를 읽지 않는다.
// CD 10.72 의 구성 사전 계약(예약 훅 필수·객체 사전 검사)은 웹 범위(10.78) 밖이라 옮기지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E212: 보호막 수명 이식으로 추가)

import { RawSquidFinder } from './raw-squid-finder.js';
import { PRIEST_TYPE } from './raw-priest-owner.js';

// 보호막 TYPE 번호, 10.78 가상 표, 두 Regular 사건 번호이다. 주소는 실행하지 않는다. (2026-11-10 WB04-E212)
export const FORCEFIELD_TYPE = 167;
export const PATCH_FORCEFIELD_VTABLE = 0x00507770;
export const FORCEFIELD_FRAME_EVENT = 1;
export const FORCEFIELD_CHECK_EVENT = 2;

// 슬롯의 상태/타입/좌표 위치와 객체 거부 비트(free+contained)이다. (2026-11-10 WB04-E212)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const INVALID_STATE = 9;
const TYPE_COUNT = 188;

// 실제 PE 의 두 주기 상수(0x3da3d70a·0x3c23d70a)를 단정도로 보존한다. (2026-11-10 WB04-E212)
const FRAME_INTERVAL = Math.fround(0.08);
const CHECK_INTERVAL = Math.fround(0.01);

// float32 비트 변환 도구이다. (2026-11-10 WB04-E212)
const floatView = new DataView(new ArrayBuffer(4));

// 비정렬 raw DWORD 를 호스트 정렬과 무관하게 읽는다. (2026-11-10 WB04-E212)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// raw DWORD 를 단정도 float 로 해석한다. (2026-11-10 WB04-E212)
function floatAt(raw, offset) {
    floatView.setUint32(0, readDword(raw, offset), true);
    return floatView.getFloat32(0, true);
}

// 원본 float→int→float→int 의 정상 범위에서는 첫 절삭 값이 유지된다. 비유한/범위 밖은 거부한다. (2026-11-10 WB04-E212)
function cell(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value > 2147483647) throw new RangeError('보호막 사제 검사 좌표 범위 오류');
    return Math.trunc(value) | 0;
}

// 선택된 효과가 없으면 그 위치에서 거부하고, 있으면 호출 동안 쓸 사본을 돌려준다. (2026-11-10 WB04-E212)
function need(device) {
    if (typeof device !== 'function') throw new Error('보호막 선택 효과 미연결');
    return device;
}

// 실제 보호막 클래스만 명시 연결한다. 잘못된 SID·free/contained·TYPE·가상 표는 효과 전에 거부한다. 읽기 전용 검사이다. (2026-11-10 WB04-E212)
function validateObject(pool, sid) {
    const raw = pool.slot(sid);
    if (sid < 5 || (raw[STATE_OFFSET] & INVALID_STATE) || raw[TYPE_OFFSET] !== FORCEFIELD_TYPE || readDword(raw, 0) !== PATCH_FORCEFIELD_VTABLE) {
        throw new TypeError('보호막 타입/가상 표 불일치');
    }
}

// 보호막 후처리 접두이다. pool 은 10.78 SidPool, hooks = { reserveRegular() → boolean, addRegular(sid, 사건, 지연) } 이다(PATCH 는 비어 있어도 구성된다).
// 사용: const body = new RawForcefieldPostPop(pool, hooks); postPop.setBody('forcefield', body). (2026-11-10 WB04-E212)
export class RawForcefieldPostPop {
    // 객체 슬롯을 읽는 SID 풀과 예약 효과 묶음이다.
    #pool;
    #hooks;

    // 풀 판본을 확인한다. 효과는 도달한 위치에서만 확인한다(CD 의 구성 사전 계약은 웹 범위 밖). (2026-11-10 WB04-E212)
    constructor(pool, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('보호막 후처리는 10.78 SID 풀만 받습니다.');
        this.#pool = pool;
        this.#hooks = hooks ?? {};
    }

    // 다른 월드의 일반 후처리에 연결되지 않도록 구성 단계에서 비교하는 풀이다. (2026-11-10 WB04-E212)
    get pool() {
        return this.#pool;
    }

    // 공간 쓰기 전의 명시 사전 검사이다. 예약과 장부는 바꾸지 않는다. (2026-11-10 WB04-E212)
    validate(sid) {
        validateObject(this.#pool, sid);
    }

    // flags 비트 1 일 때만 사건 1(0.08f)·사건 2(0.01f)를 순서대로 예약한다. 첫 확보가 실패해도 두 번째 확보를 계속하며 객체 재검사는 넣지 않는다.
    // 공통 base 는 호출자가 이어 준다(원본 00448d20 의 공통 후처리 직전 접두). (2026-11-10 WB04-E212)
    prefix(sid, flags) {
        if (!(flags & 1)) return;
        if (need(this.#hooks.reserveRegular)()) need(this.#hooks.addRegular)(sid, FORCEFIELD_FRAME_EVENT, FRAME_INTERVAL);
        if (need(this.#hooks.reserveRegular)()) need(this.#hooks.addRegular)(sid, FORCEFIELD_CHECK_EVENT, CHECK_INTERVAL);
    }
}

// 보호막 Regular 가 읽는 현재 상태이다. priestType 은 독립 재생이 공급하는 사제 TYPE 이고, sharedPriestType 이 있으면 장기 전역 DWORD(005412d0)를 매 검사마다 다시 읽는다.
// checking 은 호출마다 현재 DWORD 를 읽는 getter 이다(독립 기본 0; 진단 callback 이 값을 바꿔도 다음 읽기가 그것을 본다). (2026-11-10 WB04-E212)
export class ForcefieldRegularState {
    // 독립 재생의 사제 TYPE 번호 이다.
    priestType = PRIEST_TYPE;
    // 현재 checking DWORD 를 돌려주는 getter 이다.
    checking = () => 0;
    // 장기 전역 사제 TYPE 을 돌려주는 getter 이며 null 이면 priestType 을 쓴다.
    sharedPriestType = null;

    // 사건 2 의 검색 요청 시점에 현재 DWORD 전체를 읽는다(BYTE 로 줄이지 않는다). (2026-11-10 WB04-E212)
    currentPriestType() {
        return this.sharedPriestType ? this.sharedPriestType() >>> 0 : this.priestType >>> 0;
    }
}

// 보호막 Regular 처리기이다. pool/hash/types(188칸, footX·footY)/state 는 처리기보다 오래 살아야 한다.
// hooks = { advance(sid, delta, flags), destroy(sid, flags), assertion(식, 파일, 줄), findTypeAt(x, y, 사제 TYPE) → sid }. findTypeAt 은 실제 월드의 타입별 단계 선택(004b1fa0)을 재사용하며, 없으면 독립 일반 finder 를 쓴다.
// 사용: regular.handle(sid, 사건, count, payload) 의 반환값이 다음 지연이다(음수는 종료). (2026-11-10 WB04-E212)
export class RawForcefieldRegular {
    // 객체 슬롯 풀, 좌표별 후보 해시, 타입 표, 현재 상태, 외부 효과 묶음이다.
    #pool;
    #hash;
    #types;
    #state;
    #hooks;

    // 판본별 타입 수를 확인한다. (2026-11-10 WB04-E212)
    constructor(pool, hash, types, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT) throw new TypeError('보호막 Regular 타입/효과 연결 오류');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 프레임 진행/후처리 연결의 풀 일치를 확인하기 위한 풀이다. (2026-11-10 WB04-E212)
    get pool() {
        return this.#pool;
    }

    // 현재 좌표를 0 방향 절삭해 원본 기본 finder 순서의 첫 타입 일치를 돌려준다. 후보의 소유자/HP/state 는 별도 필터가 아니며 일반 finder 의 매장 제외/발자국 조건만 적용된다. (2026-11-10 WB04-E212)
    #findPriest(sid) {
        const find = this.#hooks.findTypeAt;
        if (find) {
            // 실제 월드는 같은 typed finder 에 raw float 와 현재 사제 DWORD 를 전달한다.
            const type = this.#state.currentPriestType();
            const raw = this.#pool.slot(sid);
            const y = floatAt(raw, Y_OFFSET);
            const x = floatAt(raw, X_OFFSET);
            return find(x, y, type);
        }
        const raw = this.#pool.slot(sid);
        const x = cell(floatAt(raw, X_OFFSET));
        const y = cell(floatAt(raw, Y_OFFSET));
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 현재 해시 체인의 순서를 유지하며 DWORD 타입을 BYTE 로 줄이지 않는다.
        for (let current = finder.begin({ left: x, top: y, right: x, bottom: y }); current !== 0; current = finder.next()) {
            if (this.#pool.slot(current)[TYPE_OFFSET] === this.#state.currentPriestType()) return current;
        }
        return 0;
    }

    // 사건 1 은 (1,0) 진행 뒤 0.08f, 사건 2 는 사제가 있으면 0.01f/없으면 destroy(0) 뒤 -1 이다. 다른 사건은 0 이며 checking 이 비영이면 진단을 올린 뒤 계속 0 을 돌려준다.
    // count/payload 는 읽지 않는다. 삭제 효과가 실행 중인 Regular 도 지울 수 있으므로 이후 그 객체/슬롯을 다시 쓰지 않는다. (2026-11-10 WB04-E212)
    handle(sid, event, count, payload) {
        if (event === FORCEFIELD_FRAME_EVENT) {
            need(this.#hooks.advance)(sid, 1, 0);
            return FRAME_INTERVAL;
        }
        if (event === FORCEFIELD_CHECK_EVENT) {
            if (this.#findPriest(sid)) return CHECK_INTERVAL;
            need(this.#hooks.destroy)(sid, 0);
            return -1;
        }
        // 진단이 처리기나 부모를 삭제해도 선택 callable 의 사본으로 호출을 마치며 이후 슬롯/상태를 읽지 않는다.
        if (this.#state.checking()) need(this.#hooks.assertion)('0', '.\\Dais.cpp', 0x479);
        return 0;
    }
}

// 예약 성공 입력 이후에는 실제 Process 호스트의 Regular 추가를 쓰는 접두 훅 묶음을 만든다. 호스트와 풀이 다르거나 예약 효과가 없으면 거부한다. (2026-11-10 WB04-E212)
export function makeForcefieldPostPopProcessHooks(pool, host, reserve) {
    if (pool !== host?.pool || typeof reserve !== 'function') throw new TypeError('보호막/프로세스 풀 또는 확보 효과 오류');
    return { reserveRegular: reserve, addRegular: (sid, event, payload) => { host.addRegular(sid, event, payload); } };
}

// 프레임 진행기의 실제 프레임 지정/공간 효과를 Regular 의 advance 로 연결한다. 나머지 효과(destroy 등)는 hooks 를 그대로 둔다. (2026-11-10 WB04-E212)
export function makeForcefieldFrameHooks(pool, advance, hooks = {}) {
    if (pool !== advance?.pool) throw new TypeError('보호막/프레임 진행 풀 불일치');
    return { ...hooks, advance: (sid, delta, flags) => { advance.advance(sid, delta, flags); } };
}
