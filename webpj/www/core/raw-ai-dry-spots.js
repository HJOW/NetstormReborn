// AI 의 에너지 부족 배치 좌표 목록을 같은 0xb08 본체/12바이트 기록에서 처리하는 모듈이다(cpppj `o/RawAiDrySpots.cpp/.h`, 10.78: 패치 0040f1c0 추가·0040f260 제거·0040f2b0 순환 검사·00417c70 유지·004133e0 가짜 좌표 선택).
//  - new RawAiDrySpots(memory, pool, hooks): memory 는 E164 `RawAiBrainMemory`, pool 은 같은 PATCH SidPool(월드 정체성), hooks = { energy(box, x, y, type, owner)→DWORD(004734d0, box 는 초기 빈 문자열 `{ text: '' }` 이며 원래 DWORD 를 bool 로 줄이지 않는다),
//    log(token, fake)(00417c70 의 Killed [Fake] Dry Spot 과 004105b0 의 현재 실제 로그) } 이며 도달한 위치에서만 요구한다. 별도 좌표 사본 없이 원본 +0xadc(용량)·+0xae0(개수)·+0xae4(기록 포인터)·+0xae8(cursor)·+0xaec(owner)와 같은 소유 backing 을 쓴다.
//  - add(token, x, y, type, bits = {}): 용량 우선(가득 차면 포인터/중복 검색을 읽지 않고 false) → 좌표만 중복 검사(float 동등 비교라 signed 0 은 같고 NaN 은 서로 다르며 TYPE 이 달라도 같은 좌표면 중복) → 원문 12바이트(x/y float 비트 + TYPE DWORD)를 개수 위치에 추가한다.
//    NaN 페이로드를 보존해야 하면 bits = { xBits, yBits } 로 원본 float 비트를 준다.
//  - remove(token, index): 뒤 기록을 x → y → TYPE 순으로 안정 압축하고 개수만 줄인다(index 가 끝 이상이면 복사 없이 줄이고 미사용 꼬리/cursor 는 초기화하지 않는다).
//  - scan(token): 개수 0 이면 cursor 를 바꾸지 않고 0, 아니면 `(cursor + 1) % 개수`(signed) 한 항목만 검사한다(cursor 를 먼저 쓴 뒤 에너지 승인, 충족하면 현재 기록 TYPE 를 다시 읽어 제거하고 그 TYPE 를 반환, 제거 뒤 cursor 보정 없음, 개수 −1 의 signed 나눗셈 넘침은 명시 거부).
//  - maintain(token): scan 의 TYPE 가 0 이 아니면 현재 가짜 TYPE(+0x280)와 비교한 분류와 현재 디버그(+0xb00) 값으로 로그를 분배한다. seed(token, random): 능력 BYTE(+0x248 하위 0x20)가 있을 때 공용 난수 8 로 경계 좌표 여덟 방향 중 하나를 고르고
//    네 float 합/곱을 전부 마친 뒤 float 로 저장해 +0xaf0/+0xaf4 에 쓰고 현재 가짜 TYPE 로 등록한다(가득 참/중복이어도 좌표와 난수 소비를 되돌리지 않는다).
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E170: AI 부족 좌표 목록 이식으로 추가)

// 본체 안 내장 DrySpot 목록 필드 위치와 기록 크기(x/y float 비트 + TYPE DWORD 한 개), 00501550/00500ee4 의 정확한 상수(중간 합에는 float 저장을 넣지 않는다)이다. (2026-11-09 WB04-E170)
const CAPACITY = 0xadc;
const COUNT = 0xae0;
const POINTER = 0xae4;
const CURSOR = 0xae8;
const OWNER = 0xaec;
const RECORD_BYTES = 12;
const QUARTER = 0.25;
const HALF = 0.5;
const BODY_BYTES = 0xb08;

// 비정렬 DWORD 를 물리 범위 안에서 읽는다. (2026-11-09 WB04-E170)
function read(raw, offset) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI DrySpot DWORD 읽기 범위');
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 지정 네 바이트만 쓰고 주변/미사용 꼬리를 보존한다. (2026-11-09 WB04-E170)
function put(raw, offset, value) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI DrySpot DWORD 쓰기 범위');
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// DWORD 의 signed 비교/나머지에서 구현 의존 변환을 피한다. (2026-11-09 WB04-E170)
const signed = (value) => value | 0;

// 단정도 float 와 비트의 상호 변환이다. (2026-11-09 WB04-E170)
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 실제 선택된 하위가 없으면 앞선 원문 쓰기를 유지한 채 거부한다. (2026-11-09 WB04-E170)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI DrySpot 실제 하위 미연결');
    return hook;
}

// 부족 좌표 목록 처리기이다. 별도 좌표 사본 없이 같은 소유 backing 을 쓴다. (2026-11-09 WB04-E170)
export class RawAiDrySpots {
    #memory;
    #pool;
    #hooks;

    // 생성 시 게임 판단/목록 할당을 수행하지 않는다. 같은 PATCH 풀만 받는다. (2026-11-09 WB04-E170)
    constructor(memory, pool, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI DrySpot 본체 소유자 누락');
        if (pool?.edition !== 'patch1078') throw new TypeError('AI DrySpot은 같은 PATCH 풀만 지원합니다');
        this.#memory = memory;
        this.#pool = pool;
        this.#hooks = hooks ?? {};
    }

    // 같은 실제 월드의 풀인지 확인할 수 있도록 제공한다. (2026-11-09 WB04-E170)
    get pool() { return this.#pool; }

    // 해제 토큰/작은 가짜 본체를 거부하고 현재 살아 있는 0xb08 본체를 빌린다. (2026-11-09 WB04-E170)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== BODY_BYTES) throw new RangeError('AI DrySpot 실제0xb08본체누락');
        return raw;
    }

    // 하위 효과 뒤 현재 본체 DWORD 를 다시 읽는다. (2026-11-09 WB04-E170)
    #field(token, offset) { return read(this.#body(token), offset); }

    // 선택 필드만 쓴다. (2026-11-09 WB04-E170)
    #set(token, offset, value) { put(this.#body(token), offset, value >>> 0); }

    // 포인터를 미리 복사한 별도 목록으로 대체하지 않고 현재 +0xae4 가 가리키는 같은 backing 을 다시 읽는다. (2026-11-09 WB04-E170)
    #records(token) { return this.#memory.bytes(this.#field(token, POINTER)); }

    // 원본 정상 범위 밖 메모리 접근을 다른 항목/자동 clamp 로 바꾸지 않고 해당 지점에서 거부한다. (2026-11-09 WB04-E170)
    #offset(raw, index) {
        if (index < 0 || index > Math.floor(raw.length / RECORD_BYTES) || raw.length - index * RECORD_BYTES < RECORD_BYTES) throw new RangeError('AI DrySpot 기록 물리 범위');
        return index * RECORD_BYTES;
    }

    // 0040f1c0: 용량이 찼으면 포인터/중복 검색을 읽지 않고 나간다. 현재 TYPE 가 달라도 같은 좌표면 중복이다. (2026-11-09 WB04-E170)
    add(token, x, y, type, bits = {}) {
        const count = signed(this.#field(token, COUNT));
        if (count >= signed(this.#field(token, CAPACITY))) return false;
        const pointX = Math.fround(x);
        const pointY = Math.fround(y);
        // float 동등 비교로 signed 0 을 같게 보고 NaN 좌표는 중복으로 제거하지 않는다.
        for (let index = 0; index < signed(this.#field(token, COUNT)); index += 1) {
            const raw = this.#records(token);
            const at = this.#offset(raw, index);
            if (pointX === bitsFloat(read(raw, at)) && pointY === bitsFloat(read(raw, at + 4))) return false;
        }
        const raw = this.#records(token);
        const at = this.#offset(raw, count);
        put(raw, at, bits.xBits ?? floatBits(pointX));
        put(raw, at + 4, bits.yBits ?? floatBits(pointY));
        put(raw, at + 8, type);
        this.#set(token, COUNT, (this.#field(token, COUNT) + 1) >>> 0);
        return true;
    }

    // 0040f260: 원본은 index 가 끝 이상이면 복사 없이 count 만 감소하며 cursor/미사용 꼬리를 초기화하지 않는다. (2026-11-09 WB04-E170)
    remove(token, index) {
        index |= 0;
        // 각 기록을 x → y → TYPE 순서로 옮기고 현재 포인터/count 를 재조회한다.
        while (index < signed((this.#field(token, COUNT) - 1) >>> 0)) {
            const raw = this.#records(token);
            const target = this.#offset(raw, index);
            const source = this.#offset(raw, (index + 1) | 0);
            put(raw, target, read(raw, source));
            put(raw, target + 4, read(raw, source + 4));
            put(raw, target + 8, read(raw, source + 8));
            index = (index + 1) | 0;
        }
        this.#set(token, COUNT, (this.#field(token, COUNT) - 1) >>> 0);
    }

    // 0040f2b0: 한 번에 한 기록만 검사하며 제거 뒤 원본 cursor 를 보정하지 않는다. (2026-11-09 WB04-E170)
    scan(token) {
        const count = signed(this.#field(token, COUNT));
        if (!count) return 0;
        const numerator = signed((this.#field(token, CURSOR) + 1) >>> 0);
        if (count === -1 && numerator === -2147483648) throw new RangeError('AI DrySpot signed 나눗셈 넘침');
        const index = (numerator % count) | 0;
        const pointer = this.#field(token, POINTER);
        const owner = this.#field(token, OWNER);
        this.#set(token, CURSOR, index);
        const box = { text: '' };
        const raw = this.#memory.bytes(pointer);
        const at = this.#offset(raw, index);
        if (!need(this.#hooks.energy)(box, bitsFloat(read(raw, at)), bitsFloat(read(raw, at + 4)), read(raw, at + 8), owner)) return 0;
        const current = signed(this.#field(token, CURSOR));
        const records = this.#records(token);
        const type = read(records, this.#offset(records, current) + 8);
        this.remove(token, current);
        return type;
    }

    // 반환 TYPE 가 0 이면 로그를 내지 않고 현재 fake 종류와 디버그 값을 읽는다. (2026-11-09 WB04-E170)
    maintain(token) {
        const type = this.scan(token);
        if (type) {
            const fake = type === this.#field(token, 0x280);
            if (this.#field(token, 0xb00)) need(this.#hooks.log)(token, fake);
        }
        return type;
    }

    // 004133e0: 전체 합/곱 뒤 네 float 를 저장하고, 난수 한 번 뒤 원본 순서로 두 좌표를 기록한다. (2026-11-09 WB04-E170)
    seed(token, random) {
        if (!(this.#field(token, 0x248) & 0x20)) return;
        // 이 조회는 현재 본체 float 를 넓혀 읽고 계산 중간의 임의 float 반올림을 피한다.
        const number = (at) => bitsFloat(this.#field(token, at));
        const combinedX = Math.fround((((number(0x2c0) + number(0x2b8)) + number(0x278)) + number(0x270)) * QUARTER);
        const combinedY = Math.fround((((number(0x2c4) + number(0x2bc)) + number(0x27c)) + number(0x274)) * QUARTER);
        const ownX = Math.fround((number(0x278) + number(0x270)) * HALF);
        const ownY = Math.fround((number(0x27c) + number(0x274)) * HALF);
        switch (random.next(8)) {
        case 0: this.#set(token, 0xaf4, this.#field(token, 0x274)); this.#set(token, 0xaf0, floatBits(combinedX)); break;
        case 1: this.#set(token, 0xaf4, this.#field(token, 0x27c)); this.#set(token, 0xaf0, floatBits(combinedX)); break;
        case 2: this.#set(token, 0xaf0, this.#field(token, 0x270)); this.#set(token, 0xaf4, floatBits(combinedY)); break;
        case 3: this.#set(token, 0xaf4, floatBits(combinedY)); this.#set(token, 0xaf0, this.#field(token, 0x278)); break;
        case 4: this.#set(token, 0xaf4, this.#field(token, 0x274)); this.#set(token, 0xaf0, floatBits(ownX)); break;
        case 5: this.#set(token, 0xaf4, this.#field(token, 0x27c)); this.#set(token, 0xaf0, floatBits(ownX)); break;
        case 6: this.#set(token, 0xaf0, this.#field(token, 0x270)); this.#set(token, 0xaf4, floatBits(ownY)); break;
        default: this.#set(token, 0xaf4, floatBits(ownY)); this.#set(token, 0xaf0, this.#field(token, 0x278)); break;
        }
        const type = this.#field(token, 0x280);
        const y = this.#field(token, 0xaf4);
        const x = this.#field(token, 0xaf0);
        this.add(token, bitsFloat(x), bitsFloat(y), type, { xBits: x, yBits: y });
    }
}
