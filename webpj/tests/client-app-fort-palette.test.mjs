// 실제 게임 파일·구름·Screen과 색표로 앱 팔레트 전환 부모를 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { ARCHIVE_PATH, GameData, SETUP_PATH } from '../www/assets/game-data.js';
import { GamePalette } from '../www/assets/palette.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { IndexedPresenter } from '../www/render/presenter.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { readData } from './helpers.mjs';

// 보호된 웹 자산과 실제 Client 공통 수명을 만든다. Canvas는 숨김 장면이다.
function actualApp() {
    const app = new Netstorm(), asset = realCaptureAssets(), palette = GamePalette.parse(readData('d/GIFCLOUD.COL'));
    const files = new Map([[ARCHIVE_PATH, readData(ARCHIVE_PATH)], [SETUP_PATH, readData(SETUP_PATH)],
        ['d/Gifcloud.gif', readData('d/Gifcloud.gif')], ['d/GIFCLOUD.COL', readData('d/GIFCLOUD.COL')]]);
    app._initializeClientRuntime(false); app._gameData = GameData.build({ files }); app._syncClientCloudOptions();
    app._layout = { logicalWidth: 8, logicalHeight: 4, visible: false }; app._surface = new IndexedSurface(8, 4); app._presenter = new IndexedPresenter(null);
    const game = new GameAssets(asset.table, asset.database, palette); app._initializeClientPalette(game); app._initializeClientPaletteCache(game);
    return app;
}

// 실제 파일을 적재하고 같은 이름은 하위 구름·Screen·난수를 다시 건드리지 않는다.
test('Client 팔레트 전환: 실제 구름·파일·Screen과 고정 입력을 사용한다', (t) => {
    t.mock.method(console, 'info', () => {});
    const app = actualApp(), cloud = app._clientKencloud, colors = app._clientColorSort, screen = app._clientPaletteScreen;
    const fixed = colors.logical.slice(), initialRandom = app._clientRandom.game.state;
    app._surface.fill(9); screen.lock().fill(9); screen.unlock(); app._changeClientFortPalette(app._gameData.pathSpec('battlePal'));
    assert.ok(cloud.globals().cache); assert.equal(cloud.allocations(), 1); assert.notEqual(app._clientRandom.game.state, initialRandom);
    assert.equal(new TextDecoder().decode(cloud.paletteName().subarray(0, 8)), 'gifcloud');
    assert.ok(app._surface.pixels.every((value) => value === 0)); assert.deepEqual(colors.logical, fixed); assert.equal(colors.mapsReady, true);
    const cache = cloud.globals().cache, random = app._clientRandom.game.state;
    app._surface.fill(6); app._changeClientFortPalette('GIFCLOUD');
    assert.equal(cloud.globals().cache, cache); assert.equal(app._clientRandom.game.state, random);
    assert.ok(app._surface.pixels.every((value) => value === 6));
});

// 파일이 없으면 앞서 적용된 이름·구름 효과를 남기고 Screen 색표 후단은 실행하지 않는다.
test('Client 팔레트 전환: 없는 파일은 원본 순서의 부분 효과를 보존한다', (t) => {
    t.mock.method(console, 'info', () => {});
    const app = actualApp(), cloud = app._clientKencloud, palette = app._clientPaletteScreen.palette().slice();
    assert.throws(() => app._changeClientFortPalette('missing'), /팔레트 파일 누락/u);
    assert.equal(new TextDecoder().decode(cloud.paletteName().subarray(0, 7)), 'missing');
    assert.ok(cloud.globals().cache); assert.deepEqual(app._clientPaletteScreen.palette(), palette);
});
