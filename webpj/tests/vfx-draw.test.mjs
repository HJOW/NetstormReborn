// VFX 셰이프 plain/mapped 그리기(vfx-draw.js·vfx-mapped-draw.js)와 SHP 추가 헤더(shapes.js squidHeader)를 검사한다.
//  - graphics-x86.tsv 709행: 원본 x86 그리기 함수의 반환값과 화면 전체(색 0/255·모든 방향 클리핑·원점 이동·오류 반환)를 대조한다(cpppj AssetTests VFXDraw_MatchesBothOriginalX86Editions).
//  - mapped 그리기와 squidHeader 는 cpppj 전용 fixture 가 없어 같은 몸체의 계약(불투명 소스 색만 치환·배경 유지·반환값 공유·포구 WORD 원문 보존)을 실제 보호 SHP 로 확인한다(웹 계약 검사).
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E199: ShapeToBuffer 선행 그리기 부품 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT, readData } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { drawShape } from '../www/render/vfx-draw.js';
import { drawMappedShape } from '../www/render/vfx-mapped-draw.js';

// 원본 x86 그리기 대조 fixture 이다(입력 열만 설치하고 마지막 두 열은 비교에만 쓴다). (2026-11-09 WB04-E199)
const graphicsRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'graphics-x86.tsv')) ?? [];

// 16진 문자열을 바이트로 바꾼다. '-' 는 빈 버퍼다. (2026-11-09 WB04-E199)
const unhex = (text) => (text === '-' ? new Uint8Array(0) : new Uint8Array(Buffer.from(text, 'hex')));

// 예외가 나는지 본다. (2026-11-09 WB04-E199)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

test('VFX plain 그리기: 원본 x86 709행의 반환값과 화면 전체가 같다', { skip: graphicsRows.length === 0 && 'graphics-x86.tsv 없음' }, () => {
    let count = 0;
    // 각 행의 셰이프·화면·pane·원점·배경으로 그린 뒤 원문 결과와 비교한다.
    for (const row of graphicsRows) {
        assert.equal(row.length, 12);
        const [shapeHex, width, height, left, top, right, bottom, x, y, background, expectedResult, expectedHex] = row;
        const database = new ShapeDatabase(unhex(shapeHex));
        const w = Number(width);
        const h = Number(height);
        const canvas = new Uint8Array(w > 0 && h > 0 ? w * h : 0).fill(Number(background));
        const actual = drawShape(canvas, w, h, database, 0, 0, Number(x), Number(y), { left: Number(left), top: Number(top), right: Number(right), bottom: Number(bottom) });
        assert.equal(actual, Number(expectedResult), `행 ${count} 반환`);
        assert.deepEqual(canvas, unhex(expectedHex), `행 ${count} 화면`);
        count += 1;
    }
    assert.equal(count, 709);
});

test('VFX mapped 그리기: 불투명 소스 색만 치환하고 배경·반환값·pane 은 plain 과 같다(웹 계약 검사)', () => {
    const database = new ShapeDatabase(readData('d/_shapes.shp'));
    const identity = Uint8Array.from({ length: 256 }, (_, index) => index);
    const reversed = Uint8Array.from({ length: 256 }, (_, index) => 255 - index);
    // 실제 보호 SHP 의 몇 블록/프레임과 화면 경계를 걸친 원점에서 비교한다.
    for (const [block, frame, x, y] of [[0, 0, 20, 30], [1, 2, -3, 5], [5, 0, 60, 70], [12, 1, 31, -4]]) {
        const pane = { left: 2, top: 1, right: 61, bottom: 50 };
        const plainA = new Uint8Array(64 * 52).fill(7);
        const plainB = new Uint8Array(64 * 52).fill(9);
        const resultA = drawShape(plainA, 64, 52, database, block, frame, x, y, pane);
        drawShape(plainB, 64, 52, database, block, frame, x, y, pane);
        const same = new Uint8Array(64 * 52).fill(7);
        assert.equal(drawMappedShape(same, 64, 52, database, block, frame, x, y, pane, identity), resultA);
        assert.deepEqual(same, plainA);
        const mapped = new Uint8Array(64 * 52).fill(7);
        assert.equal(drawMappedShape(mapped, 64, 52, database, block, frame, x, y, pane, reversed), resultA);
        let opaque = 0;
        // 두 배경에서 같은 값이면 그 픽셀은 불투명 소스 픽셀이다(색 7/9 와 같은 소스도 두 배경 모두에서 같게 남는다).
        for (let index = 0; index < plainA.length; index += 1) {
            const written = plainA[index] === plainB[index];
            if (written) opaque += 1;
            assert.equal(mapped[index], written ? reversed[plainA[index]] : 7, `블록 ${block} 프레임 ${frame} 픽셀 ${index}`);
        }
        if (resultA === 0) assert.ok(opaque > 0);
    }
    // 색표 길이와 출력 버퍼 크기를 확인한다.
    assert.ok(throws(() => drawMappedShape(new Uint8Array(16), 4, 4, database, 0, 0, 0, 0, { left: 0, top: 0, right: 3, bottom: 3 }, new Uint8Array(255))));
    assert.ok(throws(() => drawMappedShape(new Uint8Array(15), 4, 4, database, 0, 0, 0, 0, { left: 0, top: 0, right: 3, bottom: 3 }, identity)));
    assert.equal(drawMappedShape(new Uint8Array(0), 0, 4, database, 0, 0, 0, 0, { left: 0, top: 0, right: 3, bottom: 3 }, identity), -1);
    assert.ok(throws(() => drawShape(new Uint8Array(16), 4, 4, database, database.blocks.length, 0, 0, 0, { left: 0, top: 0, right: 3, bottom: 3 })));
});

test('SHP squidHeader: 프레임 앞 36바이트를 그대로 복사하고 squidMetrics 와 같은 위치를 읽는다(웹 계약 검사)', () => {
    const database = new ShapeDatabase(readData('d/_shapes.shp'));
    const header = database.squidHeader(3, 0);
    const metrics = database.squidMetrics(3, 0);
    const view = new DataView(header.buffer, header.byteOffset, header.byteLength);
    assert.equal(header.length, 36);
    assert.equal(view.getFloat32(0, true), metrics.cellWidth);
    assert.equal(view.getFloat32(20, true), metrics.walkStepY);
    assert.equal(view.getInt16(24, true), metrics.width);
    assert.equal(view.getInt16(30, true), metrics.hotspotY);
    // 복사본이므로 원본 바이트를 바꾸지 않는다.
    header[0] ^= 0xff;
    assert.notEqual(database.squidHeader(3, 0)[0], header[0]);
    // 순수 VFX(추가 헤더 없음)는 거부한다.
    const plain = new ShapeDatabase(unhex(graphicsRows[0]?.[0] ?? '312e31300100000010000000000000000200080000000000ffffffff010000000600000002000000010401010101031a020000100000'));
    assert.ok(throws(() => plain.squidHeader(0, 0)));
});
