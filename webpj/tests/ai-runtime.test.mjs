// AI 전체 행동/개별 Tick/Player 1..8 순회·수명 raw-ai-runtime.js(RawAiRuntime)를 실제 PE 관측 145행과 cpppj 계약 시험으로 검사한다.
//  - fixture ai-runtime-x86.tsv: 열 = 번호, 진입(0 actions·1 tick·2 frame), 명시 하위 입력 10개(QWORD), 본체 두 개 입력 hex, Player 9기록 입력 hex, 정규화한 본체 출력, Player 출력, 해제 mask, Tick 반환, 도달한 하위/인자 순서.
//    행마다 입력만 설치하고 반복 횟수만큼 진입을 호출한 뒤 본체/Player/해제 mask/반환/순서를 모두 비교한다(반납한 본체는 부재 0 으로 정규화하고 해제 메모리 바이트를 주장하지 않는다).
//  - cpppj 계약 시험(254 종료는 초기화/시계/행동 없이 현재 Player 본체 소멸 → 반납 → 0 쓰기만 수행)과 본체 검증/누락 하위/비전투/초기화 미완료/생성자 거부의 추가 계약을 옮겼다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E165: AI 런타임 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat } from './raw-support.mjs';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiRuntime } from '../www/core/raw-ai-runtime.js';

// 실제 PE 관측 행 수, 열 수, 첫 소유 핸들이다. (2026-11-09 WB04-E165)
const ROWS = 145;
const COLUMNS = 10;
const BASE = 0x10000000;
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'ai-runtime-x86.tsv')) ?? [];

// 예외가 나는지 본다. (2026-11-09 WB04-E165)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E165)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 64비트 QWORD 비트를 double 로 해석한다(손실 없이). (2026-11-09 WB04-E165)
function doubleOf(bits) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, BigInt.asUintN(64, bits), true);
    return view.getFloat64(0, true);
}

// double 의 실제 64비트 표현을 십진 문자열로 만든다(원본 trace 표현). (2026-11-09 WB04-E165)
function doubleBits(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true).toString();
}

// 본체/Player/명시 효과 입력만 설치하고 실제 프레임 결과는 모듈이 계산하도록 하는 장면이다. (2026-11-09 WB04-E165)
class RuntimeScene {
    // 독립 fixture 의 입력만 복사하며 기대 본체/순서를 설치하지 않는다.
    constructor(row) {
        this.memory = new RawAiBrainMemory(BASE, (bytes) => bytes.fill(0));
        this.tokens = [this.memory.allocate(AI_BRAIN_BYTES), this.memory.allocate(AI_BRAIN_BYTES)];
        this.cfg = row[2].split(',').map((value) => BigInt(value));
        const bodies = Buffer.from(row[3], 'hex');
        this.players = new Uint8Array(Buffer.from(row[4], 'hex'));
        // 같은 실제 본체 순서로 두 개를 설치한다.
        this.tokens.forEach((token, index) => this.memory.bytes(token).set(bodies.subarray(index * AI_BRAIN_BYTES, (index + 1) * AI_BRAIN_BYTES)));
        this.trace = [];
        this.timing = Number(this.cfg[1]) >>> 0;
        this.dryLog = 99;
        this.active = this.tokens[0];
        this.freed = 0;
        this.walls = 0;
        this.hooks = {};
        this.bind();
    }

    // 현재 하위 진입을 기록하고 현재 token 을 후속 시계/SP 경계에 유지한다. (2026-11-09 WB04-E165)
    event(name, token) {
        this.active = token;
        this.trace.push(`${name},${token}`);
    }

    // 원본 도구와 같은 명시 효과만 공급한다(전체 분기/비교/순회는 모듈이 수행한다). (2026-11-09 WB04-E165)
    bind() {
        const flags = Number(this.cfg[7]);
        const low = (value) => Number(value & 0xffffffffn);
        this.hooks = {
            initialize: (token) => { this.event('initialize', token); put(this.memory.bytes(token), 0x258, low(this.cfg[2])); },
            prepare: (token) => { this.event('prepare', token); if (flags & 1) put(this.memory.bytes(token), 0x258, 0); },
            selectGoal: (token) => this.event('select', token),
            collectors: (token) => { this.event('collectors', token); if (flags & 2) put(this.memory.bytes(token), 0x29c, 37); },
            factories: (token) => this.event('factories', token),
            dryScan: (token) => { this.event('dry', token); if (flags & 4) put(this.memory.bytes(token), 0x280, low(this.cfg[3])); return low(this.cfg[3]); },
            dryNotice: (token, kind, fake) => this.trace.push(`dryNotice,${token},${kind},${fake ? '1' : '0'}`),
            priest: (token, priest) => { this.event('priest', token); this.trace[this.trace.length - 1] += `,${priest}`; if (flags & 8) put(this.memory.bytes(token), 0x260, 3); },
            drops: (token) => { this.event('drops', token); if (flags & 64) put(this.memory.bytes(token), 0x254, 1); },
            bridges: (token) => { this.event('bridges', token); if (flags & 128) this.timing = 1; },
            carriers: (token) => this.event('carriers', token),
            economy: (token) => { this.event('economy', token); if (flags & 16) put(this.memory.bytes(token), 0x2d8, low(this.cfg[8])); },
            money: (owner) => {
                this.trace.push(`money,${owner}`);
                if (flags & 32) put(this.memory.bytes(this.active), 0x2d8, 0);
                return bitsToFloat(low(this.cfg[4]));
            },
            wall: () => {
                this.trace.push(`wall,${this.walls}`);
                const value = this.cfg[this.walls % 2 === 0 ? 5 : 6];
                this.walls += 1;
                return doubleOf(value);
            },
            took: (token, elapsed) => this.trace.push(`took,${token},${doubleBits(elapsed)}`),
            assertion: (text, line) => this.trace.push(`assert,${line}`),
            destruct: (token) => this.trace.push(`destruct,${token}`),
            free: (token) => {
                this.trace.push(`free,${token}`);
                this.tokens.forEach((value, index) => { if (value === token) this.freed |= 1 << index; });
                this.memory.free(token);
            }
        };
    }

    // 같은 현재 Player/본체와 동적 타이밍 전역을 전달한다. (2026-11-09 WB04-E165)
    make() {
        return new RawAiRuntime(this.memory, this.players, { battle: () => this.cfg[0] !== 0n, timing: () => this.timing, dryLog: () => this.dryLog }, this.hooks);
    }

    // 반납한 본체는 부재 0 으로 정규화하고 해제 메모리의 바이트 일치를 주장하지 않는다. (2026-11-09 WB04-E165)
    bodiesHex() {
        return this.tokens.map((token) => (this.memory.contains(token) ? hex(this.memory.bytes(token)) : '0'.repeat(AI_BRAIN_BYTES * 2))).join('');
    }
}

test('AI 런타임 fixture: 실제 PE 145행의 본체/Player/float 지갑/동적 기한 로그/현재 필드/수명/순서를 모두 대조한다', () => {
    assert.equal(rows.length, ROWS);
    // 원본 caller 입력만 모듈에 공급하고 같은 현재 저장소로 재개한다.
    rows.forEach((row, index) => {
        assert.equal(row.length, COLUMNS);
        const scene = new RuntimeScene(row);
        const runtime = scene.make();
        const entry = Number(row[1]);
        let alive = false;
        // 원본과 같은 진입과 반복 호출이다.
        for (let repeat = 0n; repeat < scene.cfg[9]; repeat += 1n) {
            if (entry === 0) runtime.actions(scene.tokens[0]);
            else if (entry === 1) alive = runtime.tick(scene.tokens[0]);
            else runtime.frame();
        }
        const label = `행 ${index}`;
        assert.equal(scene.bodiesHex(), row[5], `${label} 본체`);
        assert.equal(hex(scene.players), row[6], `${label} Player`);
        assert.equal(scene.freed, Number(row[7]), `${label} 수명`);
        if (entry === 1) assert.equal(alive, Number(row[8]) !== 0, `${label} Tick 반환`);
        assert.equal(scene.trace.join(';'), row[9], `${label} 순서`);
    });
});

test('AI 런타임: 254 종료는 초기화/시계/행동 없이 현재 Player 본체 소멸 → 반납 → 0 쓰기만 수행한다', () => {
    const scene = new RuntimeScene(rows[0]);
    scene.hooks = {};
    put(scene.memory.bytes(scene.tokens[0]), 0x254, 1);
    put(scene.memory.bytes(scene.tokens[1]), 0x254, 1);
    const flag = scene.players[180 + 0x50];
    scene.hooks.destruct = (token) => scene.trace.push(`D${token}`);
    scene.hooks.free = (token) => { scene.trace.push(`F${token}`); scene.memory.free(token); };
    scene.make().frame();
    assert.ok(scene.trace.length === 4 && !get(scene.players, 180 + 0xb0) && !get(scene.players, 8 * 180 + 0xb0) && scene.players[180 + 0x50] === flag && get(scene.players, 0xb0) === scene.tokens[0]);
});

test('AI 런타임: 본체 검증은 어떤 효과보다 먼저이며 누락 하위는 도달한 위치에서 거부하고 부분 효과를 보존한다', () => {
    const scene = new RuntimeScene(rows[0]);
    const runtime = scene.make();
    // 해제/가짜/잘못된 크기 본체는 hook 호출 전에 거부된다.
    const fake = scene.memory.allocate(16);
    assert.ok(throws(() => runtime.actions(fake)) && throws(() => runtime.actions(0x1234)) && throws(() => runtime.tick(fake)));
    assert.deepEqual(scene.trace, []);
    // 수집자와 시설만 있으면 부족 좌표 위치에서 멈추고 앞선 효과는 남는다.
    const partial = new RuntimeScene(rows[0]);
    partial.hooks = { collectors: partial.hooks.collectors, factories: partial.hooks.factories };
    assert.ok(throws(() => partial.make().actions(partial.tokens[0])));
    assert.deepEqual(partial.trace, [`collectors,${partial.tokens[0]}`, `factories,${partial.tokens[0]}`]);
    // 느린 프레임을 알릴 took 이 없으면 행동이 끝난 뒤 그 위치에서 거부된다(본체의 행동 결과는 유지).
    const slow = new RuntimeScene(rows.find((row) => Number(row[1]) === 1 && Number(row[8]) !== 0));
    slow.cfg[1] = 1n;
    slow.timing = 1;
    slow.cfg[5] = 0n;
    slow.cfg[6] = BigInt(doubleBits(0.5));
    slow.hooks.took = undefined;
    assert.ok(throws(() => slow.make().tick(slow.tokens[0])));
    assert.ok(slow.trace.includes(`economy,${slow.tokens[0]}`) && slow.trace.filter((value) => value.startsWith('wall,')).length === 2);
});

test('AI 런타임: 느린 프레임은 현재 timing 전역이 켜져 있고 0.1초를 엄격히 넘을 때만 알린다', () => {
    const base = rows.find((row) => Number(row[1]) === 1 && Number(row[8]) !== 0);
    const run = (timing, elapsed) => {
        const scene = new RuntimeScene(base);
        scene.timing = timing;
        scene.cfg[1] = BigInt(timing);
        scene.cfg[5] = 0n;
        scene.cfg[6] = BigInt(doubleBits(elapsed));
        // 행동 중 timing 을 바꾸지 않도록 해당 플래그를 끈다.
        scene.cfg[7] &= ~128n;
        scene.cfg[9] = 1n;
        // 초기화가 끝난 상태에서 본체를 준비한다(기존 입력은 그대로 두고 +0x254/+0x258 만 확인).
        put(scene.memory.bytes(scene.tokens[0]), 0x254, 0);
        put(scene.memory.bytes(scene.tokens[0]), 0x258, 1);
        scene.bind();
        const result = scene.make().tick(scene.tokens[0]);
        return { result, took: scene.trace.filter((value) => value.startsWith('took,')) };
    };
    assert.deepEqual(run(0, 5), { result: true, took: [] });
    assert.deepEqual(run(1, 0.1), { result: true, took: [] });
    assert.equal(run(1, 0.1000001).took.length, 1);
    assert.equal(run(1, 5).took.length, 1);
});

test('AI 런타임: 전투가 아니면 어떤 Player 도 방문하지 않고 초기화가 끝나지 않은 Tick 은 본체를 유지한 채 true 이다', () => {
    const scene = new RuntimeScene(rows[0]);
    scene.cfg[0] = 0n;
    const before = hex(scene.players);
    scene.make().frame();
    assert.ok(scene.trace.length === 0 && hex(scene.players) === before);
    // 초기화 hook 이 +0x258 을 채우지 않으면 시계/준비/행동 없이 true 이다.
    const waiting = new RuntimeScene(rows[0]);
    put(waiting.memory.bytes(waiting.tokens[0]), 0x254, 0);
    put(waiting.memory.bytes(waiting.tokens[0]), 0x258, 0);
    waiting.cfg[2] = 0n;
    waiting.bind();
    assert.equal(waiting.make().tick(waiting.tokens[0]), true);
    assert.deepEqual(waiting.trace, [`initialize,${waiting.tokens[0]}`]);
});

test('AI 런타임 생성자: 실제 9 Player/본체 소유자/현재 전역 조회기가 없으면 거부한다', () => {
    const scene = new RuntimeScene(rows[0]);
    const globals = { battle: () => true, timing: () => 0, dryLog: () => 0 };
    assert.ok(throws(() => new RawAiRuntime(scene.memory, new Uint8Array(1619), globals, scene.hooks)));
    assert.ok(throws(() => new RawAiRuntime(undefined, scene.players, globals, scene.hooks)));
    for (const name of ['battle', 'timing', 'dryLog']) assert.ok(throws(() => new RawAiRuntime(scene.memory, scene.players, { ...globals, [name]: undefined }, scene.hooks)));
    assert.ok(!throws(() => new RawAiRuntime(scene.memory, scene.players, globals, {})));
});
