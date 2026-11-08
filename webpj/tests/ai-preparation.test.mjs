// AI 준비 raw-ai-preparation.js(RawAiPreparation)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 강제 대상과 첫 비동맹 선택의 서로 다른 하위 호출/조기 반환(사제 조회 중 현재 owner 가 바뀌면 그 owner 행으로 판단),
//    같은 최초 스캔 위치·그래프/범위·두 수량 판정과 QWORD 시각을 합친 준비, 절반 HP 경계·전체 동맹 거리 방문·현재 적 조기 반환과 HP 선저장, FILD 정수 비교와 float 차액 저장 및 두 번째 SP 읽기의 현재 owner/지갑 보존,
//    +0x324 포인터·+0x338 개수의 실제 조합과 하위 뒤 현재 개수/포인터/기준 그래프 재조회, 선택되지 않은 누락 하위는 요구하지 않고 선택된 지점에서는 앞선 실제 쓰기를 유지한 채 오류.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E181: AI 준비 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiPreparation } from '../www/core/raw-ai-preparation.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E181)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 배열 두 개의 원소가 모두 같은지 본다. (2026-11-09 WB04-E181)
const same = (left, right) => left.length === right.length && left.every((value, index) => value === right[index]);

const F = Math.fround;

// 실제 등록된 메모리에 합성 입력을 넣으며 원본 실행 관측값이나 게임 대체 AI 로 쓰지 않는다. (2026-11-09 WB04-E181)
class PreparationInput {
    // 원본 생성자를 대신했다고 주장하지 않는 명시 0 본체와 실제 목록 블록을 준비한다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x42000000, (bytes) => bytes.fill(0xa7));
        this.pool = new SidPool(32768, { server: true });
        this.players = new Uint8Array(1620);
        this.allies = new Array(81).fill(0);
        this.priests = new Array(9).fill(0);
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.localOwner = 8;
        this.playerKnown = 1;
        this.invalidGraph = 254;
        this.noRegion = 127;
        this.now = 16777216.25;
        this.demo = 0;
        this.scans = 0;
        this.assertions = 0;
        this.knowledge = false;
        this.blocking = false;
        this.temples = new Map();
        this.graphs = new Map();
        this.priestQueries = [];
        this.graphQueries = [];
        this.spQueries = [];
        this.additions = [];
        this.distances = [];
        const raw = this.memory.bytes(this.token);
        raw.fill(0);
        this.field(0x260, 1);
        this.field(0x264, 2);
        this.field(0x25c, 1);
        this.field(0x324, this.memory.allocate(160));
        this.field(0x328, 40);
        this.field(0x228, floatToBits(3.25));
        // 모든 Player 의 현재 지역을 없으며 자기 방향만 동맹으로 명시한다.
        for (let owner = 0; owner < 9; owner += 1) {
            put(this.players, owner * 180 + 0x78, 127);
            this.allies[owner * 9 + owner] = 1;
        }
        this.hooks = {
            missingEnemyAssertion: () => { this.assertions += 1; },
            priest: (owner) => { this.priestQueries.push(owner); if (owner >= this.priests.length) throw new RangeError('검사 사제 owner 범위'); return this.priests[owner]; },
            temple: (region) => this.temples.get(region) ?? 0,
            maxHitPoints: () => 100,
            pointDistance: (source, target) => { this.distances.push([source, target]); const dx = source[0] - target[0]; const dy = source[1] - target[1]; return Math.sqrt(dx * dx + dy * dy); },
            graph: (sid) => { this.graphQueries.push(sid); return this.graphs.get(sid) ?? this.invalidGraph; },
            regionBounds: () => [16, 32, 64, 96],
            storedSp: (owner) => { this.spQueries.push(owner); return 0.0; },
            addLocalSp: (delta) => { this.additions.push(delta); },
            demo: () => this.demo,
            knowledgeOpen: () => this.knowledge,
            blockingGumpOpen: () => this.blocking,
            initialScan: (brain) => { assert.equal(brain, this.token); this.scans += 1; }
        };
    }

    // 같은 실제 입력을 빌리는 준비 몸체를 구성한다. (2026-11-09 WB04-E181)
    make() {
        return new RawAiPreparation(this.memory, this.pool, this.players, this.allies, {
            localOwner: () => this.localOwner, playerKnown: () => this.playerKnown, noRegion: () => this.noRegion, invalidGraph: () => this.invalidGraph, now: () => this.now
        }, this.hooks);
    }

    // 본체의 현재 DWORD 를 읽거나(값 없음) 바꾼다. (2026-11-09 WB04-E181)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 현재 QWORD 시각을 결과 관찰용 double 로 해석한다. (2026-11-09 WB04-E181)
    time(at) {
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, this.field(at), true);
        view.setUint32(4, this.field(at + 4), true);
        return view.getFloat64(0, true);
    }

    // 원본 목록 포인터의 실제 입력 칸에만 쓴다. (2026-11-09 WB04-E181)
    geyser(index, value) { put(this.memory.bytes(this.field(0x324)), index * 4, value); }

    // 현재 Player 참여 DWORD 와 같은 풀의 사제 raw HP·좌표를 준비한다. (2026-11-09 WB04-E181)
    priest(owner, hp, x, y) {
        const sid = this.pool.allocate(2);
        this.priests[owner] = sid;
        put(this.players, owner * 180 + 0x2c, 1);
        const raw = this.pool.allocated(sid);
        put(raw, 0x1a, hp >>> 0);
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        return sid;
    }
}

test('AI 준비: 강제 대상과 첫 비동맹 선택이 서로 다른 하위 호출/조기 반환 계약을 유지한다', () => {
    const input = new PreparationInput();
    let preparation = input.make();
    input.field(0x240, 0xffffffff);
    preparation.selectEnemy(input.token);
    assert.ok(input.field(0x264) === 0xffffffff && input.priestQueries.length === 0);
    input.field(0x240, 0);
    input.priests[1] = 10;
    input.priests[2] = 11;
    input.priests[3] = 12;
    input.allies[1 * 9 + 2] = 1;
    input.allies[3 * 9 + 1] = 1;
    preparation.selectEnemy(input.token);
    assert.ok(input.field(0x264) === 3 && same(input.priestQueries, [1, 2, 3]));
    input.priestQueries.length = 0;
    input.hooks.priest = (owner) => { input.priestQueries.push(owner); if (owner === 1) input.field(0x260, 2); return owner === 1 ? 10 : 0; };
    preparation = input.make();
    preparation.selectEnemy(input.token);
    assert.ok(input.field(0x264) === 1 && same(input.priestQueries, [1]));
});

test('AI 준비: 같은 최초 스캔 위치·그래프/범위·두 수량 판정과 QWORD 시각을 합쳐 대조한다', () => {
    const input = new PreparationInput();
    input.localOwner = 1;
    input.demo = 1;
    input.knowledge = true;
    input.hooks.blockingGumpOpen = () => { throw new Error('지식 창 참이면 차단 창 조회 금지'); };
    put(input.players, 180 + 0x78, 3);
    put(input.players, 360 + 0x78, 4);
    input.temples = new Map([[3, 100], [4, 200]]);
    input.graphs = new Map([[100, 7], [200, 8], [700, 7], [701, 8]]);
    input.field(0x238, 4);
    input.field(0x220, floatToBits(6.0));
    input.field(0x25c, 0);
    input.field(0x234, 3);
    input.field(0x230, 2);
    input.field(0x2e4, 2);
    input.field(0x2b0, 0xdeadbeef);
    input.hooks.initialScan = (brain) => {
        assert.equal(brain, input.token);
        assert.ok(input.field(0x29c) === 0 && input.field(0x2a0) === 0 && input.spQueries.length === 1);
        input.scans += 1;
        input.geyser(0, 700);
        input.geyser(1, 701);
        input.field(0x338, 2);
        input.field(0x32c, 2);
    };
    let preparation = input.make();
    preparation.update(input.token);
    assert.ok(input.time(0x5a8) === 16777219.5 && input.scans === 1 && input.field(0x25c) === 1);
    assert.ok(input.field(0x294) === 100 && input.field(0x2a8) === 200 && input.field(0x298) === 7 && input.field(0x2ac) === 8 && input.field(0x5f8) === 7);
    assert.ok(input.field(0x604) === 4 && input.field(0x5fc) === 1 && input.field(0x600) === floatToBits(6.0));
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, input.field(0x2b0), true);
    const centerX = view.getFloat32(0, true);
    view.setUint32(0, input.field(0x2b4), true);
    assert.ok(centerX === 40 && view.getFloat32(0, true) === 64);
    assert.ok(input.field(0x2cc) === 0 && input.field(0x2d0) === 0 && input.additions.length === 0);
    assert.ok(same(input.graphQueries, [100, 200, 700, 701, 700, 701]));
    input.field(0x2d8, 12001);
    input.hooks.storedSp = () => 12001.0;
    preparation = input.make();
    preparation.update(input.token);
    assert.ok(input.scans === 1 && input.field(0x2d0) === 1 && preparation.pool === input.pool);
});

test('AI 준비: 절반 HP 경계·전체 동맹 거리 방문·현재 적 조기 반환과 HP 선저장을 확인한다', () => {
    const input = new PreparationInput();
    input.field(0x264, 4);
    input.field(0x248, 0x100);
    const enemy = input.priest(2, 10, 11, 10);
    input.priest(1, 100, 10, 10);
    input.priest(3, 100, 50, 10);
    input.allies[1 * 9 + 3] = 1;
    input.hooks.maxHitPoints = (sid) => { assert.equal(sid, enemy); put(input.pool.allocated(sid), 0x1a, 100); return 100; };
    input.make().update(input.token);
    assert.ok(input.field(0x264) === 2 && input.distances.length === 2);
    assert.ok(same(input.distances[0][0], [10, 10]) && same(input.distances[0][1], [11, 10]) && same(input.distances[1][0], [50, 10]));
    const boundary = new PreparationInput();
    boundary.field(0x264, 4);
    boundary.field(0x248, 0x100);
    boundary.priest(2, 50, 11, 10);
    boundary.make().update(boundary.token);
    assert.ok(boundary.field(0x264) === 4 && boundary.distances.length === 0);
    const current = new PreparationInput();
    current.field(0x248, 0x100);
    current.priest(2, -1, 11, 10);
    current.priest(5, -2, 12, 10);
    current.hooks.pointDistance = () => { throw new Error('현재 취약 적이면 동맹 거리 조회 금지'); };
    current.make().update(current.token);
    assert.ok(current.field(0x264) === 2 && same(current.priestQueries, [2, 1, 2]));
});

test('AI 준비: FILD 정수 비교와 float 차액 저장을 구분하고 두 번째 SP 읽기의 현재 owner/지갑을 보존한다', () => {
    const input = new PreparationInput();
    input.localOwner = 1;
    input.field(0x2d8, 16777217);
    input.hooks.storedSp = (owner) => {
        input.spQueries.push(owner);
        if (input.spQueries.length === 1) input.localOwner = 2;
        else input.field(0x2d8, 16777219);
        return 16777216.0;
    };
    input.make().update(input.token);
    assert.ok(same(input.spQueries, [1, 2]) && same(input.additions, [3.0]));
    const unknown = new PreparationInput();
    unknown.localOwner = 1;
    unknown.playerKnown = 0;
    unknown.field(0x2d8, 9);
    unknown.hooks.storedSp = () => { throw new Error('PlayerKnown0은 분산 SP 읽기 금지'); };
    unknown.make().update(unknown.token);
    assert.ok(same(unknown.additions, [9.0]));
});

test('AI 준비: +0x324 포인터·+0x338 개수의 실제 조합과 하위 뒤 현재 개수/포인터/기준 그래프를 재조회한다', () => {
    const input = new PreparationInput();
    input.field(0x32c, 0xffffffff);
    input.field(0x338, 2);
    input.field(0x298, 7);
    input.geyser(0, 700);
    input.geyser(1, 701);
    const replacement = input.memory.allocate(8);
    put(input.memory.bytes(replacement), 4, 702);
    input.hooks.graph = (sid) => {
        input.graphQueries.push(sid);
        if (sid === 700) { input.field(0x324, replacement); input.field(0x298, 8); }
        return 8;
    };
    const preparation = input.make();
    assert.ok(preparation.countAttachedGeysers(input.token) === 2 && same(input.graphQueries, [700, 702]));
    input.field(0x338, 0);
    input.field(0x324, 0xdeadbeef);
    assert.equal(preparation.countAttachedGeysers(input.token), 0);
    input.field(0x338, 2);
    input.field(0x324, input.memory.allocate(4));
    assert.ok(throws(() => preparation.countAttachedGeysers(input.token)));
});

test('AI 준비: 선택되지 않은 누락 하위는 요구하지 않고 선택된 지점에서는 앞선 실제 쓰기를 유지한 채 오류를 낸다', () => {
    const input = new PreparationInput();
    input.hooks.regionBounds = undefined;
    input.hooks.maxHitPoints = undefined;
    input.hooks.pointDistance = undefined;
    input.hooks.graph = undefined;
    input.make().update(input.token);
    assert.ok(input.field(0x298) === 254 && input.field(0x2ac) === 254 && input.field(0x2d0) === 1);
    input.field(0x25c, 0);
    input.hooks.initialScan = undefined;
    assert.ok(throws(() => input.make().update(input.token)));
    assert.ok(input.field(0x25c) === 0 && input.field(0x5fc) === 1);
    input.field(0x264, 0);
    input.hooks.priest = () => 0;
    input.hooks.missingEnemyAssertion = undefined;
    assert.ok(throws(() => input.make().update(input.token)));
    assert.equal(input.field(0x264), 0);
    const shortBody = input.memory.allocate(16);
    assert.ok(throws(() => input.make().selectEnemy(shortBody)));
    const globals = { localOwner: () => input.localOwner, playerKnown: () => input.playerKnown, noRegion: () => input.noRegion, invalidGraph: () => input.invalidGraph, now: () => input.now };
    assert.ok(throws(() => new RawAiPreparation(input.memory, input.pool, input.players.slice(0, 1619), input.allies, globals, {})));
    assert.ok(throws(() => new RawAiPreparation(undefined, input.pool, input.players, input.allies, globals, {})));
    assert.ok(throws(() => new RawAiPreparation(input.memory, input.pool, input.players, input.allies.slice(1), globals, {})));
    assert.ok(throws(() => new RawAiPreparation(input.memory, input.pool, input.players, input.allies, { ...globals, now: undefined }, {})));
});
