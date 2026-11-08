// 원본 352바이트(0x160) Deck 의 공유 초기화(44f200)·항목 추가(44f290)·열기/닫기(44f250/44f280)와 독립 벽시각 난수 추첨(44f510)을 옮긴 모듈이다(cpppj `o/RawDeckDraw.cpp/.h`, 10.78 전용).
// Deck 은 5바이트 항목(type, chance, power, 255, 선택 횟수) 최대 66개와 끝의 다섯 DWORD(+0x14c 개수·+0x150 합계·+0x154 남은 선택·+0x158 seed·+0x15c 열림)로 이루어진다. Player 의 Deck 과 스택 Deck 이 같은 본문을 쓴다.
//  - RawDeckDraw.initialize(raw): 다섯 DWORD 만 0 으로 만든다(항목과 미초기화 꼬리는 보존). RawDeckDraw.append(raw, type, chance, assertion): 같은 type 이 있으면 진단(DuplicateType)을 부르고도 계속 추가하며, 가득 차거나 개수가 음수면 진단(Capacity) 뒤 거부한다.
//    새 항목은 power 1·BYTE 로 줄인 type/chance·255·선택 0 이고 type 이 255 초과/chance 가 BYTE 에 안 맞으면 진단(TypeTruncated/ChanceTruncated)을 부르며 합계에 chance(DWORD 비트)를 더하고 개수를 올린다.
//  - open(seed): seed 가 0 이면 같은 게임 난수를 다시 한 번 소비(0..9999)해 seed 를 쓰고 열림 bit0 을 켠다. close(): 열림 bit0 만 지운다(다른 열림 비트와 seed 는 보존).
//  - draw(power): 열려 있지 않으면 진단(0xe5), 현재 합계가 저장된 합계와 다르면 로그(mismatch) 뒤 합계를 다시 쓴다. 호출 시점 벽시각으로 만든 별도 난수(Open 에 저장한 seed 는 쓰지 않음)로 [0, 합계) 값을 뽑아 power 가 양수인 항목의 chance 를 차례로 빼며
//    음수가 되는 항목을 고르고 그 항목의 선택 횟수와 남은 선택 DWORD 를 올린 뒤 type 을 돌려준다(없으면 0). power 가 `{ value }` 이면 선택 항목이 상품 TYPE 일 때 0, 아니면 -1(없으면 0)을 쓴다.
//  - RawDeckDraw.wallSeed(seconds): 시각 × 1013 의 signed64 절삭 하위 DWORD `low` 로 `low × 33 + (low >> 4 부호 확장)` (DWORD 감김)이다.
// hooks = { wallSeconds()→double, assertion(line), mismatch(stored, sum), isProduct(type)→bool }, global 은 같은 게임 난수이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E105: Deck 추첨 이식으로 추가)
import { GameRandom } from './random.js';

// 원본 Deck 객체의 전체 물리 크기와 항목 폭/최대 개수, 끝 다섯 DWORD 의 위치이다. (2026-11-08 WB04-E105)
export const PRODUCTION_DECK_BYTES = 0x160;
const ENTRY_BYTES = 5;
const ENTRY_CAPACITY = 66;
const COUNT_AT = 0x14c;
const CHANCE_AT = 0x150;
const REMAINING_AT = 0x154;
const SEED_AT = 0x158;
const OPEN_AT = 0x15c;
// 기존 목록 추가의 원본 진단 종류이다(C++ ProductionDeckAssertion 과 같은 순서). (2026-11-08 WB04-E105)
export const DeckAssertion = Object.freeze({ DuplicateType: 0, Capacity: 1, TypeTruncated: 2, ChanceTruncated: 3, PowerTruncated: 4, RemainingTruncated: 5 });
// Draw 의 열림 미확인 진단 번호, 시각 배율, 상위 부호 확장 마스크, ftol 경계이다. (2026-11-08 WB04-E105)
const NOT_OPEN = 0xe5;
const WALL_SCALE = 1013;
const INT64_BOUNDARY = 2 ** 63;

// 원본 DWORD 를 정렬과 무관하게 읽는다. (2026-11-08 WB04-E105)
const read = (raw, offset) => (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;

// 원본 DWORD 만 쓰고 인접 항목과 미초기화 꼬리는 보존한다. (2026-11-08 WB04-E105)
function put(raw, offset, value) {
    // 낮은 바이트부터 쓴다.
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 실제 352바이트 이외 저장소는 받아들이지 않는다. (2026-11-08 WB04-E105)
function check(raw) {
    if (!raw || raw.length !== PRODUCTION_DECK_BYTES) throw new RangeError('Deck 의 실제 352바이트 누락');
}

// signed 음수 개수는 빈 순회이며 양수 물리 초과만 거부한다. (2026-11-08 WB04-E105)
function count(raw) {
    const value = read(raw, COUNT_AT) | 0;
    if (value > ENTRY_CAPACITY) throw new RangeError('Deck 의 66항목 물리 범위');
    return value;
}

// 선택된 외부 효과를 빈 성공으로 바꾸지 않는다. (2026-11-08 WB04-E105)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('Deck 추첨의 실제 효과 미연결');
    return hook;
}

// 현재 signed power 가 양수인 항목의 현재 BYTE chance 만 합산한다(power 0/음수는 제외). (2026-11-08 WB04-E105)
function sum(raw) {
    let total = 0;
    const limit = count(raw);
    // 각 원본 순회의 현재 개수만큼 본다.
    for (let index = 0; index < limit; index += 1) {
        const offset = index * ENTRY_BYTES;
        if (((raw[offset + 2] << 24) >> 24) > 0) total = (total + raw[offset + 1]) >>> 0;
    }
    return total;
}

// 같은 물리 Deck 을 빌려 열기/닫기/추첨을 수행하는 추첨기이다. (2026-11-08 WB04-E105)
export class RawDeckDraw {
    // 실제 항목 66개와 다섯 DWORD 의 단일 저장소이다. (2026-11-08 WB04-E105)
    #raw;
    // Open 의 0 seed 보충만 쓰는 같은 게임 난수이다. (2026-11-08 WB04-E105)
    #global;
    // 실제 시각·진단·로그·optional power 분류이다. (2026-11-08 WB04-E105)
    #hooks;

    // 같은 실제 물리 Deck 을 빌리며 어떤 필드도 초기화하지 않는다. (2026-11-08 WB04-E105)
    constructor(raw, global, hooks = {}) {
        check(raw);
        this.#raw = raw;
        this.#global = global;
        this.#hooks = hooks;
    }

    // 44f200: 원본 생성자의 쓰기 순서와 폭을 유지한다(다섯 DWORD 만 0). (2026-11-08 WB04-E105)
    static initialize(raw) {
        check(raw);
        put(raw, COUNT_AT, 0);
        put(raw, SEED_AT, 0);
        put(raw, CHANCE_AT, 0);
        put(raw, REMAINING_AT, 0);
        put(raw, OPEN_AT, 0);
    }

    // 44f290: 현재 Player 용 추가와 스택용 추가가 같은 본문을 쓴다. 중복 진단이 돌아와도 계속 순회하고 추가한다. (2026-11-08 WB04-E105)
    static append(raw, type, chance, assertion) {
        check(raw);
        const used = count(raw);
        // 최초 개수는 중복 진단 전에 보관한다.
        for (let index = 0; index < used; index += 1) if (raw[index * ENTRY_BYTES] === type) need(assertion)(DeckAssertion.DuplicateType);
        if (used < 0 || used >= ENTRY_CAPACITY) {
            need(assertion)(DeckAssertion.Capacity);
            throw new RangeError('Deck 추가의 물리 범위');
        }
        const offset = used * ENTRY_BYTES;
        raw[offset + 2] = 1;
        raw[offset] = type & 0xff;
        raw[offset + 1] = chance & 0xff;
        raw[offset + 3] = 255;
        raw[offset + 4] = 0;
        if (type > 255) need(assertion)(DeckAssertion.TypeTruncated);
        if (raw[offset + 1] !== (chance >>> 0)) need(assertion)(DeckAssertion.ChanceTruncated);
        put(raw, CHANCE_AT, (read(raw, CHANCE_AT) + (chance >>> 0)) >>> 0);
        put(raw, COUNT_AT, (used + 1) >>> 0);
    }

    // 44f250: Open 의 0 입력은 두 번째 전역 난수 소비를 유발할 수 있다. (2026-11-08 WB04-E105)
    open(seed) {
        if (!seed) seed = this.#global.next(10000);
        put(this.#raw, SEED_AT, seed);
        put(this.#raw, OPEN_AT, (read(this.#raw, OPEN_AT) | 1) >>> 0);
    }

    // 44f280: 다른 open 플래그와 저장 seed 는 지우지 않는다. (2026-11-08 WB04-E105)
    close() { put(this.#raw, OPEN_AT, (read(this.#raw, OPEN_AT) & ~1) >>> 0); }

    // 4bcab0: x87 ftol 의 무효 64비트 결과 8000000000000000 은 낮은 DWORD 가 0 이다. 곱 1013, 하위 DWORD, 부호 확장 >>4 와 *33 의 DWORD 감김을 보존한다. (2026-11-08 WB04-E105)
    static wallSeed(seconds) {
        const scaled = seconds * WALL_SCALE;
        let low = 0;
        if (Number.isFinite(scaled) && scaled >= -INT64_BOUNDARY && scaled < INT64_BOUNDARY) low = Number(BigInt.asUintN(32, BigInt(Math.trunc(scaled))));
        const shifted = ((low >>> 4) | ((low & 0x80000000) ? 0xf0000000 : 0)) >>> 0;
        return (Math.imul(low, 33) + shifted) >>> 0;
    }

    // 44f510: 합계 진단 뒤 현재 표를 다시 합산하고 시각 효과 뒤 현재 항목을 선택한다. power 는 `{ value }` 또는 null 이다. (2026-11-08 WB04-E105)
    draw(power = null) {
        const raw = this.#raw;
        const hooks = this.#hooks;
        if (!(raw[OPEN_AT] & 1)) need(hooks.assertion)(NOT_OPEN);
        if (sum(raw) !== read(raw, CHANCE_AT)) {
            const stored = read(raw, CHANCE_AT);
            const current = sum(raw);
            need(hooks.mismatch)(stored, current);
            put(raw, CHANCE_AT, sum(raw));
        }
        // 원본은 시각 공급 전에 chance 를 보관한다.
        const chance = read(raw, CHANCE_AT);
        const local = new GameRandom(RawDeckDraw.wallSeed(need(hooks.wallSeconds)()));
        let remaining = (chance | 0) > 0 ? local.next(chance) : 0;
        let index = 0;
        const limit = count(raw);
        // 현재 개수/항목은 시각 효과가 끝난 뒤 읽으며 선택 성공 뒤 남은 필드는 읽지 않는다.
        while (index < limit) {
            const offset = index * ENTRY_BYTES;
            if (((raw[offset + 2] << 24) >> 24) > 0) remaining = (remaining - raw[offset + 1]) >>> 0;
            if ((remaining | 0) < 0) break;
            index += 1;
        }
        if (index < limit) {
            const offset = index * ENTRY_BYTES;
            const type = raw[offset];
            raw[offset + 4] = (raw[offset + 4] + 1) & 0xff;
            put(raw, REMAINING_AT, (read(raw, REMAINING_AT) + 1) >>> 0);
            if (power) power.value = need(hooks.isProduct)(type) ? 0 : -1;
            return type;
        }
        if (power) power.value = 0;
        return 0;
    }
}
