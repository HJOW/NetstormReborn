// 앱 시작 팔레트와 고정 ColorSort 입력을 실제 게임 파일 및 두 수명으로 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { GamePalette, buildRgbaLookup } from '../www/assets/palette.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { IndexedPresenter } from '../www/render/presenter.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';
import { ScreenMode } from '../www/render/screen.js';
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

// 현재 화면 색의 BGRX를 표시기의 RGB로 연결하며 팔레트 변경 때 재표시를 예약한다.
test('Client 화면 팔레트: 실제 표시기의 색상과 갱신 요청을 바꾼다', () => {
    const app = new Netstorm(), palette = GamePalette.parse(readData('d/GIFCLOUD.COL'));
    app._layout = { logicalWidth: 1024, logicalHeight: 768 }; app._presenter = new IndexedPresenter(null);
    app._initializeClientPalette({ palette });
    const rgb = palette.rgb.slice(); rgb.fill(0, 0, 3); rgb.fill(255, 765, 768);
    assert.deepEqual(app._presenter.lookup, buildRgbaLookup(rgb));
    const changed = rgb.slice(); changed.set([17, 33, 97], 3 * 37);
    app._displayDirty = false; app._presenter.stale = false;
    app._clientPaletteScreen.loadPalette(new GamePalette(changed), 0);
    assert.deepEqual(app._presenter.lookup, buildRgbaLookup(changed));
    assert.equal(app._displayDirty, true); assert.equal(app._presenter.stale, true);
    assert.throws(() => app._applyClientScreenPalette(new Uint8Array(3)), /BGRX/u);
});

// 창 모드 DIB의 4바이트 정렬 행 간격과 전체 clip에서 clear가 실제 색 번호 영역만 지운다.
test('Client 화면 DIB: 팔레트 clear가 현재 표면을 지운다', () => {
    const app = new Netstorm(), palette = GamePalette.parse(readData('d/GIFCLOUD.COL'));
    app._layout = { logicalWidth: 7, logicalHeight: 4, visible: false }; app._presenter = new IndexedPresenter(null); app._surface = new IndexedSurface(7, 4); app._initializeClientPalette({ palette });
    const screen = app._clientPaletteScreen;
    assert.equal(screen.flags, ScreenMode.fallbackWindowed | ScreenMode.modeSet);
    assert.equal(screen.pitch, 8); assert.deepEqual(screen.clip(), { left: 0, top: 0, right: 7, bottom: 4 });
    screen.lock().fill(9); screen.unlock(); screen.loadPalette(palette, 1);
    const result = screen.lock().slice(); screen.unlock();
    // 패딩은 칠하지 않고 각 행의 논리 픽셀만 검정으로 만든다.
    for (let row = 0; row < 4; row += 1) assert.deepEqual([...result.subarray(row * 8, row * 8 + 8)], [0, 0, 0, 0, 0, 0, 0, 9]);
});

// 실제 Screen의 부분 출력은 패딩을 건너뛰고 같은 표면의 선택된 영역만 표시한다.
test('Client 화면 DIB: 부분 출력이 현재 표면과 표시기에 전달된다', () => {
    const app = new Netstorm(), palette = GamePalette.parse(readData('d/GIFCLOUD.COL')), calls = [];
    app._layout = { logicalWidth: 7, logicalHeight: 4, visible: true }; app._ctx = {};
    app._presenter = { setPalette: () => {}, present: (ctx, surface, layout, changed) => calls.push([ctx, surface, layout, changed]) };
    app._surface = new IndexedSurface(7, 4); app._surface.fill(3); app._initializeClientPalette({ palette });
    const screen = app._clientPaletteScreen, pixels = screen.lock(); pixels.fill(9); screen.unlock();
    screen.update({ left: 2, top: 1, right: 5, bottom: 3 });
    assert.equal(calls.length, 1); assert.equal(calls[0][0], app._ctx); assert.equal(calls[0][1], app._surface); assert.equal(calls[0][3], true);
    assert.deepEqual([...app._surface.pixels], [3,3,3,3,3,3,3, 3,3,9,9,9,3,3, 3,3,9,9,9,3,3, 3,3,3,3,3,3,3]);
    assert.equal(app._displayDirty, true);
    app._surface = null; assert.throws(() => screen.update({ left: 0, top: 0, right: 1, bottom: 1 }), /출력 수명 누락/u);
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

// 논리 크기 변경은 현재 화면색을 새 DIB로 옮기고 최초 입력과 loaded 상자를 유지한다.
test('Client 화면 크기: 현재 팔레트와 같은 고정 색표를 보존한다', () => {
    const app = new Netstorm(), assets = realCaptureAssets();
    const game = new GameAssets(assets.table, assets.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
    app._initializeClientRuntime(false); app._layout = { logicalWidth: 7, logicalHeight: 4, visible: false }; app._presenter = new IndexedPresenter(null);
    app._initializeClientPalette(game); app._initializeClientPaletteCache(game);
    const old = app._clientPaletteScreen, renderer = app._clientRenderer, owner = app._clientColorSort, loaded = old.paletteLoaded, fixed = owner.logical.slice();
    const display = app._campaignGameplayServices().surfaceDisplay; renderer.clearDirty();
    const rgb = game.palette.rgb.slice(); rgb.set([17, 33, 97], 37 * 3); old.loadPalette(new GamePalette(rgb), 0);
    const current = old.palette().slice(); app._resizeClientPaletteScreen(9, 4);
    const next = app._clientPaletteScreen;
    const nextRenderer = app._clientRenderer; assert.notEqual(nextRenderer, renderer); assert.equal(display.suppressed(), true);
    assert.notEqual(next, old); assert.equal(next.width, 9); assert.equal(next.pitch, 12); assert.equal(next.paletteLoaded, loaded);
    assert.deepEqual(next.palette(), current); assert.deepEqual(owner.logical, fixed); assert.equal(app._clientColorSort, owner);
    assert.equal(next.shadeMaps().bright, owner.maps[4]); assert.equal(next.shadeMaps().dark, owner.maps[6]);
    assert.deepEqual(next.clip(), { left: 0, top: 0, right: 9, bottom: 4 });
    app._resizeClientPaletteScreen(9, 4); assert.equal(app._clientPaletteScreen, next); assert.equal(app._clientRenderer, nextRenderer);
});
