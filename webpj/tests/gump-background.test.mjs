// 원본 배경 계획59·가상/직접 출력209행과 실제 자산/픽셀·와이드 반복을 대조한다. 게임을 실행하지 않는다. (2026-11-14 WB04-E415)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, put } from './raw-support.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { buildShadeMaps } from '../www/assets/palette.js';
import { GumpCreateState, GumpCreateNode, GumpCreate } from '../www/ui/gump-create.js';
import { GumpBackgroundNode, GumpBackgroundCreate } from '../www/ui/gump-background-create.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { GumpDrawState, GumpPixelTarget, setGumpDrawClip } from '../www/render/gump-draw.js';
import { GumpBackgroundFlags as F, planGumpBackground, planGumpTexture } from '../www/render/gump-visual.js';
import { drawRawGumpBackground } from '../www/render/gump-background.js';
import { GumpBackgroundDrawState, GumpBackgroundDraw, GumpBackgroundAssets, GumpBackgroundPixelTarget, bindGumpBackgroundAssets, bindGumpBackgroundDraw, setGumpBackgroundColors } from '../www/render/gump-background-draw.js';

// 같은 현재 노드·기본 생성/Bounds와 실제 half-open/VFX 픽셀 저장소다.
function scene(width = 8, height = 8, initial = 9) {
    const state = new GumpCreateState(); state.screenWidth = width; state.screenHeight = height;
    const nodes = new Map([[1, new GumpCreateNode()], [2, new GumpBackgroundNode()]]); const node = nodes.get(2);
    const create = new GumpCreate(state, { node: (token) => nodes.get(token), allocate: () => 1 }); create.initializeRoot();
    const collection = new GumpCollection(state.collection, create.collectionHooks()); const clip = new GumpDrawState(); setGumpDrawClip(clip, { left: 0, top: 0, right: width, bottom: height });
    const guarded = new Uint8Array(width * height + 2).fill(initial); const pixels = guarded.subarray(1, -1); const target = new GumpPixelTarget(pixels, width, height, clip); const globals = new GumpBackgroundDrawState();
    return { state, nodes, node, create, collection, clip, guarded, pixels, target, globals };
}
// DWORD 사건 인자를 원본 관측 형식으로 기록한다.
const event = (kind, ...values) => [kind, ...values.map((v) => v >>> 0)].join(',');
const rectValues = (r) => [r.left, r.top, r.right, r.bottom];
const rect = (values) => { const [left, top, right, bottom] = values.map((v) => Number(v) | 0); return { left, top, right, bottom }; };
// 기대색을 손으로 구별할 수 있는 소스 색+10/+20 표다.
const maps = { bright: Uint8Array.from({ length: 256 }, (_, i) => i + 10), dark: Uint8Array.from({ length: 256 }, (_, i) => i + 20) };
// CPP 독립 입력과 같은 물리2/논리1 프레임이다. 두 번째 프레임은 메타5×4/실제3×3·투명 중앙이다.
function shapes() {
    const bytes = new Array(24).fill(0); put(bytes, 0, 0x30312e31); put(bytes, 4, 2);
    // 물리 프레임의36BYTE Squid 메타/24BYTE VFX 헤더·행 RLE를 순서대로 기록한다.
    for (let frame = 0; frame < 2; frame += 1) { const prefix = bytes.length; bytes.push(...new Array(60).fill(0)); put(bytes, 8 + frame * 8, prefix + 36); put(bytes, prefix + 24, frame ? 5 : 3, 2); put(bytes, prefix + 26, frame ? 4 : 3, 2); put(bytes, prefix + 36, 2, 2); put(bytes, prefix + 38, 2, 2); put(bytes, prefix + 52, 2); put(bytes, prefix + 56, 2);
        // 원본과 독립인 명시 색 입력만 제공한다.
        for (let y = 0; y < 3; y += 1) { for (let x = 0; x < 3; x += 1) bytes.push(...(frame && x === 1 && y === 1 ? [1, 1] : [2, [1, 2, 1, 3, 4, 3, 1, 2, 1][y * 3 + x] + frame * 4])); bytes.push(0); }
    }
    return new ShapeDatabase(Uint8Array.from(bytes));
}
// 실제 자산/현재 색 표를 제품 배경 출력기에 묶는다.
function bindScene(s, assets) { const createHooks = { node: (token) => s.nodes.get(token) }; const drawHooks = { node: createHooks.node }; bindGumpBackgroundAssets(createHooks, drawHooks, assets); const colors = new Uint8Array(256).fill(77); const raster = new GumpBackgroundPixelTarget(s.target, assets, maps, colors); const hooks = raster.hooks(s.clip, drawHooks); return { colors, raster, hooks, createHooks, draw: new GumpBackgroundDraw(s.collection, s.clip, s.globals, hooks) }; }

test('GumpBackground_Native59: 원본10.78 명암1536바이트·전체56계획의 clip/명암/여분 타일이 같다', () => {
    const rows = fs.readFileSync(path.join(REPO_ROOT, 'cpppj/tests/fixtures/gumpbackground-x86.tsv'), 'utf8').split(/\r?\n/).filter((line) => line.startsWith('originals')).map((line) => line.split(/\s+/)); assert.equal(rows.length, 59); let mapCount = 0;
    // 사건 기대값은 원본 fixture 열만 읽고 현재 결과를 그대로 직렬화한다.
    for (const row of rows) { if (row[1] === 'maps') { const bytes = Buffer.from(row[3], 'hex'); const logical = Uint32Array.from({ length: 256 }, (_, i) => bytes.readUInt32LE(i * 4)); const actual = buildShadeMaps(logical); assert.equal(Buffer.from(actual.bright).toString('hex'), row[4]); assert.equal(Buffer.from(actual.dark).toString('hex'), row[5]); mapCount += 1; continue; }
        assert.equal(row.length, 15); const clip = rect(row.slice(7, 11)); const passes = planGumpBackground(rect(row.slice(3, 7)), clip, Number(row[12]), Number(row[13]), Number(row[11])); const events = [];
        // 모든 pass/clip/명암/여분 타일과 복구 순서를 비교한다.
        for (const pass of passes) { events.push(['p', ...rectValues(pass.rect), pass.flags].join(',')); if (!pass.validTile) continue; events.push(['c', ...rectValues(pass.clip)].join(',')); if (pass.shade) events.push(`m,${pass.shade}`); for (const p of pass.origins) events.push(`d,${p.x},${p.y},${pass.shade}`); events.push(['c', ...rectValues(clip)].join(',')); }
        assert.equal(events.join('|'), row[14], row.slice(0, 3).join(':'));
    }
    assert.equal(mapCount, 3);
});

test('GumpBackgroundDraw_Native209: 가상/직접 분배·모든 사건·768 실제 fill 픽셀·clip이 원본과 같다', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/gumpbackgrounddraw-x86.tsv')).filter((row) => row[0] === 'originals'); assert.equal(rows.length, 209);
    // 원본이 실행한 선/VFX 그림의 호출 경계와 실제 소프트웨어 fill을 구별한다.
    for (const row of rows) { assert.equal(row.length, 20); const s = scene(32, 24, 77); Object.assign(s.node, { x: Number(row[2]), y: Number(row[3]), width: bitsToFloat(row[4]), height: bitsToFloat(row[5]), backgroundFlags: Number(row[6]), type: Number(row[7]), frame: Number(row[8]), shape: 0xdeadbeef, tileWidth: 123, tileHeight: 456 }); const saved = rect(row[11].split(',')); setGumpDrawClip(s.clip, saved); Object.assign(s.globals, { debug: Number(row[12]) !== 0, debugColor: Number(row[13]), nearColor: Number(row[14]), farColor: Number(row[15]) }); s.pixels.set(Uint8Array.from({ length: 768 }, (_, i) => i * 19 + 37)); const events = []; let current = 0;
        const draw = new GumpBackgroundDraw(s.collection, s.clip, s.globals, { node: (token) => s.nodes.get(token), frame: (type, frame) => { events.push(event('T', type, frame)); return { width: Number(row[9]), height: Number(row[10]) }; }, clip: (r) => { events.push(event('C', ...rectValues(r))); setGumpDrawClip(s.clip, r); }, shade: (shade) => { current = shade; events.push(event('M', shade)); }, shape: (r) => events.push(event('D', r.type, r.frame, r.position.x, r.position.y, r.mapped ? current : 0)), fill: (r, color, checker) => { events.push(event('F', ...rectValues(r), color, checker ? 1 : 0)); s.target.fill(r, color, checker); }, line: (r) => events.push(event('L', r.first.x, r.first.y, r.last.x, r.last.y, r.color)) });
        if (row[1] === 'Virtual') draw.draw(2); else draw.background({ left: s.node.x, top: s.node.y, right: Math.trunc(s.node.x + s.node.width), bottom: Math.trunc(s.node.y + s.node.height) }, s.node.backgroundFlags, s.node.type, s.node.frame);
        assert.deepEqual([events.join(';') || '-', s.globals.debugColor, current, Buffer.from(s.pixels).toString('hex')], [row[16], Number(row[17]), Number(row[18]), row[19]], row.slice(0, 9).join(':')); assert.deepEqual(s.clip.clip, saved); assert.equal(s.guarded[0], 77); assert.equal(s.guarded.at(-1), 77);
    }
});

test('GumpBackgroundCreate 같은 기본 생성/등록·현재 signed색과 프레임 뒤 shape·type0 보존', () => {
    const s = scene(); const assets = new GumpBackgroundAssets(shapes(), [{ type: 82, block: 0, frameCount: 1, flags: 0 }, { type: 83, block: 0, frameCount: 1, flags: 0x440000 }]); const b = bindScene(s, assets); let color = -7; const create = new GumpBackgroundCreate(s.state, s.create, () => color, b.createHooks); create.construct(2, 13, 9, F.BUTTON, 1, 83, 1); assert.deepEqual([s.node.shape, s.node.tileWidth, s.node.tileHeight, s.node.command, s.node.width, s.node.height, s.state.count], [83, 5, 4, 0xfffffff9, 13, 9, 2]); assert.equal(assets.type(83), assets.type(83)); assert.equal(assets.frame(83, 0, 1), assets.frame(83, 0, 1)); assert.notEqual(assets.type(82), assets.type(83)); color = 121; create.construct(2, 11, 8, 0, 0x100000, 0, 0); assert.deepEqual([s.node.shape, s.node.tileWidth, s.node.tileHeight, s.node.type, s.node.command, s.state.count], [83, 5, 4, 0, 121, 2]);
    const asset = { token: 82, shape: 7 }; const hooks = { node: (token) => s.nodes.get(token), type: () => { s.node.frame = 1; return asset; }, frame: (type, abstract, frame) => { assert.deepEqual([type, abstract, frame], [82, 0, 1]); asset.shape = 99; return { width: -2, height: -3 }; } }; new GumpBackgroundCreate(s.state, s.create, () => -128, hooks).construct(2, 1, 2, 3, 0x100000, 82, 0); assert.deepEqual([s.node.shape, s.node.tileWidth, s.node.tileHeight, s.node.command, s.node.frame], [99, -2, -3, 0xffffff80, 1]); assert.throws(() => new GumpBackgroundCreate(s.state, s.create, () => 0, { node: () => new GumpBackgroundNode() }).construct(2, 1, 1, 0, 0, 0, 0), /공유/);
});

test('GumpBackgroundAssets 실제 논리/물리 레이어·signed 프레임·바인딩 계약', () => {
    const database = shapes(); const binding = { type: 82, block: 0, frameCount: 1, flags: 0 }; const assets = new GumpBackgroundAssets(database, [binding, { ...binding, type: 83, flags: 0x440000 }]); assert.equal(assets.frame(82, 0, 0).width, 3); assert.equal(assets.frame(83, 0, 1).width, 5);
    // 잘못된 입력은 각 실제 호출에서 거부한다.
    for (const args of [[82, 0, 1], [83, 0, 2], [83, 0, -1], [83, 1, 0], [0, 0, 0], [1000, 0, 0]]) assert.throws(() => assets.frame(...args));
    for (const bindings of [[binding, binding], [{ ...binding, type: 0 }], [{ ...binding, block: 1 }], [{ ...binding, frameCount: 0 }], [{ ...binding, frameCount: 3 }], [{ ...binding, repeatFrame: 1 }]]) assert.throws(() => new GumpBackgroundAssets(database, bindings));
});

test('GumpBackgroundDraw 실제 SHP·현재 type/frame·투명·공유 명암과 원문 테두리 픽셀', () => {
    const assets = new GumpBackgroundAssets(shapes(), [{ type: 82, block: 0, frameCount: 1, flags: 0 }, { type: 83, block: 0, frameCount: 1, flags: 0x440000 }]); const s = scene(); Object.assign(s.node, { x: 1, y: 1, width: 6, height: 6, type: 82, backgroundFlags: F.BUTTON, shape: 999, tileWidth: 99, tileHeight: 99 }); const b = bindScene(s, assets); b.draw.draw(2); assert.deepEqual([s.pixels[27], s.pixels[11], s.pixels[51]], [4, 14, 22]); assert.deepEqual(b.colors, maps.dark); assert.deepEqual([s.guarded[0], s.guarded.at(-1), s.clip.clip.right, s.clip.vfxClip.right], [9, 9, 8, 7]); s.pixels.fill(9); s.node.backgroundFlags |= F.PRESSED; b.draw.draw(2); assert.deepEqual([s.pixels[11], s.pixels[51]], [24, 12]); assert.deepEqual(b.colors, maps.bright); s.pixels.fill(9); Object.assign(s.node, { type: 83, frame: 1, backgroundFlags: F.TEXTURE }); b.draw.draw(2); assert.deepEqual([s.pixels[9], s.pixels[28], s.node.shape, s.node.tileWidth], [9, 5, 999, 99]); assert.deepEqual(b.colors, maps.bright);
});

test('GumpBackground 순수 IndexedImage 출력·상대점/opacity와 과도 반복·감김 안전 경계', () => {
    const texture = { width: 3, height: 3, indices: Uint8Array.of(1, 2, 1, 3, 4, 3, 1, 2, 1), opacity: new Uint8Array(9).fill(255) }; const bounds = { left: 0, top: 0, right: 8, bottom: 8 }; const area = { left: 1, top: 1, right: 7, bottom: 7 };
    // 정상/눌림은 화면 격자·테두리 덮기 순서의 수동 기대색을 사용한다.
    for (const pressed of [false, true]) { const canvas = { width: 8, height: 8, indices: new Uint8Array(64).fill(9), opacity: new Uint8Array(64).fill(255) }; drawRawGumpBackground(canvas, texture, { x: 0, y: 0 }, planGumpBackground(area, bounds, 3, 3, F.BUTTON | (pressed ? F.PRESSED : 0)), maps); assert.deepEqual([canvas.indices[27], canvas.indices[11], canvas.indices[51], canvas.indices[14], canvas.indices[49]], pressed ? [4, 24, 12, 23, 12] : [4, 14, 22, 13, 22]); assert.equal(canvas.indices[0], 9); assert.equal(canvas.indices[63], 9); assert.ok(canvas.opacity.every((v) => v === 255)); }
    const canvas = { width: 8, height: 8, indices: new Uint8Array(64).fill(9) }; drawRawGumpBackground(canvas, { ...texture, opacity: new Uint8Array(9) }, { x: -1, y: -1 }, planGumpBackground(area, bounds, 3, 3, F.BUTTON), maps); assert.ok(canvas.indices.every((v) => v === 9)); drawRawGumpBackground(canvas, texture, { x: 1, y: 1 }, planGumpBackground(area, { left: 3, top: 3, right: 5, bottom: 5 }, 3, 3, F.TEXTURE), maps); assert.deepEqual([canvas.indices[27], canvas.indices[36], canvas.indices[19], canvas.indices[29]], [1, 4, 9, 9]);
    for (const flags of [0, 0x06000000, 0x0a000000]) assert.throws(() => planGumpBackground(area, bounds, 3, 3, flags)); assert.throws(() => planGumpTexture({ ...area, left: 9 }, bounds, 0, 0, 0)); assert.throws(() => planGumpBackground({ left: 0, top: 0, right: 100000, bottom: 100000 }, bounds, 2, 2, F.BUTTON), /범위/); assert.throws(() => planGumpBackground({ left: 0x7fffffff, top: 0, right: 0x7fffffff, bottom: 4 }, bounds, 3, 3, F.BUTTON));
});

test('GumpBackgroundDraw flat/checker/debug 실제 픽셀·현재 색 조회와 증가 순서', () => {
    const s = scene(); Object.assign(s.node, { x: 1, y: 1, width: 6, height: 6, backgroundFlags: 0x0fe40091 }); Object.assign(s.globals, { debugColor: 0xffffffff, nearColor: 77, farColor: 33 }); const hooks = { node: (token) => s.nodes.get(token), frame: () => { throw new Error('flat 자산 조회 금지'); }, fill: (...args) => s.target.fill(...args), line: (line) => s.target.lines([line]) }; const draw = new GumpBackgroundDraw(s.collection, s.clip, s.globals, hooks); draw.draw(2); assert.deepEqual([s.pixels[11], s.pixels[51], s.pixels[27], s.pixels[28]], [77, 33, 9, 145]); s.node.backgroundFlags = F.SOLID | 42; s.pixels.fill(9); draw.draw(2); assert.equal(s.pixels[27], 42); s.globals.debug = true; draw.draw(2); assert.deepEqual([s.globals.debugColor, s.pixels[28], s.pixels[27]], [0, 255, 42]); setGumpBackgroundColors(s.globals, { basic: [0, 0, 0, 0, 0, 0, 0, 19, 21] }); assert.deepEqual([s.globals.nearColor, s.globals.farColor, s.globals.debug, s.globals.debugColor], [19, 21, true, 0]);
    s.globals.debugColor = 7; hooks.fill = (r, color, checker) => { assert.equal(s.globals.debugColor, 8); assert.equal(color, 7); assert.equal(checker, true); }; draw.draw(2);
});

test('GumpBackgroundDraw 콜백 뒤 현재 clip/선 색·예외 복구·가상 fallback/필수 경계', () => {
    const s = scene(); Object.assign(s.globals, { nearColor: 11, farColor: 13 }); const colors = []; let shade = 0; let draws = 0; const saved = { left: 2, top: 2, right: 6, bottom: 6 }; const hooks = { node: (token) => s.nodes.get(token), line: (line) => { colors.push(line.color); s.globals.nearColor = 17; s.globals.farColor = 19; }, frame: () => { setGumpDrawClip(s.clip, saved); return { width: 3, height: 3 }; }, clip: (r) => setGumpDrawClip(s.clip, r), shade: (value) => { shade = value; }, shape: () => { if (++draws === 2) throw new Error('실제 출력 실패'); } }; const draw = new GumpBackgroundDraw(s.collection, s.clip, s.globals, hooks); draw.background({ left: 1, top: 1, right: 7, bottom: 7 }, 0x01e00000, 0, 0); assert.deepEqual(colors, [11, 17, 19, 19]); assert.throws(() => draw.background({ left: 1, top: 1, right: 7, bottom: 7 }, F.TEXTURE | F.DARK, 82, 0), /출력 실패/); assert.deepEqual([draws, shade, s.clip.vfxClip.right], [2, 2, 5]); assert.deepEqual(s.clip.clip, saved);
    hooks.frame = () => ({ width: 1, height: 3 }); delete hooks.clip; delete hooks.shade; delete hooks.shape; draw.background({ left: 1, top: 1, right: 7, bottom: 7 }, F.BUTTON, 82, 0); assert.throws(() => draw.background(saved, F.SOLID, 0, 0), /경계/); assert.throws(() => draw.draw(99)); assert.throws(() => new GumpBackgroundDraw(s.collection, s.clip, s.globals), /경계/); const fallback = []; const dispatch = { draw: (token) => fallback.push(token) }; bindGumpBackgroundDraw(dispatch, draw, (token) => token === 2); dispatch.draw(1); dispatch.draw(2); assert.deepEqual(fallback, [1]); assert.throws(() => bindGumpBackgroundDraw(dispatch, draw), /식별기/);
    let first = true; hooks.frame = () => ({ width: 3, height: 3 }); hooks.clip = (r) => { setGumpDrawClip(s.clip, r); if (first) { first = false; throw new Error('클립 실패'); } }; assert.throws(() => draw.texture(saved, 0, 82, 0), /클립 실패/); assert.deepEqual(s.clip.clip, saved);
});

// 명시 선두 검정/투명과 본문 행 색을 가진 독립 단일 프레임 RLE다. 원본 파일에는 쓰지 않는다.
function wideShapes(width = 80, height = 774, header = 25, transparent = false) { const bytes = new Array(76).fill(0); put(bytes, 0, 0x30312e31); put(bytes, 4, 1); put(bytes, 8, 52); put(bytes, 40, width + 1, 2); put(bytes, 42, height + 1, 2); put(bytes, 52, height - 1, 2); put(bytes, 54, width - 1, 2); put(bytes, 68, width - 1); put(bytes, 72, height - 1); for (let y = 0; y < height; y += 1) bytes.push(...(transparent && y < header ? [1, width, 0] : [width * 2, y < header ? 0 : y % 31 + 1, 0])); return new ShapeDatabase(Uint8Array.from(bytes)); }

test('GumpWidePanel 첫774행 전체 보존·749본문 주기·현재 dirty/명암 위상과 guard', () => {
    const database = wideShapes(); const binding = { type: 92, block: 0, frameCount: 1, flags: 0 }; const original = new GumpBackgroundAssets(database, [binding]); const wide = new GumpBackgroundAssets(database, [{ ...binding, repeatFrame: 0 }]); assert.equal(original.repeatBodyStart(92, 0), 0); assert.equal(wide.repeatBodyStart(92, 0), 25); const before = scene(80, 1700, 55); const after = scene(80, 1700, 55); const region = { left: 0, top: 0, right: 80, bottom: 1700 }; bindScene(before, original).draw.texture(region, F.TEXTURE, 92, 0); bindScene(after, wide).draw.texture(region, F.TEXTURE, 92, 0); assert.deepEqual(before.pixels.subarray(0, 774 * 80), after.pixels.subarray(0, 774 * 80)); assert.equal(before.pixels[774 * 80], 0); assert.equal(after.pixels[24 * 80 + 79], 0);
    // 기대 원본 행은 CPP 검사에도 명시한 독립 행쌍이다.
    for (const [y, source] of [[774, 25], [798, 49], [799, 50], [1080, 331], [1522, 773], [1523, 25], [1699, 201]]) for (let x = 0; x < 80; x += 1) assert.equal(after.pixels[y * 80 + x], source % 31 + 1);
    const dirty = scene(80, 1080, 55); const clip = { left: 3, top: 780, right: 77, bottom: 805 }; setGumpDrawClip(dirty.clip, clip); bindScene(dirty, wide).draw.texture({ left: 0, top: 26, right: 80, bottom: 1080 }, F.TEXTURE | F.BRIGHT, 92, 0);
    // 현재 dirty 밖을 쓰지 않고 최초 타일 뒤의 전역 본문 위상을 사용한다.
    for (let y = 0; y < 1080; y += 1) for (let x = 0; x < 80; x += 1) assert.equal(dirty.pixels[y * 80 + x], x >= 3 && x < 77 && y >= 780 && y < 805 ? (y - 749) % 31 + 11 : 55);
    assert.deepEqual(dirty.clip.clip, clip); assert.equal(dirty.clip.vfxClip.bottom, 804); assert.deepEqual([before.guarded[0], before.guarded.at(-1), after.guarded[0], after.guarded.at(-1), dirty.guarded[0], dirty.guarded.at(-1)], [55, 55, 55, 55, 55, 55]);
});

test('GumpWidePanel 실제 메타 크기와 불투명 색0만 선두로 취급하며 실제10.78 A02를 연결한다', () => {
    const binding = { type: 92, block: 0, frameCount: 1, flags: 0, repeatFrame: 0 }; const assets = new GumpBackgroundAssets(wideShapes(7, 13, 3), [binding]); assert.equal(assets.repeatBodyStart(92, 0), 3); const s = scene(7, 37, 55); bindScene(s, assets).draw.texture({ left: 0, top: 0, right: 7, bottom: 37 }, F.TEXTURE, 92, 0); assert.deepEqual([s.pixels[0], s.pixels[20], s.pixels[91], s.pixels[160], s.pixels[161]], [0, 0, 4, 13, 4]); const transparent = new GumpBackgroundAssets(wideShapes(7, 13, 3, true), [binding]); assert.equal(transparent.repeatBodyStart(92, 0), 0); const t = scene(7, 37, 55); bindScene(t, transparent).draw.texture({ left: 0, top: 0, right: 7, bottom: 37 }, F.TEXTURE, 92, 0); assert.deepEqual([t.pixels[0], t.pixels[97], t.pixels[112]], [55, 55, 4]);
    const raw = realCaptureAssets(); const game = new GameAssets(raw.table, raw.database); const real = new GumpBackgroundAssets(game); const panel = game.find('fortGump'); const type = panel.block + 70; assert.equal(real.repeatBodyStart(type, 2), 25); assert.equal(real.repeatBodyStart(type, 0), 0); const header = real.frame(type, 0, 2); assert.equal(header.height, 775); const a = scene(header.width - 1, 1080, 55); bindScene(a, real).draw.texture({ left: 0, top: 0, right: header.width - 1, bottom: 1080 }, F.TEXTURE, type, 2); const image = raw.database.decode(panel.block, 2);
    // 실제 첫 타일과749행 뒤의 본문은 독립 SHP 해독 바이트를 기대값으로 쓴다.
    for (let y = 0; y < 1080; y += 1) { const source = y < 774 ? y : 25 + (y - 774) % 749; assert.deepEqual(a.pixels.subarray(y * image.width, (y + 1) * image.width), image.indices.subarray(source * image.width, (source + 1) * image.width)); }
});
