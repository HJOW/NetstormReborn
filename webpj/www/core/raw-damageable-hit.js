// 일반 자산 피격 두 몸체(BaseHit 004add10·Damageable Hit 0044c8e0)의 HP/사망/섬 피격 경고 순서와 현재 vtable 분배를 옮긴 모듈이다(cpppj `o/RawDamageableHit.cpp/.h`, 10.78 전용).
//  - new DamageableHitState(levels): levels 는 사제 피격과 공유하는 PriestDamageState(`raw-priest-damage.js`, 무적·레벨 보정)다. 그 밖의 전역은 server(사망 권한)·checking(005e4794 전체 DWORD, sharedChecking getter 로 빌림)·
//    localOwner(현재 local Player BYTE)·primaryIslands(Player+0x78 주 섬 번호 40칸)·warningTime(실제 시계 기준 경고 마감 double)·warningDelay(float 30초)·killedVictim/killerOwner(서버 사망 직전 기록 두 DWORD)와
//    현재 저장소를 읽는 serverRead/localOwnerRead/primaryIslandRead getter(없으면 위 값)다.
//  - new RawDamageableHit(pool, types, state, hooks): pool 은 10.78 SidPool, types 는 188칸 TYPE 표({ flags1, flags2 }), hooks = { rejectDamage(victim, attacker)→bool(가상+0x9c), destroy(sid, flags)(가상+0x10),
//    ownerDamageNotice(owner, victim, attacker), refresh(victim)(burning 갱신), unrealize(victim)(가상+0x88), islandAt(x, y)→지면 SID, realTime()→실제 초, warningEvent()(event 12), warningSound()(islandUnderAttack.wav),
//    warningText()(islandUnderAttack 알림) } 이며 도달한 것만 필요하다(누락은 묵시 성공이 아니라 오류).
//    · baseHit(victim, damage, attackerOwner, attacker): dead 는 원본 assert, free 면 참 → extra(+40) 비트 0/3 이 켜지면(held) checking 일 때 assert, 아니면 참 → 무적이면 피해 0 → 가상 방어가 거부하면 참 →
//      레벨 보정 피해를 현재 signed DWORD HP 에서 감김 뺄셈 → 1 미만이고 서버면 사망 전역 기록 뒤 destroy(공격자 소유자<<16 | 0x100000, 공격자 TYPE flags2 0x100 이면 | 0x4000000) 하고 거짓 →
//      아니면 HP 를 최소 1 로 쓰고 현재 소유자에게 통지한 뒤 참.
//    · hit(victim, damage, attackerOwner, attacker): HP 보유(다이스 genus 0x400000 은 extra 비트 0, 그 밖은 TYPE flags1 0x10)가 아니면 거짓 → local 소유자의 주 섬 피격 경고 → extra 비트 2 면 unrealize → baseHit → 살아 있으면 burning 갱신.
//    · 섬 경고: 피해자 소유자가 local 이 아니면 생략. 현재 좌표의 지면이 섬 genus(2)이고 3×3 섬(0x1000000)이 아니며 번호(다리 genus 4 면 −1, 아니면 +8 WORD)가 소유자의 주 섬이면, 경고 마감 ≤ 실제 시각일 때
//      event 12 → 소리 → 알림 순서로 내고, 실제 시각을 다시 읽어 마감 = 시각 + float 간격으로 저장한다.
//  - makeDamageableGunHitHooks(damage, entries, hooks): 현재 raw vtable 이 entries 의 실제 +0x98 진입(0x44c8e0 Hit / 0x4add10 BaseHit)과 같으면 그 몸체로, 아니면 hooks.damage(fallback, 없으면 오류)로 보낸다.
//  - makeDamageableHitBurningHooks(pool, burning, hooks): 같은 풀의 RawDamageableBurning.refresh 를 refresh 에 연결한다.
// CD 10.72 분기(signed WORD HP·별도 오프셋)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E210: 일반 자산 피격 이식으로 추가)
import { RawPriestDamage } from './raw-priest-damage.js';
import { setRawSquidHitPoints } from './raw-squid-word.js';

// 슬롯의 vtable·지면 번호·TYPE·상태·좌표·HP·소유자·extra 위치이다. (2026-11-09 WB04-E210)
const VTABLE_AT = 0;
const ISLAND_ID_AT = 8;
const TYPE_AT = 10;
const STATE_AT = 11;
const X_AT = 14;
const Y_AT = 18;
const HEALTH_AT = 26;
const OWNER_AT = 34;
const EXTRA_AT = 40;
// 다이스 genus·HAS_HP·섬·3×3 섬·다리 genus 비트와 유닛 공격 비트, 사망 삭제 flags 이다. (2026-11-09 WB04-E210)
const DAIS = 0x400000;
const HAS_HP = 0x10;
const ISLAND = 2;
const ISLAND_3X3 = 0x1000000;
const BRIDGE = 4;
const UNIT_ATTACKER = 0x100;
const DEATH_FLAGS = 0x100000;
const UNIT_DEATH_FLAGS = 0x4100000;
// 10.78 TYPE 표 크기, 주 섬 표 크기, 실제 +0x98 진입 주소 두 개이다. (2026-11-09 WB04-E210)
const TYPE_COUNT = 188;
const PLAYER_SLOTS = 40;
const HIT_ENTRY = 0x44c8e0;
const BASE_HIT_ENTRY = 0x4add10;

// float 비트 변환용 공용 보기이다. (2026-11-09 WB04-E210)
const FLOAT_VIEW = new DataView(new ArrayBuffer(4));

// 비정렬 DWORD 를 부호 없이 읽는다. (2026-11-09 WB04-E210)
function dwordAt(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// DWORD 비트를 float 로 읽는다. (2026-11-09 WB04-E210)
function floatAt(raw, offset) {
    FLOAT_VIEW.setUint32(0, dwordAt(raw, offset), true);
    return FLOAT_VIEW.getFloat32(0, true);
}

// 일반 피격이 매번 읽는 전역이다. levels 는 사제 피격과 공유한다. (2026-11-09 WB04-E210)
export class DamageableHitState {
    // 공유 PriestDamageState 를 받는다. (2026-11-09 WB04-E210)
    constructor(levels) {
        if (!levels || typeof levels.isInvulnerable !== 'function') throw new TypeError('일반 피해 레벨 상태 누락');
        this.levels = levels;
    }
    // 서버 사망 권한과 패치 held 피격 진단 전역(전체 DWORD, sharedChecking 이 있으면 그것을 읽음)이다. (2026-11-09 WB04-E210)
    server = true;
    checking = 0;
    sharedChecking = null;
    // 현재 local Player BYTE 와 Player 별 주 섬 번호이다. (2026-11-09 WB04-E210)
    localOwner = 1;
    primaryIslands = new Array(PLAYER_SLOTS).fill(0);
    // 실제 시계 기준 경고 마감(double)과 float 30초 간격이다. (2026-11-09 WB04-E210)
    warningTime = 0;
    warningDelay = Math.fround(30);
    // 서버 사망 직전에 기록하는 피해자 SID 와 공격자 소유자이다. (2026-11-09 WB04-E210)
    killedVictim = 0;
    killerOwner = 0;
    // 세션의 현재 원본 저장소를 읽는 getter 이다. 없으면 위 독립 값을 쓴다. (2026-11-09 WB04-E210)
    serverRead = null;
    localOwnerRead = null;
    primaryIslandRead = null;

    // 사망 권한을 호출 시 읽는다. (2026-11-09 WB04-E210)
    isServer() { return this.serverRead ? Boolean(this.serverRead()) : this.server; }
    // 진단 전역을 비영으로만 판정한다(원래 저장소는 고치지 않는다). (2026-11-09 WB04-E210)
    isChecking() { return (this.sharedChecking ? this.sharedChecking() >>> 0 : this.checking >>> 0) !== 0; }
    // 현재 local Player BYTE 이다. (2026-11-09 WB04-E210)
    currentLocalOwner() { return this.localOwnerRead ? this.localOwnerRead() & 0xff : this.localOwner & 0xff; }
    // 소유자의 현재 주 섬 번호(signed)이다. (2026-11-09 WB04-E210)
    primaryIsland(owner) { return this.primaryIslandRead ? this.primaryIslandRead(owner) | 0 : this.primaryIslands[owner] | 0; }
}

// 일반 피해 몸체이다. 효과 뒤 현재 raw 를 다시 읽는다. (2026-11-09 WB04-E210)
export class RawDamageableHit {
    // 같은 풀, 현재 TYPE 표, 전역, 하위 효과이다.
    #pool;
    #types;
    #state;
    #hooks;

    // 같은 판본의 현재 타입/전역만 연결한다. (2026-11-09 WB04-E210)
    constructor(pool, types, state, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT || !(state instanceof DamageableHitState)) throw new TypeError('일반 피해 타입 판본 오류');
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 같은 풀 연결을 검사하는 풀이다. (2026-11-09 WB04-E210)
    get pool() { return this.#pool; }

    // 현재 raw SID/TYPE 범위를 검사한 사본이다. free/dead 는 각 몸체가 판정한다. (2026-11-09 WB04-E210)
    #slot(sid) {
        if (!sid || sid >= this.#pool.capacity) throw new RangeError('일반 피해 SID 오류');
        const raw = this.#pool.slot(sid);
        if (raw[TYPE_AT] >= this.#types.length) throw new RangeError('일반 피해 타입 오류');
        return raw;
    }

    // 현재 HP 의 signed DWORD 이다. (2026-11-09 WB04-E210)
    #health(victim) { return dwordAt(this.#slot(victim), HEALTH_AT) | 0; }

    // 다이스 genus 는 extra 비트 0, 그 밖은 TYPE_HAS_HP 비트이다. (2026-11-09 WB04-E210)
    #hasHitPoints(victim) {
        const raw = this.#slot(victim);
        const type = this.#types[raw[TYPE_AT]];
        return (type.flags2 & DAIS) ? (raw[EXTRA_AT] & 1) !== 0 : (type.flags1 & HAS_HP) !== 0;
    }

    // 현재 local Player 주 섬에 대한 경고만 내고 원본 double 시계+float 간격 저장 순서를 지킨다. (2026-11-09 WB04-E210)
    #warnOwner(victim) {
        const hooks = this.#hooks;
        const state = this.#state;
        const raw = this.#slot(victim);
        const owner = raw[OWNER_AT];
        if (owner !== state.currentLocalOwner()) return;
        if (owner >= state.primaryIslands.length || typeof hooks.islandAt !== 'function') throw new Error('일반 피해 실제 주 섬 조회 경계 누락');
        const island = hooks.islandAt(floatAt(raw, X_AT), floatAt(raw, Y_AT));
        if (!island) return;
        const ground = this.#slot(island);
        const genus = this.#types[ground[TYPE_AT]].flags2;
        if (!(genus & ISLAND) || (genus & ISLAND_3X3)) return;
        const islandId = (genus & BRIDGE) ? -1 : ground[ISLAND_ID_AT] | (ground[ISLAND_ID_AT + 1] << 8);
        if (islandId !== state.primaryIsland(owner)) return;
        if (typeof hooks.realTime !== 'function') throw new Error('일반 피해 실제 경고 시계 누락');
        if (state.warningTime <= hooks.realTime()) {
            if (typeof hooks.warningSound !== 'function' || typeof hooks.warningEvent !== 'function' || typeof hooks.warningText !== 'function') throw new Error('일반 피해 실제 섬 피격 알림 경계 누락');
            hooks.warningEvent();
            hooks.warningSound();
            hooks.warningText();
        }
        const now = hooks.realTime();
        state.warningTime = now + state.warningDelay;
    }

    // 004add10: 방어/레벨/HP/서버 사망 또는 HP 최소 1·소유자 통지이다. 반환은 살아 있으면 참이다. (2026-11-09 WB04-E210)
    baseHit(victim, damage, attackerOwner, attacker) {
        const hooks = this.#hooks;
        const state = this.#state;
        const raw = this.#slot(victim);
        if (raw[STATE_AT] & 2) throw new Error('일반 피해 dead 원본 assert');
        if (raw[STATE_AT] & 3) return true;
        if (raw[EXTRA_AT] & 9) {
            if (state.isChecking()) throw new Error('일반 피해 held 원본 assert');
            return true;
        }
        if (typeof hooks.rejectDamage !== 'function') throw new Error('일반 피해 실제 가상 방어 판정 누락');
        let amount = damage | 0;
        if (state.levels.isInvulnerable()) amount = 0;
        if (hooks.rejectDamage(victim, attacker)) return true;
        const owner = this.#slot(victim)[OWNER_AT];
        amount = RawPriestDamage.adjust(amount, attackerOwner >>> 0, owner, state.levels);
        const next = (this.#health(victim) - amount) | 0;
        if (next < 1 && state.isServer()) {
            if (typeof hooks.destroy !== 'function') throw new Error('일반 피해 실제 서버 삭제 경계 누락');
            state.killedVictim = victim;
            state.killerOwner = attackerOwner >>> 0;
            const ownerMask = attackerOwner ? (attackerOwner << 16) >>> 0 : 0;
            let flags = (ownerMask | DEATH_FLAGS) >>> 0;
            if (attacker && (this.#types[this.#slot(attacker)[TYPE_AT]].flags2 & UNIT_ATTACKER)) flags = (ownerMask | UNIT_DEATH_FLAGS) >>> 0;
            hooks.destroy(victim, flags);
            return false;
        }
        if (typeof hooks.ownerDamageNotice !== 'function') throw new Error('일반 피해 실제 현재 owner 통지 누락');
        setRawSquidHitPoints(this.#pool, victim, next < 1 ? 1 : next);
        hooks.ownerDamageNotice(this.#slot(victim)[OWNER_AT], victim, attacker);
        return true;
    }

    // 0044c8e0: HP 보유 판정·local 섬 경고·unrealize·baseHit·burning 갱신 순서이다. (2026-11-09 WB04-E210)
    hit(victim, damage, attackerOwner, attacker) {
        if (!this.#hasHitPoints(victim)) return false;
        this.#warnOwner(victim);
        if (this.#slot(victim)[EXTRA_AT] & 4) {
            if (typeof this.#hooks.unrealize !== 'function') throw new Error('일반 피해 실제 unrealize 경계 누락');
            this.#hooks.unrealize(victim);
        }
        const alive = this.baseHit(victim, damage, attackerOwner, attacker);
        if (alive) {
            if (typeof this.#hooks.refresh !== 'function') throw new Error('일반 피해 실제 burning 몸체 누락');
            this.#hooks.refresh(victim);
        }
        return alive;
    }
}

// 현재 raw vtable 의 실제 +0x98 진입 주소로 Base/Damageable 을 분배하고 다른 파생 피해는 기존 hooks.damage 로 넘긴다. entries 는 [{ vtable, address }]. (2026-11-09 WB04-E210)
export function makeDamageableGunHitHooks(damage, entries, hooks = {}) {
    const pool = damage.pool;
    if (!Array.isArray(entries) || entries.length === 0) throw new TypeError('일반 피해 실제 가상 진입 표 오류');
    const table = entries.map((item) => ({ vtable: item.vtable >>> 0, address: item.address >>> 0 }));
    const fallback = hooks.damage;
    return {
        ...hooks,
        damage: (victim, amount, owner, attacker) => {
            if (!victim || victim >= pool.capacity) throw new RangeError('일반 피해 분배 SID 오류');
            const vtable = dwordAt(pool.slot(victim), VTABLE_AT);
            let entry = 0;
            // raw 의 현재 vtable 과 일치하는 첫 실제 진입만 쓴다.
            for (const item of table) {
                if (item.vtable === vtable) { entry = item.address; break; }
            }
            if (entry === HIT_ENTRY) { damage.hit(victim, amount, owner, attacker); return; }
            if (entry === BASE_HIT_ENTRY) { damage.baseHit(victim, amount, owner, attacker); return; }
            if (typeof fallback !== 'function') throw new Error('다른 파생 실제 피해 미복원');
            fallback(victim, amount, owner, attacker);
        }
    };
}

// 같은 풀의 burning 판정 몸체를 일반 피격 refresh 에 연결한다. burning 은 callback 보다 오래 살아야 한다. (2026-11-09 WB04-E210)
export function makeDamageableHitBurningHooks(pool, burning, hooks = {}) {
    if (pool !== burning?.pool) throw new TypeError('일반 피해/burning 실제 풀 불일치');
    return { ...hooks, refresh: (victim) => { burning.refresh(victim); } };
}
