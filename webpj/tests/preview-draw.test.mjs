// 커서 미리보기 그리기 render/preview-draw.js 를 원본 10.78 관측(previewdraw-x86.tsv originals 477행)과 cpppj `PreviewDrawTests.cpp` 의 공통 수집→정렬→분배→픽셀·픽셀 색/투명/clip·체크 무늬/윤곽·계약·격자 뒤 현재 상태 시험으로 검사한다.
//  - 관측 한 행 = 완성 입력 한 건(수집 상태·Draw 상태 40값, 방향 글자 바이트 열, 격자 144바이트, raw SID 5 의 타입/화면 WORD/프레임/좌표, tail)에서 원본이 낸 출력 명령열이다:
//    M(색 문맥 선택), S(그림: 타입·프레임·위치·모드(0 원색·3 불법 표·그 밖은 직전 색 문맥)), R(사각형: 체크 무늬 여부·좌표·색), T(문구 번호·위치). 픽셀/글꼴/번역만 경계이다. raw 슬롯은 Draw 전후 같아야 한다.
//    original1037·originalCD 행(각 477)은 웹 범위 밖(10.78 전용)이라 읽지 않는다.
//  - 이식한 cpppj 시험: PreviewDraw_PatchNative·CommonCollectionDispatch(실제 수집→CRT 정렬→kind 4 분배→VFX 픽셀)·PixelsColorsTransparencyClip·PixelsCheckerAndOutline·Contracts·CurrentStateAfterGrid.
//    실제 커서 전역에 묶는 `PatchNativeSharedCursor`·`SharedCursorCurrentReadsAndContracts`·`CurrentCursorProtectedAssetBinding` 은 커서 전역(K 영역) 단위가 옮겨진 뒤 건다. `makePreviewAssetShapes` 는 실제 자산으로 따로 확인한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E387)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { collectDrawEntries, dispatchDrawEntries, sortRenderEntries } from '../www/render/draw-submission.js';
import { PreviewCollection, PreviewCollectionState, bindPreviewCollection } from '../www/render/preview-collection.js';
import { PreviewColors, PreviewDraw, PreviewDrawState, PreviewPixelTarget, PreviewShapeMode, bindPreviewDraw, makePreviewAssetShapes } from '../www/render/preview-draw.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E387)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'previewdraw-x86.tsv'));
// native fixture 의 raw 객체와 호스트 풀 SID 5 를 연결하는 DWORD 기준이다. (2026-11-13 WB04-E387)
const TOKEN_BASE = 0x16000200 - 5 * 50;
const ENTRY = 16;
const unhex = (text) => (text === '-' ? new Uint8Array(0) : Uint8Array.from(Buffer.from(text, 'hex')));
// 16바이트 항목(토큰·tail)을 만든다. (2026-11-13 WB04-E387)
function entry(token, tail, kind = 4) {
    const bytes = new Uint8Array(ENTRY);
    new DataView(bytes.buffer).setUint32(8, token >>> 0, true);
    bytes[14] = kind;
    bytes[15] = tail;
    return bytes;
}
const throwsOf = (action) => { try { action(); } catch { return true; } return false; };

// 한 관측 행을 재생해 호출 순서와 raw 보존 여부를 돌려준다. (2026-11-13 WB04-E387)
function replay(pool, row) {
    const v = row[2].split(',').map(Number);
    const c = new PreviewCollectionState({ parentGump: v[0], legal: v[1], localPlayer: v[8] });
    const s = new PreviewDrawState({
        selectedType: v[2], destroyType: v[3], noOutlineType: v[4], overlayTrigger: v[5], overlayType: v[6], labelType: v[7], cameraX: v[9] | 0, cameraY: v[10] | 0, gridOption: v[11], showLetters: v[12],
        spacing: v[13] | 0, labelOffsetX: v[14] | 0, labelOffsetY: v[15] | 0, labelWidth: v[16] | 0, labelHeight: v[17] | 0, flags: v[18], messageOverride: v[19], blocked: v[20], powered: v[21], reachable: v[22],
        priceWidth: v[23] | 0, priceIconOffset: v[24] | 0, priceIconX: v[25] | 0, priceIconY: v[26] | 0, priceType: v[27], goodColor: 231, badColor: 189,
    });
    s.ownerMaps = new Array(74).fill(false);
    s.ownerMaps[70] = v[28] !== 0;
    s.grid.set(unhex(row[4]));
    s.labels = unhex(row[3]);
    // 원본 signed char 프레임 표와 측정된 문구 크기 입력이다.
    for (let i = 0; i < 256; i += 1) s.labelFrames[i] = ((i << 24) >> 24) >>> 0;
    for (let i = 0; i < 5; i += 1) { s.messageWidths[i] = 21 + i * 13; s.messageHeights[i] = 9 + i * 7; }
    const types = Array.from({ length: 74 }, () => ({ footX: 0, footY: 0, hotspotX: 0, hotspotY: 0, flags2: 0 }));
    Object.assign(types[70], { footX: v[29] | 0, footY: v[30] | 0, hotspotX: v[31] | 0, hotspotY: v[32] | 0, flags2: v[33] });
    const raw = pool.physical(5);
    raw.fill(0);
    raw[10] = 70;
    put(raw, 22, v[36] & 0xffff, 2);
    put(raw, 24, v[37] & 0xffff, 2);
    put(raw, 14, v[38]);
    put(raw, 18, v[39]);
    put(raw, 36, v[35], 4);
    const before = Uint8Array.from(raw);
    const events = [];
    let current = 0;
    const u = (value) => value >>> 0;
    // 색 문맥 호출까지 포함해 native 출력 순서를 기록한다.
    const hooks = {
        setColors: (request) => { assert.equal(request.type, 70); assert.equal(request.owner, v[8]); current = request.colors; events.push(`M,${u(current)}`); },
        shape: (request) => {
            const mode = request.mode === PreviewShapeMode.plain ? 0 : request.mode === PreviewShapeMode.illegal ? 3 : current;
            events.push(`S,${u(request.type)},${u(request.frame)},${u(request.position.x)},${u(request.position.y)},${u(mode)}`);
        },
        rectangle: (rect, color, checker) => { events.push(`R,${checker ? 1 : 0},${u(rect.left)},${u(rect.top)},${u(rect.right)},${u(rect.bottom)},${u(color)}`); },
        text: (request) => { events.push(`T,${u(request.index)},${u(request.position.x)},${u(request.position.y)}`); },
    };
    new PreviewDraw(pool, types, c, s, hooks, TOKEN_BASE).draw(entry(0x16000200, v[34]));
    return { observed: events.length ? events.join(';') : '-', same: Buffer.compare(before, raw) === 0 };
}

test('커서 미리보기 그리기: 원본 10.78 관측 477행의 분기·현재 색 표·명령 순서가 모두 일치하고 raw 객체는 바뀌지 않는다', { skip: rows === null }, () => {
    const originals = rows.filter((row) => row[0] === 'originals');
    assert.equal(originals.length, 477);
    const pool = new SidPool(24000, { server: false });
    for (const row of originals) {
        const { observed, same } = replay(pool, row);
        assert.equal(observed, row[5], `PreviewDraw ${row[1]}`);
        assert.ok(same, `raw 보존 ${row[1]}`);
    }
});

// 실제 수집과 픽셀/계약 검사를 위한 작은 원본 상태이다(cpppj PreviewDrawScene). (2026-11-13 WB04-E387)
function scene() {
    const pool = new SidPool(24000, { server: false });
    const types = Array.from({ length: 74 }, () => ({ footX: 0, footY: 0, hotspotX: 0, hotspotY: 0, flags2: 0, zOrder: 0 }));
    const c = new PreviewCollectionState({ legal: 1, pieceCount: 1, pieces: [5], type: 70, localPlayer: 2 });
    const s = new PreviewDrawState();
    const raw = pool.physical(5);
    raw.fill(0);
    raw[10] = 70;
    put(raw, 22, 4, 2);
    put(raw, 24, 4, 2);
    Object.assign(types[70], { footX: 1, footY: 1, zOrder: 7 });
    s.ownerMaps = new Array(74).fill(false);
    s.noOutlineType = 70;
    s.flags = 0x20;
    return { pool, types, c, s };
}
// 투명 한 칸/불투명 0·255 를 포함한 실제 VFX 압축 입력이다(3×1 그림). (2026-11-13 WB04-E387)
function pixelShape() {
    const bytes = new Uint8Array(46);
    put(bytes, 0, 0x30312e31);
    put(bytes, 4, 1);
    put(bytes, 8, 16);
    put(bytes, 32, 2);
    bytes.set([1, 1, 5, 0, 255, 0], 40);
    return new ShapeDatabase(bytes);
}

test('커서 미리보기 그리기: 실제 수집→CRT 정렬→kind 4 분배→VFX 픽셀이 한 흐름으로 이어지고 본체/보조 레이어가 차례로 그려진다(PreviewDraw_CommonCollectionDispatch)', () => {
    const w = scene();
    w.c.parentGump = 1;
    const spots = new Uint8Array(65536);
    const pixels = new Uint8Array(64).fill(9);
    const shapes = pixelShape();
    const target = new PreviewPixelTarget(pixels, 8, 8, { left: 0, top: 0, right: 8, bottom: 8 }, () => ({ database: shapes, block: 0 }), null, null);
    const collector = new PreviewCollection(w.pool, w.types, spots, w.c, { gumpDepth: () => 16 }, TOKEN_BASE);
    const draw = new PreviewDraw(w.pool, w.types, w.c, w.s, target.hooks(), TOKEN_BASE);
    const collection = {};
    bindPreviewCollection(collection, collector);
    const dispatch = {};
    bindPreviewDraw(dispatch, draw);
    const work = new Uint8Array(ENTRY * 8);
    const request = { screen: { left: 0, top: 0, right: 8, bottom: 8 }, visible: { left: 0, top: 0, right: 8, bottom: 8 }, unclipped: { left: 0, top: 0, right: 8, bottom: 8 }, dirtyFlags: 0, groupRowLimit: 8 };
    const result = collectDrawEntries(request, work, collection);
    assert.equal(result.count, 2);
    assert.deepEqual([work[15], ENTRY + 15 < work.length ? work[ENTRY + 15] : -1], [0, 2]);
    const entries = work.subarray(0, 2 * ENTRY);
    sortRenderEntries(entries);
    dispatchDrawEntries(entries, {}, dispatch);
    // 투명 칸은 배경 9 그대로, 이어서 색 0 과 255 가 위치(4,4)부터 놓인다.
    assert.deepEqual([pixels[4 * 8 + 4], pixels[4 * 8 + 5], pixels[4 * 8 + 6]], [9, 0, 255]);
});

test('커서 미리보기 그리기: 실제 압축 해제의 투명/원색/현재 색/명시 불법 표와 클립을 지킨다(PreviewDraw_PixelsColorsTransparencyClip)', () => {
    const shapes = pixelShape();
    const pixels = new Uint8Array(32).fill(9);
    let colorCalls = 0;
    // 원색 0/255 를 서로 다른 색으로 바꿔 경계 구분을 확인한다. 치환하지 않는 색은 항등으로 유지한다.
    const color = (request) => {
        colorCalls += 1;
        const map = Uint8Array.from({ length: 256 }, (_, index) => index);
        map[0] = request.colors === PreviewColors.illegal ? 7 : 20;
        map[255] = 42;
        return map;
    };
    const target = new PreviewPixelTarget(pixels, 8, 4, { left: 1, top: 1, right: 5, bottom: 3 }, () => ({ database: shapes, block: 0 }), color, null);
    const hooks = target.hooks();
    hooks.shape({ type: 70, frame: 0, position: { x: 1, y: 1 }, mode: PreviewShapeMode.plain });
    assert.deepEqual([pixels[9], pixels[10], pixels[11]], [9, 0, 255]);
    hooks.setColors({ colors: PreviewColors.owner, type: 70, owner: 2 });
    hooks.shape({ type: 70, frame: 0, position: { x: 1, y: 1 }, mode: PreviewShapeMode.currentColors });
    assert.deepEqual([pixels[10], pixels[11]], [20, 42]);
    hooks.shape({ type: 70, frame: 0, position: { x: 1, y: 2 }, mode: PreviewShapeMode.illegal });
    assert.deepEqual([pixels[18], pixels[19]], [7, 42]);
    hooks.shape({ type: 70, frame: 0, position: { x: 4, y: 2 }, mode: PreviewShapeMode.plain });
    assert.deepEqual([pixels[21], colorCalls], [9, 2]);
    assert.ok(throwsOf(() => hooks.shape({ type: 70, frame: 1, position: { x: 1, y: 1 }, mode: PreviewShapeMode.plain })));
});

test('커서 미리보기 그리기: 체크 무늬는 잘린 위치에서도 절대 좌표 위상을 유지하고 윤곽은 잘린 면에 새 선을 만들지 않는다(PreviewDraw_PixelsCheckerAndOutline)', () => {
    const pixels = new Uint8Array(64).fill(9);
    const target = new PreviewPixelTarget(pixels, 8, 8, { left: 2, top: 2, right: 6, bottom: 6 }, null, null, null);
    const hooks = target.hooks();
    hooks.rectangle({ left: 1, top: 1, right: 7, bottom: 7 }, 189, true);
    // clip 안의 모든 픽셀에서 native (~y^x)&1 의 행별 위상을 확인한다.
    for (let y = 0; y < 8; y += 1) {
        for (let x = 0; x < 8; x += 1) assert.equal(pixels[y * 8 + x], x >= 2 && x < 6 && y >= 2 && y < 6 && ((x ^ y) & 1) ? 189 : 9, `(${x},${y})`);
    }
    const before = Uint8Array.from(pixels);
    hooks.rectangle({ left: 1, top: 1, right: 7, bottom: 7 }, 231, false);
    assert.deepEqual(pixels, before);
    hooks.rectangle({ left: 2, top: 2, right: 5, bottom: 5 }, 231, false);
    assert.deepEqual([pixels[18], pixels[36], pixels[27]], [231, 231, before[27]]);
    hooks.rectangle({ left: 4, top: 4, right: 4, bottom: 4 }, 42, false);
    assert.deepEqual([pixels[27], pixels[28], pixels[35], pixels[36]], [42, 42, 42, 42]);
});

test('커서 미리보기 그리기: raw 토큰/타입 표/격자/표면/그림/필수 경계 부족은 기록된 분기에 도달할 때 거부한다(PreviewDraw_Contracts)', () => {
    const w = scene();
    const draw = new PreviewDraw(w.pool, w.types, w.c, w.s, {}, TOKEN_BASE);
    assert.ok(throwsOf(() => draw.draw(entry(0, 0))));
    assert.ok(throwsOf(() => draw.draw(entry(TOKEN_BASE + 1, 0))));
    assert.ok(throwsOf(() => draw.draw(entry(0x16000200, 0))));
    w.c.parentGump = 1;
    draw.draw(entry(0x16000200, 2));
    w.c.parentGump = 0;
    w.c.legal = 0;
    w.s.noOutlineType = 90;
    w.s.gridOption = 1;
    w.types[70].footX = 11;
    assert.throws(() => draw.draw(entry(0x16000200, 2)), /grid footprint/);
    // 출력 경계가 현재 footprint 를 늘려도 다음 표 조회에서 배열 밖을 거부한다.
    w.types[70].footX = 1;
    w.s.grid.fill(1);
    const growing = { rectangle: () => { w.types[70].footX = 11; } };
    const guarded = new PreviewDraw(w.pool, w.types, w.c, w.s, growing, TOKEN_BASE);
    assert.throws(() => guarded.draw(entry(0x16000200, 2)), /grid index/);
    const pixels = new Uint8Array(16);
    assert.ok(throwsOf(() => new PreviewPixelTarget(pixels, 8, 8, { left: 0, top: 0, right: 8, bottom: 8 }, null, null, null)));
    const target = new PreviewPixelTarget(pixels, 4, 4, { left: 0, top: 0, right: 4, bottom: 4 }, null, null, null);
    const hooks = target.hooks();
    assert.ok(throwsOf(() => hooks.setColors({ colors: PreviewColors.owner, type: 70, owner: 2 })));
    assert.ok(throwsOf(() => hooks.shape({ type: 70, frame: 0, position: { x: 0, y: 0 }, mode: PreviewShapeMode.plain })));
});

test('커서 미리보기 그리기: 원본의 뒤쪽 분기가 출력 도중 바뀐 현재 parent/표시 플래그를 읽는다(PreviewDraw_CurrentStateAfterGrid)', () => {
    const w = scene();
    w.c.legal = 0;
    w.s.noOutlineType = 90;
    w.s.gridOption = 1;
    w.s.grid.fill(1);
    w.s.flags = 0x40;
    let rectangles = 0;
    let texts = 0;
    let shapes = 0;
    const hooks = { rectangle: (rect, color, checker) => { assert.ok(checker); rectangles += 1; w.c.parentGump = 1; }, text: () => { texts += 1; }, shape: () => { shapes += 1; } };
    new PreviewDraw(w.pool, w.types, w.c, w.s, hooks, TOKEN_BASE).draw(entry(0x16000200, 2));
    assert.deepEqual([rectangles, texts, shapes], [9, 0, 0]);
});

test('커서 미리보기 그리기 계약: 자산 바인딩은 로딩 타입의 SHP 블록과 비용 그림 바인딩을 구분하고 생성 검증을 거친다', () => {
    const { table, database } = realCaptureAssets();
    const assets = new GameAssets(table, database);
    const price = { database, block: 5 };
    const binding = makePreviewAssetShapes(assets, 0xfffffffe, price);
    assert.equal(binding(0xfffffffe), price);
    const bridge = assets.find('bridge');
    assert.deepEqual(binding(assets.typeNumber(bridge)), { database: assets.shapes, block: bridge.block });
    for (const bad of [69, 70 + 116, 0]) assert.throws(() => binding(bad), /asset type/, String(bad));
    const w = scene();
    assert.throws(() => new PreviewDraw(w.pool, [], w.c, w.s, {}, TOKEN_BASE), /tables\/token range/);
    assert.throws(() => new PreviewDraw(w.pool, w.types, w.c, w.s, {}, 0xffffffff), /tables\/token range/);
    // 방향 글자: 숨김 글자도 기준은 한 칸씩 내려가고 글자 표시가 켜지면 소문자도 그린다. 프레임 표는 signed char 이다.
    w.c.legal = 1;
    w.s.labelType = 123;
    w.s.spacing = 6;
    w.s.labels = Uint8Array.from([0x41, 0x7a, 0x80, 0x00, 0x42]);
    for (let i = 0; i < 256; i += 1) w.s.labelFrames[i] = i;
    const log = [];
    const hooks = { setColors: () => {}, shape: (r) => { log.push([r.type, r.frame, r.position.x, r.position.y, r.mode]); }, rectangle: () => {}, text: () => {} };
    new PreviewDraw(w.pool, w.types, w.c, w.s, hooks, TOKEN_BASE).draw(entry(0x16000200, 2));
    // 'A'(0x41)는 그림자+본문, 'z'(0x7a)는 숨김, 0x80(signed −128)은 그림자+본문(소문자 아님), 0 에서 멈춤.
    const frames = log.map((item) => item[1]);
    assert.deepEqual(frames, [0x41, 0x41, 0x80, 0x80]);
    assert.deepEqual(log.map((item) => item[3]), [log[0][3], log[0][3] - 1, log[2][3], log[2][3] - 1]);
    assert.equal(log[2][3], log[0][3] - 12);
    w.s.showLetters = 1;
    log.length = 0;
    new PreviewDraw(w.pool, w.types, w.c, w.s, hooks, TOKEN_BASE).draw(entry(0x16000200, 2));
    assert.deepEqual(log.map((item) => item[1]), [0x41, 0x41, 0x7a, 0x7a, 0x80, 0x80]);
});

test('커서 미리보기 그리기 계약: 상태 기본값과 생성 경계(토큰 범위 정확히 2^32·기준 기본 0)', () => {
    const fresh = new PreviewDrawState();
    assert.deepEqual({ ...fresh, grid: undefined, labelFrames: undefined, labels: undefined, messageWidths: undefined, messageHeights: undefined }, {
        selectedType: 0, destroyType: 0, noOutlineType: 0, overlayTrigger: 0, overlayType: 0, labelType: 0, priceType: 0, cameraX: 0, cameraY: 0, spacing: 0, labelOffsetX: 0, labelOffsetY: 0,
        labelWidth: 0, labelHeight: 0, priceWidth: 0, priceIconOffset: 0, priceIconX: 0, priceIconY: 0, gridOption: 0, showLetters: 0, flags: 0, messageOverride: 0, blocked: 0, powered: 0, reachable: 0,
        goodColor: 0, badColor: 0, grid: undefined, labelFrames: undefined, labels: undefined, messageWidths: undefined, messageHeights: undefined, ownerMaps: [], sharedOwnerMaps: [],
    });
    assert.deepEqual([fresh.grid.length, fresh.labelFrames.length, fresh.labels.length, fresh.messageWidths.length, fresh.messageHeights.length], [144, 256, 0, 5, 5]);
    const w = scene();
    const capacity = w.pool.capacity;
    assert.doesNotThrow(() => new PreviewDraw(w.pool, w.types, w.c, w.s, {}, 0x100000000 - capacity * 50));
    assert.throws(() => new PreviewDraw(w.pool, w.types, w.c, w.s, {}, 0x100000000 - capacity * 50 + 1), /tables\/token range/);
    // 토큰 기준을 생략하면 0 이어서 SID 5 의 토큰은 250 이다.
    const drawn = [];
    new PreviewDraw(w.pool, w.types, w.c, w.s, { shape: (r) => { drawn.push(r.type); }, rectangle: () => {} }).draw(entry(250, 0));
    assert.deepEqual(drawn, [70]);
});

test('커서 미리보기 그리기 계약: 토큰·SID·타입 오류의 문구와 경계, 부호 있는 화면 WORD, 윤곽 제외 비트, 프레임 DWORD 폭', () => {
    const w = scene();
    const draw = new PreviewDraw(w.pool, w.types, w.c, w.s, { shape: () => {}, rectangle: () => {} }, TOKEN_BASE);
    assert.throws(() => draw.draw(entry(TOKEN_BASE - 1, 0)), /^RangeError: Preview Draw token$/);
    assert.throws(() => draw.draw(entry(TOKEN_BASE, 0)), /^RangeError: Preview Draw SID$/);
    assert.throws(() => draw.draw(entry(TOKEN_BASE + 51, 0)), /^RangeError: Preview Draw SID$/);
    assert.throws(() => draw.draw(entry(TOKEN_BASE + 24000 * 50, 0)), /^RangeError: Preview Draw SID$/);
    // 마지막 유효 SID(23999)는 타입 표 검사까지 도달한다(타입 0 은 표 안).
    assert.doesNotThrow(() => draw.draw(entry(TOKEN_BASE + 23999 * 50, 0)));
    const edge = w.pool.physical(7);
    edge.fill(0);
    edge[10] = 74;
    assert.throws(() => draw.draw(entry(TOKEN_BASE + 7 * 50, 0)), /^RangeError: Preview Draw type$/);
    edge[10] = 73;
    assert.doesNotThrow(() => draw.draw(entry(TOKEN_BASE + 7 * 50, 0)));
    // 화면 WORD 는 부호 있는 16비트이다: 0xfff0 → −16, 0x8000 → −32768, 0x7fff → 32767. x/y 는 서로 다른 필드다.
    const seen = [];
    const probe = new PreviewDraw(w.pool, w.types, w.c, w.s, { shape: (r) => { seen.push([r.position.x, r.position.y]); }, rectangle: () => {} }, TOKEN_BASE);
    for (const [wordX, wordY, expectedX, expectedY] of [[0xfff0, 0x0010, -16, 16], [0x8000, 0x7fff, -32768, 32767], [0x00ff, 0xff00, 255, -256]]) {
        const raw = w.pool.physical(5);
        put(raw, 22, wordX, 2);
        put(raw, 24, wordY, 2);
        seen.length = 0;
        probe.draw(entry(0x16000200, 0));
        assert.deepEqual(seen, [[expectedX, expectedY]], `WORD ${wordX.toString(16)}/${wordY.toString(16)}`);
    }
    // 윤곽 제외 비트는 flags2 의 4 와 0x100000 뿐이다. 다른 비트만 있으면 윤곽 사각형을 낸다.
    const outlines = (flags2) => {
        w.types[70].flags2 = flags2;
        w.s.noOutlineType = 90;
        let count = 0;
        new PreviewDraw(w.pool, w.types, w.c, w.s, { shape: () => {}, rectangle: () => { count += 1; } }, TOKEN_BASE).draw(entry(0x16000200, 0));
        return count;
    };
    for (const flags2 of [0, 1, 2, 8, 0x200000, 0x800000]) assert.equal(outlines(flags2), 1, `flags2 ${flags2.toString(16)}`);
    for (const flags2 of [4, 0x100000, 0x100004, 5, 0x100001]) assert.equal(outlines(flags2), 0, `flags2 ${flags2.toString(16)}`);
    // 프레임은 raw 의 4바이트 DWORD 이다(상위 바이트까지).
    put(w.pool.physical(5), 36, 0x01020304, 4);
    const frames = [];
    new PreviewDraw(w.pool, w.types, w.c, w.s, { shape: (r) => { frames.push(r.frame); }, rectangle: () => {} }, TOKEN_BASE).draw(entry(0x16000200, 0));
    assert.deepEqual(frames, [0x01020304]);
});

test('커서 미리보기 그리기 계약: 소유자 변환표 길이·공유 표 우선·양수 플레이어 조건, 격자 footprint 경계(10 허용, 11 거부), 호출 플래그 비트 마스크', () => {
    const w = scene();
    const modes = (state, mutate = () => {}) => {
        mutate(w.s);
        const log = [];
        new PreviewDraw(w.pool, w.types, w.c, state, { setColors: (r) => { log.push(`M${r.colors}`); }, shape: (r) => { log.push(`S${r.mode}`); }, rectangle: () => {}, text: () => {} }, TOKEN_BASE).draw(entry(0x16000200, 0));
        return log;
    };
    w.s.noOutlineType = 70;
    w.s.ownerMaps = new Array(71).fill(false);
    w.s.ownerMaps[70] = true;
    // 변환표가 있고 로컬 플레이어가 양수면 소유자 색 문맥 + 현재 색, 0/음수면 원색이다.
    w.c.localPlayer = 2;
    assert.deepEqual(modes(w.s), ['M1', 'ScurrentColors']);
    for (const player of [0, 0xffffffff, 0x80000000]) { w.c.localPlayer = player; assert.deepEqual(modes(w.s), ['Splain'], `player ${player}`); }
    w.c.localPlayer = 1;
    assert.deepEqual(modes(w.s), ['M1', 'ScurrentColors']);
    // 표 길이가 타입 번호와 같으면 거부하고 하나 더 길면 허용한다.
    w.s.ownerMaps = new Array(70).fill(true);
    assert.throws(() => modes(w.s), /owner maps/);
    w.s.ownerMaps = new Array(71).fill(false);
    assert.deepEqual(modes(w.s), ['Splain']);
    // 공유 표(DWORD)가 있으면 그 표가 이기고 비영(0x80000000 포함)만 변환표 존재로 본다.
    for (const [value, expected] of [[0, ['Splain']], [1, ['M1', 'ScurrentColors']], [0x80000000, ['M1', 'ScurrentColors']]]) {
        const shared = new Array(71).fill(0);
        shared[70] = value;
        assert.deepEqual(modes(w.s, (s) => { s.sharedOwnerMaps = shared; s.ownerMaps = new Array(71).fill(true); }), expected, `shared ${value}`);
    }
    assert.throws(() => modes(w.s, (s) => { s.sharedOwnerMaps = new Array(70).fill(1); }), /owner maps/);
    w.s.sharedOwnerMaps = [];
    // 격자 footprint 는 10 까지 허용(최대 칸 (10+1)*12+10+1=143)하고 11 은 거부한다.
    w.c.legal = 0;
    w.s.noOutlineType = 90;
    w.s.gridOption = 1;
    w.s.grid.fill(1);
    const grid = (footX, footY) => {
        w.types[70].footX = footX;
        w.types[70].footY = footY;
        let count = 0;
        new PreviewDraw(w.pool, w.types, w.c, w.s, { rectangle: () => { count += 1; } }, TOKEN_BASE).draw(entry(0x16000200, 2));
        return count;
    };
    assert.equal(grid(10, 10), 12 * 12);
    assert.throws(() => grid(11, 0), /grid footprint/);
    assert.throws(() => grid(0, 11), /grid footprint/);
    // 호출 플래그: 0x20 이 서 있으면 경고를 숨기고(덮어쓰기가 없으면), 0x40 이 서 있어야 비용을 낸다. 다른 낮은 비트(1)만으로는 둘 다 영향이 없다.
    w.types[70].footX = 1;
    w.types[70].footY = 1;
    w.s.gridOption = 0;
    w.s.blocked = 1;
    w.s.powered = 1;
    w.s.reachable = 1;
    const texts = (flags) => {
        w.s.flags = flags;
        const indexes = [];
        new PreviewDraw(w.pool, w.types, w.c, w.s, { setColors: () => {}, shape: () => {}, rectangle: () => {}, text: (r) => { indexes.push(r.index); } }, TOKEN_BASE).draw(entry(0x16000200, 2));
        return indexes;
    };
    assert.deepEqual(texts(0), [2]);
    assert.deepEqual(texts(1), [2]);
    assert.deepEqual(texts(0x20), []);
    assert.deepEqual(texts(0x21), []);
    assert.deepEqual(texts(0x40), [2, 5]);
    assert.deepEqual(texts(0x41), [2, 5]);
    assert.deepEqual(texts(0x60), [5]);
    assert.deepEqual(texts(0x1f), [2]);
});

test('커서 미리보기 그리기 계약: 픽셀 출력 경계 — 표면·clip 검증의 각 조건과 자산 바인딩의 타입 번호 경계', () => {
    const pixels = new Uint8Array(16);
    const make = (pitch, height, clip, buffer = pixels) => () => new PreviewPixelTarget(buffer, pitch, height, clip, null, null, null);
    const full = { left: 0, top: 0, right: 4, bottom: 4 };
    assert.doesNotThrow(make(4, 4, full));
    assert.doesNotThrow(make(4, 4, { left: 2, top: 2, right: 2, bottom: 2 }));
    assert.throws(make(0, 4, full), /surface\/clip/);
    assert.throws(make(4, 0, full), /surface\/clip/);
    assert.throws(make(-4, 4, full), /surface\/clip/);
    assert.throws(make(4, -4, full), /surface\/clip/);
    assert.throws(make(4, 5, full), /surface\/clip/);
    assert.throws(make(5, 4, full), /surface\/clip/);
    assert.throws(make(4, 4, { left: 3, top: 0, right: 2, bottom: 4 }), /surface\/clip/);
    assert.throws(make(4, 4, { left: 0, top: 3, right: 4, bottom: 2 }), /surface\/clip/);
    // 표면 밖으로 넘치는 clip 은 표면 경계로 자른다: 체크 무늬가 표면 밖을 건드리지 않는다.
    const wide = new Uint8Array(16).fill(9);
    new PreviewPixelTarget(wide, 4, 4, { left: -5, top: -5, right: 9, bottom: 9 }, null, null, null).hooks().rectangle({ left: -5, top: -5, right: 9, bottom: 9 }, 1, true);
    assert.equal(wide.filter((value) => value === 1).length, 8);
    // 자산 바인딩: 타입 70(첫 자산)과 185(마지막 자산)는 유효하고 69·186 은 거부한다.
    const { table, database } = realCaptureAssets();
    const assets = new GameAssets(table, database);
    const binding = makePreviewAssetShapes(assets, 0xfffffffe, null);
    assert.equal(binding(70).block, 0);
    assert.equal(binding(185).block, 115);
    for (const bad of [69, 186]) assert.throws(() => binding(bad), /asset type/, String(bad));
});

// 프레임 헤더의 기준 상자(rect)를 바꾼 3×1 그림과 특수 레코드를 포함한 두 프레임 셰이프이다. (2026-11-13 WB04-E387)
function offsetShape(left, top, special = false) {
    const bytes = new Uint8Array(special ? 100 : 46);
    put(bytes, 0, 0x30312e31);
    put(bytes, 4, special ? 2 : 1);
    put(bytes, 8, special ? 24 : 16);
    if (special) put(bytes, 16, 70);
    const first = special ? 24 : 16;
    put(bytes, first + 8, left | 0);
    put(bytes, first + 12, top | 0);
    put(bytes, first + 16, (left + 2) | 0);
    put(bytes, first + 20, top | 0);
    bytes.set([1, 1, 5, 0, 255, 0], first + 24);
    if (special) {
        // 두 번째 프레임은 좌표 대신 메타데이터가 들어 있는 특수 레코드(왼쪽이 0x7fffffff 이상)이다.
        put(bytes, 70 + 8, 0x7fffffff);
        put(bytes, 70 + 16, 0x7fffffff);
        put(bytes, 70 + 20, 0x7fffffff);
    }
    return new ShapeDatabase(bytes);
}

test('커서 미리보기 그리기 계약: 색 문맥 번호, 격자 좌표 비유한 거부, 공유 표 길이 1, 자산 프레임 범위 문구, 불법 표 소유자, 그림 위치에 프레임 기준 상자 더하기, 특수 레코드·현재 색 누락', () => {
    assert.deepEqual({ ...PreviewColors }, { owner: 1, destroy: 2, illegal: 3, labelShadow: 4 });
    // 격자 앵커 좌표가 NaN/무한대이면 좌표 오류로 거부한다.
    for (const bad of [Number.NaN, Infinity, -Infinity]) {
        const w = scene();
        w.c.legal = 0;
        w.s.noOutlineType = 90;
        w.s.gridOption = 1;
        put(w.pool.physical(5), 14, floatToBits(bad));
        assert.throws(() => new PreviewDraw(w.pool, w.types, w.c, w.s, { rectangle: () => {} }, TOKEN_BASE).draw(entry(0x16000200, 2)), /Preview Draw coordinate/, String(bad));
    }
    // 공유 표는 길이 1 이어도 공유 표로 취급해 타입 번호가 길이 이상이면 거부한다(변환표가 충분히 길어도).
    const w = scene();
    w.s.ownerMaps = new Array(80).fill(true);
    w.s.sharedOwnerMaps = [1];
    assert.throws(() => new PreviewDraw(w.pool, w.types, w.c, w.s, { setColors: () => {}, shape: () => {}, rectangle: () => {} }, TOKEN_BASE).draw(entry(0x16000200, 0)), /owner maps/);
    // 1×1 표면도 유효하다.
    assert.doesNotThrow(() => new PreviewPixelTarget(new Uint8Array(1), 1, 1, { left: 0, top: 0, right: 1, bottom: 1 }, null, null, null));
    // 자산 프레임: 블록 번호/프레임 번호가 길이와 같으면 전용 오류 문구이다.
    const shapes = pixelShape();
    const pixels = new Uint8Array(64).fill(9);
    const request = (block, frame, mode = PreviewShapeMode.plain) => ({ type: 70, frame, position: { x: 0, y: 0 }, mode, block });
    let owner = -1;
    const color = (r) => { owner = r.owner; return Uint8Array.from({ length: 256 }, (_, i) => i); };
    const target = new PreviewPixelTarget(pixels, 8, 8, { left: 0, top: 0, right: 8, bottom: 8 }, (type) => ({ database: shapes, block: type === 70 ? 0 : 1 }), color, null);
    const hooks = target.hooks();
    assert.throws(() => hooks.shape({ type: 71, frame: 0, position: { x: 0, y: 0 }, mode: PreviewShapeMode.plain }), /^RangeError: Preview Draw frame$/);
    assert.throws(() => hooks.shape(request(0, 1)), /^RangeError: Preview Draw frame$/);
    // 불법 표는 소유자 0 으로 요청하고 현재 색 모드는 색 문맥을 고르기 전에는 거부한다.
    hooks.shape({ type: 70, frame: 0, position: { x: 0, y: 0 }, mode: PreviewShapeMode.illegal });
    assert.equal(owner, 0);
    const fresh = new PreviewPixelTarget(new Uint8Array(64), 8, 8, { left: 0, top: 0, right: 8, bottom: 8 }, () => ({ database: shapes, block: 0 }), color, null).hooks();
    assert.throws(() => fresh.shape({ type: 70, frame: 0, position: { x: 0, y: 0 }, mode: PreviewShapeMode.currentColors }), /current colors missing/);
    // 그림 위치는 화면 기준점에 프레임의 기준 상자 왼쪽/위를 더한다: rect.left 1, top 2 이면 (3,4)에 놓은 그림은 (4,6)부터 그려진다.
    const offset = offsetShape(1, 2);
    const surface = new Uint8Array(64).fill(9);
    new PreviewPixelTarget(surface, 8, 8, { left: 0, top: 0, right: 8, bottom: 8 }, () => ({ database: offset, block: 0 }), null, null).hooks().shape({ type: 70, frame: 0, position: { x: 3, y: 4 }, mode: PreviewShapeMode.plain });
    // 첫 칸은 투명(건너뜀)이라 (4,6)은 그대로, (5,6)=0, (6,6)=255.
    assert.deepEqual([surface[6 * 8 + 4], surface[6 * 8 + 5], surface[6 * 8 + 6], surface.filter((value) => value !== 9).length], [9, 0, 255, 2]);
    // 특수 레코드는 그림이 없으므로 아무것도 그리지 않는다.
    const special = offsetShape(0, 0, true);
    const blank = new Uint8Array(64).fill(9);
    new PreviewPixelTarget(blank, 8, 8, { left: 0, top: 0, right: 8, bottom: 8 }, () => ({ database: special, block: 0 }), null, null).hooks().shape({ type: 70, frame: 1, position: { x: 0, y: 0 }, mode: PreviewShapeMode.plain });
    assert.ok(blank.every((value) => value === 9));
});

test('커서 미리보기 그리기 계약: 윤곽 사각형은 clip 안의 테두리 픽셀만 정확히 칠하고 체크 무늬는 절대 위상이다', () => {
    const clip = { left: 1, top: 1, right: 7, bottom: 7 };
    const outline = (rect) => {
        const pixels = new Uint8Array(64).fill(9);
        new PreviewPixelTarget(pixels, 8, 8, clip, null, null, null).hooks().rectangle(rect, 77, false);
        return pixels;
    };
    // 독립 정의: 사각형이 비어 있지 않을 때 테두리는 [left..right-1]×[top..bottom-1] 의 가장자리 칸이며 clip 밖은 칠하지 않는다.
    const expected = (rect) => {
        const out = new Uint8Array(64).fill(9);
        for (let y = clip.top; y < clip.bottom; y += 1) {
            for (let x = clip.left; x < clip.right; x += 1) {
                const inside = x >= rect.left && x <= rect.right - 1 && y >= rect.top && y <= rect.bottom - 1;
                if (inside && (x === rect.left || x === rect.right - 1 || y === rect.top || y === rect.bottom - 1)) out[y * 8 + x] = 77;
            }
        }
        return out;
    };
    for (const rect of [
        { left: 2, top: 2, right: 6, bottom: 5 }, { left: 1, top: 1, right: 7, bottom: 7 }, { left: 0, top: 0, right: 8, bottom: 8 }, { left: 4, top: 1, right: 12, bottom: 4 },
        { left: 1, top: 5, right: 4, bottom: 10 }, { left: -3, top: 2, right: 3, bottom: 5 }, { left: 2, top: -4, right: 5, bottom: 3 }, { left: 3, top: 3, right: 4, bottom: 4 },
        { left: 2, top: 2, right: 6, bottom: 3 }, { left: 2, top: 2, right: 3, bottom: 6 }, { left: 6, top: 2, right: 8, bottom: 5 }, { left: 2, top: 6, right: 5, bottom: 8 },
    ]) assert.deepEqual(outline(rect), expected(rect), JSON.stringify(rect));
    // 체크 무늬: clip 가장자리를 포함해 빈 교차는 아무것도 칠하지 않고, 한 칸 폭도 위상을 따른다.
    const checker = (rect) => {
        const pixels = new Uint8Array(64).fill(9);
        new PreviewPixelTarget(pixels, 8, 8, clip, null, null, null).hooks().rectangle(rect, 55, true);
        return pixels.filter((value) => value === 55).length;
    };
    assert.equal(checker({ left: 7, top: 1, right: 9, bottom: 7 }), 0);
    assert.equal(checker({ left: 1, top: 7, right: 7, bottom: 9 }), 0);
    assert.equal(checker({ left: 0, top: 0, right: 1, bottom: 8 }), 0);
    assert.equal(checker({ left: 1, top: 1, right: 2, bottom: 2 }), 0);
    assert.equal(checker({ left: 2, top: 1, right: 3, bottom: 2 }), 1);
    assert.equal(checker({ left: 3, top: 3, right: 3, bottom: 6 }), 0);
});

test('커서 미리보기 그리기 계약: 방향 글자 경계 — 0x60 은 소문자가 아니라 그리고 0x61 부터는 숨기며 글자 표시가 켜지면 모두 그린다', () => {
    const w = scene();
    w.c.legal = 1;
    w.s.labelType = 123;
    w.s.spacing = 6;
    w.s.labels = Uint8Array.from([0x60, 0x61, 0x7f]);
    for (let i = 0; i < 256; i += 1) w.s.labelFrames[i] = i;
    const frames = [];
    new PreviewDraw(w.pool, w.types, w.c, w.s, { setColors: () => {}, shape: (r) => { frames.push(r.frame); }, rectangle: () => {}, text: () => {} }, TOKEN_BASE).draw(entry(0x16000200, 2));
    assert.deepEqual(frames, [0x60, 0x60]);
});
