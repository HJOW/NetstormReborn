// 색 변환표 소단위 sprite-color-maps.js(buildSpriteColorMaps)·preview-color-maps.js(buildPreviewColorMaps)·production-color-maps.js(buildProductionColorMaps)를
// 원본 10.78 전체 ColorSort 0043c010 관측과 계약으로 대조한다.
//  - 원본 관측(각 3행: 실제 GIFCLOUD 파일 팔레트·혼합 RGB·중복 회색 입력의 논리 팔레트 256색): sprite-colors-x86.tsv 의 밝음/건설/그림자 표, preview-colors-x86.tsv 의 파괴/불법/글자 그림자 표
//    (현재 기본 빨강 포함), production-colors-x86.tsv 의 비활성/선행 표가 256바이트 전부 같다.
//  - 계약: 팔레트 길이 거부, 입력 불변, 결과 동결, 그림자 표의 항등 꼬리(228..245·254·255), 파괴 표의 빨강 하위 BYTE, 글자 그림자/선행 표의 0, 불법·비활성 표가 같은 계산이라는 관계.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E373: 5-A 색표 소단위 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { buildSpriteColorMaps } from '../www/core/sprite-color-maps.js';
import { buildPreviewColorMaps } from '../www/core/preview-color-maps.js';
import { buildProductionColorMaps } from '../www/core/production-color-maps.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E373)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const spriteRows = fixture('sprite-colors-x86.tsv');
const previewRows = fixture('preview-colors-x86.tsv');
const productionRows = fixture('production-colors-x86.tsv');

// 16진 문자열을 바이트 배열로 읽는다(각 원본 BYTE 를 그대로). (2026-11-13 WB04-E373)
const bytes = (hex) => Uint8Array.from(Buffer.from(hex, 'hex'));

// 원본 RGB/예약 DWORD 256개(리틀 엔디언 1024바이트)를 논리 팔레트로 읽는다. (2026-11-13 WB04-E373)
function logicalOf(hex) {
    const raw = Buffer.from(hex, 'hex');
    assert.equal(raw.length, 1024);
    return Uint32Array.from({ length: 256 }, (_, index) => raw.readUInt32LE(index * 4));
}

test('SpriteColorMaps: 원본 10.78 세 입력의 밝음·건설·그림자 표 256바이트가 모두 같다', { skip: !spriteRows && '원본 fixture 없음' }, () => {
    let count = 0;
    let different = 0;
    // 세 입력의 색 256개 × 세 표를 동결한 원문 그대로 대조한다.
    for (const row of spriteRows) {
        assert.equal(row.length, 6);
        const maps = buildSpriteColorMaps(logicalOf(row[2]));
        assert.deepEqual(maps.bright, bytes(row[3]), `행 ${count} 밝음`);
        assert.deepEqual(maps.dead, bytes(row[4]), `행 ${count} 건설`);
        assert.deepEqual(maps.shadow, bytes(row[5]), `행 ${count} 그림자`);
        if (Buffer.compare(maps.dead, maps.shadow) !== 0) different += 1;
        count += 1;
    }
    assert.ok(count === 3 && different >= 2);
});

test('PreviewColorMaps: 원본 10.78 세 입력의 파괴·불법·글자 그림자 표 256바이트가 모두 같다', { skip: !previewRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 세 입력을 각각 독립 논리 팔레트와 현재 기본 빨강으로 재생한다.
    for (const row of previewRows) {
        assert.equal(row.length, 6);
        const maps = buildPreviewColorMaps(logicalOf(row[2]), Number(row[1]));
        assert.deepEqual(maps.destroy, bytes(row[3]), `행 ${count} 파괴`);
        assert.deepEqual(maps.illegal, bytes(row[4]), `행 ${count} 불법`);
        assert.deepEqual(maps.labelShadow, bytes(row[5]), `행 ${count} 글자 그림자`);
        count += 1;
    }
    assert.equal(count, 3);
});

test('ProductionColorMaps: 원본 10.78 세 입력의 비활성·선행 표 256바이트가 모두 같다', { skip: !productionRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 세 입력의 비활성/선행 표를 원문 그대로 대조한다.
    for (const row of productionRows) {
        assert.equal(row.length, 5);
        const maps = buildProductionColorMaps(logicalOf(row[2]));
        assert.deepEqual(maps.disabled, bytes(row[3]), `행 ${count} 비활성`);
        assert.deepEqual(maps.underlay, bytes(row[4]), `행 ${count} 선행`);
        count += 1;
    }
    assert.equal(count, 3);
});

// 계약 시험용 합성 팔레트: 색 i 를 (i, 255 - i, (i * 7) & 255) 로 만든다(모든 후보가 서로 다른 RGB 라 동률이 없다). (2026-11-13 WB04-E373)
const synthetic = () => Uint32Array.from({ length: 256 }, (_, index) => (index | ((255 - index) << 8) | (((index * 7) & 255) << 16)) >>> 0);

test('색표 소단위: 합성 팔레트 계약 — 길이 거부·입력 불변·결과 동결·고정 구간과 채움 값', () => {
    const logical = synthetic();
    const before = Uint32Array.from(logical);
    const sprite = buildSpriteColorMaps(logical);
    const preview = buildPreviewColorMaps(logical, 0x1bd);
    const production = buildProductionColorMaps(logical);
    // 입력 팔레트는 바뀌지 않고 결과는 동결되며 모든 표가 256바이트이다.
    assert.deepEqual(logical, before);
    for (const maps of [sprite, preview, production]) {
        assert.ok(Object.isFrozen(maps));
        for (const table of Object.values(maps)) assert.ok(table instanceof Uint8Array && table.length === 256);
    }
    // 그림자 표의 예약 꼬리는 항등이다(228..245, 254, 255).
    for (let index = 228; index <= 245; index += 1) assert.equal(sprite.shadow[index], index);
    assert.equal(sprite.shadow[254], 254);
    assert.equal(sprite.shadow[255], 255);
    // 파괴 표는 빨강의 하위 BYTE, 글자 그림자와 선행 표는 0 이다.
    assert.ok(preview.destroy.every((value) => value === 0xbd));
    assert.ok(preview.labelShadow.every((value) => value === 0));
    assert.ok(production.underlay.every((value) => value === 0));
    // 불법 표와 비활성 표는 같은 계산이다(소스 빨강 2.1배·blue→green→red 제곱 거리).
    assert.deepEqual(preview.illegal, production.disabled);
    // 합성 팔레트에서는 모든 색이 서로 달라 고정 구간 밖 그림자 표가 자기 자신이 아닌 어두운 색을 고른다(대부분 자신보다 작은 빨강 쪽).
    assert.ok(sprite.shadow.some((value, index) => index < 228 && value !== index));
    // 길이가 256 이 아니거나 없으면 거부한다.
    for (const build of [buildSpriteColorMaps, (value) => buildPreviewColorMaps(value, 0), buildProductionColorMaps]) {
        assert.throws(() => build(new Uint32Array(255)), RangeError);
        assert.throws(() => build(new Uint32Array(257)), RangeError);
        assert.throws(() => build(null), RangeError);
    }
});

test('색표 소단위: 파괴 표 채움은 빨강 인자의 하위 BYTE 만 쓰고 0·255·256 경계를 지킨다', () => {
    const logical = synthetic();
    for (const [red, expected] of [[0, 0], [255, 255], [256, 0], [0x1bd, 0xbd], [-1, 255]]) {
        assert.ok(buildPreviewColorMaps(logical, red).destroy.every((value) => value === expected), `빨강 ${red}`);
    }
});

test('색표 소단위: 거리가 정확히 0 인 동률은 앞 번호를 고르고 반올림으로 늘어난 최소 거리는 뒤 번호가 다시 이긴다(엄격 비교 뒤 float 저장)', () => {
    // 검정 단색: 목표와 모든 후보의 거리가 정확히 0 이라 첫 후보(0)만 최소로 저장된다.
    const black = new Uint32Array(256);
    assert.ok(buildPreviewColorMaps(black, 0).illegal.every((value) => value === 0));
    assert.ok(buildProductionColorMaps(black).disabled.every((value) => value === 0));
    const blackSprite = buildSpriteColorMaps(black);
    for (let index = 0; index < 228; index += 1) assert.equal(blackSprite.shadow[index], 0);
    // 단색 0x336699: 거리 d 가 모든 후보에서 같지만 float32 로 저장한 최소가 d 보다 크게 반올림되면(d < fround(d)) 다음 후보가 다시 최소가 되어 마지막 번호(255)까지 간다.
    // 반대로 정확히 표현되거나 작게 반올림되면 첫 번호(0)에 머문다(원본 PaletteShade 주석의 '동일 RGB도 저장 반올림에 따라 뒤 번호'). 기대 번호는 이 규칙에서만 정한다.
    const flat = new Uint32Array(256).fill(0x00336699);
    const red = 0x99 / 255;
    const target = Math.min(1.0, red * 2.1);
    const difference = target - red;
    const distance = 0 * 0 + 0 * 0 + difference * difference;
    const expected = distance < Math.fround(distance) ? 255 : 0;
    assert.ok(buildPreviewColorMaps(flat, 0).illegal.every((value) => value === expected));
    assert.ok(buildProductionColorMaps(flat).disabled.every((value) => value === expected));
    // 예약 BYTE(최상위)는 거리에 쓰지 않는다: 최상위 바이트만 다른 두 팔레트의 표가 같다.
    const withFlags = Uint32Array.from(synthetic(), (value) => (value | 0xff000000) >>> 0);
    assert.deepEqual(buildSpriteColorMaps(withFlags).shadow, buildSpriteColorMaps(synthetic()).shadow);
    assert.deepEqual(buildPreviewColorMaps(withFlags, 0).illegal, buildPreviewColorMaps(synthetic(), 0).illegal);
    assert.deepEqual(buildProductionColorMaps(withFlags).disabled, buildProductionColorMaps(synthetic()).disabled);
});
