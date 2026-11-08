// 사제 낙하 요청(004941f0)과 0x25b 이벤트의 조건/효과 순서를 옮긴 모듈이다(cpppj `o/RawPriestFall.cpp/.h`, 10.78 전용).
//  - begin(004941f0): 보호막 확보 → 예약(new(0x28))이 성공하면 공유 Regular(사건 0x25b, payload 0.01) 등록 → 현재 권한(보호막·생성 효과 뒤 다시 읽음)이면 Unpop(0)/Repop(0x800) → 현재 좌표의 소리 요청. 기존 낙하 예약 중복은 검색하지 않는다.
//  - tryFall(사제 vtable +0xc8, 00494e80): 현재 signed 방향이 J(9)가 아닐 때만 begin 을 부르고 항상 참을 반환한다.
//  - handle(0x25b, 00494580→00493e60): 사제가 이동 불가이면 보호막 확보 → 방향이 J 이면 프레임 진행(+1) 아니면 낙하 프레임 지정 → 지면(거의 올림 좌표의 표면 지도 WORD ≠ 0, 지도 밖은 없음)이 없으면 0.1 반환(재예약),
//    있으면 도착 프레임 지정 → 이동 가능해지면 보호막 해제 → 0 반환. count/payload 는 읽지 않는다.
// 풀/상태/프레임/지도는 이 객체보다 오래 살아야 한다. 일반 사제 Pop 지원 범위를 바꾸지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E65: 사제 낙하 이식으로 추가)

import { pathDirection } from './raw-path-animation.js';
import { PRIEST_VTABLE } from './raw-priest-owner.js';

// 낙하 처리기가 선택하는 실제 이벤트 번호(004941f0), 슬롯의 좌표/프레임/TYPE 위치, 낙하 시작/반복 payload 와 거의 올림 배율(float32 0.01·0.1·0.9999), 낙하 방향 J, 지도/타입 표 크기이다. (2026-11-08 WB04-E65)
export const PRIEST_FALL_EVENT = 0x25b;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const FRAME_OFFSET = 36;
const TYPE_OFFSET = 10;
const INITIAL_PAYLOAD = Math.fround(0.01);
const REPEAT_PAYLOAD = Math.fround(0.1);
const COORDINATE_BIAS = Math.fround(0.9999);
const FALLING_DIRECTION = 9;
const WORLD_CELLS = 256;
const TYPE_COUNT = 188;

// 낙하 요청이 권한 효과 직전에 다시 읽는 전역이다: authority(00540bc4)의 독립 값과 실제 Client 의 같은 boss 를 돌려주는 getter(sharedAuthority)이다. (2026-11-08 WB04-E65)
export class PriestFallState {
    authority = true;
    sharedAuthority = null;

    // 보호막·프로세스 생성 효과 뒤 현재 권한을 다시 읽는다. (2026-11-08 WB04-E65)
    isAuthority() {
        return this.sharedAuthority ? Boolean(this.sharedAuthority()) : this.authority;
    }
}

// 정렬되지 않은 필드를 리틀엔디언으로 읽는다. (2026-11-08 WB04-E65)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 비트를 단정도 float 로 해석한다. (2026-11-08 WB04-E65)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    return view.getFloat32(0, true);
}

// pool 은 10.78 SidPool, priest 는 RawPriestState, frames 는 188칸 FrameCodeTable 표, choices 는 188칸 { falling, landed } 표, surfaces 는 65536칸 표면 지도 WORD 배열, state 는 PriestFallState,
//   hooks = { ensureShield, reserveRegular, addSharedRegular(sid, 사건, payload), unpop(sid, flags), repop(sid, flags), sound(sid, x, y), setFrame(sid, 프레임, flags), advanceFrame(sid, 증분, flags), clearShield } 이며 모두 필요하다. (2026-11-08 WB04-E65)
export class RawPriestFall {
    // 객체 슬롯 풀, 사제 상태, 프레임/선택/표면 표, 권한 전역, 외부 효과이다.
    #pool;
    #priest;
    #frames;
    #choices;
    #surfaces;
    #state;
    #hooks;

    // 모든 실행 경계를 미리 확인하여 미연결 상태에서 일부 효과만 실행되는 일을 막는다. (2026-11-08 WB04-E65)
    constructor(pool, priest, frames, choices, surfaces, state, hooks) {
        const required = ['ensureShield', 'reserveRegular', 'addSharedRegular', 'unpop', 'repop', 'sound', 'setFrame', 'advanceFrame', 'clearShield'];
        if (pool?.edition !== 'patch1078' || priest?.pool !== pool || !Array.isArray(frames) || frames.length !== TYPE_COUNT || !Array.isArray(choices) || choices.length !== frames.length
            || surfaces?.length !== WORLD_CELLS * WORLD_CELLS || required.some((name) => typeof hooks?.[name] !== 'function')) {
            throw new TypeError('사제 낙하 연결 오류');
        }
        this.#pool = pool;
        this.#priest = priest;
        this.#frames = frames;
        this.#choices = choices;
        this.#surfaces = surfaces;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 분배기/효과 어댑터의 같은 월드 연결 검사에 쓰는 풀이다. (2026-11-08 WB04-E65)
    get pool() {
        return this.#pool;
    }

    // 확보 실패여도 권한 공간 전환/소리 효과를 계속한다. 권한은 생성 효과 다음에 다시 읽는다. (2026-11-08 WB04-E65)
    begin(sid) {
        const hooks = this.#hooks;
        this.#pool.slot(sid);
        hooks.ensureShield(sid);
        if (hooks.reserveRegular()) hooks.addSharedRegular(sid, PRIEST_FALL_EVENT, INITIAL_PAYLOAD);
        if (this.#state.isAuthority()) {
            hooks.unpop(sid, 0);
            hooks.repop(sid, 0x800);
        }
        const raw = this.#pool.slot(sid);
        hooks.sound(sid, floatOf(readDword(raw, X_OFFSET)), floatOf(readDword(raw, Y_OFFSET)));
    }

    // 이미 낙하 방향이면 요청을 반복하지 않는다. 반환은 실제 효과 발생 여부와 관계없이 참이다. (2026-11-08 WB04-E65)
    tryFall(sid) {
        const raw = this.#pool.slot(sid);
        if (raw[TYPE_OFFSET] >= this.#frames.length) throw new RangeError('사제 낙하 요청 프레임 타입 오류');
        const frame = readDword(raw, FRAME_OFFSET) | 0;
        if (pathDirection(this.#frames[raw[TYPE_OFFSET]], frame) !== FALLING_DIRECTION) this.begin(sid);
        return true;
    }

    // float 입력의 합을 double 로 유지한 뒤 0 방향 절삭한다. 지도 WORD 는 유효 SID 여부 없이 검사한다. 지도 밖은 지면 없음이며 비유한 입력은 진단한다. (2026-11-08 WB04-E65)
    #surfaceAt(sid) {
        const raw = this.#pool.slot(sid);
        const x = Math.trunc(floatOf(readDword(raw, X_OFFSET)) + COORDINATE_BIAS);
        const y = Math.trunc(floatOf(readDword(raw, Y_OFFSET)) + COORDINATE_BIAS);
        if (!Number.isFinite(x) || !Number.isFinite(y)) throw new RangeError('사제 낙하 좌표 오류');
        if (x < 0 || y < 0 || x >= WORLD_CELLS || y >= WORLD_CELLS) return false;
        return this.#surfaces[y * WORLD_CELLS + x] !== 0;
    }

    // 시작/도착 프레임 효과 뒤 이동 불가를 다시 조회한다. 공통 Regular 가 반환값으로 재예약/삭제한다. (2026-11-08 WB04-E65)
    handle(sid, event) {
        if (event !== PRIEST_FALL_EVENT) throw new RangeError('사제 낙하 이벤트 번호 오류');
        const hooks = this.#hooks;
        if (this.#priest.immobile(sid)) hooks.ensureShield(sid);
        const raw = this.#pool.slot(sid);
        if (raw[TYPE_OFFSET] >= this.#frames.length) throw new RangeError('사제 낙하 프레임 타입 오류');
        const frame = readDword(raw, FRAME_OFFSET) | 0;
        if (pathDirection(this.#frames[raw[TYPE_OFFSET]], frame) === FALLING_DIRECTION) hooks.advanceFrame(sid, 1, 0);
        else hooks.setFrame(sid, this.#choices[raw[TYPE_OFFSET]].falling, 0);
        if (!this.#surfaceAt(sid)) return REPEAT_PAYLOAD;
        const type = this.#pool.slot(sid)[TYPE_OFFSET];
        if (type >= this.#choices.length) throw new RangeError('사제 도착 프레임 타입 오류');
        hooks.setFrame(sid, this.#choices[type].landed, 0);
        if (!this.#priest.immobile(sid)) hooks.clearShield(sid);
        return 0;
    }
}

// 실제 사제 vtable 의 0x25b 를 처리하고 다른 사건은 명시 fallback 에 넘긴다. 사제 미복원 사건은 기본 허용하지 않는다. 반환은 Regular 핸들러 (sid, 사건, 개수, payload) → payload 이다. (2026-11-08 WB04-E65)
export function makePriestFallHandler(pool, fall, fallback = null) {
    if (fall.pool !== pool) throw new TypeError('사제 낙하 분배 풀 불일치');
    return (sid, event, count, payload) => {
        const raw = pool.slot(sid);
        const priest = readDword(raw, 0) === PRIEST_VTABLE;
        if (priest && event === PRIEST_FALL_EVENT) return fall.handle(sid, event, count, payload);
        if (fallback) return fallback(sid, event, count, payload);
        if (priest) throw new TypeError('미복원 사제 이벤트 분배');
        return payload;
    };
}

// 실제 사제 vtable 의 +0xc8 을 연결한다. 다른 walker 는 명시 fallback 이 있어야 처리한다. 표면 삭제/Carrier 조회에서 같은 사제 낙하 요청을 공유하며 일반 Pop 지원은 넓히지 않는다. (2026-11-08 WB04-E65)
export function makePriestFallWalker(pool, fall, fallback = null) {
    if (fall.pool !== pool) throw new TypeError('walker 낙하 분배 풀 불일치');
    return (sid) => {
        if (readDword(pool.slot(sid), 0) === PRIEST_VTABLE) return fall.tryFall(sid);
        if (fallback) return fallback(sid);
        throw new TypeError('미복원 walker 낙하 가상 표');
    };
}

// 실제 공유 Regular/보호막 생성·조회와 공급된 공간/소리 훅을 합친다. host 는 { pool, addSharedRegular }, shield 는 RawPriestShield, lookup 은 RawPriestForcefield 이다. (2026-11-08 WB04-E65)
export function makePriestFallHooks(host, shield, lookup, destroy, hooks) {
    const pool = host.pool;
    if (pool !== shield.pool || pool !== lookup.pool || typeof destroy !== 'function') throw new TypeError('사제 낙하 효과 풀 불일치');
    return {
        ...hooks,
        ensureShield: (sid) => shield.ensure(sid),
        reserveRegular: () => true,
        addSharedRegular: (sid, event, payload) => { host.addSharedRegular(sid, event, payload); },
        clearShield: (sid) => {
            const found = lookup.find(sid);
            if (found > 0 && found < pool.capacity) destroy(found, 0);
        }
    };
}

// 0x25b 의 시작/도착 지정과 J 진행을 같은 실제 프레임 지정기에 연결한다. 이벤트의 DWORD 증분은 signed 인자 비트로 전달하고 진행 반환값은 버린다. (2026-11-08 WB04-E65)
export function makePriestFallFrameHooks(setter, advance, hooks) {
    if (setter.pool !== advance.pool || setter !== advance.setter) throw new TypeError('사제 낙하 프레임 연결 오류');
    return {
        ...hooks,
        setFrame: (sid, frame, flags) => { setter.set(sid, frame, flags); },
        advanceFrame: (sid, delta, flags) => { advance.advance(sid, delta | 0, flags); }
    };
}
