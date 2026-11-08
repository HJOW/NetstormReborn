// AI 시설 생산 raw-ai-factory-production.js(RawAiFactoryProduction)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 원소/flags4/현재 생산 보유를 내용물 순서대로 판정한 첫 일치 미등록 기술, battery 치환 필드 0/1/2/기타의 본래 원소·중립 3 치환,
//    생산 조회 효과로 링크가 바뀌어도 커서가 먼저 저장한 next 유지, 한 종류 추가 뒤 슬롯이 남으면 같은 첫 시설 유지, 가득 찬 시설의 모든 중복 압축과 현재 level/남은 기술로 업그레이드 대기 추가, 후보가 없으면 큐만 정리하고 행동 기한/업그레이드 미합성,
//    중복 생산 총수, 미처리 0/충족/지갑 1200/업그레이드 대기와 요청 → 제거 → 기한 순서, 선택된 미복원 업그레이드/누락 등록은 앞선 큐를 소비하지 않고 실패, 완료 시설 중복 허용과 TYPE 음수 비용 절삭, 큐가 가득 차도 계속하는 지갑과 현재 데모 지갑 1190 하한.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E179: AI 시설 생산 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiFactoryProduction } from '../www/core/raw-ai-factory-production.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E179)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 배열 두 개의 원소가 모두 같은지 본다. (2026-11-09 WB04-E179)
const same = (left, right) => left.length === right.length && left.every((value, index) => value === right[index]);

// 같은 본체/목록/기술·생산 내용물과 명시 하위 효과를 준비한다. (2026-11-09 WB04-E179)
class FactoryAiInput {
    // 각 큐의 실제 backing 과 원소가 같은 첫 기술을 준비한다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x47000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.invalidGraph = 254;
        this.pool = new SidPool(32768, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ theme: 0, flags2: 0, cost: 0 }));
        this.finder = new ContainedFinderState();
        this.now = 16777216.25;
        this.products = new Map();
        this.queries = [];
        this.trace = [];
        this.added = 0;
        this.upgrades = 0;
        this.levels = 0;
        this.logs = 0;
        this.parent = this.newSlot();
        this.factory = this.newSlot();
        this.other = this.newSlot();
        this.pool.allocated(this.factory)[10] = 85;
        this.pool.allocated(this.other)[10] = 85;
        this.types[85].theme = 3;
        this.types[100].theme = 3;
        this.types[101].theme = 3;
        this.types[102].theme = 1;
        this.field(0x260, 2);
        this.field(0x294, 100);
        this.field(0x298, 7);
        this.field(0x2cc, 1);
        this.field(0x2d8, 1200);
        this.field(0x228, floatToBits(2.5));
        this.field(0x30c, this.memory.allocate(16));
        this.field(0x310, 4);
        this.field(0x314, 1);
        put(this.memory.bytes(this.field(0x30c)), 0, this.factory);
        this.field(0x318, this.memory.allocate(16));
        this.field(0x31c, 4);
        this.firstTech = this.content(this.parent, 100, 4);
        this.hooks = {
            technology: (owner) => { assert.equal(owner, this.field(0x260)); this.trace.push('technology'); return this.parent; },
            product: (brain, type) => { assert.equal(brain, this.token); this.queries.push(type); return this.products.get(type) ?? 0; },
            addProduct: (brain, target, type) => {
                assert.ok(brain === this.token && target === this.factory);
                this.trace.push('add');
                this.added += 1;
                this.products.set(type, this.content(this.parent, type, 8));
                this.content(target, type, 16);
            },
            level: (target) => { assert.equal(target, this.factory); this.levels += 1; this.trace.push('level'); return this.pool.slot(target)[11] >> 5; },
            upgrade: (target) => { assert.equal(target, this.factory); this.upgrades += 1; this.trace.push('upgrade'); },
            log: (brain) => { assert.equal(brain, this.token); this.logs += 1; this.trace.push('log'); }
        };
    }

    // 실제 할당 슬롯을 명시 0 으로 준비한다. (2026-11-09 WB04-E179)
    newSlot() {
        const sid = this.pool.allocate(2);
        this.pool.allocated(sid).fill(0);
        return sid;
    }

    // 같은 풀의 부모 head 에 기술/생산 내용물 입력을 연결한다. (2026-11-09 WB04-E179)
    content(owner, type, flags) {
        const sid = this.newSlot();
        const raw = this.pool.allocated(sid);
        raw[10] = 6;
        put(raw, 18, type, 2);
        put(raw, 20, flags, 2);
        put(raw, 4, get(this.pool.slot(owner), 6, 2), 2);
        put(this.pool.allocated(owner), 6, sid, 2);
        return sid;
    }

    // 현재 본체 DWORD 를 읽거나(값 없음) 선택된 DWORD 만 쓴다. (2026-11-09 WB04-E179)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 현재 목록의 실제 칸을 읽는다. (2026-11-09 WB04-E179)
    item(pointer, index) { return get(this.memory.bytes(this.field(pointer)), index * 4); }

    // 현재 QWORD 생각 기한을 읽는다. (2026-11-09 WB04-E179)
    time(at) {
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, this.field(at), true);
        view.setUint32(4, this.field(at + 4), true);
        return view.getFloat64(0, true);
    }

    // 같은 저장소/현재 참조를 빌리는 시설 생산 몸체이다. (2026-11-09 WB04-E179)
    make() {
        return new RawAiFactoryProduction(this.memory, this.pool, this.types, this.finder, { now: () => this.now, invalidGraph: () => this.invalidGraph }, this.hooks);
    }
}

test('AI 시설 생산: 원소/flags4/현재 생산 보유를 원래 내용물 순서대로 검사한다', () => {
    const input = new FactoryAiInput();
    input.content(input.parent, 102, 4);
    input.content(input.parent, 101, 0);
    assert.ok(input.make().selectTechnology(input.token, 3) === 100 && same(input.queries, [100]));
    input.products.set(100, input.firstTech);
    assert.equal(input.make().selectTechnology(input.token, 3), 0);
    input.types[102].theme = 3;
    assert.equal(input.make().selectTechnology(input.token, 3), 102);
});

test('AI 시설 생산: battery mode 0/1/2/기타의 본래 원소·중립 3 치환을 교차 확인한다', () => {
    const input = new FactoryAiInput();
    input.types[100].theme = 1;
    input.types[100].flags2 = 0x800;
    // 원본 치환 필드의 네 값을 현재 본체에 각각 공급한다.
    for (let mode = 0; mode < 4; mode += 1) {
        input.field(0x3b8, mode);
        // 본래 원소/중립/다른 원소에서 첫 기술 선택을 대조한다.
        for (const theme of [1, 3, 2]) {
            const matches = mode === 0 ? theme === 1 : mode === 1 ? theme === 3 : mode === 2 ? theme === 1 || theme === 3 : false;
            assert.equal(input.make().selectTechnology(input.token, theme), matches ? 100 : 0);
        }
    }
});

test('AI 시설 생산: 현재 후보의 링크가 생산 조회에서 바뀌어도 커서가 먼저 저장한 next 는 유지한다', () => {
    const input = new FactoryAiInput();
    const newer = input.content(input.parent, 101, 4);
    input.hooks.product = (brain, type) => {
        assert.equal(brain, input.token);
        input.queries.push(type);
        if (type === 101) { put(input.pool.allocated(newer), 4, 0, 2); return newer; }
        return 0;
    };
    assert.ok(input.make().selectTechnology(input.token, 3) === 100 && same(input.queries, [101, 100]));
});

test('AI 시설 생산: 한 종류 추가 뒤 슬롯이 남으면 같은 첫 시설을 미처리 큐에 유지한다', () => {
    const input = new FactoryAiInput();
    input.field(0xb00, 1);
    input.make().manage(input.token);
    assert.ok(input.added === 1 && input.levels === 1 && input.field(0x314) === 1 && input.field(0x320) === 0 && input.logs === 1 && input.make().countProducts(input.factory) === 1 && input.time(0x5a8) === input.now + 2.5 && input.time(0x5b0) === input.now + 3.5);
});

test('AI 시설 생산: 가득 찬 시설의 모든 중복을 압축하고 현재 level/남은 기술로 업그레이드 대기만 추가한다', () => {
    const input = new FactoryAiInput();
    input.content(input.parent, 101, 4);
    input.content(input.factory, 99, 16);
    put(input.memory.bytes(input.field(0x30c)), 4, input.factory);
    put(input.memory.bytes(input.field(0x30c)), 8, input.other);
    put(input.memory.bytes(input.field(0x30c)), 12, 0xdeadbeef);
    input.field(0x314, 3);
    input.make().manage(input.token);
    assert.ok(input.added === 1 && input.levels === 2 && input.field(0x314) === 1 && input.item(0x30c, 0) === input.other && input.item(0x30c, 3) === 0xdeadbeef && input.field(0x320) === 1 && input.item(0x318, 0) === input.factory);
    const maxed = new FactoryAiInput();
    maxed.pool.allocated(maxed.factory)[11] = 64;
    maxed.content(maxed.parent, 101, 4);
    maxed.content(maxed.factory, 90, 16);
    maxed.content(maxed.factory, 91, 16);
    maxed.content(maxed.factory, 92, 16);
    maxed.make().manage(maxed.token);
    assert.ok(maxed.added === 1 && maxed.field(0x314) === 0 && maxed.field(0x320) === 0);
});

test('AI 시설 생산: 후보가 없으면 큐를 정리하되 행동 기한/업그레이드를 합성하지 않으며 중복 생산도 각각 센다', () => {
    const input = new FactoryAiInput();
    input.products.set(100, input.firstTech);
    input.make().manage(input.token);
    assert.ok(input.field(0x314) === 0 && !input.added && !input.upgrades && input.time(0x5a8) === 0);
    input.content(input.factory, 100, 16);
    const duplicate = input.content(input.factory, 100, 0x110);
    input.pool.allocated(duplicate)[11] = 3;
    input.content(input.factory, 100, 8);
    assert.equal(input.make().countProducts(input.factory), 2);
});

test('AI 시설 생산: 미처리 0/충족/지갑 1200/업그레이드 대기와 원본 요청 → 제거 → 기한의 순서다', () => {
    const input = new FactoryAiInput();
    input.field(0x314, 0);
    input.field(0x320, 2);
    put(input.memory.bytes(input.field(0x318)), 0, input.factory);
    put(input.memory.bytes(input.field(0x318)), 4, input.factory);
    input.field(0x2d8, 1199);
    input.make().manage(input.token);
    assert.ok(!input.upgrades && input.field(0x320) === 2);
    input.field(0x2d8, 1200);
    input.hooks.upgrade = (sid) => {
        assert.ok(sid === input.factory && input.field(0x320) === 2 && input.time(0x5a8) === 0);
        input.upgrades += 1;
    };
    input.make().manage(input.token);
    assert.ok(input.upgrades === 1 && input.field(0x320) === 0 && input.time(0x5a8) === input.now + 2.5);
});

test('AI 시설 생산: 실제 선택된 미복원 업그레이드/누락 등록은 앞선 큐를 소비하지 않은 채 실패한다', () => {
    const missing = new FactoryAiInput();
    missing.hooks.addProduct = undefined;
    assert.ok(throws(() => missing.make().manage(missing.token)));
    assert.ok(missing.field(0x314) === 1 && missing.time(0x5a8) === 0);
    const upgrade = new FactoryAiInput();
    upgrade.field(0x314, 0);
    upgrade.field(0x320, 1);
    put(upgrade.memory.bytes(upgrade.field(0x318)), 0, upgrade.factory);
    upgrade.hooks.upgrade = undefined;
    assert.ok(throws(() => upgrade.make().manage(upgrade.token)));
    assert.ok(upgrade.field(0x320) === 1 && upgrade.time(0x5a8) === 0);
    const inactive = new FactoryAiInput();
    inactive.field(0x294, 0);
    inactive.hooks = {};
    inactive.make().manage(inactive.token);
    inactive.field(0x294, 100);
    inactive.field(0x298, 254);
    inactive.make().manage(inactive.token);
    assert.equal(inactive.trace.length, 0);
    const small = inactive.memory.allocate(4);
    assert.ok(throws(() => inactive.make().selectTechnology(small, 3)));
});

test('AI 시설 생산: 완료 시설 중복을 허용하며 TYPE 의 음수 비용을 반올림 없이 절삭한다', () => {
    const input = new FactoryAiInput();
    input.types[85].cost = 500.75;
    input.hooks.demo = () => false;
    input.make().upgraded(input.token, input.factory);
    assert.ok(input.field(0x314) === 2 && input.item(0x30c, 1) === input.factory && input.field(0x2d8) === 700);
    input.make().upgraded(input.token, input.factory);
    assert.ok(input.field(0x314) === 3 && input.item(0x30c, 2) === input.factory && input.field(0x2d8) === 200);
});

test('AI 시설 생산: 목록이 가득 차도 지갑 처리는 계속하며 데모 판정 뒤 현재 지갑에 1190 하한을 적용한다', () => {
    const input = new FactoryAiInput();
    input.field(0x314, 4);
    input.types[85].cost = 1500.75;
    input.hooks.demo = () => {
        assert.ok(input.field(0x314) === 4 && (input.field(0x2d8) | 0) === -300);
        input.field(0x2d8, 1400);
        return true;
    };
    input.make().upgraded(input.token, input.factory);
    assert.ok(input.field(0x314) === 4 && input.field(0x2d8) === 1400);
    input.hooks.demo = () => true;
    input.make().upgraded(input.token, input.factory);
    assert.equal(input.field(0x2d8), 1190);
});

test('AI 시설 생산 생성자: 본체 소유자/PATCH 풀/TYPE 표/내용물 권한 상태/현재 시각 조회기가 없으면 거부한다', () => {
    const input = new FactoryAiInput();
    const globals = { now: () => 0, invalidGraph: () => 254 };
    const make = (memory, pool, types, finder, g) => () => new RawAiFactoryProduction(memory, pool, types, finder, g, input.hooks);
    assert.ok(throws(make(undefined, input.pool, input.types, input.finder, globals)));
    assert.ok(throws(make(input.memory, {}, input.types, input.finder, globals)));
    assert.ok(throws(make(input.memory, input.pool, [], input.finder, globals)));
    assert.ok(throws(make(input.memory, input.pool, input.types, {}, globals)));
    assert.ok(throws(make(input.memory, input.pool, input.types, input.finder, { invalidGraph: () => 254 })));
    assert.equal(input.make().pool, input.pool);
});
