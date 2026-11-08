// 사제 AI 의 다음 건설 TYPE 선택과 수집자 여유 비용/적 자금 조건을 옮긴 모듈이다(cpppj `o/RawAiBuildChoice.cpp/.h`, 10.78: 패치 00411a40·00411ad0·00411b40·004709d0·00470a30).
//  - new RawAiBuildChoice(memory, types, globals, hooks): memory 는 E164 `RawAiBrainMemory`(본체 소유자), types 는 현재 188 TYPE({ cost })의 같은 배열(복사하지 않는다, 매 비용 조회에서 현재 값을 읽는다),
//    globals = { localOwner()(00540c70), playerKnown()(00595344, 0 이면 분산 SP 를 읽지 않고 0.0f), collectorType()(0054126c, 초기 133), daisType()(005412ec), workshops(현재 0..3 워크샾 TYPE 배열, 004709d0), temple(remainder)(00470a30 의 541244/248/24c) },
//    hooks = { captured(owner)(00492340), dead(owner)(004922e0), canAfford(token, type)(00410280), wallet(owner)→int32(00413c20), storedSp(owner)→float(0040ec50), assertion(text, line)(Isleinfo.cpp 복귀 가능한 진단) } 이며 도달한 위치에서만 요구한다.
//  - reserveForCollectors(token): `ftol(현재 수집자 TYPE cost) × 본체+0x234` 의 하위 DWORD 곱(감김)을 signed 로 보아 현재 지갑(+0x2d8) 이하인지 판단한다.
//  - enemyLowFunds(token): 적(+0x264)이 로컬이 아니면 원격 지갑, 로컬이면 playerKnown 일 때 분산 SP(미확정은 0.0f)를 ftol 한 signed 값이 1000 미만인지 본다(자원 변경 없음, NaN 은 ftol 0).
//  - select(token): 포획 다이스 관문(+0x248 bit 0x200, +0x2a4 없음, 포획·미사망·구매 가능, 신전이 있거나 적 자금 부족)이면 현재 다이스 TYPE, 신전이 없고 +0x390 이 3 이 아니면 원소 신전 TYPE(비용 ≤ 지갑일 때만), 그 밖에는 후보 워크샾(+0x394)이
//    지갑/수집자 여유 비용을 모두 통과하면 선호 순서(+0x398.., 개수 +0x3b0)대로 다섯 theme 수량 복사본을 감소시켜 처음 음수가 되는 선호 TYPE 을 돌려준다(실제 본체 수량표는 쓰지 않는다). 각 관문 뒤 본체 필드를 다시 읽는다.
//  - workshopType(theme): 0..3 현재 값, 4/5 는 0, 범위 밖은 진단(0x1a) 뒤 0 이다. templeType(theme): signed `theme % 5` 의 0..2 만 현재 신전이고 그 밖은 진단(0x47) 뒤 0 이다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E167: AI 건설 선택 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 00411ad0 의 signed 지갑/SP 절삭 경계, 로컬 소유자 미확정 SP 대체값(PE 00500fc4), 현재 TYPE 개수와 워크샾 개수, 선호/수량 위치이다. (2026-11-09 WB04-E167)
const LOW_FUNDS = 1000;
const UNKNOWN_SP = 0;
const TYPE_COUNT = 188;
const WORKSHOP_COUNT = 4;
const COUNTS_AT = 0x6d4;
const PREFERENCES_AT = 0x398;
const THEME_COUNT = 5;
const TWO_63 = 9223372036854775808;

// 비정렬 원문 DWORD 를 물리 범위 안에서 읽는다. (2026-11-09 WB04-E167)
function read(raw, at) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 건설 선택 DWORD 범위');
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}

// 원본 DWORD 의 signed 비교 해석이다. (2026-11-09 WB04-E167)
const signed = (value) => value | 0;

// 원본 ftol 의 signed64 절삭 하위 DWORD 이며 무효 FISTP 는 하위 0 이다. (2026-11-09 WB04-E167)
function ftol(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// 선택된 하위 누락을 가짜 구매 허용/기본 TYPE 으로 대신하지 않는다. (2026-11-09 WB04-E167)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 건설 선택 실제 하위 미연결');
    return hook;
}

// 건설 선택 처리기이다. 독립 구매 추천/수량표 없이 원본 지갑/선호 목록/현재 TYPE 비용을 읽는다. (2026-11-09 WB04-E167)
export class RawAiBuildChoice {
    #memory;
    #types;
    #globals;
    #hooks;

    // 같은 실제 188 TYPE/워크샾 4/본체를 빌린다. 생성은 비용/사제 상태/자금을 읽지 않는다. (2026-11-09 WB04-E167)
    constructor(memory, types, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 건설 선택 본체 소유자 누락');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !globals || !globals.workshops || globals.workshops.length !== WORKSHOP_COUNT) throw new RangeError('AI 건설 선택 실제188TYPE/워크샾4 누락');
        for (const name of ['localOwner', 'playerKnown', 'collectorType', 'daisType']) {
            if (typeof globals[name] !== 'function') throw new TypeError(`AI 건설 선택 현재 전역 조회기 누락: ${name}`);
        }
        this.#memory = memory;
        this.#types = types;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 해제/가짜 본체를 거부하고 현재 살아 있는 0xb08 본체를 빌린다. (2026-11-09 WB04-E167)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 건설 선택 실제0xb08본체 누락');
        return raw;
    }

    // 하위 효과 뒤 현재 본체 DWORD 를 다시 읽는다. (2026-11-09 WB04-E167)
    #field(token, at) { return read(this.#body(token), at); }

    // 비용 조회에 종류/소유자 필터를 추가하지 않고 현재 TYPE 을 물리 범위 검사 뒤 조회한다. (2026-11-09 WB04-E167)
    #type(type) {
        if (type >= this.#types.length) throw new RangeError('AI 건설 선택 현재TYPE 범위');
        return this.#types[type];
    }

    // 현재 TYPE 의 원본 cost(float)를 ftol 한 DWORD 이다. (2026-11-09 WB04-E167)
    #cost(type) { return ftol(Math.fround(this.#type(type).cost)); }

    // 00411a40: 원본 imul 의 하위 DWORD 곱과 signed 지갑 비교이다(곱셈 overflow 를 감김으로 둔다). (2026-11-09 WB04-E167)
    reserveForCollectors(token) {
        const cost = this.#cost(this.#globals.collectorType() >>> 0);
        const reserve = Math.imul(cost, this.#field(token, 0x234)) >>> 0;
        return signed(reserve) <= signed(this.#field(token, 0x2d8));
    }

    // 00411ad0: 원격은 AI 지갑, 로컬 확정은 분산 SP, 미확정은 0 을 사용하며 리소스를 변경하지 않는다. (2026-11-09 WB04-E167)
    enemyLowFunds(token) {
        const enemy = this.#field(token, 0x264);
        const local = this.#globals.localOwner() >>> 0;
        if (enemy !== local) return (need(this.#hooks.wallet)(enemy) | 0) < LOW_FUNDS;
        const value = this.#globals.playerKnown() ? Math.fround(need(this.#hooks.storedSp)(local)) : UNKNOWN_SP;
        return signed(ftol(value)) < LOW_FUNDS;
    }

    // 004709d0: 범위 진단이 반환하면 원본 switch 의 default 0 으로 계속한다. (2026-11-09 WB04-E167)
    workshopType(theme) {
        theme |= 0;
        if (theme < 0 || theme > 5) need(this.#hooks.assertion)('theIndex>=0 && theIndex<=(MAX_THEMES)', 0x1a);
        return theme >= 0 && theme < WORKSHOP_COUNT ? this.#globals.workshops[theme] >>> 0 : 0;
    }

    // 00470a30: IDIV5 의 signed remainder 이며 원본 점프표의 5번 칸은 정상 나머지에서 결코 선택되지 않는다. (2026-11-09 WB04-E167)
    templeType(theme) {
        const index = ((theme | 0) % 5) | 0;
        if (index >= 0 && index < 3) return need(this.#globals.temple)(index) >>> 0;
        need(this.#hooks.assertion)('0', 0x47);
        return 0;
    }

    // 00411b40: 모든 비용은 현재 TYPE 의 원본 cost → ftol 이며 각 관문 뒤 현재 본체를 재조회한다. (2026-11-09 WB04-E167)
    select(token) {
        const hooks = this.#hooks;
        const globals = this.#globals;
        if ((this.#field(token, 0x248) & 0x200) && !this.#field(token, 0x2a4) && need(hooks.captured)(this.#field(token, 0x264)) && !need(hooks.dead)(this.#field(token, 0x264))
            && need(hooks.canAfford)(token, globals.daisType() >>> 0) && (this.#field(token, 0x294) || this.enemyLowFunds(token))) return globals.daisType() >>> 0;
        if (!this.#field(token, 0x294) && this.#field(token, 0x390) !== 3) {
            const temple = this.templeType(signed(this.#field(token, 0x390)));
            const cost = this.#cost(temple);
            return signed(this.#field(token, 0x2d8)) >= signed(cost) ? temple : 0;
        }
        const candidate = this.#field(token, 0x394);
        if (!candidate) return 0;
        if (signed(this.#field(token, 0x2d8)) < signed(this.#cost(candidate)) || !this.reserveForCollectors(token)) return 0;
        const counts = new Uint32Array(THEME_COUNT);
        // 원본은 선호 count 가 0 이어도 다섯 theme 수량을 먼저 복사하며 theme 4 는 TYPE 0 의 수량이다.
        for (let index = 0; index < counts.length; index += 1) {
            const type = this.workshopType(index);
            counts[index] = this.#field(token, COUNTS_AT + type * 4);
        }
        const count = signed(this.#field(token, 0x3b0));
        // 선호 중복마다 같은 지역 수량 복사본을 감소시켜 첫 음수의 현재 선호 TYPE 을 반환한다(실제 수량표는 쓰지 않는다).
        for (let index = 0; index < count; index += 1) {
            const theme = this.#field(token, PREFERENCES_AT + index * 4);
            if (theme >= counts.length) throw new RangeError('AI 건설 선택 원본임시수량 인덱스 범위');
            counts[theme] -= 1;
            if (signed(counts[theme]) < 0) return this.workshopType(signed(this.#field(token, PREFERENCES_AT + index * 4)));
        }
        return 0;
    }
}
