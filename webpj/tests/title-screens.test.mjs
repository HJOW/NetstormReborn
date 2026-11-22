// 시작 title 그림 세 장의 시간표·가운데 좌표·팔레트 픽셀 변환과 실제 원본 자산 해독을 확인한다(원문 Netstorm.exe 0x4ab3e0). (2026-11-22 WB04-E1184)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readData } from './helpers.mjs';
import { decodeGif } from '../www/assets/gif.js';
import { GamePalette, buildScreenPalette } from '../www/assets/palette.js';
import { TITLE_SCREEN_ASSETS } from '../www/assets/menu-assets.js';
import { TITLE_SCREEN_COUNT, TITLE_SCREEN_MILLISECONDS, titleScreenIndexAt, titleScreenOrigin, titleScreenRgba } from '../www/ui/title-screens.js';

test('시작 title 시간표: 세 장을 3초씩 보이고 9초에 끝난다', () => {
    assert.equal(TITLE_SCREEN_MILLISECONDS, 3000); assert.equal(TITLE_SCREEN_COUNT, 3);
    for (const [elapsed, expected] of [[0, 0], [1, 0], [2999, 0], [3000, 1], [5999, 1], [6000, 2], [8999, 2], [9000, 3], [100000, 3]]) assert.equal(titleScreenIndexAt(elapsed), expected, String(elapsed));
    // 음수·비유한 값은 첫 그림이다.
    for (const bad of [-1, NaN, Infinity, -Infinity, undefined]) assert.equal(titleScreenIndexAt(bad), 0);
});

test('시작 title 가운데 좌표: 640×480 을 화면 가운데에 놓고 큰 그림은 음수가 된다', () => {
    assert.deepEqual(titleScreenOrigin(1024, 768, 640, 480), { x: 192, y: 144 });
    assert.deepEqual(titleScreenOrigin(640, 480, 640, 480), { x: 0, y: 0 });
    assert.deepEqual(titleScreenOrigin(1920, 1080, 640, 480), { x: 640, y: 300 });
    assert.deepEqual(titleScreenOrigin(641, 481, 640, 480), { x: 0, y: 0 });
    assert.deepEqual(titleScreenOrigin(600, 400, 640, 480), { x: -20, y: -40 });
});

test('시작 title 그림 변환: 색 번호를 팔레트 RGB 불투명 픽셀로 바꾸고 잘못된 입력을 거부한다', () => {
    const rgb = new Uint8Array(768); rgb.set([10, 20, 30], 3); rgb.set([200, 100, 50], 6);
    const screen = { image: { width: 2, height: 2, indices: Uint8Array.of(0, 1, 2, 1) }, palette: { rgb } };
    assert.deepEqual([...titleScreenRgba(screen)], [0, 0, 0, 255, 10, 20, 30, 255, 200, 100, 50, 255, 10, 20, 30, 255]);
    assert.throws(() => titleScreenRgba(null), /필요/u);
    assert.throws(() => titleScreenRgba({ image: screen.image, palette: { rgb: new Uint8Array(10) } }), /필요/u);
    assert.throws(() => titleScreenRgba({ image: { width: 3, height: 2, indices: Uint8Array.of(0, 1) }, palette: { rgb } }), /불일치/u);
});

test('시작 title 실제 자산: 세 그림이 640×480 이고 팔레트로 모든 색 번호를 칠한다', () => {
    assert.equal(TITLE_SCREEN_ASSETS.length, TITLE_SCREEN_COUNT);
    // 원문 순서는 title0+title00.col, title6+title06.col, title7+title06.col 이다.
    assert.deepEqual(TITLE_SCREEN_ASSETS.map((screen) => [screen.gif, screen.palette]), [['d/title0.gif', 'd/TITLE00.COL'], ['d/title6.gif', 'd/TITLE06.COL'], ['d/TITLE7.GIF', 'd/TITLE06.COL']]);
    for (const screen of TITLE_SCREEN_ASSETS) {
        const image = decodeGif(readData(screen.gif)), palette = buildScreenPalette(GamePalette.parse(readData(screen.palette)));
        assert.equal(image.width, 640); assert.equal(image.height, 480); assert.equal(image.indices.length, 640 * 480);
        const pixels = titleScreenRgba({ image, palette });
        assert.equal(pixels.length, 640 * 480 * 4);
        // 모든 알파는 255 이고 그림이 한 가지 색만은 아니다.
        let opaque = true; const colors = new Set();
        for (let index = 0; index < pixels.length; index += 4) { if (pixels[index + 3] !== 255) opaque = false; colors.add(pixels[index] << 16 | pixels[index + 1] << 8 | pixels[index + 2]); }
        assert.ok(opaque, screen.gif); assert.ok(colors.size > 8, `${screen.gif} 색 ${colors.size}`);
    }
});
