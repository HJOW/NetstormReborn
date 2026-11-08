// AI 전략 말미의 같은 지갑/SP 정리 raw-ai-economy.js(RawAiEconomy)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 기한과 같을 때만 단일 지원(큰 시간 점프도 누적 반복 없음), 지원의 DWORD 감김/데모 하한(signed),
//    포획/데모 하위가 바꾼 owner·비용·지갑의 다음 원문 읽기 재사용, 비용 + 2200 의 DWORD 감김 뒤 비교와 현재 지갑 재조회, 최종 SP 쓰기는 매번(float 반올림이 본체 정수 지갑을 바꾸지 않음), NaN 기한과 선택된 누락 하위의 도달 위치 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E166: AI 경제 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiEconomy } from '../www/core/raw-ai-economy.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E166)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 원본 본체와 실제 난수/분산 SP 를 하나의 검사 장면에 연결한다(복사본 없이 같은 저장소를 빌린다). (2026-11-09 WB04-E166)
class EconomyInput {
    // 같은 지갑·타이머를 초기 입력으로 준비한다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x44000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.random = new GameRandom(0x12345678);
        this.sp = new ScrambledSpStore(this.random, () => 19);
        this.now = 16777216.25;
        this.demo = false;
        this.queries = [];
        this.demos = 0;
        this.field(0x260, 1);
        this.field(0x264, 2);
        this.field(0x2d8, 100);
        this.field(0x290, 500);
        this.field(0x24c, 7);
        this.hooks = {
            demo: () => { this.demos += 1; return this.demo; },
            captured: (owner) => { this.queries.push(owner); return false; }
        };
    }

    // 현재 본체 DWORD 를 읽거나(값 없음) 지정 DWORD 만 쓴다. (2026-11-09 WB04-E166)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 현재 QWORD 기한을 명시 입력으로 쓴다(값 없음이면 실제 본체에 기록된 기한을 읽는다). (2026-11-09 WB04-E166)
    due(value) {
        const view = new DataView(new ArrayBuffer(8));
        if (value === undefined) {
            view.setUint32(0, this.field(0x5d8), true);
            view.setUint32(4, this.field(0x5dc), true);
            return view.getFloat64(0, true);
        }
        view.setFloat64(0, value, true);
        this.field(0x5d8, view.getUint32(0, true));
        this.field(0x5dc, view.getUint32(4, true));
        return undefined;
    }

    // 복사본 없이 실제 저장소를 연결한다. (2026-11-09 WB04-E166)
    make() { return new RawAiEconomy(this.memory, this.sp, () => this.now, this.hooks); }
}

// float 단정도 비트이다. (2026-11-09 WB04-E166)
const spBits = (value) => floatToBits(Math.fround(value));

test('AI 경제: 기한과 같을 때만 단일 지원을 수행하며 큰 시간 점프도 누적 보충 반복으로 바꾸지 않는다', () => {
    const input = new EconomyInput();
    input.due(input.now + 0.25);
    const economy = input.make();
    economy.reconcile(input.token);
    assert.ok(input.field(0x2d8) === 100 && input.sp.get(1) === spBits(100) && input.demos === 0 && JSON.stringify(input.queries) === '[2,1]');
    input.now += 0.25;
    economy.reconcile(input.token);
    assert.ok(input.field(0x2d8) === 107 && input.due() === 16777217.5 && input.demos === 1);
    economy.reconcile(input.token);
    assert.ok(input.field(0x2d8) === 107 && input.demos === 1);
    input.now += 100;
    economy.reconcile(input.token);
    assert.ok(input.field(0x2d8) === 114 && input.due() === input.now + 1 && input.demos === 2);
});

test('AI 경제: 일반 지원의 DWORD 감김과 데모 하한을 signed 값으로 적용한다', () => {
    const normal = new EconomyInput();
    normal.field(0x2d8, 0x80000010);
    normal.field(0x24c, 0xffffffff);
    normal.make().reconcile(normal.token);
    assert.ok(normal.field(0x2d8) === 0x8000000f && normal.sp.get(1) === spBits(0x8000000f | 0));
    const demo = new EconomyInput();
    demo.demo = true;
    demo.field(0x2d8, 0);
    demo.field(0x24c, 0x80000000);
    demo.make().reconcile(demo.token);
    assert.ok(demo.field(0x2d8) === 1190 && demo.sp.get(1) === spBits(1190));
    const ceiling = new EconomyInput();
    ceiling.field(0x2d8, 2700);
    ceiling.make().reconcile(ceiling.token);
    assert.ok(ceiling.field(0x2d8) === 2700 && ceiling.demos === 0 && ceiling.due() === ceiling.now + 1);
});

test('AI 경제: 포획/데모 하위가 바꾼 owner·비용·지갑을 다음 원문 읽기에서 다시 사용한다', () => {
    const input = new EconomyInput();
    input.due(input.now + 100);
    input.hooks.captured = (owner) => {
        input.queries.push(owner);
        if (owner === 2) {
            input.field(0x290, 100);
            return true;
        }
        assert.equal(owner, 3);
        input.field(0x290, 5000);
        return true;
    };
    input.hooks.demo = () => {
        input.demos += 1;
        if (input.demos === 1) {
            assert.equal(input.field(0x2d8), 2300);
            input.field(0x260, 3);
            input.field(0x2d8, 5000);
        }
        return true;
    };
    input.make().reconcile(input.token);
    assert.ok(JSON.stringify(input.queries) === '[2,3]' && input.demos === 2 && input.field(0x2d8) === 7200 && input.sp.get(3) === spBits(7200));
});

test('AI 경제: 비용 + 2200 도 DWORD 감김 뒤 비교하며 데모 클램프는 현재 signed 지갑을 다시 읽는다', () => {
    const input = new EconomyInput();
    input.due(input.now + 100);
    input.field(0x290, 0x7fffffff);
    input.field(0x2d8, 0x80000000);
    input.hooks.captured = (owner) => owner === 2;
    input.hooks.demo = () => {
        assert.equal(input.field(0x2d8), 0x80000897);
        input.field(0x2d8, 1191);
        return true;
    };
    input.make().reconcile(input.token);
    assert.ok(input.field(0x2d8) === 1191 && input.sp.get(1) === spBits(1191));
});

test('AI 경제: 최종 SP 쓰기는 매번 수행하며 float 반올림 때문에 본체 정수 지갑을 바꾸지 않는다', () => {
    const input = new EconomyInput();
    input.due(input.now + 100);
    input.field(0x2d8, 16777217);
    input.hooks.demo = undefined;
    const before = Buffer.from(input.memory.bytes(input.token));
    const economy = input.make();
    economy.reconcile(input.token);
    assert.ok(input.sp.get(1) === spBits(16777216) && input.field(0x2d8) === 16777217 && Buffer.from(input.memory.bytes(input.token)).equals(before));
    const random = input.random.state;
    economy.reconcile(input.token);
    assert.ok(input.random.state !== random && input.field(0x2d8) === 16777217);
});

test('AI 경제: unordered 기한은 지원만 생략하고 포획/최종 SP 는 계속하며 누락된 실제 하위는 도달 위치에서 실패한다', () => {
    const input = new EconomyInput();
    input.due(Number.NaN);
    input.hooks.demo = undefined;
    input.make().reconcile(input.token);
    assert.ok(input.field(0x2d8) === 100 && JSON.stringify(input.queries) === '[2,1]' && Number.isNaN(input.due()));
    // 데모 조회가 없으면 지원이 지갑에 더해진 뒤 그 위치에서 실패하고 기한은 이미 갱신되며 SP 는 쓰지 않는다.
    const missing = new EconomyInput();
    missing.hooks.demo = undefined;
    assert.ok(throws(() => missing.make().reconcile(missing.token)));
    assert.ok(missing.field(0x2d8) === 107 && missing.due() === missing.now + 1 && !missing.sp.initialized);
    const capture = new EconomyInput();
    capture.due(capture.now + 100);
    capture.hooks.captured = undefined;
    assert.ok(throws(() => capture.make().reconcile(capture.token)));
    assert.ok(!capture.sp.initialized);
    // 크기가 다른 가짜 본체는 어떤 효과보다 먼저 거부한다.
    const small = capture.memory.allocate(4);
    assert.ok(throws(() => capture.make().reconcile(small)));
});

test('AI 경제 생성자: 본체 소유자/분산 SP 저장소/현재 시각 조회기가 없으면 거부하고 생성은 효과가 없다', () => {
    const input = new EconomyInput();
    const before = Buffer.from(input.memory.bytes(input.token));
    assert.ok(throws(() => new RawAiEconomy(undefined, input.sp, () => 0, input.hooks)));
    assert.ok(throws(() => new RawAiEconomy(input.memory, undefined, () => 0, input.hooks)));
    assert.ok(throws(() => new RawAiEconomy(input.memory, input.sp, 0, input.hooks)));
    new RawAiEconomy(input.memory, input.sp, () => 0, undefined);
    assert.ok(Buffer.from(input.memory.bytes(input.token)).equals(before) && !input.sp.initialized);
});
