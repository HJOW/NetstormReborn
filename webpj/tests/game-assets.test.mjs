// 게임 자산 이름 조회 층 assets/game-assets.js(cpppj `client/GameAssets.cpp` 의 Find·FrameIndex·SHP 블록 수 검사, E272 이월분)를 실제 10.78 자료와 계약으로 검사한다.
//  - 실제 자료: SHP 블록 116개와 로딩 순서 116개가 맞고, 파일 이름 별칭 3개(dude→Man·edgeFarm→efarm·anim→Flare)를 typename 으로도 찾으며, typename 'flare' 를 두 타입이 쓰는데 파일 이름 'flare' 가 먼저 선택된다.
//    모든 타입에 대해 find(파일 이름)·find(typename)이 앞선 로딩 순서의 일치와 같고, 모든 클러스터가 `layer × 클러스터 수 + 순번` 프레임이며 SHP 블록의 프레임 수와 맞아떨어진다.
//  - 계약: ASCII 대소문자 무시(비 ASCII 는 그대로), 없는 이름/클러스터는 Error, 레이어·프레임 범위 밖은 RangeError, 블록 수 불일치 거부, 중복 클러스터는 첫 일치, 정의가 null 인 타입은 typename 검색에서 건너뜀.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E385)
import test from 'node:test';
import assert from 'node:assert/strict';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { TYPE_LOAD_ORDER, FIRST_ASSET_TYPE_NUMBER, TypeTable } from '../www/assets/type-table.js';
import { GameAssets } from '../www/assets/game-assets.js';

// 실제 자료로 만든 저장소이다. (2026-11-13 WB04-E385)
const { table, database } = realCaptureAssets();
const assets = new GameAssets(table, database);

test('게임 자산: 실제 자료의 로딩 순서 목록이 타입 정의와 SHP 블록을 같은 번호로 잇는다', () => {
    const types = assets.types();
    assert.equal(types.length, 116);
    types.forEach((type, index) => {
        assert.equal(type.assetName, TYPE_LOAD_ORDER[index]);
        assert.equal(type.block, index);
        assert.equal(type.definition, table.definitions[index]);
        assert.equal(assets.typeNumber(type), FIRST_ASSET_TYPE_NUMBER + index);
    });
    assert.equal(assets.shapes, database);
    assert.equal(assets.typeTable, table);
    assert.throws(() => { types.push(null); }, TypeError);
});

test('게임 자산: find 는 파일 이름을 먼저 typename 을 다음에 찾고 대소문자를 구분하지 않으며 같은 이름이면 앞선 로딩 순서가 이긴다', () => {
    const types = assets.types();
    // 별칭: 파일 이름이 typename 과 다른 세 타입을 typename 으로도 찾는다.
    assert.equal(assets.find('dude').assetName, 'dude');
    assert.equal(assets.find('Man').assetName, 'dude');
    assert.equal(assets.find('MAN').assetName, 'dude');
    assert.equal(assets.find('efarm').assetName, 'edgeFarm');
    assert.equal(assets.find('Flare').assetName, 'flare');
    assert.equal(assets.find('anim').assetName, 'anim');
    assert.equal(assets.find('BRIDGE').assetName, 'bridge');
    // 모든 타입: 파일 이름 조회는 첫 일치, typename 조회는 같은 이름의 파일이 없으면 첫 typename 일치이다.
    for (const type of types) {
        assert.equal(assets.find(type.assetName), types.find((other) => other.assetName.toLowerCase() === type.assetName.toLowerCase()));
        const byFile = types.find((other) => other.assetName.toLowerCase() === type.definition.name.toLowerCase());
        const byName = types.find((other) => other.definition.name.toLowerCase() === type.definition.name.toLowerCase());
        assert.equal(assets.find(type.definition.name), byFile ?? byName, type.assetName);
    }
    assert.throws(() => assets.find('noSuchType'), /Unknown game asset type: noSuchType/);
    assert.throws(() => assets.find(''), /Unknown game asset type/);
});

test('게임 자산: frameIndex 는 layer × 클러스터 수 + 클러스터 순번이고 SHP 블록의 프레임 수와 맞아떨어지며 범위 밖을 거부한다', () => {
    let checked = 0;
    for (const type of assets.types()) {
        const clusters = type.definition.clusters;
        if (clusters.length === 0) continue;
        const frames = database.blocks[type.block].frames.length;
        // 클러스터 순번마다 첫 레이어는 그 순번(첫 일치) 그대로이다.
        clusters.forEach((cluster, index) => {
            const first = clusters.findIndex((other) => other.name.toLowerCase() === cluster.name.toLowerCase());
            assert.equal(assets.frameIndex(type, cluster.name, 0), first, `${type.assetName}/${cluster.name}`);
            assert.equal(assets.frameIndex(type, cluster.name.toUpperCase(), 0), first);
            checked += 1;
        });
        // 마지막 유효 레이어와 그다음 레이어의 경계: 프레임 수 이상이 되는 레이어는 RangeError 이다.
        const name = clusters[0].name;
        const layers = Math.ceil(frames / clusters.length);
        assert.equal(assets.frameIndex(type, name, layers - 1), (layers - 1) * clusters.length);
        assert.throws(() => assets.frameIndex(type, name, layers), RangeError, type.assetName);
    }
    assert.ok(checked > 100);
    const bridge = assets.find('bridge');
    assert.equal(assets.frameIndex(bridge, 'A11', 0), 1);
    assert.throws(() => assets.frameIndex(bridge, 'noCluster', 0), /Unknown shape cluster: noCluster/);
    for (const bad of [-1, 1.5, Number.NaN, 2 ** 60]) assert.throws(() => assets.frameIndex(bridge, 'A11', bad), /Shape layer index/, String(bad));
});

test('게임 자산 계약: SHP 블록 수가 로딩 순서와 다르면 거부하고 정의가 없는 타입은 typename 검색에서 건너뛰며 중복 클러스터는 첫 일치이다', () => {
    assert.throws(() => new GameAssets(table, { blocks: database.blocks.slice(0, 115) }), /Shape block count does not match/);
    assert.throws(() => new GameAssets(table, { blocks: [...database.blocks, database.blocks[0]] }), /Shape block count does not match/);
    // 정의가 null 인 타입(읽지 못한 .type)은 파일 이름으로만 찾고 frameIndex 는 거부한다.
    const holes = new TypeTable(table.definitions.map((definition, index) => (index === 0 ? null : definition)));
    const partial = new GameAssets(holes, database);
    assert.equal(partial.find('dude').assetName, 'dude');
    assert.throws(() => partial.find('Man'), /Unknown game asset type/);
    assert.throws(() => partial.frameIndex(partial.find('dude'), 'A01', 0), /Shape layer index/);
    // 중복 클러스터: 이름이 같은 두 클러스터가 있으면 첫 번째의 프레임을 쓴다.
    const duplicate = { assetName: 'x', block: 0, definition: { name: 'x', clusters: [{ name: 'AA' }, { name: 'bb' }, { name: 'aa' }] } };
    const shapes = { blocks: Array.from({ length: 116 }, () => ({ frames: new Array(6) })) };
    const synthetic = new GameAssets({ definitions: new Array(116).fill(null) }, shapes);
    assert.equal(synthetic.frameIndex(duplicate, 'AA', 1), 3);
    assert.equal(synthetic.frameIndex(duplicate, 'bb', 1), 4);
    assert.throws(() => synthetic.frameIndex(duplicate, 'aa', 2), /Shape frame index/);
    assert.throws(() => synthetic.frameIndex({ ...duplicate, block: 116 }, 'AA', 0), /Shape type block index/);
});

test('게임 자산 계약: 타입 정의 수가 로딩 순서와 다르면 거부하고 이름 비교는 ASCII 대소문자만 무시하며 정의가 없는 타입이 문자열 undefined 와 일치하지 않는다', () => {
    const shapes = { blocks: Array.from({ length: 116 }, () => ({ frames: [] })) };
    assert.throws(() => new GameAssets({ definitions: new Array(115).fill(null) }, shapes), /타입 정의 수가 로딩 순서와 다릅니다/);
    // 정의 없는 타입의 typename 은 없다 — 이름이 'undefined' 여도 일치하지 않는다.
    const none = new GameAssets({ definitions: new Array(116).fill(null) }, shapes);
    assert.throws(() => none.find('undefined'), /Unknown game asset type/);
    assert.equal(none.find('dude').assetName, 'dude');
    // 비 ASCII 대문자는 그대로 비교한다(É 와 é 는 다른 이름, ASCII 부분만 대소문자 무시).
    const definitions = new Array(116).fill(null);
    definitions[5] = { name: 'Éa', clusters: [] };
    const wide = new GameAssets({ definitions }, shapes);
    assert.equal(wide.find('ÉA').block, 5);
    assert.throws(() => wide.find('éa'), /Unknown game asset type/);
});
