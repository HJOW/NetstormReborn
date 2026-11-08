// AI 원소별 기술 선택과 시설 생산/업그레이드 대기 목록을 원본 본체/목록/내용물에 적용하는 모듈이다(cpppj `o/RawAiFactoryProduction.cpp/.h`, 10.78: 패치 00413570 기술 선택·004acc60 생산 총수·004136b0 생산 관리·00411180 업그레이드 완료).
//  - new RawAiFactoryProduction(memory, pool, types, finder, globals, hooks): memory 는 E164 `RawAiBrainMemory`(본체 0xb08 + 목록 블록), pool 은 같은 PATCH SidPool, types 는 현재 TYPE 의 `{ theme, flags2, cost }` 표(비어 있지 않아야 함),
//    finder 는 현재 권한/containedType 와 저장 next 를 공유하는 `ContainedFinderState`, globals = { now()(0055b4d0 현재 QWORD 게임 시각), invalidGraph()(00540410 현재 무효 그래프, 도달할 때만 요구) },
//    hooks = { technology(owner)→SID(0048fc90 현재 owner 기술 부모), product(token, type)→SID(00410360 같은 본체의 현재 기술 부모 mask 8 조회), addProduct(token, factory, type)(00410420 실제 로컬/원격 생산 등록),
//    level(factory)→level(가상 +80/00453f40 현재 시설 level), upgrade(factory)(00453bf0 실제 업그레이드 요청, 미복원 호출은 선택된 위치에서 거부), log(token, action, type)(현재 원본 로그 종류와 TYPE 이름), demo()→bool(00411180 현재 데모 지갑 하한 여부) } 는 도달한 위치에서만 요구한다.
//  - selectTechnology(token, theme): 00413570 — 현재 owner 기술 부모의 내용물(RawContainedFinder 의 먼저 저장한 next 유지) 중 flags4(+20 의 0x4)인 것만 현재 TYPE 의 원소(theme)로 판정하고, flags2 0x800(battery)이면 본체의 치환 필드(+0x3b4 + 원소×4)가 1 이면 theme==3 만,
//    2 이면 본래 원소 또는 theme==3, 그 밖의 0 아닌 값이면 불일치로 바꾼다. 현재 생산 조회에 이미 있는 TYPE 은 건너뛰고 첫 일치 미등록 TYPE 번호(없으면 0)를 돌려준다.
//  - countProducts(factory): 004acc60(mask 0x10, kind 0) — 시설의 실제 내용물 총수이며 같은 종류도 각각 센다.
//  - manage(token): 004136b0 — 신전/graph/기한(+0x5a8) 관문 뒤 미처리 시설 목록(+0x30c/+0x314)이 비어 있으면 충족(+0x2cc)·지갑(+0x2d8 > 1199)·업그레이드 대기(+0x318/+0x320)의 첫 시설을 업그레이드 요청 → 목록 제거 → 생각 기한 순서로 처리하고,
//    그렇지 않으면 첫 시설에 선택된 기술을 추가 등록(슬롯이 남으면 유지)하거나 모든 중복을 압축 제거하고 현재 level 과 남은 기술이 있을 때만 업그레이드 대기 목록에 추가한다.
//  - upgraded(token, factory): 00411180 — 같은 시설을 미처리 목록(가득 차도 비용/데모 처리는 계속, 중복 허용)에 추가 → 현재 TYPE 의 음수 비용 절삭을 지갑(+0x2d8)에 가산 → 데모 판정 뒤 현재 지갑에 1190 하한을 적용한다.
//  - 목록은 원본 네이티브 0040e9b0(추가, 가득 차면 포인터를 읽지 않고 생략)/0040ea00(모든 일치의 안정 압축 제거, 미사용 꼬리 보존)과 같다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E179: AI 시설 생산 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';
import { ContainedFinderState, RawContainedFinder, countContained } from './raw-contained-finder.js';

// 원본 지갑 > 0x4af 의 업그레이드 후보 관문과 데모 지갑 하한, 원소 치환 필드 시작, 중립 원소 번호이다. (2026-11-09 WB04-E179)
const UPGRADE_BUDGET = 1199;
const DEMO_FLOOR = 1190;
const THEME_REPLACEMENT = 0x3b4;
const NEUTRAL_THEME = 3;
const TWO_63 = 9223372036854775808;
// 시설이 요청하는 두 행동(기술 추가/업그레이드 요청)이며 둘 다 생각 기한을 갱신한다. (2026-11-09 WB04-E179)
export const AI_FACTORY_ACTION = Object.freeze({ addedProduct: 0, upgrade: 1 });

// 비정렬 little endian DWORD 를 물리 범위 안에서 읽는다. (2026-11-09 WB04-E179)
function read(raw, offset) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 시설 생산 DWORD 읽기 범위');
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 지정 네 BYTE 만 바꾸고 다른 본체/목록 원문을 보존한다. (2026-11-09 WB04-E179)
function write(raw, offset, value) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 시설 생산 DWORD 쓰기 범위');
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// signed 비교에서 DWORD 감김을 보존한다. (2026-11-09 WB04-E179)
const signed = (value) => value | 0;

// 원본 ftol 의 signed64 절삭 하위 DWORD 와 무효 입력의 0 을 보존한다. (2026-11-09 WB04-E179)
function truncate(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// 두 DWORD 를 이은 QWORD(낮은 DWORD 먼저)를 배정도 시각으로 해석한다. NaN 은 순서 없음(모든 비교 거짓)이다. (2026-11-09 WB04-E179)
function deadline(low, high) {
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, low >>> 0, true);
    view.setUint32(4, high >>> 0, true);
    return view.getFloat64(0, true);
}

// 단정도 float 비트를 수로 바꾼다. (2026-11-09 WB04-E179)
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 선택되지 않은 하위는 요구하지 않고 실제 선택 지점에서만 누락을 거부한다. (2026-11-09 WB04-E179)
function need(hook, message) {
    if (typeof hook !== 'function') throw new Error(message);
    return hook;
}

// 시설 생산 처리기이다. 생성은 실제 게임 효과를 일으키지 않는다. (2026-11-09 WB04-E179)
export class RawAiFactoryProduction {
    #memory;
    #pool;
    #types;
    #finder;
    #globals;
    #hooks;

    // 같은 PATCH/TYPE 와 실제 내용물 순회 상태를 빌린다. (2026-11-09 WB04-E179)
    constructor(memory, pool, types, finder, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 시설 생산 본체 소유자 누락');
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length === 0) throw new TypeError('AI 시설 생산 같은 PATCH/TYPE 계약 누락');
        if (!(finder instanceof ContainedFinderState)) throw new TypeError('AI 시설 생산 내용물 권한 상태 누락');
        if (typeof globals?.now !== 'function') throw new TypeError('AI 시설 생산 현재 시각 조회기 누락');
        this.#memory = memory;
        this.#pool = pool;
        this.#types = types;
        this.#finder = finder;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 현재 세션 조립의 같은 풀인지 확인하기 위한 풀이다. (2026-11-09 WB04-E179)
    get pool() { return this.#pool; }

    // 가짜/해제 토큰이나 다른 크기의 본체를 거부하고 현재 본체를 다시 빌린다. (2026-11-09 WB04-E179)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 시설 생산 실제0xb08 본체 누락');
        return raw;
    }

    // 외부 효과 뒤 현재 같은 본체를 다시 읽는다. (2026-11-09 WB04-E179)
    #field(token, offset) { return read(this.#body(token), offset); }

    // 선택된 필드만 현재 본체에 쓴다. (2026-11-09 WB04-E179)
    #setField(token, offset, value) { write(this.#body(token), offset, value >>> 0); }

    // 현재 목록의 같은 소유 backing 의 DWORD 를 읽는다. (2026-11-09 WB04-E179)
    #item(token, pointer, index) {
        if (index < 0) throw new RangeError('AI 시설 생산 음수 목록 인덱스');
        return read(this.#memory.bytes(this.#field(token, pointer)), index * 4);
    }

    // 0040ea00: 원본처럼 모든 일치 항목만 제거하며 사용하지 않는 꼬리는 지우지 않는다(매 방문 현재 count/포인터를 다시 읽고 상대 순서 보존). (2026-11-09 WB04-E179)
    #remove(token, pointer, value) {
        let removed = 0;
        for (let index = 0; index < signed(this.#field(token, pointer + 8)); index += 1) {
            const item = this.#item(token, pointer, index);
            if (item === value) removed += 1;
            else write(this.#memory.bytes(this.#field(token, pointer)), (index - removed) * 4, item);
        }
        this.#setField(token, pointer + 8, this.#field(token, pointer + 8) - removed);
    }

    // 0040e9b0: 원본은 가득 찼을 때 포인터를 읽지 않으며 중복을 검사하지 않는다. (2026-11-09 WB04-E179)
    #append(token, pointer, value) {
        const count = signed(this.#field(token, pointer + 8));
        if (count >= signed(this.#field(token, pointer + 4))) return;
        if (count < 0) throw new RangeError('AI 시설 생산 음수 추가 인덱스');
        write(this.#memory.bytes(this.#field(token, pointer)), count * 4, value);
        this.#setField(token, pointer + 8, this.#field(token, pointer + 8) + 1);
    }

    // 원본 풀 밖 접근을 잘린 다른 SID 로 바꾸지 않는다. (2026-11-09 WB04-E179)
    #physicalSid(value) {
        if (value >= this.#pool.capacity || value > 65535) throw new RangeError('AI 시설 생산 현재 SID 범위');
        return value;
    }

    // TYPE 0 은 원본 getter 처럼 허용하며 표 밖은 거부한다. (2026-11-09 WB04-E179)
    #type(type) {
        if (type >= this.#types.length) throw new RangeError('AI 시설 생산 현재 TYPE 범위');
        return this.#types[type];
    }

    // 004ac970: 같은 시설의 현재 BYTE TYPE 에서 원소를 다시 읽는다. (2026-11-09 WB04-E179)
    #theme(factory) { return this.#type(this.#pool.slot(factory)[10]).theme; }

    // 현재 +0x5a8 QWORD 기한과 비교하며 QWORD 시각을 float 로 축소하지 않고 unordered 는 거짓이다. (2026-11-09 WB04-E179)
    #ready(token) { return this.#globals.now() >= deadline(this.#field(token, 0x5a8), this.#field(token, 0x5ac)); }

    // 00410620: 부가 행동은 현재 rate 와 QWORD now 로 두 생각 기한을 기록한 뒤 +0xb00 이 켜졌을 때만 디버그 로그를 처리한다. (2026-11-09 WB04-E179)
    #action(token, action, type) {
        const rate = bitsFloat(this.#field(token, 0x228));
        const view = new DataView(new ArrayBuffer(16));
        view.setFloat64(0, rate + this.#globals.now(), true);
        view.setFloat64(8, (rate + 1) + this.#globals.now(), true);
        this.#setField(token, 0x5a8, view.getUint32(0, true));
        this.#setField(token, 0x5ac, view.getUint32(4, true));
        this.#setField(token, 0x5b0, view.getUint32(8, true));
        this.#setField(token, 0x5b4, view.getUint32(12, true));
        if (this.#field(token, 0xb00)) need(this.#hooks.log, 'AI 시설 생산 현재 행동 로그 미연결')(token, action, type);
    }

    // 00413570: 원본 기술 순회는 다음 SID 를 먼저 저장하며 기술 추가/생산 조회 효과 뒤에도 그 next 를 유지한다. (2026-11-09 WB04-E179)
    selectTechnology(token, theme) {
        const parent = need(this.#hooks.technology, 'AI 시설 생산 현재 기술부모 조회 미연결')(this.#field(token, 0x260));
        const cursor = new RawContainedFinder(this.#pool, this.#finder);
        // flags4 기술 내용물만 현재 TYPE/원소로 판정하며 생산이 없는 첫 종류를 돌려준다.
        for (let form = cursor.begin(parent); form; form = cursor.next()) {
            const raw = this.#pool.slot(form);
            if (!(raw[20] & 4)) continue;
            const type = raw[18] | (raw[19] << 8);
            const entry = this.#type(type);
            let matches = entry.theme === theme;
            if (entry.flags2 & 0x800) {
                const offset = THEME_REPLACEMENT + entry.theme * 4;
                if (offset < 0 || offset > AI_BRAIN_BYTES - 4) throw new RangeError('AI 시설 생산 원소 치환 필드 범위');
                const replacement = this.#field(token, offset);
                if (replacement === 1) matches = theme === NEUTRAL_THEME;
                else if (replacement === 2) matches = matches || theme === NEUTRAL_THEME;
                else if (replacement) matches = false;
            }
            if (matches && !need(this.#hooks.product, 'AI 시설 생산 현재 생산 조회 미연결')(token, type)) return type;
        }
        return 0;
    }

    // 004acc60(mask 0x10, kind 0): 수집과 같은 몸체를 사용하며 같은 종류의 중복 생산도 원본 총수에는 각각 포함한다. (2026-11-09 WB04-E179)
    countProducts(factory) { return countContained(this.#pool, this.#finder, factory, 0x10, 0); }

    // 004136b0: 미처리 첫 시설의 생산 등록 또는 업그레이드 대기 첫 시설의 요청만 처리한다. (2026-11-09 WB04-E179)
    manage(token) {
        if (!this.#field(token, 0x294) || this.#field(token, 0x298) === need(this.#globals.invalidGraph, 'AI 시설 생산 현재 무효 그래프 조회 미연결')() || !this.#ready(token)) return;
        if (signed(this.#field(token, 0x314)) < 1) {
            if (!this.#field(token, 0x314) && this.#field(token, 0x2cc) && signed(this.#field(token, 0x2d8)) > UPGRADE_BUDGET && signed(this.#field(token, 0x320)) > 0) {
                const value = this.#item(token, 0x318, 0);
                need(this.#hooks.upgrade, 'AI 시설 업그레이드00453bf0 실제 요청 미연결')(this.#physicalSid(value));
                this.#remove(token, 0x318, value);
                this.#action(token, AI_FACTORY_ACTION.upgrade, 0);
            }
            return;
        }
        const value = this.#item(token, 0x30c, 0);
        const factory = this.#physicalSid(value);
        const level = need(this.#hooks.level, 'AI 시설 현재 가상level 미연결')(factory);
        const capacity = signed(level + 2);
        const count = this.countProducts(factory);
        const type = this.selectTechnology(token, this.#theme(factory));
        if (type && count < capacity) {
            need(this.#hooks.addProduct, 'AI 시설 실제 생산 등록00410420 미연결')(token, factory, type);
            this.#type(type);
            this.#action(token, AI_FACTORY_ACTION.addedProduct, type);
            if (signed(count + 1) < capacity) return;
        }
        this.#remove(token, 0x30c, value);
        if (type && signed(need(this.#hooks.level, 'AI 시설 현재 가상level 재조회 미연결')(factory)) < 2 && this.selectTechnology(token, this.#theme(factory))) this.#append(token, 0x318, value);
    }

    // 00411180: 큐가 가득 차도 비용/데모 처리는 계속하며 중복 시설도 원본처럼 추가한다. (2026-11-09 WB04-E179)
    upgraded(token, factory) {
        this.#append(token, 0x30c, factory);
        const delta = truncate(-Math.fround(this.#type(this.#pool.slot(factory)[10]).cost));
        this.#setField(token, 0x2d8, this.#field(token, 0x2d8) + delta);
        if (need(this.#hooks.demo, 'AI 업그레이드 현재 데모 판정 미연결')()) {
            const wallet = signed(this.#field(token, 0x2d8));
            this.#setField(token, 0x2d8, wallet > DEMO_FLOOR ? wallet : DEMO_FLOOR);
        }
    }
}
