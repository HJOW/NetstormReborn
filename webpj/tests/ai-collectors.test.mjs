// AI 수집자 확보 raw-ai-collectors.js(RawAiCollectors)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 신전/그래프/후보 관문은 난수와 선택되지 않은 하위를 요구하지 않음, 일반 선택의 1000 → 후보 종류 → y → x 난수와 세 번 생산 조회 순서,
//    부족하지 않음/기한 전/보류 조각/자금 부족의 각 종료 지점, 난수로 고른 타입의 생산이 없으면 첫 후보로 대체, 취약 적용 비행 타입은 일반 충족/보류/자금 관문을 추가하지 않고 HP 를 최대 HP 하위 전에 저장, 실패 후보 최대 20회와 매 시도의 현재 경계 snapshot,
//    배치 하위 뒤 현재 rate/시각과 로그 뒤 현재 로컬 소유자 재조회, 정수 절반 HP·목록 물리 범위·unordered 기한과 선택된 효과 누락.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E179: AI 수집자 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiCollectors } from '../www/core/raw-ai-collectors.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E179)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 배열 두 개의 원소가 모두 같은지 본다. (2026-11-09 WB04-E179)
const same = (left, right) => left.length === right.length && left.every((value, index) => value === right[index]);

// 실제 본체/목록/사제와 원본 난수에 명시 하위 효과를 연결한다. (2026-11-09 WB04-E179)
class CollectorsInput {
    // 자기 영역과 지상/비행 두 타입의 실제 목록 backing 을 준비한다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x46000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.localOwner = 1;
        this.invalidGraph = 254;
        this.pool = new SidPool(32768, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        this.random = new GameRandom(100);
        this.now = 16777216.25;
        this.affordable = true;
        this.failures = 0;
        this.productQueries = [];
        this.affordQueries = [];
        this.candidates = [];
        this.deployed = [];
        this.logs = [];
        this.sounds = 0;
        this.enemyQueries = 0;
        this.hpQueries = 0;
        this.priest = this.newSlot();
        this.ground = this.newSlot();
        this.flying = this.newSlot();
        put(this.pool.allocated(this.priest), 26, 10);
        this.types[133].flags2 = 0x10000;
        this.types[134].flags2 = 0x20000;
        this.products = new Map([[133, this.ground], [134, this.flying]]);
        this.field(0x260, 2);
        this.field(0x264, 3);
        this.field(0x294, 100);
        this.field(0x298, 7);
        this.field(0x2a0, this.priest);
        this.field(0x388, 1);
        this.field(0x284, this.memory.allocate(8));
        this.field(0x28c, 1);
        put(this.memory.bytes(this.field(0x284)), 0, 133);
        put(this.memory.bytes(this.field(0x284)), 4, 134);
        this.field(0x2dc, this.memory.allocate(8));
        this.field(0x228, floatToBits(2.25));
        this.bounds(16, 32, 80, 96);
        this.hooks = {
            product: (brain, type) => { assert.equal(brain, this.token); this.productQueries.push(type); return this.products.get(type) ?? 0; },
            canAfford: (brain, type) => { assert.equal(brain, this.token); this.affordQueries.push(type); return this.affordable; },
            maxHitPoints: (sid) => { assert.equal(sid, this.priest); this.hpQueries += 1; return 100; },
            enemyAvailable: (owner) => { assert.equal(owner, 3); this.enemyQueries += 1; return true; },
            mayPlace: (query) => { this.candidates.push(query); return this.candidates.length > this.failures; },
            deploy: (brain, type, x, y, argument, direction) => { assert.equal(brain, this.token); this.deployed.push([type, x, y, argument, direction]); },
            log: (brain, type, x, y) => { assert.equal(brain, this.token); this.logs.push([type, x, y]); },
            dropSound: () => { this.sounds += 1; }
        };
    }

    // 실제 할당 슬롯을 명시 0 입력으로 준비한다. (2026-11-09 WB04-E179)
    newSlot() {
        const sid = this.pool.allocate(2);
        this.pool.allocated(sid).fill(0);
        return sid;
    }

    // 현재 본체 DWORD 를 읽거나(값 없음) 선택된 DWORD 만 쓴다. (2026-11-09 WB04-E179)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 현재 자기 사각형을 명시 입력으로 쓴다. (2026-11-09 WB04-E179)
    bounds(left, top, right, bottom) {
        this.field(0x270, floatToBits(left));
        this.field(0x274, floatToBits(top));
        this.field(0x278, floatToBits(right));
        this.field(0x27c, floatToBits(bottom));
    }

    // 원본 QWORD 기한을 명시 입력으로 쓴다. (2026-11-09 WB04-E179)
    due(at, time) {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, time, true);
        this.field(at, view.getUint32(0, true));
        this.field(at + 4, view.getUint32(4, true));
    }

    // 현재 실제 본체의 QWORD 시각을 읽는다. (2026-11-09 WB04-E179)
    time(at) {
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, this.field(at), true);
        view.setUint32(4, this.field(at + 4), true);
        return view.getFloat64(0, true);
    }

    // 같은 실제 저장소/현재 참조로 수집자 몸체를 구성한다. (2026-11-09 WB04-E179)
    make() {
        return new RawAiCollectors(this.memory, this.pool, this.types, this.random, { now: () => this.now, localOwner: () => this.localOwner, invalidGraph: () => this.invalidGraph }, this.hooks);
    }
}

test('AI 수집자: 신전/그래프/후보 관문은 난수와 선택되지 않은 하위를 요구하지 않는다', () => {
    const input = new CollectorsInput();
    input.hooks = {};
    const seed = input.random.state;
    input.field(0x294, 0);
    input.make().ensure(input.token);
    input.field(0x294, 100);
    input.field(0x298, 254);
    input.make().ensure(input.token);
    input.field(0x298, 7);
    input.field(0x28c, 0);
    input.make().ensure(input.token);
    assert.ok(input.random.state === seed && input.deployed.length === 0);
    const small = input.memory.allocate(4);
    assert.ok(throws(() => input.make().ensure(small)));
});

test('AI 수집자: 일반 선택은 1000 → 후보 종류 → y → x 난수와 세 번 생산 조회를 원문 순서로 소비한다', () => {
    const input = new CollectorsInput();
    const expected = new GameRandom(100);
    assert.equal(expected.next(1000), 100);
    expected.next(1);
    const y = 32 + expected.next(64);
    const x = 16 + expected.next(64);
    input.make().ensure(input.token);
    assert.ok(same(input.productQueries, [133, 133, 133]) && same(input.affordQueries, [133]) && input.candidates.length === 1 && input.deployed.length === 1 && input.random.state === expected.state);
    const query = input.candidates[0];
    assert.ok(query.type === 133 && query.argument === 0 && query.x === x && query.y === y && query.flags === 0 && query.owner === 2 && query.mode === 1);
    assert.ok(input.time(0x5a8) === input.now + 2.25 && input.time(0x5b0) === input.now + 3.25 && input.sounds === 0 && input.logs.length === 0);
});

test('AI 수집자: 부족하지 않음/기한 전/보류 조각/자금 부족은 각 원문 지점에서만 종료한다', () => {
    const satisfied = new CollectorsInput();
    satisfied.field(0x2cc, 1);
    const first = new GameRandom(100);
    first.next(1000);
    satisfied.make().ensure(satisfied.token);
    assert.ok(satisfied.random.state === first.state && satisfied.productQueries.length === 0);
    const late = new CollectorsInput();
    late.due(0x5a8, late.now + 1);
    late.make().ensure(late.token);
    const onlyChoice = new GameRandom(100);
    onlyChoice.next(1);
    assert.ok(late.random.state === onlyChoice.state && late.productQueries.length === 2 && late.deployed.length === 0);
    const pending = new CollectorsInput();
    pending.pool.allocated(pending.ground)[21] = 1;
    pending.make().ensure(pending.token);
    assert.ok(pending.productQueries.length === 3 && pending.affordQueries.length === 0 && pending.candidates.length === 0);
    const poor = new CollectorsInput();
    poor.affordable = false;
    poor.make().ensure(poor.token);
    assert.ok(poor.affordQueries.length === 1 && poor.candidates.length === 0);
});

test('AI 수집자: 난수로 고른 타입의 생산이 없으면 현재 첫 후보로 바꾸고 다시 두 번 조회한다', () => {
    const input = new CollectorsInput();
    input.field(0x28c, 2);
    input.products.delete(134);
    input.make().ensure(input.token);
    assert.ok(same(input.productQueries, [134, 133, 133]) && input.deployed.length === 1 && input.deployed[0][0] === 133);
});

test('AI 수집자: 취약 적용 비행 타입은 일반 충족/보류/자금 관문을 추가하지 않으며 HP 를 최대 HP 하위 전에 저장한다', () => {
    const input = new CollectorsInput();
    input.random.setState(1);
    input.field(0x248, 0x100);
    input.field(0x28c, 2);
    input.field(0x2cc, 1);
    input.pool.allocated(input.flying)[21] = 1;
    input.hooks.canAfford = undefined;
    input.hooks.maxHitPoints = (sid) => { assert.equal(sid, input.priest); put(input.pool.allocated(sid), 26, 1000); return 100; };
    input.make().ensure(input.token);
    assert.ok(same(input.productQueries, [133, 134]) && input.affordQueries.length === 0 && input.enemyQueries === 1 && input.deployed.length === 1 && input.deployed[0][0] === 134);
    const existing = new CollectorsInput();
    existing.random.setState(1);
    existing.field(0x248, 0x100);
    existing.field(0x2cc, 1);
    existing.field(0x2e4, 1);
    const flight = existing.newSlot();
    existing.pool.allocated(flight)[10] = 134;
    existing.pool.allocated(flight)[11] = 3;
    put(existing.memory.bytes(existing.field(0x2dc)), 0, flight);
    existing.make().ensure(existing.token);
    assert.ok(existing.make().hasFlying(existing.token) && existing.deployed.length === 0 && existing.productQueries.length === 0);
});

test('AI 수집자: 실패 후보는 최대 20회에서 끝나며 각 시도에서 현재 경계를 따로 저장한다', () => {
    const full = new CollectorsInput();
    full.failures = 20;
    full.make().ensure(full.token);
    assert.ok(full.candidates.length === 20 && full.deployed.length === 0 && full.time(0x5a8) === 0);
    const changed = new CollectorsInput();
    changed.field(0x260, 0x102);
    changed.hooks.mayPlace = (query) => {
        changed.candidates.push(query);
        if (changed.candidates.length === 1) { changed.bounds(128, 96, 160, 128); return false; }
        return true;
    };
    changed.make().placeInHome(changed.token, 133);
    assert.ok(changed.candidates.length === 2 && changed.candidates[0].owner === 2 && changed.candidates[1].owner === 2
        && changed.candidates[1].x >= 128 && changed.candidates[1].x < 160 && changed.candidates[1].y >= 96 && changed.candidates[1].y < 128 && changed.deployed.length === 1);
});

test('AI 수집자: 배치 하위 뒤 현재 rate/시각, 로그 뒤 현재 로컬 소유자를 다시 읽는다', () => {
    const input = new CollectorsInput();
    input.field(0xb00, 1);
    input.hooks.deploy = (brain, type, x, y, argument, direction) => {
        assert.equal(brain, input.token);
        input.deployed.push([type, x, y, argument, direction]);
        input.now += 3;
        input.field(0x228, floatToBits(4.25));
    };
    input.hooks.log = (brain, type, x, y) => { assert.equal(brain, input.token); input.logs.push([type, x, y]); input.localOwner = 2; };
    input.make().deployWithNotice(input.token, 133, -4.5, 7.75, 9, 11);
    assert.ok(input.logs.length === 1 && same(input.logs[0], [133, -4, 7]) && input.time(0x5a8) === input.now + 4.25 && input.time(0x5b0) === input.now + 5.25 && input.sounds === 1);
});

test('AI 수집자: 정수 절반 HP/목록 물리 범위/unordered 기한과 선택된 효과 누락을 확인한다', () => {
    const input = new CollectorsInput();
    const collectors = input.make();
    put(input.pool.allocated(input.priest), 26, 50);
    assert.ok(!collectors.vulnerable(input.priest));
    put(input.pool.allocated(input.priest), 26, 49);
    assert.ok(collectors.vulnerable(input.priest));
    input.field(0x2e4, 1);
    input.field(0x2dc, input.memory.allocate(1));
    assert.ok(throws(() => collectors.hasFlying(input.token)));
    const missing = new CollectorsInput();
    missing.hooks.mayPlace = undefined;
    assert.ok(throws(() => missing.make().ensure(missing.token)));
    assert.ok(missing.deployed.length === 0 && missing.time(0x5a8) === 0);
    const zero = new CollectorsInput();
    zero.bounds(16, 32, 80, 32);
    assert.ok(throws(() => zero.make().placeInHome(zero.token, 133)));
    const unordered = new CollectorsInput();
    unordered.due(0x5a8, Number.NaN);
    unordered.make().ensure(unordered.token);
    assert.ok(unordered.deployed.length === 0 && unordered.productQueries.length === 2);
});

test('AI 수집자 생성자: 본체 소유자/PATCH 풀/TYPE 표/난수/현재 전역 조회기가 없으면 거부한다', () => {
    const input = new CollectorsInput();
    const globals = { now: () => 0, localOwner: () => 1, invalidGraph: () => 254 };
    const make = (memory, pool, types, random, g) => () => new RawAiCollectors(memory, pool, types, random, g, input.hooks);
    assert.ok(throws(make(undefined, input.pool, input.types, input.random, globals)));
    assert.ok(throws(make(input.memory, {}, input.types, input.random, globals)));
    assert.ok(throws(make(input.memory, input.pool, [], input.random, globals)));
    assert.ok(throws(make(input.memory, input.pool, input.types, {}, globals)));
    for (const name of ['now', 'localOwner']) assert.ok(throws(make(input.memory, input.pool, input.types, input.random, { ...globals, [name]: undefined })));
    assert.equal(input.make().pool, input.pool);
});
