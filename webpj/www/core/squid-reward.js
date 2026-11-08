// 원본 10.78 패치판의 삭제/해체 보상(0044c2f0)과 SP 가산(0040eec0/0041df80)·AI 지갑 가산(00413bb0)·최대 HP 조회(004adcb0/0049fce0)를 옮긴 모듈이다.
//  - 비용 곡선(0044b220): 로딩 비용표(DAT_0059ab50, ftol(cost + 번호*23) 저장·읽을 때 번호*23 감산)의 정수 단가를 float 로 바꿔 (개수+1) 배하고 0방향 절삭한다.
//    제단/다이스는 개수를 하나 줄인다. 성장형 레인 바리케이드는 가상 개수를 다시 물은 뒤 현재 일반 레인 바리케이드 TYPE 의 비용을 쓴다.
//  - 삭제 보상은 현재 비율(DAT_005424bc)의 DWORD 곱/100, 해체 환불(0043cb30)은 너겟 외 1/4 배율과 남은 HP 비례(절반 모드면 HP 절반 미만 0)다.
//  - 지급: SP 가산(고정 모드면 194162 와의 차이)·float 저장·delta 의 절삭 DWORD 를 AI 지갑으로, 그 뒤 SP 표시 갱신. 지급하지 않은 가치(비용−지급)는 샘 풀(DAT_00595088)에 더한다.
//  - 효과(가상 개수·SP 표시·진단) 뒤에는 현재 TYPE BYTE·현재 특별 TYPE 번호·현재 비율/자원을 소비 위치에서 다시 읽는다(cpppj 와 같은 호출 순서).
// 장부(bookkeeping)는 cpppj SquidPostPopState 와 같은 객체이며 이 모듈은 localOwner(BYTE)와 aiAttached[0..8] 만 소비 위치에서 읽는다.
// 전체 SquidPostPop 이식(WB04-E)이 같은 객체를 공급한다. CD 10.72(SP 전역 float·WORD HP) 경로는 범위 밖이다.
// 근거: cpppj `o/SquidReward.cpp/.h`(기준선 1f00a25a). 원본 관측: `cpppj/tests/fixtures/reward-x86.tsv`(10.78 2400행)·`sp-add-bits-x86.tsv`(504행)·
//       `rifttype-hit-points-x86.tsv`(10.78 579행). 검사는 tests/squid-reward.test.mjs 에 있다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-07 WB04-D5b: ContentFormLifetime 의 너겟 수입 경제 의존으로 추가)
import { ScrambledSpStore } from './sp-store.js';
import { currentRiftTypeNumber, readRiftHitPoints } from './squid-factory.js';

// 슬롯의 TYPE·상태 BYTE·HP DWORD·다이스 부착 BYTE 위치이다(10.78). (2026-11-07 WB04-D5b)
const TYPE = 10, STATE = 11, HP = 26, DAIS_EXTRA = 40;
// AI 지갑 하한, SP 고정값(0x500ff8 의 double), 비용표 번호 계수, 해체 환불 배율이다. (2026-11-07 WB04-D5b)
const WALLET_FLOOR = 0x4a6, FIXED_SP = 194162.0, COST_BIAS = 23, SALVAGE_SCALE = 0.25;
// 타입 flags2 의 다이스 부류와 flags1 의 HP 보유 비트이다. (2026-11-07 WB04-D5b)
const DAIS_KIND = 0x400000, HAS_HP = 0x10;
// 비용 곡선 합이 정확히 표현되는 한계(2^53)와 곡선 항 수 상한이다. (2026-11-07 WB04-D5b)
const EXACT_LIMIT = 9007199254740992, MAX_TERMS = 0x01000000;
// 소유자 번호 상한(kPlayerCount)이다. (2026-11-07 WB04-D5b)
const PLAYER_COUNT = 8;
// SquidRewardState 가 다른 소유자에게서 빌릴 수 있는 칸 이름과 GameAuxState 의 같은 원본 주소 이름이다. (2026-11-07 WB04-D5b)
const SHARED_FIELDS = Object.freeze({
    percent: 'refundPercent', geyserPool: 'resourcePool', halfHealthMode: null, priestQuarterHp: 'priestQuarterHp', priestType: 'priestType',
    altarType: 'altarType', daisType: 'daisType', silentType: 'rainBlockerType', aliasType: 'growingRainBlockerType', nuggetType: 'nuggetType',
    // SP 고정 두 모드는 GameAuxState 에 대응 칸이 없어 기본 키가 없다(share 에 소유자 칸 이름을 직접 준다). (2026-11-09 WB04-E166: cpppj 2b048419 의 sharedFixedSpA/B 동기화)
    fixedSpA: null, fixedSpB: null,
});

// 원본 __ftol: 0방향 절삭 뒤 하위 32비트(signed)이다. 비유한/64비트 범위 밖은 원본에서 부정 정수이므로 거부한다. (2026-11-07 WB04-D5b)
function ftol(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= 9.2e18) throw new RangeError('보상 정수 변환 범위 오류');
    return value | 0;
}

// SP 가산의 지갑 절삭: 비유한/64비트 밖은 CRT 정수 불확정값의 하위 DWORD 0 이고, 그 밖은 0방향 절삭의 하위 32비트이다. (2026-11-07 WB04-D5b)
function spMirrorFtol(value) {
    if (!Number.isFinite(value) || value >= 9223372036854775808 || value < -9223372036854775808) return 0;
    return value | 0;
}

// float 비트(uint32)와 수의 상호 변환이다. (2026-11-07 WB04-D5b)
const BITS_VIEW = new DataView(new ArrayBuffer(4));
function bitsToFloat(bits) {
    BITS_VIEW.setUint32(0, bits >>> 0, true);
    return BITS_VIEW.getFloat32(0, true);
}
function floatToBits(value) {
    BITS_VIEW.setFloat32(0, value, true);
    return BITS_VIEW.getUint32(0, true);
}

// x86 부동소수 연산의 NaN 규칙: quiet 비트와 실수 부정값(real indefinite)이다. (2026-11-07 WB04-D5b)
const QUIET_NAN = 0x00400000, INDEFINITE_NAN = 0xffc00000;

// float 비트가 NaN 인지 본다(지수 전부 1·가수 0 아님). (2026-11-07 WB04-D5b)
function isNanBits(bits) {
    return ((bits >>> 0) & 0x7f800000) === 0x7f800000 && (bits & 0x7fffff) !== 0;
}

// x86 float 연산 결과 비트이다. operands 는 기계어 피연산자 순서의 float 비트(상수 double 은 null)이며, 첫 NaN 피연산자가 있으면 그 quiet 비트,
// 없으면 compute() 를 float 로 저장한 비트, 결과만 NaN 이면(무한대 상쇄 등) 실수 부정값 0xffc00000 이다. JS 산술의 NaN 페이로드 소실을 막는다. (2026-11-07 WB04-D5b)
function x86FloatBits(compute, ...operands) {
    // 피연산자 순서대로 첫 NaN 을 찾는다.
    for (const bits of operands) if (bits !== null && isNanBits(bits)) return (bits | QUIET_NAN) >>> 0;
    const value = Math.fround(compute());
    return Number.isNaN(value) ? INDEFINITE_NAN : floatToBits(value);
}

// 보상이 읽고 갱신하는 판본 전역과 AI 지갑이다(cpppj SquidRewardState). 필드는 독립 값형 입력이며 share/bindGameAux 로 빌린 칸은 매 소비 위치에서 소유자 값을 읽는다.
// 사용: const state = new SquidRewardState(); state.bindGameAux(aux); state.share('halfHealthMode', world, 'halfHealthMode'). (2026-11-07 WB04-D5b)
export class SquidRewardState {
    // 원본 타입 번호 전역의 초기값(제단 164·다이스 165·레인 바리케이드 147·성장형 150·너겟 154·사제 158)이다.
    types = { none: 164, dais: 165, silent: 147, alias: 150, fullValue: 154, priest: 158 };
    // 삭제 보상 비율(DAT_005424bc)과 샘 풀 자원(DAT_00595088, signed)이다.
    percent = 25;
    geyserPool = 0;
    // SP 고정 두 전역(DAT_00594fb8·DAT_005c85a4)과 해체 절반 HP 모드(DAT_00594fbc)이다.
    fixedSpA = false;
    fixedSpB = false;
    halfHealthMode = false;
    // AI 지갑 비활성(DAT_00594fc8)·지갑 하한 조건(DAT_005634a0->+0x68)·사제 1/4 HP(DAT_005ca8f0)이다.
    aiMirrorDisabled = false;
    walletClamp = false;
    priestQuarterHp = false;
    // checking(005e4794) DWORD 이다. 수(독립 값) 또는 현재 DWORD 를 돌려주는 함수(같은 소유자 빌림)다.
    debugAsserts = 0;
    // 독립 값형 AI 지갑(소유자 0..8, signed DWORD)이다. 실제 세션은 SquidReward.bindPatchWallet 로 brain+2d8 을 연결한다.
    aiWallet = new Int32Array(PLAYER_COUNT + 1);
    // 빌린 칸 이름 → { owner, key } 이다.
    #shared = new Map();

    // name 칸을 owner[key] 에서 매 소비 위치마다 읽도록 빌린다(쓰기 칸 geyserPool 은 같은 소유자에 쓴다). owner 가 null 이면 빌림을 푼다.
    share(name, owner, key = SHARED_FIELDS[name]) {
        if (!Object.hasOwn(SHARED_FIELDS, name)) throw new RangeError(`보상 상태에 빌릴 수 없는 칸: ${name}`);
        if (owner === null) {
            this.#shared.delete(name);
            return;
        }
        if (!owner || typeof owner !== 'object' || typeof key !== 'string' || !(key in owner)) throw new TypeError(`보상 상태 ${name} 의 소유자 칸 오류`);
        this.#shared.set(name, { owner, key });
    }

    // 같은 GameAuxState 의 비율·자원·사제 1/4·특별 TYPE 번호 칸을 모두 빌린다(절반 HP 모드는 World 칸이라 따로 share 한다).
    bindGameAux(aux) {
        // GameAuxState 에 있는 칸만 같은 원본 주소 이름으로 빌린다.
        for (const [name, key] of Object.entries(SHARED_FIELDS)) if (key) this.share(name, aux, key);
    }

    // 빌린 칸이면 소유자의 현재값, 아니면 독립 값이다.
    #read(name, fallback) {
        const shared = this.#shared.get(name);
        return shared ? shared.owner[shared.key] : fallback;
    }

    // 현재 비율(DWORD)이다.
    currentPercent() { return this.#read('percent', this.percent) >>> 0; }
    // 현재 샘 풀 자원(signed DWORD)이다.
    currentGeyserPool() { return this.#read('geyserPool', this.geyserPool) | 0; }
    // 샘 풀 자원에 signed DWORD 를 쓴다(빌린 칸이면 같은 소유자에).
    storeGeyserPool(value) {
        const shared = this.#shared.get('geyserPool');
        if (shared) shared.owner[shared.key] = value | 0;
        else this.geyserPool = value | 0;
    }
    // 현재 해체 절반 HP 모드이다.
    currentHalfHealthMode() { return Boolean(this.#read('halfHealthMode', this.halfHealthMode)); }
    // 현재 사제 1/4 HP 모드이다.
    currentPriestQuarterHp() { return Boolean(this.#read('priestQuarterHp', this.priestQuarterHp)); }
    // 현재 특별 TYPE 번호들(사제·제단·다이스·레인 바리케이드·성장형·너겟)이다.
    currentPriestType() { return this.#read('priestType', this.types.priest) >>> 0; }
    currentAltarType() { return this.#read('altarType', this.types.none) >>> 0; }
    currentDaisType() { return this.#read('daisType', this.types.dais) >>> 0; }
    currentSilentType() { return this.#read('silentType', this.types.silent) >>> 0; }
    currentAliasType() { return this.#read('aliasType', this.types.alias) >>> 0; }
    currentNuggetType() { return this.#read('nuggetType', this.types.fullValue) >>> 0; }
    // 현재 SP 고정 모드 A(fort bool)와 B(editor DWORD)이다. 빌린 칸은 소유자의 현재 값(bool 이면 그대로, DWORD 이면 0 이 아닐 때 켜짐)이고 아니면 독립 값형 입력이다. (2026-11-09 WB04-E166: cpppj 2b048419 FixedSpA/FixedSpB 동기화)
    currentFixedSpA() { return this.#truthy(this.#read('fixedSpA', this.fixedSpA)); }
    currentFixedSpB() { return this.#truthy(this.#read('fixedSpB', this.fixedSpB)); }
    // bool 은 그대로, 수(DWORD)는 하위 32비트가 0 이 아닌지로 판정한다. (2026-11-09 WB04-E166)
    #truthy(value) { return typeof value === 'boolean' ? value : (value >>> 0) !== 0; }
    // 현재 checking DWORD 이다.
    currentChecking() { return (typeof this.debugAsserts === 'function' ? this.debugAsserts() : this.debugAsserts) >>> 0; }
}

// 삭제/해체 보상 처리기이다(cpppj SquidReward, 10.78). pool 은 SidPool, types 는 188칸 TYPE 레코드({ cost, maxHitPoints, flags1, flags2 })이며 생성 때 복사한다.
// bookkeeping 은 { localOwner, aiAttached[9] } 를 가진 장부(SquidPostPopState), state 는 SquidRewardState, store 는 ScrambledSpStore 이다.
// hooks: { virtualCount(sid) — 파생 vtable +0x80 개수(없으면 상태 상위 3비트), refreshSp() — SP 표시 갱신(0043dad0, 없어도 됨) }.
// 사용: const reward = new SquidReward(pool, types, books, state, store); reward.refund(sid, recipient, salvage). (2026-11-07 WB04-D5b)
export class SquidReward {
    // 같은 풀·생성 시 TYPE 사본·빌린 현재 표·빌린 로딩 비용표이다.
    #pool;
    #types;
    #currentTypes = null;
    #loadedCostTypes = null;
    // 같은 장부·상태·SP 저장소·훅이다.
    #bookkeeping;
    #state;
    #store;
    #hooks;
    // 현재 TYPE HP 진단 장치와 실제 PATCH 지갑 처리기, 가상 개수 호출 횟수이다.
    #hitPointAssertion = null;
    #creditWallet = null;
    #virtualCalls = 0;

    // 판본/크기·장부·저장소를 확인한다. 패치판 보상은 SP 저장소가 필요하다.
    constructor(pool, types, bookkeeping, state, store, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('보상은 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== 188) throw new RangeError('보상 타입 판본/크기 오류');
        if (!bookkeeping || typeof bookkeeping !== 'object' || !('localOwner' in bookkeeping) || bookkeeping.aiAttached?.length !== PLAYER_COUNT + 1) {
            throw new TypeError('보상 장부는 localOwner 와 aiAttached[9] 를 가져야 합니다.');
        }
        if (!(state instanceof SquidRewardState)) throw new TypeError('보상 상태는 SquidRewardState 여야 합니다.');
        if (!(store instanceof ScrambledSpStore)) throw new TypeError('패치판 보상은 SP 저장소가 필요합니다.');
        this.#pool = pool;
        this.#types = types.map((record) => ({ ...record }));
        this.#bookkeeping = bookkeeping;
        this.#state = state;
        this.#store = store;
        this.#hooks = hooks ?? {};
    }

    // 같은 실제 풀·장부이다(삭제 장부 연결 확인용).
    get pool() { return this.#pool; }
    get bookkeeping() { return this.#bookkeeping; }

    // 원본 vtable +0x80 을 물은 횟수이다(호출 순서 대조용 진단 값).
    get virtualCountCalls() { return this.#virtualCalls; }

    // 같은 World 의 현재 TYPE 표(188칸)를 복사 없이 빌린다. null/빈 배열이면 생성 시 사본으로 돌아간다. 잘못된 요청은 기존 연결을 바꾸기 전에 거부한다.
    bindCurrentTypes(types) {
        if (types !== null && (!Array.isArray(types) || (types.length !== 0 && types.length !== 188))) throw new RangeError('보상 현재 TYPE 대여의 판본/길이 오류');
        this.#currentTypes = types?.length ? types : null;
    }

    // PATCH 로딩 비용표의 최초 기록(188칸)을 빌린다. null/빈 배열이면 생성 시 비용 사본으로 돌아간다.
    bindLoadedCosts(types) {
        if (types !== null && (!Array.isArray(types) || (types.length !== 0 && types.length !== 188))) throw new RangeError('보상 로딩 비용표 대여의 판본/길이 오류');
        this.#loadedCostTypes = types?.length ? types : null;
    }

    // TYPE HP 의 선택된 원문 진단(expression, file, line)을 연결한다. null 은 명시 미연결이다.
    bindHitPointDiagnostics(assertion) {
        if (assertion !== null && typeof assertion !== 'function') throw new TypeError('보상 HP 진단은 함수여야 합니다.');
        this.#hitPointAssertion = assertion;
    }

    // PATCH 실제 세션의 00413bb0(owner, amount)을 연결한다. 이후 모든 지갑 가산은 이 처리기를 쓴다.
    bindPatchWallet(credit) {
        if (typeof credit !== 'function') throw new TypeError('PATCH 실제 AI 지갑 가산 처리 누락');
        this.#creditWallet = credit;
    }

    // 조회 시점의 현재 TYPE 표(빌린 표 또는 생성 시 사본)이다.
    #current() { return this.#currentTypes ?? this.#types; }

    // 현재 표의 지정 TYPE 기록이다. 물리 범위 밖은 오류다.
    #record(type) {
        const types = this.#current();
        if (type >= types.length) throw new RangeError('보상 TYPE 기록 범위 오류');
        return types[type];
    }

    // SID 범위와 현재 TYPE BYTE 범위를 확인한 슬롯 사본이다.
    #raw(sid) {
        if (!Number.isInteger(sid) || sid === 0 || sid >= this.#pool.capacity) throw new RangeError('보상 SID 범위 오류');
        const raw = this.#pool.slot(sid);
        if (raw[TYPE] >= this.#current().length) throw new RangeError('보상 타입 번호 범위 오류');
        return raw;
    }

    // 로더의 ftol(cost + 번호*23) 저장과 읽을 때의 감산을 합친 정수 단가이다(DAT_0059ab50). 빌린 로딩 표가 있으면 그 표, 없으면 생성 시 사본이다.
    typeCostUnits(type) {
        const costs = this.#loadedCostTypes ?? this.#types;
        if (!Number.isInteger(type) || type < 0 || type >= costs.length) throw new RangeError('보상 로딩 비용표 번호 범위 오류');
        const encoded = Math.fround(costs[type].cost ?? 0) + type * COST_BIAS;
        if (encoded < -2147483648 || encoded >= 2147483648) throw new RangeError('보상 패치 비용 표 범위 오류');
        return (Math.trunc(encoded) - type * COST_BIAS) | 0;
    }

    // vtable +0x80 의 개수이다. 훅이 있으면 파생 구현, 없으면 base(상태 상위 3비트)다.
    #count(sid) {
        this.#virtualCalls = (this.#virtualCalls + 1) >>> 0;
        if (typeof this.#hooks.virtualCount === 'function') {
            const count = this.#hooks.virtualCount(sid);
            if (!Number.isInteger(count)) throw new TypeError('보상 가상 개수는 정수여야 합니다.');
            return count | 0;
        }
        return this.#raw(sid)[STATE] >> 5;
    }

    // 0049fce0: 선택한 TYPE 기록의 최대 HP 이다. checking 진단 뒤 현재 약화/사제 번호를 같은 기록과 그때 현재 표 기준으로 다시 읽는다.
    typeHitPoints(type) {
        const selectedBase = this.#current();
        const record = this.#record(type);
        const state = this.#state;
        return readRiftHitPoints(record, {
            checking: () => state.currentChecking(),
            quarter: () => state.currentPriestQuarterHp(),
            priest: () => state.currentPriestType(),
            dais: () => state.currentDaisType(),
        }, {
            // 원문 getNum 이 소비되는 시점의 현재 표 기준 번호이다.
            number: () => currentRiftTypeNumber(record, selectedBase, this.#current()),
            // 호출 순간 선택된 진단 장치를 끝까지 쓴다.
            assertion: (expression, file, line) => {
                const selected = this.#hitPointAssertion;
                if (!selected) throw new Error('보상 TYPE HP 의 실제 진단 장치 미연결');
                selected(expression, file, line);
            },
        });
    }

    // 004adc60: 다이스 부류(flags2 0x400000)는 객체 +40 bit0, 그 밖은 타입 flags1 0x10 이 HP 보유를 정한다.
    hasHitPoints(sid) {
        const raw = this.#raw(sid);
        const type = this.#record(raw[TYPE]);
        return ((type.flags2 >>> 0) & DAIS_KIND) ? (raw[DAIS_EXTRA] & 1) !== 0 : ((type.flags1 >>> 0) & HAS_HP) !== 0;
    }

    // 004adcb0: 다이스 부류는 켜져 있으면 현재 제단 TYPE 의 HP, 꺼져 있으면 0 이고 그 밖은 자기 TYPE HP 이다.
    maxHitPoints(sid) {
        const raw = this.#raw(sid);
        const type = this.#record(raw[TYPE]);
        if (!((type.flags2 >>> 0) & DAIS_KIND)) return this.typeHitPoints(raw[TYPE]);
        return (raw[DAIS_EXTRA] & 1) ? this.typeHitPoints(this.#state.currentAltarType()) : 0;
    }

    // 곡선 항 수: 현재 제단/다이스 번호면 개수를 하나 줄인다(DWORD 감김).
    #terms(type, count) {
        const state = this.#state;
        return (type === state.currentAltarType() || type === state.currentDaisType()) ? (count - 1) | 0 : count;
    }

    // 0044b220: 같은 float 단가를 n+1 번 누적한 합(정확 범위만)을 0방향 절삭한다.
    #costCurve(type, count) {
        if (type >= this.#current().length) throw new RangeError('보상 비용 타입 범위 오류');
        const n = this.#terms(type, count);
        if (n < 0) return 0;
        if (n >= MAX_TERMS) throw new RangeError('보상 비용 곡선 항 수 범위 오류');
        const sum = Math.fround(this.typeCostUnits(type)) * (n + 1);
        if (Math.abs(sum) >= EXACT_LIMIT) throw new RangeError('보상 비용 곡선 누적 범위 오류');
        return ftol(sum);
    }

    // 0043cb30: 해체 환불용 float 비용이다. 합계를 x87 에 둔 채 너겟 외에는 1/4 을 곱하고 float 로 저장한다.
    #salvageCost(type, count) {
        if (type >= this.#current().length) throw new RangeError('보상 환불 타입 범위 오류');
        const n = this.#terms(type, count);
        let sum = 0;
        if (n >= 0) {
            if (n >= MAX_TERMS) throw new RangeError('보상 비용 곡선 항 수 범위 오류');
            sum = Math.fround(this.typeCostUnits(type)) * (n + 1);
            if (Math.abs(sum) >= EXACT_LIMIT) throw new RangeError('보상 비용 곡선 누적 범위 오류');
        }
        return Math.fround(type !== this.#state.currentNuggetType() ? sum * SALVAGE_SCALE : sum);
    }

    // 0044c240: 메뉴가 보존한 명시 TYPE/가상 개수와 현재 HP 로 환불만 계산한다(SP 지급·새 가상 개수 조회 없음).
    salvageRefund(sid, type, count) {
        if (type === this.#state.currentAliasType()) return 0;
        const cost = this.#salvageCost(type, count);
        if (!this.hasHitPoints(sid)) return ftol(cost);
        const raw = this.#raw(sid);
        const hp = (raw[HP] | (raw[HP + 1] << 8) | (raw[HP + 2] << 16) | (raw[HP + 3] << 24)) | 0;
        const max = this.maxHitPoints(sid);
        // 절반 미만이면 환불 없음. 반 값은 signed 2 나눗셈(0방향)이다.
        if (this.#state.currentHalfHealthMode() && hp < Math.trunc(max / 2)) return 0;
        // 최대 HP 0 은 x87 무한대/NaN 의 절삭 하위 DWORD 0 이다(기계어 확인).
        if (max === 0) return 0;
        // HP×비용을 float 로 저장한 뒤 최대 HP 로 나눈다.
        const product = Math.fround(hp * cost);
        return ftol(product / max);
    }

    // 상태를 바꾸지 않고 비용/지급액을 계산한다. 첫 TYPE 은 가상 개수 전에 저장하고, 별칭·환불 TYPE 은 원문 순서의 현재 BYTE 를 읽는다.
    plan(sid, recipient, salvage) {
        const raw = this.#raw(sid);
        if (!Number.isInteger(recipient) || recipient < 0 || recipient > PLAYER_COUNT) throw new RangeError('보상 수신자 번호는 0~8이어야 합니다.');
        const state = this.#state;
        const type = raw[TYPE];
        if (type === state.currentAltarType() || (!salvage && type === state.currentSilentType())) return { applied: false, cost: 0, refund: 0 };
        let cost = this.#costCurve(type, this.#count(sid));
        // 가상 개수 뒤의 현재 TYPE BYTE 로 성장형 별칭을 판정하고, 두 번째 개수 뒤의 현재 레인 바리케이드 번호를 읽는다.
        if (this.#pool.slot(sid)[TYPE] === state.currentAliasType()) {
            const count = this.#count(sid);
            cost = this.#costCurve(state.currentSilentType(), count);
        }
        if (cost === 0) return { applied: false, cost: 0, refund: 0 };
        let refund;
        if (salvage) {
            // PATCH 0044c37f 는 가상 개수 전에 현재 TYPE 을 저장한다.
            const refundType = this.#pool.slot(sid)[TYPE];
            const count = this.#count(sid);
            refund = this.salvageRefund(sid, refundType, count);
        } else {
            refund = Math.trunc((Math.imul(state.currentPercent(), cost >>> 0) | 0) / 100) | 0;
        }
        if (recipient === 0) refund = 0;
        return { applied: true, cost, refund };
    }

    // 00413bb0: 실제 지갑 처리기가 있으면 그것, 없으면 AI 가 부착된 소유자 1~8 의 독립 지갑에 정수 금액을 더한다(DWORD 감김·하한).
    #addWallet(owner, amount) {
        const credit = this.#creditWallet;
        if (credit) {
            credit(owner, amount);
            return;
        }
        const state = this.#state;
        if (state.aiMirrorDisabled || owner < 1 || owner > PLAYER_COUNT || !this.#bookkeeping.aiAttached[owner]) return;
        let wallet = (state.aiWallet[owner] + amount) | 0;
        if (state.walletClamp && wallet <= WALLET_FLOOR) wallet = WALLET_FLOOR;
        state.aiWallet[owner] = wallet;
    }

    // 너겟 내용물 삭제가 SP 를 바꾸기 전에 따로 하는 AI 지갑 정수 선가산이다(SP 표시/난수 갱신 없음).
    creditWallet(owner, amount) {
        if (!Number.isInteger(amount)) throw new TypeError('지갑 가산액은 정수여야 합니다.');
        this.#addWallet(owner, amount | 0);
    }

    // 0040eec0/0041df80: 소유자 SP 에 delta(float, 수)를 더한다. JS 수의 NaN 은 페이로드가 없으므로 0x7fc00000 으로 본다(비트 보존은 addSpBits).
    addSp(owner, delta) {
        if (typeof delta !== 'number') throw new TypeError('SP 가산액은 수여야 합니다.');
        this.addSpBits(owner, floatToBits(Math.fround(delta)));
    }

    // 0040eec0/0041df80 의 비트 단위 몸체이다: SP 읽기 → (고정 모드면 delta = 194162 − SP) → float 합 저장 → delta 절삭 DWORD 를 AI 지갑 → SP 표시 갱신.
    // NaN SP/delta 의 quiet 비트 전파와 무한대 상쇄의 실수 부정값을 원본 비트대로 저장한다.
    addSpBits(owner, deltaBits) {
        if (!Number.isInteger(deltaBits) || deltaBits < 0 || deltaBits > 0xffffffff) throw new RangeError('SP 가산 비트는 DWORD 여야 합니다.');
        const state = this.#state;
        let amount = deltaBits >>> 0;
        const fixed = state.currentFixedSpA() || state.currentFixedSpB();
        const spBits = this.#store.get(owner);
        const sp = bitsToFloat(spBits);
        // 고정 모드: double 상수에서 현재 SP 를 빼 float 로 저장한다.
        if (fixed) amount = x86FloatBits(() => FIXED_SP - sp, null, spBits);
        const delta = bitsToFloat(amount);
        const sum = x86FloatBits(() => sp + delta, spBits, amount);
        const mirror = spMirrorFtol(delta);
        this.#store.set(owner, sum);
        this.#addWallet(owner, mirror);
        if (typeof this.#hooks.refreshSp === 'function') this.#hooks.refreshSp();
    }

    // 0044c2f0: 계획한 지급을 SP 로 주고(로컬/다른 소유자 모두 패치판 SP 가산) 그 뒤 현재 샘 풀에 비용−지급을 더한다.
    refund(sid, recipient, salvage) {
        const plan = this.plan(sid, recipient, salvage);
        if (!plan.applied) return plan;
        // 로컬 소유자는 0040eec0, 다른 소유자는 0041df80 이며 패치판에서는 같은 SP 가산 몸체다.
        if (recipient === this.#bookkeeping.localOwner) this.addSp(recipient, Math.fround(plan.refund));
        else this.addSp(recipient, Math.fround(plan.refund));
        const state = this.#state;
        state.storeGeyserPool((state.currentGeyserPool() + ((plan.cost - plan.refund) | 0)) | 0);
        return plan;
    }
}
