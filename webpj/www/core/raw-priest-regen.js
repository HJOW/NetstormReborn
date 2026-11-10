// 사제 이벤트 0x25a(00494580)의 실제 HP 회복과 중립 후처리, 패치 전용 SP 추적 순서를 옮긴 모듈이다(cpppj `o/RawPriestRegen.cpp/.h`, 10.78 전용).
// 순서: 사제 TYPE/가상 표(0x50f210)를 확인 → 난수(9) 표본과 추적 sequence(+3, 9600 초과 시 한 번 -9600) 갱신 → 현재 HP/최대 HP/현재 사제 타입의 증분(E15 `readPriestRegenHitPoints`)이 있으면 HP 지정(절반 경계 공간 전환은 E62
//   `RawPriestState.setHitPoints`) → 표시 갱신 → Damageable 후처리 → 패치 감사 → 중립 사제 후처리(권한자이고 차단 전역 둘이 꺼져 있고 소유자 0 이며 현재 HP 가 최대 HP 절반 이상이고 점유가 없을 때 중립 행동) →
//   추적 확인: 관측 sequence 가 현재 sequence 와 다르거나 / 남은 측정이 있고 측정이 7000 을 넘거나 / 첫 분산 배열이 없거나 / 첫 순열 WORD 가 -1 이면 현재 표본을 변경 표본에 기록한다(남은 측정이 있으면 하나 줄임) → payload 를 그대로 반환한다.
// 하위 효과 뒤의 현재 소유자·HP·전역을 다시 읽는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E69: 사제 회복 이식으로 추가)

import { readPriestRegenHitPoints } from './raw-priest-regen-hit-points.js';
import { PRIEST_TYPE, PRIEST_VTABLE } from './raw-priest-owner.js';
import { PRIEST_REGEN_EVENT } from './raw-priest-post-pop.js';

// 추적 sequence 의 증분과 순환 상한, 측정 비교의 원본 상수, 슬롯의 TYPE/소유자 위치이다. (2026-11-08 WB04-E69)
const SEQUENCE_STEP = 3;
const SEQUENCE_LIMIT = 9600;
const MEASUREMENT_LIMIT = 7000.0;
const TYPE_OFFSET = 10;
const OWNER_OFFSET = 34;
const SAMPLE_LIMIT = 9;

// 정렬되지 않은 vtable DWORD 를 읽는다. (2026-11-08 WB04-E69)
function vtableOf(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}

// 원본 signed ADD 의 32비트 감김이다. (2026-11-08 WB04-E69)
const add = (first, second) => (first + second) | 0;

// 패치 사제 회복이 조회/갱신하는 추적 상태이다: sample(00540cb8; Next(9) 결과), tracking({value}; 0054d4e0 sequence 를 소유하는 holder 이며 SP 저장소와 같은 객체여야 함), observedSequence(독립 입력의 00568af8),
//   changedSample(005318ec), remaining(005c89c4), gate(독립 입력의 00594fd0), sentinel(독립 입력의 00531890 signed WORD)이다. 실제 Client 의 장기 표본/진단 DWORD 는 {value} holder 로 빌린다(sharedSample, sharedChangedSample). (2026-11-08 WB04-E69)
export class PriestRegenPatchState {
    sample = 0;
    tracking = { value: 0 };
    observedSequence = 0;
    changedSample = 0;
    remaining = 0;
    gate = 0;
    sentinel = 0;
    sharedSample = null;
    sharedChangedSample = null;

    // 추적 sequence(signed DWORD)를 holder 에서 읽고 쓴다. (2026-11-08 WB04-E69)
    get sequence() {
        return this.tracking.value | 0;
    }

    set sequence(value) {
        this.tracking.value = value | 0;
    }

    // 현재 표본 DWORD 를 읽는다(공유 holder 가 있으면 그 값). (2026-11-08 WB04-E69)
    readSample() {
        return (this.sharedSample ? this.sharedSample.value : this.sample) >>> 0;
    }

    // 표본 DWORD 를 쓴다(공유 holder 가 있으면 그쪽). (2026-11-08 WB04-E69)
    writeSample(value) {
        if (this.sharedSample) this.sharedSample.value = value >>> 0;
        else this.sample = value >>> 0;
    }

    // 변경 표본(진단) DWORD 를 읽는다(공유 holder 가 있으면 그 값). (2026-11-08 WB04-E69)
    readChangedSample() {
        return (this.sharedChangedSample ? this.sharedChangedSample.value : this.changedSample) >>> 0;
    }

    // 각 원문 store 위치에서 현재 연결된 변경 표본 DWORD 에 쓴다. (2026-11-08 WB04-E69)
    writeChangedSample(value) {
        if (this.sharedChangedSample) this.sharedChangedSample.value = value >>> 0;
        else this.changedSample = value >>> 0;
    }
}

// 사제 회복 처리 중 조회하는 상태이다: neutralBlocked(005c89b8 / 중립 후처리 차단), modeBlocked(00594fa4 / 두 번째 차단), patch(PriestRegenPatchState), 현재 사제 타입을 읽는 getter(sharedPriestType; 없으면 158)이다. (2026-11-08 WB04-E69)
export class PriestRegenState {
    neutralBlocked = false;
    modeBlocked = false;
    patch = new PriestRegenPatchState();
    sharedPriestType = null;

    // 증분마다 현재 TYPE 번호를 읽는다(실제 Actor 의 raw TYPE 와 구별). (2026-11-08 WB04-E69)
    priestType() {
        return this.sharedPriestType ? this.sharedPriestType() >>> 0 : PRIEST_TYPE;
    }
}

// SP 저장소의 현재값을 각 원문 읽기 지점에서 조회하는 묶음을 만든다. patch 를 주면 저장소의 추적 holder 가 patch.tracking 과 같은 객체여야 하고(다르면 거부), 주지 않으면 이미 초기화한 공통 시작의 장기 추적 holder 를
//   빌린다(Session 지역 sequence 로 재연결하지 않음). 반환 = { observedSequence, firstPool, firstPermutation, trackingSequence? }. (2026-11-08 WB04-E69)
export function bindPriestRegenSpStore(store, patch = null) {
    const holder = store.trackingOwner;
    if (patch && holder !== patch.tracking) throw new TypeError('사제 회복과 SP 저장소의 추적 주소 불일치');
    const reads = {
        observedSequence: () => store.offset | 0,
        firstPool: () => (store.firstPoolReady ? true : null),
        firstPermutation: () => store.permutation()[0]
    };
    if (!patch) reads.trackingSequence = () => holder;
    return reads;
}

// pool 은 10.78 SidPool, hp 는 같은 풀의 SquidReward, priest 는 RawPriestState, mode 는 PriestHitPointMode, space = { unpop, repop }, random 은 GameRandom, state 는 PriestRegenState,
//   hooks = { refresh(sid), damageable(sid), shape(타입)→수, occupied(sid, shape)→bool, neutralAction(sid), patchAudit(), measurement()→수, spReads? } 이다. (2026-11-08 WB04-E69)
export class RawPriestRegen {
    // 객체 슬롯 풀, HP 모듈, 사제 상태, 모드, 공간 효과, 난수, 상태, 하위 효과이다.
    #pool;
    #hp;
    #priest;
    #mode;
    #space;
    #random;
    #state;
    #hooks;

    // 미복원 효과를 빠뜨린 연결과 서로 다른 풀을 실행 전에 거부한다. (2026-11-08 WB04-E69)
    constructor(pool, hp, priest, mode, space, random, state, hooks) {
        const reads = hooks?.spReads;
        if (pool?.edition !== 'patch1078' || hp?.pool !== pool || priest?.pool !== pool || typeof space?.unpop !== 'function' || typeof space.repop !== 'function'
            || ['refresh', 'damageable', 'shape', 'occupied', 'neutralAction', 'patchAudit', 'measurement'].some((name) => typeof hooks?.[name] !== 'function')
            || (reads && (typeof reads.observedSequence !== 'function' || typeof reads.firstPool !== 'function' || typeof reads.firstPermutation !== 'function'))) {
            throw new TypeError('사제 회복 풀/외부 효과 연결 오류');
        }
        this.#pool = pool;
        this.#hp = hp;
        this.#priest = priest;
        this.#mode = mode;
        this.#space = space;
        this.#random = random;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 분배기의 동일 풀 보호에 쓴다. (2026-11-08 WB04-E69)
    get pool() {
        return this.#pool;
    }

    // 00494580 의 회복 분기를 실행하며 다른 이벤트를 회복으로 오인하지 않는다. count 는 원본도 읽지 않으며 반환 payload 비트를 유지한다. (2026-11-08 WB04-E69)
    handle(sid, event, count, payload) {
        void count;
        if (event !== PRIEST_REGEN_EVENT) throw new RangeError('미복원 사제 이벤트');
        const raw = this.#pool.slot(sid);
        // 원본 가상 진입은 현재 TYPE BYTE 가 158 인지 별도 검사하지 않고 가상 표만 본다. (2026-11-10 WB04-E260: cpppj 기준선 뒤 PriestRegen 재반영)
        if (vtableOf(raw) !== PRIEST_VTABLE) throw new TypeError('사제 회복 가상 표 오류');
        const hooks = this.#hooks;
        const patch = this.#state.patch;
        const reads = hooks.spReads;
        patch.writeSample(this.#random.next(SAMPLE_LIMIT));
        // 실제 사제/통지/SP 가 공유하는 현재 추적 DWORD 이다.
        const holder = reads?.trackingSequence ? reads.trackingSequence() : patch.tracking;
        holder.value = add(holder.value | 0, SEQUENCE_STEP);
        if ((holder.value | 0) > SEQUENCE_LIMIT) holder.value = add(holder.value | 0, -SEQUENCE_LIMIT);
        const value = readPriestRegenHitPoints({
            current: () => this.#priest.currentHitPoints(sid),
            maximum: () => this.#hp.maxHitPoints(sid),
            priestType: () => this.#state.priestType(),
            typeMaximum: (type) => this.#hp.typeHitPoints(type)
        });
        if (value !== null) {
            this.#priest.setHitPoints(sid, value, this.#mode, this.#space);
            hooks.refresh(sid);
            hooks.damageable(sid);
        }
        hooks.patchAudit();
        // HP setter/외부 효과가 바꾼 현재 소유자·HP·전역을 이 시점에 다시 읽는다.
        const owner = this.#pool.slot(sid)[OWNER_OFFSET];
        if (this.#mode.isAuthority() && !this.#state.neutralBlocked && owner === 0 && !this.#state.modeBlocked
            && this.#priest.currentHitPoints(sid) >= Math.trunc(this.#hp.maxHitPoints(sid) / 2)) {
            const shape = hooks.shape(this.#pool.slot(sid)[TYPE_OFFSET]);
            if (!hooks.occupied(sid, shape)) hooks.neutralAction(sid);
        }
        const observed = reads ? reads.observedSequence() | 0 : patch.observedSequence | 0;
        // 중립/외부 효과 뒤 같은 현재 추적 전역을 다시 읽는다.
        const current = reads?.trackingSequence ? reads.trackingSequence().value | 0 : patch.sequence;
        if (observed !== current) patch.writeChangedSample(patch.readSample());
        if (patch.remaining !== 0) {
            if (hooks.measurement() > MEASUREMENT_LIMIT) patch.writeChangedSample(patch.readSample());
            patch.remaining = (patch.remaining - 1) >>> 0;
        }
        // 원본은 두 독립 if 다. 첫 배열이 NULL 이어도 뒤의 signed WORD 를 반드시 현재 읽는다.
        if (reads ? !reads.firstPool() : patch.gate === 0) patch.writeChangedSample(patch.readSample());
        if ((reads ? reads.firstPermutation() : patch.sentinel) === -1) patch.writeChangedSample(patch.readSample());
        return payload;
    }
}

// 실제 사제 vtable 의 0x25a 만 연결한다. 다른 사제 이벤트는 fallback 이 없으면 미복원 예외이고, 비사제는 fallback 또는 payload 반환으로 넘긴다. (2026-11-08 WB04-E69)
export function makePriestRegenHandler(pool, regen, fallback = null) {
    if (regen.pool !== pool) throw new TypeError('사제 회복 분배 풀 불일치');
    return (sid, event, count, payload) => {
        const priest = vtableOf(pool.slot(sid)) === PRIEST_VTABLE;
        if (priest && event === PRIEST_REGEN_EVENT) return regen.handle(sid, event, count, payload);
        if (fallback) return fallback(sid, event, count, payload);
        if (priest) throw new RangeError('미복원 사제 이벤트 분배');
        return payload;
    };
}
