// 다리 이벤트 처리기(다리 vtable +0x5c, 00422740)와 끝 칸 변환(004215d0)을 실제 raw SID 풀 위에서 옮긴 모듈이다(cpppj `o/RawBridgeEvents.cpp/.h`, 10.78 전용).
// 이웃 탐색·삭제·생성·소유자 지정·Pop·표면 알림은 호출자가 훅으로 연결하며 순서·인자만 원본대로 호출한다.
//  - handle(bridge, event, count, payload): 이벤트 0x2691(자기 destroy)과 0x2692(지연 낙하: payload 의 좌표 포장 float 를 풀어 끝 칸 변환)를 처리하고 Regular 계약의 반환값을 돌려준다
//    (권한이 없으면 0=종료, 있으면 -1=유지, 처리하지 않는 이벤트는 payload 그대로). count 는 원본이 읽지 않는다.
//  - convertEnd(bridge, x, y): 현재 프레임의 방향 글자(J 는 y, K 는 x 비교)로 끝 글자(L·M·N·O)를 고르고 임시 프레임으로 flag 0 이웃을 묻는다. 이웃이 없으면 destroy, 있으면 수명 비트를 쓰고
//    (단단한 프레임이 아니면) 같은 타입의 새 객체를 만들어 끝 프레임·소유자·수명·부모·플래그 비트를 옮긴 뒤 옛 칸을 지우고 같은 위치에 Pop 한다.
//  - makeBridgeRegularHandler(pool, events, fallback): Regular 프로세스가 부르는 부모 이벤트 처리기를 가상 표로 분배한다(다리 클래스면 다리 처리기, 아니면 fallback, 없으면 payload).
//  - makeBridgeNeighborHooks(neighbors, effects): 같은 타입 표의 프레임 조회와 실제 flag 0 첫 이웃을 연결한다.
// 프레임 표는 { codes: [{side, variant, number, flags}], run(letter) → { first, count } } (FrameCodeTable 모양)이다. 이 모듈은 DOM 을 쓰지 않는다. CD 10.72 분기는 옮기지 않았다.
// (2026-11-08 WB04-E43: RawBridgeEvents 이식으로 추가)
import { BRIDGE_FALL_EVENT } from './bridge-lifecycle.js';
import { FrameCodeTable } from '../assets/terrain-tiles.js';
import { RawSquidNeighborWalk } from './raw-squid-neighbors.js';

// 이벤트 0x2691: 권한이 있으면 다리 칸이 스스로 destroy 한다. (0x2692 = BRIDGE_FALL_EVENT 는 지연 낙하이다.) (2026-11-08 WB04-E43)
export const BRIDGE_DESTROY_EVENT = 0x2691;

// 처리기 반환값은 Regular 이벤트 계약을 따른다: 양수 재예약, 0 종료, 음수 유지. 권한이 없으면 0, 있으면 -1 이다. (2026-11-08 WB04-E43)
export const BRIDGE_EVENT_END = 0;
export const BRIDGE_EVENT_KEEP = -1;

// 다리 클래스의 가상 표 주소 기록값(10.78)이다. raw 슬롯 +0 의 DWORD 와 비교만 한다. (2026-11-08 WB04-E43)
export const BRIDGE_VTABLE = 0x005034c8;

// 수명 단어(+12)의 수명 비트 마스크(3~6비트)와 금 간 수명이다. 끝 칸 변환은 (금 간 수명 − 1)을 수명 비트에 쓴다. 값은 `bridge-lifecycle.js`와 같다. (2026-11-08 WB04-E43)
const LIFE_MASK = 0x78;
const CRACK_LIFE = 5;

// 공통 raw 필드: 부모 단어(+8), 타입(+10), 수명 단어(+12), 위치 float x(+14)·y(+18), 소유자(+34), 프레임 DWORD(+36), 플래그(+40). (2026-11-08 WB04-E43)
const PARENT = 8;
const TYPE_OFFSET = 10;
const WORD = 12;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const OWNER_OFFSET = 34;
const FRAME_OFFSET = 36;
const FLAG_OFFSET = 40;

// 프레임 코드 플래그 0x40(단단한 프레임: 교체하지 않음), 새 객체로 옮기는 플래그 비트 0x10, 원본 assert("!isForm()")가 보는 첫 객체 타입(0x46) 이다. (2026-11-08 WB04-E43)
const HARD_FRAME_FLAG = 0x40;
const COPIED_FLAG_BIT = 0x10;
const FIRST_OBJECT_TYPE = 0x46;

// 끝 칸 변환의 방향 글자: 현재 프레임 J(0x4a)·K(0x4b), 바꿀 끝 프레임 L·M·N·O 이다. (2026-11-08 WB04-E43)
const SIDE_J = 0x4a;
const SIDE_K = 0x4b;
const END_L = 'L';
const END_M = 'M';
const END_N = 'N';
const END_O = 'O';

// 정렬되지 않은 리틀엔디언 단어/DWORD 읽기와 쓰기, float 비트 읽기이다. (2026-11-08 WB04-E43)
const floatView = new DataView(new ArrayBuffer(4));
function read(raw, offset, width) {
    let value = 0;
    // 낮은 바이트부터 합친다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}
function write(raw, offset, value, width) {
    // 낮은 바이트부터 쓴다.
    for (let index = 0; index < width; index += 1) raw[offset + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}
function coordinate(raw, offset) {
    floatView.setUint32(0, read(raw, offset, 4), true);
    return floatView.getFloat32(0, true);
}

// 원본 CRT _ftol 은 64비트 절삭값의 하위 DWORD 를 쓴다. 비유한/64비트 밖은 정수 불확정값이라 하위 0 이다. (2026-11-08 WB04-E43)
function ftolLow(value) {
    if (!(value > -9223372036854775808 && value < 9223372036854775808)) return 0;
    return Math.trunc(value) >>> 0;
}

// 같은 raw 타입 표의 프레임 조회와 실제 flag 0 첫 이웃을 연결한 훅 묶음을 돌려준다. 나머지 효과(notifySurface·create·destroy·setOwner·pop)는 effects 가 공급한다.
// 사용: makeBridgeNeighborHooks(neighbors, { notifySurface, create, destroy, setOwner, pop }). neighbors 는 반환 훅보다 오래 살아야 하며 조회 중 바뀐 raw 프레임을 그때 읽는다. (2026-11-08 WB04-E43)
export function makeBridgeNeighborHooks(neighbors, effects) {
    return {
        ...effects,
        frames: (type) => new FrameCodeTable(neighbors.frames(type)),
        firstNeighbor: (bridge) => new RawSquidNeighborWalk(neighbors, bridge, 0).current,
    };
}

// 다리 이벤트 처리기이다. hooks = { frames(type) → 프레임 표, firstNeighbor(bridge) → sid(0=없음), notifySurface(sid), create(type) → sid, destroy(sid, flags), setOwner(sid, owner), pop(sid, x, y, flags) } 가 모두 있어야 한다.
// state = { authority(기본 true), debugKeep, bridgeType }. 풀·상태·훅은 이 객체보다 오래 살아야 한다. 사용: new RawBridgeEvents(pool, state, hooks).handle(bridge, 0x2692, 0, payload). (2026-11-08 WB04-E43)
export class RawBridgeEvents {
    #pool;
    #state;
    #hooks;

    // 외부 효과는 빠짐없이 연결되어야 한다.
    constructor(pool, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('다리 이벤트는 10.78 풀만 받습니다.');
        for (const name of ['frames', 'firstNeighbor', 'notifySurface', 'create', 'destroy', 'setOwner', 'pop']) {
            if (typeof hooks?.[name] !== 'function') throw new TypeError('다리 이벤트 훅이 모두 연결되어야 한다');
        }
        this.#pool = pool;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 프레임 DWORD(부호 있는 번호)를 읽고 쓴다.
    #frame(sid) { return read(this.#pool.slot(sid), FRAME_OFFSET, 4) | 0; }

    #setFrame(sid, frame) { write(this.#pool.physical(sid), FRAME_OFFSET, frame >>> 0, 4); }

    // 004ac220: 수명 단어를 쓴 뒤 객체 타입이 다리 타입이면 표면 알림을 부른다.
    #setWord(sid, word) {
        const raw = this.#pool.physical(sid);
        write(raw, WORD, word, 2);
        if (raw[TYPE_OFFSET] === this.#state.bridgeType) this.#hooks.notifySurface(sid);
    }

    // 004215d0: 다리 칸을 방향에 맞는 끝 칸(L·M·N·O)으로 바꾼다. (x, y)는 지연 낙하가 가리키는 칸 좌표이다. 권한이 없거나 디버그 유지가 켜져 있으면 아무것도 하지 않는다.
    convertEnd(bridge, x, y) {
        const state = this.#state;
        const hooks = this.#hooks;
        const pool = this.#pool;
        // 첫 조건: 디버그 유지가 꺼져 있고 권한이 있어야 한다.
        if (state.debugKeep || !state.authority) return;
        const type = pool.slot(bridge)[TYPE_OFFSET];
        if (type < FIRST_OBJECT_TYPE) throw new Error('다리 타입이 form이다(004ad680 assert)');
        const frames = hooks.frames(type);
        const codes = frames.codes;
        const oldFrame = this.#frame(bridge);
        if (oldFrame < 0 || oldFrame >= codes.length) throw new RangeError('다리 현재 프레임 범위 오류');
        const raw = pool.slot(bridge);
        const objectX = coordinate(raw, X_OFFSET);
        const objectY = coordinate(raw, Y_OFFSET);
        // 004ad680: 현재 프레임 코드의 방향 글자를 signed char 로 읽는다. J 는 y, K 는 x 를 비교한다. 비교가 거짓이거나 NaN 이면 각각 L, O 쪽이다.
        const side = (codes[oldFrame].side << 24) >> 24;
        let letter;
        if (side === ((SIDE_J << 24) >> 24)) letter = objectY < y ? END_N : END_L;
        else if (side === ((SIDE_K << 24) >> 24)) letter = objectX < x ? END_M : END_O;
        else return;
        // 타입의 글자별 첫 프레임 표. 끝 프레임이 없는 타입은 원본도 -1 을 프레임으로 써 정의되지 않는 동작이다.
        const first = frames.run(letter).first;
        if (first < 0) throw new Error('다리 타입에 끝 프레임 글자가 없다');
        const newFrame = first >>> 0;
        // 임시로 새 프레임을 써서 flag 0 이웃 탐색기를 만들고 곧바로 원래 프레임을 되돌린다.
        this.#setFrame(bridge, newFrame);
        let neighbor;
        try {
            neighbor = hooks.firstNeighbor(bridge);
        } catch (error) {
            this.#setFrame(bridge, oldFrame);
            throw error;
        }
        this.#setFrame(bridge, oldFrame);
        // 이웃이 없으면 이 칸은 끝 칸이 될 수 없으므로 destroy 한다.
        if (neighbor === 0) {
            hooks.destroy(bridge, 0);
            return;
        }
        // 수명 비트(+0xc 의 3~6비트)를 (금 간 수명 - 1)로 쓰고 표면 알림을 부른다.
        const lifeBits = (CRACK_LIFE * 8 - 8) & LIFE_MASK;
        const word = ((read(pool.slot(bridge), WORD, 2) & ~LIFE_MASK) | lifeBits) & 0xffff;
        write(pool.physical(bridge), WORD, word, 2);
        hooks.notifySurface(bridge);
        // 단단한 프레임 검사는 되돌려 놓은 옛 프레임의 플래그에 대해 한다. 켜져 있으면 객체를 교체하지 않는다.
        const current = this.#frame(bridge);
        if (current < 0 || current >= codes.length) throw new RangeError('다리 프레임 범위 오류');
        if (codes[current].flags & HARD_FRAME_FLAG) return;
        // 같은 타입의 새 객체를 만들어 끝 프레임·소유자·수명 단어·부모 단어·플래그 비트를 옮긴다.
        const born = hooks.create(type);
        this.#setFrame(born, newFrame);
        hooks.setOwner(born, pool.slot(bridge)[OWNER_OFFSET]);
        this.#setWord(born, read(pool.slot(bridge), WORD, 2));
        write(pool.physical(born), PARENT, read(pool.slot(bridge), PARENT, 2), 2);
        const newRaw = pool.physical(born);
        newRaw[FLAG_OFFSET] = (newRaw[FLAG_OFFSET] & ~COPIED_FLAG_BIT) | (pool.slot(bridge)[FLAG_OFFSET] & COPIED_FLAG_BIT);
        // 옛 칸의 위치를 읽고 옛 칸을 지운 다음 새 객체를 같은 위치에 놓는다.
        const atX = coordinate(pool.slot(bridge), X_OFFSET);
        const atY = coordinate(pool.slot(bridge), Y_OFFSET);
        hooks.destroy(bridge, 0);
        hooks.pop(born, atX, atY, 0);
    }

    // 00422740: 이벤트를 처리하고 Regular 계약의 반환값을 돌려준다. 0x2691 은 자기 destroy, 0x2692 는 payload(좌표 포장 float)를 풀어 끝 칸 변환을 한다. 그 밖의 이벤트는 payload 를 그대로 돌려준다.
    handle(bridge, event, count, payload) {
        const state = this.#state;
        if (event === BRIDGE_DESTROY_EVENT) {
            // 권한이 없으면 아무것도 하지 않고 예약을 끝낸다(반환 0).
            if (!state.authority) return BRIDGE_EVENT_END;
            this.#hooks.destroy(bridge, 0);
            return BRIDGE_EVENT_KEEP;
        }
        if (event === BRIDGE_FALL_EVENT) {
            if (!state.authority) return BRIDGE_EVENT_END;
            // payload 는 (x & 255) | (y << 8)을 float 로 바꾼 값이다. _ftol 하위 DWORD 에서 x 는 하위 8비트, y 는 부호 있는 오른쪽 8비트 이동이다.
            const low = ftolLow(Math.fround(payload));
            this.convertEnd(bridge, low & 0xff, (low | 0) >> 8);
            return BRIDGE_EVENT_KEEP;
        }
        // 처리하지 않는 이벤트는 payload 를 그대로 돌려준다(base 이벤트 처리기와 같다).
        return payload;
    }
}

// Regular 프로세스가 부르는 부모 이벤트 처리기(vtable +0x5c)를 객체의 가상 표로 분배한다. 부모의 가상 표가 다리 클래스이면 다리 처리기를, 아니면 fallback 을 부른다.
// fallback 이 없으면 원본 base 처리기처럼 payload 를 돌려준다. pool·events 는 반환한 처리기보다 오래 살아야 한다. (2026-11-08 WB04-E43)
export function makeBridgeRegularHandler(pool, events, fallback = null) {
    return (parent, event, count, payload) => {
        // 슬롯 맨 앞 DWORD 가 가상 표 주소 기록값이다.
        if (read(pool.slot(parent), 0, 4) === BRIDGE_VTABLE) return events.handle(parent, event, count, payload);
        return fallback ? fallback(parent, event, count, payload) : payload;
    };
}
