// AI 의 간헐천 선택·신전 귀환과 포획 운반자/끊긴 간헐천 목록을 같은 본체에서 처리하는 모듈이다(cpppj `o/RawAiResourceChoice.cpp/.h`, 10.78: 패치 00410a30·00410980·004109d0·004112b0·00411350·00411900).
//  - new RawAiResourceChoice(memory, pool, types, invalidGraph, hooks): memory 는 E164 `RawAiBrainMemory`, pool 은 같은 PATCH SidPool, types 는 현재 188 TYPE({ flags2 })의 같은 배열(보조 genus 를 합성하지 않는다), invalidGraph() 는 같은 장부의 현재 무효 그래프(선택된 원본 위치에서 읽음),
//    hooks = { center(sid)→점(004adea0 현재 신전 발 중심), graph(sid)→DWORD(00462bc0 실제 현재 그래프), crystals(sid)→int(004acc60 내용물 총수), target(token, actor, target)(004116d0 같은 실제 이동), home(token, sid)(004118a0 신전 복귀),
//    captureLog(token, sid)(004105b0 종류 29 현재 장치 로그), disconnectedLog(token, sid)(현재 끊긴 간헐천 종류 로그), assertion(text)(원본의 유효 간헐천 진단, 반환하면 계속) } 이다. 별도 공략/대상 복사본 없이 본체의 +0x330/+0x348/+0x384 목록(포인터/capacity/count)과 실제 월드를 사용한다.
//  - homeCenter(token): 현재 신전이 있으면 center hook, 없으면 본체 +0x268/+0x26c 원문 float 를 그대로(NaN/부호 0 은 xBits/yBits 로 보존) 돌려준다.
//  - select(token, list, point, excludedGraph, requiredGraph): 00410a30 — 목록의 각 객체까지 거리 제곱을 signed 로 절삭하고(그래프는 거리 탈락 후보에서도 조회) 초기 65536 보다 strict 하게 작으며 제외/필수 그래프(현재 무효 sentinel 이면 조건 없음)를 통과하는 첫 최소 객체를 고른다.
//  - collectOrHome(token, actor): 00411900 — 신전이 없거나 현재 그래프가 무효면 아무것도 하지 않고, 크리스털이 없고 수집 능력(+0x248 bit 0x10)이 있으면 신전 중심/actor 그래프로 +0x330 목록에서 대상을 골라 이동, 없으면 귀환한다.
//  - hasFlyingCaptureCarrier(token): +0x384 목록의 현재 TYPE flags2 0x20000 만 검사. rememberCaptureCarrier(token, actor): 최초 count/포인터에서 중복이면 로그도 생략하고, full 이면 추가만 생략한 채 현재 +0xb00 이면 로그한다(기한을 바꾸지 않음).
//  - reclassifyDisconnected(token): 00411350 — +0x330 의 다른 그래프 간헐천을 모든 중복까지 안정 제거해 +0x348 에 순서대로 옮기며(대상 full 이면 추가만 생략) SID 0/비간헐천은 진단 뒤 무효 그래프로 계속한다. 로그 뒤 현재 원문을 재조회한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E175: AI 자원 선택 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 00410a42 의 최소 거리 제곱 초기값(float)과 간헐천/비행 genus 비트, 목록 필드 위치이다. (2026-11-09 WB04-E175)
const INITIAL_DISTANCE = 65536;
const GEYSER = 0x10000000;
const FLYING = 0x20000;
const GEYSERS = 0x330;
const DISCONNECTED = 0x348;
const CAPTURE = 0x384;
const TWO_63 = 9223372036854775808;

// 원본 비정렬 little endian DWORD 를 물리 범위 안에서 읽는다. (2026-11-09 WB04-E175)
function read(bytes, at) {
    if (at < 0 || at > bytes.length || bytes.length - at < 4) throw new RangeError('AI 자원 선택 DWORD 범위');
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 현재 지정 DWORD 만 바꾸고 주변 본체·여유 원문을 보존한다. (2026-11-09 WB04-E175)
function put(bytes, at, value) {
    if (at < 0 || at > bytes.length || bytes.length - at < 4) throw new RangeError('AI 자원 선택 쓰기 범위');
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 0xff;
}

// 원본 DWORD 의 signed 비교를 구현 의존 변환 없이 적용한다. (2026-11-09 WB04-E175)
const signed = (value) => value | 0;

// 원본 CRT ftol 의 signed64 절삭 하위 DWORD(signed)이며 무효 FISTP 의 하위 DWORD 도 0 이다. (2026-11-09 WB04-E175)
function ftol(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 단정도 float 비트를 수로 바꾼다. (2026-11-09 WB04-E175)
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 선택된 실제 하위 누락을 빈 목록/가짜 이동 성공으로 대신하지 않는다. (2026-11-09 WB04-E175)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 자원 선택 실제 하위 미연결');
    return hook;
}

// 간헐천 선택/귀환/포획 대기·끊긴 간헐천 처리기이다. (2026-11-09 WB04-E175)
export class RawAiResourceChoice {
    #memory;
    #pool;
    #types;
    #invalidGraph;
    #hooks;

    // 생성은 현재 원문을 읽거나 어떤 게임 효과도 발생시키지 않는다. (2026-11-09 WB04-E175)
    constructor(memory, pool, types, invalidGraph, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 자원 선택 본체 소유자 누락');
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== 188 || typeof invalidGraph !== 'function') throw new TypeError('AI 자원 선택 같은 PATCH188 누락');
        this.#memory = memory;
        this.#pool = pool;
        this.#types = types;
        this.#invalidGraph = invalidGraph;
        this.#hooks = hooks ?? {};
    }

    // 같은 실제 월드의 풀인지 연결 단계에서 확인할 수 있도록 제공한다. (2026-11-09 WB04-E175)
    get pool() { return this.#pool; }

    // 해제 핸들이나 작은 가짜 본체를 거부한다. (2026-11-09 WB04-E175)
    #body(token) {
        const bytes = this.#memory.bytes(token);
        if (bytes.length !== AI_BRAIN_BYTES) throw new RangeError('AI 자원 선택 실제0xb08 본체 누락');
        return bytes;
    }

    // 실제 현재 본체의 DWORD 를 다시 읽는다. (2026-11-09 WB04-E175)
    #field(token, at) { return read(this.#body(token), at); }

    // 비양수 count 는 원본의 자연 루프 종료를 유지한다(목록 + 8 의 signed count). (2026-11-09 WB04-E175)
    #count(token, list) { return signed(this.#field(token, list + 8)); }

    // 손상된 음수 인덱스의 호스트 주소 접근만 명시 거부하고 현재 실제 포인터의 DWORD 를 읽는다. (2026-11-09 WB04-E175)
    #item(token, list, index) {
        if (index < 0) throw new RangeError('AI 자원 선택 음수 인덱스');
        return read(this.#memory.bytes(this.#field(token, list)), index * 4);
    }

    // 용량이 없으면 실제 목록 포인터에 접근하지 않고 full 에서는 일반 중복을 유지한다. (2026-11-09 WB04-E175)
    #append(token, list, value) {
        const count = this.#count(token, list);
        if (count >= signed(this.#field(token, list + 4))) return;
        if (count < 0) throw new RangeError('AI 자원 선택 음수 추가 인덱스');
        put(this.#memory.bytes(this.#field(token, list)), count * 4, value);
        put(this.#body(token), list + 8, (count + 1) >>> 0);
    }

    // 제거한 중복 수만 count 에서 빼며 남은 원소의 상대 순서를 유지한다(inactive 꼬리는 보존). (2026-11-09 WB04-E175)
    #remove(token, list, value) {
        let removed = 0;
        // 현재 count/포인터로 전체 active 목록을 안정 압축한다.
        for (let index = 0; index < this.#count(token, list); index += 1) {
            const current = this.#item(token, list, index);
            if (current === (value >>> 0)) removed += 1;
            else put(this.#memory.bytes(this.#field(token, list)), (index - removed) * 4, current);
        }
        put(this.#body(token), list + 8, (this.#count(token, list) - removed) >>> 0);
    }

    // 잘못된 DWORD 를 다른 정상 SID 로 잘라서 사용하지 않는다(0 도 실제 슬롯). (2026-11-09 WB04-E175)
    #object(value) {
        if (value >= this.#pool.capacity || value > 65535) throw new RangeError('AI 자원 선택 실제SID 범위');
        return value;
    }

    // free/dead/소유자 필터를 추가하지 않고 현재 TYPE 의 순수 flags2 를 읽는다. (2026-11-09 WB04-E175)
    #genus(value) {
        const type = this.#pool.slot(this.#object(value))[10];
        if (type >= this.#types.length) throw new RangeError('AI 자원 선택 TYPE 범위');
        return this.#types[type].flags2 >>> 0;
    }

    // 004112b0: 신전이 없으면 center 하위를 요구하지 않고 두 raw float 를 그대로 반환한다. (2026-11-09 WB04-E175)
    homeCenter(token) {
        const temple = this.#field(token, 0x294);
        if (temple) return need(this.#hooks.center)(this.#object(temple));
        // 원문 float 비트(NaN 페이로드/부호 0)도 xBits/yBits 로 함께 돌려준다.
        return { x: bitsFloat(this.#field(token, 0x268)), y: bitsFloat(this.#field(token, 0x26c)), xBits: this.#field(token, 0x268), yBits: this.#field(token, 0x26c) };
    }

    // 00410a30: 거리값이 같으면 먼저 방문한 대상을 유지하며 그래프는 거리 탈락 후보에서도 조회한다. (2026-11-09 WB04-E175)
    select(token, list, point, excludedGraph, requiredGraph) {
        let best = INITIAL_DISTANCE;
        let selected = 0;
        // 그래프 getter 의 효과 뒤 다음 원소/현재 count 와 포인터를 다시 읽는다.
        for (let index = 0; index < this.#count(token, list); index += 1) {
            const value = this.#item(token, list, index);
            const raw = this.#pool.slot(this.#object(value));
            const x = point.x - bitsFloat(read(raw, 14));
            const y = point.y - bitsFloat(read(raw, 18));
            const distance = ftol(x * x + y * y);
            const graph = need(this.#hooks.graph)(this.#object(value));
            if (!(distance < best)) continue;
            const invalid = need(this.#invalidGraph)() >>> 0;
            if (((excludedGraph >>> 0) === invalid || (graph >>> 0) !== (excludedGraph >>> 0)) && ((requiredGraph >>> 0) === invalid || (requiredGraph >>> 0) === (graph >>> 0))) {
                best = Math.fround(distance);
                selected = value;
            }
        }
        return this.#object(selected);
    }

    // 00411900: 첫 관문 뒤에는 초기 신전/그래프 상태를 다시 검사하지 않고 원래 순서의 실제 하위를 호출한다. (2026-11-09 WB04-E175)
    collectOrHome(token, actor) {
        const hooks = this.#hooks;
        if (!this.#field(token, 0x294) || this.#field(token, 0x298) === (need(this.#invalidGraph)() >>> 0)) return;
        if (!need(hooks.crystals)(actor) && (this.#field(token, 0x248) & 0x10)) {
            const center = this.homeCenter(token);
            const graph = need(hooks.graph)(actor);
            const excluded = need(this.#invalidGraph)();
            const target = this.select(token, GEYSERS, center, excluded, graph);
            if (target) {
                need(hooks.target)(token, actor, target);
                return;
            }
        }
        need(hooks.home)(token, actor);
    }

    // 00410980: 물리 슬롯 0 이나 free 항목도 원본처럼 현재 TYPE 로만 분류하며 현재 포획 대기 목록만 방문한다. (2026-11-09 WB04-E175)
    hasFlyingCaptureCarrier(token) {
        // 현재 포획 대기 목록만 방문한다.
        for (let index = 0; index < this.#count(token, CAPTURE); index += 1) if (this.#genus(this.#item(token, CAPTURE, index)) & FLYING) return true;
        return false;
    }

    // 004109d0: 중복은 로그도 생략하고 full 은 로그까지 진행한다(기한/사제 HP 를 갱신하지 않는다). (2026-11-09 WB04-E175)
    rememberCaptureCarrier(token, actor) {
        const count = this.#count(token, CAPTURE);
        if (count > 0) {
            const list = this.#memory.bytes(this.#field(token, CAPTURE));
            // 최초 count/포인터의 중복을 확인한다.
            for (let index = 0; index < count; index += 1) if (read(list, index * 4) === actor) return;
        }
        this.#append(token, CAPTURE, actor);
        if (this.#field(token, 0xb00)) need(this.#hooks.captureLog)(token, actor);
    }

    // 00411350: 로그 뒤 현재 원문을 재조회하며 연속된 제거 항목을 건너뛰지 않는다. (2026-11-09 WB04-E175)
    reclassifyDisconnected(token) {
        const hooks = this.#hooks;
        let index = 0;
        // 다른 그래프 항목은 같은 인덱스에서 중복을 모두 제거한 뒤 다음 현재 원소를 검사한다.
        while (index < this.#count(token, GEYSERS)) {
            const value = this.#item(token, GEYSERS, index);
            if (!value || value >= this.#pool.capacity || !(this.#genus(value) & GEYSER)) need(hooks.assertion)('s.isValid() && (s.getGenus()&gGEYSER)');
            const graph = value ? need(hooks.graph)(this.#object(value)) : need(this.#invalidGraph)();
            if ((graph >>> 0) === this.#field(token, 0x298)) index += 1;
            else {
                if (this.#field(token, 0xb00)) need(hooks.disconnectedLog)(token, this.#object(value));
                this.#remove(token, GEYSERS, value);
                this.#append(token, DISCONNECTED, value);
            }
        }
    }
}
