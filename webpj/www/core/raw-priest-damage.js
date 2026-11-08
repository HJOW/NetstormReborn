// 사제 피격(가상 +0x98: 00494380)의 회복 취소·HP 1 제한·소유자 통지·HP 1 감시 해제와 기본 피해 레벨 보정(0044bfe0)을 옮긴 모듈이다(cpppj `o/RawPriestDamage.cpp/.h`, 10.78 전용).
//  - adjust(피해, 공격자 소유자, 피해자 소유자, 상태): 보정 차단 전역이 비영이거나 어느 소유자든 0 이면 피해를 그대로 두고, 아니면 Challenge rank BYTE 를 24칸 표로 실제 레벨로 바꾼 뒤(소유자 1..8 밖은 0) 피해자 레벨이 낮으면
//    `(공격자-피해자+20)*피해/20`, 높으면 `피해*20/(피해자-공격자+20)` 를 DWORD 곱셈 감김과 0 방향 나눗셈으로 계산한다. 나눗셈 0/최소값÷-1 은 거부한다.
//  - hit(피해자, 피해, 공격자 소유자, 공격자): dead 는 원본 assert 로 거부, 그 밖에 live 몸체의 무적 전역이 비영이면 피해 0 → 현재 회복 사건(0x25d)이 있거나 프레임 방향이 10..14 이면 0x25c/0x25d 를 모두 취소하고 기본 프레임 지정
//    → 보정 피해를 현재 HP 에서 빼서 최소 1 로 HP 지정(공간 전환 포함) → 현재 소유자에게 통지 → 표시/burning 갱신 → 현재 권한자이고 HP 가 1 이면 의존 감시 체인을 저장된 다음 순서로 순회하며 감시 쌍을 통지/제거하고 거짓을 반환한다.
// 효과 뒤 현재 필드는 다시 읽는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E67: 사제 피격 이식으로 추가)

import { pathDirection } from './raw-path-animation.js';
import { PRIEST_TYPE } from './raw-priest-owner.js';

// 슬롯의 HP/의존 체인 머리/다음 위치, 상태/TYPE/소유자/프레임 위치, 타입 표 크기이다. (2026-11-08 WB04-E67)
const HEALTH_OFFSET = 26;
const HEAD_OFFSET = 6;
const NEXT_OFFSET = 4;
const STATE_OFFSET = 11;
const TYPE_OFFSET = 10;
const OWNER_OFFSET = 34;
const FRAME_OFFSET = 36;
const TYPE_COUNT = 188;
const PRIEST_PLAYER_MAX = 8;
const SCALE = 20;
const PRIEST_REGEN_CANCEL_EVENTS = [0x25c, 0x25d];

// 패치 0041c5b0 의 Challenge rank BYTE 를 실제 피해 레벨로 바꾸는 24칸 표이다. (2026-11-08 WB04-E67)
const RANK_LEVELS = Object.freeze([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 35, 40, 45, 50, 60, 70, 80, 90, 100]);

// 사제 피격이 읽는 전역이다: invulnerable(55a48c 의 전체 DWORD 와 실제 Client 의 같은 값을 빌리는 sharedInvulnerable getter), boss 권한(sharedBoss getter), 보정 차단(levelCorrectionDisabled 와 호출 시 읽는 Read getter),
//   소유자 번호별 Challenge rank 를 공급하는 40칸 levels 이다. (2026-11-08 WB04-E67)
export class PriestDamageState {
    invulnerable = 0;
    sharedInvulnerable = null;
    boss = true;
    sharedBoss = null;
    levelCorrectionDisabled = false;
    levelCorrectionDisabledRead = null;
    levels = new Array(40).fill(0);

    // 효과 뒤 현재 무적 비트를 다시 읽는다(비영만 켜짐). (2026-11-08 WB04-E67)
    isInvulnerable() {
        return (this.sharedInvulnerable ? this.sharedInvulnerable() >>> 0 : this.invulnerable >>> 0) !== 0;
    }

    // 피격/소유자/표시 효과 뒤 HP 1 감시 해제 관문에서 현재 권한을 읽는다. (2026-11-08 WB04-E67)
    isBoss() {
        return this.sharedBoss ? Boolean(this.sharedBoss()) : this.boss;
    }

    // 레벨 피해 보정을 생략하는 전역을 호출 시 읽는다. (2026-11-08 WB04-E67)
    isCorrectionDisabled() {
        return this.levelCorrectionDisabledRead ? Boolean(this.levelCorrectionDisabledRead()) : this.levelCorrectionDisabled;
    }
}

// 정렬되지 않은 슬롯 필드를 리틀엔디언으로 읽는다. (2026-11-08 WB04-E67)
function readField(raw, offset, width) {
    let value = 0;
    // 요청한 폭의 낮은 바이트부터 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 원본 DWORD 곱셈의 하위 32비트를 signed 로 읽는다. (2026-11-08 WB04-E67)
const multiply = (first, second) => Math.imul(first, second);

// 0 방향 정수 나눗셈이며 음의 0 을 만들지 않는다. (2026-11-08 WB04-E67)
const divide = (first, second) => Math.trunc(first / second) | 0;

// pool 은 10.78 SidPool, types 는 188칸 TYPE 표(길이 검사만), frames 는 같은 길이의 FrameCodeTable 표(방향 판정), state 는 PriestDamageState,
//   hooks = { hasRegenEvent(sid), cancelEvent(sid, 사건), defaultFrame(sid), setFrame(sid, 프레임, flags), setHitPoints(sid, 값), ownerDamageNotice(소유자, sid, 공격자), damageableRefresh(sid) } 이며 도달한 것만 필요하다,
//   depend 는 HP 1 감시가 있을 때 필요한 DependFormHost 이다. (2026-11-08 WB04-E67)
export class RawPriestDamage {
    // 현재 사제/프로세스/감시쌍을 공유하는 풀, 타입/프레임 표, 전역, 하위 효과, 감시 호스트이다.
    #pool;
    #types;
    #frames;
    #state;
    #hooks;
    #depend;

    // 같은 월드의 판본/프레임/감시 저장소를 구성 때 검사한다. (2026-11-08 WB04-E67)
    constructor(pool, types, frames, state, hooks, depend = null) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(frames) || frames.length !== types.length || (depend && depend.pool !== pool)) {
            throw new TypeError('Priest 피해 타입/프레임/감시 풀 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#state = state;
        this.#hooks = hooks ?? {};
        this.#depend = depend;
    }

    // 같은 풀의 GunHit 소비자만 연결하는 풀이다. (2026-11-08 WB04-E67)
    get pool() {
        return this.#pool;
    }

    // 0044bfe0: 패치 레벨차 + 20 을 기준으로 정수 피해를 보정한다. 플레이어 0 이거나 보정 차단이면 피해를 그대로 돌려준다. (2026-11-08 WB04-E67)
    static adjust(damage, attackerOwner, victimOwner, state) {
        if (state.isCorrectionDisabled() || !attackerOwner || !victimOwner) return damage | 0;
        // 패치 getter 는 실제 플레이어 1..8 밖의 소유자에 레벨 0 을 반환한다.
        const level = (owner) => {
            if (owner > PRIEST_PLAYER_MAX) return 0;
            const rank = state.levels[owner] & 0xff;
            if (rank >= RANK_LEVELS.length) throw new RangeError('피해 Challenge rank 범위 오류');
            return RANK_LEVELS[rank];
        };
        const attacker = level(attackerOwner);
        const victim = level(victimOwner);
        if (victim < attacker) return divide(multiply(attacker - victim + SCALE, damage), SCALE);
        if (attacker < victim) {
            const divisor = victim - attacker + SCALE;
            if (!divisor) throw new RangeError('피해 레벨 나눗셈 범위 오류');
            const product = multiply(damage, SCALE);
            if (product === -2147483648 && divisor === -1) throw new RangeError('피해 레벨 나눗셈 overflow');
            return divide(product, divisor);
        }
        return damage | 0;
    }

    // 현재 HP DWORD 를 signed 로 읽는다(HP 쓰기 뒤 재조회에 쓴다). (2026-11-08 WB04-E67)
    #health(victim) {
        return readField(this.#pool.slot(victim), HEALTH_OFFSET, 4) | 0;
    }

    // 현재 감시 대상 쪽 Depend Form 체인을 저장된 다음 순서로 순회하며 통지/감시 제거를 수행한다. 같은 호스트가 통지에서 현재 쌍을 없애도 먼저 저장한 다음 항목으로 계속한다. (2026-11-08 WB04-E67)
    #dropDependents(victim) {
        let current = readField(this.#pool.slot(victim), HEAD_OFFSET, 2);
        const visited = new Set();
        // 원본 FormFinder 처럼 현재 후보를 처리하기 전에 다음 SID 를 저장한다.
        while (current) {
            if (current >= this.#pool.capacity || visited.has(current)) throw new Error('Priest 피해 의존 체인 오류');
            visited.add(current);
            const raw = this.#pool.slot(current);
            const next = readField(raw, NEXT_OFFSET, 2);
            if (raw[TYPE_OFFSET] === 1) {
                if (!this.#depend) throw new Error('Priest HP1 실제 감시 통지 Host 누락');
                this.#depend.notify(current, victim);
                if (!(this.#pool.slot(current)[STATE_OFFSET] & 1)) this.#depend.remove(this.#depend.watcher(current), victim, 0);
            }
            current = next;
        }
    }

    // 00494380: live 몸체의 무적·회복 취소·HP 1 제한·소유자/burning·감시 해제 순서이다. 반환은 HP 1 사제의 감시를 해제했으면 거짓이다. (2026-11-08 WB04-E67)
    hit(victim, damage, attackerOwner, attacker) {
        if (!victim || victim >= this.#pool.capacity) throw new RangeError('Priest 피해 SID 오류');
        const hooks = this.#hooks;
        // 하위 효과 전후의 현재 필드를 다시 읽을 수 있도록 live 슬롯을 본다.
        const raw = this.#pool.physical(victim);
        if (raw[STATE_OFFSET] & 2) throw new Error('Priest 피해 dead 원본 assert');
        if ((raw[STATE_OFFSET] & 1) || raw[TYPE_OFFSET] >= this.#types.length) throw new Error('Priest 피해 live/타입 계약 오류');
        if (typeof hooks.hasRegenEvent !== 'function' || typeof hooks.setHitPoints !== 'function' || typeof hooks.ownerDamageNotice !== 'function' || typeof hooks.damageableRefresh !== 'function') {
            throw new Error('Priest 실제 피격 하위 몸체 누락');
        }
        let applied = damage | 0;
        if (this.#state.isInvulnerable()) applied = 0;
        let cancel = Boolean(hooks.hasRegenEvent(victim));
        if (!cancel) {
            const frame = readField(raw, FRAME_OFFSET, 4) | 0;
            const direction = pathDirection(this.#frames[raw[TYPE_OFFSET]], frame);
            cancel = direction > 9 && direction < 15;
        }
        if (cancel) {
            if (typeof hooks.cancelEvent !== 'function' || typeof hooks.defaultFrame !== 'function' || typeof hooks.setFrame !== 'function') throw new Error('Priest 실제 회복 취소/프레임 경계 누락');
            // 회복 사건 두 종류를 모두 취소한 뒤 기본 프레임으로 되돌린다.
            for (const event of PRIEST_REGEN_CANCEL_EVENTS) hooks.cancelEvent(victim, event);
            hooks.setFrame(victim, hooks.defaultFrame(victim), 0);
        }
        applied = RawPriestDamage.adjust(applied, attackerOwner, this.#pool.slot(victim)[OWNER_OFFSET], this.#state);
        const next = (this.#health(victim) - applied) | 0;
        hooks.setHitPoints(victim, next < 1 ? 1 : next);
        hooks.ownerDamageNotice(this.#pool.slot(victim)[OWNER_OFFSET], victim, attacker);
        hooks.damageableRefresh(victim);
        if (this.#state.isBoss() && this.#health(victim) === 1) {
            this.#dropDependents(victim);
            return false;
        }
        return true;
    }
}

// 실제 사제 표적의 가상 피해를 GunHit 콜백에 연결하며 다른 표적은 명시 fallback 에 넘긴다(없으면 거부). hooks 의 나머지 필드는 보존한다. (2026-11-08 WB04-E67)
export function makePriestGunHitHooks(pool, priest, hooks, fallback = null) {
    if (priest.pool !== pool) throw new TypeError('Gun/Priest 피해 SID 풀 불일치');
    return {
        ...hooks,
        damage: (victim, damage, owner, attacker) => {
            if (pool.slot(victim)[TYPE_OFFSET] === PRIEST_TYPE) {
                priest.hit(victim, damage, owner, attacker);
                return;
            }
            if (!fallback) throw new Error('비사제 실제 피해 분배 미복원');
            fallback(victim, damage, owner, attacker);
        }
    };
}

// 회복 프로세스 삭제와 HP 경계 전환, burning 갱신을 같은 실제 원본 저장소에 연결한다. host 는 SquidProcessHost, hp 는 RawPriestState, mode 는 PriestHitPointMode,
//   space = { unpop, repop }, burning 은 RawDamageableBurning 이다. (2026-11-08 WB04-E67)
export function makePriestDamageWorldHooks(pool, host, hp, mode, space, burning, hooks) {
    if (host.pool !== pool || hp.pool !== pool || burning.pool !== pool) throw new TypeError('Priest 피해 실제월드 풀 불일치');
    return {
        ...hooks,
        hasRegenEvent: (victim) => host.findEvent(victim, 0x25d) !== null,
        // 원본 cancel helper 처럼 첫 현재 일치 Regular 를 반복 조회하며 실제 Form/Kernel 을 삭제한다.
        cancelEvent: (victim, event) => {
            for (let process = host.findEvent(victim, event); process; process = host.findEvent(victim, event)) host.kill(process, 0);
        },
        setHitPoints: (victim, value) => { hp.setHitPoints(victim, value, mode, space); },
        damageableRefresh: (victim) => { burning.refresh(victim); }
    };
}
