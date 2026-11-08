// AI 전략 준비 raw-ai-strategy-setup.js(RawAiStrategySetup)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 적/양쪽 지역이 없을 때 하위나 +0x258/+0x25c 를 건드리지 않고 재시도, 자기 경계 → 다리 → 현재 신전 → 첫 생산 → +0x258 의 실제 순서와 QWORD 기한,
//    원격 AI 의 기술 부모 mask 8 과 신전 mask 10 생산 및 기존 생산이 있으면 첫 생산 생략, 보류 중인 다리는 총수에만 포함, 직선 강제도 매번 난수를 소비하고 serial signed 감김에서도 4배수 규칙 유지, 중복 진단을 계속 진행시킨 뒤 현재 원격 내용물 삭제와 새 생산 등록,
//    signed 비용+1200 의 엄격한 경계와 목표 우선순위, 같은 목표에서는 전체 본체 보존·변경 때만 여섯 필드 초기화와 엄격한 좌표 경계, 신전 없음/충족/기한 전/데모 조건에서도 기존 재고를 먼저 스캔하며 한 번에 하나만 보충, 원격은 기한 전 데모 getter 미호출과 unordered 시각 대기.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E182: AI 전략 준비 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { PlayerWorldState, RawPlayerWorld } from '../www/core/raw-player-world.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawCarrierPreDestroy } from '../www/core/raw-carrier-pre-destroy.js';
import { RawContainedCreate } from '../www/core/contained-create.js';
import { selectBridgePattern } from '../www/core/raw-canon-type.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiPreparation } from '../www/core/raw-ai-preparation.js';
import { RawAiStrategySetup } from '../www/core/raw-ai-strategy-setup.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E182)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 배열 두 개의 원소가 모두 같은지 본다. (2026-11-09 WB04-E182)
const same = (left, right) => left.length === right.length && left.every((value, index) => value === right[index]);

// 선택되지 않은 실제 하위는 빈 성공을 공급하지 않고 즉시 거부한다. (2026-11-09 WB04-E182)
const missing = () => { throw new Error('검사 외 실제 하위'); };

// 내용물의 실제 생성/부착을 같은 풀에 적용하는 작은 검사 장면이다. (2026-11-09 WB04-E182)
class StrategyInput {
    // 기존 모듈을 같은 저장소에 조립하며 게임/원본 실행을 대신하지 않는다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x43000000, (raw) => raw.fill(0));
        this.pool = new SidPool(32768, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, theme: 0 }));
        this.bookkeeping = new SquidPostPopState();
        this.post = new SquidPostPop(this.pool, this.types, this.bookkeeping);
        this.players = new Uint8Array(1620);
        this.allies = new Uint32Array(81);
        this.playerState = new PlayerWorldState();
        this.finderState = new ContainedFinderState();
        this.random = new GameRandom(0x12345678);
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.product = 133;
        this.invalidGraph = 254;
        this.noRegion = 127;
        this.mapLimit = 256;
        this.now = 16777216.25;
        this.attached = [];
        this.assertions = [];
        this.trace = [];
        this.playerState.playerKnown = 1;
        this.player = new RawPlayerWorld(this.pool, this.post, this.players, this.playerState, { assertion: missing, find: missing, create: missing, setOwner: missing, pop: missing, reserveRegular: missing, addRegular: missing });
        this.finder = new RawCarrierPreDestroy(this.pool, this.finderState, { validateDamageable: missing, damageablePre: missing, dependentFound: missing });
        this.create = new RawContainedCreate(this.pool, { containedType: () => 6, bridgeType: () => 82 }, {
            create: (type, flags) => { assert.ok(type === 6 && flags === 2); const sid = this.newSlot(); this.pool.allocated(sid)[10] = 6; return sid; },
            notifySurface: missing,
            attach: (form, parent, flags) => { this.link(parent, form); this.attached.push([form, parent, flags]); this.trace.push('create'); }
        });
        this.technology = this.newSlot();
        this.temple = this.newSlot();
        this.pool.allocated(this.temple)[10] = 87;
        this.types[87].theme = 2;
        // 자기 지역/적 지역과 기술 부모를 명시하고 자기 방향 동맹만 설정한다.
        for (let owner = 0; owner < 9; owner += 1) {
            put(this.players, owner * 180 + 0x78, 127);
            this.allies[owner * 9 + owner] = 1;
        }
        put(this.players, 180 + 0x9c, this.technology);
        put(this.players, 360 + 0x9c, this.technology);
        put(this.players, 180 + 0x78, 3);
        put(this.players, 360 + 0x78, 4);
        this.field(0x260, 1);
        this.field(0x264, 2);
        this.field(0x238, 3);
        this.field(0x600, floatToBits(3.25));
        this.field(0xaf0, 0xc61c4000);
        this.field(0xaf4, 0xc61c4000);
        this.preparation = new RawAiPreparation(this.memory, this.pool, this.players, this.allies, {
            localOwner: () => this.playerState.battleOwner, playerKnown: () => this.playerState.playerKnown, noRegion: () => this.noRegion, invalidGraph: () => this.invalidGraph, now: () => this.now
        }, { priest: (owner) => (owner === 2 ? 99 : 0) });
        this.hooks = {
            regionBounds: (region) => { assert.equal(region, 3); this.trace.push('bounds'); return [16, 32, 64, 96]; },
            temple: (region) => { assert.equal(region, 3); this.trace.push('temple'); return this.temple; },
            destroy: (form, flags) => {
                assert.equal(flags, 0);
                assert.equal(get(this.pool.slot(this.technology), 6, 2), form);
                put(this.pool.allocated(this.technology), 6, get(this.pool.slot(form), 4, 2), 2);
                this.trace.push('destroy');
            },
            assertion: (text) => { this.assertions.push(text); }
        };
    }

    // 같은 풀의 실제 할당을 0 입력으로 준비한다. (2026-11-09 WB04-E182)
    newSlot() {
        const sid = this.pool.allocate(2);
        this.pool.allocated(sid).fill(0);
        return sid;
    }

    // 부모 head 와 새 내용물 next 를 같은 풀에서 연결한다. (2026-11-09 WB04-E182)
    link(parent, form) {
        put(this.pool.allocated(form), 4, get(this.pool.slot(parent), 6, 2), 2);
        put(this.pool.allocated(parent), 6, form, 2);
    }

    // 원본 내용물 조건/패턴의 명시 입력을 만든다. (2026-11-09 WB04-E182)
    content(parent, flags, type, pattern) {
        const sid = this.newSlot();
        const raw = this.pool.allocated(sid);
        raw[10] = 6;
        put(raw, 20, flags, 2);
        put(raw, 18, type, 2);
        put(raw, 12, pattern, 2);
        this.link(parent, sid);
        return sid;
    }

    // 현재 본체 DWORD 를 읽거나(값 없음) 해당 DWORD 만 바꾼다. (2026-11-09 WB04-E182)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 같은 실제 부모/내용물/난수/전역을 연결한다. (2026-11-09 WB04-E182)
    make() {
        return new RawAiStrategySetup(this.memory, this.player, this.preparation, this.finder, this.create, this.finderState, this.types, this.random, this.allies, {
            localOwner: () => this.playerState.battleOwner, noRegion: () => this.noRegion, templeProduct: () => this.product, bridgeType: () => 82, mapLimit: () => this.mapLimit, now: () => this.now
        }, this.hooks);
    }
}

test('AI 전략 준비: 적/양쪽 지역이 없을 때 하위나 +0x258/+0x25c 를 건드리지 않고 재시도를 허용한다', () => {
    const input = new StrategyInput();
    put(input.players, 360 + 0x78, 127);
    input.hooks = {};
    const before = Buffer.from(input.memory.bytes(input.token));
    input.make().initialize(input.token);
    assert.ok(before.equals(Buffer.from(input.memory.bytes(input.token))) && input.attached.length === 0);
    input.field(0x264, 0);
    input.make().initialize(input.token);
    assert.ok(input.field(0x264) === 2 && !input.field(0x258) && !input.field(0x25c));
    put(input.players, 180 + 0x78, 127);
    input.field(0x264, 1);
    input.make().initialize(input.token);
    assert.ok(input.field(0x264) === 2 && input.trace.length === 0);
});

test('AI 전략 준비: 자기 경계 → 다리 → 현재 신전 → 첫 생산 → +0x258 의 실제 순서와 QWORD 기한을 확인한다', () => {
    const input = new StrategyInput();
    input.make().initialize(input.token);
    assert.ok(input.field(0x258) === 1 && input.field(0x25c) === 0 && input.field(0x628) === 3 && input.field(0x5f4) === 3 && input.field(0x390) === 2);
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, input.field(0x268), true);
    const centerX = view.getFloat32(0, true);
    view.setUint32(0, input.field(0x26c), true);
    assert.ok(centerX === 40 && view.getFloat32(0, true) === 64);
    assert.ok(input.attached.length === 4 && same(input.trace, ['bounds', 'create', 'create', 'create', 'temple', 'create']));
    assert.equal(input.field(0x608), 0);
    const clock = new DataView(new ArrayBuffer(8));
    clock.setUint32(0, input.field(0x630), true);
    clock.setUint32(4, input.field(0x634), true);
    assert.equal(clock.getFloat64(0, true), 16777219.5);
    const [form, parent, flags] = input.attached.at(-1);
    assert.ok(parent === input.temple && flags === 0x10 && get(input.pool.slot(form), 18, 2) === 133 && get(input.pool.slot(form), 20, 2) === 0x10);
    assert.ok(!input.make().findProduct(input.token, 133));
});

test('AI 전략 준비: 원격 AI 는 mask 8 기술 부모와 mask 10 신전 생산을 함께 만들고 기존 mask 8 이 있으면 첫 생산을 생략한다', () => {
    const input = new StrategyInput();
    input.field(0x260, 2);
    put(input.players, 360 + 0x78, 3);
    put(input.players, 180 + 0x78, 4);
    input.field(0x264, 1);
    input.field(0x238, 0);
    const setup = input.make();
    setup.initialize(input.token);
    assert.ok(input.attached.length === 2 && input.field(0x258) === 1);
    assert.ok(input.attached[0][1] === input.technology && get(input.pool.slot(input.attached[0][0]), 20, 2) === 8);
    assert.ok(input.attached[1][1] === input.temple && get(input.pool.slot(input.attached[1][0]), 20, 2) === 0x10);
    assert.ok(setup.findProduct(input.token, 133) !== 0);
    const oldCount = input.attached.length;
    input.field(0x390, 3);
    setup.initialize(input.token);
    assert.ok(input.attached.length === oldCount && input.field(0x390) === 3);
});

test('AI 전략 준비: 보류 중인 다리는 총수에만 포함하며 기존 패턴의 TYPE/free/dead 를 임의 필터하지 않는다', () => {
    const input = new StrategyInput();
    input.content(input.technology, 0x40, 99, 7);
    const pending = input.content(input.technology, 0x140, 98, 8);
    input.pool.allocated(pending)[11] = 3;
    input.content(input.technology, 0x20, 82, 9);
    input.make().seedBridges(input.token, input.technology, 3);
    assert.ok(input.attached.length === 1 && input.field(0x628) === 2 && input.field(0x608) === 7 && input.field(0x60c) === 0 && input.field(0x5f4) === 1);
    assert.equal(get(input.pool.slot(input.attached[0][0]), 20, 2), 0x40);
});

test('AI 전략 준비: 직선 강제도 매번 난수를 소비하고 serial signed 감김에서도 4배수 규칙을 유지한다', () => {
    const input = new StrategyInput();
    const expected = new GameRandom(input.random.state);
    const patterns = [];
    // 기대값은 원본 난수와 가중 표의 독립 호출로 만들고 생산 횟수만 명시한다.
    for (let index = 0; index < 5; index += 1) {
        let pattern = selectBridgePattern(expected.next(10000));
        if (index % 4 === 0) pattern = 0;
        patterns.push(pattern);
    }
    input.make().seedBridges(input.token, input.technology, 5);
    assert.ok(input.random.state === expected.state && input.attached.length === 5);
    // 실제 재고와 실제 생성 내용물의 같은 패턴을 대조한다.
    patterns.forEach((pattern, index) => assert.ok(input.field(0x608 + index * 4) === pattern && get(input.pool.slot(input.attached[index][0]), 12, 2) === pattern));
    const wrap = new StrategyInput();
    wrap.field(0x5f4, 0x80000000);
    wrap.make().seedBridges(wrap.token, wrap.technology, 1);
    assert.ok(wrap.field(0x608) === 0 && wrap.field(0x5f4) === 0x80000001);
});

test('AI 전략 준비: 원본 중복 진단을 계속 진행시킨 뒤 현재 원격 내용물을 삭제하고 새 생산을 등록한다', () => {
    const input = new StrategyInput();
    input.field(0x260, 2);
    const previous = input.content(input.technology, 8, 133, 0);
    input.make().addProduct(input.token, input.temple, 133);
    assert.ok(same(input.assertions, ['!haveProd( typeNum )', '!haveTeleportable( typeNum )']));
    assert.ok(same(input.trace, ['destroy', 'create', 'create']) && input.make().findProduct(input.token, 133) !== previous);
    const noBounds = new StrategyInput();
    noBounds.hooks.regionBounds = undefined;
    assert.ok(throws(() => noBounds.make().initialize(noBounds.token)));
    assert.ok(!noBounds.field(0x258) && noBounds.attached.length === 0);
    const noTemple = new StrategyInput();
    noTemple.hooks.temple = undefined;
    assert.ok(throws(() => noTemple.make().initialize(noTemple.token)));
    assert.ok(noTemple.attached.length === 3 && !noTemple.field(0x258) && noTemple.field(0x628) === 3);
    const overflow = new StrategyInput();
    assert.ok(throws(() => overflow.make().seedBridges(overflow.token, overflow.technology, 9)));
    assert.ok(overflow.field(0x628) === 8 && overflow.attached.length === 8 && !overflow.field(0x258));
});

test('AI 전략 준비: signed 비용+1200 의 엄격한 경계와 목표 우선순위를 독립 기대 표로 대조한다', () => {
    const input = new StrategyInput();
    const setup = input.make();
    // 원본 네 목표의 선택 조건을 교차 입력한다.
    for (const attached of [0, 1]) {
        // 경제/공격 기능 비트의 독립 조합이다.
        for (const abilities of [0, 4, 16, 20]) {
            // 비용+1200 경계 아래/같음/초과와 signed 감김을 포함한다.
            for (const [cost, wallet, rich] of [[100, 1299, false], [100, 1300, false], [100, 1301, true], [0x7fffffff, 0, true]]) {
                // 자기/적 그래프가 같은 경우 공격 목표 1 은 선택되지 않는다.
                for (const sameGraph of [false, true]) {
                    // 특정 좌표가 있더라도 목표 1 또는 초기 경제 목표 2 가 우선한다.
                    for (const point of [false, true]) {
                        input.field(0x2d0, attached);
                        input.field(0x248, abilities);
                        input.field(0x290, cost);
                        input.field(0x2d8, wallet);
                        input.field(0x298, 3);
                        input.field(0x2ac, sameGraph ? 3 : 4);
                        input.field(0xaf0, floatToBits(point ? 32.0 : 0.0));
                        input.field(0xaf4, floatToBits(32.0));
                        const expected = attached && rich && (abilities & 4) && !sameGraph ? 1 : !attached && (abilities & 16) ? 2 : point ? 4 : attached && rich ? 3 : (abilities & 16) ? 2 : 3;
                        setup.selectGoal(input.token);
                        assert.equal(input.field(0x2d4), expected, `목표 attached=${attached} abilities=${abilities} cost=${cost} wallet=${wallet} same=${sameGraph} point=${point}`);
                    }
                }
            }
        }
    }
});

test('AI 전략 준비: 같은 목표에서는 전체 본체를 보존하고 변경 때만 원본 6 DWORD 를 초기화하며 좌표 경계는 엄격하다', () => {
    const input = new StrategyInput();
    input.field(0x2d4, 3);
    input.field(0x5f0, 77);
    input.field(0x3cc, 88);
    const setup = input.make();
    const original = Buffer.from(input.memory.bytes(input.token));
    setup.selectGoal(input.token);
    assert.ok(original.equals(Buffer.from(input.memory.bytes(input.token))));
    input.field(0xaf0, floatToBits(1.0));
    input.field(0xaf4, floatToBits(255.0));
    setup.selectGoal(input.token);
    assert.ok(input.field(0x2d4) === 4 && input.field(0x5f0) === 0 && input.field(0x3cc) === 0 && input.field(0x3d0) === 0xc61c4000 && input.field(0x3d4) === 0xc61c4000 && input.field(0x3d8) === 1);
    // 지도 끝·0·NaN 은 특정 좌표 목표로 인정하지 않는다.
    for (const invalid of [0.0, 256.0, Number.NaN]) {
        input.field(0xaf0, floatToBits(invalid));
        setup.selectGoal(input.token);
        assert.equal(input.field(0x2d4), 3);
    }
});

test('AI 전략 준비: 신전 없음/충족/기한 전/데모 조건에서도 기존 재고를 먼저 스캔하며 한 번에 하나만 보충한다', () => {
    const input = new StrategyInput();
    input.field(0x604, 3);
    input.content(input.technology, 0x40, 82, 7);
    const setup = input.make();
    setup.refreshBridges(input.token, input.technology);
    assert.ok(input.field(0x628) === 1 && input.attached.length === 0);
    input.pool.allocated(input.technology)[34] = 1;
    put(input.players, 180 + 0x7c, 99);
    const future = new DataView(new ArrayBuffer(8));
    future.setFloat64(0, input.now + 100, true);
    input.field(0x630, future.getUint32(0, true));
    input.field(0x634, future.getUint32(4, true));
    input.hooks.demo = () => false;
    input.make().refreshBridges(input.token, input.technology);
    assert.equal(input.attached.length, 0);
    input.hooks.demo = () => true;
    input.make().refreshBridges(input.token, input.technology);
    assert.ok(input.attached.length === 1 && input.field(0x628) === 2);
    input.make().refreshBridges(input.token, input.technology);
    assert.ok(input.attached.length === 2 && input.field(0x628) === 3);
    input.hooks.demo = undefined;
    input.make().refreshBridges(input.token, input.technology);
    assert.equal(input.attached.length, 2);
});

test('AI 전략 준비: 원격은 기한 전 데모 getter 를 호출하지 않고 unordered 시각도 원본 C0 분기로 대기한다', () => {
    const input = new StrategyInput();
    input.field(0x604, 2);
    input.pool.allocated(input.technology)[34] = 2;
    put(input.players, 360 + 0x7c, 99);
    const due = new DataView(new ArrayBuffer(8));
    due.setFloat64(0, input.now + 1, true);
    input.field(0x630, due.getUint32(0, true));
    input.field(0x634, due.getUint32(4, true));
    const setup = input.make();
    setup.refreshBridges(input.token, input.technology);
    assert.equal(input.attached.length, 0);
    input.now += 1;
    setup.refreshBridges(input.token, input.technology);
    assert.equal(input.attached.length, 1);
    input.now = Number.NaN;
    setup.refreshBridges(input.token, input.technology);
    assert.ok(input.attached.length === 1 && input.field(0x628) === 1);
    input.pool.allocated(input.technology)[34] = 1;
    put(input.players, 180 + 0x7c, 99);
    assert.ok(throws(() => setup.refreshBridges(input.token, input.technology)));
    assert.equal(input.field(0x628), 1);
});

test('AI 전략 준비 생성자: 본체 소유자/같은 풀의 하위 몸체/Player 9기록/동맹 81/TYPE 표/난수/현재 전역 조회기가 없으면 거부한다', () => {
    const input = new StrategyInput();
    const globals = { localOwner: () => 1, noRegion: () => 127, templeProduct: () => 133, bridgeType: () => 82, mapLimit: () => 256, now: () => 0 };
    const args = [input.memory, input.player, input.preparation, input.finder, input.create, input.finderState, input.types, input.random, input.allies, globals, input.hooks];
    const make = (index, value) => () => new RawAiStrategySetup(...args.map((original, at) => (at === index ? value : original)));
    assert.ok(throws(make(0, undefined)));
    assert.ok(throws(make(1, undefined)));
    assert.ok(throws(make(2, { pool: new SidPool(32768, { server: true }) })));
    assert.ok(throws(make(3, { pool: new SidPool(32768, { server: true }) })));
    assert.ok(throws(make(4, { pool: new SidPool(32768, { server: true }) })));
    assert.ok(throws(make(6, [])));
    assert.ok(throws(make(7, {})));
    assert.ok(throws(make(8, new Uint32Array(80))));
    for (const name of Object.keys(globals)) assert.ok(throws(make(9, { ...globals, [name]: undefined })));
    assert.equal(input.make().pool, input.pool);
});
