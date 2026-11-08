// 연소점 목표 개수의 원문 DWORD 감김과 signed IDIV 를 보관한 순수 정수 모듈이다(cpppj `o/RawBurningSourceCount.cpp/.h`, 10.78: 원문 4241a2 의 곱셈/덧셈 감김과 나눗셈 실패를 그대로 구별).
//  - burningSourceCount(hitPoints, maximum, width, height): 현재 HP 는 getter 전 저장값이며(flag 0x400 강제 1 은 호출자가 이 계산 뒤 현재 TYPE 에서 선택), 최대 HP ×2 의 하위 DWORD 를 signed /4 한 값 half 로 분모 = 최대 − half(감김), 분자 = HP × 5(감김)를 signed 나눗셈해
//    목표 = 6 − 몫(감김)이고 행 수 = (너비 + 높이)(감김)/5 로 `max(1, min(목표, 행 수))` 를 돌려준다. 분모가 0 이면 `BurningSourceCountError('zero')`, 분자가 INT_MIN 이고 분모가 −1 이면 'overflow' 이다(원본 IDIV 예외).
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E183: Burning 연소점 개수 이식으로 추가)

// 원문 IDIV 의 두 예외를 구별하는 오류이다. kind 는 'zero'(분모 0) 또는 'overflow'(INT_MIN / −1)이다. (2026-11-09 WB04-E183)
export class BurningSourceCountError extends Error {
    // 오류 종류와 문구를 기록한다.
    constructor(kind, message) {
        super(message);
        this.kind = kind;
    }
}

// C++ signed 오버플로/0 나눗셈을 직접 실행하지 않고 x86 의 감김과 IDIV 순서를 그대로 계산한다. (2026-11-09 WB04-E183)
export function burningSourceCount(hitPoints, maximum, width, height) {
    // x86 LEA 의 하위 DWORD 를 저장한 뒤 signed /4 를 수행한다.
    const twice = Math.imul(maximum | 0, 2);
    const half = Math.trunc(twice / 4) | 0;
    const divisor = ((maximum | 0) - half) | 0;
    const numerator = Math.imul(hitPoints | 0, 5);
    if (!divisor) throw new BurningSourceCountError('zero', '연소점 원문 IDIV의0 나눗셈');
    if (numerator === -2147483648 && divisor === -1) throw new BurningSourceCountError('overflow', '연소점 원문 IDIV의signed 몫 범위 초과');
    const quotient = Math.trunc(numerator / divisor) | 0;
    const wanted = (6 - quotient) | 0;
    const rows = Math.trunc((((width | 0) + (height | 0)) | 0) / 5) | 0;
    // 원문 계산이 끝난 뒤에만 목표 행을 1 이상으로 제한한다.
    return Math.max(1, Math.min(wanted, rows));
}
