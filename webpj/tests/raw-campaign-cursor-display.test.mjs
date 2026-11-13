// 실제 커서 표시기를 실제 자산·Factory·SP·시각·FontSet·Renderer와 조립한다. 전체 Session 부모 시험과 구별한다. (2026-11-14 WB04-E407)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { put, get, floatToBits } from './raw-support.mjs';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette, buildScreenPalette } from '../www/assets/palette.js';
import { FontSet, ALL_FONT_PATHS } from '../www/assets/font-set.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidProcessState } from '../www/core/squid-process.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { CursorDropState } from '../www/core/raw-cursor-drop.js';
import { ClientCursorGlobals } from '../www/core/client-cursor-globals.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { bindCursorLifecycleAssets } from '../www/core/raw-cursor-lifecycle.js';
import { ClientColorSortInput } from '../www/core/client-color-sort-input.js';
import { OwnerColorTableState } from '../www/core/owner-color-table-state.js';
import { buildPaletteColorTable } from '../www/core/palette-colors.js';
import { RawCampaignCursorDisplay, cursorDisplayLogicalPalette } from '../www/render/raw-campaign-cursor-display.js';
import { drawIndexedImage } from '../www/render/indexed-surface.js';
import { Renderer } from '../www/render/renderer.js';
import { decodeGumpTextContext } from '../www/render/gump-text-draw.js';

// 명시 표시 표면/전체 viewport다. 게임 창은 열지 않는다.
const WIDTH = 640; const HEIGHT = 480; const SCREEN = Object.freeze({ left: 0, top: 0, right: WIDTH, bottom: HEIGHT });
// 원본 경고 키의 입력 순서다. 치수는 실제 글꼴로 잰다.
const MESSAGES = ["Priest can't get here.", 'Must be placed over constructing artifact', 'Cannot place here', 'Energy not satisfied', 'Only on Controlled Island'];

// 화면의 BGRX 입력을 준비한다. 실제 팔레트 색 번호와 예약BYTE는 따로 유지한다.
function filePalette() {
    const p = buildScreenPalette(GamePalette.parse(readData('d/GIFCLOUD.COL'))); const bytes = new Uint8Array(1024);
    // 각 색의 RGB를 현재 화면의 BGRX 물리 순서로 둔다.
    for (let i = 0; i < 256; i += 1) { bytes[i * 4] = p.rgb[i * 3 + 2]; bytes[i * 4 + 1] = p.rgb[i * 3 + 1]; bytes[i * 4 + 2] = p.rgb[i * 3]; bytes[i * 4 + 3] = 0x5a; }
    return bytes;
}

// 표시기가 실제로 읽는 RawWorldCore 네 참조만 갖춘 조립 장면이다. 별도 게임 계산 모델은 만들지 않는다.
function scene(type = 75, installPalette = true) {
    const { table, database } = realCaptureAssets(); const assets = new GameAssets(table, database); const types = realPlayerAssets().types.map((r) => ({ ...r, hotspotX: r.hotspotX ?? 0, hotspotY: r.hotspotY ?? 0 }));
    const pool = new SidPool(24000, { server: false }); const factory = new SquidFactory(pool, types); const sid = factory.create(type, 2); const raw = pool.allocated(sid); const frame = table.definitions[type - 70].defaultFrame;
    put(raw, 14, floatToBits(20)); put(raw, 18, floatToBits(21)); put(raw, 22, 320, 2); put(raw, 24, 231, 2); put(raw, 36, frame);
    const drop = Object.assign(new CursorDropState(), { lockDepth: 0, localPlayer: 1, type, x: 20, y: 21, costFlags: 0, legal: 0 }); const owner = new ClientCursorGlobals(); const lifecycle = owner.lifecycle(); lifecycle.abstractType = type; lifecycle.previewCount = 1; lifecycle.previews[0] = sid;
    const placement = owner.placement(); Object.assign(placement, { placementAllowed: 1, descriptionAllowed: 1, spaceAllowed: 1, screenOrigin: { x: 100, y: 50 } }); const surfaces = Object.assign(new BridgeConnectState(), { bridgeType: 82, connectorType: 155, islandType: 94, stalagType: 95 }); const grid = new Uint8Array(144);
    const source = { pool, drop, lifecycle, placement, surfaces, grid }; const processState = new SquidProcessState(); const core = { pool, types, spots: new Uint8Array(65536), processState }; const known = { value: 1 }; const tracking = { value: 0 }; const sp = new ScrambledSpStore(new GameRandom(7), () => 17); sp.bindTrackingOffset(tracking); sp.initialize(); sp.set(1, floatToBits(5000));
    const display = new RawCampaignCursorDisplay(core, source, assets, sp, known); const palette = filePalette(); if (installPalette) display.setPalette(palette); display.setView(SCREEN);
    return { assets, types, pool, factory, sid, raw, frame, drop, owner, lifecycle, placement, surfaces, source, core, known, tracking, sp, display, palette };
}

// 본체만의 독립 기대: 명시 네 외곽선과 실제 SHP의 단일 합성이다(CPP CampaignSession 표시 시험의 계산).
function expectedBody(s, background = 199, colors = null) {
    const pixels = new Uint8Array(WIDTH * HEIGHT).fill(background); const type = s.raw[10]; const t = s.types[type]; const x = ((get(s.raw, 22, 2) << 16) >> 16) - s.placement.screenOriginX(); const y = ((get(s.raw, 24, 2) << 16) >> 16) - s.placement.screenOriginY();
    if (!s.lifecycle.currentGump && !(t.flags2 & 0x100004) && type !== s.surfaces.connectorType) {
        const left = x - t.footX * 16; const top = y - t.footY * 11; const ink = s.placement.placementAllowed ? s.display.settings().goodColor : s.display.settings().badColor;
        // 가로/세로 선은 양 끝 픽셀을 포함한다.
        for (let px = left; px < x; px += 1) { if (px >= 0 && px < WIDTH && top >= 0 && top < HEIGHT) pixels[top * WIDTH + px] = ink; if (px >= 0 && px < WIDTH && y - 1 >= 0 && y - 1 < HEIGHT) pixels[(y - 1) * WIDTH + px] = ink; }
        // 세로 두 변도 현재 표면 안에서만 직접 계산한다.
        for (let py = top; py < y; py += 1) { if (py >= 0 && py < HEIGHT && left >= 0 && left < WIDTH) pixels[py * WIDTH + left] = ink; if (py >= 0 && py < HEIGHT && x - 1 >= 0 && x - 1 < WIDTH) pixels[py * WIDTH + x - 1] = ink; }
    }
    const block = s.assets.types()[type - 70].block; const frame = get(s.raw, 36); const picture = s.assets.shapes.blocks[block].frames[frame]; drawIndexedImage(pixels, WIDTH, HEIGHT, s.assets.shapes.decode(block, frame), x - t.hotspotX + picture.rect.left, y - t.hotspotY + picture.rect.top, SCREEN, colors); return pixels;
}

test('CursorDisplay 실제 자산/카메라/dirty: kind4 두 깊이·전체 픽셀·SP피드백과 작은 dirty 뒤 전체 스냅샷을 유지한다', () => {
    const s = scene(); const sprites = s.display.sprites(SCREEN, 0); assert.equal(sprites.length, 2); assert.deepEqual(sprites.map((v) => v.order), [{ x: 20, y: 21, depth: s.types[75].zOrder }, { x: 20, y: 21, depth: s.types[75].zOrder - 1 }]); assert.deepEqual(s.display.sprites(SCREEN, 4), []);
    const actual = new Uint8Array(WIDTH * HEIGHT).fill(199); sprites[0].procedure.draw(actual, WIDTH, HEIGHT, SCREEN); assert.deepEqual(actual, expectedBody(s)); assert.ok(actual.filter((v) => v !== 199).length > 100);
    s.sp.set(1, floatToBits(8001)); s.core.processState.now = 12.5; s.display.feedback().countdown = 20; const renderer = new Renderer(WIDTH, HEIGHT); let calls = 0; renderer.setExtraCollection((cut, flags) => { calls += 1; return s.display.sprites(cut, flags); }); renderer.setScene([]); renderer.draw(actual, WIDTH); renderer.present(() => {});
    assert.equal(calls, 1); assert.deepEqual([s.display.feedback().countdown, s.display.feedback().highlight], [19, floatToBits(12.5)]); const snapshot = renderer.sceneImage(); assert.equal(s.display.feedback().countdown, 19);
    renderer.invalidate({ left: 0, top: 0, right: 2, bottom: 2 }); renderer.draw(actual, WIDTH); renderer.present(() => {}); assert.equal(calls, 2); assert.equal(s.display.feedback().countdown, 18); assert.deepEqual(renderer.sceneImage().indices, snapshot.indices);
    s.drop.lockDepth = 1; s.lifecycle.previews.fill(0); s.lifecycle.previewCount = 0; renderer.invalidateAll(); renderer.draw(actual, WIDTH); renderer.present(() => {}); assert.equal(calls, 3); assert.equal(s.display.feedback().countdown, 18); assert.deepEqual(s.display.sprites(SCREEN, 0), []); sprites[0].procedure.draw(new Uint8Array(0), 0, 0, SCREEN);
});

test('CursorDisplay 실제 경고/비용 글자: 현재 번역 키·FontSet·SHP를 소비하며 dirty 밖 픽셀을 보존한다', () => {
    const s = scene(); const fonts = new FontSet(new Map(ALL_FONT_PATHS.map((name) => [name, readData(name)]))); const hooks = {}; bindCursorLifecycleAssets(hooks, s.assets, s.types, fonts);
    s.lifecycle.costImage = hooks.costImage(92, 3).token; s.lifecycle.costText = '123'; s.lifecycle.warningContext[2] = s.display.textColors()[0]; s.lifecycle.costContext[2] = s.display.textColors()[1]; s.drop.costFlags = 0x40;
    Object.assign(s.placement, { placementAllowed: 0, descriptionAllowed: 0, spaceAllowed: 0, placementFlags: 0 });
    // 위치 계산 자체의 입력 치수도 같은 실제 글꼴로 측정한다.
    for (let index = 0; index < MESSAGES.length; index += 1) { const size = fonts.measure(fonts.encode(MESSAGES[index]), decodeGumpTextContext(s.lifecycle.warningContext)); s.lifecycle.warningWidths[index] = size.width; s.lifecycle.warningHeights[index] = size.height; }
    const cost = fonts.measure(fonts.encode(s.lifecycle.costText), decodeGumpTextContext(s.lifecycle.costContext)); s.drop.cursorWidth = cost.width; s.lifecycle.costHeight = cost.height; const translated = []; s.display.setText(fonts, (key) => { translated.push(key); return key; });
    const clip = { left: 100, top: 80, right: 400, bottom: 300 }; const pixels = new Uint8Array(WIDTH * HEIGHT).fill(199);
    // 본체/보조를 같은 dirty 표면에 출력한다.
    for (const sprite of s.display.sprites(clip, 0)) sprite.procedure.draw(pixels, WIDTH, HEIGHT, clip);
    assert.ok(translated.includes('Energy not satisfied')); assert.ok(pixels.some((v) => v !== 199));
    // 글자/격자/본체 어느 출력도 변경 영역 밖을 건드리지 않는다.
    for (let y = 0; y < HEIGHT; y += 1) {
        // 각 행의 dirty 바깥만 확인한다.
        for (let x = 0; x < WIDTH; x += 1) if (x < clip.left || x >= clip.right || y < clip.top || y >= clip.bottom) assert.equal(pixels[y * WIDTH + x], 199);
    }
});

test('CursorDisplay 공통OwnerColor: 현재 Player 색과 같은 토큰 저장소를 읽고 파일 팔레트 변경에도 공통 표를 보존한다', () => {
    const s = scene(82); const logical = cursorDisplayLogicalPalette(s.palette); const primary = new ClientColorSortInput(); primary.captureWithPrimary(logical); const owner = new OwnerColorTableState(); owner.initialize(primary.primary, 188, 82, { value: 0 }); s.surfaces.ownerColors[1] = 2; s.display.bindOwnerColors(owner); assert.equal(s.display.settings().sharedOwnerMaps, owner.tokens);
    const pixels = new Uint8Array(WIDTH * HEIGHT).fill(199); s.display.sprites(SCREEN, 0)[0].procedure.draw(pixels, WIDTH, HEIGHT, SCREEN); assert.deepEqual(pixels, expectedBody(s, 199, owner.color(82, 2))); const table = owner.color(82, 2); const tableBytes = table.slice();
    const changed = new Uint8Array(1024); changed.set([255, 255, 255, 0], 17 * 4); changed.set([0, 255, 255, 0], 19 * 4); changed.set([255, 255, 255, 0], 255 * 4); s.display.setPalette(changed, logical); assert.deepEqual([...s.display.textColors()], [17, 19]); assert.equal(owner.color(82, 2), table); assert.deepEqual(table, tableBytes); assert.equal(s.display.settings().sharedOwnerMaps, owner.tokens);
    s.surfaces.ownerColors[1] = -1; assert.throws(() => s.display.sprites(SCREEN, 0)[0].procedure.draw(pixels, WIDTH, HEIGHT, SCREEN), /owner 색표 범위/); assert.throws(() => s.display.bindOwnerColors(new OwnerColorTableState()), /최초 등록/);
});

test('CursorDisplay 현재 명명색/고정 lookup: 실제9표를 복사하며 외부 표 변경을 현재 파일 색과 혼동하지 않는다', () => {
    const s = scene(); const logical = cursorDisplayLogicalPalette(s.palette); const primary = new ClientColorSortInput(); primary.captureWithPrimary(logical); const maps = primary.rebuild(buildPaletteColorTable(logical).basic[1]); const illegal = maps[2].slice();
    s.display.setPalette(s.palette, logical, maps); maps[2].fill(7); s.placement.placementAllowed = 0; const pixels = new Uint8Array(WIDTH * HEIGHT).fill(199); s.display.sprites(SCREEN, 0)[0].procedure.draw(pixels, WIDTH, HEIGHT, SCREEN); assert.deepEqual(pixels, expectedBody(s, 199, illegal));
    const colors = s.display.textColors(); colors[0] = 1234; assert.notEqual(s.display.textColors()[0], 1234); assert.equal(s.display.settings(), s.display.settings()); assert.equal(s.display.feedback(), s.display.feedback());
});

test('CursorDisplay 현재 잠금/목록/부모·선택된 의존: 미선택은 생략하고 누락/다른 풀·비용·글꼴 오류를 거부한다', () => {
    const s = scene(75, false); const sprites = s.display.sprites(SCREEN, 0); const pixels = new Uint8Array(WIDTH * HEIGHT); assert.throws(() => sprites[0].procedure.draw(pixels, WIDTH, HEIGHT, SCREEN), /팔레트 미설치/); s.drop.lockDepth = 1; sprites[0].procedure.draw(new Uint8Array(), 0, 0, SCREEN); s.drop.lockDepth = 0; s.display.setPalette(s.palette);
    s.lifecycle.previews[0] = 0; sprites[0].procedure.draw(new Uint8Array(), 0, 0, SCREEN); s.lifecycle.previews[0] = s.sid; s.lifecycle.currentGump = 1; assert.throws(() => s.display.sprites(SCREEN, 0), /Gump 깊이/); let depth = 19; s.display.setExternal(() => depth, null); assert.equal(s.display.sprites(SCREEN, 0)[0].order.depth, 10); depth = 27; assert.equal(s.display.sprites(SCREEN, 0)[0].order.depth, 18); s.lifecycle.currentGump = 0;
    s.placement.descriptionAllowed = 0; assert.throws(() => sprites[1].procedure.draw(pixels, WIDTH, HEIGHT, SCREEN), /글꼴\/번역/); s.placement.descriptionAllowed = 1;
    assert.throws(() => new RawCampaignCursorDisplay({ ...s.core, pool: new SidPool(24000) }, s.source, s.assets, s.sp, s.known), /판본\/풀/); assert.throws(() => s.display.setPalette(new Uint8Array(3)), /256색/);
    const entries = new Uint8Array(32); assert.equal(s.display.collectDrawing(SCREEN, entries, 1), 2); const drawn = new Uint8Array(WIDTH * HEIGHT).fill(199); s.display.drawEntry(entries.subarray(0, 16), drawn, WIDTH, HEIGHT, SCREEN); assert.deepEqual(drawn, expectedBody(s));
    s.display.settings().ownerMaps[75] = true; assert.throws(() => sprites[0].procedure.draw(pixels, WIDTH, HEIGHT, SCREEN), /미연결 TYPE75/); const custom = new Uint8Array(256).fill(200); let requested = 0;
    s.display.setExternal(null, (type, owner) => { assert.deepEqual([type, owner], [75, 1]); requested += 1; return custom; }); pixels.fill(199); sprites[0].procedure.draw(pixels, WIDTH, HEIGHT, SCREEN); assert.equal(requested, 1); assert.deepEqual(pixels, expectedBody(s, 199, custom)); s.display.settings().ownerMaps[75] = false;
    const fonts = new FontSet(new Map(ALL_FONT_PATHS.map((name) => [name, readData(name)]))); s.display.setText(fonts, (key) => key); s.drop.costFlags = 0x40; s.lifecycle.costImage = 0;
    assert.throws(() => sprites[1].procedure.draw(pixels, WIDTH, HEIGHT, SCREEN), /cost image/);
});

test('CursorDisplay 화면 팔레트 변환: Screen BGRX와 구조체 RGBX는 예약BYTE까지 같고 잘못된 길이는 거부한다', () => {
    const palette = filePalette(); const logical = cursorDisplayLogicalPalette(palette); const colors = Array.from({ length: 256 }, (_, i) => ({ red: palette[i * 4 + 2], green: palette[i * 4 + 1], blue: palette[i * 4], reserved: palette[i * 4 + 3] })); assert.deepEqual(cursorDisplayLogicalPalette(colors), logical); assert.equal(logical[17] >>> 24, 0x5a); assert.throws(() => cursorDisplayLogicalPalette([]), /256색/);
});
