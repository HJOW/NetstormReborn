// 자산 복사본·매니페스트와 원본 자산 해독기를 검사한다.
// (2026-10-23 WB03-M1: 메인 메뉴 자산 묶음 검증)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import prepareTool from '../tools/prepare-assets.js';
import { REPO_ROOT, readData } from './helpers.mjs';
import { GamePalette, buildButtonPalette, buildScreenPalette, buildShadeMaps, findPaletteColor } from '../www/assets/palette.js';
import { decodeGif } from '../www/assets/gif.js';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { BitmapFont } from '../www/assets/bitmap-font.js';
import { encodeWindows1252 } from '../www/assets/text-encoding.js';
import { decodeCursorResource } from '../www/ui/cursor.js';
import { FORT_GUMP_BLOCK, MENU_ASSET_PATHS, STONE_BUTTON_TEXTURE_FRAME } from '../www/assets/menu-assets.js';
import { MANIFEST_FORMAT } from '../www/assets/asset-loader.js';

// 매니페스트의 모든 파일이 원본과 같은 크기·해시로 복사돼 있는지 확인한다.
test('자산 복사본과 매니페스트가 assets/game-data 원본과 같다', () => {
    assert.deepEqual(prepareTool.checkAssets(), []);
    const manifest = JSON.parse(fs.readFileSync(prepareTool.MANIFEST_PATH, 'utf8'));
    assert.equal(manifest.format, MANIFEST_FORMAT);
    assert.equal(prepareTool.MANIFEST_FORMAT, MANIFEST_FORMAT);
    const paths = manifest.files.map((entry) => entry.path);
    // 런타임이 쓰는 메인 메뉴 경로가 모두 매니페스트에 있어야 한다.
    for (const assetPath of Object.values(MENU_ASSET_PATHS)) assert.ok(paths.includes(assetPath), assetPath);
});

// COL 팔레트와 원본 화면 팔레트 규칙(0 검정·255 흰색)을 확인한다.
test('팔레트: COL 해독과 0/255 고정, 버튼 색 검색', () => {
    const file = GamePalette.parse(readData(MENU_ASSET_PATHS.palette));
    assert.deepEqual([...file.rgb.subarray(765, 768)], [0, 255, 255]);
    const screen = buildScreenPalette(file);
    assert.deepEqual([...screen.rgb.subarray(0, 3)], [0, 0, 0]);
    assert.deepEqual([...screen.rgb.subarray(765, 768)], [255, 255, 255]);
    assert.deepEqual(buildButtonPalette(screen.logical), { black: 0, white: 195, edge: 15 });
    assert.equal(findPaletteColor(screen.logical, 255, 255, 255), 195);
    assert.throws(() => GamePalette.parse(new Uint8Array(10)));
});

// 원본 캐시 d/!color.dat(보호 원본, 읽기만 함)의 밝음(offset 768)·어두움(offset 512) 표와 계산 결과를 대조한다.
test('명암 변환표가 원본 캐시 !color.dat 과 512바이트 모두 같다', (t) => {
    const cachePath = path.join(REPO_ROOT, 'originals', 'd', '!color.dat');
    if (!fs.existsSync(cachePath)) {
        t.skip('originals/d/!color.dat 이 없는 환경');
        return;
    }
    const cache = fs.readFileSync(cachePath);
    const maps = buildShadeMaps(buildScreenPalette(GamePalette.parse(readData(MENU_ASSET_PATHS.palette))).logical);
    assert.deepEqual([...maps.bright], [...cache.subarray(768, 1024)]);
    assert.deepEqual([...maps.dark], [...cache.subarray(512, 768)]);
});

// GIF 해독기가 원본 배경 그림의 크기와 색 번호를 보존하는지 확인한다.
test('GIF: 타이틀 639x480, 구름 512x512, 색 번호 보존', () => {
    const title = decodeGif(readData(MENU_ASSET_PATHS.title));
    assert.equal(title.width, 639);
    assert.equal(title.height, 480);
    assert.equal(title.indices.length, 639 * 480);
    const clouds = decodeGif(readData(MENU_ASSET_PATHS.clouds));
    assert.equal(clouds.width, 512);
    assert.equal(clouds.height, 512);
    assert.ok(clouds.opacity.every((value) => value === 255));
    assert.throws(() => decodeGif(new Uint8Array([1, 2, 3])));
});

// 셰이프 데이터베이스의 블록 수와 돌 버튼 질감 프레임의 치수를 확인한다.
test('셰이프: 116블록, fortGump 프레임 1 메타 200x146·그림 199x145', () => {
    const shapes = new ShapeDatabase(readData(MENU_ASSET_PATHS.shapes));
    assert.equal(shapes.blocks.length, 116);
    const metrics = shapes.squidMetrics(FORT_GUMP_BLOCK, STONE_BUTTON_TEXTURE_FRAME);
    assert.equal(metrics.width, 200);
    assert.equal(metrics.height, 146);
    const texture = shapes.decode(FORT_GUMP_BLOCK, STONE_BUTTON_TEXTURE_FRAME);
    assert.equal(texture.width, 199);
    assert.equal(texture.height, 145);
    assert.equal(texture.left, 0);
    assert.equal(texture.top, 0);
    assert.strictEqual(shapes.decode(FORT_GUMP_BLOCK, STONE_BUTTON_TEXTURE_FRAME), texture);
});

// 원본 글꼴 캐시의 치수와 라벨 측정 폭(원본 버튼 폭 75 = Multiplayer 63 + 12)을 확인한다.
test('글꼴: 슬롯 0 높이 14, 라벨 측정 폭', () => {
    const font = BitmapFont.parse(readData(MENU_ASSET_PATHS.font));
    assert.equal(font.height, 14);
    assert.equal(font.ascent, 11);
    assert.equal(font.descent, 3);
    const widths = Object.fromEntries(['Campaign', 'Multiplayer', 'Demo', 'Help', 'Edit', 'Credits', 'Options', 'Quit'].map((label) => [label, font.measure(encodeWindows1252(label))]));
    assert.deepEqual(widths, { Campaign: 55, Multiplayer: 63, Demo: 32, Help: 24, Edit: 21, Credits: 42, Options: 44, Quit: 23 });
});

// Windows-1252 변환의 일반·특수 구간을 확인한다.
test('Windows-1252 변환', () => {
    assert.deepEqual([...encodeWindows1252('Aé€한')], [0x41, 0xe9, 0x80, 0x3f]);
});

// 원본 기본 커서(번호 1)의 크기·핫스폿과 반전 픽셀 없음을 확인한다.
test('커서: RT_CURSOR_8 32x32, 핫스폿 (10,6), 흰 화살표', () => {
    const cursor = decodeCursorResource(readData(MENU_ASSET_PATHS.cursor));
    assert.equal(cursor.width, 32);
    assert.equal(cursor.height, 32);
    assert.equal(cursor.hotspotX, 10);
    assert.equal(cursor.hotspotY, 6);
    assert.equal(cursor.inverted, 0);
    // 화살표 끝(10,6, 핫스폿과 같음)은 검정 테두리, 그 아래 (11,8)은 흰 채움이다.
    const pixel = (x, y) => [...cursor.rgba.subarray((y * 32 + x) * 4, (y * 32 + x) * 4 + 4)];
    assert.deepEqual(pixel(10, 6), [0, 0, 0, 255]);
    assert.deepEqual(pixel(11, 8), [255, 255, 255, 255]);
    assert.deepEqual(pixel(0, 0), [0, 0, 0, 0]);
});
