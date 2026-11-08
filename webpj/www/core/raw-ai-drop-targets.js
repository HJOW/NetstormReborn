// AI 건물 배치의 실제 표적 점수와 Gun/장애물 후보 평가를 옮긴 모듈이다(cpppj `o/RawAiDropTargets.cpp/.h`, 10.78: 패치 00412770·004128f0·00412950·00412a70·0048ece0).
//  - new RawAiDropTargets(memory, pool, types, globals, hooks): memory 는 E164 `RawAiBrainMemory`, pool 은 같은 PATCH SidPool, types 는 현재 188 TYPE({ flags2, group })의 같은 배열, globals = { altarType()(005412e8 현재 제단 TYPE),
//    directionalType()(00541290 현재 윈드 블로커 TYPE) }, hooks = { captive(sid)→SID(004acb80 실제 첫 사제 내용물), rate(sid)→float(004ac550→0049acf0 현재 TYPE+48 hpPerSec), maxHitPoints(sid)→int32(004adcb0, 현재 HP 는 먼저 저장),
//    gun(x, y, type, direction, owner)→SID(0046b6f0 실제 Archer/Cannon finder), ground(x, y, type, owner)→SID(0046b540 별도 실제 지상 검색), blockers(owner, point, capacity)→SID DWORD 배열(00469590 같은 Gun 수집, 임시 용량 30),
//    center(sid)→점(004adea0 현재 실제 발자국 중심), assertion(text, line)(SID 0 운반체에서 원본 진단 후 계속) } 이며 도달한 위치에서만 요구한다. 출력 SID 는 `{ value }` 상자이다.
//  - score(token, target): 00412770 전체 — 가산점(기본 10, flags2 0x4000000 이면 5, 0x200000 이면 +(본체 +0x2a8 비영 ? 5 : 200), 0x200 +6, group 6 +7, 현재 제단 TYPE +8, 운반 genus 0x30000 에서 포획 사제가 있으면 +400) →
//    현재 hpPerSec 를 더한 값을 ftol(하위 DWORD)한 뒤 → 본체 두 목록(+0x36c/+0x378)의 최초 포인터/count 에서 첫 일치마다 각 +100 → 집중 대상(+0x3f8)이면 HP(getter 전에 저장)가 signed 최대 HP/2(0 방향) 이상일 때 +50 →
//    마지막 owner BYTE 가 적 owner DWORD(+0x264) 전체와 다르면 signed 절반(0 방향)이다. gun/ground: 실제 검색 결과를 점수 계산 전에 출력에 쓰고(0 도 기록) 비영만 점수화한다. blockers: 모든 등장의 점수를 더하고(DWORD 감김) 방향 블로커 TYPE 이면
//    점수 뒤 두 축 −1 로 facing 을 확인해 불일치는 점수 0 으로 하며, 양수 최고 갱신 때는 원본처럼 목록 첫 SID 를 출력한다. facing: flags2 0x100000 이면 중심 조회 전에 false, 아니면 실제 중심과의 방향이 네 방향 중 하나이며 같아야 한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E172: AI 배치 표적 점수 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';
import { RawSquidNeighbors } from './raw-squid-neighbors.js';

// PE 005012d0 의 두 축 보정값(발자국 절반과 다름), 00412950 의 임시 목록 용량(new 78h/count 1eh)이다. (2026-11-09 WB04-E172)
const BLOCKER_OFFSET = 1;
const BLOCKER_CAPACITY = 30;
const TWO_63 = 9223372036854775808;

// 비정렬 little endian DWORD 를 읽는다(범위 밖 거부). (2026-11-09 WB04-E172)
function read(raw, at) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 배치 표적 DWORD 범위');
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}

// DWORD 감김 뒤 signed 해석이다. (2026-11-09 WB04-E172)
const signed = (value) => value | 0;

// 원본 ftol 의 signed64 절삭 하위 DWORD(signed)이며 무효 FISTP 의 하위값은 0 이다. (2026-11-09 WB04-E172)
function ftol(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 선택되지 않은 하위는 요구하지 않고 선택된 미연결은 명시 실패한다. (2026-11-09 WB04-E172)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 배치 표적 실제 하위 미연결');
    return hook;
}

// 같은 본체/물리 풀/현재 TYPE 와 실제 검색 하위로만 점수·출력 SID 를 계산하는 표적 평가기이다. (2026-11-09 WB04-E172)
export class RawAiDropTargets {
    #memory;
    #pool;
    #types;
    #globals;
    #hooks;

    // 같은 PATCH 저장소를 빌리며 생성 중 검색하지 않는다. (2026-11-09 WB04-E172)
    constructor(memory, pool, types, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 배치 표적 본체 소유자 누락');
        if (pool?.edition !== 'patch1078') throw new TypeError('AI 배치 표적 같은 PATCH풀 누락');
        if (!Array.isArray(types) || typeof globals?.altarType !== 'function' || typeof globals.directionalType !== 'function') throw new TypeError('AI 배치 표적 TYPE 표/현재 전역 조회기 누락');
        this.#memory = memory;
        this.#pool = pool;
        this.#types = types;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 같은 실제 월드의 풀인지 확인할 수 있도록 제공한다. (2026-11-09 WB04-E172)
    get pool() { return this.#pool; }

    // 현재 본체의 정확한 크기와 수명을 확인한다. (2026-11-09 WB04-E172)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 배치 표적 실제0xb08본체 누락');
        return raw;
    }

    // 하위 효과 뒤 현재 본체를 다시 읽는다. (2026-11-09 WB04-E172)
    #field(token, at) { return read(this.#body(token), at); }

    // 원본 BYTE TYPE 의 현재 flags/group 레코드를 읽는다(표 밖은 거부). (2026-11-09 WB04-E172)
    #type(target) {
        const type = this.#pool.slot(target)[10];
        if (type >= this.#types.length) throw new RangeError('AI 배치 표적 현재TYPE 범위');
        return this.#types[type];
    }

    // 손상된 DWORD 를 정상 SID 로 잘라 쓰지 않는다. (2026-11-09 WB04-E172)
    #object(value) {
        if (value >= this.#pool.capacity || value > 65535) throw new RangeError('AI 배치 표적 물리SID 범위');
        return value;
    }

    // 원본 최초 포인터/count 만 사용하며 중복 가산은 하지 않는다(첫 일치까지만 읽는다). (2026-11-09 WB04-E172)
    #contains(token, pointer, target) {
        const count = signed(this.#field(token, pointer + 8));
        if (count <= 0) return false;
        const list = this.#memory.bytes(this.#field(token, pointer));
        // 최초 개수에서 첫 일치까지만 읽는다.
        for (let index = 0; index < count; index += 1) if (read(list, index * 4) === target) return true;
        return false;
    }

    // 00412770: 정밀 C 가 누락한 group 가산과 실제 모든 점수를 기계어 순서대로 보존한다. (2026-11-09 WB04-E172)
    score(token, target) {
        const hooks = this.#hooks;
        const flags = this.#type(target).flags2 >>> 0;
        let value = (flags & 0x4000000) ? 5 : 10;
        if (flags & 0x200000) value += this.#field(token, 0x2a8) ? 5 : 200;
        if (flags & 0x200) value += 6;
        if (this.#type(target).group === 6) value += 7;
        if (this.#pool.slot(target)[10] === (this.#globals.altarType() >>> 0)) value += 8;
        if (flags & 0x30000) {
            if (!target) need(hooks.assertion)('sid', 0xaeb);
            if (need(hooks.captive)(target)) value += 400;
        }
        value = ftol(Math.fround(need(hooks.rate)(target)) + signed(value)) >>> 0;
        if (this.#contains(token, 0x36c, target)) value = (value + 100) >>> 0;
        if (this.#contains(token, 0x378, target)) value = (value + 100) >>> 0;
        if (this.#field(token, 0x3f8) === target) {
            const hp = signed(read(this.#pool.slot(target), 26));
            const maximum = need(hooks.maxHitPoints)(target) | 0;
            if (hp >= Math.trunc(maximum / 2)) value = (value + 50) >>> 0;
        }
        const result = signed(value);
        return this.#pool.slot(target)[34] === this.#field(token, 0x264) ? result : Math.trunc(result / 2);
    }

    // 004128f0: 출력은 검색 뒤 점수 계산 전이며 0 도 기록한다. (2026-11-09 WB04-E172)
    gun(token, type, point, direction, result) {
        const owner = this.#field(token, 0x260);
        const found = need(this.#hooks.gun)(point.x, point.y, type, direction, owner);
        result.value = found;
        return found ? this.score(token, found) : 0;
    }

    // 00412a70: 별도 지상 검색도 출력 0/점수 하위 실패 전 쓰기 순서를 보존한다. (2026-11-09 WB04-E172)
    ground(token, type, point, result) {
        const owner = this.#field(token, 0x260);
        const found = need(this.#hooks.ground)(point.x, point.y, type, owner);
        result.value = found;
        return found ? this.score(token, found) : 0;
    }

    // 0048ece0: 특수 flags 는 중심 조회 전에 반환하며 다른 방향값은 네 방향 중 하나와 정확히 같아야 한다. (2026-11-09 WB04-E172)
    facing(point, direction, target) {
        if (this.#type(target).flags2 & 0x100000) return false;
        const center = need(this.#hooks.center)(target);
        const facing = RawSquidNeighbors.direction(point.x, point.y, center.x, center.y);
        return (direction === 0 || direction === 2 || direction === 4 || direction === 6) && facing === direction;
    }

    // 00412950: 모든 등장마다 점수를 계산하고 같은 양의 최고값은 출력 SID 를 다시 쓰지 않는다(총점은 DWORD 감김). (2026-11-09 WB04-E172)
    blockers(token, type, point, direction, result) {
        const owner = this.#field(token, 0x260);
        const list = need(this.#hooks.blockers)(owner, point, BLOCKER_CAPACITY);
        let total = 0;
        let best = 0;
        // 원본 임시 목록의 순서/중복을 보존하고 방향 필터도 점수 계산 뒤 수행한다.
        for (const value of list) {
            const target = this.#object(value);
            let score = this.score(token, target);
            if (type === (this.#globals.directionalType() >>> 0) && !this.facing({ x: Math.fround(point.x - BLOCKER_OFFSET), y: Math.fround(point.y - BLOCKER_OFFSET) }, direction, target)) score = 0;
            if (score > best) {
                best = score;
                result.value = this.#object(list[0]);
            }
            total = (total + (score >>> 0)) >>> 0;
        }
        return signed(total);
    }
}
