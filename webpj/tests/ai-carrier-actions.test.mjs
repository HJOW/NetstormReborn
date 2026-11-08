// AI 수집자 행동 raw-ai-carrier-actions.js(RawAiCarrierActions·findContainedGenus·daisApproachPoint)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 물리 적 SID 면 free/dead 여도 최대 HP prefix 를 실행하고 빈 일반 목록은 그 뒤 종료, 포획 귀환은 빈 일반 목록/미도달 기한보다 우선하고 이동 효과 뒤 현재 큐를 한 actor 로 축소,
//    less/unordered 기한은 순환 인덱스도 바꾸지 않음, 순환은 다음 항목부터이며 free 는 모든 중복을 안정 제거하고 cursor 를 보정하지 않음, free 진단이 반환하며 상태를 고치면 계속하고 priest genus 진단은 checking 0 에서도 호출, 서버 포획은 첫 크리스털만 삭제하고 현재 적으로 이동한 뒤 등록,
//    현재 boss 는 첫 크리스털 조회 뒤 재판정하며 비서버 내려놓기는 이동/등록을 생략, 첫 비행 포획 운반자는 그래프를 요구하지 않고 이미 있으면 그래프로 판단, 참여 효과 뒤 현재 적을 HP 판정에 전달하고 포획 기한 NaN 이면 비행/그래프를 조회하지 않음, 포획 사제 귀환/수집 뒤 현재 큐 정리,
//    포획 대기 중 경로 유지와 targetA 0 은 정지하지 않음, 살아 있는 등록 간헐천의 비행/같은 그래프 경로 유지와 다른 그래프 재분류 뒤 정지, 재분류 뒤 정지 하위 누락은 앞선 효과 보존과 signed 음수 cursor, 내용물 genus 의 kind 전체 WORD/필터 순서, 다이스 접근점의 진단 효과 뒤 재조회.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E181: AI 운반 행동 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiCarrierActions, daisApproachPoint, findContainedGenus } from '../www/core/raw-ai-carrier-actions.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E181)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 배열 두 개의 원소가 모두 같은지 본다. (2026-11-09 WB04-E181)
const same = (left, right) => left.length === right.length && left.every((value, index) => value === right[index]);

// 원본 raw 입력과 선택된 하위 효과/호출 순서를 준비한다. (2026-11-09 WB04-E181)
class AiCarrierScene {
    // 기본은 정지한 일반 수집자 한 명이며 결과 판단은 입력에 합성하지 않는다.
    constructor() {
        this.pool = new SidPool(32768, { server: true });
        this.memory = new RawAiBrainMemory(0x4c000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.invalid = 254;
        this.checking = 0;
        this.now = 0;
        this.boss = true;
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        this.prisoners = new Map();
        this.paths = new Map();
        this.trace = [];
        this.crystals = 0;
        this.available = true;
        this.vulnerable = true;
        this.flying = false;
        this.targets = [0, 0];
        this.actor = this.newSlot(133);
        this.enemy = this.newSlot(158);
        this.dais = this.newSlot(165);
        this.geyser = this.newSlot(80);
        this.other = this.newSlot(133);
        this.crystal = this.newSlot(6);
        this.types[158].flags2 = 0x200000;
        this.types[165].flags2 = 0x400000;
        this.types[80].flags2 = 0x10000000;
        this.field(0x2a0, this.enemy);
        this.field(0x264, 2);
        this.field(0x294, this.dais);
        this.field(0x298, 7);
        this.list(0x2dc, [this.actor]);
        this.list(0x384, []);
        this.list(0x330, []);
        this.graphs = new Map([[this.actor, 7], [this.enemy, 7], [this.geyser, 7]]);
        this.hooks = {
            maxHitPoints: () => { this.trace.push('max'); return 100; },
            prisoner: (sid) => { this.trace.push(`prisoner:${this.role(sid)}`); return this.prisoners.get(sid) ?? 0; },
            path: (sid) => { this.trace.push(`path:${this.role(sid)}`); return this.paths.get(sid) ?? 0; },
            daisPoint: (sid) => { assert.equal(sid, this.dais); this.trace.push('dais'); return { x: 10, y: 12 }; },
            point: (brain, sid, p) => { assert.ok(brain === this.token && p.x === 10 && p.y === 12); this.trace.push(`point:${this.role(sid)}`); },
            home: (brain, sid) => { assert.equal(brain, this.token); this.trace.push(`home:${this.role(sid)}`); },
            enemyAvailable: (owner) => { assert.equal(owner, 2); this.trace.push('enemy'); return this.available; },
            vulnerable: () => { this.trace.push('vulnerable'); return this.vulnerable; },
            flyingGetter: (brain) => { assert.equal(brain, this.token); this.trace.push('flying'); return this.flying; },
            graph: (sid) => { this.trace.push(`graph:${this.role(sid)}`); return this.graphs.get(sid) ?? 0; },
            crystals: (sid) => { assert.equal(sid, this.actor); this.trace.push('crystals'); return this.crystals; },
            firstCrystal: (sid) => { assert.equal(sid, this.actor); this.trace.push('first'); return this.crystal; },
            dropRequest: (sid) => { this.trace.push(`drop:${this.role(sid)}`); },
            destroy: (sid) => { assert.equal(sid, this.crystal); this.trace.push('destroy'); },
            target: (brain, sid, target) => { assert.ok(brain === this.token && sid === this.actor); this.trace.push(`target:${this.role(target)}`); },
            remember: (brain, sid) => { assert.equal(brain, this.token); this.trace.push(`remember:${this.role(sid)}`); },
            collect: (brain, sid) => { assert.equal(brain, this.token); this.trace.push(`collect:${this.role(sid)}`); },
            targets: (sid) => { assert.equal(sid, this.actor); this.trace.push('targets'); return this.targets; },
            reclassify: (brain) => { assert.equal(brain, this.token); this.trace.push('reclassify'); },
            stop: (sid) => { this.trace.push(`stop:${this.role(sid)}`); },
            assertion: (text, line) => { this.trace.push(`assert:${line}`); }
        };
    }

    // 확보한 실제 payload 만 명시 0 으로 초기화한다. (2026-11-09 WB04-E181)
    newSlot(type) {
        const sid = this.pool.allocate(2);
        const raw = this.pool.allocated(sid);
        raw.fill(0);
        raw[10] = type;
        return sid;
    }

    // 호출 인자를 쉽게 구별하는 고정 입력 이름이다. (2026-11-09 WB04-E181)
    role(sid) {
        if (sid === this.actor) return 'A';
        if (sid === this.enemy) return 'E';
        if (sid === this.other) return 'O';
        if (sid === this.geyser) return 'G';
        return 'other';
    }

    // 현재 본체 DWORD 를 읽거나(값 없음) 쓴다. (2026-11-09 WB04-E181)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 원본 QWORD 기한의 두 DWORD 를 입력한다. (2026-11-09 WB04-E181)
    due(at, value) {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, value, true);
        this.field(at, view.getUint32(0, true));
        this.field(at + 4, view.getUint32(4, true));
    }

    // 실제 backing 에 입력 목록을 연결한다(반복문은 명시 입력 순서만 씀). (2026-11-09 WB04-E181)
    list(at, values, capacity = 8) {
        const pointer = this.memory.allocate(capacity * 4);
        values.forEach((sid, index) => put(this.memory.bytes(pointer), index * 4, sid));
        this.field(at, pointer);
        this.field(at + 4, capacity);
        this.field(at + 8, values.length);
        return pointer;
    }

    // 같은 현재 전역과 실제 저장소를 빌린다. (2026-11-09 WB04-E181)
    make() {
        return new RawAiCarrierActions(this.memory, this.pool, this.types, { now: () => this.now, boss: () => this.boss, checking: () => this.checking, invalidGraph: () => this.invalid }, this.hooks);
    }
}

test('AI 운반 행동: 물리 적 SID 면 free/dead 여도 최대 HP prefix 를 실행하며 빈 일반 목록은 그 뒤 종료한다', () => {
    const input = new AiCarrierScene();
    input.field(0x2e4, 0);
    input.pool.allocated(input.enemy)[11] = 3;
    input.make().act(input.token);
    assert.ok(same(input.trace, ['max']));
    input.trace.length = 0;
    input.field(0x2a0, 0);
    input.hooks = {};
    input.make().act(input.token);
    assert.equal(input.trace.length, 0);
});

test('AI 운반 행동: 포획 귀환은 빈 일반 목록/미도달 기한보다 우선하고 이동 효과 뒤 현재 큐를 한 actor 로 축소한다', () => {
    const input = new AiCarrierScene();
    const backing = input.list(0x384, [0, input.actor, input.other]);
    input.prisoners.set(input.actor, input.crystal);
    input.field(0x2e4, 0);
    input.due(0x5a8, Number.NaN);
    input.field(0x2a4, input.dais);
    input.hooks.point = (brain, actor, p) => { assert.ok(actor === input.actor && p.x === 10); input.trace.push('point:A'); input.field(0x38c, 7); };
    input.make().act(input.token);
    assert.ok(same(input.trace, ['max', 'prisoner:A', 'path:A', 'dais', 'point:A']) && input.field(0x38c) === 1 && get(input.memory.bytes(backing), 0) === input.actor && get(input.memory.bytes(backing), 8) === input.other);
    input.trace.length = 0;
    input.field(0x388, 0);
    input.field(0x2a4, 0);
    input.make().act(input.token);
    assert.ok(input.trace.at(-1) === 'home:A' && input.field(0x38c) === 0);
});

test('AI 운반 행동: less/unordered 기한은 순환 인덱스도 바꾸지 않고 prefix 까지만 진행한다', () => {
    const input = new AiCarrierScene();
    input.field(0x6d0, 77);
    input.due(0x5a8, 1);
    input.make().act(input.token);
    assert.ok(input.field(0x6d0) === 77 && same(input.trace, ['max']));
    input.trace.length = 0;
    input.due(0x5a8, Number.NaN);
    input.make().act(input.token);
    assert.ok(input.field(0x6d0) === 77 && input.trace.length === 1);
    input.trace.length = 0;
    input.due(0x5a8, 0);
    input.now = Number.NaN;
    input.make().act(input.token);
    assert.ok(input.field(0x6d0) === 77 && input.trace.length === 1);
    input.now = 0;
    input.make().act(input.token);
    assert.ok(input.field(0x6d0) === 0 && input.trace.at(-1) === 'collect:A');
});

test('AI 운반 행동: 순환은 다음 항목부터 시작하며 free 는 모든 중복을 안정 제거하고 cursor 를 보정하지 않는다', () => {
    const input = new AiCarrierScene();
    const backing = input.list(0x2dc, [input.actor, input.other, input.other]);
    input.pool.allocated(input.other)[11] = 1;
    input.checking = 1;
    input.make().act(input.token);
    assert.ok(input.field(0x6d0) === 1 && input.field(0x2e4) === 1 && get(input.memory.bytes(backing), 0) === input.actor && get(input.memory.bytes(backing), 8) === input.other && same(input.trace, ['max', 'assert:3076']));
});

test('AI 운반 행동: free 진단이 반환하며 상태를 고치면 계속하고 priest genus 진단은 checking 0 에서도 호출한다', () => {
    const input = new AiCarrierScene();
    const raw = input.pool.allocated(input.actor);
    input.checking = 1;
    raw[11] = 1;
    input.hooks.assertion = (text, line) => { assert.ok(text === '!s.isFree()' && line === 0xc04); raw[11] = 0; input.trace.push('fixed'); };
    input.make().act(input.token);
    assert.ok(input.field(0x2e4) === 1 && same(input.trace, ['max', 'fixed', 'path:A', 'prisoner:A', 'collect:A']));
    input.trace.length = 0;
    input.checking = 0;
    input.types[133].flags2 = 0x200000;
    input.hooks.assertion = (text, line) => { assert.ok(text === '!( s.getGenus() & gPRIEST )' && line === 0xc0a); input.trace.push('priestAssert'); };
    input.make().act(input.token);
    assert.equal(input.trace[1], 'priestAssert');
});

test('AI 운반 행동: 서버 포획은 첫 크리스털만 삭제하고 삭제 효과 뒤 현재 적으로 이동한 다음 등록한다', () => {
    const input = new AiCarrierScene();
    input.field(0x248, 0x100);
    input.crystals = 2;
    input.hooks.destroy = (crystal) => { assert.equal(crystal, input.crystal); input.trace.push('destroy'); input.field(0x2a0, input.other); };
    input.make().act(input.token);
    assert.ok(same(input.trace, ['max', 'path:A', 'enemy', 'vulnerable', 'graph:A', 'graph:E', 'crystals', 'first', 'destroy', 'target:O', 'remember:A']));
});

test('AI 운반 행동: 현재 boss 는 첫 크리스털 조회 뒤 재판정하며 비서버 내려놓기는 이동/등록을 생략한다', () => {
    const input = new AiCarrierScene();
    input.field(0x248, 0x100);
    input.crystals = 1;
    input.hooks.firstCrystal = () => { input.trace.push('first'); input.boss = false; return input.crystal; };
    input.make().act(input.token);
    assert.ok(input.trace.at(-1) === 'drop:A' && !input.trace.includes('destroy') && !input.trace.includes('remember:A'));
    input.trace.length = 0;
    input.hooks.firstCrystal = () => { input.trace.push('first'); input.boss = true; return input.crystal; };
    input.make().act(input.token);
    assert.ok(input.trace.at(-3) === 'destroy' && input.trace.at(-1) === 'remember:A');
});

test('AI 운반 행동: 첫 비행 포획 운반자는 그래프를 요구하지 않으며 비행 운반자가 이미 있으면 그래프로 판단한다', () => {
    const input = new AiCarrierScene();
    input.field(0x248, 0x100);
    input.types[133].flags2 = 0x20000;
    input.hooks.graph = undefined;
    input.make().act(input.token);
    assert.ok(same(input.trace, ['max', 'path:A', 'enemy', 'vulnerable', 'flying', 'crystals', 'target:E', 'remember:A']));
    input.trace.length = 0;
    input.flying = true;
    input.hooks.graph = (sid) => { input.trace.push(`graph:${input.role(sid)}`); return sid === input.actor ? 7 : 8; };
    input.make().act(input.token);
    assert.ok(input.trace.at(-2) === 'prisoner:A' && input.trace.at(-1) === 'collect:A');
});

test('AI 운반 행동: 참여 효과 뒤 현재 적을 HP 판정에 전달하고 포획 기한 NaN 이면 비행/그래프를 조회하지 않는다', () => {
    const input = new AiCarrierScene();
    input.field(0x248, 0x100);
    input.hooks.enemyAvailable = () => { input.trace.push('enemy'); input.field(0x2a0, input.other); return true; };
    input.hooks.vulnerable = (sid) => { assert.equal(sid, input.other); input.trace.push('vulnerable'); input.due(0x5e0, Number.NaN); return true; };
    input.hooks.graph = undefined;
    input.make().act(input.token);
    assert.ok(same(input.trace, ['max', 'path:A', 'enemy', 'vulnerable', 'prisoner:A', 'collect:A']));
});

test('AI 운반 행동: 일반 선택의 포획 사제 귀환은 큐를 축소하지 않으며 수집 뒤에는 현재 큐의 모든 중복을 제거한다', () => {
    const input = new AiCarrierScene();
    input.prisoners.set(input.actor, input.crystal);
    input.make().act(input.token);
    assert.ok(same(input.trace, ['max', 'path:A', 'prisoner:A', 'home:A']) && input.field(0x38c) === 0);
    input.trace.length = 0;
    input.prisoners.clear();
    input.hooks.collect = () => { input.trace.push('collect:A'); input.list(0x384, [input.other, input.actor, input.actor, input.geyser]); };
    input.make().act(input.token);
    const list = input.memory.bytes(input.field(0x384));
    assert.ok(input.field(0x38c) === 2 && get(list, 0) === input.other && get(list, 4) === input.geyser && get(list, 12) === input.geyser);
});

test('AI 운반 행동: 포획 대기 중 경로는 유지하고 신전/유효 그래프 없이는 목표 조회 없이 정지하며 targetA 0 은 정지하지 않는다', () => {
    const input = new AiCarrierScene();
    input.paths.set(input.actor, 100);
    input.list(0x384, [input.actor]);
    input.make().act(input.token);
    assert.ok(input.trace.at(-1) === 'path:A' && !input.trace.includes('targets'));
    input.trace.length = 0;
    input.field(0x38c, 0);
    input.field(0x294, 0);
    input.make().act(input.token);
    assert.equal(input.trace.at(-1), 'stop:A');
    input.trace.length = 0;
    input.field(0x294, input.dais);
    input.make().act(input.token);
    assert.equal(input.trace.at(-1), 'targets');
});

test('AI 운반 행동: 살아 있는 등록 간헐천의 비행/같은 그래프 경로는 유지하고 다른 그래프는 재분류한 뒤 정지한다', () => {
    const input = new AiCarrierScene();
    input.paths.set(input.actor, 100);
    input.targets = [input.geyser, 0xffffffff];
    input.list(0x330, [input.geyser]);
    input.types[133].flags2 = 0x20000;
    input.make().act(input.token);
    assert.equal(input.trace.at(-1), 'targets');
    input.trace.length = 0;
    input.types[133].flags2 = 0;
    input.make().act(input.token);
    assert.ok(input.trace.at(-2) === 'graph:G' && input.trace.at(-1) === 'graph:A');
    input.trace.length = 0;
    input.graphs.set(input.geyser, 8);
    input.make().act(input.token);
    assert.ok(input.trace.at(-2) === 'reclassify' && input.trace.at(-1) === 'stop:A');
    input.trace.length = 0;
    input.pool.allocated(input.geyser)[11] = 2;
    input.hooks.graph = undefined;
    input.make().act(input.token);
    assert.ok(same(input.trace, ['max', 'path:A', 'targets', 'reclassify', 'stop:A']));
});

test('AI 운반 행동: 재분류 뒤 정지 하위 누락은 앞선 효과를 보존하며 signed 음수 cursor 는 쓴 뒤 물리 접근을 거부한다', () => {
    const input = new AiCarrierScene();
    input.paths.set(input.actor, 100);
    input.targets = [input.other, 0];
    input.hooks.stop = undefined;
    assert.ok(throws(() => input.make().act(input.token)) && input.trace.at(-1) === 'reclassify');
    const cursor = new AiCarrierScene();
    cursor.list(0x2dc, [cursor.actor, cursor.other]);
    cursor.field(0x6d0, 0xfffffffe);
    assert.ok(throws(() => cursor.make().act(cursor.token)) && cursor.field(0x6d0) === 0xffffffff && same(cursor.trace, ['max']));
});

test('AI 운반 행동: 내용물 genus 는 kind 전체 WORD 의 TYPE 로 먼저 계산하며 부분 비트/상위 mask/순번을 원본대로 처리한다', () => {
    const input = new AiCarrierScene();
    const state = new ContainedFinderState();
    const parent = input.pool.allocated(input.actor);
    const first = input.pool.allocated(input.crystal);
    const second = input.pool.allocated(input.other);
    put(parent, 6, input.crystal, 2);
    put(first, 4, input.other, 2);
    first[10] = 6;
    second[10] = 6;
    first[11] = 3;
    put(first, 18, 158, 2);
    put(first, 20, 0x120, 2);
    put(second, 18, 158, 2);
    put(second, 20, 0x20, 2);
    input.types[158].flags2 = 0x200001;
    assert.ok(findContainedGenus(input.pool, state, input.types, input.actor, 0x100, 0x200000, 0) === input.crystal
        && findContainedGenus(input.pool, state, input.types, input.actor, 0x20, 0x200000, 1) === input.other
        && !findContainedGenus(input.pool, state, input.types, input.actor, 0, 0x200003, 0));
    put(first, 18, 0x19e, 2);
    put(first, 20, 0, 2);
    assert.ok(throws(() => findContainedGenus(input.pool, state, input.types, input.actor, 0x20, 0x200000, 0)));
});

test('AI 운반 행동: 다이스는 free/dead 도 좌표 −3 이며 진단 효과 뒤 현재 TYPE/좌표/공유 실패점을 재조회한다', () => {
    const input = new AiCarrierScene();
    const raw = input.pool.allocated(input.dais);
    put(raw, 14, floatToBits(10.0));
    put(raw, 18, floatToBits(12.0));
    raw[11] = 3;
    const invalid = { x: -10000, y: -10000 };
    let point = daisApproachPoint(input.pool, input.types, () => input.checking, invalid, undefined, input.dais);
    assert.ok(point.x === 7 && point.y === 9);
    input.checking = 1;
    input.types[165].flags2 = 0;
    let assertion = (text) => { assert.equal(text, 'dais.isValid() && dais.getGenus() & gDAIS'); input.types[165].flags2 = 0x400000; put(raw, 14, floatToBits(20.0)); };
    point = daisApproachPoint(input.pool, input.types, () => input.checking, invalid, assertion, input.dais);
    assert.ok(point.x === 17 && point.y === 9);
    assertion = () => { invalid.x = 88; invalid.y = 99; };
    point = daisApproachPoint(input.pool, input.types, () => input.checking, invalid, assertion, 0);
    assert.ok(point.x === 88 && point.y === 99);
});

test('AI 운반 행동 생성자: 본체 소유자/PATCH188/현재 전역 조회기가 없으면 거부하고 가짜 본체를 거부한다', () => {
    const input = new AiCarrierScene();
    const globals = { now: () => 0, boss: () => true, checking: () => 0, invalidGraph: () => 254 };
    assert.ok(throws(() => new RawAiCarrierActions(undefined, input.pool, input.types, globals, input.hooks)));
    assert.ok(throws(() => new RawAiCarrierActions(input.memory, {}, input.types, globals, input.hooks)));
    assert.ok(throws(() => new RawAiCarrierActions(input.memory, input.pool, input.types.slice(1), globals, input.hooks)));
    for (const name of ['now', 'boss', 'checking']) assert.ok(throws(() => new RawAiCarrierActions(input.memory, input.pool, input.types, { ...globals, [name]: undefined }, input.hooks)));
    assert.equal(input.make().pool, input.pool);
    assert.ok(throws(() => input.make().act(input.memory.allocate(16))));
});
