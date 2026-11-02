// 지형 장면과 개발용 화면의 연결을 검사한다. 게임 월드/미션 상태를 기대값으로 만들어 쓰지 않는다. (2026-11-02 WB03-AA)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, WEBPJ_ROOT, readDataMap } from './helpers.mjs';
import { GameData } from '../www/assets/game-data.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { buildTerrainScene, loadTerrainScene, parseZOrder, terrainMapName, typeDrawDepth, Z_ORDERS } from '../www/assets/terrain-scene.js';
import { createFortTypes } from '../www/assets/type-flags.js';
import { parseFort } from '../www/assets/fort.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';
import { TerrainPreview } from '../www/ui/terrain-preview.js';
import { Netstorm, AppState } from '../www/netstorm.js';

// 공개 메뉴 묶음만으로 지형을 읽는다. 원본/느슨한 사용자 맵 폴더는 등록하지 않는다.
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
const data = GameData.build({ files, language: 'english' });
const resources = buildMenuResources(files);
const scene = loadTerrainScene(data.files, 'savetheisland');

// 기존 독립 Python 기대값의 칸 수/받침 수를 읽는다. 이번 JS 결과로 기대값을 만들지 않는다.
function expected(name) {
    return new Map(fs.readFileSync(path.join(WEBPJ_ROOT, 'tests', 'fixtures', name), 'utf8').split(/\r?\n/)
        .filter((line) => line && !line.startsWith('#')).map((line) => { const row = line.split('\t'); return [row[0].toLowerCase(), row]; }));
}

// 원본 이름 표의 부분 일치 순서·숫자 오프셋·BYTE 저장을 확인한다.
test('zorder: 원본23이름·부분 문자열·대소문자·signed BYTE 깊이', () => {
    assert.equal(Z_ORDERS.length, 23);
    for (const [name, depth] of Z_ORDERS) assert.equal(parseZOrder(name), depth);
    assert.equal(parseZOrder('BRIDGE'), 0);
    assert.equal(parseZOrder('! zoSTALAG + 3'), 23);
    assert.equal(parseZOrder('zoISLAND - 257'), -257);
    assert.equal(typeDrawDepth({ properties: [{ name: 'zorder', value: 'zoISLAND - 257' }] }), -1);
    assert.equal(typeDrawDepth({ properties: [{ name: 'zorder', value: 250 }, { name: 'ZORDER', value: 128 }] }), -128);
    assert.equal(typeDrawDepth({ properties: [] }), 0);
    assert.equal(parseZOrder(-2147483648.5), -2147483648);
    assert.equal(parseZOrder(2147483648), 0);
    assert.throws(() => parseZOrder('zostalag'));
    assert.throws(() => parseZOrder('zoSTALAG + 2147483648'), RangeError);
});

// 저장 오브젝트에 게임 번호를 붙이지 않고 기존 지형 계산 결과만 출력에 연결한다.
test('공식32맵의 지면·절벽·받침 수가 기존 독립 기대값과 같다', () => {
    const masks = expected('terrain-expected.tsv');
    const supports = expected('island-supports-expected.tsv');
    const names = data.files.find('d/*.fort');
    assert.equal(names.length, 32);
    // 아카이브의 공식 맵 전체에서 독립 기대 개수와 실제 프레임 범위를 확인한다.
    for (const name of names) {
        const fort = parseFort(data.files.read(name), createFortTypes(scene.table));
        const built = buildTerrainScene(fort, scene.table);
        const key = `tarc:${name}`.toLowerCase();
        assert.equal(built.entries.filter((entry) => entry.kind === 'land').length, Number(masks.get(key)[2]), name);
        const count = Number(supports.get(key)[2]);
        assert.equal(built.entries.filter((entry) => entry.kind === 'support').length, count, name);
        assert.equal(built.entries.filter((entry) => entry.kind === 'stalag').length, count, name);
        assert.equal(built.entries.filter((entry) => entry.kind === 'fringe').length, built.fringe.count(), name);
        for (const entry of built.entries) {
            assert.equal(entry.ownerColor, 0);
            assert.equal(entry.depth, ['land', 'support'].includes(entry.kind) ? 0 : 20);
            assert.ok(entry.frame >= 0 && entry.frame < scene.table.view(entry.type).frameCount);
            if (entry.kind === 'support' || entry.kind === 'stalag') assert.equal(entry.frame, 8);
        }
    }
});

// 명시적인 색 입력에서만 색칠하며 절벽 생성 소유자0과 받침 프레임 선택을 유지한다.
test('영역/받침 색은 명시 입력만 사용하고 절벽은 중립이며 받침은 같은 anchor에 놓인다', () => {
    const fort = parseFort(data.files.read('d/savetheisland.fort'), createFortTypes(scene.table));
    const colored = buildTerrainScene(fort, scene.table, { colorOfTerritory: () => 3, colorOfSupport: () => 2 });
    assert.ok(colored.entries.some((entry) => entry.kind === 'land'));
    assert.ok(colored.entries.some((entry) => entry.kind === 'support'));
    // 지면3·받침2·절벽0의 색 번호를 구별한다.
    for (const entry of colored.entries) {
        assert.equal(entry.ownerColor, entry.kind === 'land' ? 3 : entry.kind === 'fringe' ? 0 : 2);
        if (entry.kind === 'support' || entry.kind === 'stalag') assert.equal(entry.frame, 1);
    }
    for (const support of colored.supports.supports) {
        const pair = colored.entries.filter((entry) => ['support', 'stalag'].includes(entry.kind) && entry.x === support.anchorX && entry.y === support.anchorY);
        assert.deepEqual(pair.map((entry) => entry.type), [94, 95]);
    }
    assert.throws(() => buildTerrainScene(fort, scene.table, { colorOfSupport: () => 9 }), RangeError);
});

// 패스/URL을 맵 이름으로 오인하지 않고 공개 자산에 없는 이름은 분명히 거부한다.
test('맵 조회: 대소문자·확장자·경로 거부와 누락 오류', () => {
    assert.equal(terrainMapName('SaveTheIsland.FORT'), 'SaveTheIsland');
    assert.equal(loadTerrainScene(data.files, 'SaveTheIsland.FORT').entries.length, scene.entries.length);
    for (const name of ['../originals', 'https://example.com', 'd/map', 'd\\map', '', 'x\0y']) assert.throws(() => terrainMapName(name));
    assert.throws(() => loadTerrainScene(data.files, 'map-does-not-exist'), /맵 파일/);
});

// 실제8비트 합성·dirty·드래그/키·화면 크기 변경과 취소를 점검한다.
test('정적 화면: 실제 출력·같은 상태 재현·카메라 이동·resize·끌기 취소', () => {
    const preview = new TerrainPreview(scene, resources);
    preview.resize(1024, 768);
    const surface = new IndexedSurface(1024, 768);
    preview.render(surface);
    const pixels = surface.pixels.slice();
    assert.equal(preview.dirty, false);
    assert.ok(new Set(pixels).size > 100);
    preview.render(surface);
    assert.deepEqual(surface.pixels, pixels);
    const before = preview.camera.pixels;
    assert.equal(preview.keyDown('ArrowRight'), true);
    assert.equal(preview.camera.pixels.x, before.x + 64);
    assert.equal(preview.keyDown('KeyX'), false);
    assert.equal(preview.pointerDown(0, 767), false);
    assert.equal(preview.pointerDown(500, 400), true);
    preview.pointerMove(540, 420);
    assert.deepEqual(preview.camera.pixels, { x: before.x + 24, y: before.y - 20 });
    preview.cancel();
    assert.equal(preview.pointerMove(700, 700), false);
    preview.resize(1366, 768);
    assert.equal(preview.width, 1366);
    assert.ok(preview.dirty);
    preview.moveTo(-10000, -10000);
    assert.deepEqual(preview.camera.pixels, { x: 0, y: 0 });
    preview.moveTo(1e10, 1e10);
    assert.deepEqual(preview.camera.pixels, { x: preview.bounds.right - 1, y: preview.bounds.bottom - 1 });
});

// 미션 Config를 건드리지 않으며 읽기 실패 시에도 현재 화면을 유지한다.
test('Netstorm 연결: 시작 관문·조회 실패 보존·크기·키와 메뉴 복귀·미션Config 불변', () => {
    const app = new Netstorm({ audio: false, language: 'english' });
    assert.equal(app.options.previewMap, null);
    assert.throws(() => app.previewMap('savetheisland'), /시작 입력/);
    // DOM/장치를 필요로 하지 않는 상태 연결만 설치한다. 실제 브라우저 검사는 별도로 한다.
    app._resources = resources;
    app._gameData = data;
    app._layout = { logicalWidth: 1024, logicalHeight: 768 };
    app._root = { ownerDocument: { title: 'NetStorm Main Menu' } };
    app._mainMenu = { cancel() {}, dirty: false };
    app._setState(AppState.MAIN_MENU);
    const beforeTell = data.currentTell;
    const preview = app.previewMap('savetheisland');
    assert.equal(app.state, AppState.TERRAIN_PREVIEW);
    assert.equal(preview.width, 1024);
    assert.equal(data.currentTell, beforeTell);
    assert.throws(() => app.previewMap('map-does-not-exist'), /맵 파일/);
    assert.equal(app._terrainPreview, preview);
    assert.equal(app.state, AppState.TERRAIN_PREVIEW);
    app._onKeyDown({ code: 'Escape', preventDefault() {} });
    assert.equal(app.state, AppState.MAIN_MENU);
    assert.equal(app._terrainPreview, null);
    assert.equal(app._root.ownerDocument.title, 'NetStorm Main Menu');
    assert.equal(app.closeTerrainPreview(), false);
});
