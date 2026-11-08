// AI 피해 반응의 현재 목록과 조건부 추가·로그 순서를 원본 본체/목록에 적용하는 모듈이다(cpppj `o/RawAiDamage.cpp/.h`, 10.78: 패치 00415460 피해 통지·00414450 용량 우선/중복 방지 추가·00411200 미움 등록).
//  - new RawAiDamage(memory, pool, types, hooks): memory 는 E164 `RawAiBrainMemory`(본체 0xb08 + 네 목록 블록), pool 은 같은 PATCH SidPool(피해/공격자/홈 raw 슬롯), types 는 현재 188 TYPE 의 `{ flags2, group }` 표,
//    hooks = { home(attacker)→SID(0045c100 같은 Host/FlyerProcessFields::Home 조회), log(token, { more, attacker, type, x, y })(004105b0 실제 소비자, `more` 가 false 면 "I hate", true 면 "I hate MORE" 서식이고 좌표는 y→x 절삭 정수) } 는 도달한 위치에서만 요구한다(없으면 그 위치에서 거부하고 앞선 효과는 취소하지 않음).
//  - notice(token, victim, attacker): 00415460 — 공격자가 0 이면 아무것도 하지 않고, 피해 대상 TYPE genus(TYPE 164 는 0x400000 추가)와 현재 네 목록(+0x360 방어 · +0x36c 미움 · +0x378 매우 미움 · +0x384 추적)으로 두 단계 반응을 순서대로 수행한다.
//    추적 목록에 이미 있는 대상이거나 genus 0x80604200 이면 미움/매우 미움이 되고(genus 0x80404200 이고 공격자가 매우 미움 목록에 없으면 +0x404 에 능력 비트 1 을 기록), 방어 genus 0x4000000 이고 방어 목록에 있으면 미움이 되며, 추적 genus 0x30000 이 추적 목록에 있거나 미움이면 공격자를 미움 목록에 등록하고
//    공격자가 비행(0x100000)이면 같은 홈(group 3, free 가 아님, dead 는 거부하지 않음)도 등록하며, 매우 미움이고 공격자가 매우 미움 목록에 없으면 추가(비행 홈 포함) 뒤 "I hate MORE" 로그를 요청한다. 미움 등록은 0/중복에서만 반환하고 목록이 가득 차도 로그는 호출한다.
//  - 목록 추가는 00414450 처럼 가득 차면 중복 검색도 하지 않고, count 가 음수면 거부하며, 호출 위치마다 현재 공격자 TYPE/좌표를 다시 읽는다. 목록 순서/중복 검사는 현재 active count 안에서만 한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E180: AI 피해 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 원본 네 목록(방어/미움/매우 미움/추적)의 본체 위치(pointer/capacity/count 12바이트)와 비행 genus 비트이다. (2026-11-09 WB04-E180)
const GUARD = 0x360;
const HATE = 0x36c;
const VERY = 0x378;
const TRACKED = 0x384;
const FLYING = 0x100000;
const TWO_63 = 9223372036854775808;

// 비정렬 little endian DWORD 만 읽는다. (2026-11-09 WB04-E180)
function read(raw, offset) {
    if (offset < 0 || offset + 4 > raw.length) throw new RangeError('AI 피해 DWORD 범위 오류');
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 주변 padding 을 보존하는 실제 DWORD 쓰기이다. (2026-11-09 WB04-E180)
function write(raw, offset, value) {
    if (offset < 0 || offset + 4 > raw.length) throw new RangeError('AI 피해 쓰기 범위 오류');
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 정상 유한 좌표의 원본 signed64 절삭 후 낮은 DWORD(signed)를 보존하며 비유한/범위 밖은 호스트 계약 오류이다. (2026-11-09 WB04-E180)
function coordinate(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) throw new RangeError('AI 피해 CRT 좌표 범위 오류');
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 단정도 float 비트를 수로 바꾼다. (2026-11-09 WB04-E180)
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// AI 피해 반응 처리기이다. 생성은 실제 효과를 일으키지 않는다. (2026-11-09 WB04-E180)
export class RawAiDamage {
    #memory;
    #pool;
    #types;
    #hooks;

    // 같은 실제 PATCH 타입과 명시 효과만 보관한다. (2026-11-09 WB04-E180)
    constructor(memory, pool, types, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 피해 본체 소유자 누락');
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== 188) throw new TypeError('AI 피해는 PATCH188의 같은 실제 세계가 필요합니다');
        this.#memory = memory;
        this.#pool = pool;
        this.#types = types;
        this.#hooks = hooks ?? {};
    }

    // 같은 실제 월드에 연결하는지 검사할 수 있는 풀 참조이다. (2026-11-09 WB04-E180)
    get pool() { return this.#pool; }

    // 해제되었거나 크기가 다른 본체를 목록 주소로 해석하지 않는다. (2026-11-09 WB04-E180)
    #brain(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 피해 실제0xb08 본체 누락');
        return raw;
    }

    // 원본은 count>0 일 때 현재 목록을 앞에서부터 검색하며 첫 중복에서 반환한다. (2026-11-09 WB04-E180)
    #contains(token, offset, sid) {
        const raw = this.#brain(token);
        const count = read(raw, offset + 8) | 0;
        if (count <= 0) return false;
        const list = this.#memory.bytes(read(raw, offset));
        if (count > Math.floor(list.length / 4)) throw new RangeError('AI 피해 목록 count 범위 오류');
        for (let index = 0; index < count; index += 1) if (read(list, index * 4) === sid) return true;
        return false;
    }

    // 00414450: full 이면 검색도 하지 않는 순서이다. (2026-11-09 WB04-E180)
    #append(token, offset, sid) {
        const raw = this.#brain(token);
        const count = read(raw, offset + 8) | 0;
        const capacity = read(raw, offset + 4) | 0;
        if (count >= capacity) return;
        if (count < 0) throw new RangeError('AI 피해 음수 목록 count 오류');
        if (this.#contains(token, offset, sid)) return;
        write(this.#memory.bytes(read(raw, offset)), count * 4, sid);
        write(raw, offset + 8, count + 1);
    }

    // 현재 TYPE 의 genus 이며 free/dead 는 호출 지점의 별도 조건이다. (2026-11-09 WB04-E180)
    #genus(sid) {
        const raw = this.#pool.slot(sid);
        if (raw[10] >= this.#types.length) throw new RangeError('AI 피해 현재 TYPE 오류');
        return this.#types[raw[10]].flags2 >>> 0;
    }

    // 홈의 범위/free/group 3 조건이며 dead 비트는 거부하지 않는다(원본의 포인터 범위는 SID 1 도 허용). (2026-11-09 WB04-E180)
    #validHome(sid) {
        if (!sid || sid >= this.#pool.capacity) return false;
        const raw = this.#pool.slot(sid);
        if (raw[11] & 1) return false;
        if (raw[10] >= this.#types.length) throw new RangeError('AI 피해 홈 TYPE 오류');
        return this.#types[raw[10]].group === 3;
    }

    // 좌표는 Y/X 순서로 절삭하고 TYPE 은 그 뒤 현재 슬롯에서 조회해 실제 로그 소비자에 전달한다(호출 뒤에는 저장한 포인터를 재사용하지 않음). (2026-11-09 WB04-E180)
    #log(token, sid, more) {
        if (typeof this.#hooks.log !== 'function') throw new Error('AI 피해 실제 로그004105b0 미연결');
        const raw = this.#pool.slot(sid);
        const x = bitsFloat(read(raw, 14));
        const y = bitsFloat(read(raw, 18));
        const iy = coordinate(y);
        const ix = coordinate(x);
        this.#hooks.log(token, { more, attacker: sid, type: this.#pool.slot(sid)[10], x: ix, y: iy });
    }

    // 00411200: 실제 hate 의 full 자연 경로도 로그 요청을 남기며 0/중복에서만 반환한다. (2026-11-09 WB04-E180)
    #hate(token, sid) {
        if (!sid || this.#contains(token, HATE, sid)) return;
        this.#append(token, HATE, sid);
        this.#log(token, sid, false);
    }

    // 비행 공격자의 홈 조회가 필요한 위치에서만 홈 하위를 요구한다. (2026-11-09 WB04-E180)
    #home(attacker, message) {
        if (typeof this.#hooks.home !== 'function') throw new Error(message);
        return this.#hooks.home(attacker);
    }

    // 00415460: 피해 대상 genus 와 네 현재 목록으로 두 단계 hate 반응을 순서대로 수행한다. (2026-11-09 WB04-E180)
    notice(token, victim, attacker) {
        if (!attacker) return;
        this.#brain(token);
        let genus = this.#genus(victim);
        if (this.#pool.slot(victim)[10] === 164) genus = (genus | 0x400000) >>> 0;
        let hate = this.#contains(token, TRACKED, victim);
        let very = hate;
        if (genus & 0x80604200) {
            hate = true;
            very = true;
            if ((genus & 0x80404200) && !this.#contains(token, VERY, attacker)) {
                const raw = this.#brain(token);
                write(raw, 0x404, (read(raw, 0x248) >>> 1) & 1);
            }
        }
        if ((genus & 0x4000000) && this.#contains(token, GUARD, victim)) hate = true;
        if (((genus & 0x30000) && this.#contains(token, TRACKED, victim)) || hate) {
            this.#hate(token, attacker);
            if (this.#genus(attacker) & FLYING) {
                const home = this.#home(attacker, 'AI 피해 실제 비행 홈0045c100 미연결');
                if (this.#validHome(home)) this.#hate(token, home);
            }
        }
        if (very && !this.#contains(token, VERY, attacker)) {
            this.#append(token, VERY, attacker);
            if (this.#genus(attacker) & FLYING) {
                const home = this.#home(attacker, 'AI 피해 실제 very 비행 홈0045c100 미연결');
                if (this.#validHome(home)) this.#append(token, VERY, home);
            }
            this.#log(token, attacker, true);
        }
    }
}
