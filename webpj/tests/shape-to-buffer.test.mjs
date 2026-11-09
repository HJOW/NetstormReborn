// 실제 ShapeToBuffer 캡처(raw-shape-to-buffer.js)와 표시 그림 헤더/렌더러 어댑터(squid-renderer.js)를 원본 x86 fixture 로 대조한다(cpppj ShapeToBufferTests 의 10.78 범위).
//  - shape-to-buffer-x86.tsv originals 80행: 보호 Dude(TYPE 70) frame·fixed16 배율·TYPE 기준점 옵션·free/dead 상태 입력의 메타데이터와 전체 plain RLE 버퍼.
//  - support-capture-x86.tsv 162행: 섬 조각 두 TYPE(95 islandStalag·157 noIsland)의 기본/추가 그림(TYPE baseFrame)·소유자 0/1/8·TYPE 기준점(−2, 3) 보정.
//  - 각 행은 입력(raw 슬롯·상태)만 설치하고 모듈이 계산한다. 기대 열은 비교에만 쓴다. CD(10.72) 행은 범위 밖이고, Factory/ThunderCannon/SunAviary/Geyser 캡처 행은 shape-to-buffer-derived.test.mjs 가 대조한다(2026-11-09 WB04-E202).
//  - 거부·mapped·추가 그림 계약과 SquidRenderer 어댑터는 웹 계약 검사다. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E199: ShapeToBuffer 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { realCaptureAssets, clientPool, hexOrDash as hex } from './capture-assets-support.mjs';
import { TypeTable, TYPE_LOAD_ORDER } from '../www/assets/type-table.js';
import { DirtyRegions } from '../www/render/dirty-regions.js';
import { drawShape } from '../www/render/vfx-draw.js';
import { drawMappedShape } from '../www/render/vfx-mapped-draw.js';
import { SquidRenderer, squidRendererShapes } from '../www/render/squid-renderer.js';
import { RawShapeToBuffer, createShapeToBufferState } from '../www/render/raw-shape-to-buffer.js';

// 원본 x86 fixture 이다. (2026-11-09 WB04-E199)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const bufferRows = (readRows(path.join(FIXTURES, 'shape-to-buffer-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');
const supportRows = readRows(path.join(FIXTURES, 'support-capture-x86.tsv')) ?? [];

// 예외가 나는지 본다. (2026-11-09 WB04-E199)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실제 자료·client 50 풀·16진 표기는 capture-assets-support.mjs 와 공유한다(2026-11-09 WB04-E200 분리).
const realAssets = realCaptureAssets;

// 캡처 결과를 fixture 의 네 출력 칸과 같은 문자열로 만든다. (2026-11-09 WB04-E199)
const output = (image) => [`${image.width},${image.height},${image.hotspotX},${image.hotspotY}`, hex(image.pixels), String(image.pixels.length), '1'];

test('ShapeToBuffer: 원본 10.78 Dude 80행의 메타데이터와 전체 plain RLE 버퍼가 같다', { skip: bufferRows.length === 0 && 'shape-to-buffer-x86.tsv 없음' }, () => {
    const assets = realAssets();
    const types = assets.types.slice();
    types[70] = { ...types[70], hotspotX: -2, hotspotY: 3 };
    let count = 0;
    // 각 행의 frame·배율·기준점 옵션·상태·가상 표만 설치하고 실제 캡처를 비교한다.
    for (const row of bufferRows) {
        assert.equal(row.length, 10);
        const pool = clientPool();
        const raw = pool.allocated(50);
        raw.fill(0);
        put(raw, 0, Number(row[5]));
        raw[10] = 70;
        put(raw, 36, Number(row[1]));
        raw[11] = Number(row[4]);
        const state = createShapeToBufferState({ zoom: Number(row[2]) | 0, addTypeHotspot: Number(row[3]) !== 0 });
        const image = new RawShapeToBuffer(pool, types, assets.shapes, assets.database, state).capture(50);
        assert.deepEqual(output(image), row.slice(6), `행 ${count}`);
        count += 1;
    }
    assert.equal(count, 80);
});

test('ShapeToBuffer: 섬 조각 162행의 기본/추가 그림·소유자·TYPE 기준점 보정이 원문과 같다', { skip: supportRows.length === 0 && 'support-capture-x86.tsv 없음' }, () => {
    const assets = realAssets();
    assert.equal(TYPE_LOAD_ORDER[95 - 70], 'islandStalag');
    assert.equal(TYPE_LOAD_ORDER[157 - 70], 'noIsland');
    const types = assets.types.slice();
    types[95] = { ...types[95], hotspotX: -2, hotspotY: 3 };
    types[157] = { ...types[157], hotspotX: -2, hotspotY: 3 };
    const maps = new Uint32Array(188);
    let count = 0;
    // 기본/추가 Draw 가 공통인 두 정확한 가상 표만 구성한다.
    for (const row of supportRows) {
        const type = row[0] === 'islandStalag' ? 95 : 157;
        const pool = clientPool();
        const raw = pool.allocated(50);
        raw.fill(0);
        put(raw, 0, type === 95 ? 0x506e50 : 0x506f08);
        raw[10] = type;
        raw[34] = Number(row[3]);
        raw[40] = Number(row[2]);
        put(raw, 36, Number(row[1]));
        let additional = 0;
        const state = createShapeToBufferState({
            addTypeHotspot: true,
            ownerTypeMaps: maps,
            // 실제 4acfe0 의 TYPE 기본 프레임이다.
            mainFrame: () => assets.table.definitions[type - 70].baseFrame,
            // 실제 41c100 은 인자를 읽지 않는 ret 8 이다.
            additional: () => { additional += 1; },
        });
        const image = new RawShapeToBuffer(pool, types, assets.shapes, assets.database, state).capture(50);
        assert.equal(row[4], row[6]);
        assert.equal(row[5], row[7]);
        assert.deepEqual(output(image).slice(0, 2), [row[4], row[5]], `${row[0]} 행 ${count}`);
        assert.equal(additional, (Number(row[2]) & 2) ? 1 : 0);
        count += 1;
    }
    assert.equal(count, 162);
});

test('ShapeToBuffer: 무효 객체·해제 표식·미연결 소비자를 그리기 전에 거부한다(웹 계약 검사)', () => {
    const assets = realAssets();
    const pool = clientPool();
    const raw = pool.allocated(50);
    // 정상 입력을 다시 설치한다.
    const reset = () => {
        raw.fill(0);
        put(raw, 0, 0x508cc8);
        raw[10] = 70;
        put(raw, 36, 1);
    };
    const capture = (types = assets.types, state = createShapeToBufferState()) => new RawShapeToBuffer(pool, types, assets.shapes, assets.database, state).capture(50);
    reset();
    assert.equal(capture().pixels.length > 0, true);
    // 해제 상태·가상 표 0/해제 표식·frame 범위·자산 타입 범위.
    for (const change of [() => { raw[11] = 1; }, () => put(raw, 0, 0), () => put(raw, 0, 0xdddddddd), () => put(raw, 0, 0xcdcdcdcd), () => put(raw, 36, -1), () => put(raw, 36, assets.shapes[70].frameCount), () => { raw[10] = 69; }]) {
        reset();
        change();
        assert.ok(throws(() => capture()));
    }
    // 타입 maxHP 의 두 진단 표식.
    for (const marker of [0xdddddddd | 0, 0xcdcdcdcd | 0]) {
        reset();
        const types = assets.types.slice();
        types[70] = { ...types[70], maxHitPoints: marker };
        assert.ok(throws(() => capture(types)));
    }
    // 추가 그림인데 파생 소비자가 없으면 거부한다.
    reset();
    raw[40] = 2;
    assert.ok(throws(() => capture()));
    assert.ok(throws(() => capture(assets.types, createShapeToBufferState({ mainFrame: () => 0 }))));
    // 소유자가 있으면 현재 색표 토큰이 필요하고, 토큰이 있으면 실제 색 소비자와 256바이트 표가 필요하다.
    reset();
    raw[34] = 3;
    assert.ok(throws(() => capture()));
    const maps = new Uint32Array(188);
    maps[70] = 1;
    assert.ok(throws(() => capture(assets.types, createShapeToBufferState({ ownerTypeMaps: maps }))));
    assert.ok(throws(() => capture(assets.types, createShapeToBufferState({ ownerTypeMaps: maps, ownerColors: () => new Uint8Array(255) }))));
    // TYPE 기준점 보정은 레코드 기준점이 있어야 한다.
    reset();
    const missing = assets.types.slice();
    missing[70] = { ...missing[70], hotspotX: undefined };
    assert.ok(throws(() => capture(missing, createShapeToBufferState({ addTypeHotspot: true }))));
    // 판본 크기가 다른 표는 거부한다.
    assert.ok(throws(() => new RawShapeToBuffer(pool, assets.types.slice(0, 171), assets.shapes, assets.database, createShapeToBufferState())));
});

test('ShapeToBuffer: mapped 는 현재 frame 을 치환하고, 추가 그림은 기본 그림 뒤·TYPE 기준점 보정 전에 호출된다(웹 계약 검사)', () => {
    const assets = realAssets();
    const pool = clientPool();
    const raw = pool.allocated(50);
    raw.fill(0);
    put(raw, 0, 0x508cc8);
    raw[10] = 70;
    put(raw, 36, 7);
    raw[34] = 5;
    raw[40] = 2;
    const types = assets.types.slice();
    types[70] = { ...types[70], hotspotX: 4, hotspotY: -6 };
    const maps = new Uint32Array(188);
    maps[70] = 0x1234;
    const colors = Uint8Array.from({ length: 256 }, (_, index) => (index * 7 + 3) & 0xff);
    const calls = [];
    const state = createShapeToBufferState({
        addTypeHotspot: true,
        ownerTypeMaps: maps,
        ownerColors: (type, owner) => { calls.push(`colors ${type} ${owner}`); return colors; },
        mainFrame: (sid) => { calls.push(`main ${sid}`); return 2; },
        additional: (sid, image, x, y) => { calls.push(`additional ${sid} ${x} ${y} ${image.hotspotX} ${image.hotspotY}`); },
    });
    const image = new RawShapeToBuffer(pool, types, assets.shapes, assets.database, state).capture(50);
    const header = assets.shapes[70].frames[7];
    assert.deepEqual(calls, ['colors 70 5', 'main 50', `additional 50 ${header.hotspotX} ${header.hotspotY} ${header.hotspotX} ${header.hotspotY}`]);
    assert.deepEqual([image.width, image.height, image.hotspotX, image.hotspotY], [header.width, header.height, header.hotspotX + 4, header.hotspotY - 6]);
    // 원본 비대칭: 가상 main(2) 이 아니라 현재 frame(7) 을 mapped 로 그린다.
    const expected = new Uint8Array(header.width * header.height).fill(255);
    drawMappedShape(expected, header.width, header.height, assets.database, 0, 7, header.hotspotX, header.hotspotY, { left: 0, top: 0, right: header.width - 1, bottom: header.height - 1 }, colors);
    assert.deepEqual(image.pixels, expected);
    // 소유자 0(plain)이면 가상 main 을 그리고, -1 이면 기본 그림을 생략한다.
    raw[34] = 0;
    const plain = new RawShapeToBuffer(pool, types, assets.shapes, assets.database, state).capture(50);
    const main = new Uint8Array(header.width * header.height).fill(255);
    drawShape(main, header.width, header.height, assets.database, 0, 2, header.hotspotX, header.hotspotY, { left: 0, top: 0, right: header.width - 1, bottom: header.height - 1 });
    assert.deepEqual(plain.pixels, main);
    state.mainFrame = () => -1;
    const skipped = new RawShapeToBuffer(pool, types, assets.shapes, assets.database, state).capture(50);
    assert.ok(skipped.pixels.every((value) => value === 255));
    // 캡처는 같은 상태 객체의 현재 배율을 매번 읽는다.
    state.zoom = 32768;
    const half = new RawShapeToBuffer(pool, types, assets.shapes, assets.database, state).capture(50);
    assert.deepEqual([half.width, half.height], [header.width >> 1, header.height >> 1]);
});

test('SquidRenderer: 실제 로딩 순서의 그림 헤더 표와 변경 표 어댑터(웹 계약 검사)', () => {
    const assets = realAssets();
    const { shapes, table, database } = assets;
    assert.equal(shapes.length, 188);
    // 내장 타입과 여분 칸은 빈 칸이다.
    for (const number of [0, 10, 69, 186, 187]) assert.deepEqual(shapes[number], { frameCount: 0, loaded: false, frames: [] });
    // 자산 타입은 클러스터 수와 물리 프레임 전체·포구 WORD 를 담는다.
    for (let number = 70; number < 186; number += 1) {
        const shape = shapes[number];
        const block = table.view(number).block;
        assert.equal(shape.loaded, true);
        assert.equal(shape.frameCount, table.definitions[number - 70].clusters.length);
        assert.equal(shape.frames.length, database.blocks[block].frames.length);
        assert.ok(shape.frames.length >= shape.frameCount);
        const header = database.squidHeader(block, 0);
        const view = new DataView(header.buffer, header.byteOffset, header.byteLength);
        assert.deepEqual([shape.frames[0].muzzleX, shape.frames[0].muzzleY, shape.frames[0].width], [view.getInt16(32, true), view.getInt16(34, true), view.getInt16(24, true)]);
    }
    // 읽지 못한 정의는 빈 그림으로 대체하지 않는다.
    const broken = new TypeTable(table.definitions.map((definition, index) => (index === 3 ? null : definition)));
    assert.ok(throws(() => squidRendererShapes(broken, database)));
    // 어댑터는 사각형을 다시 자르지 않고 전달하며, 전체 갱신 대기 여부를 그대로 읽는다.
    const regions = new DirtyRegions(640, 480);
    const renderer = new SquidRenderer({ fullRedrawPending: () => regions.fullRedraw, invalidate: (rect, flags) => regions.add(rect, flags) });
    assert.equal(renderer.suppressed(), false);
    renderer.invalidate({ left: -20, top: 5, right: 700, bottom: 30 }, 4);
    assert.deepEqual(regions.entries.map((entry) => [entry.rect.left, entry.rect.top, entry.rect.right, entry.rect.bottom, entry.flags]), [[-20, 5, 700, 30, 4]]);
    regions.invalidateAll();
    assert.equal(renderer.suppressed(), true);
    assert.ok(throws(() => new SquidRenderer({ invalidate() {} })));
});
