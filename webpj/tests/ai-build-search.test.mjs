// AI 배치 탐색 raw-ai-build-search.js(RawAiBuildSearch)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 발자국을 더한 왼쪽/위 절삭과 전체 owner/cache 쓰기·34 BYTE 주변 원문 보존, 3f 마스크/전체 footprint 와 0 이하 폭/높이,
//    TYPE 0 은 시계도 요구하지 않음, strict 최대 거리만 전체 배치에 넘기며 owner 는 현재 BYTE, 동률/거리 0/NaN 점수, spot 탈락의 지연 TYPE 접근, 행 예산 중단/재개, deadline 동일/unordered, 행 안에서는 시계 재조회 없음,
//    행 snapshot 과 cached TYPE/owner 의 현재 재조회 및 배치 후 무조건 점수 저장, 최대 X 의 현재 재조회와 선택 하위 누락의 앞선 효과 보존, x 차이 float 저장/y 미축소/비교는 float 저장보다 먼저, 초기화 실패의 앞선 쓰기 보존과 지도 밖 주소 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E169: AI 배치 탐색 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiBuildSearch } from '../www/core/raw-ai-build-search.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E169)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 검사의 opaque32 TYPE 표 기준이며 500 stride 를 보존한다. (2026-11-09 WB04-E169)
const TYPE_BASE = 0x3e000000;
const bits = (value) => floatToBits(Math.fround(value));

// 같은 실제 본체/TYPE/spot 과 명시 벽시각/배치 효과를 준비한다. (2026-11-09 WB04-E169)
class AiSearchScene {
    // 기본 범위는 x 2..4/y 2..3 이고 모든 칸이 비어 있다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x4d000000, (raw) => raw.fill(0x5a));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.types = Array.from({ length: 188 }, () => ({ footX: 0, footY: 0 }));
        this.spots = new Uint8Array(65536).fill(2);
        this.queries = [];
        this.times = [];
        this.clockCalls = 0;
        this.now = 0;
        this.allow = true;
        this.types[80].footX = 1;
        this.types[80].footY = 1;
        this.types[81].footX = 1;
        this.types[81].footY = 1;
        this.hooks = {
            wall: () => {
                const index = this.clockCalls;
                this.clockCalls += 1;
                if (this.times.length === 0) return this.now;
                assert.ok(index < this.times.length);
                return index < this.times.length ? this.times[index] : Number.NaN;
            },
            mayPlace: (query) => { this.queries.push(query); return this.allow; }
        };
        this.make().initialize(this.token, 80, { left: 1, top: 1, right: 5, bottom: 4 }, { x: 0, y: 0 }, 0x102);
    }

    // 현재 state 상대 DWORD 를 쓰거나(값 있음) 읽는다. (2026-11-09 WB04-E169)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), 0x574 + at);
        put(this.memory.bytes(this.token), 0x574 + at, value >>> 0);
        return undefined;
    }

    // 같은 현재 저장소/전역 효과를 빌린다. (2026-11-09 WB04-E169)
    make() { return new RawAiBuildSearch(this.memory, this.types, this.spots, TYPE_BASE, this.hooks); }
}

test('AI 배치 탐색: footprint 를 더한 왼쪽/위 절삭과 전체 owner/cache 를 쓰고 34 BYTE 주변 원문을 보존한다', () => {
    const input = new AiSearchScene();
    input.types[80].footX = 2;
    input.types[80].footY = 3;
    const before = Buffer.from(input.memory.bytes(input.token));
    input.make().initialize(input.token, 80, { left: Math.fround(-1.9), top: Math.fround(-2.9), right: Math.fround(7.9), bottom: Math.fround(9.9) }, { x: Number.NaN, y: -0, xBits: 0x7fc01234 }, 0xdead0102);
    assert.ok(input.field(0) === 80 && input.field(4) === 0 && input.field(8) === 0xc61c4000 && input.field(0xc) === 0xc61c4000 && input.field(0x10) === 0x7fc01234 && input.field(0x14) === 0x80000000
        && input.field(0x18) === TYPE_BASE + 80 * 500 && input.field(0x1c) === 0xdead0102 && input.field(0x20) === 0 && input.field(0x24) === 0 && input.field(0x28) === 7 && input.field(0x2c) === 9 && input.field(0x30) === 0);
    const raw = Buffer.from(input.memory.bytes(input.token));
    assert.ok(raw.subarray(0, 0x574).equals(before.subarray(0, 0x574)) && raw.subarray(0x5a8).equals(before.subarray(0x5a8)));
});

test('AI 배치 탐색: 3f 마스크/전체 footprint 를 검사하며 0 이하 폭/높이는 칸 접근 없이 성공할 수 있다', () => {
    const input = new AiSearchScene();
    input.types[80].footX = 2;
    input.types[80].footY = 3;
    input.spots[5 * 256 + 4] = 0xc2;
    assert.ok(input.make().freeFootprint(80, 5, 6));
    input.spots[5 * 256 + 4] = 0x42;
    assert.ok(input.make().freeFootprint(80, 5, 6));
    input.spots[5 * 256 + 4] = 3;
    assert.ok(!input.make().freeFootprint(80, 5, 6));
    input.types[80].footY = 0;
    assert.ok(input.make().freeFootprint(80, -2, -2));
    input.types[80].footY = 3;
    input.types[80].footX = 0;
    assert.ok(input.make().freeFootprint(80, -2, -2));
});

test('AI 배치 탐색: TYPE 0 은 시계도 요구하지 않고 TYPE 비영의 빈 행 범위는 최초 시계만 읽는다', () => {
    const zero = new AiSearchScene();
    zero.field(0, 0);
    zero.hooks = {};
    zero.make().scan(zero.token, 1);
    assert.equal(zero.clockCalls, 0);
    const empty = new AiSearchScene();
    empty.field(0x30, empty.field(0x2c));
    empty.hooks.mayPlace = undefined;
    empty.make().scan(empty.token, 1);
    assert.ok(empty.clockCalls === 1 && empty.queries.length === 0);
});

test('AI 배치 탐색: 순서대로 strict 최대 거리만 전체 배치에 넘기며 owner 는 현재 BYTE/나머지 인자는 원본 값이다', () => {
    const input = new AiSearchScene();
    input.make().scan(input.token, 0);
    assert.ok(input.clockCalls === 1 && input.queries.length === 4 && input.field(4) === bits(25) && input.field(8) === bits(4) && input.field(0xc) === bits(3) && input.field(0x30) === 4 && input.field(0) === 80);
    // 선택된 네 후보의 현재 인자를 비교하며 실제 허용 결과를 다시 계산하지 않는다.
    for (const query of input.queries) assert.ok(query.type === 80 && !query.argument && !query.flags && query.owner === 2 && query.mode === 1);
    assert.ok(input.queries[0].x === 2 && input.queries[0].y === 2 && input.queries[3].x === 4 && input.queries[3].y === 3);
    input.make().scan(input.token, 0);
    assert.ok(input.clockCalls === 2 && input.queries.length === 4);
});

test('AI 배치 탐색: 동률은 앞선 후보를 유지하고 거리 0/NaN 은 최초 점수 0 보다 크지 않다', () => {
    const tie = new AiSearchScene();
    tie.make().initialize(tie.token, 80, { left: 0, top: 0, right: 3, bottom: 2 }, { x: 1.5, y: 1 }, 2);
    tie.make().scan(tie.token, 0);
    assert.ok(tie.queries.length === 1 && tie.field(8) === bits(1));
    const zero = new AiSearchScene();
    zero.make().initialize(zero.token, 80, { left: 0, top: 0, right: 2, bottom: 2 }, { x: 1, y: 1 }, 2);
    zero.make().scan(zero.token, 0);
    assert.ok(zero.queries.length === 0 && zero.field(8) === 0xc61c4000);
    const nan = new AiSearchScene();
    nan.field(4, 0x7fc01234);
    nan.make().scan(nan.token, 0);
    assert.ok(nan.queries.length === 0 && nan.field(4) === 0x7fc01234);
});

test('AI 배치 탐색: spot 탈락은 현재 TYPE 를 조회하지 않고 선택된 칸에서만 실제 footprint 를 요구한다', () => {
    const input = new AiSearchScene();
    input.field(0, 200);
    input.spots.fill(0);
    input.make().scan(input.token, 0);
    assert.ok(input.queries.length === 0 && input.field(0x30) === 4);
    input.field(0x30, 2);
    input.spots[2 * 256 + 2] = 2;
    assert.ok(throws(() => input.make().scan(input.token, 0)) && input.field(0x30) === 2);
});

test('AI 배치 탐색: 현재 행을 완료하고 다음 행 예산에서 멈추며 다음 호출은 그 행부터 현재 점수를 유지해 재개한다', () => {
    const input = new AiSearchScene();
    input.times = [0, 0.02, 0.031];
    input.make().scan(input.token, 1);
    assert.ok(input.clockCalls === 3 && input.field(0x30) === 3 && input.field(4) === bits(20) && input.queries.length === 3);
    input.times = [1, 1];
    input.clockCalls = 0;
    input.make().scan(input.token, 1);
    assert.ok(input.clockCalls === 2 && input.field(0x30) === 4 && input.field(4) === bits(25) && input.queries.length === 4);
});

test('AI 배치 탐색: deadline 동일은 진행하고 unordered 는 중단하며 비예산 모드는 최초 시각 NaN 도 행 완료를 막지 않는다', () => {
    const equal = new AiSearchScene();
    equal.times = [0, Math.fround(0.03), Number.NaN];
    equal.make().scan(equal.token, 1);
    assert.ok(equal.field(0x30) === 3 && equal.queries.length === 3);
    const unordered = new AiSearchScene();
    unordered.times = [Number.NaN, 0];
    unordered.make().scan(unordered.token, 1);
    assert.ok(unordered.field(0x30) === 2 && unordered.queries.length === 0);
    const full = new AiSearchScene();
    full.times = [Number.NaN];
    full.make().scan(full.token, 0);
    assert.ok(full.clockCalls === 1 && full.field(0x30) === 4);
});

test('AI 배치 탐색: 배치 효과가 시간을 초과시켜도 행 안에서는 시계를 추가 조회하지 않는다', () => {
    const input = new AiSearchScene();
    input.hooks.mayPlace = (query) => { input.queries.push(query); input.now = 1; return true; };
    input.make().scan(input.token, 1);
    assert.ok(input.clockCalls === 3 && input.queries.length === 3 && input.field(0x30) === 3 && input.field(8) === bits(4));
});

test('AI 배치 탐색: 행 snapshot 과 cached TYPE/owner 의 현재 재조회 및 배치 후 무조건 점수 저장을 보존한다', () => {
    const input = new AiSearchScene();
    input.hooks.mayPlace = (query) => {
        input.queries.push(query);
        input.field(4, bits(100000));
        input.field(0x18, TYPE_BASE + 81 * 500);
        input.field(0x1c, 0x1ff);
        input.field(0x20, 100);
        input.field(0x30, 10);
        return true;
    };
    input.make().scan(input.token, 0);
    assert.ok(input.queries.length === 3 && input.queries[0].type === 80 && input.queries[1].type === 81 && input.queries[1].owner === 255 && input.queries[2].y === 2 && input.field(4) === bits(20) && input.field(0x30) === 11 && input.field(0x20) === 100);
});

test('AI 배치 탐색: maxX 는 후보 효과 뒤 현재 값을 읽고 cached TYPE/선택 하위 누락은 앞선 시계 효과까지만 보존한다', () => {
    const bound = new AiSearchScene();
    bound.field(0x2c, 3);
    bound.hooks.mayPlace = (query) => { bound.queries.push(query); bound.field(0x28, 3); return true; };
    bound.make().scan(bound.token, 0);
    assert.ok(bound.queries.length === 1 && bound.field(0x30) === 3);
    const invalid = new AiSearchScene();
    invalid.field(0x18, TYPE_BASE + 1);
    assert.ok(throws(() => invalid.make().scan(invalid.token, 0)) && invalid.clockCalls === 1 && invalid.field(4) === 0 && invalid.field(0x30) === 2);
    const missing = new AiSearchScene();
    missing.hooks.mayPlace = undefined;
    assert.ok(throws(() => missing.make().scan(missing.token, 0)) && missing.clockCalls === 1 && missing.field(4) === 0 && missing.field(0x30) === 2);
});

test('AI 배치 탐색: x 차이는 float 저장/y 차이는 미축소이며 거리 비교는 float 점수 저장보다 먼저다', () => {
    const input = new AiSearchScene();
    input.make().initialize(input.token, 80, { left: 0, top: 0, right: 2, bottom: 2 }, { x: -33554432, y: -33554432 }, 2);
    const same = floatToBits(2 ** 51);
    input.field(4, same);
    input.make().scan(input.token, 0);
    assert.ok(input.queries.length === 1 && input.field(4) === same && input.field(8) === bits(1) && input.field(0xc) === bits(1));
});

test('AI 배치 탐색: 초기화의 TYPE 조회 실패는 먼저 쓴 TYPE 를 유지하며 지도 밖 주소를 합법 칸으로 clamp 하지 않는다', () => {
    const input = new AiSearchScene();
    const cached = input.field(0x18);
    assert.ok(throws(() => input.make().initialize(input.token, 200, { left: 0, top: 0, right: 0, bottom: 0 }, { x: 0, y: 0 }, 2)) && input.field(0) === 200 && input.field(0x18) === cached);
    const address = new AiSearchScene();
    address.field(0x20, 0xffffffff);
    assert.ok(throws(() => address.make().scan(address.token, 0)) && address.clockCalls === 1 && address.field(0x30) === 2);
});

test('AI 배치 탐색 생성자: 본체 소유자/188 TYPE/65536 spot/TYPE 핸들이 없으면 거부하고 생성은 시계/배치를 실행하지 않는다', () => {
    const input = new AiSearchScene();
    const before = input.clockCalls;
    assert.ok(throws(() => new RawAiBuildSearch(undefined, input.types, input.spots, TYPE_BASE, input.hooks)));
    assert.ok(throws(() => new RawAiBuildSearch(input.memory, input.types.slice(0, 187), input.spots, TYPE_BASE, input.hooks)));
    assert.ok(throws(() => new RawAiBuildSearch(input.memory, input.types, new Uint8Array(65535), TYPE_BASE, input.hooks)));
    assert.ok(throws(() => new RawAiBuildSearch(input.memory, input.types, input.spots, 0, input.hooks)));
    assert.ok(throws(() => new RawAiBuildSearch(input.memory, input.types, input.spots, 0xffffffff - 186 * 500, input.hooks)));
    new RawAiBuildSearch(input.memory, input.types, input.spots, TYPE_BASE, undefined);
    assert.ok(input.clockCalls === before && input.queries.length === 0);
    const small = input.memory.allocate(16);
    assert.ok(throws(() => input.make().scan(small, 0)));
});
