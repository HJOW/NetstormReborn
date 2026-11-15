// 전투 옵션 슬롯·자원 옵션 raw-battle-option-slots.js(RawBattleOptionSlots·RawBattleResourceOptions)를 cpppj 소스 계약(원본 0041c4f0~0041c6a0, 0041c800~0041c870)으로 대조한다(WB04-E558).
// 원본 관측 fixture 가 없는 소단위이므로 읽는/쓰는 BYTE 위치·폭·순환 경계·유효 슬롯 관문·물리 범위를 직접 단언한다. 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E558)
import test from 'node:test';
import assert from 'node:assert/strict';
import {
    BATTLE_OPTIONS_BYTES, BATTLE_SLOT_RANKS, battleGeyserPeriod, battleIncomeAmount, battleIncomePeriod, battleSlotRank, decreaseBattleSlotColor, decreaseBattleSlotRank,
    increaseBattleSlotColor, increaseBattleSlotRank, toggleBattleSlotSpecial
} from '../www/core/raw-battle-option-slots.js';

// 87바이트 옵션을 서로 구별되는 값으로 채운다(다른 위치를 잘못 쓰면 바로 드러나도록). (2026-11-15 WB04-E558)
const makeOptions = () => Uint8Array.from({ length: 87 }, (_, index) => (index * 7 + 11) & 0xff);
// 슬롯 필드의 BYTE 위치(색 0x47·랭크 0x4f·특수 0x3f)이다. (2026-11-15 WB04-E558)
const COLOR = 0x47;
const RANK = 0x4f;
const SPECIAL = 0x3f;

test('슬롯 색 증가: BYTE 증가를 먼저 저장하고 7 보다 클 때만 0 을 쓴다', () => {
    // 0..6 은 단순 증가, 7 은 8>7 이므로 0, 큰 입력값도 증가 뒤 7 초과이면 0 이다.
    for (const [before, after] of [[0, 1], [1, 2], [6, 7], [7, 0], [8, 0], [100, 0], [254, 0]]) {
        const options = makeOptions();
        options[COLOR + 3] = before;
        const snapshot = Uint8Array.from(options);
        assert.equal(increaseBattleSlotColor(options, 3), 1);
        snapshot[COLOR + 3] = after;
        assert.deepEqual(options, snapshot, `색 증가 ${before}`);
    }
    // 255+1 은 BYTE 0 으로 먼저 감기며 0 은 7 이하이므로 그대로 0 이다.
    const wrapped = makeOptions();
    wrapped[COLOR] = 255;
    assert.equal(increaseBattleSlotColor(wrapped, 0), 1);
    assert.equal(wrapped[COLOR], 0);
});

test('슬롯 색 감소: BYTE 0xff 가 된 경우만 7 로 바꾼다', () => {
    for (const [before, after] of [[0, 7], [1, 0], [7, 6], [8, 7], [100, 99], [255, 254]]) {
        const options = makeOptions();
        options[COLOR + 5] = before;
        const snapshot = Uint8Array.from(options);
        assert.equal(decreaseBattleSlotColor(options, 5), 1);
        snapshot[COLOR + 5] = after;
        assert.deepEqual(options, snapshot, `색 감소 ${before}`);
    }
});

test('슬롯 랭크 증가/감소: zero 확장 값으로 23 순환을 판정하고 최종 BYTE 만 쓴다', () => {
    // 증가: 0..22 는 +1, 23 은 24>23 이므로 0, 24..255 도 모두 0 이다.
    for (const [before, after] of [[0, 1], [10, 11], [22, 23], [23, 0], [24, 0], [200, 0], [255, 0]]) {
        const options = makeOptions();
        options[RANK + 2] = before;
        const snapshot = Uint8Array.from(options);
        assert.equal(increaseBattleSlotRank(options, 2), 1);
        snapshot[RANK + 2] = after;
        assert.deepEqual(options, snapshot, `랭크 증가 ${before}`);
    }
    // 감소: 0 에서만 23 이고 그 밖의 큰 입력값은 그대로 1 감소한다.
    for (const [before, after] of [[0, 23], [1, 0], [23, 22], [24, 23], [255, 254]]) {
        const options = makeOptions();
        options[RANK + 6] = before;
        const snapshot = Uint8Array.from(options);
        assert.equal(decreaseBattleSlotRank(options, 6), 1);
        snapshot[RANK + 6] = after;
        assert.deepEqual(options, snapshot, `랭크 감소 ${before}`);
    }
});

test('슬롯 랭크 조회: 24칸 표를 읽고 표 밖 BYTE 는 호스트 오류이다', () => {
    assert.deepEqual([...BATTLE_SLOT_RANKS], [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 35, 40, 45, 50, 60, 70, 80, 90, 100]);
    for (let index = 0; index < 24; index += 1) {
        const options = makeOptions();
        options[RANK + 4] = index;
        assert.equal(battleSlotRank(options, 4), BATTLE_SLOT_RANKS[index]);
    }
    // 읽기 전용이므로 읽기 전용 view 도 받는다.
    const frozenView = makeOptions();
    frozenView[RANK + 4] = 23;
    const before = Uint8Array.from(frozenView);
    battleSlotRank(frozenView, 4);
    assert.deepEqual(frozenView, before);
    for (const bad of [24, 25, 255]) {
        const options = makeOptions();
        options[RANK + 4] = bad;
        assert.throws(() => battleSlotRank(options, 4), RangeError);
    }
});

test('슬롯 특수 상태: 정확히 1 이면 0, 그 밖이면 1 이다(XOR 가 아님)', () => {
    for (const [before, after] of [[0, 1], [1, 0], [2, 1], [3, 1], [255, 1]]) {
        const options = makeOptions();
        options[SPECIAL + 7] = before;
        const snapshot = Uint8Array.from(options);
        assert.equal(toggleBattleSlotSpecial(options, 7), 1);
        snapshot[SPECIAL + 7] = after;
        assert.deepEqual(options, snapshot, `특수 ${before}`);
    }
});

test('유효 슬롯 관문: signed 0..7 밖은 읽기/쓰기 없이 0 이며 옵션 배열보다 먼저 판정한다', () => {
    const writers = [increaseBattleSlotColor, decreaseBattleSlotColor, increaseBattleSlotRank, decreaseBattleSlotRank, battleSlotRank, toggleBattleSlotSpecial];
    for (const slot of [-1, 8, 9, 255, -0x80000000, 0x7fffffff]) {
        for (const operation of writers) {
            const options = makeOptions();
            const snapshot = Uint8Array.from(options);
            assert.equal(operation(options, slot), 0);
            assert.deepEqual(options, snapshot);
            // 무효 슬롯은 옵션이 비어 있어도 물리 오류 없이 0 이다.
            assert.equal(operation(new Uint8Array(0), slot), 0);
        }
    }
    // 유효 경계 0 과 7 은 통과한다.
    assert.equal(toggleBattleSlotSpecial(makeOptions(), 0), 1);
    assert.equal(toggleBattleSlotSpecial(makeOptions(), 7), 1);
    // 호출 규약: int32 가 아닌 슬롯은 거부한다.
    for (const bad of [0.5, NaN, 0x80000000, -0x80000001, '1', undefined]) assert.throws(() => increaseBattleSlotColor(makeOptions(), bad), RangeError);
});

test('물리 범위: 실제 필드 BYTE 가 옵션 밖이면 쓰기·읽기 전에 RangeError 이다', () => {
    // 각 필드의 슬롯 7 위치 바로 아래 길이에서는 거부하고 바로 그 길이에서는 통과한다.
    const cases = [[increaseBattleSlotColor, COLOR], [decreaseBattleSlotColor, COLOR], [increaseBattleSlotRank, RANK], [decreaseBattleSlotRank, RANK], [battleSlotRank, RANK], [toggleBattleSlotSpecial, SPECIAL]];
    for (const [operation, field] of cases) {
        const exact = new Uint8Array(field + 8);
        assert.doesNotThrow(() => operation(exact, 7));
        const short = new Uint8Array(field + 7);
        assert.throws(() => operation(short, 7), RangeError);
        // 낮은 슬롯은 같은 길이에서도 통과한다.
        assert.doesNotThrow(() => operation(short, 6));
    }
    // 실패한 쓰기는 옵션을 건드리지 않는다.
    const short = new Uint8Array(COLOR + 7).fill(5);
    assert.throws(() => increaseBattleSlotColor(short, 7), RangeError);
    assert.ok(short.every((byte) => byte === 5));
});

test('자원 옵션: +0x14·+0x15·+0x13 의 현재 선택값을 원문 표로 읽는다', () => {
    const options = new Uint8Array(BATTLE_OPTIONS_BYTES);
    assert.equal(BATTLE_OPTIONS_BYTES, 87);
    const periods = [0, 30, 60, 120, 300, 600];
    for (let index = 0; index < 6; index += 1) {
        options[20] = index;
        assert.equal(battleIncomePeriod(options), periods[index]);
    }
    const amounts = [500, 1000, 2500, 5000];
    for (let index = 0; index < 4; index += 1) {
        options[21] = index;
        assert.equal(battleIncomeAmount(options), amounts[index]);
    }
    const geysers = [0, 30, 60, 120, 300];
    for (let index = 0; index < 5; index += 1) {
        options[19] = index;
        assert.equal(battleGeyserPeriod(options), geysers[index]);
    }
});

test('자원 옵션: 세 필드는 서로 독립이고 호출 시점의 현재 옵션을 읽는다', () => {
    const options = new Uint8Array(87);
    options[19] = 4;
    options[20] = 5;
    options[21] = 3;
    assert.equal(battleGeyserPeriod(options), 300);
    assert.equal(battleIncomePeriod(options), 600);
    assert.equal(battleIncomeAmount(options), 5000);
    // 기본값 사본이 없으므로 옵션을 바꾸면 다음 호출에 바로 반영된다.
    options[20] = 1;
    assert.equal(battleIncomePeriod(options), 30);
    assert.equal(battleGeyserPeriod(options), 300);
});

test('자원 옵션: 선택 범위 밖과 87바이트가 아닌 옵션은 호스트 오류이다', () => {
    const options = new Uint8Array(87);
    for (const [field, operation, count] of [[20, battleIncomePeriod, 6], [21, battleIncomeAmount, 4], [19, battleGeyserPeriod, 5]]) {
        options.fill(0);
        options[field] = count - 1;
        assert.doesNotThrow(() => operation(options));
        options[field] = count;
        assert.throws(() => operation(options), RangeError);
        options[field] = 255;
        assert.throws(() => operation(options), RangeError);
    }
    // 크기가 87 이 아니면 선택 BYTE 가 유효해도 거부한다.
    for (const size of [0, 86, 88]) {
        for (const operation of [battleIncomePeriod, battleIncomeAmount, battleGeyserPeriod]) assert.throws(() => operation(new Uint8Array(size)), RangeError);
    }
});
