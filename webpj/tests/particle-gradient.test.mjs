// 입자 색 띠 particle-gradient.js(buildParticleGradient·mergeParticleGradients·buildFieryTailPalette·buildLightningPalette)를 실제 PE 관측 fixture 로 대조한다.
//  - particle-gradient-x86.tsv: 기본 027f 24입력(각 입력 = 실제 COL 팔레트 256색 + 두 색 띠 A/B(시작/끝 RGB double 6개·색 수·폭)의 전체 256항목 의미 48바이트(RGB 차 세 double·색 번호·거리·축 위치·선택 표식)와 머리(시작/끝/선택 수/임계 비트), 1/2/3색 띠 병합 번호 전부).
//    같은 입력의 037f(비교 정밀도) 행은 중복 가산하지 않고 건너뛴다. 부동소수점 허용오차 없이 DWORD/QWORD 전체를 비교한다.
//  - fierytail-lifetime-x86.tsv 의 10.78 cold 기본 생성자 3행(색 표 11개 번호)으로 불꽃 꼬리 표(어두운 빨강 → 주황 8색 + 주황 → 밝은색 3색)의 두 RGB 끝점 순서를 검증한다(나머지 행은 불꽃 꼬리 이식 단위의 몫).
//  - 번개 표(buildLightningPalette)는 raw-lightning-lifetime 시험의 공유 색 표 11개 대조에서 실제 PE 와 비교된다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E186: 입자 색 띠 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { buildFieryTailPalette, buildParticleGradient, mergeParticleGradients } from '../www/core/particle-gradient.js';

// 실제 PE 관측 fixture 이다(헤더 `#` 행 제외, CRLF 허용, 빈 마지막 열 보존). (2026-11-09 WB04-E186)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
function rowsOf(name) {
    return fs.readFileSync(path.join(FIXTURES, name), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));
}

// 16진 문자열을 BYTE 배열로 복구한다. (2026-11-09 WB04-E186)
const bytesOf = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16));

// 비정렬 4/8바이트 값을 호스트 구조체로 덮어씌우지 않고 읽는다(BigInt). (2026-11-09 WB04-E186)
function number(bytes, offset, width) {
    let result = 0n;
    for (let index = 0; index < width; index += 1) result |= BigInt(bytes[offset + index]) << BigInt(index * 8);
    return result;
}

// 실제 COL 에서 공급한 256개 RGBX 입력을 원래 색 번호 순서로 복구한다. (2026-11-09 WB04-E186)
function paletteOf(text) {
    const raw = bytesOf(text);
    assert.equal(raw.length, 1024);
    return Uint32Array.from({ length: 256 }, (_, index) => Number(number(raw, index * 4, 4)));
}

// double 의 원문 비트(BigInt)이다. (2026-11-09 WB04-E186)
function doubleBits(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true);
}

// 비트(BigInt 십진 문자열)를 double 로 바꾼다. (2026-11-09 WB04-E186)
function bitsDouble(text) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, BigInt(text), true);
    return view.getFloat64(0, true);
}

// 오직 fixture 입력의 RGB/요청 수/폭을 제품 함수에 공급한다(기대값 계산에는 사용하지 않음). (2026-11-09 WB04-E186)
function make(palette, row, column) {
    const rgb = row[column].split(',');
    assert.equal(rgb.length, 6);
    const values = rgb.map(bitsDouble);
    return buildParticleGradient(palette, values.slice(0, 3), values.slice(3, 6), Number(row[column + 1]), bitsDouble(row[column + 2]));
}

// 한 색 띠의 머리와 전체 256항목 의미 필드를 원본과 DWORD/QWORD 전체로 대조한다. (2026-11-09 WB04-E186)
function checkGradient(actual, header, payload, context) {
    const expected = header.split(',');
    const bytes = bytesOf(payload);
    assert.equal(expected.length, 4);
    assert.equal(bytes.length, 256 * 48);
    assert.ok(actual.from === Number(expected[0]) && actual.to === Number(expected[1]) && actual.selected === Number(expected[2]) && doubleBits(actual.threshold) === BigInt(expected[3]), `${context} 머리 값`);
    // 의미 48바이트의 실제 필드 위치/폭이다(HSV/패딩은 비교 범위가 아님).
    const offsets = [0, 8, 16, 24, 28, 36, 44];
    const widths = [8, 8, 8, 4, 8, 8, 4];
    actual.entries.forEach((entry, index) => {
        const values = [doubleBits(entry.translated[0]), doubleBits(entry.translated[1]), doubleBits(entry.translated[2]), BigInt(entry.index), doubleBits(entry.distance), doubleBits(entry.position), BigInt(entry.selected)];
        values.forEach((value, field) => assert.equal(value, number(bytes, index * 48 + offsets[field], widths[field]), `${context} 항목 ${index} 필드 ${field}`));
    });
}

// 원본 병합 결과의 DWORD 번호 전체와 요청 길이를 확인한다. (2026-11-09 WB04-E186)
function checkMerge(gradients, payload, context) {
    const expected = bytesOf(payload);
    const actual = mergeParticleGradients(gradients);
    assert.equal(actual.length * 4, expected.length, `${context} 병합 길이`);
    actual.forEach((value, index) => assert.equal(BigInt(value), number(expected, index * 4, 4), `${context} 병합 색 ${index}`));
}

test('입자 색 띠: 기본 027f 24입력의 48색 띠(전체 256항목 의미 필드)와 72 병합을 실제 PE 와 비트까지 대조한다', () => {
    const rows = rowsOf('particle-gradient-x86.tsv');
    let compared = 0;
    let extended = 0;
    // 기본/비교 정밀도의 독립 행을 읽되 제품 기본 정밀도만 완료로 센다.
    for (const row of rows) {
        assert.equal(row.length, 16);
        const precision = Number(row[1]);
        if (precision === 0x037f) { extended += 1; continue; }
        assert.equal(precision, 0x027f);
        const palette = paletteOf(row[2]);
        const a = make(palette, row, 3);
        const b = make(palette, row, 6);
        checkGradient(a, row[9], row[10], `${row[0]} A`);
        checkGradient(b, row[11], row[12], `${row[0]} B`);
        checkMerge([a, b], row[13], `${row[0]} 2색 띠`);
        checkMerge([a, b, a], row[14], `${row[0]} 3색 띠`);
        checkMerge([a], row[15], `${row[0]} 1색 띠`);
        compared += 1;
    }
    assert.equal(compared, 24);
    assert.equal(extended, 24);
});

test('입자 색 띠: 불꽃 꼬리 cold 생성자 3행의 8+3 색 표를 두 RGB 끝점 순서로 실제 PE 와 대조한다', () => {
    let count = 0;
    // 단위 99 생성자의 cold 기본 입력만 고른다(명시 RGB helper 입력을 생성자의 증거로 쓰지 않음).
    for (const row of rowsOf('fierytail-lifetime-x86.tsv')) {
        assert.equal(row.length, 12);
        if (Number(row[1]) !== 0x027f || !row[2].includes('.COL,0,0,-1,50,127,0,1')) continue;
        const actual = buildFieryTailPalette(paletteOf(row[3]));
        const expected = bytesOf(row[6]);
        assert.ok(actual.length === 11 && expected.length === 44);
        actual.forEach((value, index) => assert.equal(BigInt(value), number(expected, index * 4, 4), `불꽃 꼬리 색 ${index}`));
        count += 1;
    }
    assert.equal(count, 3);
});

test('입자 색 띠: 잘못된 색 수/폭/비유한 RGB/후보 수 범위/영길이 축/병합 개수를 거부한다', () => {
    const palette = Uint32Array.from({ length: 256 }, (_, index) => (index | (index << 8) | (index << 16)) >>> 0);
    const black = [0, 0, 0];
    const white = [1, 1, 1];
    const throwsOf = (action) => { try { action(); } catch { return true; } return false; };
    assert.ok(throwsOf(() => buildParticleGradient(palette, black, white, 1)));
    assert.ok(throwsOf(() => buildParticleGradient(palette, black, white, 257)));
    assert.ok(throwsOf(() => buildParticleGradient(palette, black, white, 8, 0)));
    assert.ok(throwsOf(() => buildParticleGradient(palette, black, white, 8, Number.NaN)));
    assert.ok(throwsOf(() => buildParticleGradient(palette, [Number.NaN, 0, 0], white, 8)));
    assert.ok(throwsOf(() => buildParticleGradient(palette, black, white, 256, 2.4)));
    assert.ok(throwsOf(() => buildParticleGradient(palette, black, black, 8)));
    assert.ok(throwsOf(() => buildParticleGradient(new Uint32Array(255), black, white, 8)));
    assert.ok(throwsOf(() => mergeParticleGradients([])));
    const one = buildParticleGradient(palette, black, white, 8);
    assert.ok(throwsOf(() => mergeParticleGradients([one, one, one, one])));
    assert.ok(throwsOf(() => mergeParticleGradients([{ ...one, count: 257 }])));
});
