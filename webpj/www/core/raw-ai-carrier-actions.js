// AI 수집자의 수집/포획/귀환/경로 취소와 필요한 원본 genus 조회·다이스 접근점을 원본 본체/목록/월드에 적용하는 모듈이다(cpppj `o/RawAiCarrierActions.cpp/.h`, 10.78: 패치 00412130 전체 행동·004acb80 내용물 genus 조회·004487e0 다이스 접근점·00411630/004116d0 경로·이동).
//  - findContainedGenus(pool, state, types, parent, mask, genus, index): 004acb80 — 현재 내용물의 kind WORD 로 TYPE genus 를 먼저 읽고(mask 에서 탈락한 후보라도 TYPE 범위를 거부) 전체 WORD mask 와 모든 genus 비트(부분 비트 불가), 일치 순번(DWORD 감소)을 검사해 첫 일치 SID(없으면 0)를 돌려준다. kind 는 BYTE 로 자르지 않는다.
//  - daisApproachPoint(pool, types, checking, invalid, assertion, dais): 004487e0 — checking()(005e4794 현재 DWORD getter)이 비영이고 다이스가 물리 범위(0 이 아니고 용량 미만) 밖이거나 genus 0x400000 이 아니면 진단 `assertion(text)` 을 호출하고(복귀하면 계속), 진단 뒤 현재 TYPE/좌표/실패점을 다시 읽어
//    유효한 다이스면 좌표 −3 의 float 쌍을, 아니면 현재 `invalid`(공유 실패점 `{ x, y }`)를 돌려준다. free/dead 필터는 추가하지 않는다.
//  - new RawAiCarrierActions(memory, pool, types, globals, hooks): memory 는 E164 `RawAiBrainMemory`, pool 은 같은 PATCH SidPool, types 는 현재 188 TYPE 의 `{ flags2 }` 표, globals = { now()(0055b4d0 현재 시각), boss()(00540bc4 현재 권한, 첫 크리스털 조회 뒤 판정), checking()(005e4794 free 진단 활성 DWORD), invalidGraph()(00540410 현재 무효 그래프, 도달할 때만 요구) },
//    hooks = { maxHitPoints(sid)(prefix 004adcb0, 결과가 버려져도 실행), prisoner(sid)→SID(004acb80 mask 20·genus 200000·index 0), path(sid)→SID(00411630 현재 TYPE/경로/종속), daisPoint(sid)→점(004487e0), point(token, sid, point)(004117e0 실제 좌표 이동), home(token, sid)(004118a0 신전 귀환),
//    enemyAvailable(owner)→bool(00492280), vulnerable(sid)→bool(00411ca0), flyingGetter(token)→bool(00410980 같은 384 목록 비행 여부), graph(sid)→DWORD(00410260→00462bc0), crystals(sid)→정수(004acc60 mask 20·크리스털 TYPE), firstCrystal(sid)→SID(004ac9f0), dropRequest(sid)(00427320→00427070),
//    destroy(sid)(첫 크리스털 Form 가상 +10 flags 0), target(token, sid, target)(004116d0), remember(token, sid)(004109d0 같은 384 등록/로그, 이동 실패여도 호출), collect(token, sid)(00411900), targets(sid)→[TargetA, TargetB](00426150), reclassify(token)(00411350), stop(sid)(00411680 경로 가상 삭제), assertion(text, line)(Ai.cpp 복귀 가능 진단) } 는 도달한 위치에서만 요구한다.
//  - act(token): 00412130 — (1) 물리 적 SID 가 있으면 최대 HP prefix, (2) 포획 목록(+0x384)에서 포획 사제를 든 정지 운반자를 가장 먼저 귀환(다이스가 있으면 접근점 이동, 없으면 신전 귀환)하고 포획 목록을 한 actor 로 축소(생각 기한/일반 수집자보다 우선),
//    (3) 일반 수집자 목록(+0x2dc)이 비어 있거나 기한(+0x5a8, less/unordered 는 모두 미도달)이 아니면 종료, 아니면 순환 cursor(+0x6d0, signed 나머지)로 한 수집자를 골라 free 는 모든 중복을 안정 제거하고 priest genus 는 진단하며, 경로가 없으면 포획 가능(능력·용량·적 참여·취약·기한·비행/그래프)이면 포획(크리스털 비우기/내려놓기·적 이동·등록),
//    아니면 포획 사제 귀환 또는 수집/귀환 판단 뒤 포획 목록에서 제거하고, 경로가 있으면 포획 대기 보호·간헐천 목표 경로 유지·재분류 뒤 정지한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E181: AI 운반 행동 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';
import { RawContainedFinder } from './raw-contained-finder.js';

// 00501908 의 다이스 접근 좌표 보정과 다이스/간헐천/비행 genus 비트이다. (2026-11-09 WB04-E181)
const DAIS_OFFSET = 3.0;

// 비정렬 DWORD 와 WORD 를 읽는다(WORD 는 상위 BYTE 도 유지). (2026-11-09 WB04-E181)
function read(raw, at) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 운반 행동 DWORD 범위');
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}
const word = (raw, at) => raw[at] | (raw[at + 1] << 8);

// 원본 DWORD 의 네 BYTE 만 쓴다(주변 원문 보존). (2026-11-09 WB04-E181)
function write(raw, at, value) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 운반 행동 쓰기 범위');
    for (let index = 0; index < 4; index += 1) raw[at + index] = (value >>> (index * 8)) & 0xff;
}

// 원본 signed DWORD 비교/나머지의 비트 해석이다. (2026-11-09 WB04-E181)
const signed = (value) => value | 0;

// 단정도 float 비트를 수로 바꾼다. (2026-11-09 WB04-E181)
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 선택된 실제 하위 누락을 가짜 행동 성공으로 처리하지 않는다. (2026-11-09 WB04-E181)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 운반 행동 실제 하위 미연결');
    return hook;
}

// 후보 kind 는 BYTE 로 자르지 않고 0049a840 의 전체 인덱스로 TYPE genus 를 조회한다. (2026-11-09 WB04-E181)
function typeGenus(types, type) {
    if (type >= types.length) throw new RangeError('AI 운반 행동 현재TYPE 범위');
    return types[type].flags2 >>> 0;
}

// 004acb80: 원본은 mask 탈락 후보라도 먼저 kind TYPE 를 조회한다. (2026-11-09 WB04-E181)
export function findContainedGenus(pool, state, types, parent, mask, genus, index) {
    const finder = new RawContainedFinder(pool, state);
    let remaining = index >>> 0;
    // 현재 종속 체인을 원래 순서로 방문하고 일치 순번만 DWORD 로 감소시킨다.
    for (let sid = finder.begin(parent); sid; sid = finder.next()) {
        const raw = pool.slot(sid);
        const matches = ((typeGenus(types, word(raw, 18)) & genus) >>> 0) === (genus >>> 0);
        if ((!mask || (word(raw, 20) & mask)) && matches) {
            if (!remaining) return sid;
            remaining = (remaining - 1) >>> 0;
        }
    }
    return 0;
}

// 004487e0: 진단 callback 뒤 현재 genus/좌표/실패점도 다시 읽으며 free/dead 필터를 추가하지 않는다. (2026-11-09 WB04-E181)
export function daisApproachPoint(pool, types, checking, invalid, assertion, dais) {
    const valid = () => dais && dais < pool.capacity;
    const isDais = () => (typeGenus(types, pool.slot(dais)[10]) & 0x400000) !== 0;
    if (checking() && (!valid() || !isDais())) need(assertion)('dais.isValid() && dais.getGenus() & gDAIS');
    if (!valid() || !isDais()) return { x: invalid.x, y: invalid.y };
    const raw = pool.slot(dais);
    const x = bitsFloat(read(raw, 14));
    const y = bitsFloat(read(raw, 18));
    return { x: Math.fround(x - DAIS_OFFSET), y: Math.fround(y - DAIS_OFFSET) };
}

// AI 수집자 행동 처리기이다. 생성은 어떤 원본 행동/장치 효과도 실행하지 않는다. (2026-11-09 WB04-E181)
export class RawAiCarrierActions {
    #memory;
    #pool;
    #types;
    #globals;
    #hooks;

    // 같은 PATCH188 저장소와 실제 하위를 빌린다. (2026-11-09 WB04-E181)
    constructor(memory, pool, types, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 운반 행동 본체 소유자 누락');
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== 188) throw new TypeError('AI 운반 행동 같은PATCH188 누락');
        for (const name of ['now', 'boss', 'checking']) {
            if (typeof globals?.[name] !== 'function') throw new TypeError(`AI 운반 행동 현재 전역 조회기 누락: ${name}`);
        }
        this.#memory = memory;
        this.#pool = pool;
        this.#types = types;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 실제 월드 연결 확인용 같은 풀이다. (2026-11-09 WB04-E181)
    get pool() { return this.#pool; }

    // 가짜/해제 토큰은 즉시 거부한다. (2026-11-09 WB04-E181)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 운반 행동 실제0xb08 누락');
        return raw;
    }

    // 현재 본체의 값을 읽고(필드 쓰기는 지정된 DWORD 만 바꿈) 목록 count 를 signed 로 조회한다. (2026-11-09 WB04-E181)
    #field(token, at) { return read(this.#body(token), at); }
    #setField(token, at, value) { write(this.#body(token), at, value >>> 0); }
    #count(token, list) { return signed(this.#field(token, list + 8)); }

    // 현재 포인터를 사용하되 손상된 음수 주소 산술은 명시 거부한다. (2026-11-09 WB04-E181)
    #item(token, list, index) {
        if (index < 0) throw new RangeError('AI 운반 행동 음수 인덱스');
        return read(this.#memory.bytes(this.#field(token, list)), index * 4);
    }

    // 현재 active 목록만 검사하며 일치하지 않으면 원문을 전혀 쓰지 않는다. (2026-11-09 WB04-E181)
    #contains(token, list, value) {
        for (let index = 0; index < this.#count(token, list); index += 1) if (this.#item(token, list, index) === value) return true;
        return false;
    }

    // full 은 추가를 생략하며 꼬리를 정리하지 않는다. (2026-11-09 WB04-E181)
    #append(token, list, value) {
        const count = this.#count(token, list);
        if (count >= signed(this.#field(token, list + 4))) return;
        if (count < 0) throw new RangeError('AI 운반 행동 음수 추가 인덱스');
        write(this.#memory.bytes(this.#field(token, list)), count * 4, value);
        this.#setField(token, list + 8, count + 1);
    }

    // 모든 중복을 안정 제거하고 비활성 꼬리를 보존한다(원본 순서로 현재 목록을 압축). (2026-11-09 WB04-E181)
    #remove(token, list, value) {
        let removed = 0;
        for (let index = 0; index < this.#count(token, list); index += 1) {
            const current = this.#item(token, list, index);
            if (current === value) removed += 1;
            else write(this.#memory.bytes(this.#field(token, list)), (index - removed) * 4, current);
        }
        this.#setField(token, list + 8, this.#count(token, list) - removed);
    }

    // 원본 isValid 에는 free/dead/종류 조건이 없다(물리 1..용량-1 만). (2026-11-09 WB04-E181)
    #valid(value) { return value !== 0 && value < this.#pool.capacity; }

    // 잘못된 DWORD 를 다른 SID 로 잘라서 사용하지 않는다. (2026-11-09 WB04-E181)
    #object(value) {
        if (value >= this.#pool.capacity || value > 65535) throw new RangeError('AI 운반 행동 물리SID 범위');
        return value;
    }

    // 실제 현재 TYPE 의 genus 이다. (2026-11-09 WB04-E181)
    #genus(actor) { return typeGenus(this.#types, this.#pool.slot(actor)[10]); }

    // 원본 0 wrapper 만 sentinel 을 반환하며 그 외는 현재 실제 그래프이다. (2026-11-09 WB04-E181)
    #graph(actor) { return actor ? need(this.#hooks.graph)(actor) : need(this.#globals.invalidGraph)(); }

    // C0 가 1 인 less/unordered 는 모두 미도달이다(디컴파일의 단순 now<due 와 구별). (2026-11-09 WB04-E181)
    #ready(token, at) {
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, this.#field(token, at), true);
        view.setUint32(4, this.#field(token, at + 4), true);
        return this.#globals.now() >= view.getFloat64(0, true);
    }

    // 현재 다이스 유무는 실제 이동 직전에 읽는다. (2026-11-09 WB04-E181)
    #deliver(token, actor) {
        const dais = this.#field(token, 0x2a4);
        if (dais) {
            const point = need(this.#hooks.daisPoint)(this.#object(dais));
            need(this.#hooks.point)(token, actor, point);
        } else need(this.#hooks.home)(token, actor);
    }

    // 각 효과 뒤 원문의 현재 적 SID/기한/분류를 다시 조회한다. (2026-11-09 WB04-E181)
    #mayCapture(token, actor) {
        if (!(this.#field(token, 0x248) & 0x100) || this.#count(token, 0x384) >= signed(this.#field(token, 0x388)) || !this.#valid(this.#field(token, 0x2a0))
            || !need(this.#hooks.enemyAvailable)(this.#field(token, 0x264)) || !need(this.#hooks.vulnerable)(this.#object(this.#field(token, 0x2a0))) || !this.#ready(token, 0x5e0)) return false;
        if ((this.#genus(actor) & 0x20000) && !need(this.#hooks.flyingGetter)(token)) return true;
        const source = this.#graph(actor);
        return source === this.#graph(this.#object(this.#field(token, 0x2a0)));
    }

    // 첫 크리스털 조회 뒤 현재 boss 를 읽는다. 비서버 요청은 이동/등록 없이 반환한다. (2026-11-09 WB04-E181)
    #capture(token, actor) {
        if (need(this.#hooks.crystals)(actor)) {
            const crystal = need(this.#hooks.firstCrystal)(actor);
            if (!this.#globals.boss()) {
                need(this.#hooks.dropRequest)(actor);
                return;
            }
            need(this.#hooks.destroy)(crystal);
        }
        need(this.#hooks.target)(token, actor, this.#object(this.#field(token, 0x2a0)));
        need(this.#hooks.remember)(token, actor);
    }

    // 00412130: 전체 원본의 prefix/포획 귀환/순환 한 항목/정지 우선순위를 유지한다. (2026-11-09 WB04-E181)
    act(token) {
        const enemy = this.#field(token, 0x2a0);
        if (this.#valid(enemy)) need(this.#hooks.maxHitPoints)(this.#object(enemy));
        // 포획 사제를 든 정지 운반자를 가장 먼저 귀환시키며 생각 기한/일반 수집자보다 우선한다.
        for (let index = 0; index < this.#count(token, 0x384); index += 1) {
            const value = this.#item(token, 0x384, index);
            if (value && need(this.#hooks.prisoner)(this.#object(value)) && !need(this.#hooks.path)(this.#object(value))) {
                const actor = this.#object(value);
                this.#deliver(token, actor);
                this.#setField(token, 0x38c, 0);
                this.#append(token, 0x384, value);
                return;
            }
        }
        const count = this.#count(token, 0x2dc);
        if (!count || !this.#ready(token, 0x5a8)) return;
        const next = signed(this.#field(token, 0x6d0) + 1);
        if (next === -2147483648 && count === -1) throw new RangeError('AI 운반 행동 원본IDIV overflow');
        const cursor = next % count;
        this.#setField(token, 0x6d0, cursor);
        const actor = this.#object(this.#item(token, 0x2dc, cursor));
        if (this.#globals.checking() && (this.#pool.slot(actor)[11] & 1)) need(this.#hooks.assertion)('!s.isFree()', 0xc04);
        if (this.#pool.slot(actor)[11] & 1) {
            this.#remove(token, 0x2dc, actor);
            return;
        }
        if (this.#genus(actor) & 0x200000) need(this.#hooks.assertion)('!( s.getGenus() & gPRIEST )', 0xc0a);
        if (!need(this.#hooks.path)(actor)) {
            if (this.#mayCapture(token, actor)) {
                this.#capture(token, actor);
                return;
            }
            if (!actor) need(this.#hooks.assertion)('sid', 0xaeb);
            if (need(this.#hooks.prisoner)(actor)) {
                this.#deliver(token, actor);
                return;
            }
            need(this.#hooks.collect)(token, actor);
            if (this.#contains(token, 0x384, actor)) this.#remove(token, 0x384, actor);
            return;
        }
        if (this.#contains(token, 0x384, actor)) return;
        if (this.#field(token, 0x294) && this.#field(token, 0x298) !== need(this.#globals.invalidGraph)()) {
            const targets = need(this.#hooks.targets)(actor);
            if (!targets[0]) return;
            const target = this.#object(targets[0]);
            if (!(this.#pool.slot(target)[11] & 3) && (this.#genus(target) & 0x10000000) && this.#contains(token, 0x330, target)) {
                if (this.#genus(actor) & 0x20000) return;
                const destination = this.#graph(target);
                if (destination === this.#graph(actor)) return;
            }
            need(this.#hooks.reclassify)(token);
        }
        need(this.#hooks.stop)(actor);
    }
}
