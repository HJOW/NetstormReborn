// 실제 미션 옵션의 범위·선택 TYPE 비용·파생값·밀도·타입 사용표 초기화를 옮긴 모듈이다(cpppj `o/RawMissionBattleOptions.cpp/.h`, 10.78 전용, 원문 0041cb90/0041cc10 초기화·004b4860 범위·0041c8b0 비용·0041ca70 값·004b4900 파생).
//  - resetMissionBattleOptions(options): 87바이트 옵션의 BYTE 0..22 와 63..86 을 0 으로 쓰고(BYTE 23..62 와 양쪽 이웃은 보존) BYTE 71..78 에 색 0..7 을, BYTE 0..21 에 22개 기본 선택을 설치한다. 87바이트가 아니면 쓰기 전에 TypeError.
//  - RawMissionBattleOptions: 같은 현재 저장소를 빌려(옵션 사본을 만들지 않음) 매 호출마다 현재 값을 읽는다. state = { options(87바이트), types(188개 { cost }), typeUse(256바이트), rangeTable(float 비트 Uint32Array 3 또는 4개: 14/22/30/38),
//    selectedType()(현재 비용 수정 대상 getter), range: { bits }, rangeSquared: { bits }, value: { value } }. float 는 NaN 페이로드를 보존하기 위해 비트로 보관한다.
//    updateRange() [004b4860]: options[4]가 고른 표 항목을 x87 FLD/FSTP32 처럼(signaling NaN 은 quiet 비트를 켜서) 저장하고, min(표[2], 현재 범위) 후보보다 표[0]이 크면 표[0]로, 아니면 표[2] 가 현재 범위보다 작으면 표[2]로 바꾼 뒤 제곱을 쓴다(NaN 은 std::clamp/min/max 로 바꾸지 않고 원문 비교 분기 그대로, NaN 제곱은 피연산자 NaN 을 보존).
//    typeCost() [0041c8b0]: options[0x10] → 1000/2000/3000/5000. currentValue() [0041ca70]: options[0xf] → 0/25/50/75/100/150. minimumDensity()/maximumDensity(): options[7] → 10/15/20/25 와 30/35/40/45 (서로 다른 현재 선택표).
//    initializeDensity(random, minimum, maximum, percent): State7 서버의 두 한계 쓰기 → 각 호출이 현재 한계의 반값 범위로 난수를 두 번 뽑아 DWORD 감김 합계를 percent 에 쓴다.
//    setSelectedTypeCost(cost): 현재 selectedType 의 typed cost 필드 쓰기(범위 밖 TYPE 은 RangeError). applyDerived(): 범위 → 비용 조회 → 현재 TYPE 쓰기 → 현재 파생값 쓰기. clearTypes(): 현재 256바이트만 전부 0 (이웃 보존).
// 옵션 인덱스가 표 밖이면 원본의 스택/인접 전역 읽기라 호스트에서는 RangeError 로 구별한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E37: 미션 전투 옵션 이식으로 추가)

// 원문 상수: PE 0052f5e0 부터 24바이트 간격의 기본 선택 BYTE 22개, 비용/값/밀도 표, 옵션·TYPE·사용표·범위표 크기이다. (2026-11-08 WB04-E37)
const INITIAL_OPTIONS = Object.freeze([0, 2, 2, 0, 3, 0, 2, 1, 1, 0, 0, 1, 0, 1, 1, 2, 2, 1, 2, 2, 0, 0]);
const TYPE_COSTS = Object.freeze([1000, 2000, 3000, 5000]);
const VALUES = Object.freeze([0, 25, 50, 75, 100, 150]);
const MINIMUM_DENSITY = Object.freeze([10, 15, 20, 25]);
const MAXIMUM_DENSITY = Object.freeze([30, 35, 40, 45]);
const OPTION_BYTES = 87;
const TYPE_COUNT = 188;
const TYPE_USE_BYTES = 256;
const QUIET_BIT = 0x00400000;

// float 비트와 값 사이 변환이다. (2026-11-08 WB04-E37)
const view = new DataView(new ArrayBuffer(4));
function floatOf(bits) {
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}
function floatBits(value) {
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 004b4867 의 FLD/FSTP32 는 signaling NaN 을 quiet 로 저장한다. 다른 비트는 보존한다. (2026-11-08 WB04-E37)
function storeX87(bits) {
    const value = bits >>> 0;
    return (value & 0x7f800000) === 0x7f800000 && (value & 0x007fffff) !== 0 ? (value | QUIET_BIT) >>> 0 : value;
}

// 원본은 전체 87바이트를 지우지 않는다. 미작성 예약 40바이트와 양쪽 이웃은 건드리지 않는다. (2026-11-08 WB04-E37)
export function resetMissionBattleOptions(options) {
    if (!options || options.length !== OPTION_BYTES) throw new TypeError('미션 옵션 초기화의 PATCH 87BYTE 계약 오류');
    options.fill(0, 0, 23);
    options.fill(0, 63);
    // 실제 색상 0..7 을 BYTE 71..78 에 설치한다.
    for (let index = 0; index < 8; index += 1) options[71 + index] = index;
    INITIAL_OPTIONS.forEach((value, index) => { options[index] = value; });
}

// 범위 밖 인덱스가 원본 스택/인접 전역을 읽는 손상 경로는 호스트의 명시 오류로 구분한다. (2026-11-08 WB04-E37)
export class RawMissionBattleOptions {
    #state;

    // PATCH 87 옵션 / 188 TYPE / 256 사용표 / 3~4 float 를 정확히 빌린다(실제 로그인은 기본 인덱스 3 을 위해 4개를 공급).
    constructor(state) {
        if (state.options.length !== OPTION_BYTES || state.types.length !== TYPE_COUNT || state.typeUse.length !== TYPE_USE_BYTES || (state.rangeTable.length !== 3 && state.rangeTable.length !== 4)) {
            throw new TypeError('미션 옵션의 실제 PATCH 저장소 크기 오류');
        }
        this.#state = state;
    }

    // 원본은 초기 선택값을 먼저 저장한다. NaN 에서 clamp/min/max 로 바꾸지 않는다.
    updateRange() {
        const state = this.#state;
        const index = state.options[4];
        if (index >= state.rangeTable.length) throw new RangeError('미션 범위 옵션 인덱스');
        state.range.bits = storeX87(state.rangeTable[index]);
        const low = floatOf(state.rangeTable[0]);
        const high = floatOf(state.rangeTable[2]);
        const current = floatOf(state.range.bits);
        const candidate = high < current ? high : current;
        if (low > candidate) state.range.bits = storeX87(state.rangeTable[0]);
        else if (high < floatOf(state.range.bits)) state.range.bits = storeX87(state.rangeTable[2]);
        // NaN 제곱은 피연산자 NaN(이미 quiet)을 보존한다.
        const range = floatOf(state.range.bits);
        state.rangeSquared.bits = Number.isNaN(range) ? state.range.bits : floatBits(Math.fround(range * range));
    }

    // 옵션 BYTE 는 호출 때마다 같은 원문에서 읽는다.
    typeCost() {
        const index = this.#state.options[0x10];
        if (index >= TYPE_COSTS.length) throw new RangeError('미션 TYPE 비용 옵션 인덱스');
        return TYPE_COSTS[index];
    }

    // 이후 설정 변경을 이전 파생값 캐시로 가리지 않는다.
    currentValue() {
        const index = this.#state.options[0xf];
        if (index >= VALUES.length) throw new RangeError('미션 파생값 옵션 인덱스');
        return VALUES[index];
    }

    // 원문 옵션 BYTE 를 매 getter 마다 다시 읽으며 잘못된 인덱스는 호스트 오류로 구별한다. 상한 조회를 하한 조회의 선택값으로 고정하지 않는다.
    minimumDensity() {
        const index = this.#state.options[7];
        if (index >= MINIMUM_DENSITY.length) throw new RangeError('일반 전투 최소 밀도 옵션 범위');
        return MINIMUM_DENSITY[index];
    }

    maximumDensity() {
        const index = this.#state.options[7];
        if (index >= MAXIMUM_DENSITY.length) throw new RangeError('일반 전투 최대 밀도 옵션 범위');
        return MAXIMUM_DENSITY[index];
    }

    // PE 004b7666..769e 의 signed 반값·두 004558f0 호출과 현재 공유 전역 쓰기를 그대로 연결한다. minimum/maximum/percent 는 { value } 보관 객체이다.
    initializeDensity(random, minimum, maximum, percent) {
        minimum.value = this.minimumDensity();
        maximum.value = this.maximumDensity();
        // 각 호출이 같은 현재 한계를 다시 읽고 원문 DWORD 범위 차이/합계 감김을 보존한다.
        const sample = () => {
            const low = Math.trunc(minimum.value / 2) | 0;
            const high = Math.trunc(maximum.value / 2) | 0;
            return (random.next(((high >>> 0) - (low >>> 0)) >>> 0) + (low >>> 0)) >>> 0;
        };
        const first = sample();
        const second = sample();
        percent.value = (first + second) >>> 0;
    }

    // 원본 0049a840 의 현재 번호를 쓰기 직전에 확인한다.
    setSelectedTypeCost(cost) {
        const type = this.#state.selectedType();
        if (type >= this.#state.types.length) throw new RangeError('미션 비용 대상 TYPE 범위');
        this.#state.types[type].cost = Math.fround(cost);
    }

    // 같은 호출 순서를 작은 실제 몸체로 연결하며 별도 성공 표식을 만들지 않는다.
    applyDerived() {
        this.updateRange();
        const cost = this.typeCost();
        this.setSelectedTypeCost(cost);
        this.#state.value.value = this.currentValue();
    }

    // 원본 64DWORD 쓰기의 현재 256바이트 결과만 적용한다.
    clearTypes() {
        this.#state.typeUse.fill(0);
    }
}
