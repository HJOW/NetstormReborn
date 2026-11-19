// Client의 실제 가상 파일에서 월드 자산을 구성하고 누락을 명시적으로 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameFiles } from '../www/assets/game-files.js';
import { TaffArchive } from '../www/assets/taff.js';
import { loadClientWorldAssets } from '../www/core/client-world-assets.js';
import { readData, readDataMap } from './helpers.mjs';

// 웹에 제공된 실제 아카이브·SHP·팔레트가 같은 타입 집계에 연결된다.
test('Client 월드 자산: 실제 가상 파일과 필수 자산 누락', () => {
    const files = new GameFiles({ loose: readDataMap(['d/_shapes.shp', 'd/GIFCLOUD.COL']), archives: [new TaffArchive(readData('netstorm.tarc'))] });
    const game = loadClientWorldAssets(files);
    assert.equal(game.types().length, 116); assert.equal(game.shapes.blocks.length, 116);
    assert.equal(game.typeNumber(game.find('isle')), 97); assert.equal(game.palette.rgb.length, 768);
    assert.deepEqual(loadClientWorldAssets(files, '\\D\\GIFCLOUD.COL').palette.rgb, game.palette.rgb);
    assert.throws(() => loadClientWorldAssets(files, 'd/missing.col'), /SHP 또는 팔레트/);
    assert.throws(() => loadClientWorldAssets(null), /파일 조회/);
    assert.throws(() => loadClientWorldAssets(new GameFiles()), /SHP 또는 팔레트/);
});
