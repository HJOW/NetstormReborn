// AI 건물 배치 표적 점수 raw-ai-drop-targets.js(RawAiDropTargets)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 디컴파일에서 누락된 점수·실제 조회 순서·첫 SID 출력의 경계를 직접 단언한다). 같은 입력과 단언을 옮겼다: 정수 기본/사제/건물/group6/제단 가산, hpPerSec 합산 뒤 ftol 과 NaN 의 하위 DWORD 0,
//    포획 사제 400점은 운반 genus 에서만 조회하고 현재 rate, 모든 가산의 결합, 두 목록은 각 첫 일치에만 100(하위 효과 뒤 현재 목록), 집중 대상의 HP 선저장과 signed 최대 HP/2(0 방향), 적 owner 전체 DWORD 비교와 음수 점수 절반(0 방향),
//    32비트 가산의 하위 DWORD 감김, 물리 SID 0 운반체 진단 후 계속, Gun 결과를 점수 계산 전에 쓰고 0 으로 덮어씀, 블로커 총점과 첫 항목 출력·양의 최고 부재 시 출력 보존, 방향 블로커 필터(점수 뒤 두 축 −1, 현재 전역 TYPE 에만), 홀수 방향과 세로 동률.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E172: AI 배치 표적 점수 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiDropTargets } from '../www/core/raw-ai-drop-targets.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E172)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const F = Math.fround;

// 실제 본체/목록/슬롯에 명시 입력을 넣고 실제 하위의 호출 경계를 관찰한다. (2026-11-09 WB04-E172)
class AiDropScene {
    // 기본은 적 owner 2 의 평범한 TYPE 80/81 표적과 비어 있는 원본 두 목록이다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x50000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.first = this.memory.allocate(32);
        this.second = this.memory.allocate(32);
        this.pool = new SidPool(32768, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0, group: 0 }));
        this.target = 5;
        this.other = 6;
        this.altar = 164;
        this.directional = 142;
        this.rates = new Array(188).fill(0);
        this.maximum = 1000;
        this.captive = 0;
        this.found = 0;
        this.candidates = [];
        this.center = { x: 20, y: 29 };
        this.trace = [];
        // 실제 할당 순서와 지정 슬롯 입력만 준비한다.
        for (let id = 5; id <= 9; id += 1) {
            assert.equal(this.pool.allocate(2), id);
            const raw = this.pool.allocated(id);
            raw.fill(0);
            raw[10] = 75 + id;
            raw[34] = 2;
            put(raw, 26, 1000);
        }
        this.field(0x36c, this.first);
        this.field(0x378, this.second);
        this.field(0x260, 1);
        this.field(0x264, 2);
        this.hooks = {
            captive: (actor) => { this.trace.push(`captive:${actor}`); return this.captive; },
            rate: (actor) => { this.trace.push(`rate:${actor}`); return this.rates[this.pool.slot(actor)[10]]; },
            maxHitPoints: (actor) => { this.trace.push(`max:${actor}`); return this.maximum; },
            gun: (x, y, type, direction, owner) => { assert.ok(x === 20 && y === 30 && type === 128 && direction === 2 && owner === 1); this.trace.push('gun'); return this.found; },
            blockers: (owner, point, capacity) => { assert.ok(owner === 1 && point.x === 20 && point.y === 30 && capacity === 30); this.trace.push('blockers'); return this.candidates; },
            center: (actor) => { this.trace.push(`center:${actor}`); return this.center; },
            assertion: (text, line) => { assert.ok(text === 'sid' && line === 0xaeb); this.trace.push('assert'); }
        };
    }

    // 지정 본체 DWORD 만 입력한다. (2026-11-09 WB04-E172)
    field(at, value) { put(this.memory.bytes(this.token), at, value >>> 0); }

    // 지정 목록의 실제 backing 에 중복을 그대로 입력한다. (2026-11-09 WB04-E172)
    list(pointer, values) {
        this.field(pointer + 8, values.length);
        const raw = this.memory.bytes(get(this.memory.bytes(this.token), pointer));
        // 입력 등장 순서를 보존한다.
        values.forEach((value, index) => put(raw, index * 4, value));
    }

    // 같은 실제 저장소에 처리기를 연결하며 호출 자체는 각 검사에서 선택한다. (2026-11-09 WB04-E172)
    actions() { return new RawAiDropTargets(this.memory, this.pool, this.types, { altarType: () => this.altar, directionalType: () => this.directional }, this.hooks); }

    // 현재 TYPE/호출 하위의 점수를 읽는다. (2026-11-09 WB04-E172)
    score() {
        const actions = this.actions();
        assert.equal(actions.pool, this.pool);
        return actions.score(this.token, this.target);
    }
}

test('AI 배치 표적: 원본 정수 기본/사제/건물/group6/제단 가산을 각각 검사한다', () => {
    const scene = new AiDropScene();
    assert.equal(scene.score(), 10);
    scene.types[80].flags2 = 0x4000000;
    assert.equal(scene.score(), 5);
    scene.types[80].flags2 = 0x200000;
    assert.equal(scene.score(), 210);
    scene.field(0x2a8, 9);
    assert.equal(scene.score(), 15);
    scene.types[80].flags2 = 0x200;
    assert.equal(scene.score(), 16);
    scene.types[80].flags2 = 0;
    scene.types[80].group = 6;
    assert.equal(scene.score(), 17);
    scene.types[80].group = 0;
    scene.altar = 80;
    assert.equal(scene.score(), 18);
});

test('AI 배치 표적: 정수 가산과 현재 hpPerSec 를 합한 다음 ftol 하며 NaN 의 하위 DWORD 0 을 보존한다', () => {
    const scene = new AiDropScene();
    scene.rates[80] = F(-0.9);
    assert.equal(scene.score(), 9);
    scene.rates[80] = Number.NaN;
    assert.equal(scene.score(), 0);
    scene.rates[80] = 1.75;
    assert.equal(scene.score(), 11);
});

test('AI 배치 표적: 포획 사제 400점은 운반 genus 30000 에서만 조회하며 앞선 flags/group 은 보존한다', () => {
    const scene = new AiDropScene();
    scene.types[80].flags2 = 0x10000;
    scene.types[80].group = 6;
    scene.rates[81] = 23;
    scene.hooks.captive = (actor) => { assert.equal(actor, scene.target); scene.trace.push('captive'); scene.pool.allocated(actor)[10] = 81; return scene.other; };
    assert.equal(scene.score(), 440);
    assert.deepEqual(scene.trace, ['captive', 'rate:5']);
    scene.trace.length = 0;
    assert.ok(scene.score() === 33 && JSON.stringify(scene.trace) === '["rate:5"]');
});

test('AI 배치 표적: 모든 실제 가산을 한 번씩 더하고 제단 TYPE 는 현재 전역을 사용한다', () => {
    const scene = new AiDropScene();
    scene.types[80].flags2 = 0x4000000 | 0x200000 | 0x200 | 0x10000;
    scene.types[80].group = 6;
    scene.altar = 80;
    scene.captive = scene.other;
    scene.rates[80] = 1.75;
    assert.equal(scene.score(), 627);
});

test('AI 배치 표적: 두 목록은 각 첫 일치에만 100 을 더하며 하위 효과 뒤 현재 목록을 읽는다', () => {
    const scene = new AiDropScene();
    scene.list(0x36c, [scene.target, scene.target]);
    scene.list(0x378, [scene.target, scene.target]);
    assert.equal(scene.score(), 210);
    scene.hooks.rate = (actor) => { assert.equal(actor, scene.target); scene.field(0x374, 0); return 0; };
    assert.equal(scene.score(), 110);
    scene.field(0x380, 0xffffffff);
    assert.equal(scene.score(), 10);
});

test('AI 배치 표적: 집중 대상의 HP 는 getter 전 저장하고 signed 최대 HP/2 는 0 쪽 나눗셈이다', () => {
    const scene = new AiDropScene();
    scene.field(0x3f8, scene.target);
    put(scene.pool.allocated(scene.target), 26, 0xffffffff);
    scene.hooks.maxHitPoints = (actor) => { assert.equal(actor, scene.target); scene.trace.push('max'); put(scene.pool.allocated(actor), 26, 0xfffffff6); return -3; };
    assert.ok(scene.score() === 60 && JSON.stringify(scene.trace) === '["rate:5","max"]');
});

test('AI 배치 표적: 마지막 owner BYTE 는 전체 enemy DWORD 와 비교하고 음수 점수도 0 쪽으로 절반을 낸다', () => {
    const scene = new AiDropScene();
    scene.rates[80] = -21;
    assert.equal(scene.score(), -11);
    scene.field(0x264, 0x102);
    assert.equal(scene.score(), -5);
    scene.field(0x264, 2);
    scene.field(0x3f8, scene.target);
    scene.hooks.maxHitPoints = (actor) => { scene.pool.allocated(actor)[34] = 3; scene.field(0x264, 3); return 99999; };
    assert.equal(scene.score(), -11);
});

test('AI 배치 표적: 32비트 가산은 signed overflow 대신 원본 하위 DWORD 감김을 유지한다', () => {
    const scene = new AiDropScene();
    scene.rates[80] = 2147483520;
    scene.list(0x36c, [scene.target]);
    scene.list(0x378, [scene.target]);
    assert.equal(scene.score(), -2147483566);
});

test('AI 배치 표적: 물리 SID 0 의 운반체 진단은 선택된 곳에서 호출하고 실제 내용물 조회를 계속한다', () => {
    const scene = new AiDropScene();
    const zeroType = scene.pool.slot(0)[10];
    scene.types[zeroType].flags2 = 0x10000;
    scene.field(0x3f8, 9);
    scene.actions().score(scene.token, 0);
    assert.deepEqual(scene.trace, ['assert', 'captive:0', 'rate:0']);
});

test('AI 배치 표적: 실제 검색 결과는 점수 계산보다 먼저 쓰고 빈 검색도 0 으로 덮어쓴다', () => {
    const scene = new AiDropScene();
    const result = { value: 9 };
    assert.ok(scene.actions().gun(scene.token, 128, { x: 20, y: 30 }, 2, result) === 0 && !result.value);
    scene.found = scene.target;
    scene.hooks.rate = (actor) => { assert.ok(actor === scene.target && result.value === scene.target); return 5; };
    assert.ok(scene.actions().gun(scene.token, 128, { x: 20, y: 30 }, 2, result) === 15 && result.value === scene.target);
    scene.hooks.rate = undefined;
    assert.ok(throws(() => scene.actions().gun(scene.token, 128, { x: 20, y: 30 }, 2, result)) && result.value === scene.target);
    // 별도 지상 검색도 같은 쓰기 순서이다.
    const ground = new AiDropScene();
    ground.hooks.ground = (x, y, type, owner) => { assert.ok(x === 20 && y === 30 && type === 90 && owner === 1); ground.trace.push('ground'); return ground.target; };
    const output = { value: 0 };
    assert.ok(ground.actions().ground(ground.token, 90, { x: 20, y: 30 }, output) === 10 && output.value === ground.target && JSON.stringify(ground.trace) === '["ground","rate:5"]');
});

test('AI 배치 표적: 최고 점수의 항목이 둘째여도 원본은 첫 SID 를 출력하며 중복 총점은 모두 센다', () => {
    const scene = new AiDropScene();
    scene.rates[80] = -10;
    scene.rates[81] = 20;
    scene.candidates = [5, 6, 6];
    const result = { value: 9 };
    assert.ok(scene.actions().blockers(scene.token, 78, { x: 20, y: 30 }, 2, result) === 60 && result.value === scene.target);
    assert.deepEqual(scene.trace, ['blockers', 'rate:5', 'rate:6', 'rate:6']);
});

test('AI 배치 표적: 양의 최고가 없으면 출력은 보존하고 음수/0 도 총점에 포함한다', () => {
    const scene = new AiDropScene();
    scene.rates[80] = -11;
    scene.candidates = [5, 5];
    const result = { value: 9 };
    assert.ok(scene.actions().blockers(scene.token, 78, { x: 20, y: 30 }, 2, result) === -2 && result.value === 9);
    scene.rates[80] = 2147483520;
    assert.ok(scene.actions().blockers(scene.token, 78, { x: 20, y: 30 }, 2, result) === -236 && result.value === scene.target);
    scene.candidates = [];
    result.value = 9;
    assert.ok(scene.actions().blockers(scene.token, 78, { x: 20, y: 30 }, 2, result) === 0 && result.value === 9);
});

test('AI 배치 표적: 방향 블로커 필터는 점수 뒤 두 축 −1 을 저장하고 현재 전역 TYPE 에만 적용한다', () => {
    const scene = new AiDropScene();
    scene.candidates = [5];
    scene.center = { x: 19, y: F(29.5) };
    const result = { value: 9 };
    assert.ok(scene.actions().blockers(scene.token, 142, { x: 20, y: 30 }, 4, result) === 10 && result.value === scene.target);
    assert.deepEqual(scene.trace, ['blockers', 'rate:5', 'center:5']);
    scene.trace.length = 0;
    result.value = 9;
    assert.ok(scene.actions().blockers(scene.token, 142, { x: 20, y: 30 }, 2, result) === 0 && result.value === 9);
    scene.types[80].flags2 = 0x100000;
    scene.trace.length = 0;
    assert.equal(scene.actions().blockers(scene.token, 142, { x: 20, y: 30 }, 0, result), 0);
    assert.deepEqual(scene.trace, ['blockers', 'rate:5']);
    scene.types[80].flags2 = 0;
    scene.directional = 78;
    scene.trace.length = 0;
    assert.ok(scene.actions().blockers(scene.token, 142, { x: 20, y: 30 }, 2, result) === 10 && JSON.stringify(scene.trace) === '["blockers","rate:5"]');
});

test('AI 배치 표적: 홀수 방향도 원문대로 중심을 먼저 읽고 제외하며 대각 동률은 세로다', () => {
    const scene = new AiDropScene();
    scene.center = { x: 21, y: 29 };
    assert.ok(scene.actions().facing({ x: 20, y: 30 }, 0, scene.target));
    scene.trace.length = 0;
    assert.ok(!scene.actions().facing({ x: 20, y: 30 }, 1, scene.target));
    assert.deepEqual(scene.trace, ['center:5']);
});

test('AI 배치 표적 생성자: 본체 소유자/같은 PATCH 풀/TYPE 표/현재 전역 조회기가 없으면 거부하고 가짜 본체/표 밖 TYPE 을 거부한다', () => {
    const scene = new AiDropScene();
    const globals = { altarType: () => 164, directionalType: () => 142 };
    assert.ok(throws(() => new RawAiDropTargets(undefined, scene.pool, scene.types, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropTargets(scene.memory, {}, scene.types, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropTargets(scene.memory, scene.pool, undefined, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropTargets(scene.memory, scene.pool, scene.types, { altarType: () => 164 }, scene.hooks)));
    const small = scene.memory.allocate(16);
    assert.ok(throws(() => scene.actions().score(small, scene.target)));
    scene.types.length = 70;
    assert.ok(throws(() => scene.actions().score(scene.token, scene.target)));
});
