// AI 에너지 부족 좌표 목록 raw-ai-dry-spots.js(RawAiDrySpots)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 좌표만 중복 검사(signed 0 은 같고 NaN 은 다름, TYPE 비트 보존), 가득 참/빈 목록은 포인터 접근 없음과 빈 검사의 cursor 불변,
//    한 항목만 조회하고 제거 뒤 cursor 보정 없음, 제거의 안정 압축과 미사용 꼬리/cursor 보존과 끝 이상 index, 에너지 효과가 backing/cursor/count 를 바꾼 뒤 현재 기록 TYPE 재조회, 현재 fake/디버그로 로그 분배(TYPE 0 제거는 로그 없음),
//    선택된 하위 누락/물리 오류 앞의 cursor 보존과 signed 나눗셈 넘침 거부, 능력 BYTE 만 검사하고 금지 상태는 효과 없음, 공용 난수 한 번으로 여덟 방향, 전체 합 뒤 float 저장, 가득 참/중복/영 TYPE/물리 실패에서 좌표·난수 보존.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E170: AI 부족 좌표 목록 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiDrySpots } from '../www/core/raw-ai-dry-spots.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E170)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실제 크기의 소유 블록과 명시 에너지/로그 입력을 준비한다. (2026-11-09 WB04-E170)
class DryScene {
    // 원본 생성자의 내장 목록 필드만 설정한다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x48000000, (raw) => raw.fill(0xa5));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.backing = this.memory.allocate(120);
        this.result = 0;
        this.logs = 0;
        this.pool = new SidPool(32768, { server: true });
        this.calls = [];
        this.fakeLogs = [];
        this.field(0xadc, 10);
        this.field(0xae0, 0);
        this.field(0xae4, this.backing);
        this.field(0xae8, 0);
        this.field(0xaec, 2);
        this.field(0x280, 73);
        this.field(0xb00, 1);
        this.hooks = {
            energy: (box, x, y, type, owner) => { assert.equal(box.text, ''); this.calls.push([x, y, type, owner]); box.text = 's'; return this.result; },
            log: (brain, fake) => { assert.equal(brain, this.token); this.logs += 1; this.fakeLogs.push(fake); }
        };
    }

    // 현재 본체 DWORD 를 읽거나(값 없음) 선택된 DWORD 만 쓴다. (2026-11-09 WB04-E170)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 현재 backing 의 기록 한 DWORD 를 읽는다. (2026-11-09 WB04-E170)
    item(index, field) { return get(this.memory.bytes(this.field(0xae4)), index * 12 + field * 4); }

    // 같은 본체/풀과 명시 하위로 제품을 조립한다. (2026-11-09 WB04-E170)
    make() { return new RawAiDrySpots(this.memory, this.pool, this.hooks); }

    // 원본 자기 경계와 비교 영역 경계를 서로 다른 좌표로 설정한다. (2026-11-09 WB04-E170)
    seedBounds() {
        this.field(0x248, 0x20);
        this.field(0x270, floatToBits(10));
        this.field(0x274, floatToBits(20));
        this.field(0x278, floatToBits(30));
        this.field(0x27c, floatToBits(60));
        this.field(0x2b8, floatToBits(110));
        this.field(0x2bc, floatToBits(120));
        this.field(0x2c0, floatToBits(130));
        this.field(0x2c4, floatToBits(160));
    }
}

test('AI 부족 좌표: 좌표만 중복 검사하며 signed 0 은 같고 NaN 은 원본처럼 서로 다르다(TYPE 비트는 그대로 저장)', () => {
    const input = new DryScene();
    assert.ok(input.make().add(input.token, 0, 4, 73));
    assert.ok(!input.make().add(input.token, -0, 4, 99));
    assert.ok(input.make().add(input.token, Number.NaN, 4, 0x7fc01234, { xBits: 0x7fc12345 }));
    assert.ok(input.make().add(input.token, Number.NaN, 4, 0xdeadbeef));
    assert.ok(input.field(0xae0) === 3 && input.item(0, 0) === 0 && input.item(1, 0) === 0x7fc12345 && input.item(1, 2) === 0x7fc01234 && input.item(2, 2) === 0xdeadbeef && input.calls.length === 0);
});

test('AI 부족 좌표: 가득 찬/빈 목록은 포인터에 접근하지 않으며 빈 검사는 cursor 를 변경하지 않는다', () => {
    const input = new DryScene();
    input.field(0xae4, 0);
    input.field(0xae0, 10);
    input.hooks = {};
    assert.ok(!input.make().add(input.token, 1, 2, 73));
    input.field(0xae0, 0);
    input.field(0xae8, 7);
    assert.ok(input.make().scan(input.token) === 0 && input.field(0xae8) === 7);
    assert.ok(input.make().maintain(input.token) === 0 && !input.logs);
});

test('AI 부족 좌표: 원래 cursor+1 의 한 항목만 조회하고 제거해도 shift 된 항목용 cursor 보정을 추가하지 않는다', () => {
    const input = new DryScene();
    input.make().add(input.token, 1, 2, 71);
    input.make().add(input.token, 3, 4, 72);
    input.make().add(input.token, 5, 6, 73);
    input.result = 0;
    assert.ok(input.make().scan(input.token) === 0 && input.field(0xae8) === 1 && input.field(0xae0) === 3 && input.calls.length === 1 && input.calls.at(-1)[2] === 72);
    input.result = 0x80000000;
    assert.ok(input.make().scan(input.token) === 73 && input.field(0xae0) === 2 && input.field(0xae8) === 2);
    assert.ok(input.make().scan(input.token) === 72 && input.field(0xae0) === 1 && input.field(0xae8) === 1);
    assert.equal(input.calls.at(-1)[2], 72);
});

test('AI 부족 좌표: 제거는 12바이트 기록을 안정 압축하고 미사용 꼬리/cursor 를 그대로 둔다', () => {
    const input = new DryScene();
    input.make().add(input.token, 1, 2, 71);
    input.make().add(input.token, 3, 4, 72);
    input.make().add(input.token, 5, 6, 0x7fc01234);
    input.field(0xae8, 2);
    const tail = Buffer.from(input.memory.bytes(input.backing).subarray(24));
    input.make().remove(input.token, 0);
    assert.ok(input.field(0xae0) === 2 && input.field(0xae8) === 2 && input.item(0, 2) === 72 && input.item(1, 2) === 0x7fc01234);
    assert.ok(Buffer.from(input.memory.bytes(input.backing).subarray(24)).equals(tail));
    input.make().remove(input.token, 99);
    assert.equal(input.field(0xae0), 1);
    input.field(0xae0, 0);
    input.field(0xae4, 0);
    input.make().remove(input.token, 0);
    assert.equal(input.field(0xae0), 0xffffffff);
});

test('AI 부족 좌표: 에너지 효과가 backing/cursor/count 를 바꾼 경우 현재 기록 TYPE 와 압축 대상을 재조회한다', () => {
    const input = new DryScene();
    input.make().add(input.token, 1, 2, 71);
    input.make().add(input.token, 3, 4, 72);
    const replacement = input.memory.allocate(120);
    put(input.memory.bytes(replacement), 8, 91);
    put(input.memory.bytes(replacement), 20, 92);
    input.hooks.energy = (box, x, y, type, owner) => {
        assert.ok(box.text === '' && x === 3 && y === 4 && type === 72 && owner === 2 && input.field(0xae8) === 1);
        input.memory.free(input.backing);
        input.field(0xae4, replacement);
        input.field(0xae8, 0);
        input.field(0xae0, 2);
        input.field(0xaec, 7);
        input.field(0x280, 91);
        return 1;
    };
    assert.ok(input.make().maintain(input.token) === 91 && input.field(0xae0) === 1 && input.item(0, 2) === 92 && !input.memory.contains(input.backing) && JSON.stringify(input.fakeLogs) === '[true]');
});

test('AI 부족 좌표: 실제 제거 TYPE 와 효과 뒤 현재 fake TYPE/디버그 값으로 로그를 선택하며 TYPE 0 제거는 로그하지 않는다', () => {
    const input = new DryScene();
    input.make().add(input.token, 1, 2, 73);
    input.result = 1;
    assert.ok(input.make().maintain(input.token) === 73 && JSON.stringify(input.fakeLogs) === '[true]');
    input.make().add(input.token, 3, 4, 74);
    assert.ok(input.make().maintain(input.token) === 74 && JSON.stringify(input.fakeLogs) === '[true,false]');
    input.hooks.energy = () => { input.field(0xb00, 0); return 1; };
    input.hooks.log = undefined;
    input.make().add(input.token, 5, 6, 75);
    assert.ok(input.make().maintain(input.token) === 75 && input.logs === 2);
    input.field(0xb00, 1);
    input.make().add(input.token, 7, 8, 0);
    assert.ok(input.make().maintain(input.token) === 0 && input.field(0xae0) === 0 && input.logs === 2);
});

test('AI 부족 좌표: 선택된 하위 누락/물리 오류 앞에서도 이미 쓴 cursor 는 보존하고 정상 범위 밖 나눗셈은 명시 거부한다', () => {
    const input = new DryScene();
    input.make().add(input.token, 1, 2, 71);
    input.make().add(input.token, 3, 4, 72);
    input.hooks.energy = undefined;
    assert.ok(throws(() => input.make().scan(input.token)) && input.field(0xae8) === 1 && input.field(0xae0) === 2);
    input.field(0xae4, 0);
    input.field(0xae8, 0);
    assert.ok(throws(() => input.make().scan(input.token)) && input.field(0xae8) === 1);
    input.field(0xae0, 0xffffffff);
    input.field(0xae8, 0x7fffffff);
    assert.ok(throws(() => input.make().scan(input.token)));
    const small = input.memory.allocate(4);
    assert.ok(throws(() => input.make().scan(small)));
});

test('AI 부족 좌표: 능력은 하위 BYTE 20h 만 검사하며 금지 상태에서 난수/좌표/목록/효과를 바꾸지 않는다', () => {
    const input = new DryScene();
    input.seedBounds();
    input.field(0x248, 0x2000);
    input.field(0xae4, 0);
    input.hooks = {};
    const before = Buffer.from(input.memory.bytes(input.token));
    const random = new GameRandom(8);
    input.make().seed(input.token, random);
    assert.ok(random.state === 8 && Buffer.from(input.memory.bytes(input.token)).equals(before) && input.calls.length === 0 && !input.logs);
});

test('AI 부족 좌표: 공용 난수 한 번으로 여덟 방향 모두를 선택하고 원본 TYPE DWORD 와 두 float 비트를 등록한다', () => {
    // 각 switch 번호의 독립적인 기대 좌표이다.
    const expected = [[70, 20], [70, 60], [10, 90], [30, 90], [20, 20], [20, 60], [10, 40], [30, 40]];
    // 작은 실제 시드 1..8 은 next(8) 에서 1..7,0 을 반환하므로 전 분기를 원본 생성기로 선택한다.
    for (let seed = 1; seed <= 8; seed += 1) {
        const input = new DryScene();
        input.seedBounds();
        input.field(0x280, 0xdeadbeef);
        input.field(0xae8, 7);
        const before = Buffer.from(input.memory.bytes(input.token));
        const random = new GameRandom(seed);
        const oracle = new GameRandom(seed);
        const choice = oracle.next(8);
        assert.equal(choice, seed % 8);
        input.make().seed(input.token, random);
        const x = floatToBits(expected[choice][0]);
        const y = floatToBits(expected[choice][1]);
        assert.ok(random.state === oracle.state && input.field(0xaf0) === x && input.field(0xaf4) === y && input.field(0xae0) === 1 && input.item(0, 0) === x && input.item(0, 1) === y && input.item(0, 2) === 0xdeadbeef && input.field(0xae8) === 7 && input.calls.length === 0 && !input.logs);
        const after = Buffer.from(before);
        put(after, 0xaf0, x);
        put(after, 0xaf4, y);
        put(after, 0xae0, 1);
        assert.ok(after.equals(Buffer.from(input.memory.bytes(input.token))));
    }
});

test('AI 부족 좌표: x87 원본의 합/곱 전체 뒤 float 저장과 중간 float 덧셈을 구별한다', () => {
    const input = new DryScene();
    input.seedBounds();
    input.field(0x2c0, floatToBits(16777216));
    input.field(0x2b8, floatToBits(1));
    input.field(0x278, floatToBits(-16777216));
    input.field(0x270, floatToBits(3));
    input.make().seed(input.token, new GameRandom(8));
    assert.ok(input.field(0xaf0) === floatToBits(1) && input.item(0, 0) === floatToBits(1));
});

test('AI 부족 좌표: 가득 참은 등록을 건너뛰지만 앞서 쓴 가짜 좌표와 공용 난수 소비를 되돌리지 않는다', () => {
    const input = new DryScene();
    input.seedBounds();
    input.field(0xadc, 0);
    input.field(0xae4, 0);
    input.hooks = {};
    const random = new GameRandom(8);
    const expected = new GameRandom(8);
    expected.next(8);
    input.make().seed(input.token, random);
    assert.ok(!input.field(0xae0) && input.field(0xaf0) === floatToBits(70) && input.field(0xaf4) === floatToBits(20) && random.state === expected.state);
});

test('AI 부족 좌표: 중복 좌표의 기존 TYPE 를 교체하지 않으며 새 좌표/난수 소비는 유지한다', () => {
    const input = new DryScene();
    input.seedBounds();
    assert.ok(input.make().add(input.token, 70, 20, 91));
    input.make().seed(input.token, new GameRandom(8));
    assert.ok(input.field(0xae0) === 1 && input.item(0, 2) === 91 && input.field(0x280) === 73 && input.field(0xaf0) === input.item(0, 0) && input.field(0xaf4) === input.item(0, 1));
});

test('AI 부족 좌표: 원본에는 fake TYPE 0/지도 경계 검사가 없으므로 선택된 음수 좌표도 같은 목록에 등록한다', () => {
    const input = new DryScene();
    input.seedBounds();
    input.field(0x280, 0);
    input.field(0x274, floatToBits(-300));
    input.make().seed(input.token, new GameRandom(8));
    assert.ok(input.field(0xae0) === 1 && input.item(0, 2) === 0 && input.item(0, 1) === floatToBits(-300));
});

test('AI 부족 좌표: 등록 위치의 물리 오류는 앞선 좌표/난수 쓰기를 유지하고 count 를 가짜 성공으로 증가시키지 않는다', () => {
    const input = new DryScene();
    input.seedBounds();
    input.field(0xae4, 0);
    const random = new GameRandom(8);
    const expected = new GameRandom(8);
    expected.next(8);
    assert.ok(throws(() => input.make().seed(input.token, random)) && random.state === expected.state && input.field(0xaf0) === floatToBits(70) && input.field(0xaf4) === floatToBits(20) && !input.field(0xae0));
});

test('AI 부족 좌표 생성자: 본체 소유자와 같은 PATCH 풀이 없으면 거부하고 생성은 목록/게임 판단을 건드리지 않는다', () => {
    const input = new DryScene();
    const before = Buffer.from(input.memory.bytes(input.token));
    assert.ok(throws(() => new RawAiDrySpots(undefined, input.pool, input.hooks)));
    assert.ok(throws(() => new RawAiDrySpots(input.memory, {}, input.hooks)));
    const made = new RawAiDrySpots(input.memory, input.pool, undefined);
    assert.ok(made.pool === input.pool && Buffer.from(input.memory.bytes(input.token)).equals(before));
});
