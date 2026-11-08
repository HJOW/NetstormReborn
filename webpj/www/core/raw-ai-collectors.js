// AI 수집자 확보와 자기 영역 후보 배치를 원본 본체/목록/난수에 적용하는 모듈이다(cpppj `o/RawAiCollectors.cpp/.h`, 10.78: 패치 00415a10 수집자 확보·00415920 자기 영역 후보 배치·004108f0 일반 배치 통지·004115e0 비행 수집자 검사·00411ca0 취약 사제 검사).
//  - new RawAiCollectors(memory, pool, types, random, globals, hooks): memory 는 E164 `RawAiBrainMemory`(본체 0xb08 + 목록 블록), pool 은 같은 PATCH SidPool, types 는 현재 TYPE 의 `{ flags2 }` 표(비어 있지 않아야 함), random 은 같은 게임 공용 난수(`next(limit)`),
//    globals = { now()(0055b4d0 현재 QWORD 시각), localOwner()(00540c70 현재 로컬 소유자), invalidGraph()(00540410 현재 무효 그래프, 도달할 때만 요구) } 는 각 원본 조회 위치에서 현재 값을 다시 읽고,
//    hooks = { product(token, type)→SID(00410360/004103c0 같은 본체의 현재 기술 부모 mask 8 조회), canAfford(token, type)→bool(00410280 실제 signed 비용/지갑 비교), maxHitPoints(sid)→정수(004adcb0 현재 최대 HP, 현재 HP 는 호출 전에 저장),
//    enemyAvailable(owner)→bool(00492280 현재 Player flags22 와 ownerState 0), mayPlace(query)→bool(0049b510 전체 배치 검사, 질의는 `{ type, argument, x, y, flags, owner, mode }`), deploy(token, type, x, y, argument, direction)(004106e0 실제 생산 소비/건설),
//    log(token, type, x, y)(004105b0 현재 Lay 형식/로그 종류 장치, 좌표는 y→x 절삭 정수), dropSound()(로컬 owner 일 때만 004a9cb0 droppiece.wav) } 는 도달한 위치에서만 요구한다(없으면 그 위치에서 거부하고 앞선 효과는 취소하지 않음).
//  - ensure(token): 00415a10 — 신전(+0x294)/graph(+0x298)/후보(+0x28c) 관문을 통과하면 (1) 취약한 적 사제용 비행 수집자 경로(기한 +0x5a8·난수 1000 중 100 미만·+0x248 의 0x100 비트·+0x38c<+0x388·적 사제 SID 1.. 용량 미만·참여·취약·기한 +0x5e0·기존 비행 수집자 없음)에서 첫 생산 보유 비행(flags2 0x20000) 타입을
//    자기 영역에 배치하고 끝내거나, (2) 이미 충족(+0x2cc)이면 끝내고 아니면 난수로 고른 후보 타입(생산이 없으면 첫 후보)을 생산 재조회·기한·대기 조각 제외·비용 확인 뒤 자기 영역에 배치한다. 비행 경로는 일반 비용/대기 조각 판정을 추가하지 않는다.
//  - placeInHome(token, type): 00415920 — 매 시도마다 자기 경계(+0x270/+0x274/+0x278/+0x27c)를 다시 읽고 y → x 순서로 두 난수를 소비해 후보를 만들고, 전체 mayPlace 가 허용한 첫 후보만 배치하며 최대 20회 뒤에는 아무 조각도 만들지 않는다.
//  - deployWithNotice(token, type, x, y, argument, direction): 004108f0 — 실제 배치 → 생각 기한 두 개(현재 rate + 현재 시각, rate + 1 + 현재 시각, +0x5a8/+0x5b0 QWORD)와 +0xb00 이 켜졌을 때의 로그 → 현재 로컬 소유자와 같으면 드롭 음향 순서이다.
//  - hasFlying(token)·vulnerable(priest): 현재 수집자 목록(+0x2dc/+0x2e4)의 flags2 0x20000 검사와 현재 HP 선저장 → 최대 HP/2(정수 나눗셈) signed 비교이다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E179: AI 수집자 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 후보 배치 최대 반복 횟수, 비행 수집자 genus 비트, 대기 조각 비트이다. (2026-11-09 WB04-E179)
const ATTEMPTS = 20;
const FLYING = 0x20000;
const TWO_63 = 9223372036854775808;

// 비정렬 원본 DWORD 를 물리 범위 안에서 읽는다. (2026-11-09 WB04-E179)
function read(raw, offset) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 수집자 DWORD 읽기 범위');
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 선택된 DWORD 만 바꾸고 주변 본체를 보존한다. (2026-11-09 WB04-E179)
function write(raw, offset, value) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 수집자 DWORD 쓰기 범위');
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 원본 signed 분기의 비트 해석이다. (2026-11-09 WB04-E179)
const signed = (value) => value | 0;

// CRT ftol 의 signed64 절삭 후 하위 DWORD 이며 무효 FISTP 의 하위 DWORD 는 0 이다. (2026-11-09 WB04-E179)
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

// 단정도 float 와 비트의 상호 변환이다. (2026-11-09 WB04-E179)
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 선택된 하위의 누락을 도달한 위치에서만 명시 오류로 남긴다. (2026-11-09 WB04-E179)
function need(hook, message) {
    if (typeof hook !== 'function') throw new Error(message);
    return hook;
}

// 수집자 확보 처리기이다. 생성은 게임 판단/난수/필드 변경을 실행하지 않는다. (2026-11-09 WB04-E179)
export class RawAiCollectors {
    #memory;
    #pool;
    #types;
    #random;
    #globals;
    #hooks;

    // 같은 PATCH 풀/현재 TYPE 와 난수, 현재 전역/하위를 빌린다. (2026-11-09 WB04-E179)
    constructor(memory, pool, types, random, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 수집자 본체 소유자 누락');
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length === 0) throw new TypeError('AI 수집자 같은 PATCH/TYPE 계약 누락');
        if (typeof random?.next !== 'function') throw new TypeError('AI 수집자 공용 난수 누락');
        for (const name of ['now', 'localOwner']) {
            if (typeof globals?.[name] !== 'function') throw new TypeError(`AI 수집자 현재 전역 조회기 누락: ${name}`);
        }
        this.#memory = memory;
        this.#pool = pool;
        this.#types = types;
        this.#random = random;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 실제 월드 조립에서 같은 풀 연결을 확인하기 위한 풀이다. (2026-11-09 WB04-E179)
    get pool() { return this.#pool; }

    // 해제되거나 다른 크기의 가짜 본체를 거부하고 현재 소유 블록을 다시 조회한다. (2026-11-09 WB04-E179)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 수집자 실제0xb08 본체 누락');
        return raw;
    }

    // 외부 효과 뒤에도 현재 본체를 다시 읽는다. (2026-11-09 WB04-E179)
    #field(token, offset) { return read(this.#body(token), offset); }

    // 현재 본체의 해당 DWORD 만 쓴다. (2026-11-09 WB04-E179)
    #setField(token, offset, value) { write(this.#body(token), offset, value >>> 0); }

    // 목록의 개수와 별개로 현재 backing 안의 실제 칸만 읽는다. (2026-11-09 WB04-E179)
    #item(token, pointer, index) {
        if (index < 0) throw new RangeError('AI 수집자 음수 목록 인덱스');
        return read(this.#memory.bytes(this.#field(token, pointer)), index * 4);
    }

    // 현재 TYPE 0 도 허용하되 표 밖 접근은 거부한다. (2026-11-09 WB04-E179)
    #type(type) {
        if (type >= this.#types.length) throw new RangeError('AI 수집자 현재 TYPE 범위');
        return this.#types[type];
    }

    // 원본 적 유효성은 별도로 0 을 제외하며 이 물리 getter 는 0 도 보존한다. (2026-11-09 WB04-E179)
    #physicalSid(value) {
        if (value >= this.#pool.capacity || value > 65535) throw new RangeError('AI 수집자 현재 SID 범위');
        return value;
    }

    // 원본 QWORD 기한 ≤ 현재 시각이며 unordered/기한 전은 모두 거짓이다. (2026-11-09 WB04-E179)
    #ready(token, offset) { return this.#globals.now() >= deadline(this.#field(token, offset), this.#field(token, offset + 4)); }

    // 같은 원본 생산 조회를 다시 호출한 뒤 현재 조각의 pending bit(0x100)만 제외한다. (2026-11-09 WB04-E179)
    #availableProduct(token, type) {
        const form = need(this.#hooks.product, 'AI 수집자 현재 생산 조회 미연결')(token, type);
        return form && (this.#pool.slot(form)[21] & 1) ? 0 : form;
    }

    // 00410620: 현재 rate(float)와 QWORD 시각으로 두 생각 기한을 쓰고 +0xb00 이 켜졌을 때만 현재 Lay 로그를 남긴다(좌표는 y → x 절삭). (2026-11-09 WB04-E179)
    #notice(token, type, x, y) {
        const iy = signed(truncate(y));
        const ix = signed(truncate(x));
        this.#type(type);
        const rate = bitsFloat(this.#field(token, 0x228));
        const view = new DataView(new ArrayBuffer(16));
        view.setFloat64(0, rate + this.#globals.now(), true);
        view.setFloat64(8, (rate + 1) + this.#globals.now(), true);
        this.#setField(token, 0x5a8, view.getUint32(0, true));
        this.#setField(token, 0x5ac, view.getUint32(4, true));
        this.#setField(token, 0x5b0, view.getUint32(8, true));
        this.#setField(token, 0x5b4, view.getUint32(12, true));
        if (this.#field(token, 0xb00)) need(this.#hooks.log, 'AI 수집자 현재 Lay 로그 미연결')(token, type, ix, iy);
    }

    // 004115e0: free/dead/추가 소유자 필터 없이 현재 수집자 목록의 TYPE genus 만 검사한다(하위가 목록을 바꾸면 다음 원문 읽기에서 반영). (2026-11-09 WB04-E179)
    hasFlying(token) {
        for (let index = 0; index < signed(this.#field(token, 0x2e4)); index += 1) {
            if (this.#type(this.#pool.slot(this.#physicalSid(this.#item(token, 0x2dc, index)))[10]).flags2 & FLYING) return true;
        }
        return false;
    }

    // 00411ca0: 최대 HP getter 의 효과가 HP 를 바꾸더라도 먼저 읽은 현재 HP 로 원본 비교를 한다(정수 나눗셈 signed). (2026-11-09 WB04-E179)
    vulnerable(priest) {
        const hp = signed(read(this.#pool.slot(priest), 26));
        const maximum = need(this.#hooks.maxHitPoints, 'AI 수집자 현재 최대HP 조회 미연결')(priest);
        return hp < Math.trunc(signed(maximum) / 2);
    }

    // 004108f0: 일반 배치와 생각 기한/로그, 현재 로컬 소유자의 드롭 음향을 같은 수명에서 이어 실행한다. (2026-11-09 WB04-E179)
    deployWithNotice(token, type, x, y, argument, direction) {
        const fx = Math.fround(x);
        const fy = Math.fround(y);
        need(this.#hooks.deploy, 'AI 수집자 실제 생산/건설 배치 미연결')(token, type, fx, fy, argument, direction);
        this.#notice(token, type, fx, fy);
        if (this.#field(token, 0x260) === (this.#globals.localOwner() >>> 0)) need(this.#hooks.dropSound, 'AI 수집자 로컬 드롭 음향 미연결')();
    }

    // 00415920: 매 시도에 경계를 snapshot 하고 y → x 순서로 두 원본 난수를 소비한다. (2026-11-09 WB04-E179)
    placeInHome(token, type) {
        // 전체 mayPlace 가 허용한 첫 후보만 배치하고 최대 20회 뒤에는 아무 조각도 만들지 않는다.
        for (let remaining = ATTEMPTS; remaining !== 0; remaining -= 1) {
            const left = bitsFloat(this.#field(token, 0x270));
            const top = bitsFloat(this.#field(token, 0x274));
            const right = bitsFloat(this.#field(token, 0x278));
            const bottom = bitsFloat(this.#field(token, 0x27c));
            const y = Math.fround(signed(this.#random.next(truncate(bottom - top))) + top);
            const x = Math.fround(signed(this.#random.next(truncate(right - left))) + left);
            const query = { type, argument: 0, x, y, flags: 0, owner: this.#field(token, 0x260) & 255, mode: 1 };
            this.#type(type);
            if (need(this.#hooks.mayPlace, 'AI 수집자 전체 MayPlace 미연결')(query)) {
                this.deployWithNotice(token, type, x, y, 0, 0);
                return;
            }
        }
    }

    // 00415a10: 원본 취약 사제용 비행 수집자 경로는 일반 비용/대기 조각 판정을 추가로 적용하지 않는다. (2026-11-09 WB04-E179)
    ensure(token) {
        if (!this.#field(token, 0x294) || this.#field(token, 0x298) === need(this.#globals.invalidGraph, 'AI 수집자 현재 무효 그래프 조회 미연결')() || !this.#field(token, 0x28c)) return;
        const enemy = this.#field(token, 0x2a0);
        if (this.#ready(token, 0x5a8) && this.#random.next(1000) < 100 && (this.#field(token, 0x248) & 0x100)
            && signed(this.#field(token, 0x38c)) < signed(this.#field(token, 0x388)) && enemy >= 1 && enemy < this.#pool.capacity
            && need(this.#hooks.enemyAvailable, 'AI 수집자 현재 적 사제 참여 조회 미연결')(this.#field(token, 0x264))
            && this.vulnerable(this.#physicalSid(this.#field(token, 0x2a0))) && this.#ready(token, 0x5e0) && !this.hasFlying(token)) {
            // 현재 후보 목록을 원래 순서로 방문해 첫 생산 보유 비행 타입에서 끝낸다.
            for (let index = 0; index < signed(this.#field(token, 0x28c)); index += 1) {
                const type = this.#item(token, 0x284, index);
                if (need(this.#hooks.product, 'AI 수집자 현재 생산 조회 미연결')(token, type) && (this.#type(type).flags2 & FLYING)) {
                    this.placeInHome(token, type);
                    return;
                }
            }
        }
        if (this.#field(token, 0x2cc)) return;
        const index = signed(this.#random.next(this.#field(token, 0x28c)));
        let type = this.#item(token, 0x284, index);
        if (!need(this.#hooks.product, 'AI 수집자 현재 생산 조회 미연결')(token, type)) type = this.#item(token, 0x284, 0);
        if (need(this.#hooks.product, 'AI 수집자 현재 생산 재조회 미연결')(token, type) && this.#ready(token, 0x5a8) && this.#availableProduct(token, type) && need(this.#hooks.canAfford, 'AI 수집자 실제 비용 비교 미연결')(token, type)) {
            this.placeInHome(token, type);
        }
    }
}
