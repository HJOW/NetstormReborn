// 파생 그림 4종(Factory·썬더 캐논·훨리베이스·간헐천)의 기본/추가 가상 그림을 실제 ShapeToBuffer 위에서 원본 x86 fixture 로 대조한다(cpppj ShapeToBufferTests 의 파생 캡처 TEST_CASE, 10.78).
//  - factory-capture-x86.tsv 200행: Factory/아웃포스트 두 가상 표·Level 0..7·extra 2/3/10·frame·TYPE 기준점 옵션·배율 32768/65536/98304 의 메타데이터와 전체 plain RLE 버퍼(추가 그림 60회).
//  - thunder-cannon-capture-x86.tsv 333행: TYPE 131 의 37프레임 × extra 0/2/3 × 소유자 0/1/8, TYPE 기준점(−2, 3).
//  - sun-aviary-capture-x86.tsv 384행: TYPE 72 의 32프레임 생산/추가/매몰/그림자 경계 × extra × 소유자.
//  - geyser-capture-x86.tsv 459행: TYPE 122 의 51프레임 × extra 0/2/3 × 소유자 0/1/8, 고정 본체(TYPE baseFrame)와 현재 그림.
//  - 각 행은 입력(raw 슬롯·상태)만 설치하고 모듈이 계산한다. 기대 열은 비교에만 쓴다. 거부·생략 경로는 cpppj 손검사를 1:1 로 옮겼고 Factory 처리기 계약은 웹 계약 검사다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E202: 파생 그림·캡처 4종 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { realCaptureAssets, clientPool, hexOrDash as hex } from './capture-assets-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { TYPE_LOAD_ORDER } from '../www/assets/type-table.js';
import { SidPool } from '../www/core/sid-pool.js';
import { drawShape } from '../www/render/vfx-draw.js';
import { RawShapeToBuffer, createShapeToBufferState } from '../www/render/raw-shape-to-buffer.js';
import { RawFactoryDraw } from '../www/core/raw-factory-draw.js';
import { isThunderCannonCapture, thunderCannonCaptureMainFrame, drawThunderCannonCapture } from '../www/core/raw-thunder-cannon-capture.js';
import { isSunAviaryCapture, sunAviaryCaptureMainFrame, drawSunAviaryCapture } from '../www/core/raw-sun-aviary-capture.js';
import { isGeyserCapture, drawGeyserCapture } from '../www/core/raw-geyser-capture.js';

// 원본 x86 fixture 이다(10.78 단일 판본이라 판본 열이 없다). (2026-11-09 WB04-E202)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const factoryRows = readRows(path.join(FIXTURES, 'factory-capture-x86.tsv')) ?? [];
const thunderRows = readRows(path.join(FIXTURES, 'thunder-cannon-capture-x86.tsv')) ?? [];
const aviaryRows = readRows(path.join(FIXTURES, 'sun-aviary-capture-x86.tsv')) ?? [];
const geyserRows = readRows(path.join(FIXTURES, 'geyser-capture-x86.tsv')) ?? [];

// 예외가 지정 종류로 나는지 본다(종류를 주지 않으면 아무 예외). (2026-11-09 WB04-E202)
function throws(action, kind = Error) {
    try { action(); } catch (error) { return error instanceof kind; }
    return false;
}

// 캡처 결과의 메타데이터 칸 문자열이다(fixture 의 "폭,높이,기준점X,기준점Y"). (2026-11-09 WB04-E202)
const metadataOf = (image) => `${image.width},${image.height},${image.hotspotX},${image.hotspotY}`;

// 같은 캡처 버퍼 전체를 pane 으로 보호 SHP 의 (type, frame) 을 그리는 cpppj DrawShape 소비자이다. (2026-11-09 WB04-E202)
function drawInto(assets, image, type, frame, x, y) {
    drawShape(image.pixels, image.width, image.height, assets.database, type - 70, frame, x, y, { left: 0, top: 0, right: image.width - 1, bottom: image.height - 1 });
}

// 기준점을 원문 입력(−2, 3)으로 덮어쓴 TYPE 레코드 사본이다. (2026-11-09 WB04-E202)
function typesWithHotspot(assets, type) {
    const types = assets.types.slice();
    types[type] = { ...types[type], hotspotX: -2, hotspotY: 3 };
    return types;
}

// 188칸 FrameCodeTable 표이다(자산 타입은 실제 로딩 순서 정의, 나머지는 빈 표). (2026-11-09 WB04-E202)
function frameTables(assets) {
    // 각 TYPE 번호의 정의를 프레임 코드 표로 바꾼다.
    return Array.from({ length: 188 }, (_, number) => {
        const definition = number >= 70 && number < 70 + TYPE_LOAD_ORDER.length ? assets.table.definitions[number - 70] : null;
        return definition ? FrameCodeTable.fromDefinition(definition) : new FrameCodeTable([]);
    });
}

test('ShapeToBuffer: Factory 200행의 기본·추가 그림과 전체 픽셀이 원본과 같다', { skip: factoryRows.length === 0 && 'factory-capture-x86.tsv 없음' }, () => {
    const assets = realCaptureAssets();
    const types = typesWithHotspot(assets, 85);
    const frames = frameTables(assets);
    const maps = new Uint32Array(188);
    let count = 0;
    let overlays = 0;
    // 동결된 원문 입력으로만 준비하고 전체 기대 픽셀은 비교에만 쓴다(머리 행 제외).
    for (const row of factoryRows) {
        if (row[0] === 'vtable') continue;
        assert.equal(row.length, 10);
        const pool = clientPool();
        const raw = pool.allocated(50);
        raw.fill(0);
        put(raw, 0, Number(row[0]));
        raw[10] = 85;
        raw[11] = (Number(row[1]) << 5) & 0xff;
        raw[34] = 1;
        raw[40] = Number(row[2]);
        put(raw, 36, Number(row[3]));
        const factory = new RawFactoryDraw(pool, frames);
        const state = createShapeToBufferState({ zoom: Number(row[5]) | 0, addTypeHotspot: Number(row[4]) !== 0, ownerTypeMaps: maps });
        state.mainFrame = (sid) => factory.mainFrame(sid);
        state.additional = (sid, image, x, y) => {
            factory.additional(sid, x, y, (px, py, type, frame) => {
                assert.equal(state.zoom, 65536);
                assert.equal(type, 85);
                overlays += 1;
                drawInto(assets, image, type, frame, px, py);
            });
        };
        const image = new RawShapeToBuffer(pool, types, assets.shapes, assets.database, state).capture(50);
        const actual = [metadataOf(image), hex(image.pixels), String(image.pixels.length), '1'];
        assert.deepEqual(actual, row.slice(6), `Factory 행 ${count} VT ${row[0]} Level ${row[1]} extra ${row[2]} frame ${row[3]}`);
        count += 1;
    }
    assert.equal(count, 200);
    assert.equal(overlays, 60);
});

test('ShapeToBuffer: 썬더 캐논 37프레임 333행의 기본·추가 그림과 전체 픽셀이 원본과 같다', { skip: thunderRows.length === 0 && 'thunder-cannon-capture-x86.tsv 없음' }, () => {
    const assets = realCaptureAssets();
    const types = typesWithHotspot(assets, 131);
    const frames = FrameCodeTable.fromDefinition(assets.table.definitions[131 - 70]);
    const maps = new Uint32Array(188);
    let count = 0;
    // 37물리프레임·extra 0/2/3·owner 0/1/8 의 원본 결과만 기대값으로 읽는다.
    for (const row of thunderRows) {
        assert.equal(row.length, 7);
        const pool = clientPool();
        const raw = pool.allocated(50);
        raw.fill(0);
        put(raw, 0, 0x50bad0);
        raw[10] = 131;
        put(raw, 12, 0xa5ef, 2);
        raw[34] = Number(row[2]);
        raw[40] = Number(row[1]);
        put(raw, 36, Number(row[0]));
        const state = createShapeToBufferState({
            addTypeHotspot: true,
            ownerTypeMaps: maps,
            mainFrame: (sid) => thunderCannonCaptureMainFrame(pool.slot(sid), frames),
            additional: (sid, image, x, y) => drawThunderCannonCapture(pool.slot(sid), (type, frame, px, py) => drawInto(assets, image, type, frame, px, py), x, y),
        });
        const image = new RawShapeToBuffer(pool, types, assets.shapes, assets.database, state).capture(50);
        assert.equal(row[3], row[5]);
        assert.equal(row[4], row[6]);
        assert.deepEqual([metadataOf(image), hex(image.pixels)], [row[3], row[4]], `썬더 캐논 행 ${count} frame ${row[0]} extra ${row[1]} owner ${row[2]}`);
        count += 1;
    }
    assert.equal(count, 333);
});

test('ThunderCannonCapture: 다른 타입/가상 표와 범위 밖 프레임을 그리기 전에 거부한다', () => {
    const assets = realCaptureAssets();
    const frames = FrameCodeTable.fromDefinition(assets.table.definitions[131 - 70]);
    const raw = new Uint8Array(50);
    put(raw, 0, 0x50bad0);
    raw[10] = 131;
    assert.equal(isThunderCannonCapture(raw), true);
    put(raw, 0, 0x50b9b8);
    assert.equal(isThunderCannonCapture(raw), false);
    assert.ok(throws(() => thunderCannonCaptureMainFrame(raw, frames), TypeError));
    put(raw, 0, 0x50bad0);
    put(raw, 36, 37);
    assert.ok(throws(() => thunderCannonCaptureMainFrame(raw, frames), RangeError));
});

test('ShapeToBuffer: 훨리베이스 32프레임 384행의 생산/추가/매몰/그림자 경계와 전체 픽셀이 원본과 같다', { skip: aviaryRows.length === 0 && 'sun-aviary-capture-x86.tsv 없음' }, () => {
    const assets = realCaptureAssets();
    const types = typesWithHotspot(assets, 72);
    const maps = new Uint32Array(188);
    let count = 0;
    // 기대값은 제품 실행으로 만들지 않고 먼저 동결한 원본 결과만 읽는다.
    for (const row of aviaryRows) {
        assert.equal(row.length, 7);
        const pool = clientPool();
        const raw = pool.allocated(50);
        raw.fill(0);
        put(raw, 0, 0x509930);
        raw[10] = 72;
        put(raw, 12, 0xa5ef, 2);
        raw[34] = Number(row[2]);
        raw[40] = Number(row[1]);
        put(raw, 36, Number(row[0]));
        const state = createShapeToBufferState({
            addTypeHotspot: true,
            ownerTypeMaps: maps,
            mainFrame: (sid) => sunAviaryCaptureMainFrame(pool.slot(sid)),
            additional: (sid, image, x, y) => drawSunAviaryCapture(pool.slot(sid), (type, frame, px, py) => drawInto(assets, image, type, frame, px, py), x, y),
        });
        const image = new RawShapeToBuffer(pool, types, assets.shapes, assets.database, state).capture(50);
        assert.equal(row[3], row[5]);
        assert.equal(row[4], row[6]);
        assert.deepEqual([metadataOf(image), hex(image.pixels)], [row[3], row[4]], `훨리베이스 행 ${count} frame ${row[0]} extra ${row[1]} owner ${row[2]}`);
        count += 1;
    }
    assert.equal(count, 384);
});

test('SunAviaryCapture: 정확한 가상 표만 받고 원본 생략 경로는 그림 소비자를 요구하지 않는다', () => {
    const raw = new Uint8Array(50);
    put(raw, 0, 0x509930);
    raw[10] = 72;
    put(raw, 36, 27);
    assert.equal(isSunAviaryCapture(raw), true);
    assert.equal(sunAviaryCaptureMainFrame(raw), -1);
    let draws = 0;
    drawSunAviaryCapture(raw, (type, frame, x, y) => {
        assert.deepEqual([type, frame, x, y], [72, 27, 11, 13]);
        draws += 1;
    }, 11, 13);
    assert.equal(draws, 1);
    raw[40] = 3;
    assert.equal(sunAviaryCaptureMainFrame(raw), 27);
    drawSunAviaryCapture(raw, null, 11, 13);
    raw[40] = 10;
    assert.equal(sunAviaryCaptureMainFrame(raw), 27);
    drawSunAviaryCapture(raw, null, 11, 13);
    put(raw, 0, 0x5099e8);
    assert.equal(isSunAviaryCapture(raw), false);
    assert.ok(throws(() => sunAviaryCaptureMainFrame(raw), TypeError));
});

test('ShapeToBuffer: 간헐천 51프레임 459행의 고정 본체·현재 그림·메타데이터와 전체 픽셀이 원본과 같다', { skip: geyserRows.length === 0 && 'geyser-capture-x86.tsv 없음' }, () => {
    const assets = realCaptureAssets();
    const types = typesWithHotspot(assets, 122);
    // 원본 격리 장면의 client 50 슬롯을 한 번 확보해 현재 입력만 매 행 다시 쓴다.
    const pool = clientPool();
    const maps = new Uint32Array(188);
    const state = createShapeToBufferState({
        addTypeHotspot: true,
        ownerTypeMaps: maps,
        // 실제 4acfe0 의 TYPE+128 고정 본체를 공급한다.
        mainFrame: (sid) => {
            assert.equal(isGeyserCapture(pool.slot(sid)), true);
            return assets.table.definitions[122 - 70].baseFrame;
        },
        additional: (sid, image, x, y) => drawGeyserCapture(pool.slot(sid), (type, frame, px, py) => drawInto(assets, image, type, frame, px, py), x, y),
    });
    const capture = new RawShapeToBuffer(pool, types, assets.shapes, assets.database, state);
    let count = 0;
    // 51프레임 × extra 0/2/3 × owner 0/1/8 의 모든 독립 입력을 방문한다.
    for (const row of geyserRows) {
        assert.equal(row.length, 7);
        const raw = pool.allocated(50);
        raw.fill(0);
        put(raw, 0, 0x50da88);
        raw[10] = 122;
        put(raw, 36, Number(row[0]));
        raw[40] = Number(row[1]);
        raw[34] = Number(row[2]);
        const image = capture.capture(50);
        const actual = [metadataOf(image), hex(image.pixels)];
        assert.deepEqual([...actual, ...actual], row.slice(3), `간헐천 행 ${count} frame ${row[0]} extra ${row[1]} owner ${row[2]}`);
        count += 1;
    }
    assert.equal(count, 459);
});

test('GeyserCapture: 정확한 타입/가상 표와 실제 출력 소비자를 요구하고 현재 프레임/좌표를 바꾸지 않는다', () => {
    const raw = new Uint8Array(50);
    put(raw, 0, 0x50da88);
    raw[10] = 122;
    put(raw, 36, 7);
    assert.equal(isGeyserCapture(raw), true);
    let draws = 0;
    drawGeyserCapture(raw, (type, frame, x, y) => {
        assert.deepEqual([type, frame, x, y], [122, 7, -5, 9]);
        draws += 1;
    }, -5, 9);
    assert.equal(draws, 1);
    assert.ok(throws(() => drawGeyserCapture(raw, null, 0, 0), TypeError));
    put(raw, 0, 0x5196f0);
    assert.equal(isGeyserCapture(raw), false);
    assert.ok(throws(() => drawGeyserCapture(raw, null, 0, 0), TypeError));
});

test('RawFactoryDraw: 판본/표 크기·가상 표를 거부하고 생략·같은 프레임 경로는 소비자를 요구하지 않는다(웹 계약 검사)', () => {
    const assets = realCaptureAssets();
    const frames = frameTables(assets);
    assert.ok(throws(() => new RawFactoryDraw(new SidPool(32768), frames.slice(0, 171)), TypeError));
    assert.ok(throws(() => new RawFactoryDraw({ edition: 'cd1072' }, frames), TypeError));
    const pool = clientPool();
    const raw = pool.allocated(50);
    raw.fill(0);
    put(raw, 0, 0x509148);
    raw[10] = 85;
    const factory = new RawFactoryDraw(pool, frames);
    assert.equal(factory.pool, pool);
    assert.equal(factory.handles(50), true);
    // Level 0..7 은 각 글자 'A'..'H' 의 첫 번호다.
    for (let level = 0; level < 8; level += 1) {
        raw[11] = level << 5;
        assert.equal(factory.mainFrame(50), frames[85].run(0x41 + level).first);
    }
    // 현재 프레임이 기본 프레임과 같거나 extra&9·dead 이면 소비자 없이 생략한다.
    raw[11] = 0;
    const base = factory.mainFrame(50);
    put(raw, 36, base);
    factory.additional(50, 1, 2, null);
    put(raw, 36, base + 1);
    for (const change of [() => { raw[40] = 1; }, () => { raw[40] = 8; }, () => { raw[11] = 2; }]) {
        raw[40] = 0;
        raw[11] = 0;
        change();
        factory.additional(50, 1, 2, null);
    }
    // 다른 프레임이면 실제 소비자가 필요하고, 현재 TYPE/프레임과 좌표를 그대로 전달한다.
    raw[40] = 0;
    raw[11] = 0;
    assert.ok(throws(() => factory.additional(50, 1, 2, null)));
    const calls = [];
    factory.additional(50, 1, 2, (x, y, type, frame) => calls.push([x, y, type, frame]));
    assert.deepEqual(calls, [[1, 2, 85, base + 1]]);
    // 아웃포스트 표도 같은 몸체이며, 미복원 가상 표와 표 밖 TYPE 은 실제 대상을 남기고 거부한다.
    put(raw, 0, 0x509210);
    assert.equal(factory.handles(50), true);
    put(raw, 0, 0x509148 + 8);
    assert.equal(factory.handles(50), false);
    assert.ok(throws(() => factory.mainFrame(50)));
    assert.ok(throws(() => factory.additional(50, 0, 0, () => {})));
    put(raw, 0, 0x509148);
    raw[10] = 188;
    assert.ok(throws(() => factory.mainFrame(50)));
});
