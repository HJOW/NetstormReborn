// Unit.cpp 의 압축 총 피해 시각(004d4da0/004d4e10)과 현재 표적의 실제 가상 피해 전달(004d4eb0/004d4f00)을 옮긴 모듈이다(cpppj `o/RawGunHit.cpp/.h`, 10.78 전용).
// 총의 Q16 WORD(raw+12)를 1/8초 시각으로 사용하며 실제 현재 표적 피해를 계산한다. 원본 packed 시각의 감김·절삭·DPS 정수화를 유지하고 피해 처리기·공간 통지는 훅이다.
//  - time(firing): raw+12 WORD 의 bit0(건물 출력 상태)을 제외한 15비트 압축 시각을 `정수초 + (1/8초)×0.125` 로 읽는다. setTime(firing, now): 현재 시각을 정수초×8 | (소수×8 절삭) 로 압축해 WORD 의 상위 15비트에 쓰고 bit0 을 보존한 뒤
//    공간 통지(notifySurface)를 부른다(시각/피해 절삭은 유한 int32 범위만 허용). 발사 SID 는 0 이 아니고 범위 안이며 TYPE 이 비행(flags2 0x100000)이 아니면 group 0/1 이어야 한다(원본 assert).
//  - apply(firing, target, damage): 실제 source+c0 전용 진입(applyOverride)이 처리하면 끝낸다. 아니면 표적 0 이거나 free/dead(상태 &3)면 무시하고, 아니면 현재 가상 피해(damage(target, damage, 발사 소유자 BYTE, 발사 SID))를 부른다.
//  - hit(firing, target, now): 총 타입의 float DPS(비유한은 오류)를 읽고 패치는 별도 flyer/balloon DPS 선택(표적 TYPE flags2 0x120000)을 적용한다. 양자화 현재 시각(15비트) − 저장 시각 = 경과를 `경과 × DPS` 로 곱해 1 이상이면 정수 피해로 apply 하되,
//    경과(float 저장)가 음수이거나 30초를 넘으면 `DPS × 3` 으로 바꾼다. 어느 경우든 마지막에 현재 시각을 setTime 한다.
//  - makeMissileGunHitHooks(hit, state, hooks): Missile 직접 피해(hit)와 빗나감/배리어 코너 시각 갱신(refresh)을 같은 몸체로 연결한다(state.now 는 live).
// hitTypes[type] = { damagePerSecond, specialFlyerDamage, flyerDamagePerSecond }, hooks = { damage(target, damage, owner, firing), notifySurface(firing), applyOverride?(firing, target, damage)→bool }. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E112: 총 피해 시각/전달 이식으로 추가)

// 시각 WORD 의 15비트 마스크, 비행/풍선 genus, 슬롯 위치(TYPE·상태·WORD·소유자), 30초 상한과 3초분 배수, 판본의 TYPE 수이다. (2026-11-08 WB04-E112)
const TIME_MASK = 0x7fff;
const FLYER_OR_BALLOON = 0x120000;
const TYPE_AT = 10;
const STATE_AT = 11;
const WORD_AT = 12;
const OWNER_AT = 34;
const MAX_ELAPSED = 30;
const BURST_SECONDS = 3;
const TYPE_COUNT = 188;
const fround = Math.fround;

// 원본 정상 CRT __ftol 의 유한 int32 절삭이다(범위 밖은 오류). (2026-11-08 WB04-E112)
function truncate(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('Gun hit 시각/피해 절삭 범위 오류');
    return Math.trunc(value);
}

// 원본 정수초와 소수 8분의 1초를 OR 하고 필요한 하위 15비트를 읽는다. (2026-11-08 WB04-E112)
function packed(now) {
    const integral = truncate(now);
    const fraction = truncate((now - integral) * 8.0);
    return (Math.imul(integral, 8) | fraction) >>> 0;
}

// 총 피해 시각/전달 처리기이다. (2026-11-08 WB04-E112)
export class RawGunHit {
    // 피해 시각/소유자/표적 상태를 읽고 쓰는 실제 SID 저장소이다. (2026-11-08 WB04-E112)
    #pool;
    // group/genus 에 사용하는 같은 실제 타입 표이다. (2026-11-08 WB04-E112)
    #types;
    // 현재 타입의 실제 DPS 보충 자료이다. (2026-11-08 WB04-E112)
    #hitTypes;
    // 실제 피해/공간 통지 경계이다. (2026-11-08 WB04-E112)
    #hooks;

    // 실제 타입/DPS 자료의 판본과 수명을 검사한다. (2026-11-08 WB04-E112)
    constructor(pool, types, hitTypes, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT || !hitTypes || hitTypes.length !== types.length) throw new Error('Gun hit 타입/DPS 판본 오류');
        this.#pool = pool;
        this.#types = types;
        this.#hitTypes = hitTypes;
        this.#hooks = hooks;
    }

    // 같은 실제 풀을 사용하는 소비자만 연결한다. (2026-11-08 WB04-E112)
    get pool() { return this.#pool; }

    // 실제 시간 필드를 소유할 수 있는 원본 자산 분류를 확인한다(비행이 아니면 group 0/1). (2026-11-08 WB04-E112)
    #firing(firing) {
        if (!firing || firing >= this.#pool.capacity) throw new RangeError('Gun hit 발사 SID 오류');
        const raw = this.#pool.slot(firing);
        if (raw[TYPE_AT] >= this.#types.length) throw new RangeError('Gun hit 발사 타입 오류');
        const type = this.#types[raw[TYPE_AT]];
        if (!(type.flags2 & 0x100000) && type.group !== 0 && type.group !== 1) throw new Error('Gun hit 시간 group 원본 assert');
        return raw;
    }

    // 004d4da0: raw+12 WORD 의 bit0 을 제외한 압축 시각이다(분수는 binary 0.125 이므로 원본 double 비트를 유지). (2026-11-08 WB04-E112)
    time(firing) {
        const raw = this.#firing(firing);
        const word = raw[WORD_AT] | (raw[WORD_AT + 1] << 8);
        const packedTime = word >> 1;
        return (packedTime >> 3) + (packedTime & 7) * 0.125;
    }

    // 004d4e10: 현재 시각 WORD 쓰기와 뒤따르는 실제 공간 통지이다(bit0 보존). (2026-11-08 WB04-E112)
    setTime(firing, now) {
        this.#firing(firing);
        if (typeof this.#hooks.notifySurface !== 'function') throw new Error('Gun hit 실제 공간 통지 경계 누락');
        const raw = this.#pool.allocated(firing);
        const word = (((packed(now) << 1) | (raw[WORD_AT] & 1)) >>> 0) & 0xffff;
        raw[WORD_AT] = word & 0xff;
        raw[WORD_AT + 1] = word >> 8;
        this.#hooks.notifySurface(firing);
    }

    // 004d4eb0: 가상 피해 호출 직전 표적의 live/free/dead 와 발사 소유자를 다시 읽는다. (2026-11-08 WB04-E112)
    apply(firing, target, damage) {
        const hooks = this.#hooks;
        // Rain 45f940 은 generic 4d4eb0 의 target0/free/dead 선관문을 거치지 않는다. 미설치 typed 경로는 기존 본문 그대로다.
        if (typeof hooks.applyOverride === 'function' && hooks.applyOverride(firing, target, damage)) return;
        if (!target) return;
        if (target >= this.#pool.capacity) throw new RangeError('Gun hit 표적 SID 오류');
        if (this.#pool.slot(target)[STATE_AT] & 3) return;
        const source = this.#firing(firing);
        if (typeof hooks.damage !== 'function') throw new Error('Gun hit 실제 표적 피해 경계 누락');
        hooks.damage(target, damage, source[OWNER_AT], firing);
    }

    // 004d4f00: 양자화 시각차 × DPS 가 1 이상일 때 원본 정수 피해를 전달한다. 30초 초과 간격(또는 음수)에서 3초분 DPS 를 쓴다. (2026-11-08 WB04-E112)
    hit(firing, target, now) {
        const source = this.#firing(firing);
        let dps = this.#hitTypes[source[TYPE_AT]].damagePerSecond;
        if (!Number.isFinite(dps)) throw new Error('Gun hit 비유한 DPS');
        if (this.#hitTypes[source[TYPE_AT]].specialFlyerDamage && target) {
            if (target >= this.#pool.capacity) throw new RangeError('Gun hit DPS 표적 SID 오류');
            const victim = this.#pool.slot(target);
            if (victim[TYPE_AT] >= this.#types.length) throw new RangeError('Gun hit DPS 표적 타입 오류');
            if (this.#types[victim[TYPE_AT]].flags2 & FLYER_OR_BALLOON) dps = this.#hitTypes[source[TYPE_AT]].flyerDamagePerSecond;
        }
        const quantized = packed(now) & TIME_MASK;
        const elapsed = (quantized >> 3) + (quantized & 7) * 0.125 - this.time(firing);
        const storedElapsed = fround(elapsed);
        // 패치는 x87 에 남은 시각차로 곱하고 정확히 30초를 넘을 때만 3초분으로 바꾼다.
        const damage = elapsed * dps;
        if (damage >= 1.0) {
            const selected = (storedElapsed < 0 || storedElapsed > MAX_ELAPSED) ? dps * BURST_SECONDS : damage;
            this.apply(firing, target, truncate(selected));
        }
        this.setTime(firing, now);
    }
}

// Missile 직접 피해와 빗나감/배리어 코너 시각 갱신을 같은 몸체로 연결한다. (2026-11-08 WB04-E112)
export function makeMissileGunHitHooks(hit, state, hooks) {
    return {
        ...hooks,
        hit: (firing, target) => hit.hit(firing, target, state.now),
        refresh: (firing) => hit.setTime(firing, state.now)
    };
}
