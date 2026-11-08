// 실제 기술 내용물과 직전 후보/표적·방향·쿨다운으로 AI 의 다음 배치 TYPE 를 선택하는 모듈이다(cpppj `o/RawAiDropSelection.cpp/.h`, 10.78: 패치 00413100·00425830).
//  - AI_DROP_SELECTION_LOG: 원본 세 로그의 의미(hated 직전 표적 선호·retried 후보 재사용·scanning 현재 TYPE/회전 알림)이며 출력 여부/현재 로그 종류는 원래 위치에서 판단한다.
//  - new RawAiDropSelection(memory, pool, types, directionalTypes, random, now, hooks): memory 는 E164 `RawAiBrainMemory`, pool 은 같은 PATCH SidPool, types 는 현재 188 TYPE({ flags2 })의 같은 배열,
//    directionalTypes 는 현재 네 방향 TYPE 전역(00541264/25c/288/290 의 순서, 기본 131/129/140/142)의 같은 배열(복사하지 않는다), random 은 SP/시설/스캔과 같은 공용 `GameRandom`, now() 는 0055b4d0 의 현재 QWORD 시각,
//    hooks = { technology(owner)→SID(0048fc90 현재 owner 기술 부모), count(sid)→int(004acc60 mask 8 의 실제 모든 일치 개수, 중복도 센다), blocker(sid)→SID(004acb80 mask 8·genus 4000000 현재 블로커 생산 조각),
//    nth(sid, index)→TYPE(004acac0 mask 8·kind 0 의 index 번째 TYPE WORD, SID 아님), mayPlace(query)(0049b510 전체: argument 0/flags 0/현재 owner BYTE/mode 1), remove(token, index)(0040f390 같은 30후보 안정 압축),
//    log(token, action, type, direction)(004105b0 현재 출력, +0xb00 이 비영인 실제 위치에서만 요구) } 이다.
//  - select(token): 00413100 — 진행 위치 초기화(+0xaf8/+0xafc = 10, +0x3ec = 0) → 현재 owner 기술 부모/개수(최초 값만 사용, 0 이면 TYPE 0·재선택 표식만 쓰고 끝) → 능력 BYTE 0x80 이면 공용 난수 100 으로 직전 표적 반복 여부(표적이 직전 표적 목록에 있으면 80 미만, 목록이 비면 30 미만) →
//    블로커 우선(+0x404, 현재 TYPE 가 블로커가 아닐 때 블로커 조각이 없으면 +0x404 만 지움) → 반복이면 같은 index 에서 압축된 현재 후보를 승인 재검사하고 거부된 것은 제거(재시도 로그), 아니면 기술 개수+2 번까지 회전(방향 TYPE 이고 +0x3e4 < 6 이면 2 증가,
//    아니면 signed 나머지 순환으로 다음 TYPE)하고 블로커 우선 TYPE 를 찾으면 멈춤 → 후보 count 0 이면 최고점수 0, 재선택/표적/집중 지움, 운반 genus 와 쿨다운이 안 지난 공급 TYPE 는 선택을 지우고, 선택이 남으면 스캔 로그.
//  - directional(type): 00425830 — 현재 네 DWORD 와 전체 TYPE 번호를 순서대로 비교한다(TYPE 배열은 아직 조회하지 않는다).
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E174: AI 배치 TYPE 선택 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 원본 세 로그의 종류이다. (2026-11-09 WB04-E174)
export const AI_DROP_SELECTION_LOG = Object.freeze({ hated: 'Hated', retried: 'Retried', scanning: 'Scanning' });

// 블로커 우선 선택과 현재 생산 조각을 판단하는 원본 flags2, 같은 30후보 XY 의 본체 시작/행 크기/개수이다. (2026-11-09 WB04-E174)
const BLOCKER = 0x4000000;
const FIRST_CHOICE = 0x408;
const CHOICE_STRIDE = 8;
const CHOICES = 30;

// 비정렬 DWORD 를 실제 블록 범위 안에서 읽는다. (2026-11-09 WB04-E174)
function read(raw, at) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 배치 TYPE 선택 DWORD 범위');
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}

// DWORD 감김 뒤 signed 해석이다. (2026-11-09 WB04-E174)
const signed = (value) => value | 0;

// 원본 signed 나눗셈은 0 쪽 절삭이며 나머지도 부호를 보존한다(0 나눗셈/최소값 −1 은 거부). (2026-11-09 WB04-E174)
function remainder(value, count) {
    if (!count || (count === -1 && value === -2147483648)) throw new RangeError('AI 배치 기술 순환 signed 나눗셈 범위');
    return (value % count) | 0;
}

// 단정도 float 비트를 수로 바꾼다. (2026-11-09 WB04-E174)
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 선택된 실제 하위만 그 자리에서 요구한다. (2026-11-09 WB04-E174)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 배치 TYPE 선택 실제 하위 미연결');
    return hook;
}

// 배치 TYPE 선택기이다. 실제 본체/기술/공용 난수와 현재 네 방향 TYPE 전역만 사용한다. (2026-11-09 WB04-E174)
export class RawAiDropSelection {
    #memory;
    #pool;
    #types;
    #directional;
    #random;
    #now;
    #hooks;

    // 생성 중 게임 판단/난수/로그를 실행하지 않는다. (2026-11-09 WB04-E174)
    constructor(memory, pool, types, directionalTypes, random, now, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 배치 TYPE 선택 본체 소유자 누락');
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== 188 || !directionalTypes || directionalTypes.length !== 4 || typeof random?.next !== 'function' || typeof now !== 'function') {
            throw new TypeError('AI 배치 TYPE 선택 같은PATCH/현재TYPE누락');
        }
        this.#memory = memory;
        this.#pool = pool;
        this.#types = types;
        this.#directional = directionalTypes;
        this.#random = random;
        this.#now = now;
        this.#hooks = hooks ?? {};
    }

    // 같은 실제 월드의 풀인지 확인할 수 있도록 제공한다. (2026-11-09 WB04-E174)
    get pool() { return this.#pool; }

    // 해제/작은 가짜 본체를 거부한다. (2026-11-09 WB04-E174)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 배치 TYPE 선택 실제0xb08본체누락');
        return raw;
    }

    // 현재 같은 본체를 매 원문 위치에서 읽는다. (2026-11-09 WB04-E174)
    #field(token, at) { return read(this.#body(token), at); }

    // 지정 DWORD 만 쓴다. (2026-11-09 WB04-E174)
    #set(token, at, value) {
        const raw = this.#body(token);
        if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 배치 TYPE 선택 쓰기 범위');
        // 지정 네 바이트만 쓴다.
        for (let index = 0; index < 4; index += 1) raw[at + index] = ((value >>> 0) >>> (index * 8)) & 0xff;
    }

    // TYPE 0 도 원본처럼 조회하며 범위 밖만 명시 거부한다. (2026-11-09 WB04-E174)
    #type(type) {
        if (type >= this.#types.length) throw new RangeError('AI 배치 TYPE 선택 현재TYPE 범위');
        return this.#types[type];
    }

    // 00425830: 현재 네 전역과 DWORD 그대로 비교하며 TYPE 배열은 아직 조회하지 않는다(원본 순서의 첫 일치). (2026-11-09 WB04-E174)
    directional(type) {
        // 원본 순서의 첫 일치를 반환한다.
        for (const current of this.#directional) if ((type >>> 0) === (current >>> 0)) return true;
        return false;
    }

    // 0040e9d0(this+0x36c): 진입 시 개수와 목록 포인터를 값으로 보존하고 첫 일치만 본다. (2026-11-09 WB04-E174)
    #contains(token, target) {
        const count = signed(this.#field(token, 0x374));
        if (count <= 0) return false;
        const list = this.#memory.bytes(this.#field(token, 0x36c));
        let index = 0;
        // 최초 count 안의 첫 일치까지만 진행한다.
        do {
            if (read(list, index * 4) === (target >>> 0)) return true;
            index += 1;
        } while (index < count);
        return false;
    }

    // b00 은 로그 위치마다 현재 다시 읽으며 읽지 않을 TYPE/방향은 인자로 요구하지 않는다. (2026-11-09 WB04-E174)
    #log(token, action, type = 0, direction = 0) {
        if (this.#field(token, 0xb00)) need(this.#hooks.log)(token, action, type, direction);
    }

    // 원본 QWORD 시각을 재구성한다. (2026-11-09 WB04-E174)
    #time(token, at) {
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, this.#field(token, at), true);
        view.setUint32(4, this.#field(token, at + 4), true);
        return view.getFloat64(0, true);
    }

    // 실제 기술 개수/부모는 최초 값이고 다른 본체 필드는 각 하위 효과 뒤 현재 값이다. (2026-11-09 WB04-E174)
    select(token) {
        const hooks = this.#hooks;
        this.#set(token, 0xaf8, 10);
        this.#set(token, 0xafc, 10);
        this.#set(token, 0x3ec, 0);
        const technology = need(hooks.technology)(this.#field(token, 0x260));
        const count = need(hooks.count)(technology) | 0;
        if (!count) {
            this.#set(token, 0x3e0, 0);
            this.#set(token, 0x3f0, 1);
            return;
        }
        let repeat = false;
        if (this.#field(token, 0x248) & 0x80) {
            const draw = this.#random.next(100);
            if (this.#field(token, 0x3f4) && this.#field(token, 0x3e0)) {
                if (this.#contains(token, this.#field(token, 0x3fc))) {
                    if (draw < 80) {
                        this.#log(token, AI_DROP_SELECTION_LOG.hated);
                        repeat = true;
                    }
                } else if (!this.#field(token, 0x374) && draw < 30) repeat = true;
            }
        }
        let forceBlocker = false;
        if (this.#field(token, 0x404) && !(this.#type(this.#field(token, 0x3e0)).flags2 & BLOCKER)) {
            if (need(hooks.blocker)(technology)) forceBlocker = true;
            else this.#set(token, 0x404, 0);
        }
        if (repeat && !forceBlocker) {
            let index = 0;
            // 제거 시 같은 index/좌표 위치에서 압축된 현재 항목을 재검사한다.
            while (index < signed(this.#field(token, 0x570))) {
                if (index < 0 || index >= CHOICES) throw new RangeError('AI 배치 재사용 후보 인덱스');
                const at = FIRST_CHOICE + index * CHOICE_STRIDE;
                const query = { type: this.#field(token, 0x3e0), argument: 0, x: bitsFloat(this.#field(token, at)), y: bitsFloat(this.#field(token, at + 4)), flags: 0, owner: this.#field(token, 0x260) & 255, mode: 1 };
                this.#type(query.type);
                if (!need(hooks.mayPlace)(query)) need(hooks.remove)(token, index);
                else index += 1;
            }
            this.#log(token, AI_DROP_SELECTION_LOG.retried);
        } else {
            let remaining = ((count >>> 0) + 2) >>> 0;
            // 원본은 기술 개수+2 번까지 회전/다음 TYPE 를 시도하고 블로커 우선도 이 예산 안에서만 수행한다.
            for (;;) {
                remaining = (remaining - 1) >>> 0;
                const current = this.#field(token, 0x3e0);
                if (current && this.directional(current) && signed(this.#field(token, 0x3e4)) < 6) this.#set(token, 0x3e4, (this.#field(token, 0x3e4) + 2) >>> 0);
                else {
                    const next = remainder(signed((this.#field(token, 0x3dc) + 1) >>> 0), count);
                    this.#set(token, 0x3dc, next >>> 0);
                    const selected = need(hooks.nth)(technology, next >>> 0);
                    this.#set(token, 0x3e0, selected);
                    this.#set(token, 0x3e4, 0);
                }
                if (!remaining || !this.#field(token, 0x404)) break;
                if (this.#type(this.#field(token, 0x3e0)).flags2 & BLOCKER) break;
            }
            this.#set(token, 0x404, 0);
            this.#set(token, 0x570, 0);
        }
        const choices = this.#field(token, 0x570);
        if (!choices) this.#set(token, 0x3e8, 0);
        this.#set(token, 0x3f0, 0);
        this.#set(token, 0x3f4, 0);
        this.#set(token, 0x3fc, 0);
        this.#set(token, 0x400, signed(choices) <= 0 ? 1 : 0);
        const flags = this.#type(this.#field(token, 0x3e0)).flags2 >>> 0;
        if (flags & 0x30000) {
            this.#set(token, 0x3e0, 0);
            this.#set(token, 0x3f0, 1);
        }
        if ((flags & 0x800) && !(this.#now() >= this.#time(token, 0x5b8))) {
            this.#set(token, 0x3e0, 0);
            this.#set(token, 0x3f0, 1);
        }
        if (this.#field(token, 0x3e0)) {
            const type = this.#field(token, 0x3e0);
            const direction = signed(this.#field(token, 0x3e4));
            this.#type(type);
            this.#log(token, AI_DROP_SELECTION_LOG.scanning, type, direction);
        }
    }
}
