// AI 건설 선택 raw-ai-build-choice.js(RawAiBuildChoice)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 수집자 cost 의 곱셈 전 절삭과 하위 DWORD 곱 감김·현재 전역 TYPE, 로컬 확정 SP 의 현재 float 절삭/미확정 0/원격 signed 지갑,
//    포획 다이스 우선과 단락 순서·효과 뒤 현재 적/다이스 재조회, 신전 없는 다이스의 적 자금과 원소 신전 비용 경계, 0..3/4·5/진단과 signed%5, 무신전 theme 3 과 NULL 후보, 지갑/여유 비용 관문의 분리,
//    중복 선호의 같은 지역 복사 수량 감소와 실제 본체 수량 보존, 다섯 번째 수량(TYPE 0)과 음수 감김, 수량 복사 순서와 손상된 임시 인덱스 거부, 선택된 자금 하위 누락의 앞선 효과 보존.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E167: AI 건설 선택 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put } from './raw-support.mjs';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiBuildChoice } from '../www/core/raw-ai-build-choice.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E167)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 같은 실제 본체/현재 TYPE/전역과 원본 입력과 현재 상태를 읽는 명시 하위 효과만 준비한 장면이다. (2026-11-09 WB04-E167)
class AiChoiceScene {
    // 기본은 현재 신전/자금 600/수집자 2개분 여유/부족한 sun 워크샾이다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x4e000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.types = Array.from({ length: 188 }, () => ({ cost: 0 }));
        this.local = 1;
        this.known = 1;
        this.collector = 133;
        this.dais = 165;
        this.workshops = [86, 87, 88, 85];
        this.temples = [123, 124, 125];
        this.captured = false;
        this.dead = false;
        this.afford = true;
        this.remote = 999;
        this.stored = Math.fround(999.9);
        this.trace = [];
        this.types[133].cost = Math.fround(100.9);
        this.types[165].cost = 200;
        this.types[123].cost = Math.fround(300.9);
        this.types[124].cost = 400;
        this.types[125].cost = 500;
        this.types[86].cost = 50;
        this.field(0x264, 2);
        this.field(0x294, 1);
        this.field(0x2d8, 600);
        this.field(0x234, 2);
        this.field(0x394, 86);
        this.preferences([0]);
        this.hooks = {
            captured: (owner) => { this.trace.push(`captured:${owner}`); return this.captured; },
            dead: (owner) => { this.trace.push(`dead:${owner}`); return this.dead; },
            canAfford: (brain, type) => { assert.equal(brain, this.token); this.trace.push(`afford:${type}`); return this.afford; },
            wallet: (owner) => { this.trace.push(`wallet:${owner}`); return this.remote; },
            storedSp: (owner) => { this.trace.push(`sp:${owner}`); return this.stored; },
            assertion: (text, line) => { assert.equal(text, line === 0x1a ? 'theIndex>=0 && theIndex<=(MAX_THEMES)' : '0'); this.trace.push(`assert:${line}`); }
        };
    }

    // 현재 본체 DWORD 만 입력한다. (2026-11-09 WB04-E167)
    field(at, value) { put(this.memory.bytes(this.token), at, value >>> 0); }

    // 현재 TYPE 별 실제 수량을 입력한다. (2026-11-09 WB04-E167)
    count(type, value) { this.field(0x6d4 + type * 4, value); }

    // 같은 본체의 선호 순서를 입력한다. (2026-11-09 WB04-E167)
    preferences(values) {
        // 입력한 선호 순서만 쓴다.
        values.forEach((value, index) => this.field(0x398 + index * 4, value));
        this.field(0x3b0, values.length);
    }

    // 현재 전역은 조회기/배열로 빌리며 TYPE 값을 생성 시 복제하지 않는다. (2026-11-09 WB04-E167)
    make() {
        return new RawAiBuildChoice(this.memory, this.types, {
            localOwner: () => this.local,
            playerKnown: () => this.known,
            collectorType: () => this.collector,
            daisType: () => this.dais,
            workshops: this.workshops,
            temple: (index) => { this.trace.push(`temple:${index}`); return this.temples[index]; }
        }, this.hooks);
    }
}

// 수집자 cost 는 곱하기 전에 절삭하며 하위 DWORD 곱 감김과 현재 전역 TYPE 를 사용한다. (2026-11-09 WB04-E167)
test('AI 건설 선택: 수집자 여유 비용은 cost 를 곱하기 전에 절삭하고 DWORD 곱 감김과 현재 전역 TYPE 을 쓴다', () => {
    const input = new AiChoiceScene();
    input.field(0x2d8, 200);
    assert.ok(input.make().reserveForCollectors(input.token));
    input.field(0x2d8, 199);
    assert.ok(!input.make().reserveForCollectors(input.token));
    input.types[133].cost = Math.fround(0.9);
    input.field(0x234, 1000);
    input.field(0x2d8, 0);
    assert.ok(input.make().reserveForCollectors(input.token));
    input.types[133].cost = 2147483648;
    input.field(0x234, 2);
    assert.ok(input.make().reserveForCollectors(input.token));
    const choice = input.make();
    input.collector = 134;
    input.types[134].cost = Math.fround(20.9);
    input.field(0x2d8, 40);
    assert.ok(choice.reserveForCollectors(input.token));
});

test('AI 건설 선택: 로컬 확정 SP 는 현재 float 절삭, 미확정은 0, 원격은 signed 실제 지갑을 쓴다', () => {
    const input = new AiChoiceScene();
    assert.ok(input.make().enemyLowFunds(input.token) && JSON.stringify(input.trace) === '["wallet:2"]');
    input.remote = 1000;
    assert.ok(!input.make().enemyLowFunds(input.token));
    input.field(0x264, 1);
    input.trace.length = 0;
    assert.ok(input.make().enemyLowFunds(input.token) && JSON.stringify(input.trace) === '["sp:1"]');
    input.stored = Math.fround(1000.9);
    assert.ok(!input.make().enemyLowFunds(input.token));
    input.stored = Number.NaN;
    assert.ok(input.make().enemyLowFunds(input.token));
    input.known = 0;
    input.trace.length = 0;
    input.hooks.storedSp = undefined;
    input.hooks.wallet = undefined;
    assert.ok(input.make().enemyLowFunds(input.token) && input.trace.length === 0);
});

test('AI 건설 선택: 신전이 있으면 적 자금 관문 없이 포획 다이스를 선택하고 뒤의 잘못된 후보 TYPE 을 읽지 않는다', () => {
    const input = new AiChoiceScene();
    input.field(0x248, 0x200);
    input.field(0x394, 200);
    input.captured = true;
    input.hooks.wallet = undefined;
    input.hooks.storedSp = undefined;
    assert.ok(input.make().select(input.token) === 165 && JSON.stringify(input.trace) === '["captured:2","dead:2","afford:165"]');
});

test('AI 건설 선택: 각 사제/비용 효과 뒤 현재 적과 다이스를 다시 읽으며 이미 확인한 관문은 반복하지 않는다', () => {
    const input = new AiChoiceScene();
    input.field(0x248, 0x200);
    input.field(0x294, 0);
    input.hooks.captured = (owner) => { assert.equal(owner, 2); input.trace.push('captured'); input.field(0x264, 3); input.field(0x2a4, 1); return true; };
    input.hooks.dead = (owner) => { assert.equal(owner, 3); input.trace.push('dead'); return false; };
    input.hooks.canAfford = (brain, type) => { assert.ok(brain === input.token && type === 165); input.dais = 166; input.trace.push('afford'); return true; };
    input.hooks.wallet = (owner) => { assert.equal(owner, 3); input.dais = 167; input.trace.push('wallet'); return 999; };
    assert.ok(input.make().select(input.token) === 167 && JSON.stringify(input.trace) === '["captured","dead","afford","wallet"]');
});

test('AI 건설 선택: 이미 다이스가 있거나 포획/생존 관문 탈락이면 후속 효과를 선택하지 않는다', () => {
    const input = new AiChoiceScene();
    input.field(0x248, 0x200);
    input.field(0x2a4, 1);
    input.hooks.captured = undefined;
    assert.ok(input.make().select(input.token) === 86 && input.trace.length === 0);
    input.field(0x2a4, 0);
    input.hooks.captured = () => { input.trace.push('captured'); return false; };
    input.hooks.dead = undefined;
    assert.ok(input.make().select(input.token) === 86 && JSON.stringify(input.trace) === '["captured"]');
    input.trace.length = 0;
    input.captured = true;
    input.hooks.captured = () => { input.trace.push('captured'); return true; };
    input.hooks.dead = () => { input.trace.push('dead'); return true; };
    input.hooks.canAfford = undefined;
    assert.ok(input.make().select(input.token) === 86 && JSON.stringify(input.trace) === '["captured","dead"]');
});

test('AI 건설 선택: 신전 없는 다이스는 적 자금 1000 미만이 필요하며 실패하면 현재 원소 신전으로 가고 비용 경계를 지킨다', () => {
    const input = new AiChoiceScene();
    input.field(0x248, 0x200);
    input.field(0x294, 0);
    input.captured = true;
    assert.equal(input.make().select(input.token), 165);
    input.remote = 1000;
    input.trace.length = 0;
    assert.ok(input.make().select(input.token) === 123 && input.trace.at(-1) === 'temple:0');
    input.field(0x248, 0);
    input.field(0x2d8, 300);
    assert.equal(input.make().select(input.token), 123);
    input.field(0x2d8, 299);
    assert.ok(!input.make().select(input.token));
});

test('AI 건설 선택: 실제 getter 의 0..3/4·5/진단과 signed%5 를 구별하며 5 를 별도 신전으로 해석하지 않는다', () => {
    const input = new AiChoiceScene();
    const choice = input.make();
    assert.ok(choice.workshopType(0) === 86 && choice.workshopType(4) === 0 && choice.workshopType(5) === 0);
    assert.ok(choice.workshopType(-1) === 0 && input.trace.at(-1) === 'assert:26');
    input.workshops[0] = 87;
    assert.equal(choice.workshopType(0), 87);
    input.temples[0] = 126;
    assert.ok(choice.templeType(5) === 126 && choice.templeType(-5) === 126 && choice.templeType(6) === 124 && choice.templeType(7) === 125);
    assert.ok(choice.templeType(8) === 0 && input.trace.at(-1) === 'assert:71');
    assert.ok(choice.templeType(-1) === 0 && input.trace.at(-1) === 'assert:71');
});

test('AI 건설 선택: theme 3 의 무신전은 신전 getter 를 건너뛰고 workshop 후보 0 은 비용/수집자 TYPE 도 읽지 않는다', () => {
    const input = new AiChoiceScene();
    input.field(0x294, 0);
    input.field(0x390, 3);
    input.field(0x394, 0);
    input.collector = 200;
    assert.ok(!input.make().select(input.token) && input.trace.length === 0);
    input.field(0x294, 1);
    input.field(0x394, 86);
    input.field(0x2d8, 49);
    assert.ok(!input.make().select(input.token));
    input.field(0x2d8, 50);
    // 지갑은 후보 비용을 통과하므로 현재 전역 수집자 TYPE 200(물리 188 밖)의 cost 를 읽으려다 거부된다.
    assert.ok(throws(() => input.make().select(input.token)));
});

test('AI 건설 선택: 현재 지갑이 각각의 비용 관문을 통과하면 되며 건설 비용을 여유 비용에 추가하지 않는다', () => {
    const input = new AiChoiceScene();
    input.types[86].cost = 150;
    input.field(0x2d8, 200);
    assert.equal(input.make().select(input.token), 86);
    input.field(0x2d8, 199);
    assert.ok(!input.make().select(input.token));
});

test('AI 건설 선택: 중복 선호마다 같은 지역 복사 수량을 감소시키고 실제 본체 수량은 바꾸지 않는다', () => {
    const input = new AiChoiceScene();
    input.count(86, 1);
    input.count(87, 1);
    input.preferences([0, 1, 0]);
    const before = Buffer.from(input.memory.bytes(input.token));
    assert.ok(input.make().select(input.token) === 86 && Buffer.from(input.memory.bytes(input.token)).equals(before));
    input.preferences([0, 1]);
    assert.ok(!input.make().select(input.token));
    input.preferences([1, 2]);
    assert.equal(input.make().select(input.token), 88);
});

test('AI 건설 선택: 다섯 번째 수량은 TYPE 0 이며 음수 감김도 원본 DWORD 감소 뒤 signed 판정을 따른다', () => {
    const input = new AiChoiceScene();
    input.preferences([4, 0]);
    assert.ok(!input.make().select(input.token));
    input.count(0, 1);
    assert.equal(input.make().select(input.token), 86);
    input.preferences([0]);
    input.count(86, 0x80000000);
    assert.ok(!input.make().select(input.token));
    input.count(86, 0xffffffff);
    assert.equal(input.make().select(input.token), 86);
});

test('AI 건설 선택: 선호 count 가 0 이어도 현재 다섯 수량을 먼저 읽으며 손상된 임시 인덱스는 가짜 TYPE 으로 보정하지 않는다', () => {
    const input = new AiChoiceScene();
    input.preferences([]);
    input.workshops[0] = 0xffffffff;
    assert.ok(throws(() => input.make().select(input.token)));
    input.workshops[0] = 86;
    input.preferences([5]);
    assert.ok(throws(() => input.make().select(input.token)));
});

test('AI 건설 선택: 선택된 다이스 자금 하위 누락은 이미 호출한 사제/비용 효과를 보존한다', () => {
    const input = new AiChoiceScene();
    input.field(0x248, 0x200);
    input.field(0x294, 0);
    input.captured = true;
    input.hooks.wallet = undefined;
    assert.ok(throws(() => input.make().select(input.token)) && JSON.stringify(input.trace) === '["captured:2","dead:2","afford:165"]');
});

test('AI 건설 선택 생성자: 본체 소유자/188 TYPE/워크샾 4/현재 전역 조회기가 없으면 거부하고 가짜 본체를 거부한다', () => {
    const input = new AiChoiceScene();
    const globals = { localOwner: () => 1, playerKnown: () => 1, collectorType: () => 133, daisType: () => 165, workshops: input.workshops, temple: () => 0 };
    assert.ok(throws(() => new RawAiBuildChoice(undefined, input.types, globals, input.hooks)));
    assert.ok(throws(() => new RawAiBuildChoice(input.memory, input.types.slice(0, 187), globals, input.hooks)));
    assert.ok(throws(() => new RawAiBuildChoice(input.memory, input.types, { ...globals, workshops: [1, 2, 3] }, input.hooks)));
    for (const name of ['localOwner', 'playerKnown', 'collectorType', 'daisType']) assert.ok(throws(() => new RawAiBuildChoice(input.memory, input.types, { ...globals, [name]: undefined }, input.hooks)));
    const small = input.memory.allocate(16);
    assert.ok(throws(() => input.make().select(small)) && throws(() => input.make().reserveForCollectors(0x1234)));
    // 현재 TYPE 범위 밖 수집자는 물리 범위 오류이다.
    input.collector = 188;
    assert.ok(throws(() => input.make().reserveForCollectors(input.token)));
});
