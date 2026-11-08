// AI 자원 선택 raw-ai-resource-choice.js(RawAiResourceChoice)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 신전이 없으면 NaN/부호 0 을 포함한 fallback 원문 유지와 center 미요구, 소수 거리의 정수 절삭 동률은 먼저 방문한 항목이며 65536 경계는 선택하지 않음,
//    모든 후보의 그래프를 읽고 제외/필수 그래프는 현재 무효 sentinel 과 비교, 그래프 효과 뒤 현재 포인터/count 재조회, 원본 무효 ftol 하위 DWORD 0 과 물리 목록 거부, 수집 관문(내용물도 읽지 않음·크리스털/능력 탈락은 귀환만),
//    현재 신전 중심 → actor 그래프 → 간헐천 목록 → 실제 이동 순서와 목표 없음 귀환, 내용물 효과 뒤 능력과 현재 sentinel 재조회, 포획 대기(비행 판정·중복 로그 생략·full 로그 유지·기한 불변), 끊긴 중복의 안정 제거와 대상 full,
//    SID 0 진단 후 무효 그래프 사용과 로그 누락 시 원문 유지, 공용 내용물 총수의 전체 WORD mask/DWORD kind 비교.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E175: AI 자원 선택 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ContainedFinderState, countContained } from '../www/core/raw-contained-finder.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiResourceChoice } from '../www/core/raw-ai-resource-choice.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E175)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const F = Math.fround;

// 같은 실제 풀/본체/등록 목록과 명시 하위 효과를 준비한다. (2026-11-09 WB04-E175)
class AiResourceScene {
    // 판단 결과를 계산하지 않고 원문과 선택된 하위만 공급한다.
    constructor() {
        this.pool = new SidPool(32768, { server: true });
        this.memory = new RawAiBrainMemory(0x4b000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.invalid = 254;
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        this.graphs = new Map();
        this.trace = [];
        this.crystals = 0;
        this.moved = 0;
        this.center = { x: 20, y: 30 };
        this.actor = this.newSlot(133);
        this.temple = this.newSlot(75);
        this.first = this.newSlot(80);
        this.second = this.newSlot(80);
        this.third = this.newSlot(80);
        this.types[80].flags2 = 0x10000000;
        this.field(0x294, this.temple);
        this.field(0x298, 7);
        this.field(0x248, 0x10);
        this.field(0xb00, 1);
        this.hooks = {
            center: (sid) => { assert.equal(sid, this.temple); this.trace.push('center'); return this.center; },
            graph: (sid) => { this.trace.push(`graph:${sid}`); return this.graphs.get(sid) ?? 0; },
            crystals: (sid) => { assert.equal(sid, this.actor); this.trace.push('crystals'); return this.crystals; },
            target: (brain, sid, target) => { assert.ok(brain === this.token && sid === this.actor); this.moved = target; this.trace.push('target'); },
            home: (brain, sid) => { assert.ok(brain === this.token && sid === this.actor); this.trace.push('home'); },
            captureLog: (brain, sid) => { assert.equal(brain, this.token); this.trace.push(`capture:${sid}`); },
            disconnectedLog: (brain, sid) => { assert.equal(brain, this.token); this.trace.push(`disconnected:${sid}`); },
            assertion: (text) => { assert.equal(text, 's.isValid() && (s.getGenus()&gGEYSER)'); this.trace.push('assert'); }
        };
    }

    // 실제 확보한 payload 만 명시 0 으로 준비한다. (2026-11-09 WB04-E175)
    newSlot(type) {
        const sid = this.pool.allocate(2);
        const raw = this.pool.allocated(sid);
        raw.fill(0);
        raw[10] = type;
        return sid;
    }

    // 지정 현재 본체 DWORD 를 쓰거나(값 있음) 읽는다. (2026-11-09 WB04-E175)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 현재 실제 슬롯 위치를 입력한다. (2026-11-09 WB04-E175)
    position(sid, x, y) {
        put(this.pool.allocated(sid), 14, floatToBits(x));
        put(this.pool.allocated(sid), 18, floatToBits(y));
    }

    // 새 실제 목록에 입력을 쓰고 count/cap/포인터를 같은 본체에 연결한다. (2026-11-09 WB04-E175)
    list(at, values, capacity = 8) {
        const backing = this.memory.allocate(capacity * 4);
        // 명시 입력 순서대로 DWORD 를 쓴다.
        values.forEach((sid, index) => put(this.memory.bytes(backing), index * 4, sid));
        this.field(at, backing);
        this.field(at + 4, capacity);
        this.field(at + 8, values.length);
        return backing;
    }

    // 같은 실제 저장소/현재 전역을 빌린다. (2026-11-09 WB04-E175)
    make() { return new RawAiResourceChoice(this.memory, this.pool, this.types, () => this.invalid, this.hooks); }
}

test('AI 자원 선택: 신전이 없으면 NaN/부호 0 을 포함한 fallback 원문을 유지하고 center 를 요구하지 않는다', () => {
    const input = new AiResourceScene();
    input.field(0x294, 0);
    input.field(0x268, 0x7fc01234);
    input.field(0x26c, 0x80000000);
    input.hooks.center = undefined;
    const point = input.make().homeCenter(input.token);
    assert.ok(point.xBits === 0x7fc01234 && point.yBits === 0x80000000 && Number.isNaN(point.x) && Object.is(point.y, -0));
    input.field(0x294, input.temple);
    input.hooks.center = (sid) => { assert.equal(sid, input.temple); input.field(0x294, input.first); return { x: 7, y: 8 }; };
    const current = input.make().homeCenter(input.token);
    assert.ok(current.x === 7 && current.y === 8 && input.field(0x294) === input.first);
});

test('AI 자원 선택: 소수 거리의 정수 절삭 동률은 먼저 방문한 항목이며 65536 경계는 선택하지 않는다', () => {
    const input = new AiResourceScene();
    input.list(0x330, [input.first, input.second, input.third]);
    input.position(input.first, F(2.2), 0);
    input.position(input.second, 2, 0);
    input.position(input.third, 256, 0);
    assert.equal(input.make().select(input.token, 0x330, { x: 0, y: 0 }, 254, 254), input.first);
    assert.deepEqual(input.trace, ['graph:7', 'graph:8', 'graph:9']);
    input.list(0x330, [input.third]);
    assert.ok(!input.make().select(input.token, 0x330, { x: 0, y: 0 }, 254, 254));
    input.field(0x338, 0xffffffff);
    input.hooks = {};
    assert.ok(!input.make().select(input.token, 0x330, { x: 0, y: 0 }, 254, 254));
});

test('AI 자원 선택: 모든 후보의 그래프를 읽고 제외/필수 그래프는 현재 무효 sentinel 과 비교한다', () => {
    const input = new AiResourceScene();
    input.list(0x330, [input.first, input.second, input.third]);
    input.position(input.first, 1, 0);
    input.position(input.second, 3, 0);
    input.position(input.third, 2, 0);
    input.graphs = new Map([[input.first, 9], [input.second, 7], [input.third, 8]]);
    assert.ok(input.make().select(input.token, 0x330, { x: 0, y: 0 }, 9, 7) === input.second && input.trace.length === 3);
    input.invalid = 7;
    assert.equal(input.make().select(input.token, 0x330, { x: 0, y: 0 }, 9, 7), input.third);
});

test('AI 자원 선택: 그래프 효과가 현재 포인터/count/좌표를 바꿔도 이번 거리·후보는 이미 저장되어 있다', () => {
    const input = new AiResourceScene();
    input.list(0x330, [input.first, input.third]);
    input.position(input.first, 1, 0);
    input.position(input.second, 2, 0);
    input.hooks.graph = (sid) => {
        input.trace.push(`graph:${sid}`);
        if (sid === input.first) { input.position(input.first, 100, 0); input.list(0x330, [input.third, input.second]); }
        return 7;
    };
    assert.ok(input.make().select(input.token, 0x330, { x: 0, y: 0 }, 254, 7) === input.first && JSON.stringify(input.trace) === '["graph:7","graph:8"]');
    input.trace.length = 0;
    input.hooks.graph = (sid) => { assert.equal(sid, input.third); input.field(0x338, 0); return 7; };
    assert.ok(input.make().select(input.token, 0x330, { x: 0, y: 0 }, 254, 7) === input.third && input.field(0x338) === 0);
});

test('AI 자원 선택: 원본 무효 ftol 하위 DWORD 0 을 보존하며 잘못된 물리 목록은 성공으로 바꾸지 않는다', () => {
    const input = new AiResourceScene();
    input.list(0x330, [input.first, input.second]);
    input.position(input.first, Number.NaN, 0);
    assert.equal(input.make().select(input.token, 0x330, { x: 0, y: 0 }, 254, 254), input.first);
    input.field(0x330, 0xdeadbeef);
    assert.ok(throws(() => input.make().select(input.token, 0x330, { x: 0, y: 0 }, 254, 254)));
});

test('AI 자원 선택: 관문 탈락은 내용물도 읽지 않고 크리스털/수집능력 탈락은 실제 귀환만 호출한다', () => {
    const input = new AiResourceScene();
    input.field(0x294, 0);
    input.hooks = {};
    input.make().collectOrHome(input.token, input.actor);
    input.field(0x294, input.temple);
    input.field(0x298, 254);
    input.make().collectOrHome(input.token, input.actor);
    assert.equal(input.trace.length, 0);
    const carried = new AiResourceScene();
    carried.crystals = 1;
    carried.make().collectOrHome(carried.token, carried.actor);
    assert.deepEqual(carried.trace, ['crystals', 'home']);
    carried.trace.length = 0;
    carried.crystals = 0;
    carried.field(0x248, 0);
    carried.make().collectOrHome(carried.token, carried.actor);
    assert.deepEqual(carried.trace, ['crystals', 'home']);
});

test('AI 자원 선택: 현재 신전 중심 → actor 그래프 → 간헐천 목록 → 실제 이동 순서이며 목표가 없으면 귀환한다', () => {
    const input = new AiResourceScene();
    input.list(0x330, [input.first, input.second]);
    input.position(input.first, 21, 30);
    input.position(input.second, 20, 32);
    input.graphs = new Map([[input.actor, 7], [input.first, 8], [input.second, 7]]);
    input.make().collectOrHome(input.token, input.actor);
    assert.ok(input.moved === input.second && JSON.stringify(input.trace) === '["crystals","center","graph:5","graph:7","graph:8","target"]');
    input.trace.length = 0;
    input.field(0x338, 0);
    input.make().collectOrHome(input.token, input.actor);
    assert.deepEqual(input.trace, ['crystals', 'center', 'graph:5', 'home']);
});

test('AI 자원 선택: 내용물 효과 뒤 능력을 다시 읽으며 actor 그래프 효과 뒤 현재 sentinel 을 인자로 쓴다', () => {
    const input = new AiResourceScene();
    input.hooks.crystals = () => { input.field(0x248, 0); return 0; };
    input.make().collectOrHome(input.token, input.actor);
    assert.deepEqual(input.trace, ['home']);
    input.trace.length = 0;
    input.field(0x248, 0x10);
    input.hooks.crystals = () => 0;
    input.list(0x330, [input.first]);
    input.hooks.graph = (sid) => { if (sid === input.actor) { input.invalid = 7; return 7; } return 8; };
    input.make().collectOrHome(input.token, input.actor);
    assert.ok(input.moved === input.first && JSON.stringify(input.trace) === '["center","target"]');
});

test('AI 자원 선택: 비행 판정은 현재 TYPE 만 보며 중복은 로그도 생략하고 full 은 로그를 유지한다(기한은 불변)', () => {
    const input = new AiResourceScene();
    const backing = input.list(0x384, [input.first], 2);
    input.types[80].flags2 |= 0x20000;
    input.pool.allocated(input.first)[11] = 3;
    assert.ok(input.make().hasFlyingCaptureCarrier(input.token));
    const before = Buffer.from(input.memory.bytes(input.token));
    input.make().rememberCaptureCarrier(input.token, input.first);
    assert.equal(input.trace.length, 0);
    input.make().rememberCaptureCarrier(input.token, input.actor);
    assert.ok(input.field(0x38c) === 2 && get(input.memory.bytes(backing), 4) === input.actor);
    input.make().rememberCaptureCarrier(input.token, input.second);
    assert.ok(input.field(0x38c) === 2 && JSON.stringify(input.trace) === '["capture:5","capture:8"]');
    assert.ok(Buffer.from(input.memory.bytes(input.token)).subarray(0x5e0, 0x5e8).equals(before.subarray(0x5e0, 0x5e8)));
    input.field(0x38c, 0);
    input.field(0x388, 0);
    input.field(0x384, 0xdeadbeef);
    input.field(0xb00, 0);
    input.hooks = {};
    assert.ok(!input.make().hasFlyingCaptureCarrier(input.token));
    input.make().rememberCaptureCarrier(input.token, input.actor);
});

test('AI 자원 선택: 끊긴 중복을 모두 안정 제거하며 대상 목록 full 은 제거를 되돌리지 않는다', () => {
    const input = new AiResourceScene();
    const source = input.list(0x330, [input.first, input.first, input.second, input.third]);
    const destination = input.list(0x348, [input.third], 2);
    input.graphs = new Map([[input.first, 8], [input.second, 7], [input.third, 9]]);
    input.make().reclassifyDisconnected(input.token);
    assert.ok(input.field(0x338) === 1 && get(input.memory.bytes(source), 0) === input.second && input.field(0x350) === 2 && get(input.memory.bytes(destination), 0) === input.third && get(input.memory.bytes(destination), 4) === input.first);
    assert.deepEqual(input.trace, ['graph:7', 'disconnected:7', 'graph:8', 'graph:9', 'disconnected:9']);
    assert.equal(get(input.memory.bytes(source), 12), input.third);
});

test('AI 자원 선택: SID 0 진단이 반환하면 무효 그래프를 사용하고 로그 누락이면 이동 전 원문을 유지한다', () => {
    const zero = new AiResourceScene();
    zero.list(0x330, [0]);
    zero.field(0x298, 254);
    zero.hooks.graph = undefined;
    zero.make().reclassifyDisconnected(zero.token);
    assert.ok(JSON.stringify(zero.trace) === '["assert"]' && zero.field(0x338) === 1);
    const missing = new AiResourceScene();
    const backing = missing.list(0x330, [missing.first]);
    missing.graphs.set(missing.first, 8);
    missing.hooks.disconnectedLog = undefined;
    assert.ok(throws(() => missing.make().reclassifyDisconnected(missing.token)) && missing.field(0x338) === 1 && get(missing.memory.bytes(backing), 0) === missing.first);
});

test('AI 자원 선택: 공용 내용물 총수는 전체 WORD mask 와 DWORD kind 를 비교하고 free/dead 후보를 임의 제외하지 않는다', () => {
    const input = new AiResourceScene();
    const state = new ContainedFinderState();
    const actor = input.pool.allocated(input.actor);
    const first = input.pool.allocated(input.first);
    const second = input.pool.allocated(input.second);
    const third = input.pool.allocated(input.third);
    put(actor, 6, input.first, 2);
    put(first, 4, input.second, 2);
    put(second, 4, input.third, 2);
    first[10] = 6;
    second[10] = 6;
    third[10] = 6;
    first[11] = 3;
    put(first, 18, 154, 2);
    put(first, 20, 0x120, 2);
    put(second, 18, 154, 2);
    put(second, 20, 0x20, 2);
    put(third, 18, 155, 2);
    put(third, 20, 0x100, 2);
    assert.ok(countContained(input.pool, state, input.actor, 0x20, 154) === 2 && countContained(input.pool, state, input.actor, 0x100, 0) === 2 && countContained(input.pool, state, input.actor, 0, 0) === 3 && countContained(input.pool, state, input.actor, 0, 0x10000) === 0);
});

test('AI 자원 선택 생성자: 본체 소유자/PATCH 풀/188 TYPE/무효 그래프 조회기가 없으면 거부하고 가짜 본체/손상 SID 를 거부한다', () => {
    const input = new AiResourceScene();
    assert.ok(throws(() => new RawAiResourceChoice(undefined, input.pool, input.types, () => 254, input.hooks)));
    assert.ok(throws(() => new RawAiResourceChoice(input.memory, {}, input.types, () => 254, input.hooks)));
    assert.ok(throws(() => new RawAiResourceChoice(input.memory, input.pool, input.types.slice(0, 10), () => 254, input.hooks)));
    assert.ok(throws(() => new RawAiResourceChoice(input.memory, input.pool, input.types, undefined, input.hooks)));
    assert.equal(input.make().pool, input.pool);
    assert.ok(throws(() => input.make().homeCenter(input.memory.allocate(16))));
    input.field(0x294, 0x10000);
    assert.ok(throws(() => input.make().homeCenter(input.token)));
});
