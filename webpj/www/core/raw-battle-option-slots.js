// 실제 87바이트 전투 옵션(BattleOptions)의 8개 슬롯 색·랭크·특수 상태와 일반 SP/간헐천 선택 표를 원문 폭으로 읽고 쓰는 모듈이다
// (cpppj `o/RawBattleOptionSlots.cpp/.h`·`o/RawBattleResourceOptions.cpp/.h`, 10.78 전용: 0041c4f0·0041c520·0041c550·0041c580·0041c5b0·0041c6a0, 0041c800·0041c840·0041c870).
// 사용(options 는 현재 전투 옵션 Uint8Array 이며 별도 사본을 만들지 않고 같은 배열을 빌려 쓴다. slot 은 signed 정수이다):
//   increaseBattleSlotColor(options, slot) / decreaseBattleSlotColor(options, slot) → +0x47+slot BYTE. 증가는 BYTE 증가를 먼저 저장하고 7 보다 클 때만 0 을 쓰고, 감소는 BYTE 감소를 먼저 저장하고 0xff 일 때만 7 을 쓴다. 성공 1, 무효 슬롯 0(읽기 없음).
//   increaseBattleSlotRank(options, slot) / decreaseBattleSlotRank(options, slot) → +0x4f+slot BYTE. 증가는 zero 확장 값+1 이 23 보다 크면 0, 감소는 zero 확장 값-1 이 음수일 때만 23 으로 바꿔 최종 BYTE 만 쓴다. 성공 1, 무효 슬롯 0.
//   battleSlotRank(options, slot) → +0x4f BYTE 를 24칸 표(0..10·15..50 5단계·60·70·80·90·100)로 바꾼다. 무효 슬롯 0, 표 밖 값은 호스트 오류(RangeError)이다.
//   toggleBattleSlotSpecial(options, slot) → +0x3f BYTE 가 정확히 1 이면 0, 그 밖이면 1(XOR 가 아님). 성공 1, 무효 슬롯 0.
//   battleIncomePeriod(options) / battleIncomeAmount(options) / battleGeyserPeriod(options) → +0x14/+0x15/+0x13 의 현재 선택값을 0/30/60/120/300/600초·500/1000/2500/5000·0/30/60/120/300초로 읽는다(옵션이 87바이트가 아니거나 선택 범위 밖이면 RangeError).
// 유효 슬롯 관문(signed 0..7) 뒤에만 실제 BYTE 에 접근하며 원문에 없는 전체 초기화/범위 보정을 하지 않는다. 필드가 옵션 물리 밖이면 쓰기·읽기 전에 RangeError 로 구분한다(Native 의 메모리 오염 대신 호스트 오류).
// 원본 관측 fixture 가 없어 cpppj 소스 계약(주소·폭·순서)을 tests/raw-battle-option-slots.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E558: 전투 옵션 슬롯·자원 옵션 이식)

// 슬롯 수와 슬롯별 BYTE 필드의 옵션 안 위치(색·랭크·특수 상태)이다. (2026-11-15 WB04-E558)
const SLOT_COUNT = 8;
const COLOR_FIELD = 0x47;
const RANK_FIELD = 0x4f;
const SPECIAL_FIELD = 0x3f;
// 랭크 BYTE 의 순환 상한(23)과 색 BYTE 의 순환 상한(7)이다. (2026-11-15 WB04-E558)
const RANK_LAST = 23;
const COLOR_LAST = 7;
// 0041c5b0 의 실제 지역 24 DWORD 표이다(랭크 BYTE → 표시/피해 단계). (2026-11-15 WB04-E558)
export const BATTLE_SLOT_RANKS = Object.freeze([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 35, 40, 45, 50, 60, 70, 80, 90, 100]);
// 옵션의 원문 크기(87 BYTE)이다. 자원 옵션 조회만 이 크기를 요구한다. (2026-11-15 WB04-E558)
export const BATTLE_OPTIONS_BYTES = 87;
// 일반 SP 지급 간격·지급액·간헐천 간격의 실제 표와 옵션 안 위치이다. (2026-11-15 WB04-E558)
const INCOME_PERIOD_FIELD = 20;
const INCOME_AMOUNT_FIELD = 21;
const GEYSER_PERIOD_FIELD = 19;
const INCOME_PERIODS = Object.freeze([0, 30, 60, 120, 300, 600]);
const INCOME_AMOUNTS = Object.freeze([500, 1000, 2500, 5000]);
const GEYSER_PERIODS = Object.freeze([0, 30, 60, 120, 300]);

// 슬롯이 signed 32비트 정수가 아니면 호출 규약 위반이다(원문 인자는 int 한 개). (2026-11-15 WB04-E558)
function checkSlot(slot) {
    if (!Number.isInteger(slot) || slot < -0x80000000 || slot > 0x7fffffff) throw new RangeError('전투 옵션 슬롯은 signed 32비트 정수여야 합니다.');
}

// 원문 signed 0..7 관문이다. 옵션 배열 검사보다 먼저 판정하며 무효 슬롯은 읽기/쓰기를 하지 않는다. (2026-11-15 WB04-E558)
function valid(slot) {
    checkSlot(slot);
    return slot >= 0 && slot < SLOT_COUNT;
}

// 유효 슬롯 호출자가 실제 필드에 접근하는 위치에서만 옵션의 물리 길이를 확인하고 BYTE 위치를 돌려준다. (2026-11-15 WB04-E558)
function position(options, field, slot) {
    const at = field + slot;
    if (at >= options.length) throw new RangeError('BattleOptions 슬롯의 실제 BYTE 물리 범위');
    return at;
}

// 색 증가(0041c4f0): 255+1 은 BYTE 0 으로 먼저 감긴 뒤 7 보다 클 때만 0 을 쓴다(감긴 0 이면 그대로 둔다). (2026-11-15 WB04-E558)
export function increaseBattleSlotColor(options, slot) {
    if (!valid(slot)) return 0;
    const at = position(options, COLOR_FIELD, slot);
    const next = (options[at] + 1) & 0xff;
    options[at] = next;
    if (next > COLOR_LAST) options[at] = 0;
    return 1;
}

// 색 감소(0041c520): 0-1 의 BYTE 0xff 만 7 로 바꾸고 그 밖의 비정상 입력값을 임의로 clamp 하지 않는다. (2026-11-15 WB04-E558)
export function decreaseBattleSlotColor(options, slot) {
    if (!valid(slot)) return 0;
    const at = position(options, COLOR_FIELD, slot);
    const next = (options[at] - 1) & 0xff;
    options[at] = next;
    if (next === 0xff) options[at] = COLOR_LAST;
    return 1;
}

// 랭크 증가(0041c550): BYTE 저장 전에 zero 확장 값+1 의 23 초과를 판정하므로 24..255 는 모두 0 이 된다. (2026-11-15 WB04-E558)
export function increaseBattleSlotRank(options, slot) {
    if (!valid(slot)) return 0;
    const at = position(options, RANK_FIELD, slot);
    let next = options[at] + 1;
    if (next > RANK_LAST) next = 0;
    options[at] = next & 0xff;
    return 1;
}

// 랭크 감소(0041c580): 0 에서만 23 으로 순환하며 큰 입력값은 그대로 1 감소한다. (2026-11-15 WB04-E558)
export function decreaseBattleSlotRank(options, slot) {
    if (!valid(slot)) return 0;
    const at = position(options, RANK_FIELD, slot);
    let next = options[at] - 1;
    if (next < 0) next = RANK_LAST;
    options[at] = next & 0xff;
    return 1;
}

// 랭크 조회(0041c5b0): 유효 슬롯만 +0x4f BYTE 를 24칸 표로 바꾸며 표 밖 값은 Native 의 메모리 오염 대신 호스트 오류로 드러낸다. (2026-11-15 WB04-E558)
export function battleSlotRank(options, slot) {
    if (!valid(slot)) return 0;
    const index = options[position(options, RANK_FIELD, slot)];
    if (index >= BATTLE_SLOT_RANKS.length) throw new RangeError('전투 슬롯 랭크 표의 실제 범위');
    return BATTLE_SLOT_RANKS[index];
}

// 특수 상태 토글(0041c6a0): 원문의 `!=1` 비교를 그대로 써서 정확히 1 이면 0, 그 밖(0·2..255)이면 1 이다. (2026-11-15 WB04-E558)
export function toggleBattleSlotSpecial(options, slot) {
    if (!valid(slot)) return 0;
    const at = position(options, SPECIAL_FIELD, slot);
    options[at] = options[at] !== 1 ? 1 : 0;
    return 1;
}

// 일반 자원 표 조회의 공통 몸체이다. 옵션 크기가 87 이 아니거나 선택 BYTE 가 표 밖이면 Native 의 범위 밖 스택 읽기 대신 RangeError 로 구분한다. (2026-11-15 WB04-E558)
function selection(options, field, values) {
    if (options.length !== BATTLE_OPTIONS_BYTES || options[field] >= values.length) throw new RangeError('전투 자원 옵션의 실제 표 범위');
    return values[options[field]];
}

// 0041c800: +0x14 의 현재 선택 0..5 를 0/30/60/120/300/600초로 읽는다. (2026-11-15 WB04-E558)
export function battleIncomePeriod(options) {
    return selection(options, INCOME_PERIOD_FIELD, INCOME_PERIODS);
}

// 0041c840: +0x15 의 현재 선택 0..3 을 500/1000/2500/5000 으로 읽는다(패킷 종류 2 검증에서도 같은 표를 쓴다). (2026-11-15 WB04-E558)
export function battleIncomeAmount(options) {
    return selection(options, INCOME_AMOUNT_FIELD, INCOME_AMOUNTS);
}

// 0041c870: +0x13 의 현재 선택 0..4 를 0/30/60/120/300초로 읽는다. (2026-11-15 WB04-E558)
export function battleGeyserPeriod(options) {
    return selection(options, GEYSER_PERIOD_FIELD, GEYSER_PERIODS);
}
