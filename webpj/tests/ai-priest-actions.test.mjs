// 사제 AI 행동 raw-ai-priest-actions.js(RawAiPriestActions·snapAiPriestPoint·aiPriestPointValid·aiPriestInRitual·aiAltarContainsEnemy)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 진행 중 탐색이 사제 0/생각 기한/TYPE 0 보다 먼저, 생각 관문의 NaN·상처·예약·이동 중 최대 HP 조회·건설 기한,
//    의식은 본체 +0x29c 의 현재 사제, 제단 하위 효과 뒤 희생 요청의 두 SID 재조회, 거리 2 와 NaN 은 접근하지 않고 2 초과만 이동, TYPE 선택이 저자금보다 먼저이며 HP 선저장·현재 800h 능력, TYPE 0 은 유효 목표만 지움,
//    좌표 일치 배치는 20초 기한 뒤 raw 좌표를 넘기고 목표를 지움, 탐색 기한의 less/unordered, 완료된 같은 TYPE 탐색의 최적 좌표 소비·보정·이동, 새 탐색의 여덟 인자 snapshot 과 현재 시각 + 3, 선택 하위가 탐색 진행 상태로 바꾸면 소비하지 않음,
//    회수 대상(signed HP 최대 첫 객체)과 요청 순서·회수 기한의 0 비교(과거 기한/NaN 도 막음)·누락 하위의 앞선 효과 보존, 0.99999 보정과 mapLimit float 저장, 제단 argument WORD 와 SID 0, 의식 구간의 실제 프레임 코드.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E171: AI 사제 행동 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiPriestActions, aiAltarContainsEnemy, aiPriestInRitual, aiPriestPointValid, snapAiPriestPoint } from '../www/core/raw-ai-priest-actions.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E171)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 단정도 값과 목표 미정 비트이다. (2026-11-09 WB04-E171)
const F = Math.fround;
const UNSET = 0xc61c4000;

// 실제 본체와 슬롯에 명시 입력을 넣고 선택된 하위 호출만 관찰한다. (2026-11-09 WB04-E171)
class AiPriestScene {
    // 기본은 건강한 사제/신전/충분한 지갑/미정 목표/완료된 빈 탐색이다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x4f000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.list = this.memory.allocate(32);
        this.pool = new SidPool(32768, { server: false });
        this.priest = 5;
        this.self = 6;
        this.dais = 7;
        this.home = 8;
        this.building = 9;
        this.other = 10;
        this.altar = 11;
        this.now = 100;
        this.mapLimit = 256;
        this.daisType = 165;
        this.vulnerable = false;
        this.busy = false;
        this.ritual = false;
        this.altarEnemy = false;
        this.lowFunds = false;
        this.path = 0;
        this.maximum = 1000;
        this.distance = 2;
        this.selected = 86;
        this.trace = [];
        this.requests = [];
        // 실제 할당은 SID 5 부터 순서대로 배정하며 각 입력의 주변 바이트를 초기화한다.
        for (let id = 5; id <= 11; id += 1) {
            assert.equal(this.pool.allocate(2), id);
            const raw = this.pool.allocated(id);
            raw.fill(0);
            raw[10] = 158;
            put(raw, 26, 1000);
        }
        this.position(this.priest, { x: F(20.2), y: F(30.7) });
        this.position(this.dais, { x: 40, y: 50 });
        this.field(0x29c, this.priest);
        this.field(0x294, this.home);
        this.field(0x264, 2);
        this.field(0x260, 1);
        this.field(0x2d8, 1000);
        this.field(0x300, this.list);
        this.target({ x: -10000, y: -10000 });
        this.field(0x270, floatToBits(10));
        this.field(0x274, floatToBits(20));
        this.field(0x278, floatToBits(50));
        this.field(0x27c, floatToBits(70));
        this.field(0x2b0, floatToBits(25));
        this.field(0x2b4, floatToBits(26));
        this.hooks = {
            scan: (brain, budget) => { assert.equal(brain, this.token); this.trace.push(`scan:${budget}`); this.field(0x5a4, this.field(0x5a0)); },
            vulnerable: (actor) => { assert.equal(actor, this.priest); this.trace.push('vulnerable'); return this.vulnerable; },
            busy: (actor) => { assert.equal(actor, this.priest); this.trace.push('busy'); return this.busy; },
            path: (actor) => { assert.equal(actor, this.priest); this.trace.push('path'); return this.path; },
            maxHitPoints: (actor) => { assert.equal(actor, this.priest); this.trace.push('max'); return this.maximum; },
            ritual: (actor) => { this.trace.push(`ritual:${actor}`); return this.ritual; },
            altarEnemy: (enemy, target) => { this.trace.push(`altar:${enemy}:${target}`); return this.altarEnemy; },
            target: (brain, actor, target, sacrifice) => { assert.equal(brain, this.token); this.trace.push(`target:${actor}:${target}:${sacrifice}`); },
            distance: (point, target) => { assert.ok(point.x === F(20.2) && point.y === F(30.7)); this.trace.push(`distance:${target}`); return this.distance; },
            select: (brain) => { assert.equal(brain, this.token); this.trace.push('select'); return this.selected; },
            home: (brain, actor) => { assert.ok(brain === this.token && actor === this.priest); this.trace.push('home'); },
            collect: (brain, actor) => { assert.ok(brain === this.token && actor === this.priest); this.trace.push('collect'); },
            deploy: (brain, type, point) => { assert.ok(brain === this.token && type === this.selected && point.x === F(20.2) && point.y === F(30.7)); assert.equal(this.time(0x5c0), this.now + 20); this.trace.push('deploy'); },
            begin: (brain, request) => { assert.equal(brain, this.token); this.requests.push(request); this.trace.push('begin'); this.field(0x574, request.type); this.field(0x5a4, 2); this.field(0x5a0, 4); },
            point: (brain, actor, point) => { assert.ok(brain === this.token && actor === this.priest); assert.ok(point.x === 21 && point.y === 31); this.trace.push('point'); },
            enemyLowFunds: (brain) => { assert.equal(brain, this.token); this.trace.push('funds'); return this.lowFunds; },
            salvage: (target) => { this.trace.push(`salvage:${target}`); }
        };
    }

    // 현재 본체의 지정 DWORD 를 읽거나(값 없음) 지정 DWORD 만 입력한다. (2026-11-09 WB04-E171)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 현재 QWORD 기한을 정확한 double 로 읽거나(값 없음) 두 DWORD 이외를 바꾸지 않고 입력한다. (2026-11-09 WB04-E171)
    time(at, value) {
        const view = new DataView(new ArrayBuffer(8));
        if (value === undefined) {
            view.setUint32(0, this.field(at), true);
            view.setUint32(4, this.field(at + 4), true);
            return view.getFloat64(0, true);
        }
        view.setFloat64(0, value, true);
        this.field(at, view.getUint32(0, true));
        this.field(at + 4, view.getUint32(4, true));
        return undefined;
    }

    // 목표 좌표 입력을 같은 ad0/ad4 에 쓴다. (2026-11-09 WB04-E171)
    target(point) {
        this.field(0xad0, floatToBits(point.x));
        this.field(0xad4, floatToBits(point.y));
    }

    // 물리 슬롯의 현재 두 좌표만 입력한다. (2026-11-09 WB04-E171)
    position(actor, point) {
        put(this.pool.allocated(actor), 14, floatToBits(point.x));
        put(this.pool.allocated(actor), 18, floatToBits(point.y));
    }

    // 명시 건물 목록을 실제 backing 에 기록한다. (2026-11-09 WB04-E171)
    buildings(values) {
        this.field(0x308, values.length);
        // 명시 목록 순서를 보존한다.
        values.forEach((sid, index) => put(this.memory.bytes(this.list), index * 4, sid));
    }

    // 실제 처리기에 현재 입력을 빌려 한 번 호출한다. (2026-11-09 WB04-E171)
    act(actor) {
        const actions = new RawAiPriestActions(this.memory, this.pool, { now: () => this.now, mapLimit: () => this.mapLimit, daisType: () => this.daisType }, this.hooks);
        assert.equal(actions.pool, this.pool);
        actions.act(this.token, actor);
    }

    // 앞선 공통 관문 뒤에 기대하는 정확한 호출 목록을 만든다. (2026-11-09 WB04-E171)
    afterGates(tail) { return ['vulnerable', 'busy', 'path', ...tail]; }
}

test('AI 사제 행동: 진행 중 탐색은 사제 0/생각 기한/TYPE 0 보다 먼저 처리한다', () => {
    const scene = new AiPriestScene();
    scene.field(0x5a4, 0xffffffff);
    scene.field(0x5a0, 0);
    scene.time(0x5a8, 999);
    scene.act(0);
    assert.deepEqual(scene.trace, ['scan:0']);
    assert.ok(!scene.field(0x574));
    scene.trace.length = 0;
    scene.act(0);
    assert.equal(scene.trace.length, 0);
});

test('AI 사제 행동: 생각 관문의 NaN·상처·예약·이동 중 최대 HP 조회·건설 기한을 구분한다', () => {
    const scene = new AiPriestScene();
    scene.time(0x5a8, Number.NaN);
    scene.act(scene.priest);
    assert.equal(scene.trace.length, 0);
    scene.time(0x5a8, 100);
    scene.vulnerable = true;
    scene.act(scene.priest);
    assert.deepEqual(scene.trace, ['vulnerable']);
    scene.trace.length = 0;
    scene.vulnerable = false;
    scene.busy = true;
    scene.act(scene.priest);
    assert.deepEqual(scene.trace, ['vulnerable', 'busy']);
    scene.trace.length = 0;
    scene.busy = false;
    scene.path = scene.other;
    scene.act(scene.priest);
    assert.deepEqual(scene.trace, scene.afterGates(['max']));
    scene.trace.length = 0;
    scene.path = 0;
    scene.time(0x5c0, Number.NaN);
    scene.act(scene.priest);
    assert.deepEqual(scene.trace, scene.afterGates([]));
});

test('AI 사제 행동: 의식 구간은 명시 사제 인자 대신 본체 29c 의 현재 사제를 묻는다', () => {
    const scene = new AiPriestScene();
    scene.field(0x248, 0x400);
    scene.field(0x2a4, scene.dais);
    scene.field(0x29c, scene.self);
    scene.ritual = true;
    scene.act(scene.priest);
    assert.deepEqual(scene.trace, scene.afterGates(['ritual:6']));
});

test('AI 사제 행동: 제단 하위 효과 뒤 실제 희생 요청의 두 SID 를 다시 읽는다', () => {
    const scene = new AiPriestScene();
    scene.field(0x248, 0x400);
    scene.field(0x2a4, scene.dais);
    scene.hooks.altarEnemy = (enemy, dais) => {
        assert.ok(enemy === 2 && dais === scene.dais);
        scene.trace.push('altar');
        scene.field(0x29c, scene.self);
        scene.field(0x2a4, scene.other);
        return true;
    };
    scene.act(scene.priest);
    assert.deepEqual(scene.trace, scene.afterGates(['ritual:5', 'altar', 'target:6:10:1']));
});

test('AI 사제 행동: 거리 2 와 NaN 은 접근하지 않으며 2 초과만 현재 다이스에 이동한다', () => {
    const scene = new AiPriestScene();
    scene.field(0x248, 0x400);
    scene.field(0x2a4, scene.dais);
    scene.hooks.distance = (point, target) => { assert.equal(target, scene.dais); scene.trace.push('distance'); scene.field(0x2a4, scene.other); return 2.1; };
    scene.act(scene.priest);
    assert.deepEqual(scene.trace, scene.afterGates(['ritual:5', 'altar:2:7', 'distance', 'target:5:10:0']));
    // 두 비접근 경계는 이후 TYPE 선택과 새 탐색까지 진행해야 한다.
    for (const distance of [2.0, Number.NaN]) {
        const boundary = new AiPriestScene();
        boundary.field(0x248, 0x400);
        boundary.field(0x2a4, boundary.dais);
        boundary.distance = distance;
        boundary.act(boundary.priest);
        assert.deepEqual(boundary.trace, boundary.afterGates(['ritual:5', 'altar:2:7', 'distance:7', 'select', 'begin']));
    }
});

test('AI 사제 행동: TYPE 선택이 저자금보다 먼저이며 HP 선저장·현재 800h 능력으로 수집을 고른다', () => {
    const scene = new AiPriestScene();
    scene.field(0x2d8, 399);
    scene.hooks.maxHitPoints = (actor) => {
        assert.equal(actor, scene.priest);
        scene.trace.push('max');
        put(scene.pool.allocated(actor), 26, 0);
        scene.field(0x248, 0x800);
        return 1000;
    };
    scene.act(scene.priest);
    assert.deepEqual(scene.trace, scene.afterGates(['select', 'max', 'collect']));
    assert.equal(scene.time(0x5c8), 0);
    const flying = new AiPriestScene();
    flying.selected = 0;
    flying.field(0x2cc, 1);
    flying.act(flying.priest);
    assert.deepEqual(flying.trace, flying.afterGates(['select', 'home']));
});

test('AI 사제 행동: TYPE 0 은 유효 목표만 지우고 미정 목표에서는 귀환/수집으로 넘어간다', () => {
    const scene = new AiPriestScene();
    scene.selected = 0;
    scene.target({ x: 50, y: 60 });
    scene.act(scene.priest);
    assert.deepEqual(scene.trace, scene.afterGates(['select']));
    assert.ok(scene.field(0xad0) === UNSET && scene.field(0xad4) === UNSET);
    scene.trace.length = 0;
    scene.act(scene.priest);
    assert.deepEqual(scene.trace, scene.afterGates(['select', 'max', 'home']));
});

test('AI 사제 행동: 좌표 일치 배치는 계획 기한보다 먼저 20초를 쓰고 raw 좌표를 넘긴 뒤 목표를 지운다', () => {
    const scene = new AiPriestScene();
    scene.target({ x: 21, y: 31 });
    scene.time(0x5c8, 999);
    scene.hooks.deploy = (brain, type, point) => {
        assert.ok(brain === scene.token && type === 86 && point.x === F(20.2) && point.y === F(30.7));
        assert.equal(scene.time(0x5c0), 120);
        scene.trace.push('deploy');
        scene.target({ x: 90, y: 91 });
    };
    scene.act(scene.priest);
    assert.deepEqual(scene.trace, scene.afterGates(['select', 'deploy']));
    assert.ok(scene.field(0xad0) === UNSET && scene.field(0xad4) === UNSET && scene.time(0x5c8) === 999);
});

test('AI 사제 행동: 실제 배치 하위 누락은 앞서 쓴 20초 기한을 보존하며 목표를 지우지 않는다', () => {
    const scene = new AiPriestScene();
    scene.target({ x: 21, y: 31 });
    scene.hooks.deploy = undefined;
    assert.ok(throws(() => scene.act(scene.priest)));
    assert.ok(scene.time(0x5c0) === 120 && bitsToFloat(scene.field(0xad0)) === 21);
});

test('AI 사제 행동: 계획 기한의 less/unordered 는 새 탐색·후보 소비를 수행하지 않는다', () => {
    // 명시 두 기한만 바꾸고 같은 관문까지 도달한다.
    for (const due of [101, Number.NaN]) {
        const scene = new AiPriestScene();
        scene.time(0x5c8, due);
        scene.act(scene.priest);
        assert.ok(JSON.stringify(scene.trace) === JSON.stringify(scene.afterGates(['select'])) && scene.requests.length === 0);
    }
});

test('AI 사제 행동: 완료된 같은 TYPE 탐색은 실제 최적 좌표를 보정하고 이동하며 3초를 쓰지 않는다', () => {
    const scene = new AiPriestScene();
    scene.field(0x574, 86);
    scene.field(0x57c, floatToBits(20.2));
    scene.field(0x580, floatToBits(30.7));
    scene.act(scene.priest);
    assert.deepEqual(scene.trace, scene.afterGates(['select', 'point']));
    assert.ok(scene.field(0x574) === 0 && scene.time(0x5c8) === 0 && bitsToFloat(scene.field(0xad0)) === 21);
    const empty = new AiPriestScene();
    empty.field(0x574, 86);
    empty.field(0x57c, UNSET);
    empty.field(0x580, UNSET);
    empty.act(empty.priest);
    assert.ok(JSON.stringify(empty.trace) === JSON.stringify(empty.afterGates(['select'])) && empty.time(0x5c8) === 103);
    assert.ok(bitsToFloat(empty.field(0xad0)) === -9999 && !empty.field(0x574));
});

test('AI 사제 행동: 새 탐색은 그 시점의 8인자를 전달하고 하위 효과 뒤 현재 시각 + 3 을 쓴다', () => {
    const scene = new AiPriestScene();
    scene.hooks.begin = (brain, request) => {
        assert.equal(brain, scene.token);
        scene.trace.push('begin');
        scene.requests.push(request);
        scene.field(0x270, 0);
        scene.field(0x260, 3);
        scene.now = 125;
    };
    scene.act(scene.priest);
    assert.ok(JSON.stringify(scene.trace) === JSON.stringify(scene.afterGates(['select', 'begin'])) && scene.requests.length === 1 && scene.time(0x5c8) === 128);
    const request = scene.requests[0];
    assert.ok(request.type === 86 && request.owner === 1 && request.bounds.left === 10 && request.bounds.top === 20 && request.bounds.right === 50 && request.bounds.bottom === 70 && request.avoid.x === 25 && request.avoid.y === 26);
});

test('AI 사제 행동: 선택 하위가 탐색을 다시 진행 상태로 바꾸면 완료 후보를 소비하지 않는다', () => {
    const scene = new AiPriestScene();
    scene.field(0x574, 86);
    scene.hooks.select = (brain) => { assert.equal(brain, scene.token); scene.trace.push('select'); scene.field(0x5a4, 2); scene.field(0x5a0, 4); return 86; };
    scene.act(scene.priest);
    assert.ok(JSON.stringify(scene.trace) === JSON.stringify(scene.afterGates(['select'])) && scene.field(0x574) === 86 && scene.time(0x5c8) === 103);
});

test('AI 사제 행동: 회수 대상은 signed HP 최대 첫 객체이며 free/dead 추가 필터 없이 원본 요청 순서를 따른다', () => {
    const scene = new AiPriestScene();
    scene.selected = 165;
    scene.field(0x574, 165);
    scene.field(0x57c, UNSET);
    scene.field(0x580, UNSET);
    scene.buildings([scene.building, scene.other, scene.home]);
    put(scene.pool.allocated(scene.building), 26, 2000);
    put(scene.pool.allocated(scene.other), 26, 2000);
    put(scene.pool.allocated(scene.home), 26, 0xffffffff);
    scene.pool.allocated(scene.building)[11] = 3;
    scene.hooks.salvage = (target) => {
        assert.equal(target, scene.building);
        scene.trace.push('salvage:9');
        assert.equal(scene.time(0x5e8), 0);
        scene.now = 120;
        scene.field(0x260, 3);
        scene.field(0x270, floatToBits(11));
    };
    scene.hooks.scan = (brain, budget) => {
        assert.ok(brain === scene.token && budget === 1 && scene.time(0x5e8) === 130);
        scene.trace.push('scan:1');
        scene.now = 140;
    };
    scene.act(scene.priest);
    assert.deepEqual(scene.trace, scene.afterGates(['select', 'target:5:9:0', 'salvage:9', 'begin', 'scan:1']));
    assert.ok(scene.time(0x5e8) === 130 && scene.time(0x5c8) === 143 && scene.requests.length === 1);
    assert.ok(scene.requests[0].owner === 3 && scene.requests[0].bounds.left === 11);
});

test('AI 사제 행동: 건물 부재 시 적 저자금 확인 뒤 현재 신전을 읽으며 부재/거짓은 기한만 쓴다', () => {
    const scene = new AiPriestScene();
    scene.selected = 165;
    scene.field(0x574, 165);
    scene.field(0x57c, UNSET);
    scene.field(0x580, UNSET);
    scene.hooks.enemyLowFunds = (brain) => { assert.equal(brain, scene.token); scene.trace.push('funds'); scene.field(0x294, scene.other); return true; };
    scene.act(scene.priest);
    assert.deepEqual(scene.trace, scene.afterGates(['select', 'funds', 'target:5:10:0', 'salvage:10', 'begin', 'scan:1']));
    // 자금 조건 거짓과 신전 부재를 각각 같은 완료 탐색으로 검사한다.
    for (const funds of [false, true]) {
        const empty = new AiPriestScene();
        empty.selected = 165;
        empty.field(0x574, 165);
        empty.field(0x57c, UNSET);
        empty.field(0x580, UNSET);
        empty.field(0x294, 0);
        empty.lowFunds = funds;
        empty.act(empty.priest);
        assert.ok(JSON.stringify(empty.trace) === JSON.stringify(empty.afterGates(['select', 'funds'])) && empty.time(0x5c8) === 103 && empty.time(0x5e8) === 0);
    }
});

test('AI 사제 행동: 회수 기한은 경과 판정이 아닌 0 비교이며 과거 기한과 NaN 도 재회수를 막는다', () => {
    // 이미 경과한 값/NaN 은 요청을 막고 +0/-0 만 처음 회수를 허용한다.
    for (const due of [1, Number.NaN, 0, -0]) {
        const scene = new AiPriestScene();
        scene.selected = 165;
        scene.field(0x574, 165);
        scene.field(0x57c, UNSET);
        scene.field(0x580, UNSET);
        scene.buildings([scene.building]);
        scene.time(0x5e8, due);
        scene.act(scene.priest);
        if (due === 0) assert.deepEqual(scene.trace, scene.afterGates(['select', 'target:5:9:0', 'salvage:9', 'begin', 'scan:1']));
        else assert.deepEqual(scene.trace, scene.afterGates(['select']));
    }
});

test('AI 사제 행동: 회수 하위 누락 시 대상 이동 요청 뒤 기한/재탐색/3초 쓰기는 발생하지 않는다', () => {
    const scene = new AiPriestScene();
    scene.selected = 165;
    scene.field(0x574, 165);
    scene.field(0x57c, UNSET);
    scene.field(0x580, UNSET);
    scene.buildings([scene.building]);
    scene.hooks.salvage = undefined;
    assert.ok(throws(() => scene.act(scene.priest)));
    assert.ok(JSON.stringify(scene.trace) === JSON.stringify(scene.afterGates(['select', 'target:5:9:0'])) && scene.time(0x5e8) === 0 && scene.time(0x5c8) === 0 && scene.requests.length === 0);
});

test('AI 사제 행동: 원본 0.99999 보정은 ceil/반올림과 다르며 mapLimit 은 float 로 저장한 값을 사용한다', () => {
    let point = snapAiPriestPoint({ x: F(1.000001), y: F(-2.9) });
    assert.ok(point.x === 1 && point.y === -1);
    point = snapAiPriestPoint({ x: -10000, y: Number.NaN });
    assert.ok(point.x === -9999 && point.y === 0);
    assert.ok(aiPriestPointValid({ x: F(0.1), y: F(255.9) }, 256));
    assert.ok(!aiPriestPointValid({ x: 0, y: 1 }, 256) && !aiPriestPointValid({ x: 1, y: 256 }, 256) && !aiPriestPointValid({ x: Number.NaN, y: 1 }, 256));
    assert.ok(!aiPriestPointValid({ x: 16777216, y: 1 }, 16777217));
});

test('AI 사제 행동: 제단의 owner BYTE 가 아닌 argument WORD 를 전체 enemy DWORD 와 비교하고 찾은 SID 0 도 읽는다', () => {
    const scene = new AiPriestScene();
    put(scene.pool.allocated(scene.altar), 12, 2, 2);
    scene.pool.allocated(scene.altar)[34] = 3;
    const find = (x, y, type) => { assert.ok(x === 40 && y === 50 && type === 164); return scene.altar; };
    assert.ok(aiAltarContainsEnemy(scene.pool, scene.dais, 2, 164, find));
    assert.ok(!aiAltarContainsEnemy(scene.pool, scene.dais, 0x10002, 164, find));
    const argument = get(scene.pool.slot(0), 12, 2);
    assert.ok(aiAltarContainsEnemy(scene.pool, scene.dais, argument, 164, () => 0));
    assert.ok(throws(() => aiAltarContainsEnemy(scene.pool, scene.dais, 2, 164, undefined)));
});

test('AI 사제 행동: 의식 구간 10..14 는 물리 frame 의 첫 signed 문자에서 구하며 상태 BYTE 와 무관하다', () => {
    const scene = new AiPriestScene();
    const frames = Array.from({ length: 159 }, () => new FrameCodeTable([]));
    frames[158] = new FrameCodeTable(['J', 'K', 'O', 'P'].map((letter) => ({ side: letter.charCodeAt(0), variant: 0, flags: 0 })).concat([{ side: 0xff, variant: 0, flags: 0 }]));
    scene.pool.allocated(scene.priest)[40] = 0xe0;
    // 각 물리 프레임을 그대로 사용하며 state 상위 비트로 의식 여부를 만들지 않는다.
    for (let frame = 0; frame < 5; frame += 1) {
        put(scene.pool.allocated(scene.priest), 36, frame);
        assert.equal(aiPriestInRitual(scene.pool, frames, scene.priest), frame === 1 || frame === 2);
    }
    assert.ok(throws(() => aiPriestInRitual(scene.pool, frames.slice(0, 100), scene.priest)));
});

test('AI 사제 행동 생성자: 본체 소유자/같은 PATCH 풀/현재 전역 조회기가 없으면 거부하고 생성은 행동을 호출하지 않는다', () => {
    const scene = new AiPriestScene();
    const globals = { now: () => 0, mapLimit: () => 256, daisType: () => 165 };
    assert.ok(throws(() => new RawAiPriestActions(undefined, scene.pool, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiPriestActions(scene.memory, {}, globals, scene.hooks)));
    for (const name of ['now', 'mapLimit', 'daisType']) assert.ok(throws(() => new RawAiPriestActions(scene.memory, scene.pool, { ...globals, [name]: undefined }, scene.hooks)));
    new RawAiPriestActions(scene.memory, scene.pool, globals, undefined);
    assert.equal(scene.trace.length, 0);
    // 크기가 다른 가짜 본체는 어떤 효과보다 먼저 거부한다.
    const small = scene.memory.allocate(16);
    assert.ok(throws(() => new RawAiPriestActions(scene.memory, scene.pool, { now: () => 100, mapLimit: () => 256, daisType: () => 165 }, scene.hooks).act(small, scene.priest)) && scene.trace.length === 0);
});
