// 일반 객체 Draw render/sprite-draw.js 를 원본 10.78 관측(spritedraw-x86.tsv originals 463행)과 cpppj `SpriteDrawTests.cpp` 의 공통 분배·픽셀 모드/클립·선/막대·계약·현재 raw/필터/공유 전역 시험으로 검사한다.
//  - 관측 한 행 = 완성 입력 한 건(tail·extra·프레임·월드 좌표·카메라·타입 기준점/발자국/플래그·HP·타이머/자원/건설/상태 프로세스 입력)에서 원본이 낸 호출열이다:
//    M(소유자 색 문맥)·S(그림: 타입·프레임·위치·모드 0~5)·L(선택 선)·R(막대 fill)·F(가상 프레임 조회)·V(가상 Draw). 비교 항목은 호출열, 선택된 SID(쓰기), 그림자 alias 의 현재 그림 타입(쓰기), raw 객체 보존이다.
//    VFX/가상 처리기/게임 조회만 경계이다. original1037·originalCD 행은 웹 범위 밖(10.78 전용)이라 읽지 않는다.
//  - 이식한 cpppj 시험: PatchNative·CommonGroupDispatch·PixelModesTransparencyClip·PixelLinesAndBars·ContractsAndEarlyReturns·CurrentRawAfterVirtualDraw·SurfaceLookupBoundaries·ShadowAliasCurrentShape·CurrentFilterAfterVirtualFrame·
//    SharedDiagnosticControlsCurrentShadowAlias·SharedOwnerTablesAndBannerAfterVirtualFrame. `SpriteDraw_ObjectCollectionPixels` 는 ObjectCollection(실제 raw 해시 수집기, todo)과 함께 그 단위에서 건다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E388)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { dispatchDrawEntries } from '../www/render/draw-submission.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { GameAssets } from '../www/assets/game-assets.js';
import { SpriteDraw, SpriteDrawState, SpritePixelTarget, SpriteShapeMode, bindSpriteDraw, makeSpriteAssetShapes, makeSpriteAssetViews, makeSpriteSurfaceLookup } from '../www/render/sprite-draw.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E388)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'spritedraw-x86.tsv'));
// 실제 fixture 의 raw 주소와 호스트 SID 5 를 연결하는 기준이다. (2026-11-13 WB04-E388)
const SPRITE_RAW = 0x16001000;
const TOKEN_BASE = SPRITE_RAW - 250;
const ENTRY = 16;
const u = (value) => value >>> 0;
const throwsOf = (action) => { try { action(); } catch { return true; } return false; };

// 16바이트 항목(x·y·토큰·kind·tail)을 만든다. (2026-11-13 WB04-E388)
function entry(x, y, token, tail, kind = 0) {
    const bytes = new Uint8Array(ENTRY);
    const view = new DataView(bytes.buffer);
    view.setFloat32(0, x, true);
    view.setFloat32(4, y, true);
    view.setUint32(8, token >>> 0, true);
    bytes[14] = kind;
    bytes[15] = tail;
    return bytes;
}
// 물리 프레임 한 장의 크기/기준점이다(width·height·hotspotX·hotspotY). (2026-11-13 WB04-E388)
const frameOf = (width, height, hotspotX, hotspotY) => ({ width, height, hotspotX, hotspotY });
const blankProcess = Object.freeze({ type: 0, flags: 0, members: [], endTime: 0 });

// 한 관측 행을 재생해 불일치 설명 목록을 돌려준다. (2026-11-13 WB04-E388)
function replay(pool, row) {
    const v = row[2].split(',').map(Number);
    const integer = (index) => v[index] | 0;
    const s = new SpriteDrawState({
        cameraX: integer(5), cameraY: integer(6), localPlayer: v[15], specialMode: v[17], filteredFlags: v[18], wideOwnerType: v[14] ? 70 : 90, unfilteredType: v[21] ? 70 : 90,
        shadowAliasType: v[36], shadowAliasPinned: v[37] !== 0, quarterHitPoints: v[35] !== 0, selectionDisabled: v[28] !== 0, markerEnabled: v[24] !== 0, markerVisible: v[25] !== 0,
        markerType: 71, markerExcludeType: 73, statusType: 75, abstractStatusType: 76, costType: v[46] ? 70 : 90, timerProcessType: 40, statusProcessType: 41, now: 100.25,
        selectionColors: [180, 181, 182, 183], hitPointColors: [100, 101, 102], barBackground: 9,
    });
    s.timerTypes = [0, 1, 2].map((index) => (v[44] === index + 1 || v[44] === 4 ? 70 : 90));
    s.ownerMaps = new Array(91).fill(0);
    s.ownerMaps[70] = v[16];
    s.ownerPalettes = Array.from({ length: 301 }, (_, index) => (index % 3) + 1);
    const types = Array.from({ length: 91 }, () => ({ hotspotX: 0, hotspotY: 0, footX: 0, footY: 0, flags1: 0, flags2: 0, maxHitPoints: 0, cost: 0 }));
    // native SHP 의 모든 물리 프레임은 같은 측정 헤더를 갖는 입력이다.
    const views = Array.from({ length: 91 }, () => ({
        shape: { loaded: true, frameCount: integer(12), frames: Array.from({ length: 64 }, () => frameOf((integer(29) << 16) >> 16, (integer(30) << 16) >> 16, (integer(31) << 16) >> 16, (integer(32) << 16) >> 16)) },
        gumpFrame: v[23], capacity: 0, constructionTime: 0, duration: 0,
    }));
    views[70].shape.loaded = v[11] !== 0;
    views[74].capacity = integer(42);
    views[74].constructionTime = integer(39);
    views[75].duration = 30;
    views[76].duration = 20;
    Object.assign(types[70], { hotspotX: integer(7), hotspotY: integer(8), footX: integer(9), footY: integer(10), flags1: v[19], flags2: v[20], maxHitPoints: integer(34), cost: bitsToFloat(v[47]) });
    const raw = pool.physical(5);
    raw.fill(0);
    raw[10] = 70;
    put(raw, 14, v[3]);
    put(raw, 18, v[4]);
    put(raw, 22, Math.trunc(bitsToFloat(v[3]) * 16 + 0.5) & 0xffff, 2);
    put(raw, 24, Math.trunc(bitsToFloat(v[4]) * 11 + 0.5) & 0xffff, 2);
    put(raw, 36, v[2], 4);
    raw[40] = v[1] & 0xff;
    raw[34] = v[13] & 0xff;
    put(raw, 12, v[14] ? v[13] : v[48], 2);
    put(raw, 26, v[33], 4);
    put(raw, 42, 74);
    put(raw, 46, v[40]);
    pool.physical(8)[10] = v[27] ? 73 : 72;
    const before = Uint8Array.from(raw);
    const processes = new Array(40000).fill(blankProcess);
    if (v[49]) {
        processes[0] = { type: 41, flags: 1, members: [5], endTime: 999 };
        processes[1] = { type: 41, flags: 3, members: [6, 5, 5], endTime: 111.9 };
        processes[2] = { type: 41, flags: 2, members: [5], endTime: 122.8 };
        processes[3] = { type: 41, flags: 1, members: [5], endTime: 99 };
        processes[4] = { type: 42, flags: 1, members: [5], endTime: 900 };
        processes[5] = { type: 41, flags: 2, members: [6], endTime: 900 };
        processes[39999] = { type: 41, flags: 1, members: [5], endTime: v[49] === 2 ? 120.6 : 104 };
    }
    s.processes = processes;
    const events = [];
    const hooks = {
        // 필수 출력과 가상 호출을 같은 순서의 native DWORD 명령으로 기록한다.
        setOwnerColors: (type, owner) => { events.push(`M,${u(type)},${u(owner)}`); },
        shape: (request) => { events.push(`S,${u(request.type)},${u(request.frame)},${u(request.position.x)},${u(request.position.y)},${request.mode}`); },
        line: (first, second, color) => { events.push(`L,${u(first.x)},${u(first.y)},${u(second.x)},${u(second.y)},${u(color)}`); },
        fill: (rect, color) => { events.push(`R,${u(rect.left)},${u(rect.top)},${u(rect.right)},${u(rect.bottom)},${u(color)}`); },
        virtualFrame: (sid) => { events.push(`F,${sid}`); return integer(22); },
        virtualDraw: (sid, point) => { events.push(`V,${sid},${u(point.x)},${u(point.y)}`); },
        constructionActive: (sid) => { assert.equal(sid, 5); return v[38] !== 0; },
        resource: (sid) => { assert.equal(sid, 5); return v[41] ? { capacityType: 74, remaining: integer(43) } : null; },
        timers: (sid, type) => { assert.equal(sid, 5); assert.equal(type, 40); return v[45] ? [119.99, 160.99, 151.01] : null; },
    };
    const surfaceHash = new SquidHash();
    const ix = Math.trunc(bitsToFloat(v[3]) + Math.fround(0.9999));
    const iy = Math.trunc(bitsToFloat(v[4]) + Math.fround(0.9999));
    if (ix >= 0 && ix < 256 && iy >= 0 && iy < 256) surfaceHash.setCell(0, ix, iy, v[26] ? 8 : 0);
    hooks.surface = makeSpriteSurfaceLookup(surfaceHash);
    new SpriteDraw(pool, types, views, s, hooks, TOKEN_BASE).draw(entry(bitsToFloat(v[3]), bitsToFloat(v[4]), SPRITE_RAW, v[0]));
    const issues = [];
    const check = (name, actual, expected) => { if (actual !== expected) issues.push(`${name}: ${actual} != ${expected}`); };
    check('events', events.length ? events.join(';') : '-', row[3]);
    check('selected', s.selectedSid, Number(row[4]));
    check('shadow', 0x17000000 + (s.shadowShapeType ?? s.shadowAliasType) * 0x1000, Number(row[5]));
    check('raw', Buffer.compare(before, raw), 0);
    return issues;
}

test('일반 객체 Draw: 원본 10.78 관측 463행의 본체/그림자/표식/선택/상태 막대 호출열과 선택 SID·그림자 alias 쓰기가 모두 일치한다', { skip: rows === null }, () => {
    const originals = rows.filter((row) => row[0] === 'originals');
    assert.equal(originals.length, 463);
    const pool = new SidPool(24000, { server: false });
    for (const row of originals) assert.deepEqual(replay(pool, row), [], `SpriteDraw ${row[1]}`);
});

// 출력과 계약을 검사하는 최소 실제 raw/SHP 상태이다(cpppj SpriteScene). raw 좌표는 x*16/y*11+0.5 를 절삭한 화면 WORD 이다. (2026-11-13 WB04-E388)
function scene() {
    const pool = new SidPool(24000, { server: false });
    const types = Array.from({ length: 71 }, () => ({ hotspotX: 0, hotspotY: 0, footX: 0, footY: 0, flags1: 0, flags2: 0, maxHitPoints: 0, cost: 0 }));
    const views = Array.from({ length: 71 }, () => ({ shape: { loaded: false, frameCount: 0, frames: [] }, gumpFrame: 0, capacity: 0, constructionTime: 0, duration: 0 }));
    const s = new SpriteDrawState({ shadowAliasType: 69, costType: 69, timerTypes: [69, 69, 69], ownerPalettes: [0], selectionColors: [7] });
    const raw = pool.physical(5);
    raw.fill(0);
    raw[10] = 70;
    put(raw, 14, floatToBits(0.25));
    put(raw, 18, floatToBits(0.25));
    put(raw, 22, 4, 2);
    put(raw, 24, 3, 2);
    views[70].shape = { frameCount: 1, loaded: true, frames: [frameOf(4, 4, 1, 1)] };
    s.ownerMaps = new Array(71).fill(0);
    return { pool, types, views, s, raw };
}
// 투명 한 칸/불투명 0·255 를 포함한 실제 VFX RLE 3×1 그림이다. (2026-11-13 WB04-E388)
function pixelShape() {
    const bytes = new Uint8Array(46);
    put(bytes, 0, 0x30312e31);
    put(bytes, 4, 1);
    put(bytes, 8, 16);
    put(bytes, 32, 2);
    bytes.set([1, 1, 5, 0, 255, 0], 40);
    return new ShapeDatabase(bytes);
}

test('일반 객체 Draw: 실제 kind 0 과 kind 1 묶음 원소 모두 같은 SHP 픽셀 출력에 연결된다(SpriteDraw_CommonGroupDispatch)', () => {
    const w = scene();
    const shapes = pixelShape();
    const pixels = new Uint8Array(64).fill(9);
    const target = new SpritePixelTarget(pixels, 8, 8, { left: 0, top: 0, right: 8, bottom: 8 }, (type) => { assert.equal(type, 70); return { database: shapes, block: 0 }; }, null);
    const draw = new SpriteDraw(w.pool, w.types, w.views, w.s, target.hooks(), TOKEN_BASE);
    const hooks = {};
    bindSpriteDraw(hooks, draw);
    const single = entry(0.25, 0.25, SPRITE_RAW, 0);
    dispatchDrawEntries(single, {}, hooks);
    assert.deepEqual([pixels[28], pixels[29], pixels[30]], [9, 0, 255]);
    pixels.fill(9);
    const count = { value: 1 };
    hooks.group = () => ({ token: 1, entries: single, count });
    dispatchDrawEntries(entry(0, 0, 1, 0, 1), {}, hooks);
    assert.deepEqual([pixels[29], pixels[30]], [0, 255]);
});

test('일반 객체 Draw: 소유자는 소스 치환이고 고정 표는 별도 translated 커널이며 투명·클립·모드별 색 표를 지킨다(SpriteDraw_PixelModesTransparencyClip)', () => {
    const shapes = pixelShape();
    const pixels = new Uint8Array(32).fill(9);
    const target = new SpritePixelTarget(pixels, 8, 4, { left: 1, top: 1, right: 5, bottom: 3 }, () => ({ database: shapes, block: 0 }), (mode) => Uint8Array.from({ length: 256 }, (_, index) => (index + mode * 10) & 0xff));
    const hooks = target.hooks();
    hooks.shape({ type: 70, frame: 0, position: { x: 1, y: 1 }, mode: SpriteShapeMode.plain });
    assert.deepEqual([pixels[9], pixels[10], pixels[11]], [9, 0, 255]);
    assert.ok(throwsOf(() => hooks.shape({ type: 70, frame: 0, position: { x: 1, y: 1 }, mode: SpriteShapeMode.owner })));
    hooks.setOwnerColors(70, 2);
    hooks.shape({ type: 70, frame: 0, position: { x: 1, y: 1 }, mode: SpriteShapeMode.owner });
    assert.deepEqual([pixels[10], pixels[11]], [10, 9]);
    // 0040351d 의 같은 y 꼭짓점은 한 행도 그리지 않는다(전체 픽셀 근거는 E378 translated-pixels 관측).
    hooks.shape({ type: 70, frame: 0, position: { x: 1, y: 1 }, mode: SpriteShapeMode.shadow });
    assert.deepEqual([pixels[10], pixels[11], pixels[9]], [10, 9, 9]);
    hooks.shape({ type: 70, frame: 0, position: { x: 4, y: 2 }, mode: SpriteShapeMode.dead });
    assert.equal(pixels[21], 9);
    assert.ok(throwsOf(() => hooks.shape({ type: 70, frame: 1, position: { x: 1, y: 1 }, mode: SpriteShapeMode.plain })));
});

test('일반 객체 Draw: 선택선의 끝점/역순/클립과 half-open fill 의 빈/역전 범위를 지킨다(SpriteDraw_PixelLinesAndBars)', () => {
    const pixels = new Uint8Array(64).fill(9);
    const hooks = new SpritePixelTarget(pixels, 8, 8, { left: 2, top: 2, right: 6, bottom: 6 }, null, null).hooks();
    hooks.line({ x: 7, y: 3 }, { x: 1, y: 3 }, 17);
    assert.deepEqual([pixels[26], pixels[29], pixels[25], pixels[30]], [17, 17, 9, 9]);
    hooks.line({ x: 4, y: 7 }, { x: 4, y: 1 }, 18);
    assert.deepEqual([pixels[20], pixels[44], pixels[52]], [18, 18, 9]);
    hooks.fill({ left: 3, top: 4, right: 5, bottom: 6 }, 19);
    assert.deepEqual([pixels[35], pixels[44], pixels[45]], [19, 19, 9]);
    const before = Uint8Array.from(pixels);
    hooks.fill({ left: 5, top: 4, right: 3, bottom: 6 }, 99);
    hooks.fill({ left: 3, top: 5, right: 5, bottom: 5 }, 99);
    assert.deepEqual(pixels, before);
    assert.ok(throwsOf(() => hooks.line({ x: 2, y: 2 }, { x: 3, y: 3 }, 4)));
});

test('일반 객체 Draw: 로드되지 않은 그림은 건너뛰고 토큰/좌표 불일치는 거부하며 그림자 alias 기록은 진단 조건을 따른다(SpriteDraw_ContractsAndEarlyReturns)', () => {
    const w = scene();
    const draw = new SpriteDraw(w.pool, w.types, w.views, w.s, {}, TOKEN_BASE);
    const item = entry(0, 0, SPRITE_RAW, 0);
    w.views[70].shape.loaded = false;
    draw.draw(item);
    w.views[70].shape.loaded = true;
    assert.ok(throwsOf(() => draw.draw(item)));
    assert.ok(throwsOf(() => draw.draw(entry(0, 0, SPRITE_RAW + 1, 0))));
    put(w.raw, 22, 5, 2);
    assert.ok(throwsOf(() => draw.draw(item)));
    put(w.raw, 22, 4, 2);
    const shadow = entry(0, 0, SPRITE_RAW, 4);
    w.s.specialMode = 1;
    draw.draw(shadow);
    assert.equal(w.s.shadowShapeType, 70);
    w.s.shadowShapeType = null;
    w.s.shadowAliasPinned = true;
    draw.draw(shadow);
    assert.equal(w.s.shadowShapeType, null);
    w.s.specialMode = 0;
    w.raw[40] = 0x10;
    draw.draw(shadow);
    assert.equal(w.s.shadowShapeType, null);
});

test('일반 객체 Draw: 가상 Draw 가 선택/상태 비트를 바꾸면 후속 표시가 현재 슬롯을 사용한다(SpriteDraw_CurrentRawAfterVirtualDraw)', () => {
    const w = scene();
    w.raw[40] = 2;
    let lines = 0;
    const hooks = { virtualFrame: () => -1, virtualDraw: () => { w.raw[40] = 4; }, line: () => { lines += 1; }, resource: () => null };
    new SpriteDraw(w.pool, w.types, w.views, w.s, hooks, TOKEN_BASE).draw(entry(0, 0, SPRITE_RAW, 0));
    assert.deepEqual([lines, w.s.selectedSid], [8, 5]);
});

test('일반 객체 Draw: 표면 조회의 +0.9999/음수 절삭/경계와 비유한 거부(SpriteDraw_SurfaceLookupBoundaries)', () => {
    const hash = new SquidHash();
    hash.setCell(0, 0, 0, 5);
    hash.setCell(0, 1, 1, 6);
    const lookup = makeSpriteSurfaceLookup(hash);
    assert.deepEqual([lookup(0, 0), lookup(0.25, 0.25), lookup(-0.25, -0.25), lookup(-2, -2), lookup(256, 256)], [5, 6, 5, 0, 0]);
    assert.throws(() => lookup(Infinity, 0), /finite coordinate/);
});

test('일반 객체 Draw: 그림자 alias 는 이후 loaded/선택 헤더/색 출력에 반영되며 alias 타입의 논리 count 는 보존한다(SpriteDraw_ShadowAliasCurrentShape)', () => {
    const w = scene();
    w.s.timerTypes = [68, 68, 68];
    w.s.costType = 68;
    w.views[69].shape.frameCount = 1;
    const lines = [];
    const drawn = [];
    const hooks = { shape: (r) => { drawn.push(r); }, line: (first) => { lines.push(first); }, constructionActive: () => false, resource: () => null };
    const draw = new SpriteDraw(w.pool, w.types, w.views, w.s, hooks, TOKEN_BASE);
    draw.draw(entry(0, 0, SPRITE_RAW, 4));
    assert.equal(w.s.shadowShapeType, 70);
    const alias = w.pool.physical(6);
    alias.set(w.raw);
    alias[10] = 69;
    alias[40] = 4;
    put(alias, 42, 70);
    draw.draw(entry(0, 0, SPRITE_RAW + 50, 0));
    assert.equal(drawn.length, 2);
    assert.equal(drawn[1].type, 69);
    assert.deepEqual([lines.length, lines[0].x, w.s.selectedSid], [8, 3, 6]);
    w.views[69].shape.frameCount = 0;
    assert.ok(throwsOf(() => draw.draw(entry(0, 0, SPRITE_RAW + 50, 0))));
});

test('일반 객체 Draw: 가상 frame 조회에서 바뀐 특수 표시 전역은 조회 직후의 필터 분기에 반영한다(SpriteDraw_CurrentFilterAfterVirtualFrame)', () => {
    const w = scene();
    w.raw[40] = 2;
    w.types[70].flags2 = 8;
    w.views[70].gumpFrame = 7;
    const drawn = [];
    const hooks = { virtualFrame: () => { w.s.specialMode = 1; w.s.filteredFlags = 8; return -1; }, shape: (r) => { drawn.push(r); } };
    new SpriteDraw(w.pool, w.types, w.views, w.s, hooks, TOKEN_BASE).draw(entry(0, 0, SPRITE_RAW, 0));
    assert.deepEqual(drawn.map((r) => [r.frame, r.mode]), [[7, SpriteShapeMode.abstract]]);
});

test('일반 객체 Draw: 그림자 0 조건은 생성 때 복사한 bool 대신 같은 Draw 호출마다 현재 Client DWORD 를 읽는다(SpriteDraw_SharedDiagnosticControlsCurrentShadowAlias)', () => {
    const w = scene();
    const diagnostic = { value: 6 };
    w.s.sharedPatchDiagnostic = diagnostic;
    w.s.shadowAliasPinned = true;
    let shapes = 0;
    const draw = new SpriteDraw(w.pool, w.types, w.views, w.s, { shape: () => { shapes += 1; } }, TOKEN_BASE);
    for (const value of [6, 0, 1, 0xffffffff, 0]) {
        diagnostic.value = value;
        w.s.shadowShapeType = null;
        draw.draw(entry(0, 0, SPRITE_RAW, 4));
        assert.equal(w.s.shadowShapeType !== null, value === 0, `진단 ${value}`);
        if (value === 0) assert.equal(w.s.shadowShapeType, 70);
    }
    assert.deepEqual([shapes, w.s.shadowAliasPinned], [5, true]);
});

test('일반 객체 Draw: 가상 프레임 효과 뒤의 실제 557940/5319c0 읽기는 지역 초기값과 별도로 현재 값을 사용한다(SpriteDraw_SharedOwnerTablesAndBannerAfterVirtualFrame)', () => {
    const w = scene();
    w.raw[34] = 1;
    w.raw[40] = 2;
    w.types[70].flags1 = 0x400;
    w.pool.physical(6)[10] = 69;
    Object.assign(w.s, { markerType: 96, markerExcludeType: 157, markerVisible: true, markerEnabled: true, ownerPalettes: [0, 1] });
    w.s.ownerMaps[70] = 1;
    const tables = new Array(71).fill(0);
    const banners = { value: 0 };
    w.s.sharedOwnerMaps = tables;
    w.s.sharedUseBanners = banners;
    let requested = 0;
    const shapes = [];
    let ownerCalls = 0;
    const hooks = {
        virtualFrame: () => { tables[70] = requested; banners.value = requested; return 0; }, virtualDraw: () => {},
        setOwnerColors: (type, owner) => { assert.deepEqual([type, owner], [70, 1]); ownerCalls += 1; }, surface: () => 6, shape: (r) => { shapes.push(r); },
    };
    const draw = new SpriteDraw(w.pool, w.types, w.views, w.s, hooks, TOKEN_BASE);
    // 같은 실행기의 생존 중 옵션 전환과 가상 효과 뒤 재조회를 확인한다.
    for (const value of [0, 1, 0, 1]) {
        requested = value;
        tables[70] = banners.value = 1 - value;
        shapes.length = 0;
        ownerCalls = 0;
        draw.draw(entry(0, 0, SPRITE_RAW, 0));
        assert.deepEqual([shapes.length, ownerCalls], [1 + value, value]);
        assert.equal(shapes[0].mode, value ? SpriteShapeMode.owner : SpriteShapeMode.plain);
        if (value) assert.deepEqual([shapes[1].type, shapes[1].frame], [96, 0]);
    }
});

test('일반 객체 Draw 계약: 실제 자산 view 는 전체 타입 번호(188)를 유지하고 .type 의 기본 프레임·casttime·praytime·effecttime 을 로딩 순서대로 싣는다', () => {
    const { table, database } = realCaptureAssets();
    const assets = new GameAssets(table, database);
    const views = makeSpriteAssetViews(assets);
    assert.equal(views.length, 188);
    // 자산 없는 내장 타입(0~69)과 여분(186·187)은 로드되지 않은 그림이다.
    for (const number of [0, 1, 69, 186, 187]) assert.equal(views[number].shape.loaded, false, `타입 ${number}`);
    const bridge = assets.find('bridge');
    const bridgeView = views[assets.typeNumber(bridge)];
    assert.deepEqual([bridgeView.shape.loaded, bridgeView.shape.frameCount, bridgeView.gumpFrame], [true, bridge.definition.clusters.length, bridge.definition.defaultFrame]);
    // 시간 속성이 있는 타입을 실제 정의에서 찾아 0 방향 절삭으로 싣는지 본다(정의 속성이 없으면 0).
    let withTime = 0;
    assets.types().forEach((type, index) => {
        const view = views[70 + index];
        const number = (name) => type.definition.properties.filter((property) => property.name.toLowerCase() === name).map((property) => property.value).filter((value) => typeof value === 'number').at(-1) ?? 0;
        assert.equal(view.capacity, Math.trunc(number('casttime')), `${type.assetName} casttime`);
        assert.equal(view.constructionTime, Math.trunc(number('praytime')), `${type.assetName} praytime`);
        assert.equal(view.duration, Math.trunc(number('effecttime')), `${type.assetName} effecttime`);
        assert.equal(view.gumpFrame, type.definition.defaultFrame >>> 0);
        if (view.capacity || view.constructionTime || view.duration) withTime += 1;
    });
    assert.ok(withTime > 0);
    // 그림자 alias 바인딩: alias 타입은 현재 그림 타입의 블록을 쓰고 범위 밖 타입은 거부한다.
    const state = new SpriteDrawState({ shadowAliasType: 100 });
    const binding = makeSpriteAssetShapes(assets, state);
    const alias = assets.find('dude');
    assert.deepEqual(binding(100), { database: database, block: 30 });
    state.shadowShapeType = assets.typeNumber(alias);
    assert.deepEqual(binding(100), { database: database, block: alias.block });
    assert.deepEqual(binding(assets.typeNumber(bridge)), { database: database, block: bridge.block });
    for (const bad of [69, 186]) assert.throws(() => binding(bad), /asset type/, String(bad));
});

// 그림자 레이어·선택 막대 등 분기를 한 장면에서 반복 검사하기 위한 도우미이다. 호출열은 `종류,인자…` 문자열 목록이다. (2026-11-13 WB04-E388)
function run(w, tail = 0, extra = {}) {
    const events = [];
    const hooks = {
        setOwnerColors: (type, owner) => { events.push(`M,${type},${owner}`); },
        shape: (r) => { events.push(`S,${r.type},${r.frame},${r.position.x},${r.position.y},${r.mode}`); },
        line: (a, b) => { events.push(`L,${a.x},${a.y},${b.x},${b.y}`); },
        fill: (rect, color) => { events.push(`R,${rect.left},${rect.top},${rect.right},${rect.bottom},${color}`); },
        virtualFrame: () => -1, virtualDraw: () => { events.push('V'); },
        constructionActive: () => false, resource: () => null, timers: () => null, surface: () => 0,
        ...extra,
    };
    new SpriteDraw(w.pool, w.types, w.views, w.s, hooks, TOKEN_BASE).draw(entry(0.25, 0.25, SPRITE_RAW, tail));
    return events;
}

test('일반 객체 Draw 계약: 상태 기본값·모드 번호·생성 경계·오류 문구', () => {
    assert.deepEqual({ ...SpriteShapeMode }, { plain: 0, owner: 1, abstract: 2, dead: 3, shadow: 4, bright160: 5 });
    const fresh = new SpriteDrawState();
    assert.deepEqual({ ...fresh }, {
        cameraX: 0, cameraY: 0, localPlayer: 0, specialMode: 0, filteredFlags: 0, wideOwnerType: 0, unfilteredType: 0, shadowAliasType: 0, shadowAliasPinned: false, quarterHitPoints: false,
        selectionDisabled: false, markerEnabled: false, markerVisible: false, markerExcludeType: 0, markerType: 0, statusType: 0, abstractStatusType: 0, timerProcessType: 0, statusProcessType: 0,
        costType: 0, timerTypes: [0, 0, 0], ownerMaps: [], sharedOwnerMaps: [], sharedUseBanners: null, ownerPalettes: [], selectionColors: [], hitPointColors: [0, 0, 0], barBackground: 0,
        selectedSid: 0, shadowShapeType: null, now: 0, processes: [], sharedPatchDiagnostic: null,
    });
    const w = scene();
    const capacity = w.pool.capacity;
    assert.doesNotThrow(() => new SpriteDraw(w.pool, w.types, w.views, w.s, {}, 0x100000000 - capacity * 50));
    assert.throws(() => new SpriteDraw(w.pool, w.types, w.views, w.s, {}, 0x100000000 - capacity * 50 + 1), /tables\/token range/);
    assert.throws(() => new SpriteDraw(w.pool, [], [], w.s, {}, 0), /tables\/token range/);
    assert.throws(() => new SpriteDraw(w.pool, w.types, w.views.slice(1), w.s, {}, 0), /tables\/token range/);
    // 토큰 기준을 생략하면 0 이다(SID 5 의 토큰 250).
    const events = [];
    new SpriteDraw(w.pool, w.types, w.views, w.s, { shape: (r) => { events.push(r.type); } }).draw(entry(0.25, 0.25, 250, 0));
    assert.deepEqual(events, [70]);
    const draw = new SpriteDraw(w.pool, w.types, w.views, w.s, { shape: () => {} }, TOKEN_BASE);
    assert.throws(() => draw.draw(entry(0, 0, TOKEN_BASE - 1, 0)), /^RangeError: Sprite Draw token$/);
    assert.throws(() => draw.draw(entry(0, 0, TOKEN_BASE, 0)), /^RangeError: Sprite Draw SID$/);
    assert.throws(() => draw.draw(entry(0, 0, TOKEN_BASE + 51, 0)), /^RangeError: Sprite Draw SID$/);
    assert.throws(() => draw.draw(entry(0, 0, TOKEN_BASE + capacity * 50, 0)), /^RangeError: Sprite Draw SID$/);
    const edge = w.pool.physical(7);
    edge.fill(0);
    edge[10] = 71;
    assert.throws(() => draw.draw(entry(0, 0, TOKEN_BASE + 7 * 50, 0)), /^RangeError: Sprite Draw type$/);
    // 그림자 alias 로 바뀐 타입 번호가 view 표 길이와 같으면 거부한다.
    w.s.shadowAliasType = 70;
    w.s.shadowShapeType = 71;
    assert.throws(() => draw.draw(entry(0.25, 0.25, SPRITE_RAW, 0)), /^RangeError: Sprite Draw shape alias$/);
    // 화면 WORD 와 월드 투영이 x 만, y 만 다를 때도 각각 거부한다.
    const x = scene();
    const same = new SpriteDraw(x.pool, x.types, x.views, x.s, { shape: () => {} }, TOKEN_BASE);
    put(x.raw, 22, 5, 2);
    assert.throws(() => same.draw(entry(0.25, 0.25, SPRITE_RAW, 0)), /world\/screen mismatch/);
    put(x.raw, 22, 4, 2);
    put(x.raw, 24, 4, 2);
    assert.throws(() => same.draw(entry(0.25, 0.25, SPRITE_RAW, 0)), /world\/screen mismatch/);
    // 카메라는 화면 WORD 에서 빼고 월드 투영과 같은 값을 뺀다.
    put(x.raw, 24, 3, 2);
    x.s.cameraX = 2;
    x.s.cameraY = 1;
    assert.doesNotThrow(() => same.draw(entry(0.25, 0.25, SPRITE_RAW, 0)));
});

test('일반 객체 Draw 계약: 그림자 레이어의 프레임 보정과 extra 비트 마스크, 소유자 필드 폭, 소유자 표 경계', () => {
    // 그림자 레이어: 프레임이 논리 프레임 수 이상이면 한 번 빼고 `프레임 + 논리 프레임 수` 로 그린다.
    const shadowFrames = (frame, frameCount, extra = 0) => {
        const w = scene();
        w.views[70].shape.frameCount = frameCount;
        put(w.raw, 36, frame, 4);
        w.raw[40] = extra;
        return run(w, 4).filter((event) => event.startsWith('S')).map((event) => Number(event.split(',')[2]));
    };
    assert.deepEqual([shadowFrames(0, 2), shadowFrames(1, 2), shadowFrames(2, 2), shadowFrames(3, 2), shadowFrames(4, 2), shadowFrames(5, 3)], [[2], [3], [2], [3], [4], [5]]);
    assert.deepEqual([shadowFrames(2, 2, 0x20), shadowFrames(2, 2, 0x08), shadowFrames(2, 2, 0x02), shadowFrames(2, 2, 0x10), shadowFrames(2, 2, 0x01), shadowFrames(2, 2, 0x81)], [[2], [2], [2], [], [], []]);
    // 소유자: 넓은 소유자 타입이면 +12 의 WORD 하나(2바이트)이고 아니면 +34 의 BYTE 이다. 소유자 변환표가 있고 0 이 아니어야 소유자 색 문맥을 쓴다.
    const ownerRun = (wide, ownerWord, ownerByte, mapped) => {
        const w = scene();
        w.s.wideOwnerType = wide ? 70 : 90;
        put(w.raw, 14, floatToBits(0.25 + 2 ** -22)); // x 의 낮은 바이트를 0 이 아니게 해 WORD 폭 이상을 읽는 변형을 드러낸다.
        put(w.raw, 12, ownerWord, 2);
        w.raw[34] = ownerByte;
        w.s.ownerMaps[70] = mapped;
        return run(w, 0).filter((event) => event.startsWith('M') || event.startsWith('S')).map((event) => event.split(',').slice(0, 3).join(','));
    };
    assert.deepEqual(ownerRun(true, 0x0203, 7, 1), ['M,70,515', 'S,70,0']);
    assert.deepEqual(ownerRun(false, 0x0203, 7, 1), ['M,70,7', 'S,70,0']);
    assert.deepEqual(ownerRun(false, 0x0203, 0, 1), ['S,70,0']);
    assert.deepEqual(ownerRun(true, 0, 7, 1), ['S,70,0']);
    assert.deepEqual(ownerRun(false, 0x0203, 7, 0), ['S,70,0']);
    // 소유자 변환표 길이가 타입 번호와 같으면 거부하고, 공유 표는 길이 1 이어도 공유 표로 취급한다.
    const w = scene();
    w.s.ownerMaps = new Array(70).fill(1);
    assert.throws(() => run(w), /owner maps/);
    w.s.ownerMaps = new Array(80).fill(1);
    w.s.sharedOwnerMaps = [1];
    assert.throws(() => run(w), /owner maps/);
    w.s.sharedOwnerMaps = new Array(71).fill(0);
    w.s.sharedOwnerMaps[70] = 0x80000000;
    w.raw[34] = 3;
    assert.deepEqual(run(w).map((event) => event.split(',')[0]), ['M', 'S']);
});

test('일반 객체 Draw 계약: 표식과 상태 표시 — 배너 옵션·flags1 비트·소유자 팔레트 경계·제외 타입·상태 타입 경계', () => {
    const marker = (mutate) => {
        const w = scene();
        Object.assign(w.s, { markerType: 96, markerExcludeType: 157, markerVisible: true, markerEnabled: true, ownerPalettes: [0, 5], selectionDisabled: true });
        w.types[70].flags1 = 0x400;
        w.raw[34] = 1;
        w.pool.physical(6)[10] = 69;
        mutate(w);
        return run(w, 0, { surface: () => 6 }).filter((event) => event.startsWith('S,96'));
    };
    assert.deepEqual(marker(() => {}), ['S,96,4,4,3,0']);
    assert.deepEqual(marker((w) => { w.types[70].flags1 = 0x001; }), []);
    assert.deepEqual(marker((w) => { w.types[70].flags1 = 0x401; }).length, 1);
    assert.deepEqual(marker((w) => { w.s.markerEnabled = false; }), []);
    assert.deepEqual(marker((w) => { w.s.markerVisible = false; }), []);
    assert.deepEqual(marker((w) => { w.raw[34] = 0; }), []);
    assert.deepEqual(marker((w) => { w.pool.physical(6)[10] = 157; }), []);
    assert.throws(() => marker((w) => { w.s.ownerPalettes = [0]; }), /owner palette/);
});

test('일반 객체 Draw 계약: 선택 프레임 검사의 각 조건과 소유자 팔레트/색 경계, HP 막대 갈래(flags2 0x400000 은 extra 비트 0, 아니면 flags1 0x10), 1/4 HP, 형식 오류 문구', () => {
    const select = (mutate = () => {}) => {
        const w = scene();
        w.raw[40] = 4;
        w.s.ownerPalettes = [0, 1];
        w.s.selectionColors = [7, 8];
        mutate(w);
        return () => run(w, 0, { resource: () => null });
    };
    // 정상: 두 겹 선 8개(소유자 0 → 색 7).
    assert.equal(select()().filter((event) => event.startsWith('L')).length, 8);
    // 프레임이 물리 프레임 수 이상, 음수 DWORD, 논리 프레임 수×(1 또는 2) 이상, 논리 프레임 수 0 이하, 쓰레기 HP 표식은 거부한다.
    assert.throws(select((w) => { put(w.raw, 36, 1, 4); }), /selection frame/);
    assert.throws(select((w) => { put(w.raw, 36, 0xffffffff, 4); }), /selection frame/);
    assert.throws(select((w) => { w.views[70].shape.frameCount = 0; w.views[70].shape.frames.push(frameOf(4, 4, 1, 1)); }), /selection frame/);
    assert.throws(select((w) => { w.types[70].maxHitPoints = 0xdddddddd | 0; }), /selection frame/);
    assert.throws(select((w) => { w.types[70].maxHitPoints = 0xcdcdcdcd | 0; }), /selection frame/);
    // 프레임 수 1 에서 프레임 1 은 물리 프레임이 둘이어도 flags1 0x440000 비트가 없으면 거부하고 있으면 허용한다(두 비트 각각).
    for (const [flags1, ok] of [[0, false], [0x040000, true], [0x400000, true], [0x440000, true], [0x001000, false]]) {
        const call = select((w) => { w.views[70].shape.frames.push(frameOf(4, 4, 1, 1)); put(w.raw, 36, 1, 4); w.types[70].flags1 = flags1; });
        if (ok) assert.doesNotThrow(call, `flags1 ${flags1.toString(16)}`); else assert.throws(call, /selection frame/, `flags1 ${flags1.toString(16)}`);
    }
    // 소유자 팔레트: 길이 밖·음수·선택 색 길이 밖은 거부한다.
    assert.throws(select((w) => { w.raw[34] = 2; }), /selection colors/);
    assert.throws(select((w) => { w.s.ownerPalettes = [-1, 1]; }), /selection colors/);
    assert.throws(select((w) => { w.s.ownerPalettes = [2, 1]; }), /selection colors/);
    assert.doesNotThrow(select((w) => { w.s.ownerPalettes = [1, 1]; }));
    // HP 막대: flags1 0x10 이면 낸다. flags2 0x400000 이 서 있으면 extra 비트 0(죽음/상태)이 있을 때만 낸다.
    const bars = (mutate) => { const w = scene(); w.raw[40] = 4; w.types[70].maxHitPoints = 40; put(w.raw, 26, 20, 4); mutate(w); return run(w, 0).filter((event) => event.startsWith('R')).length; };
    assert.equal(bars((w) => { w.types[70].flags1 = 0x10; }), 2);
    assert.equal(bars((w) => { w.types[70].flags1 = 0x20; }), 0);
    assert.equal(bars((w) => { w.types[70].flags1 = 0x10; w.types[70].flags2 = 0x400000; }), 0);
    assert.equal(bars((w) => { w.types[70].flags1 = 0; w.types[70].flags2 = 0x400000; w.raw[40] = 5; }), 2);
    assert.equal(bars((w) => { w.types[70].flags1 = 0; w.types[70].flags2 = 0x400001; w.raw[40] = 4; }), 0);
    // HP: 현재 HP 는 raw +26 의 DWORD 전체(상위 바이트 포함), 최대는 TYPE 값이며 1/4 HP 옵션이 그림자 alias 타입에서만 4 로 나눈다.
    const barWidth = (mutate) => {
        const w = scene();
        w.raw[40] = 4;
        w.views[70].shape.frames[0] = frameOf(40, 4, 1, 1);
        w.types[70].flags1 = 0x10;
        mutate(w);
        const fills = run(w, 0).filter((event) => event.startsWith('R'));
        return Number(fills[1].split(',')[3]) - Number(fills[1].split(',')[1]);
    };
    assert.equal(barWidth((w) => { w.types[70].maxHitPoints = 40; put(w.raw, 26, 20, 4); }), 20);
    assert.equal(barWidth((w) => { w.types[70].maxHitPoints = 0x1000100; put(w.raw, 26, 0x1000100, 4); }), 40);
    assert.equal(barWidth((w) => { w.types[70].maxHitPoints = 40; put(w.raw, 26, 10, 4); w.s.quarterHitPoints = true; w.s.shadowAliasType = 70; }), 40);
    assert.equal(barWidth((w) => { w.types[70].maxHitPoints = 40; put(w.raw, 26, 10, 4); w.s.quarterHitPoints = true; w.s.shadowAliasType = 69; }), 10);
    assert.equal(barWidth((w) => { w.types[70].maxHitPoints = 40; put(w.raw, 26, 10, 4); w.s.quarterHitPoints = true; w.s.shadowAliasType = 70; w.s.sharedPatchDiagnostic = null; }), 40);
});

test('일반 객체 Draw 계약: 건설·자원·타이머·지속·비용 막대의 경계와 형식 오류, 프로세스 목록 규칙', () => {
    const barsOf = (mutate, hooks = {}) => {
        const w = scene();
        w.raw[40] = 4;
        w.views[75] = { ...w.views[70], shape: { ...w.views[70].shape }, duration: 30 };
        w.views.length = 71;
        mutate(w);
        return run(w, 0, hooks).filter((event) => event.startsWith('R'));
    };
    // 건설 막대: 그림자 alias 타입 객체이고 건설 중이면 목표 타입의 건설 시간에서 raw +46 을 뺀 값을 0x7b 색으로 낸다.
    const construction = (mutate) => barsOf((w) => { w.s.shadowAliasType = 70; put(w.raw, 42, 70); w.views[70].constructionTime = 20; put(w.raw, 46, 5); mutate(w); }, { constructionActive: () => true });
    assert.equal(construction(() => {}).length, 2);
    assert.throws(() => construction((w) => { put(w.raw, 42, 71); }), /construction type/);
    assert.equal(barsOf((w) => { w.s.shadowAliasType = 70; put(w.raw, 42, 70); }, { constructionActive: () => false }).length, 0);
    // 자원 막대: 용량 타입이 view 표 길이 이상이면 거부한다.
    assert.equal(barsOf(() => {}, { resource: () => ({ capacityType: 70, remaining: 3 }) }).length, 2);
    assert.throws(() => barsOf(() => {}, { resource: () => ({ capacityType: 71, remaining: 3 }) }), /resource type/);
    // 타이머: 세 타이머 타입 각각이 독립 조회/막대이며 타입 일치가 없으면 조회하지 않는다.
    let timerCalls = 0;
    const timers = (types) => barsOf((w) => { w.s.timerTypes = types; w.s.now = 100; }, { timers: () => { timerCalls += 1; return [110, 150, 190]; } });
    assert.equal(timers([70, 69, 69]).length, 2);
    assert.equal(timers([69, 70, 69]).length, 2);
    assert.equal(timers([69, 69, 70]).length, 2);
    assert.equal(timers([70, 70, 70]).length, 6);
    assert.equal(timers([69, 69, 69]).length, 0);
    assert.equal(timerCalls, 6);
    // 지속 막대: raw +40 의 0x10/0x20 비트와 프로세스 규칙. 첫 SID 일치에서 멈추고 flag 1 이 2 보다 우선하며 슬롯 0 은 건너뛴다.
    const duration = (extra, processes, mutate = () => {}) => barsOf((w) => {
        w.raw[40] = 4 | extra;
        Object.assign(w.s, { statusProcessType: 41, statusType: 70, abstractStatusType: 70, now: 100 });
        w.views[70].duration = 50;
        w.s.processes = processes;
        mutate(w);
    });
    const proc = (flags, members, endTime) => ({ type: 41, flags, members, endTime });
    const none = { type: 0, flags: 0, members: [], endTime: 0 };
    assert.equal(duration(0x10, [none, proc(1, [5], 130)]).length, 2);
    assert.equal(duration(0x30, [none, proc(1, [5], 130), proc(2, [5], 140)]).length, 4);
    assert.equal(duration(0x10, [proc(1, [5], 130)]).length, 2);
    assert.throws(() => duration(0x10, [none], (w) => { w.s.abstractStatusType = 71; }), /abstract status/);
    assert.throws(() => duration(0x20, [none], (w) => { w.s.statusType = 71; }), /status type|status duration/);
    assert.equal(duration(0x01, [none, proc(1, [5], 130)]).length, 0);
    // 비용 막대: 비용 타입 객체는 raw +12 WORD 를 현재, TYPE 비용(float 절삭)을 최대로 0xf0 색을 낸다.
    const cost = barsOf((w) => { w.s.costType = 70; w.types[70].cost = 12.9; put(w.raw, 12, 6, 2); });
    assert.equal(cost.length, 2);
    assert.match(cost[1], /,240$/);
});

test('일반 객체 Draw 계약: 표면 조회의 각 경계와 픽셀 출력의 배율·모드·오류 문구', () => {
    const hash = new SquidHash();
    hash.setCell(0, 255, 0, 3);
    hash.setCell(0, 0, 255, 4);
    hash.setCell(0, 255, 255, 5);
    const lookup = makeSpriteSurfaceLookup(hash);
    assert.deepEqual([lookup(255, 0), lookup(0, 255), lookup(255, 255), lookup(256, 0), lookup(0, 256), lookup(-1.5, 0), lookup(0, -1.5)], [3, 4, 5, 0, 0, 0, 0]);
    // 픽셀 출력: 배율(scaled) 요청은 VFX 배율 경로이고 모드마다 색 표 요청(모드·타입·소유자 0)이 달라진다.
    const shapes = pixelShape();
    const requests = [];
    const surface = () => new Uint8Array(64).fill(9);
    const map = (mode) => Uint8Array.from({ length: 256 }, (_, index) => (index + mode * 3) & 0xff);
    const target = (pixels) => new SpritePixelTarget(pixels, 8, 8, { left: 0, top: 0, right: 8, bottom: 8 }, () => ({ database: shapes, block: 0 }), (mode, type, owner) => { requests.push([mode, type, owner]); return map(mode); }).hooks();
    const unit = surface();
    target(unit).shape({ type: 70, frame: 0, position: { x: 1, y: 1 }, mode: SpriteShapeMode.plain, scaled: true, scaleX: 65536, scaleY: 65536 });
    assert.deepEqual([unit[10], unit[11]], [0, 255]);
    const half = surface();
    target(half).shape({ type: 70, frame: 0, position: { x: 1, y: 1 }, mode: SpriteShapeMode.plain, scaled: true, scaleX: 0x8000, scaleY: 0x8000 });
    assert.notDeepEqual(Array.from(half), Array.from(unit));
    for (const mode of [SpriteShapeMode.abstract, SpriteShapeMode.dead, SpriteShapeMode.bright160]) {
        requests.length = 0;
        const pixels = surface();
        target(pixels).shape({ type: 70, frame: 0, position: { x: 1, y: 1 }, mode });
        assert.deepEqual(requests, [[mode, 70, 0]], `mode ${mode}`);
    }
    // 오류 문구: 소유자 색 누락·바인딩 누락·프레임 범위·표면/clip 각 조건.
    assert.throws(() => target(surface()).shape({ type: 70, frame: 0, position: { x: 0, y: 0 }, mode: SpriteShapeMode.owner }), /owner colors missing/);
    assert.throws(() => new SpritePixelTarget(surface(), 8, 8, { left: 0, top: 0, right: 8, bottom: 8 }, () => ({ database: null, block: 0 }), null).hooks().shape({ type: 70, frame: 0, position: { x: 0, y: 0 }, mode: 0 }), /shape binding/);
    assert.throws(() => target(surface()).shape({ type: 70, frame: 1, position: { x: 0, y: 0 }, mode: 0 }), /^RangeError: Sprite Draw frame$/);
    const make = (pitch, height, clip, buffer = new Uint8Array(16)) => () => new SpritePixelTarget(buffer, pitch, height, clip, null, null);
    const full = { left: 0, top: 0, right: 4, bottom: 4 };
    assert.doesNotThrow(make(4, 4, full));
    assert.doesNotThrow(make(1, 1, { left: 0, top: 0, right: 1, bottom: 1 }));
    assert.doesNotThrow(make(4, 4, { left: 2, top: 2, right: 2, bottom: 2 }));
    for (const [pitch, height, clip] of [[0, 4, full], [4, 0, full], [-4, 4, full], [4, -4, full], [4, 5, full], [5, 4, full], [4, 4, { left: 3, top: 0, right: 2, bottom: 4 }], [4, 4, { left: 0, top: 3, right: 4, bottom: 2 }]]) assert.throws(make(pitch, height, clip), /surface\/clip/);
    // 선분과 fill 의 clip 경계: 가장자리 칸은 칠하고 바로 바깥은 칠하지 않는다.
    const edges = surface();
    const edgeHooks = new SpritePixelTarget(edges, 8, 8, { left: 2, top: 2, right: 6, bottom: 6 }, null, null).hooks();
    edgeHooks.line({ x: 2, y: 2 }, { x: 5, y: 2 }, 1);
    edgeHooks.line({ x: 2, y: 5 }, { x: 5, y: 5 }, 1);
    edgeHooks.line({ x: 2, y: 1 }, { x: 5, y: 1 }, 2);
    edgeHooks.line({ x: 2, y: 6 }, { x: 5, y: 6 }, 2);
    edgeHooks.line({ x: 2, y: 2 }, { x: 2, y: 5 }, 3);
    edgeHooks.line({ x: 5, y: 2 }, { x: 5, y: 5 }, 3);
    edgeHooks.line({ x: 1, y: 2 }, { x: 1, y: 5 }, 4);
    edgeHooks.line({ x: 6, y: 2 }, { x: 6, y: 5 }, 4);
    assert.equal(Array.from(edges).filter((value) => value === 2 || value === 4).length, 0);
    assert.equal(Array.from(edges).filter((value) => value !== 9).length, 12);
    edgeHooks.fill({ left: 0, top: 0, right: 8, bottom: 8 }, 6);
    assert.equal(Array.from(edges).filter((value) => value === 6).length, 16);
});

test('일반 객체 Draw 계약: 요청의 기본 배율, 죽은 객체의 표식, 그림자 프레임 경계, 선택 프레임 배수·flags1 비트, HP 막대 갈래와 분모, 건설·지속·비용 막대 입력 폭', () => {
    // 일반 요청은 scaled=false, 배율 65536 이다.
    const w = scene();
    const requests = [];
    new SpriteDraw(w.pool, w.types, w.views, w.s, { shape: (r) => { requests.push(r); } }, TOKEN_BASE).draw(entry(0.25, 0.25, SPRITE_RAW, 0));
    assert.deepEqual([requests[0].scaled, requests[0].scaleX, requests[0].scaleY], [false, 65536, 65536]);
    // 죽은 객체(extra 1)는 abstract 가 아니므로 표식을 낸다. abstract(0x10)는 표식을 내지 않는다.
    const marker = (extra) => {
        const m = scene();
        Object.assign(m.s, { markerType: 96, markerExcludeType: 157, markerVisible: true, markerEnabled: true, ownerPalettes: [0, 5] });
        m.types[70].flags1 = 0x400;
        m.raw[34] = 1;
        m.raw[40] = extra;
        m.pool.physical(6)[10] = 69;
        return run(m, 0, { surface: () => 6 }).filter((event) => event.startsWith('S,96')).length;
    };
    assert.deepEqual([marker(0), marker(1), marker(0x11), marker(0x10)], [1, 1, 0, 0]);
    // 그림자 레이어: 프레임 2·논리 프레임 3 이면 보정 없이 5 이다(프레임 홀수 비트 변형을 드러냄).
    const shadow = (frame, count) => { const s = scene(); s.views[70].shape.frameCount = count; put(s.raw, 36, frame, 4); return run(s, 4).map((event) => event.split(',')[2]).filter(Boolean).map(Number); };
    assert.deepEqual([shadow(2, 3), shadow(4, 5), shadow(0, 1), shadow(1, 1)], [[5], [9], [1], [1]]);
    // 선택 프레임: 논리 프레임 1 이면 프레임 1 은 flags1 0x440000 비트가 있을 때만 허용(물리 3장일 때 프레임 2 는 배수 2 를 넘어 거부), flags1 낮은 비트만은 거부한다.
    const frameCheck = (flags1, frame) => {
        const c = scene();
        c.raw[40] = 4;
        c.views[70].shape.frames.push(frameOf(4, 4, 1, 1), frameOf(4, 4, 1, 1));
        c.types[70].flags1 = flags1;
        put(c.raw, 36, frame, 4);
        return () => run(c);
    };
    for (const flags1 of [0x040000, 0x400000, 0x440000]) {
        assert.doesNotThrow(frameCheck(flags1, 1), `flags1 ${flags1.toString(16)} 프레임 1`);
        assert.throws(frameCheck(flags1, 2), /selection frame/, `flags1 ${flags1.toString(16)} 프레임 2`);
    }
    assert.throws(frameCheck(0x000001, 1), /selection frame/);
    assert.throws(frameCheck(0x000100, 1), /selection frame/);
    // HP 막대 갈래: flags2 에 0x400000 이 없으면 flags1 의 0x10 만 보고(낮은 비트 1 은 아님), 있으면 extra 비트 0 만 본다.
    const hp = (flags1, flags2, extra) => { const h = scene(); h.raw[40] = 4 | extra; h.types[70].maxHitPoints = 40; put(h.raw, 26, 20, 4); h.types[70].flags1 = flags1; h.types[70].flags2 = flags2; return run(h).filter((event) => event.startsWith('R')).length; };
    assert.deepEqual([hp(0x10, 0, 0), hp(0x01, 0, 0), hp(0x11, 0, 0), hp(0x10, 0x01, 0), hp(0x10, 0x400000, 0), hp(0, 0x400000, 1)], [2, 0, 2, 2, 0, 2]);
    // 막대 폭: 최대 HP 가 1 이면 분모 1(2 가 아님), 1/4 HP 는 40 → 10 으로 나눈 비율이다(현재 5 → 폭 절반).
    const widthOf = (max, current, mutate = () => {}) => {
        const h = scene();
        h.raw[40] = 4;
        h.views[70].shape.frames[0] = frameOf(40, 4, 1, 1);
        h.types[70].flags1 = 0x10;
        h.types[70].maxHitPoints = max;
        put(h.raw, 26, current, 4);
        mutate(h);
        const fill = run(h).filter((event) => event.startsWith('R'))[1].split(',');
        return Number(fill[3]) - Number(fill[1]);
    };
    assert.equal(widthOf(1, 1), 40);
    assert.equal(widthOf(40, 5, (h) => { h.s.quarterHitPoints = true; h.s.shadowAliasType = 70; }), 20);
    assert.equal(widthOf(40, 5, (h) => { h.s.quarterHitPoints = true; h.s.shadowAliasType = 69; }), 5);
    // 건설 막대는 raw +46 의 DWORD 전체를 빼고(짝수도 그대로), 비용 막대는 raw +12 의 WORD 하나(상위 바이트는 x 좌표의 일부)를 읽는다.
    const construct = (value) => {
        const c = scene();
        c.raw[40] = 4;
        c.s.shadowAliasType = 70;
        put(c.raw, 42, 70);
        c.views[70].constructionTime = 20;
        put(c.raw, 46, value);
        const fill = run(c, 0, { constructionActive: () => true }).filter((event) => event.startsWith('R'))[1].split(',');
        return Number(fill[3]) - Number(fill[1]);
    };
    assert.deepEqual([construct(4), construct(5), construct(0x100)], [Math.trunc(((20 - 4) * 4) / 20), Math.trunc(((20 - 5) * 4) / 20), 0]);
    const cost = (word) => {
        const c = scene();
        c.raw[40] = 4;
        c.s.costType = 70;
        c.types[70].cost = 100;
        put(c.raw, 14, floatToBits(0.25 + 2 ** -22));
        put(c.raw, 12, word, 2);
        const fill = run(c).filter((event) => event.startsWith('R'))[1].split(',');
        return Number(fill[3]) - Number(fill[1]);
    };
    assert.deepEqual([cost(50), cost(100), cost(200)], [2, 4, 4]);
});

test('일반 객체 Draw 계약: 지속 막대의 비트 조합·시간 초기값, 상태 프로세스 호출 조건, 자산 view 기본값, 표면 조회 별칭, 픽셀 clip 원점과 바인딩 블록 경계', () => {
    const w0 = scene();
    // raw +40 의 0x30 비트가 없으면 상태 프로세스 목록을 조회하지 않는다(죽음 비트 1 만으로는 조회하지 않음).
    let calls = 0;
    const barCount = (extra, processes, mutate = () => {}) => {
        const w = scene();
        w.raw[40] = extra;
        Object.assign(w.s, { statusProcessType: 41, statusType: 70, abstractStatusType: 70, now: 100 });
        w.views[70].duration = 50;
        w.s.processes = processes;
        mutate(w);
        return run(w, 0, { statusProcesses: () => { calls += 1; return processes; } }).filter((event) => event.startsWith('R'));
    };
    const none = { type: 0, flags: 0, members: [], endTime: 0 };
    const match = (flags, endTime) => ({ type: 41, flags, members: [5], endTime });
    calls = 0;
    barCount(4 | 1, [none, match(1, 130)]);
    barCount(4, [none, match(1, 130)]);
    barCount(4 | 2, [none, match(1, 130)]);
    assert.equal(calls, 0);
    // 비트 하나씩: 0x10 은 abstract 막대만, 0x20 은 상태 막대만(죽음 비트 1 이 함께 있어도 다른 막대는 없다).
    assert.equal(barCount(0x14, [none, match(1, 130)]).length, 2);
    assert.equal(barCount(0x24, [none, match(2, 130)]).length, 2);
    assert.equal(barCount(0x25, [none, match(2, 130)]).length, 2);
    assert.equal(barCount(0x15, [none, match(1, 130)]).length, 2);
    // 일치하는 프로세스가 없으면 남은 시간 0 이라 본체 fill 의 폭이 0 이다.
    const noMatch = barCount(0x24, [none]);
    assert.equal(Number(noMatch[1].split(',')[3]) - Number(noMatch[1].split(',')[1]), 0);
    // abstract 객체는 본체 상태 표시를 건너뛰고 선택에서 지속 막대의 상태 타입 경계(표 길이와 같으면 거부)를 만난다.
    assert.throws(() => barCount(0x34, [none], (w) => { w.s.statusType = 71; }), /^RangeError: Sprite Draw status duration$/);
    // 자산 view: 자산 없는 내장 타입은 기본값 그대로이다.
    const { table, database } = realCaptureAssets();
    const views = makeSpriteAssetViews(new GameAssets(table, database));
    assert.deepEqual({ gumpFrame: views[0].gumpFrame, capacity: views[0].capacity, constructionTime: views[0].constructionTime, duration: views[0].duration }, { gumpFrame: 0, capacity: 0, constructionTime: 0, duration: 0 });
    // 표면 조회: 256 이상은 칸 별칭으로 읽지 않고 0 이다.
    const hash = new SquidHash();
    hash.setCell(0, 0, 1, 9);
    hash.setCell(0, 1, 0, 8);
    const lookup = makeSpriteSurfaceLookup(hash);
    assert.deepEqual([lookup(256, 0), lookup(0, 256), lookup(256, 256), lookup(300, 0)], [0, 0, 0, 0]);
    // 픽셀 출력: 표면 밖으로 넘치는 clip 도 표면 원점(0,0)부터 전부 칠한다. 바인딩 블록이 블록 수와 같으면 프레임 범위 오류이다.
    const pixels = new Uint8Array(64).fill(9);
    new SpritePixelTarget(pixels, 8, 8, { left: -5, top: -5, right: 20, bottom: 20 }, null, null).hooks().fill({ left: -5, top: -5, right: 20, bottom: 20 }, 1);
    assert.equal(pixels.filter((value) => value === 1).length, 64);
    const shapes = pixelShape();
    const target = new SpritePixelTarget(new Uint8Array(64), 8, 8, { left: 0, top: 0, right: 8, bottom: 8 }, () => ({ database: shapes, block: 1 }), null);
    assert.throws(() => target.hooks().shape({ type: 70, frame: 0, position: { x: 0, y: 0 }, mode: 0 }), /^RangeError: Sprite Draw frame$/);
    void w0;
});

test('일반 객체 Draw 계약: 선택 프레임이 물리 프레임 수와 같으면 전용 오류이고 막대 폭은 넓은 헤더에서 건설 시작값·지속 시간 초기값을 정확히 반영한다', () => {
    // 논리 프레임 수가 물리 프레임 수보다 커도(5 > 1) 물리 프레임 수를 넘는 프레임은 선택 프레임 오류로 거부한다.
    const edge = scene();
    edge.raw[40] = 4;
    edge.views[70].shape.frameCount = 5;
    put(edge.raw, 36, 1, 4);
    assert.throws(() => run(edge), /^RangeError: Sprite Draw selection frame$/);
    // 넓은 헤더(폭 100)에서 건설 막대의 raw +46 값이 폭에 정확히 반영된다(짝수 4 → 16/20, 홀수 5 → 15/20).
    const wide = (mutate, hooks = {}) => {
        const w = scene();
        w.raw[40] = 4;
        w.views[70].shape.frames[0] = frameOf(100, 4, 1, 1);
        mutate(w);
        return run(w, 0, hooks).filter((event) => event.startsWith('R'));
    };
    const width = (fill) => Number(fill.split(',')[3]) - Number(fill.split(',')[1]);
    const construction = (value) => wide((w) => { w.s.shadowAliasType = 70; put(w.raw, 42, 70); w.views[70].constructionTime = 20; put(w.raw, 46, value); }, { constructionActive: () => true });
    assert.deepEqual([construction(4), construction(5), construction(0)].map((fills) => width(fills[1])), [80, 75, 100]);
    // 일치하는 상태 프로세스가 없으면 남은 시간 0(폭 0), 일치하면 남은 시간만큼이다.
    const status = (processes) => wide((w) => { w.raw[40] = 0x24; Object.assign(w.s, { statusProcessType: 41, statusType: 70, abstractStatusType: 70, now: 100 }); w.views[70].duration = 50; w.s.processes = processes; });
    const none = { type: 0, flags: 0, members: [], endTime: 0 };
    assert.equal(width(status([none])[1]), 0);
    assert.equal(width(status([none, { type: 41, flags: 2, members: [5], endTime: 101 }])[1]), 2);
});
