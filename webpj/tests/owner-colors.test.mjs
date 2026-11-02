// 소유자 색 변환표(owner-colors.js)를 검사한다.
//  1) cpppj 가 원본 실행 파일(10.78)을 제한 x86 으로 실행해 얻어 둔 fixture 와 바이트 단위로 대조한다:
//     - owner-hsl-colors-x86.tsv: 팔레트 3벌 × 전용 표 5종(타입 111·141·137·94·97)의 9 × 256바이트 표(CRT 기본 정밀도 53비트 출력). 열: 팔레트 번호, 타입, 논리 팔레트 DWORD 256개, 53비트 표, 64비트 표.
//     - owner-capture-x86.tsv 의 M 줄 18개: 일반 띠 표(후보 72개 + 타입 + 다리 타입 → 9 × 256바이트).
//  2) 후보표 72개가 원본 fixture 의 후보와 같고, 항등 행·소스 색·보정값·같은 거리의 앞 후보·등록 목록(createOwnerColorTables)이 설명한 규칙을 지키는지 본다.
// fixture 가 없는 환경에서는 원본 실행 대조를 건너뛴다.
// (2026-11-02 WB03-Y: 정적 맵 화면(M2) 3단계(c) 소유자 색 변환 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import {
    BRIDGE_TYPE, ISLAND_TYPE, ISLE_TYPE, OWNER_COLOR_CANDIDATES, OWNER_COLOR_COUNT, OWNER_COLOR_ROW, OWNER_COLOR_TYPES,
    applyOwnerColor, buildSpecialOwnerColorMaps, buildStandardOwnerColorMaps, createOwnerColorTables, paletteLightness, specialOwnerColorSources,
} from '../www/assets/owner-colors.js';

// 원본 실행 fixture 경로이다. (2026-11-02 WB03-Y)
const HSL_FIXTURE = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'owner-hsl-colors-x86.tsv');
const CAPTURE_FIXTURE = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'owner-capture-x86.tsv');

// 탭으로 나눈 fixture 줄을 읽는다(`#` 줄 제외). 파일이 없으면 빈 배열이다. (2026-11-02 WB03-Y)
function readRows(file) {
    if (!fs.existsSync(file)) return [];
    return fs.readFileSync(file, 'utf8').split(/\r?\n/).filter((line) => line !== '' && !line.startsWith('#')).map((line) => line.split('\t'));
}

// 16진수 글을 바이트 배열로 바꾼다. (2026-11-02 WB03-Y)
function hexBytes(text) {
    return Uint8Array.from(text.match(/../g), (pair) => parseInt(pair, 16));
}

// 바이트 배열에서 리틀 엔디언 DWORD 를 읽는다. (2026-11-02 WB03-Y)
function dwords(bytes) {
    const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    return Uint32Array.from({ length: bytes.length / 4 }, (_, i) => view.getUint32(i * 4, true));
}

// 9행 표(Uint8Array 9개)를 이은 2304바이트 배열이다. (2026-11-02 WB03-Y)
function flatten(maps) {
    const out = new Uint8Array(maps.length * OWNER_COLOR_ROW);
    maps.forEach((row, i) => out.set(row, i * OWNER_COLOR_ROW));
    return out;
}

const hslRows = readRows(HSL_FIXTURE);
const captureRows = readRows(CAPTURE_FIXTURE).filter((row) => row[0] === 'M');

test('원본 실행 fixture: 명도 전용 표 5종 × 팔레트 3벌(15줄)의 9 × 256바이트가 53비트 기본 출력과 같다', { skip: hslRows.length === 0 }, () => {
    let count = 0;
    let different = 0;
    // 줄마다 팔레트·타입을 재생한다.
    for (const row of hslRows) {
        assert.equal(row.length, 5);
        const logical = dwords(hexBytes(row[2]));
        assert.equal(logical.length, 256);
        const maps = buildSpecialOwnerColorMaps(logical, Number(row[1]));
        assert.deepEqual(flatten(maps), hexBytes(row[3]), `팔레트 ${row[0]} 타입 ${row[1]}`);
        if (row[3] !== row[4]) different += 1;
        count += 1;
    }
    assert.equal(count, 15);
    // 64비트 정밀도 관측은 53비트와 다른 줄이 있으며 기대값으로 섞지 않는다(자바스크립트 double 은 53비트 기본 출력과 같다).
    assert.ok(different > 0);
});

test('원본 실행 fixture: 일반 띠 표 18줄(타입·다리 타입·후보 72개)의 9 × 256바이트가 같고 첫 줄의 후보는 내장 후보표와 같다', { skip: captureRows.length === 0 }, () => {
    let count = 0;
    // 줄마다 후보 DWORD 72개를 읽어 표를 만든다.
    for (const row of captureRows) {
        assert.equal(row.length, 5);
        const candidates = Array.from(dwords(hexBytes(row[3])));
        assert.equal(candidates.length, 72);
        if (count === 0) assert.deepEqual(candidates, Array.from(OWNER_COLOR_CANDIDATES));
        const maps = buildStandardOwnerColorMaps(Number(row[1]), Number(row[2]), candidates);
        assert.deepEqual(flatten(maps), hexBytes(row[4]), `타입 ${row[1]} 다리 ${row[2]}`);
        count += 1;
    }
    assert.equal(count, 18);
});

test('후보표·전용 소스·등록 타입 목록이 원본 값 그대로이다', () => {
    assert.equal(OWNER_COLOR_CANDIDATES.length, 72);
    assert.deepEqual(Array.from(OWNER_COLOR_CANDIDATES.slice(0, 16)), [0, 0, 0, 0, 0, 0, 0, 0, 81, 80, 76, 145, 146, 147, 4, 10]);
    assert.equal(specialOwnerColorSources(ISLE_TYPE).length, 33);
    assert.deepEqual(specialOwnerColorSources(ISLE_TYPE), specialOwnerColorSources(ISLAND_TYPE));
    assert.equal(specialOwnerColorSources(111).length, 20);
    assert.equal(specialOwnerColorSources(141).length, 21);
    assert.equal(specialOwnerColorSources(137).length, 19);
    assert.throws(() => specialOwnerColorSources(82), RangeError);
    assert.deepEqual(Array.from(OWNER_COLOR_TYPES.banner), [85, 71, 128, 97, 94]);
    assert.deepEqual(Array.from(OWNER_COLOR_TYPES.special), [111, 141, 137]);
    assert.equal(OWNER_COLOR_TYPES.standard.length, 12);
    assert.equal(ISLE_TYPE, 97);
    assert.equal(ISLAND_TYPE, 94);
    assert.equal(BRIDGE_TYPE, 82);
});

test('일반 띠 표: 228~245 만 바꾸고 0행·그 밖의 색은 항등이며 다리는 소유자 2·7 의 238~245 를 고정한다', () => {
    const maps = buildStandardOwnerColorMaps(164, BRIDGE_TYPE);
    assert.equal(maps.length, OWNER_COLOR_COUNT + 1);
    assert.deepEqual(Array.from(maps[0]), Array.from({ length: 256 }, (_, i) => i));
    // 소유자 1: 228~237 은 밝기 0 후보, 238~245 는 밝기 7~0 후보이다.
    for (let color = 0; color < 256; color += 1) {
        const expected = color < 228 || color > 245 ? color : OWNER_COLOR_CANDIDATES[8 + (color < 238 ? 0 : 245 - color)];
        assert.equal(maps[1][color], expected, `소유자 1 색 ${color}`);
    }
    // 다리 타입이 아니면 소유자 2 의 238 은 밝기 7, 다리면 밝기 4 로 고정된다.
    assert.equal(maps[2][238], OWNER_COLOR_CANDIDATES[16 + 7]);
    const bridge = buildStandardOwnerColorMaps(BRIDGE_TYPE, BRIDGE_TYPE);
    assert.equal(bridge[2][238], OWNER_COLOR_CANDIDATES[16 + 4]);
    assert.equal(bridge[2][245], OWNER_COLOR_CANDIDATES[16 + 4]);
    assert.equal(bridge[7][238], OWNER_COLOR_CANDIDATES[56 + 3]);
    // 소유자 3 은 다리여도 일반 규칙이다.
    assert.equal(bridge[3][238], OWNER_COLOR_CANDIDATES[24 + 7]);
    assert.throws(() => buildStandardOwnerColorMaps(1, 82, [0, 1, 2]), RangeError);
});

test('명도 전용 표: 소스 색만 바꾸고 0행은 항등이며 지면 표는 타입 97 만 소유자별 보정값을 쓴다', () => {
    // 합성 팔레트: 색 i 의 R=G=B=i (명도 = i ÷ 255).
    const logical = Uint32Array.from({ length: 256 }, (_, i) => i | (i << 8) | (i << 16));
    const light = paletteLightness(logical);
    assert.ok(Math.abs(light[100] - 100 / 255) < 1e-12);
    const isle = buildSpecialOwnerColorMaps(logical, ISLE_TYPE);
    const island = buildSpecialOwnerColorMaps(logical, ISLAND_TYPE);
    assert.deepEqual(Array.from(isle[0]), Array.from({ length: 256 }, (_, i) => i));
    const sources = new Set(specialOwnerColorSources(ISLE_TYPE));
    // 소스가 아닌 색은 모든 행에서 항등이다.
    for (let owner = 1; owner <= 8; owner += 1) for (let color = 0; color < 256; color += 1) if (!sources.has(color)) assert.equal(isle[owner][color], color);
    // 소스 색은 소유자의 후보 8개 안의 색으로 바뀐다.
    for (let owner = 1; owner <= 8; owner += 1) {
        const candidates = new Set(Array.from(OWNER_COLOR_CANDIDATES.slice(owner * 8, owner * 8 + 8)));
        for (const source of sources) {
            assert.ok(candidates.has(isle[owner][source]), `isle 소유자 ${owner} 소스 ${source}`);
            assert.ok(candidates.has(island[owner][source]), `island 소유자 ${owner} 소스 ${source}`);
        }
    }
    // 보정값이 있는 isle 과 없는 island 는 소유자 3(보정 +0.15)에서 다른 색을 고르는 소스가 있다.
    assert.notDeepEqual(isle[3], island[3]);
    // 보정값이 0 인 소유자 4·5·8 은 isle 과 island 가 같다.
    for (const owner of [4, 5, 8]) assert.deepEqual(isle[owner], island[owner]);
});

test('명도 전용 표: 같은 거리는 앞 후보, 첫 후보 거리는 99 로 자른다', () => {
    // 후보 8개의 명도가 모두 같은 팔레트에서는 항상 첫 후보(밝기 0 번호)가 선택된다.
    const flat = new Uint32Array(256).fill(0x808080);
    const maps = buildSpecialOwnerColorMaps(flat, ISLAND_TYPE);
    for (let owner = 1; owner <= 8; owner += 1) for (const source of specialOwnerColorSources(ISLAND_TYPE)) assert.equal(maps[owner][source], OWNER_COLOR_CANDIDATES[owner * 8]);
    assert.throws(() => buildSpecialOwnerColorMaps(new Uint32Array(255), ISLE_TYPE), RangeError);
});

test('등록 모음: 일반 12 + 전용 3 + (배너 옵션이 0 이면) 지면 97·받침 윗면 94 의 표가 만들어지고 적용 함수가 행을 따른다', () => {
    const palette = Uint32Array.from({ length: 256 }, (_, i) => i | (((i * 7) & 255) << 8) | (((i * 13) & 255) << 16));
    const shown = createOwnerColorTables(palette, { useBanners: 0 });
    assert.equal(shown.size, 12 + 3 + 2);
    assert.ok(shown.has(ISLE_TYPE) && shown.has(ISLAND_TYPE));
    const hidden = createOwnerColorTables(palette, { useBanners: 1 });
    assert.equal(hidden.size, 12 + 3);
    assert.ok(!hidden.has(ISLE_TYPE) && !hidden.has(ISLAND_TYPE));
    // 다리 타입 표는 소유자 2 의 후반 명도가 고정된다.
    assert.equal(shown.get(82)[2][240], OWNER_COLOR_CANDIDATES[16 + 4]);
    // 처음 두 일반 표를 건너뛰는 설정.
    const skipped = createOwnerColorTables(palette, { skipFirstTwo: true });
    assert.equal(skipped.has(82), false);
    assert.equal(skipped.has(155), false);
    assert.equal(skipped.has(158), true);
    // 색 번호 줄 변환.
    const pixels = Uint8Array.from([0, 44, 64, 228, 245, 255]);
    const recolored = applyOwnerColor(shown.get(ISLE_TYPE), 1, pixels);
    assert.equal(recolored[0], 0);
    assert.equal(recolored[5], 255);
    assert.deepEqual(applyOwnerColor(shown.get(ISLE_TYPE), 0, pixels), pixels);
    // 마지막 소유자 색 번호 8 까지는 변환되고, 소수·음수는 거부한다.
    assert.equal(applyOwnerColor(shown.get(BRIDGE_TYPE), 8, Uint8Array.of(240))[0], shown.get(BRIDGE_TYPE)[8][240]);
    assert.throws(() => applyOwnerColor(shown.get(ISLE_TYPE), -1, pixels), RangeError);
    assert.throws(() => applyOwnerColor(shown.get(ISLE_TYPE), 1.5, pixels), RangeError);
    assert.throws(() => applyOwnerColor(shown.get(ISLE_TYPE), 9, pixels), RangeError);
});
