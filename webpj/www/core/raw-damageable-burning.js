// Damageable 의 현재 HP 와 최대 HP 절반 비교, 실제 burning 프로세스 생성/제거 분기(0044c060)를 옮긴 모듈이다(cpppj `o/RawDamageableBurning.cpp/.h`, 10.78 전용).
// 순서: 대상이 free(상태 비트 0)이거나 현재 TYPE 의 damageEffect 가 0 이면 아무것도 조회하지 않고 0 을 반환한다(자연 반환). 아니면 burning 프로세스 조회(0번째)를 한 뒤, 검사 전역(checking)이 켜져 있고 1번째 프로세스가 있으면
//   원문 진단('findProcess (mySid, burningSystemProcessType, 1 ) == 0')을 요구한다. 현재 HP(signed DWORD) 가 TYPE 최대 HP 의 절반(`최대*2/4`)보다 작으면: 프로세스가 없을 때만 bulf 타입은 위치 소리를 낸 뒤 생성하고,
//   아니면 기존 프로세스가 있으면 삭제하고 0 을 반환한다. 진단이 현재 HP/TYPE/전역을 바꿔도 앞서 저장한 현재 HP/절반/찾은 프로세스는 다시 읽지 않는다. 선택한 하위 경계의 누락은 묵시 성공으로 대체하지 않는다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E67: 사제 피격 이식에 필요한 burning 판정으로 추가)

// 슬롯의 상태/TYPE/현재 HP/좌표 위치, 피해 효과 진단의 원문 식과 파일, 줄 번호이다. (2026-11-08 WB04-E67)
const STATE_OFFSET = 11;
const TYPE_OFFSET = 10;
const HP_OFFSET = 26;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const TYPE_COUNT = 188;
const DUPLICATE_EXPRESSION = 'findProcess (mySid, burningSystemProcessType, 1 ) == 0';
const DAMAGEABLE_FILE = '\\Ns\\O\\Damageable.cpp';
const DUPLICATE_LINE = 0x210;

// 원본 전역의 현재 값이다: checking(005e4794 의 전체 DWORD; 비영이면 켜짐)과 실제 Client 의 같은 DWORD 를 빌리는 getter(sharedChecking), burning/bulf 프로세스 TYPE 번호(변경 가능)이다. (2026-11-08 WB04-E67)
export class DamageableBurningState {
    checking = 0;
    sharedChecking = null;
    burningType = 14;
    bulfType = 135;

    // 소비 위치마다 현재 비트를 다시 읽어 비영만 켜짐으로 본다(원래 저장소는 고치지 않는다). (2026-11-08 WB04-E67)
    isChecking() {
        return (this.sharedChecking ? this.sharedChecking() >>> 0 : this.checking >>> 0) !== 0;
    }
}

// 비트 보존을 위해 정렬되지 않은 슬롯 필드를 읽는다. (2026-11-08 WB04-E67)
function dwordAt(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 비트를 단정도 float 로 해석한다. (2026-11-08 WB04-E67)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    return view.getFloat32(0, true);
}

// pool 은 10.78 SidPool, types 는 188칸 TYPE 표({ damageEffect } 필요; 참조), hp 는 같은 풀의 SquidReward(typeHitPoints), state 는 DamageableBurningState,
//   hooks = { find(sid, 타입, 번호)→프로세스 SID(0=없음), create(sid)→SID, destroy(sid, flags), bulfSound(sid, x, y), assertion(식, 파일, 줄) } 이며 도달한 것만 필요하다. (2026-11-08 WB04-E67)
export class RawDamageableBurning {
    // 피격자와 burning Form 을 공유하는 풀, 현재 TYPE 표, 최대 HP 처리기, 전역, 하위 효과이다.
    #pool;
    #types;
    #hp;
    #state;
    #hooks;

    // 다른 풀의 최대 HP 조회가 같은 SID 를 읽지 못하도록 구성 때 막는다. (2026-11-08 WB04-E67)
    constructor(pool, types, hp, state, hooks) {
        if (pool?.edition !== 'patch1078' || hp?.pool !== pool || !Array.isArray(types) || types.length !== TYPE_COUNT) throw new TypeError('burning HP/타입/풀 연결 오류');
        this.#pool = pool;
        this.#types = types;
        this.#hp = hp;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 같은 풀 연결을 검사하는 풀이다. (2026-11-08 WB04-E67)
    get pool() {
        return this.#pool;
    }

    // zero damageEffect 는 조회/생성 경계를 요구하지 않는 실제 원본 반환이다. 반환은 현재 burning 프로세스 SID(없으면 0)이다. (2026-11-08 WB04-E67)
    refresh(victim) {
        if (!victim || victim >= this.#pool.capacity) throw new RangeError('burning 피해 SID 오류');
        // 원본은 현재 슬롯을 그대로 보며 하위 효과 뒤의 TYPE/좌표를 다시 읽는다.
        const raw = this.#pool.physical(victim);
        if (raw[STATE_OFFSET] & 1) return 0;
        if (raw[TYPE_OFFSET] >= this.#types.length) throw new RangeError('burning 피해 타입 오류');
        if (!this.#types[raw[TYPE_OFFSET]].damageEffect) return 0;
        const hooks = this.#hooks;
        if (typeof hooks.find !== 'function') throw new Error('burning 실제현재프로세스조회 누락');
        const current = dwordAt(raw, HP_OFFSET) | 0;
        const maximum = this.#hp.typeHitPoints(raw[TYPE_OFFSET]);
        const doubled = Math.imul(maximum, 2);
        const half = Math.trunc(doubled / 4) | 0;
        let found = hooks.find(victim, this.#state.burningType, 0);
        if (this.#state.isChecking() && hooks.find(victim, this.#state.burningType, 1)) {
            if (typeof hooks.assertion !== 'function') throw new Error('burning 두번째프로세스 원본 진단 장치 미연결');
            hooks.assertion(DUPLICATE_EXPRESSION, DAMAGEABLE_FILE, DUPLICATE_LINE);
        }
        if (current < half) {
            if (!found) {
                const bulf = raw[TYPE_OFFSET] === this.#state.bulfType;
                if (bulf && typeof hooks.bulfSound !== 'function') throw new Error('burning 실제소리몸체 누락');
                if (bulf) hooks.bulfSound(victim, floatOf(dwordAt(raw, X_OFFSET)), floatOf(dwordAt(raw, Y_OFFSET)));
                if (typeof hooks.create !== 'function') throw new Error('burning 실제생성몸체 누락');
                found = hooks.create(victim);
            }
        } else if (found) {
            if (typeof hooks.destroy !== 'function') throw new Error('burning 실제삭제몸체 누락');
            hooks.destroy(found, 0);
            return 0;
        }
        return found;
    }
}
