// 네 직접 피해 주문(가상 표 502908·5029d0·502aa8·502b70)의 가상 bc 자격과 c0 효과·공유 게임 난수 소비 순서를 옮긴 모듈이다(cpppj `o/RawBombStrike.cpp/.h`, 10.78 전용).
//  - handles(parent): 현재 가상 표가 위 네 표 중 하나일 때만 참이다(타입 이름이나 공통 genus 로 다른 주문까지 넓히지 않는다).
//  - mayTarget(parent, target): 자격은 현재 HP(+26) 수치나 free/dead 비트를 검사하지 않는다. 첫 표는 대상 genus & 0x120000 의 마스크 값을 그대로 돌려주고(원본 반환), 나머지 표는 HP 를 보유(dais 추상이면 extra 비트 1, 아니면 flags1 0x10)하고
//    허용 genus 0x80074200 이 있을 때 1, 아니면 0 이다. 다른 표는 거부한다.
//  - apply(parent, target): 대상이 부모의 +12 WORD(운반체)이면 끝낸다. 표별로 ① 502908: 생성(kind, 2)+현재 대상 중심에 pop 한 뒤 대상 TYPE 이 특별 종류가 아니면 피해 2000, 맞고 권한이 있으면 삭제(0x100008)
//    ② 5029d0: 대상이 dead(상태 비트 2)이면 끝, 부모 소리 → 저장한 운반체가 살아 있으면 빔 → 피해 500 ③ 502aa8: dead 면 끝, 대상 중심을 두 번 투영해 기둥 효과(수명 5)와 소리를 낸 뒤 난수로 1~4회 반복하며 3분의 2 확률로 피해 100~499(피해 전 HP 가 1 미만이면 중단, 피해가 HP 이상이면 중단)
//    ④ 502b70: 난수로 1~49회 반복하며 절반 확률로 생성/pop 후 피해 20~49(초기 dead 관문 없음, 같은 HP 중단 규칙). 효과 사이의 현재 TYPE/좌표/상태/owner/가상 표는 원본 위치에서 다시 읽는다.
// 상태 { boss, specialType(138), firstEffectType(174), hailEffectType(177) } 는 현재 값을 읽는 살아 있는 객체이고 random 은 다른 효과와 공유하는 GameRandom 이다.
// 훅 { create, pop, damage, destroy, sound, beam, project, column } 는 도달한 지점에서만 요구한다(없으면 TypeError). DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E54: RawBombStrike 이식으로 추가)

// 슬롯 필드 위치와 개수, 네 가상 표, 자격 genus 비트, 피해량/삭제 flags 상수이다. (2026-11-08 WB04-E54)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const CARRIER_OFFSET = 12;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const HP_OFFSET = 26;
const OWNER_OFFSET = 34;
const EXTRA_OFFSET = 40;
const TYPE_COUNT = 188;
const TABLES = Object.freeze([0x502908, 0x5029d0, 0x502aa8, 0x502b70]);
const DAIS_GENUS = 0x400000;
const HAS_HP_FLAG = 0x10;
const ALLOWED_GENUS = 0x80074200;
const FIRST_ALLOWED_GENUS = 0x120000;
const FIRST_DAMAGE = 2000;
const FIRST_DESTROY_FLAGS = 0x100008;
const BEAM_DAMAGE = 500;
const COLUMN_LIFE = 5.0;
const DEAD_BIT = 2;
const DEAD_MASK = 3;

// 선택한 실제 효과가 빠졌으면 무동작 성공으로 처리하지 않는다. (2026-11-08 WB04-E54)
function need(hook) {
    if (typeof hook !== 'function') throw new TypeError('직접 피해 주문 실제 하위 미연결');
    return hook;
}

// 정렬되지 않은 float 4바이트를 읽는다. (2026-11-08 WB04-E54)
const floatView = new DataView(new ArrayBuffer(4));
function floatAt(raw, offset) {
    floatView.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return floatView.getFloat32(0, true);
}

// 직접 피해 주문 실행기이다. 사용: new RawBombStrike(pool, types, random, state, hooks). (2026-11-08 WB04-E54)
export class RawBombStrike {
    #pool;
    #types;
    #random;
    #state;
    #hooks;

    // 복원 판본과 타입 표의 원래 범위를 확인한다. 훅은 생성 시점의 사본을 쓴다. (2026-11-08 WB04-E54)
    constructor(pool, types, random, state, hooks) {
        if (pool?.edition !== 'patch1078' || types?.length !== TYPE_COUNT || typeof random?.next !== 'function' || typeof state !== 'object' || state === null) throw new TypeError('직접 피해 주문 판본/TYPE 오류');
        this.#pool = pool;
        this.#types = types;
        this.#random = random;
        this.#state = state;
        this.#hooks = { ...hooks };
    }

    // 상위 어댑터가 같은 실제 저장소인지 확인하는 풀 참조이다. (2026-11-08 WB04-E54)
    get pool() {
        return this.#pool;
    }

    // 현재 바이트를 리틀엔디언의 지정 폭으로 조합한다. (2026-11-08 WB04-E54)
    #read(sid, offset, width = 4) {
        const raw = this.#pool.slot(sid);
        let value = 0;
        // 원본 물리 필드의 현재 바이트만 읽는다.
        for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
        return value;
    }

    // 큰 효과 뒤 바뀐 종류를 이전 TYPE 로 캐시하지 않는다. (2026-11-08 WB04-E54)
    #type(sid) {
        const kind = this.#read(sid, TYPE_OFFSET, 1);
        if (kind >= this.#types.length) throw new RangeError('직접 피해 주문 현재 TYPE 범위');
        return this.#types[kind];
    }

    // 4adea0: 실제 중심 함수의 double 계산 뒤 float 저장을 보존한다. (2026-11-08 WB04-E54)
    #center(sid) {
        const type = this.#type(sid);
        const raw = this.#pool.slot(sid);
        return [Math.fround(floatAt(raw, X_OFFSET) - type.footX * 0.5), Math.fround(floatAt(raw, Y_OFFSET) - type.footY * 0.5)];
    }

    // 실제 가상 98 요청 직전에 현재 표와 부모 소유자를 읽는다. (2026-11-08 WB04-E54)
    #hit(parent, target, amount) {
        need(this.#hooks.damage)(target, this.#read(target, 0), amount, this.#read(parent, OWNER_OFFSET, 1), parent);
    }

    // 생성이 현재 대상 위치/타입을 바꿀 수 있으므로 중심을 생성 전에 캡처하지 않는다. (2026-11-08 WB04-E54)
    #spawn(target, kind) {
        const created = need(this.#hooks.create)(kind, 2);
        const center = this.#center(target);
        need(this.#hooks.pop)(created, this.#read(created, 0), center[0], center[1], 0);
    }

    // 기둥/빔 효과 이후의 현재 부모 중심으로 소리를 요청한다. (2026-11-08 WB04-E54)
    #sound(parent) {
        const center = this.#center(parent);
        need(this.#hooks.sound)(center[0], center[1]);
    }

    // 타입 번호나 공통 genus 만으로 다른 주문까지 포함하지 않는다. (2026-11-08 WB04-E54)
    handles(parent) {
        return TABLES.includes(this.#read(parent, 0));
    }

    // 첫 계열의 원래 마스크 반환과 나머지 0/1 반환을 구별한다. (2026-11-08 WB04-E54)
    mayTarget(parent, target) {
        const table = this.#read(parent, 0);
        if (!this.handles(parent)) throw new TypeError('직접 피해 주문 자격 가상 표 오류');
        const type = this.#type(target);
        if (table === TABLES[0]) return (type.flags2 & FIRST_ALLOWED_GENUS) >>> 0;
        const hasHp = (type.flags2 & DAIS_GENUS) ? (this.#read(target, EXTRA_OFFSET, 1) & 1) !== 0 : (type.flags1 & HAS_HP_FLAG) !== 0;
        return hasHp && (type.flags2 & ALLOWED_GENUS) !== 0 ? 1 : 0;
    }

    // 각 계열의 실제 조기 반환과 외부 효과 사이 재조회를 독립적으로 보존한다. (2026-11-08 WB04-E54)
    apply(parent, target) {
        const hooks = this.#hooks;
        const state = this.#state;
        const random = this.#random;
        const table = this.#read(parent, 0);
        if (!this.handles(parent)) throw new TypeError('직접 피해 주문 효과 가상 표 오류');
        if (target === this.#read(parent, CARRIER_OFFSET, 2)) return;
        if (table === TABLES[0]) {
            this.#spawn(target, state.firstEffectType);
            if (this.#read(target, TYPE_OFFSET, 1) !== state.specialType) this.#hit(parent, target, FIRST_DAMAGE);
            else if (state.boss) need(hooks.destroy)(target, this.#read(target, 0), FIRST_DESTROY_FLAGS);
            return;
        }
        if (table === TABLES[1]) {
            const savedSource = this.#read(parent, CARRIER_OFFSET, 2);
            if (this.#read(target, STATE_OFFSET, 1) & DEAD_BIT) return;
            this.#sound(parent);
            if (savedSource && !(this.#read(savedSource, STATE_OFFSET, 1) & DEAD_MASK)) need(hooks.beam)(this.#read(parent, CARRIER_OFFSET, 2), target);
            this.#hit(parent, target, BEAM_DAMAGE);
            return;
        }
        if (table === TABLES[2]) {
            if (this.#read(target, STATE_OFFSET, 1) & DEAD_BIT) return;
            const center = this.#center(target);
            const from = need(hooks.project)(center[0], center[1]);
            const to = need(hooks.project)(center[0], center[1]);
            need(hooks.column)(target, [from[0], 0], to, COLUMN_LIFE);
            this.#sound(parent);
            const count = random.next(4) + 1;
            // 반복 횟수는 최초 난수이며 현재 HP 는 피해량 난수를 소비한 뒤 읽는다.
            for (let index = 0; index < count; index += 1) {
                const choice = random.next(3);
                if (choice < 2) {
                    const amount = (random.next(400) + 100) | 0;
                    const hp = this.#read(target, HP_OFFSET) | 0;
                    if (hp < 1) return;
                    this.#hit(parent, target, amount);
                    if (hp < amount) return;
                }
            }
            return;
        }
        const count = random.next(49) + 1;
        // 마지막 계열은 초기 dead 관문이 없고 성공 난수마다 생성/pop 을 피해 판단보다 먼저 실행한다.
        for (let index = 0; index < count; index += 1) {
            if (random.next(2)) {
                this.#spawn(target, state.hailEffectType);
                const amount = (random.next(30) + 20) | 0;
                const hp = this.#read(target, HP_OFFSET) | 0;
                if (hp < 1) return;
                this.#hit(parent, target, amount);
                if (hp < amount) return;
            }
        }
    }
}
