// 비미션 보충 오벨리스크 배치 raw-battle-nugget-placement.js(RawBattleNuggetPlacement, 원본 0040e050)를 cpppj 소스 계약으로 대조한다(WB04-E558).
// 원본 관측 fixture 가 없는 소단위이므로 모드별 seed(boss 의 실제 wall seed·비boss 의 지역0 byte3*103)·원문 호출 순서·값 전달 사각형·선택 장치만 요구하는 규칙을 단언한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E558)
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameRandom } from '../www/core/random.js';
import { RawFortFinishOwner } from '../www/core/raw-fort-finish-owner.js';
import { placeBattleNuggets } from '../www/core/raw-battle-nugget-placement.js';

// 호출 순서를 기록하는 완전한 장치 한 벌이다. seeds 는 place 가 받은 지역 난수의 상태를 모은다. (2026-11-15 WB04-E558)
function makeHooks(trace, overrides = {}) {
    return {
        singleAssertion: () => trace.push('assert'),
        wallSeconds: () => { trace.push('wall'); return 4321.987; },
        regionZero: () => { trace.push('region'); return Uint8Array.of(1, 2, 3, 6, 5, 4); },
        place: (random, rectangle, region, quantity, type, force) => { trace.push('place'); trace.placed = { random, rectangle, region, quantity, type, force }; return 7; },
        ...overrides
    };
}

test('비boss: 지역 0 기록의 byte3 에 103 을 곱한 값이 지역 난수 seed 이고 -1 지역·force 1 로 배치한다', () => {
    const trace = [];
    const rectangle = { left: 1.5, top: 2.25, right: 30, bottom: 40.5 };
    placeBattleNuggets(rectangle, 5, { single: false, boss: false, buriedType: 0x12 }, makeHooks(trace));
    assert.deepEqual([...trace], ['region', 'place']);
    const placed = trace.placed;
    assert.equal(placed.random.state, 6 * 103);
    assert.deepEqual(placed.rectangle, rectangle);
    assert.notEqual(placed.rectangle, rectangle);
    assert.equal(placed.region, -1);
    assert.equal(placed.quantity, 5);
    assert.equal(placed.type, 0x12);
    assert.equal(placed.force, 1);
    // seed 는 첫 난수가 쓰이기 전의 상태이며 공유 게임 난수와 별개 객체이다.
    const expected = new GameRandom(6 * 103);
    assert.equal(placed.random.next(1000), expected.next(1000));
    // byte3 외의 기록 값은 seed 에 쓰이지 않는다(최댓값 255 도 곱셈이 감기지 않는다).
    const other = [];
    placeBattleNuggets(rectangle, 1, { single: false, boss: false, buriedType: 1 }, makeHooks(other, { regionZero: () => Uint8Array.of(9, 9, 9, 255, 9, 9) }));
    assert.equal(other.placed.random.state, 255 * 103);
    // 일반 배열이 BYTE 폭을 넘는 값을 주어도 원문 uint8 저장처럼 낮은 BYTE 만 쓴다.
    const wide = [];
    placeBattleNuggets(rectangle, 1, { single: false, boss: false, buriedType: 1 }, makeHooks(wide, { regionZero: () => [0, 0, 0, 0x1ff, 0, 0] }));
    assert.equal(wide.placed.random.state, 255 * 103);
    const zero = [];
    placeBattleNuggets(rectangle, 1, { single: false, boss: false, buriedType: 1 }, makeHooks(zero, { regionZero: () => [0, 0, 0, 0, 0, 0] }));
    assert.equal(zero.placed.random.state, 0);
});

test('boss: 실제 wall seed(RawFortFinishOwner.timeSeed)를 쓰고 지역 0 기록을 요구하지 않는다', () => {
    const trace = [];
    placeBattleNuggets({ left: 0, top: 0, right: 1, bottom: 1 }, 3, { single: false, boss: true, buriedType: 0x30 }, makeHooks(trace, { regionZero: undefined }));
    assert.deepEqual([...trace], ['wall', 'place']);
    assert.equal(trace.placed.random.state, RawFortFinishOwner.timeSeed(4321.987));
    assert.notEqual(RawFortFinishOwner.timeSeed(4321.987), 6 * 103);
    assert.equal(trace.placed.type, 0x30);
    assert.equal(trace.placed.quantity, 3);
});

test('원문 순서: single 진단 → boss 재읽기 → 지역 seed → 배치 장치 선택 → 현재 타입 읽기 → 배치', () => {
    const trace = [];
    const state = {
        single: () => { trace.push('read-single'); return true; },
        boss: () => { trace.push('read-boss'); return false; },
        buriedType: () => { trace.push('read-type'); return 9; }
    };
    placeBattleNuggets({ left: 0, top: 0, right: 1, bottom: 1 }, 1, state, makeHooks(trace));
    assert.deepEqual([...trace], ['read-single', 'assert', 'read-boss', 'region', 'read-type', 'place']);
    // 진단이 현재 boss 를 바꾸면 바뀐 값으로 seed 를 고르고, 지역 기록 장치가 현재 타입을 바꾸면 바뀐 값으로 배치한다.
    const box = { boss: false, type: 5 };
    const flipped = [];
    placeBattleNuggets({ left: 0, top: 0, right: 1, bottom: 1 }, 1, { single: true, boss: { get value() { return box.boss; } }, buriedType: { get value() { return box.type; } } },
        makeHooks(flipped, { singleAssertion: () => { flipped.push('assert'); box.boss = true; }, wallSeconds: () => { flipped.push('wall'); box.type = 77; return 10; } }));
    assert.deepEqual([...flipped], ['assert', 'wall', 'place']);
    assert.equal(flipped.placed.type, 77);
    assert.equal(flipped.placed.random.state, RawFortFinishOwner.timeSeed(10));
});

test('single 이 아니면 진단 장치를 요구하지 않고 single 이면 진단 뒤 계속한다', () => {
    const trace = [];
    assert.doesNotThrow(() => placeBattleNuggets({ left: 0, top: 0, right: 1, bottom: 1 }, 1, { single: false, boss: false, buriedType: 1 }, makeHooks(trace, { singleAssertion: undefined })));
    assert.deepEqual([...trace], ['region', 'place']);
    const single = [];
    placeBattleNuggets({ left: 0, top: 0, right: 1, bottom: 1 }, 1, { single: { value: 1 }, boss: false, buriedType: 1 }, makeHooks(single));
    assert.deepEqual([...single], ['assert', 'region', 'place']);
    assert.throws(() => placeBattleNuggets({ left: 0, top: 0, right: 1, bottom: 1 }, 1, { single: true, boss: false, buriedType: 1 }, makeHooks([], { singleAssertion: undefined })), /미연결/);
});

test('선택된 장치가 없으면 앞선 효과를 유지한 채 명시 오류이고 선택되지 않은 장치는 요구하지 않는다', () => {
    const rectangle = { left: 0, top: 0, right: 1, bottom: 1 };
    // boss 인데 wall 시계가 없다.
    const boss = [];
    assert.throws(() => placeBattleNuggets(rectangle, 1, { single: false, boss: true, buriedType: 1 }, makeHooks(boss, { wallSeconds: undefined })), /미연결/);
    assert.deepEqual([...boss], []);
    // 비boss 인데 지역 0 기록 장치가 없다.
    assert.throws(() => placeBattleNuggets(rectangle, 1, { single: false, boss: false, buriedType: 1 }, makeHooks([], { regionZero: undefined })), /미연결/);
    // 배치 장치가 없으면 현재 타입을 읽기 전에 거부한다.
    let typeReads = 0;
    const state = { single: false, boss: false, buriedType: () => { typeReads += 1; return 1; } };
    const trace = [];
    assert.throws(() => placeBattleNuggets(rectangle, 1, state, makeHooks(trace, { place: undefined })), /미연결/);
    assert.deepEqual([...trace], ['region']);
    assert.equal(typeReads, 0);
    // 미선택 경로의 장치(비boss 의 wall 시계, boss 의 지역 기록)는 없어도 된다.
    assert.doesNotThrow(() => placeBattleNuggets(rectangle, 1, { single: false, boss: false, buriedType: 1 }, makeHooks([], { wallSeconds: undefined })));
});

test('지역 0 기록은 원문 6바이트이며 호스트 크기 위반은 거부한다', () => {
    const rectangle = { left: 0, top: 0, right: 1, bottom: 1 };
    for (const size of [0, 3, 4, 5, 7]) {
        const trace = [];
        assert.throws(() => placeBattleNuggets(rectangle, 1, { single: false, boss: false, buriedType: 1 }, makeHooks(trace, { regionZero: () => new Uint8Array(size) })), RangeError);
        assert.deepEqual([...trace], []);
    }
});

test('인자 변환: 수량·타입은 unsigned 32비트이고 사각형은 float 값 전달 사본이다', () => {
    const trace = [];
    const rectangle = { left: 0.1, top: -0.2, right: 3.3, bottom: 16777217 };
    placeBattleNuggets(rectangle, -1, { single: false, boss: false, buriedType: -2 }, makeHooks(trace));
    assert.equal(trace.placed.quantity, 0xffffffff);
    assert.equal(trace.placed.type, 0xfffffffe);
    assert.deepEqual(trace.placed.rectangle, { left: Math.fround(0.1), top: Math.fround(-0.2), right: Math.fround(3.3), bottom: 16777216 });
    // 호출자의 사각형은 바뀌지 않고, 배치가 사본을 바꿔도 다음 호출에 영향이 없다.
    assert.deepEqual(rectangle, { left: 0.1, top: -0.2, right: 3.3, bottom: 16777217 });
    trace.placed.rectangle.left = 99;
    assert.equal(rectangle.left, 0.1);
});
