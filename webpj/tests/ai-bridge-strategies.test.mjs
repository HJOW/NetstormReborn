// AI 네 다리 전략 raw-ai-bridge-strategies.js(RawAiBridgeStrategies)를 실제 PE 관측 fixture 와 cpppj 계약으로 대조한다.
//  - ai-bridge-strategies-x86.tsv 198행(원본 00417bd0): 열 = 번호, 하위 입력 14개(QWORD 시각·난수 상태·checking·disabled·graph·connected·advance·rectangle·randomStart XY·extend XY·select·refresh 뒤 목표), 입력 본체 hex,
//    입력 목록 4개(64바이트씩) hex, 입력 슬롯 50..55(50바이트씩) hex, 출력 본체 hex, 출력 목록 hex, 난수 상태, 도달한 하위 호출 순서와 인자.
//    네 case 전체/공통 관문/NaN 기한·좌표/재고 뒤 목표 재조회/중복 목록/기한/좌표를 한 번의 `update` 로 호출해 본체·목록 미사용 꼬리·난수·호출 순서를 모두 대조한다. 기대값은 재계산하지 않는다.
//  - cpppj 계약: checking 이 거짓이면 disabled 를 읽지 않고 재고 효과 뒤 현재 목표/좌표를 다시 쓴다, graph 하위가 대상을 바꾸면 변경 뒤 현재 SID 의 중심을 다시 읽는다, 연장 결과가 시작점과 같으면 잔여 횟수를 소비하지 않는다,
//    기한이 NaN 이면 네 전략 모두 재고 갱신까지만 하고 선택/배치를 하지 않는다, 생성자 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E178: AI 다리 전략 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiBridgeCandidates } from '../www/core/raw-ai-bridge-candidates.js';
import { RawAiBridgePlanning } from '../www/core/raw-ai-bridge-planning.js';
import { RawAiBridgeStrategies } from '../www/core/raw-ai-bridge-strategies.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';

// 실제 PE 관측 fixture 이다. (2026-11-09 WB04-E178)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const rows = readRows(path.join(FIXTURES, 'ai-bridge-strategies-x86.tsv')) ?? [];

// 예외가 나는지 본다. (2026-11-09 WB04-E178)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾸고 되돌린다. (2026-11-09 WB04-E178)
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const bytesOf = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16));

// 점을 호출 순서 비교용 원문 DWORD 쌍으로 나타낸다(NaN 비트 보존 필드가 있으면 그 값). (2026-11-09 WB04-E178)
const xy = (point) => `${(point.xBits ?? floatToBits(point.x)) >>> 0},${(point.yBits ?? floatToBits(point.y)) >>> 0}`;

// 명시 하위 입력과 원본 본체/목록/슬롯만 설치한다. 전략 결과를 미리 만들지 않는다. (2026-11-09 WB04-E178)
class StrategyScene {
    // 독립 fixture 행의 입력 바이트와 현재 하위 입력을 구성한다.
    constructor(row) {
        this.memory = new RawAiBrainMemory(0x10000000, (raw) => raw.fill(0xcd));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.lists = [];
        this.pool = new SidPool(32768, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));
        this.frames = Array.from({ length: 188 }, () => []);
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.random = new GameRandom();
        this.mapLimit = 256;
        this.logKind = 29;
        this.trace = [];
        // 네 실제 소유 목록을 원본 입력과 같은 핸들 순서로 만든다.
        for (let index = 0; index < 4; index += 1) this.lists.push(this.memory.allocate(64));
        // 명시 슬롯 50..55 를 실제 풀에 확보한다.
        for (let index = 5; index <= 55; index += 1) assert.equal(this.pool.allocate(2), index);
        // 현재 QWORD 시각도 손실 없이 보존하려면 BigInt 로 해석한다.
        this.cfg = row[1].split(',').map((value) => BigInt(value));
        const body = bytesOf(row[2]);
        const listBytes = bytesOf(row[3]);
        const slots = bytesOf(row[4]);
        this.memory.bytes(this.token).set(body);
        // 각 목록의 여유 꼬리도 그대로 설치한다.
        for (let index = 0; index < 4; index += 1) this.memory.bytes(this.lists[index]).set(listBytes.subarray(index * 64, index * 64 + 64));
        // 타입/생사/좌표 등 전체 슬롯 입력이다.
        for (let index = 0; index < 6; index += 1) this.pool.allocated(50 + index).set(slots.subarray(index * 50, index * 50 + 50));
        this.types[97].flags2 = 0x10000000;
        this.types[98].flags2 = 0x200;
        Object.assign(this.types[97], { footX: 3, footY: 3 });
        Object.assign(this.types[98], { footX: 3, footY: 3 });
        Object.assign(this.types[99], { footX: 1, footY: 1 });
        const view = new DataView(new ArrayBuffer(8));
        view.setBigUint64(0, this.cfg[0], true);
        this.now = view.getFloat64(0, true);
        this.random.setState(Number(this.cfg[1]));
        this.hooks = {};
        this.bind();
    }

    // 지정한 하위 입력 열의 Number 값이다. (2026-11-09 WB04-E178)
    value(index) { return Number(this.cfg[index]); }

    // 각 명시 하위의 입력/반환을 관측 도구와 같이 공급한다. 목록/전략 계산은 제품이 수행한다. (2026-11-09 WB04-E178)
    bind() {
        const hooks = this.hooks;
        hooks.technology = (owner) => { this.trace.push(`technology,${owner}`); return 50; };
        hooks.refresh = (token, parent) => { this.trace.push(`refresh,${parent}`); if (this.cfg[13]) put(this.memory.bytes(this.token), 0x2d4, this.value(13)); };
        hooks.graph = (sid) => { this.trace.push(`graph,${sid}`); return this.value(4); };
        hooks.center = (sid) => {
            this.trace.push(`center,${sid}`);
            const [x, y] = new RawSquidNeighbors(this.pool, this.hash, this.spots, this.types, this.frames).center(sid);
            return { x, y };
        };
        hooks.connected = (token, sid) => { this.trace.push(`connected,${sid}`); return this.cfg[5] !== 0n; };
        hooks.advance = (token, point) => { this.trace.push(`advance,${xy(point)}`); return this.cfg[6] !== 0n; };
        hooks.home = () => { this.trace.push('home'); return { x: 101.25, y: 102.75 }; };
        hooks.reclassify = () => { this.trace.push('reclassify'); };
        hooks.select = (token, point, excluded, required) => { this.trace.push(`select,${xy(point)},${excluded},${required}`); return this.value(12); };
        hooks.randomStart = (token, bounds) => {
            this.trace.push(`random,${xy({ x: bounds.left, y: bounds.top })},${xy({ x: bounds.right, y: bounds.bottom })}`);
            return { x: bitsToFloat(this.value(8)), y: bitsToFloat(this.value(9)) };
        };
        hooks.extend = (token, source, target) => {
            this.trace.push(`extend,${xy(source)},${xy(target)}`);
            return { x: bitsToFloat(this.value(10)), y: bitsToFloat(this.value(11)) };
        };
        hooks.rectangle = (left, top, right, bottom) => { this.trace.push(`rectangle,${left >>> 0},${top >>> 0},${right >>> 0},${bottom >>> 0}`); return this.cfg[7] !== 0n; };
        hooks.log = (token, kind, event, sid) => { this.trace.push(`log,${kind},${event},${sid}`); };
        hooks.assertion = (text, line) => { this.trace.push(`assert,${line}`); };
        // 같은 실제 후보/난수/시각 위에서 현재 재시도 기한을 쓰는 하위이다.
        hooks.delay = (current) => {
            this.trace.push('delay');
            const candidates = new RawAiBridgeCandidates(this.memory, this.pool, this.types, this.frames.map(() => new FrameCodeTable([])), new Uint16Array(65536), this.spots, { bridge: () => 82, fencemark: () => 90, windArcher: () => 140 });
            new RawAiBridgePlanning(candidates, this.random, () => this.now, undefined).delayRetry(current);
        };
    }

    // 같은 실제 메모리와 현재 관문을 제품의 네 전략에 전달한다. (2026-11-09 WB04-E178)
    make() {
        return new RawAiBridgeStrategies(this.memory, this.pool, this.types, this.random, {
            now: () => this.now, mapLimit: () => this.mapLimit, invalidGraph: () => 254, checking: () => this.cfg[2] !== 0n, disabled: () => this.cfg[3] !== 0n, geyserLog: () => this.logKind
        }, this.hooks);
    }

    // 실제 목록 네 개의 미사용 꼬리까지 순서대로 비교할 hex 이다. (2026-11-09 WB04-E178)
    listsHex() { return this.lists.map((list) => hex(this.memory.bytes(list))).join(''); }

    // 도달한 현재 하위 호출들을 원문과 같은 구분자로 나타낸다. (2026-11-09 WB04-E178)
    traceText() { return this.trace.join(';'); }
}

test('AI 다리 전략: 실제 PE 198행의 네 case 전체 본체/목록/난수/하위 호출 순서를 대조한다', () => {
    assert.equal(rows.length, 198);
    // 원본 입력만 설치하고 전체 제품 몸체를 한 번 호출한다.
    rows.forEach((row, index) => {
        assert.equal(row.length, 9);
        const scene = new StrategyScene(row);
        scene.make().update(scene.token);
        assert.equal(hex(scene.memory.bytes(scene.token)), row[5], `전략 본체 행 ${index}`);
        assert.equal(scene.listsHex(), row[6], `전략 목록 행 ${index}`);
        assert.equal(scene.random.state, Number(row[7]), `전략 난수 행 ${index}`);
        assert.equal(scene.traceText(), row[8], `전략 호출 순서 행 ${index}`);
    });
});

test('AI 다리 전략: checking 이 거짓이면 disabled 를 읽지 않고 재고 효과 뒤 현재 목표/좌표를 다시 쓴다', () => {
    const scene = new StrategyScene(rows[5]);
    let disabled = 0;
    scene.hooks.refresh = () => { put(scene.memory.bytes(scene.token), 0x2d4, 4); };
    scene.hooks.rectangle = () => {
        put(scene.memory.bytes(scene.token), 0xaf0, floatToBits(111));
        put(scene.memory.bytes(scene.token), 0xaf4, floatToBits(112));
        return true;
    };
    scene.hooks.advance = (token, point) => { assert.ok(point.x === 111 && point.y === 112); return false; };
    const strategy = new RawAiBridgeStrategies(scene.memory, scene.pool, scene.types, scene.random, {
        now: () => scene.now, mapLimit: () => scene.mapLimit, invalidGraph: () => 254, checking: () => false, disabled: () => { disabled += 1; return true; }, geyserLog: () => scene.logKind
    }, scene.hooks);
    strategy.update(scene.token);
    assert.ok(!disabled && get(scene.memory.bytes(scene.token), 0xaf0) === 0xc61c4000 && get(scene.memory.bytes(scene.token), 0xaf4) === 0xc61c4000);
});

test('AI 다리 전략: graph 하위가 대상을 바꾸면 중심도 변경 뒤 현재 SID 에서 다시 읽는다', () => {
    const scene = new StrategyScene(rows[5]);
    let advanced = false;
    scene.hooks.graph = (sid) => { assert.equal(sid, 51); put(scene.memory.bytes(scene.token), 0x3cc, 52); return 8; };
    scene.hooks.advance = (token, point) => { assert.ok(point.x === 125 && point.y === 124); advanced = true; return true; };
    scene.make().update(scene.token);
    assert.ok(advanced && scene.traceText() === 'technology,2;refresh,50;center,52');
});

test('AI 다리 전략: 연장 결과가 시작점과 같으면 확장 잔여 횟수를 소비하지 않는다', () => {
    const scene = new StrategyScene(rows[13]);
    put(scene.memory.bytes(scene.token), 0x2f0, 0);
    scene.cfg[10] = scene.cfg[8];
    scene.cfg[11] = scene.cfg[9];
    const before = get(scene.memory.bytes(scene.token), 0x3c8);
    scene.make().update(scene.token);
    assert.ok(get(scene.memory.bytes(scene.token), 0x3c8) === before && get(scene.memory.bytes(scene.token), 0x3d0) === scene.value(8) && get(scene.memory.bytes(scene.token), 0x3d4) === scene.value(9));
    assert.ok(scene.traceText().includes('extend,'));
});

test('AI 다리 전략: 기한이 NaN 이면 네 전략 모두 현재 재고 갱신까지만 하고 선택/배치를 실행하지 않는다', () => {
    // 각 전략의 현재 기한 필드만 quiet NaN 으로 공급한다.
    for (let goal = 1; goal <= 4; goal += 1) {
        const scene = new StrategyScene(rows[goal * 4 + 1]);
        const at = goal <= 2 ? 0x5a8 : 0x5b0;
        put(scene.memory.bytes(scene.token), at, 0);
        put(scene.memory.bytes(scene.token), at + 4, 0x7ff80000);
        scene.make().update(scene.token);
        assert.equal(scene.traceText(), 'technology,2;refresh,50');
    }
});

test('AI 다리 전략: 도달한 하위가 연결되지 않으면 그 위치에서 거부하고 앞선 재고 효과를 취소하지 않는다', () => {
    const scene = new StrategyScene(rows[5]);
    scene.hooks.advance = undefined;
    assert.ok(throws(() => scene.make().update(scene.token)));
    assert.equal(scene.traceText(), 'technology,2;refresh,50;graph,51;center,51');
    const missing = new StrategyScene(rows[5]);
    missing.hooks.technology = undefined;
    assert.ok(throws(() => missing.make().update(missing.token)) && missing.trace.length === 0);
});

test('AI 다리 전략 생성자: 본체 소유자/PATCH 188 월드/난수/현재 전역 조회기가 없으면 거부한다', () => {
    const scene = new StrategyScene(rows[0]);
    const globals = { now: () => 0, mapLimit: () => 256, geyserLog: () => 29, invalidGraph: () => 254, checking: () => false, disabled: () => false };
    const make = (memory, pool, types, random, g) => () => new RawAiBridgeStrategies(memory, pool, types, random, g, scene.hooks);
    assert.ok(throws(make(undefined, scene.pool, scene.types, scene.random, globals)));
    assert.ok(throws(make(scene.memory, {}, scene.types, scene.random, globals)));
    assert.ok(throws(make(scene.memory, scene.pool, scene.types.slice(1), scene.random, globals)));
    assert.ok(throws(make(scene.memory, scene.pool, scene.types, {}, globals)));
    for (const name of ['now', 'mapLimit', 'geyserLog']) assert.ok(throws(make(scene.memory, scene.pool, scene.types, scene.random, { ...globals, [name]: undefined })));
    assert.equal(scene.make().pool, scene.pool);
    assert.ok(throws(() => scene.make().update(scene.memory.allocate(16))));
});
