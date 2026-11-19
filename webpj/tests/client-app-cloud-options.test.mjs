// 앱의 현재 설정과 장기 구름 전역이 최초 적재 및 이후 설정 쓰기에서 연결되는지 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { ARCHIVE_PATH, GameData, SETUP_PATH } from '../www/assets/game-data.js';
import { KencloudOption } from '../www/core/client-kencloud-globals.js';
import { readData } from './helpers.mjs';

// 실제 설정을 읽고 현재 난수나 구름 할당은 건드리지 않는다.
test('Client 구름 옵션: 실제 설정의 최초 값과 이후 쓰기를 같은 전역에 반영한다', () => {
    const app = new Netstorm(); app._initializeClientRuntime(false);
    const data = GameData.build({ files: new Map([[ARCHIVE_PATH, readData(ARCHIVE_PATH)], [SETUP_PATH, readData(SETUP_PATH)]]),
        language: 'english', onChange: (key, value) => app._onConfigWrite(key, value) });
    app._gameData = data; const cloud = app._clientKencloud, initialRandom = app._clientRandom.game.state;
    const expected = data.readInt('cloudSpeed', cloud.value(KencloudOption.speed).value).value >>> 0;
    app._syncClientCloudOptions(); assert.equal(cloud.value(KencloudOption.speed).value, expected);
    data.setInt('cloudSpeed', 7); data.setInt('cloudFury', -4);
    assert.equal(cloud.value(KencloudOption.speed).value, 7); assert.equal(cloud.value(KencloudOption.fury).value, 0xfffffffc);
    data.set('cloudSpeed', '11', { silent: true }); assert.equal(cloud.value(KencloudOption.speed).value, 7);
    data.set('DoneBridgeTheGap', 1); assert.equal(cloud.value(KencloudOption.speed).value, 11);
    assert.equal(cloud.allocations(), 0); assert.equal(app._clientRandom.game.state, initialRandom);
});

// 자산과 Client가 아직 없는 초기 수명에서는 다음 적재를 기다린다.
test('Client 구름 옵션: 초기 수명에 설정이 없어도 안전하다', () => {
    const app = new Netstorm(); assert.doesNotThrow(() => app._syncClientCloudOptions());
    app._initializeClientRuntime(false); assert.doesNotThrow(() => app._syncClientCloudOptions());
});
