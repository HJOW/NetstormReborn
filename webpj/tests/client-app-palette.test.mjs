// 앱 시작 팔레트와 고정 ColorSort 입력을 실제 게임 파일 및 두 수명으로 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { GamePalette } from '../www/assets/palette.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { readData } from './helpers.mjs';

// 실제 fortPal의 적용 색과 최초 캡처가 같고 이후 화면 색 변경이 캡처를 덮지 않는다.
test('Client 시작 팔레트: 실제 자산의 논리색을 최초 입력으로 고정한다', () => {
    const app = new Netstorm();
    const palette = GamePalette.parse(readData('d/GIFCLOUD.COL'));
    assert.throws(() => app._initializeClientPalette({ palette }), /수명 누락/u);
    app._layout = { logicalWidth: 1024, logicalHeight: 768 };
    app._initializeClientPalette({ palette });
    const screen = app._clientPaletteScreen, colors = app._clientColorSort;
    assert.equal(screen.paletteLoaded.value, 1);
    assert.equal(screen.width, 1024); assert.equal(screen.height, 768);
    assert.deepEqual(colors.logical, screen.logicalPalette());
    assert.equal(colors.logical[0], 0); assert.equal(colors.logical[255], 0xffffff);
    assert.equal(colors.primary.constructed, true); assert.equal(colors.mapsReady, false);
    const initial = colors.logical.slice();
    screen.loadPalette(new GamePalette(new Uint8Array(768).fill(73)), 0);
    assert.deepEqual(colors.logical, initial);
    assert.notDeepEqual(screen.logicalPalette(), initial);
    assert.throws(() => app._initializeClientPalette({ palette }), /중복/u);
});

// 오류가 난 초기 입력은 Client 슬롯에 발행하지 않고 다시 시도할 수 있다.
test('Client 시작 팔레트: 실패한 크기는 소유자를 발행하지 않는다', () => {
    const app = new Netstorm(); app._layout = { logicalWidth: 0, logicalHeight: 768 };
    assert.throws(() => app._initializeClientPalette({ palette: GamePalette.parse(readData('d/GIFCLOUD.COL')) }), /크기 오류/u);
    assert.equal(app._clientPaletteScreen, null); assert.equal(app._clientColorSort, null);
});

// 실제 자산의 첫 miss와 같은 페이지 재시작의 hit가 같은 캐시와 별도 Client 소유자를 사용한다.
test('Client 시작 색표: 실제 TYPE의 소유자 표와 캐시를 화면에 연결한다', () => {
    const app = new Netstorm(), assets = realCaptureAssets();
    const game = new GameAssets(assets.table, assets.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
    app._initializeClientRuntime(false); app._layout = { logicalWidth: 1024, logicalHeight: 768 };
    app._initializeClientPalette(game); app._initializeClientPaletteCache(game);
    const first = app._clientColorSort, cache = app._clientColorMapFiles.read('cache/!color.dat');
    assert.equal(cache.length, 2304); assert.equal(first.mapsReady, true); assert.equal(first.ownerTables.ready, true);
    assert.equal(app._clientPaletteScreen.shadeMaps().bright, first.maps[4]);
    assert.equal(app._clientPaletteScreen.shadeMaps().dark, first.maps[6]);
    assert.ok(first.ownerTables.color(game.typeNumber(game.find('bridge')), 1).length === 256);
    assert.throws(() => app._initializeClientPaletteCache(game), /중복/u);
    cache[0] ^= 0xff; app._clientColorMapFiles.put('cache/!color.dat', cache);
    app._clientPaletteScreen = null; app._clientColorSort = null;
    app._initializeClientPalette(game); app._initializeClientPaletteCache(game);
    assert.notEqual(app._clientColorSort, first);
    assert.deepEqual(app._clientColorMapFiles.read('cache/!color.dat'), cache);
    assert.equal(app._clientColorSort.maps[2][0], cache[0]);
    assert.notEqual(app._clientColorSort.maps[2][0], first.maps[2][0]);
    assert.equal(app._clientColorSort.ownerTables.ready, true);
});
