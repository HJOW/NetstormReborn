// 사제 postPop(004950f0)의 carrier 호출 전 접두 — 목록·회복 예약·소유자 상태 초기화를 옮긴 모듈이다(cpppj `o/RawPriestPostPop.cpp/.h`, 10.78 전용).
// 순서: extra 의 abstract/buried 비트(9)가 있으면 아무 효과도 없다. 최초 flag(비트 1)에서는 사제 목록(005954c4)에 중복 없이 추가하고(00414450: 처음 읽은 signed 개수가 signed 용량 미만일 때만, 현재 개수로 중복 검색,
//   처음 읽은 개수 위치에 저장한 뒤 현재 개수를 증가) 이미 예약된 회복(사건 0x25a)이 없고 예약(new(0x28))이 성공하면 `(현재 사제 TYPE HP / 6) * 50 / 객체 최대 HP`(float) payload 로 Regular 를 등록한다.
//   예약 효과 뒤에는 현재 원래 소유자 WORD 의 하위 7비트와 현재 소유자 BYTE 가 같고 유효 소유자 1..8 이면 소유자 상태(00595428) 한 칸을 0 으로 쓴다(진단 복귀 규칙은 resetPriestOwnerState).
// PATCH 는 사전 검사 없이 현재 물리 슬롯을 읽고 도달한 장치만 그 시점에 요구한다(명시 validate 는 별도). 현재 사제 TYPE(005412d0) 번호는 호출 때마다 getter 로 다시 읽는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E63: 사제 후처리 접두 이식으로 추가)

import { SquidPostPopList } from './squid-postpop-state.js';
import { PRIEST_TYPE, PRIEST_VTABLE } from './raw-priest-owner.js';

// 회복 Regular 의 실제 이벤트 번호(004950f0), 슬롯의 TYPE/extra/소유자 위치와 마스크, 회복 분모/배율, 플레이어 수이다. (2026-11-08 WB04-E63)
export const PRIEST_REGEN_EVENT = 0x25a;
const TYPE_OFFSET = 10;
const ORIGINAL_OWNER_OFFSET = 12;
const OWNER_OFFSET = 34;
const EXTRA_OFFSET = 40;
const SKIP_PREFIX = 9;
const REGEN_DIVISOR = 6;
const REGEN_SCALE = 50;
const PLAYER_COUNT = 8;

// 사제 후처리 Pop 중 조회·갱신하는 상태 묶음이다: 사제 목록(005954c4)과 소유자 상태 9 DWORD(00595428)이다. 실제 Session 은 장기 목록/표를 빌린다(sharedPriests·sharedOwnerState). (2026-11-08 WB04-E63)
export class PriestPostPopState {
    priests = new SquidPostPopList();
    ownerState = new Array(PLAYER_COUNT + 1).fill(0);
    sharedOwnerState = null;
    sharedPriests = null;

    // 실제 writer 가 그 시점의 동일 공유 표에 직접 쓰도록 현재 소유자 상태 표를 돌려준다. (2026-11-08 WB04-E63)
    currentOwnerState() { return this.sharedOwnerState ?? this.ownerState; }

    // Pop/삭제가 같은 현재 사제 목록에 직접 쓰도록 현재 목록을 돌려준다. (2026-11-08 WB04-E63)
    currentPriests() { return this.sharedPriests ?? this.priests; }
}

// 00492240: 범위 밖 번호는 진단 뒤 쓰기를 생략하고 유효 1..8 의 한 DWORD 만 0 으로 쓴다. 진단 장치가 없으면 거부한다. (2026-11-08 WB04-E63)
export function resetPriestOwnerState(states, player, assertion = null) {
    if (player < 1 || player > PLAYER_COUNT) {
        if (typeof assertion !== 'function') throw new Error('사제 owner 상태 초기화 진단 미연결');
        assertion(player >>> 0);
        return;
    }
    states[player] = 0;
}

// 선택된 장치가 없으면 도달한 위치에서 거부한다. (2026-11-08 WB04-E63)
function need(device) {
    if (typeof device !== 'function') throw new Error('사제 postPop 선택된 실제 하위 미연결');
    return device;
}

// 00414450(&005954c4, sid): 처음 읽은 signed 개수가 signed 용량보다 작을 때만 현재 개수와 비교하며 중복을 찾고, 저장 위치는 처음 읽은 개수를 쓰며 그 뒤 현재 개수를 한 번 증가시킨다.
//   음수/짧은 배열 저장은 실제 역참조 위치의 호스트 물리 경계로 거부한다. (2026-11-08 WB04-E63)
function appendUniquePriest(list, sid) {
    const used = list.readCount();
    const capacity = list.capacity();
    if (capacity > 0xffffffff) throw new RangeError('사제 목록 DWORD 용량 물리 범위 오류');
    if ((used | 0) >= (capacity | 0)) return;
    if ((used | 0) > 0) {
        const storage = list.storage();
        let index = 0;
        // 첫 항목은 최초 양수 개수로 들어오며 이후 반복 조건은 매번 현재 개수를 다시 읽는다.
        do {
            if (index >= storage.length) throw new RangeError('사제 목록 중복 검색 물리 배열 범위 오류');
            if (storage[index] === sid) return;
            index += 1;
        } while ((index | 0) < (list.readCount() | 0));
    }
    if ((used | 0) < 0) throw new RangeError('사제 목록 음수 저장 위치');
    const storage = list.storage();
    if (used >= storage.length) throw new RangeError('사제 목록 실제 저장 범위 오류');
    storage[used] = sid;
    list.setCount(list.readCount() + 1);
}

// pool 은 10.78 SidPool, hp 는 같은 풀의 SquidReward(typeHitPoints·maxHitPoints), state 는 PriestPostPopState, hooks = { findRegular(sid, 사건) → bool, reserveRegular() → bool, addRegular(sid, 사건, payload) },
//   priestType() 는 005412d0 의 현재 사제 TYPE 를 돌려주는 getter(기본 158)이다. 풀/목록/상태/getter 는 이 객체보다 오래 살아야 한다. (2026-11-08 WB04-E63)
export class RawPriestPostPop {
    // 객체 슬롯 풀, 최대 HP 처리기, 상태, 외부 효과, 현재 사제 TYPE getter 이다.
    #pool;
    #hp;
    #state;
    #hooks;
    #priestType;

    // 풀 혼합은 효과 전에 거부한다(PATCH 훅 사전 요구는 없다). (2026-11-08 WB04-E63)
    constructor(pool, hp, state, hooks, priestType = () => PRIEST_TYPE) {
        if (pool?.edition !== 'patch1078') throw new TypeError('사제 postPop은 10.78 풀만 받습니다.');
        if (hp?.pool !== pool) throw new TypeError('사제 postPop/최대 HP의 SID 풀이 다릅니다');
        this.#pool = pool;
        this.#hp = hp;
        this.#state = state;
        this.#hooks = hooks ?? {};
        this.#priestType = priestType;
    }

    // 후반 조합의 풀 혼합 검사용 풀이다. (2026-11-08 WB04-E63)
    get pool() {
        return this.#pool;
    }

    // 명시 사전 검사: 공간 쓰기 전에 가상 표/타입과 목록 개수를 확인한다. 효과는 발생시키지 않는다. 실제 10.78 가상 진입은 이 검사를 거치지 않는다. (2026-11-08 WB04-E63)
    validate(sid) {
        const raw = this.#pool.slot(sid);
        const table = (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
        if (raw[TYPE_OFFSET] !== PRIEST_TYPE || table !== PRIEST_VTABLE) throw new TypeError('사제 postPop 가상 표/타입 불일치');
        const list = this.#state.currentPriests();
        if (list.readCount() > list.capacity()) throw new RangeError('사제 목록 개수 오류');
    }

    // x87 은 int / 6 을 0 방향으로 절삭한 뒤 곱하고 float 으로 저장한다. 분자는 현재 사제 TYPE(0049a840)의 HP, 그 뒤 객체 최대 HP(004adcb0)를 읽는다. (2026-11-08 WB04-E63)
    #regenPayload(sid) {
        // 정수 나눗셈의 0 은 부호 없는 정수 0 이므로 -0 이 생기지 않게 정수로 되돌린다(음수 HP 의 나눗셈 결과 0 이 -0.0 이 되면 비트가 달라진다).
        const numerator = Math.fround(((this.#hp.typeHitPoints(this.#priestType()) / REGEN_DIVISOR) | 0) * REGEN_SCALE);
        return Math.fround(numerator / this.#hp.maxHitPoints(sid));
    }

    // 004950f0 의 접두이다. 전체 Carrier/보호막 처리는 RawPriestPostPopTail 이 이어 준다. (2026-11-08 WB04-E63)
    prefix(sid, flags) {
        const pool = this.#pool;
        if (pool.slot(sid)[EXTRA_OFFSET] & SKIP_PREFIX) return;
        if (flags & 1) {
            appendUniquePriest(this.#state.currentPriests(), sid);
            // 이미 있는 회복은 확보/HP 조회를 생략한다. 확보 실패도 HP 조회와 생성만 생략한다.
            if (!need(this.#hooks.findRegular)(sid, PRIEST_REGEN_EVENT) && need(this.#hooks.reserveRegular)()) {
                const payload = this.#regenPayload(sid);
                need(this.#hooks.addRegular)(sid, PRIEST_REGEN_EVENT, payload);
            }
        }
        // 예약 효과 이후 현재 원래 소유자 WORD 와 현재 소유자 BYTE 를 다시 읽고 유효 1..8 한 칸만 0 으로 만든다.
        const current = pool.slot(sid);
        const owner = current[OWNER_OFFSET];
        if ((current[ORIGINAL_OWNER_OFFSET] & 0x7f) === owner && owner > 0 && owner <= PLAYER_COUNT) this.#state.currentOwnerState()[owner] = 0;
    }
}

// 기존 ProcessHost 의 실제 찾기/부착에 연결하는 훅 묶음이다. reserve 는 원본 할당 실패를 주입할 경계로 기본 성공이다. (2026-11-08 WB04-E63)
export function makePriestPostPopProcessHooks(host) {
    return {
        findRegular: (sid, event) => Boolean(host.findEvent(sid, event)),
        reserveRegular: () => true,
        addRegular: (sid, event, payload) => { host.addRegular(sid, event, payload); }
    };
}
