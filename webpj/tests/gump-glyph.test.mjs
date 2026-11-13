// 원본 글리프 반복/글꼴 선택을 실제 캐시 해독과256개10.78 사건으로 대조한다. (2026-11-14 WB04-E406)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { BitmapFont } from '../www/assets/bitmap-font.js';
import { FontSet, fontCachePath } from '../www/assets/font-set.js';
import { createTextContext, drawBitmapGlyphText, drawGlyphText } from '../www/render/glyph-text.js';
import { GumpTextDraw, GumpGlyphColorState, makeGumpGlyphPixelHooks, decodeGumpTextContext } from '../www/render/gump-text-draw.js';
import { GumpDrawState, setGumpDrawClip, GumpPixelTarget } from '../www/render/gump-draw.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';

// 저장한 입력 폭/포인터와 원본 C/G/E 관측을 한 번만 읽는다.
const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/gumpglyph-x86.tsv'));
const unhex = (s) => Uint8Array.from(Buffer.from(s, 'hex'));

// 기대 출력과 다른 입력: 첫 폭99/ABC A−31·지정 셋째 폭·한 픽셀/특별 빈 글리프 캐시를 실제 parser에 공급한다.
function cache(advances, mask) {
    assert.equal(advances.length, 1024); assert.equal(mask.length, 256); const shapes = []; let total = 4132;
    // 글리프 포인터 유무는 원본처럼 블록 바이트 크기에서 복원한다.
    for (let code = 0; code < 256; code += 1) {
        if (!mask[code]) { shapes.push(null); continue; }
        const shape = new Uint8Array(code === 32 ? 40 : 43); put(shape, 0, 0x30312e31); put(shape, 4, 1); put(shape, 8, 16);
        if (code === 32) { put(shape, 24, 0x7fffffff); put(shape, 28, 0x7fffffff); put(shape, 32, 0x80000001); put(shape, 36, 0x80000001); }
        else shape.set([2, 100, 0], 40);
        shapes.push(shape); total += shape.length;
    }
    const bytes = new Uint8Array(total); bytes.set(Uint8Array.from('BitmapFontData\x1a\0', (c) => c.charCodeAt(0))); put(bytes, 16, 1); put(bytes, 20, 31); put(bytes, 24, 26); put(bytes, 28, 5); put(bytes, 32, 256); bytes.set(advances, 36 + 2048); let at = 4132;
    // 각 표와 실제 VFX 블록을 원본 캐시 순서로 놓는다.
    for (let code = 0; code < 256; code += 1) { put(bytes, 36 + code * 4, 99); put(bytes, 36 + 1024 + code * 4, 0xffffffe1); const shape = shapes[code]; if (shape) { put(bytes, 36 + 3072 + code * 4, shape.length); bytes.set(shape, at); at += shape.length; } }
    return bytes;
}

// 검사 소유 메모리 파일에 실제 캐시를 주고 FontSet 객체의 식별만 기록한다. 원본 파일은 쓰지 않는다.
function fixture() {
    const files = new Map(); const keys = [];
    // 모든 Font 행은 입력 표이며 기대 글리프 위치가 아니다.
    for (const row of rows.filter((r) => r[0] === 'Font')) { assert.equal(row.length, 5); const slot = Number(row[1]); const style = Number(row[2]); const name = fontCachePath(slot, style); assert.ok(name); files.set(name, cache(unhex(row[3]), unhex(row[4]))); keys.push([slot, style]); }
    const fonts = new FontSet(files); const identities = new Map();
    // 실제로 해독/선택한 글꼴 객체를 역으로 식별한다.
    for (const [slot, style] of keys) identities.set(fonts.get(slot, style), [slot, style]);
    return { fonts, identities };
}

// 독립 양수 전진 폭A2·공백3·NUL1과 없는B를 가진 실제1픽셀 캐시다.
function pixelFont() { const widths = new Uint8Array(1024); const mask = new Uint8Array(256); put(widths, 65 * 4, 2); put(widths, 32 * 4, 3); put(widths, 0, 1); mask[65] = 1; mask[32] = 1; mask[0] = 1; return BitmapFont.parse(cache(widths, mask)); }

// 실제 Gump 위치 조회가 사용할 빈 노드다.
const node = (fields = {}) => ({ flags: 0, next: 0, parent: 0, child: 0, object: 0, depth: 0, width: 0, height: 0, x: 0, y: 0, cached: { x: 0, y: 0 }, ...fields });

test('GumpGlyph_PatchNative: 실제 FontSet/캐시와 기본/명시Gump 위임이 원본256행의 C/G/E 사건과 같다', { skip: rows === null }, () => {
    const f = fixture(); const cases = rows.filter((r) => r[0] === 'Case' && r[1] === 'originals'); assert.equal(cases.length, 256);
    // 입력 문자열/문맥/좌표만 주며 기대 사건은 마지막 열에서 읽는다.
    for (const row of cases) {
        assert.equal(row.length, 15); const trace = []; let color = 0;
        const hooks = { color: (value) => { color = value; trace.push(`C,${value}`); }, illegalStyle: () => trace.push('E'), glyph: (font, code, at) => { const [slot, style] = f.identities.get(font); trace.push(`G,${slot},${style},${code},${at.x >>> 0},${at.y >>> 0},${color}`); } };
        const context = createTextContext({ flags: Number(row[5]), font: Number(row[3]), color: Number(row[6]), style: Number(row[4]), extra: 0xabcdef01, shadowColor: Number(row[7]), shadowX: Number(row[8]), shadowY: Number(row[9]) });
        const bytes = unhex(row[13]); const offset = { x: Number(row[10]), y: Number(row[11]) }; const draw = new GumpTextDraw(f.fonts, hooks);
        if (row[2] === 'Direct') draw.draw(bytes, offset, context, Number(row[12]));
        else { const current = node({ parent: 1, x: -13, y: 27 }); const root = node(); const collection = new GumpCollection({ root: 1 }, { node: (token) => token === 1 ? root : current }); const words = Uint32Array.from([context.flags, context.font, context.color, context.style, context.extra, context.shadowColor, context.shadowX, context.shadowY]); draw.drawGump(collection, 2, bytes, offset, words, row[2] === 'Default' ? null : context); assert.deepEqual(current.cached, { x: -13, y: 27 }); }
        assert.equal(trace.join(';'), row[14], row.slice(1, 5).join(' '));
    }
});

test('GumpGlyph_PixelsUseDrawAdvanceAndPreserveCurrentClipAndColorMap: 특별 빈 공백/없는B와 첫 폭·ABC A의 미사용을 지킨다', () => {
    const font = pixelFont(); const state = new GumpDrawState(); setGumpDrawClip(state, { left: 0, top: 0, right: 9, bottom: 3 }); const pixels = new Uint8Array(30).fill(7); const target = new GumpPixelTarget(pixels, 10, 3, state); const colors = new GumpGlyphColorState(); colors.fontColors[17] = 81; const hooks = makeGumpGlyphPixelHooks(target, colors); const bytes = Uint8Array.of(65, 32, 66, 65, 0);
    drawBitmapGlyphText(bytes, font, { x: 1, y: 1 }, createTextContext({ color: 0x123456ab }), hooks); assert.deepEqual([pixels[11], pixels[16], pixels.filter((v) => v === 171).length, colors.vfxColors[100], colors.vfxColors[17]], [171, 171, 2, 171, 81]);
    setGumpDrawClip(state, { left: 2, top: 1, right: 7, bottom: 2 }); pixels.fill(7); drawBitmapGlyphText(bytes, font, { x: 1, y: 1 }, createTextContext({ color: 44 }), hooks); assert.deepEqual([pixels[11], pixels[16], pixels.filter((v) => v === 44).length], [7, 44, 1]);
});

test('GumpGlyph_ShadowOutlineAndEmptyTextKeepColorInstallation: 그림자/네 외곽선/본문과 빈 글꼴·문자열의 최종 색을 지킨다', () => {
    const state = new GumpDrawState(); setGumpDrawClip(state, { left: 0, top: 0, right: 5, bottom: 5 }); const pixels = new Uint8Array(25).fill(7); const colors = new GumpGlyphColorState(); const hooks = makeGumpGlyphPixelHooks(new GumpPixelTarget(pixels, 5, 5, state), colors);
    drawBitmapGlyphText(Uint8Array.of(65, 0), pixelFont(), { x: 2, y: 2 }, createTextContext({ flags: 6, color: 211, shadowColor: 33, shadowX: 1, shadowY: 1 }), hooks); assert.deepEqual([pixels[12], pixels[7], pixels[13], pixels[17], pixels[11], pixels[18]], [211, 33, 33, 33, 33, 33]); const before = pixels.slice(); drawBitmapGlyphText(Uint8Array.of(0), null, { x: 0, y: 0 }, createTextContext({ flags: 6, color: 77, shadowColor: 55, shadowX: 1, shadowY: 1 }), hooks); assert.deepEqual(pixels, before); assert.equal(colors.vfxColors[100], 77);
});

test('GumpGlyph_ExplicitNulAndInvalidRangesRespectHostContract: 명시 길이의NUL도 출력하고 잘못된 범위는 효과 전에 거부한다', () => {
    const colors = []; const points = []; const hooks = { color: (value) => colors.push(value), glyph: (font, code, at) => points.push(at) }; const bytes = Uint8Array.of(65, 0, 65); const context = createTextContext({ color: 99 }); const font = pixelFont();
    drawBitmapGlyphText(bytes, font, { x: 0, y: 0 }, context, hooks, 3); assert.deepEqual(points, [{ x: 0, y: 0 }, { x: 2, y: 0 }, { x: 3, y: 0 }]); points.length = 0; colors.length = 0;
    assert.throws(() => drawBitmapGlyphText(bytes.subarray(0, 1), font, { x: 0, y: 0 }, context, hooks), /NUL/); assert.throws(() => drawBitmapGlyphText(bytes, font, { x: 0, y: 0 }, context, hooks, 4), /길이 범위/); assert.throws(() => drawBitmapGlyphText(bytes, font, { x: 0, y: 0 }, context, hooks, -2), /길이 범위/); assert.deepEqual(points, []); assert.deepEqual(colors, []);
    assert.throws(() => new GumpTextDraw({}, {}), /출력 경계/); assert.throws(() => decodeGumpTextContext([1]), /8DWORD/);
});

test('GumpGlyph_GumpPositionReadsCurrentDefaultContextAfterProjection: Position 뒤 현재 기본 문맥과 명시 문맥을 구별한다', () => {
    const f = fixture(); const words = Uint32Array.of(0, 2, 11, 0, 0, 0, 0, 0); const current = node({ flags: 0x2000, parent: 1, object: 50 }); const root = node(); const collection = new GumpCollection({ root: 1 }, { node: (token) => token === 1 ? root : current, project: (sid) => { assert.equal(sid, 50); words[2] = 77; return { x: 4, y: 8 }; } }); const installed = [];
    const draw = new GumpTextDraw(f.fonts, { color: (value) => installed.push(value), glyph: () => assert.fail('빈 슬롯2의 글리프 없음') }); draw.drawGump(collection, 2, Uint8Array.of(65, 0), { x: 2, y: 3 }, words); draw.drawGump(collection, 2, Uint8Array.of(65, 0), { x: 2, y: 3 }, words, createTextContext({ font: 2, color: 33 })); assert.deepEqual(installed, [77, 33]);
});

test('GumpGlyph_RendererUsesSharedBitmapLoop: 기존 UI 글자 출력도 같은 전진 폭/특별 빈 프레임/그림자/clip을 쓴다', () => {
    const surface = new IndexedSurface(10, 3); surface.pixels.fill(7); const context = createTextContext({ color: 171, flags: 2, shadowColor: 33, shadowX: 1, shadowY: 1 }); drawGlyphText(surface, pixelFont(), Uint8Array.of(65, 32, 66, 65), 1, 1, context);
    assert.deepEqual([surface.pixels[11], surface.pixels[16], surface.pixels[22], surface.pixels[27]], [171, 171, 33, 33]);
});
