// 일반 자산 Draw 분배 조건 raw-plain-unit-capture.js(isPlainUnitCapture)·raw-plain-workshop-capture.js(isPlainWorkshopCapture)·raw-plain-asset-draw.js(isPlainAssetDraw)를
// cpppj 표(보호 PE 에서 확인한 TYPE/가상표 쌍)와 계약으로 대조한다. 기대 표는 시험 안에 원본 수치로 따로 적었다(모듈 상수를 재사용하지 않는다).
//  - 정확한 쌍: 유닛 13·워크샾 5·건물/제단/보호막/포스 필드 10 이 각각 참, 서로 다른 표의 쌍은 해당 판별기에서 거짓(isPlainAssetDraw 는 세 표의 합집합).
//  - 거부: TYPE 만 맞고 가상표가 다름(네 바이트 각각 +1), 가상표만 맞고 TYPE 이 다름(TYPE ±1), 슬롯 길이 49, 같은 가상표를 공유하는 쌍의 교차(78↔147, 111↔141 허용·그 밖 거부).
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E372: 5-A 착수의 `o/` 세 단위 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { isPlainUnitCapture } from '../www/core/raw-plain-unit-capture.js';
import { isPlainWorkshopCapture } from '../www/core/raw-plain-workshop-capture.js';
import { isPlainAssetDraw } from '../www/core/raw-plain-asset-draw.js';

// 보호 PE 에서 확인한 원본 표이다. 유닛은 운반/비행/일반 유닛과 애시드 145 의 실제 가상표, 워크샾은 아이스 캐논·썬 바리케이드·크로스보우·맨오워 풀·반더 타워이다.
const UNITS = [[70, 0x508cc8], [73, 0x50a1e8], [111, 0x5040c0], [133, 0x50e568], [135, 0x503e50], [137, 0x503d80], [138, 0x50a050], [141, 0x5040c0], [144, 0x50a388], [145, 0x50e738], [146, 0x504198], [156, 0x503f20], [158, 0x50f210]];
const WORKSHOPS = [[129, 0x50b9b8], [136, 0x509470], [140, 0x50b6e8], [143, 0x5099e8], [148, 0x50b7c0]];
const BUILDINGS = [[78, 0x50e940], [79, 0x50ea08], [139, 0x509aa0], [142, 0x50ead0], [147, 0x50e940], [149, 0x50e668], [150, 0x50e888], [164, 0x5076b8], [167, 0x507770], [170, 0x509388]];

// 길이 128 슬롯에 TYPE(+10)과 가상표(+0, 비정렬 little endian)를 쓴다. 나머지 바이트는 이웃 필드를 읽는 구현을 드러내도록 0xa5 로 채운다. (2026-11-13 WB04-E372)
function slot(type, table, length = 128) {
    const raw = new Uint8Array(length).fill(0xa5);
    raw[10] = type;
    for (let index = 0; index < 4; index += 1) raw[index] = Math.floor(table / 2 ** (index * 8)) & 0xff;
    return raw;
}

// 세 판별기의 결과를 [유닛, 워크샾, 전체] 로 묶는다. (2026-11-13 WB04-E372)
const verdict = (raw) => [isPlainUnitCapture(raw), isPlainWorkshopCapture(raw), isPlainAssetDraw(raw)];

test('PlainAssetDraw: 유닛 13쌍은 유닛·전체에서 참이고 워크샾은 거짓이다', () => {
    for (const [type, table] of UNITS) assert.deepEqual(verdict(slot(type, table)), [true, false, true], `유닛 ${type}`);
    assert.equal(UNITS.length, 13);
});

test('PlainAssetDraw: 워크샾 5쌍은 워크샾·전체에서 참이고 유닛은 거짓이다', () => {
    for (const [type, table] of WORKSHOPS) assert.deepEqual(verdict(slot(type, table)), [false, true, true], `워크샾 ${type}`);
    assert.equal(WORKSHOPS.length, 5);
});

test('PlainAssetDraw: 건물/제단/보호막/포스 필드 10쌍은 전체에서만 참이다', () => {
    for (const [type, table] of BUILDINGS) assert.deepEqual(verdict(slot(type, table)), [false, false, true], `건물 ${type}`);
    assert.equal(BUILDINGS.length, 10);
});

test('PlainAssetDraw: TYPE 만 맞거나 가상표만 맞으면 거부하고 가상표의 네 바이트를 모두 비교한다', () => {
    const all = [...UNITS, ...WORKSHOPS, ...BUILDINGS];
    for (const [type, table] of all) {
        // TYPE 이 표의 어떤 쌍과도 이 가상표로 맺어지지 않으면 거짓이다(표에 같은 가상표를 가진 다른 TYPE 이 있으면 그 TYPE 은 참).
        for (const wrong of [type - 1, type + 1, 0, 255]) {
            const expected = all.some(([otherType, otherTable]) => otherType === wrong && otherTable === table);
            assert.equal(isPlainAssetDraw(slot(wrong, table)), expected, `TYPE ${wrong} 가상표 ${table.toString(16)}`);
        }
        // 가상표의 각 바이트를 +1 하면(캐리 없이 해당 바이트만) 어떤 표의 쌍과도 맞지 않는다.
        for (let byte = 0; byte < 4; byte += 1) {
            const changed = table + 2 ** (byte * 8);
            const expected = all.some(([otherType, otherTable]) => otherType === type && otherTable === changed);
            assert.equal(isPlainAssetDraw(slot(type, changed)), expected, `TYPE ${type} 가상표 ${changed.toString(16)} 바이트 ${byte}`);
        }
    }
    // 같은 가상표를 공유하는 쌍은 TYPE 이 서로 다르지만 각각 참이고(78/147, 111/141) 교차한 TYPE 은 참이 아니다.
    assert.equal(isPlainAssetDraw(slot(78, 0x50e940)), true);
    assert.equal(isPlainAssetDraw(slot(147, 0x50e940)), true);
    assert.equal(isPlainAssetDraw(slot(111, 0x5040c0)), true);
    assert.equal(isPlainAssetDraw(slot(141, 0x5040c0)), true);
    assert.equal(isPlainAssetDraw(slot(79, 0x50e940)), false);
    assert.equal(isPlainAssetDraw(slot(128, 0x50b9b8)), false);
});

test('PlainAssetDraw: 슬롯 길이가 50 미만이면 모든 판별기가 거짓이고 50 이상이면 판정한다', () => {
    for (const [type, table] of [...UNITS, ...WORKSHOPS, ...BUILDINGS]) {
        assert.deepEqual(verdict(slot(type, table, 49)), [false, false, false], `길이 49 TYPE ${type}`);
        assert.equal(isPlainAssetDraw(slot(type, table, 50)), true, `길이 50 TYPE ${type}`);
    }
    // 유닛과 워크샾 판별기도 각각 길이 경계를 직접 지킨다.
    assert.equal(isPlainUnitCapture(slot(70, 0x508cc8, 49)), false);
    assert.equal(isPlainUnitCapture(slot(70, 0x508cc8, 50)), true);
    assert.equal(isPlainWorkshopCapture(slot(129, 0x50b9b8, 49)), false);
    assert.equal(isPlainWorkshopCapture(slot(129, 0x50b9b8, 50)), true);
});

test('PlainAssetDraw: 가상표는 낮은 네 바이트만 읽고 5번째 이후 바이트는 보지 않으며 높은 비트가 있는 값도 부호 없이 비교한다', () => {
    const raw = slot(70, 0x508cc8);
    raw[4] = 0xff;
    raw[5] = 0x00;
    assert.equal(isPlainUnitCapture(raw), true);
    // 최상위 바이트가 0x80 이상인 가상표도 부호 없는 값으로 읽어 표와 맞지 않는 쌍으로 거부한다.
    assert.equal(isPlainAssetDraw(slot(70, 0x808cc8c8)), false);
    assert.equal(isPlainAssetDraw(slot(70, 0xffffffff)), false);
    assert.equal(isPlainAssetDraw(new Uint8Array(128)), false);
});
