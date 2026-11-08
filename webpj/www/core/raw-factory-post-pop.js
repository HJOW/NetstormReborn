// Factory/아웃포스트의 예약·상태 프레임·중복 삭제·목록 등록을 같은 raw 월드에서 실행하는 모듈이다(cpppj `o/RawFactoryPostPop.cpp/.h`, 10.78 전용).
// 순서: 아웃포스트 wrapper(가상 표 0x509210)의 최초 flag 이고 extra 의 abstract/buried 비트(9)가 없으면 아웃포스트·거주 목록에 중복 없이 추가하고 섬 소유권을 다시 계산한다(regionOwnership).
//   최초 flag 에서는 현재 TYPE 이 첫 타이머(85)면 사건 2/8초, 둘째 타이머(87)면 사건 1/0.5초, 그리고 항상 사건 5/0.5초를 각각 독립 예약한다(실패한 예약은 해당 사건만 생략).
//   그 뒤 직접 Damageable 후처리(carrier)를 실행하고 상태별 프레임을 보정한다(현재 TYPE 이 고정 프레임 TYPE(126)이면 생략, 아니면 가상 +0x80 개수로 'A'+개수 글자 구간을 읽어 현재 프레임이 구간 밖일 때만 첫 프레임을 설정).
//   boss 이면 같은 칸의 현재 일반 finder 후보 중 같은 TYPE 이고 extra 의 선택 비트 8 이 같은 중복 워크샾을 경고 로그 뒤 삭제한다(finder 의 next 는 삭제 전에 저장).
// 가상 +0x80 개수는 확인된 Factory 표의 base getter(상태 BYTE 상위 3비트)만 허용하고, 도달한 장치만 요구한다. 현재 TYPE/프레임 구간/타이머 번호는 소비 위치마다 다시 읽는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E61: Factory 후처리 이식으로 추가)

import { RawSquidFinder } from './raw-squid-finder.js';

// 10.78 Factory(0x509148)·아웃포스트 wrapper(0x509210) 가상 표, extra 의 abstract+buried 비트, 선택 비트, 슬롯 위치이다. (2026-11-08 WB04-E61)
const FACTORY_VTABLE = 0x509148;
const OUTPOST_VTABLE = 0x509210;
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const FRAME_OFFSET = 36;
const EXTRA_OFFSET = 40;
const ABSTRACT_BURIED = 9;
const SELECT_BIT = 8;
const TYPE_COUNT = 188;
const FRAME_LETTERS = 16;
const INT64_LIMIT = 9223372036854775808;

// 선택된 장치가 없으면 그 위치에서 거부한다. (2026-11-08 WB04-E61)
function need(device) {
    if (typeof device !== 'function') throw new Error('Factory 후처리의 선택된 장치 미연결');
    return device;
}

// 슬롯의 리틀엔디언 DWORD 이다. (2026-11-08 WB04-E61)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 슬롯 좌표의 float32 값이다. (2026-11-08 WB04-E61)
function position(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, readDword(raw, offset), true);
    return view.getFloat32(0, true);
}

// CRT ftol 의 signed64 범위와 low DWORD 를 유지하며 손상 입력은 거부한다. (2026-11-08 WB04-E61)
function truncate(value) {
    if (!Number.isFinite(value) || value < -INT64_LIMIT || value >= INT64_LIMIT) throw new RangeError('Factory 좌표 CRT 범위 오류');
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 두 판본 모두 목록 용량 안에서 중복을 건너뛰고 SID 를 추가한다. (2026-11-08 WB04-E61)
function addUnique(list, sid) {
    const items = list.items();
    if (list.readCount() < list.capacity() && !items.includes(sid)) list.append(sid);
}

// 원본이 직접 읽는 타입 번호이다: 첫 타이머 Factory(DAT005411ac, 썬 워크샾 85), 둘째 타이머(005411b4, 87), 상태 프레임 보정 제외 TYPE(00541250, 126). 실제 Client 의 현재 DWORD 를 getter 로 빌릴 수 있고,
//   없으면 생성 때의 값 사본을 쓴다. 전체 DWORD 비교를 유지하고 BYTE 로 정규화하지 않는다. (2026-11-08 WB04-E61)
export class FactoryPostPopTypes {
    firstTimerFactory = 85;
    secondTimer = 87;
    fixedFrame = 126;
    currentFirstTimer = null;
    currentSecondTimer = null;
    currentFixedFrame = null;

    // 첫 예약 뒤에도 다시 읽는 첫 타이머 TYPE 이다. (2026-11-08 WB04-E61)
    readFirstTimer() { return this.currentFirstTimer ? this.currentFirstTimer() >>> 0 : this.firstTimerFactory; }

    // 첫 예약 콜백 뒤 다시 읽는 둘째 타이머 TYPE 이다. (2026-11-08 WB04-E61)
    readSecondTimer() { return this.currentSecondTimer ? this.currentSecondTimer() >>> 0 : this.secondTimer; }

    // 공통 부모 뒤 프레임 보정 진입 시 읽는 제외 TYPE 이다. (2026-11-08 WB04-E61)
    readFixedFrame() { return this.currentFixedFrame ? this.currentFixedFrame() >>> 0 : this.fixedFrame; }
}

// pool 은 10.78 SidPool, hash 는 SquidHash, types 는 188칸 TYPE 표(footX·footY·flags2), frames 는 188칸 FrameCodeTable 표(run(글자) → {first, count}), base 는 같은 풀의 SquidPostPop,
//   damageable 은 같은 풀의 RawCarrierPostPop, state 는 AssetPostPopState, hooks = { reserveRegular, addRegular(sid, 사건, 지연), setFrame(sid, 프레임, flags), regionOwnership(x, y),
//   warnDuplicate(sid, x, y), destroy(sid, flags), updateState(sid), virtualCount(sid) }, numbers 는 FactoryPostPopTypes. (2026-11-08 WB04-E61)
export class RawFactoryPostPop {
    // 모든 객체를 조회·수정하는 풀, 중복 검사에 쓰는 공간 해시, 현재 TYPE/프레임 표, 공통 장부/직접 Damageable, 목록·권한 상태, 외부 효과와 타입 번호이다.
    #pool;
    #hash;
    #types;
    #frames;
    #base;
    #damageable;
    #state;
    #hooks;
    #numbers;

    // 같은 풀/판본/프레임 표를 확인한다. (2026-11-08 WB04-E61)
    constructor(pool, hash, types, frames, base, damageable, state, hooks, numbers = new FactoryPostPopTypes()) {
        if (pool?.edition !== 'patch1078' || base?.pool !== pool || damageable?.pool !== pool || types.length !== frames.length || types.length !== TYPE_COUNT) {
            throw new TypeError('Factory 후처리 풀/판본/프레임 표 불일치');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#frames = frames;
        this.#base = base;
        this.#damageable = damageable;
        this.#state = state;
        this.#hooks = hooks ?? {};
        // cpppj 가 값으로 받는 타입 번호 묶음이므로 생성 때 사본을 만든다(독립 호출자의 이후 변경이 처리기에 섞이지 않는다).
        this.#numbers = Object.assign(new FactoryPostPopTypes(), numbers);
    }

    // 분배기의 동일 풀 계약에 쓰는 풀이다. (2026-11-08 WB04-E61)
    get pool() {
        return this.#pool;
    }

    // 실제 생성자가 기록하는 vtable 시작 주소로 0 은 미복원, 1 은 Factory, 2 는 아웃포스트 wrapper 이다. (2026-11-08 WB04-E61)
    #kind(sid) {
        const table = readDword(this.#pool.slot(sid), 0);
        if (table === FACTORY_VTABLE) return 1;
        if (table === OUTPOST_VTABLE) return 2;
        return 0;
    }

    // 가상 호출은 매번 현재 VT 로 분배한다. 미연결 독립 입력은 확인된 두 Factory 표의 base getter(상태 상위 3비트)만 허용한다. 독립 004ad0f0 도 현재 TYPE 접근을 먼저 한다. (2026-11-08 WB04-E61)
    #count(sid) {
        if (this.#hooks.virtualCount) return need(this.#hooks.virtualCount)(sid) | 0;
        if (!this.#kind(sid)) throw new Error('Factory 프레임의 현재 가상80 미연결');
        if (this.#pool.slot(sid)[TYPE_OFFSET] >= this.#types.length) throw new RangeError('Factory Count의 현재 TYPE 물리 범위 오류');
        return this.#pool.slot(sid)[STATE_OFFSET] >> 5;
    }

    // 각 가상 호출이 바꾼 현재 TYPE 의 프레임 표를 읽고 물리 범위를 확인한다. (2026-11-08 WB04-E61)
    #currentFrames(sid) {
        const type = this.#pool.slot(sid)[TYPE_OFFSET];
        if (type >= this.#frames.length) throw new RangeError('Factory 현재 프레임 TYPE 물리 범위 오류');
        return this.#frames[type];
    }

    // 타입 이름 대신 실제 가상 표가 Factory/아웃포스트 몸체를 가리키는지 판단한다. (2026-11-08 WB04-E61)
    handles(sid) {
        return this.#kind(sid) !== 0;
    }

    // 독립 사전 검사이다. 실제 10.78 몸체는 도달한 하위 장치만 확인한다. (2026-11-08 WB04-E61)
    validate(sid, flags, pop = null) {
        const raw = this.#pool.slot(sid);
        const kind = this.#kind(sid);
        if (!kind || raw[TYPE_OFFSET] >= this.#types.length) throw new Error('Factory 후처리 가상 표/타입 미복원');
        this.#base.validateBase(sid, flags, pop);
        if ((flags & 1) && (!this.#hooks.reserveRegular || !this.#hooks.addRegular)) throw new Error('Factory Regular 경계 누락');
        if (!this.#hooks.setFrame) throw new Error('Factory 실제 프레임 설정 경계 누락');
        if (this.#state.readBoss() && (!this.#hooks.destroy || !this.#hooks.warnDuplicate)) throw new Error('Factory 중복 삭제/경고 경계 누락');
        if (kind === 2) {
            this.#state.currentOutposts().items();
            this.#state.currentResidences().items();
            if (!this.#hooks.regionOwnership) throw new Error('아웃포스트 실제 섬 소유권 경계 누락');
        }
    }

    // 00453780: raw 현재 TYPE/상태/프레임을 효과 뒤 다시 읽고 원본 signed DWORD 덧셈 감김을 유지한다. 업그레이드 완료에서도 같은 상태 프레임 보정을 쓴다. (2026-11-08 WB04-E61)
    adjustFrame(sid) {
        if (this.#pool.slot(sid)[TYPE_OFFSET] === this.#numbers.readFixedFrame()) return;
        const frame = readDword(this.#pool.slot(sid), FRAME_OFFSET) | 0;
        const count = this.#count(sid);
        if (count < 0 || count >= FRAME_LETTERS) throw new RangeError('Factory 현재 프레임 글자 물리 범위 오류');
        // 00453798 가상 +0x80 뒤 004ac550 의 현재 TYPE 조회 순서이다.
        const run = this.#currentFrames(sid).run(String.fromCharCode(0x41 + count));
        if (run.first <= frame) {
            const nextCount = this.#count(sid);
            if (nextCount < 0 || nextCount >= FRAME_LETTERS) throw new RangeError('Factory 현재 프레임 개수 글자 물리 범위 오류');
            const current = this.#currentFrames(sid).run(String.fromCharCode(0x41 + nextCount));
            const end = (run.first + current.count) | 0;
            if (frame < end) return;
        }
        need(this.#hooks.setFrame)(sid, run.first, 0);
    }

    // 각각의 실패는 해당 예약만 건너뛰며 다음 호출 순서는 유지한다. (2026-11-08 WB04-E61)
    #regular(sid, event, delay) {
        if (need(this.#hooks.reserveRegular)()) need(this.#hooks.addRegular)(sid, event, delay);
    }

    // wrapper 목록·예약·공통 몸체·프레임·현재 공간 순서가 원본과 같다. (2026-11-08 WB04-E61)
    postPop(sid, flags) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        if (this.#kind(sid) === 2 && (flags & 1) && !(pool.slot(sid)[EXTRA_OFFSET] & ABSTRACT_BURIED)) {
            addUnique(this.#state.currentOutposts(), sid);
            addUnique(this.#state.currentResidences(), sid);
            need(hooks.regionOwnership)(position(pool.slot(sid), X_OFFSET), position(pool.slot(sid), Y_OFFSET));
        }
        if (flags & 1) {
            if (pool.slot(sid)[TYPE_OFFSET] === this.#numbers.readFirstTimer()) this.#regular(sid, 2, 8);
            if (pool.slot(sid)[TYPE_OFFSET] === this.#numbers.readSecondTimer()) this.#regular(sid, 1, 0.5);
            this.#regular(sid, 5, 0.5);
        }
        this.#damageable.damageablePostPop(sid, flags);
        this.adjustFrame(sid);
        if (!this.#state.readBoss()) return;
        const x = truncate(position(pool.slot(sid), X_OFFSET));
        const y = truncate(position(pool.slot(sid), Y_OFFSET));
        const finder = new RawSquidFinder(pool, this.#hash, this.#types);
        let candidate = finder.begin({ left: x, top: y, right: x, bottom: y });
        // 실제 finder 는 다음 raw 연결을 먼저 저장하므로 가상 삭제 뒤에도 원본 순서로 진행한다.
        while (candidate) {
            if (candidate !== sid && pool.slot(candidate)[TYPE_OFFSET] === pool.slot(sid)[TYPE_OFFSET]
                && !((pool.slot(candidate)[EXTRA_OFFSET] ^ pool.slot(sid)[EXTRA_OFFSET]) & SELECT_BIT)) {
                need(hooks.warnDuplicate)(candidate, truncate(position(pool.slot(candidate), X_OFFSET)), truncate(position(pool.slot(candidate), Y_OFFSET)));
                need(hooks.destroy)(candidate, 0);
            }
            candidate = finder.next();
        }
    }

    // 00475290: Factory 가상 +0x84 는 공통 Level 저장 뒤 같은 가상 +0x88 기본 표시 갱신을 호출한다. 표시 장치는 저장 후 호출 지점에서 요구하며 상태 프레임 보정과 구별한다. (2026-11-08 WB04-E61)
    setState(level, sid, value) {
        if (level.pool !== this.#pool || !this.handles(sid)) throw new TypeError('Factory State의 같은 Pool/가상 표 경계 불일치');
        level.set(sid, value);
        need(this.#hooks.updateState)(sid);
    }
}

// 동일 풀의 실제 프레임 지정/dirty·Unpop/Repop 몸체(SquidFrame)를 setFrame 으로 연결하고 나머지 효과는 호출자의 구현을 유지한다. (2026-11-08 WB04-E61)
export function makeFactoryPostPopHooks(pool, frame, hooks) {
    if (frame.pool !== pool) throw new TypeError('Factory 프레임 지정 풀 불일치');
    return { ...hooks, setFrame: (sid, value, flags) => frame.set(sid, value, flags) };
}
