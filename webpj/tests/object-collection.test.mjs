// 일반 자산/그림자 수집기 render/object-collection.js 를 원본 10.78 관측(objectcollection-x86.tsv originals 477행)과 cpppj `ObjectCollectionTests.cpp` 의 strict 한도/계약/실제 수집→정렬→분배→픽셀 시험으로 검사한다.
//  - 관측 한 행 = 완성 입력 한 건(가시 사각형 float 네 개·한도·초기 해시 선택, 네 단계 해시 칸, 완성 raw 슬롯 50바이트, 타입 flags1/flags2/최대 HP/논리 프레임 수와 물리 프레임의 상자(셀 폭·높이·월드 기준점))에서 반환 수·작업 칸 64개 전체 바이트·
//    작업 배열 앞뒤 0xa5 보호 항목이다. 정상 반환 때 해시 선택은 2단계이고 해시 네 단계와 풀은 수집 전후 같아야 하며 모든 항목 토큰은 `object()` 가 같은 풀 SID 로 되돌린다. original1037·originalCD 행(각 477)은 읽지 않는다.
//  - cpppj 시험 이식: StrictLimitsAndCapacity(기록 후 strict 한도·기록 전 실제 span 검사·실패 시 선택/기록 보존)·ContractsAndEditionFrames(패치 부분: frameCheck·쓰레기 HP 표식·로드되지 않은 그림·순환 체인·0/표 밖 타입·토큰 거부·생성 검증).
//    `ObjectCollection_CommonSubmissionIntegration`(UI·지면과의 공통 제출)은 GumpCollection·TileGroupCollection 단위에서, `ShpWorldHotspots` 는 웹 `ShapeDatabase.squidMetrics` 의 기존 시험(E199)에서 확인한다.
//    `SpriteDraw_ObjectCollectionPixels`(실제 수집→CRT 정렬→분배→SHP 픽셀)는 E388 에서 미룬 것을 여기서 잇는다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E389)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { collectDrawEntries, dispatchDrawEntries, sortRenderEntries } from '../www/render/draw-submission.js';
import { ObjectCollection, bindObjectCollection } from '../www/render/object-collection.js';
import { GumpCollection, bindGumpCollection } from '../www/render/gump-collection.js';
import { TileGroupCollection, bindTileGroupCollection } from '../www/render/tile-group-collection.js';
import { SpriteDraw, SpriteDrawState, SpritePixelTarget, bindSpriteDraw } from '../www/render/sprite-draw.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E389)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'objectcollection-x86.tsv'));
// 독립 원본의 풀 기준 토큰과 작업 배열 폭이다. (2026-11-13 WB04-E389)
const OBJECT_POOL = 0x15030000;
const CAPACITY = 64;
const ENTRY = 16;
const hex = (bytes) => Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength).toString('hex');
const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex'));
const throwsOf = (action) => { try { action(); } catch { return true; } return false; };

// 한 관측 행을 재생해 불일치 설명 목록을 돌려준다. (2026-11-13 WB04-E389)
function replay(pool, hash, row) {
    const config = row[2].split(',');
    hash.reset();
    hash.select(Number(config[5]));
    if (row[3] !== '-') {
        // 0단계 입력도 보존해 일반 수집기가 방문하지 않는지 확인한다.
        for (const cell of row[3].split(';')) { const [level, x, y, value] = cell.split(',').map(Number); hash.setCell(level, x, y, value); }
    }
    if (row[4] !== '-') {
        // next/type/state/extra 및 미사용 바이트까지 실제 풀 슬롯에 공급한다.
        for (const slot of row[4].split(';')) {
            const [sid, bytes] = slot.split('=');
            const raw = unhex(bytes);
            assert.equal(raw.length, pool.layout.stride);
            pool.physical(Number(sid)).set(raw);
        }
    }
    const types = Array.from({ length: 5 }, () => ({ flags1: 0, flags2: 0, maxHitPoints: 0 }));
    const shapes = Array.from({ length: 5 }, () => ({ loaded: false, frameCount: 0, frames: [] }));
    // 타입 플래그와 물리 SHP 경계를 공급 표로만 옮긴다.
    for (const input of row[5].split(';')) {
        const [numberText, flagsText, geometryText] = input.split(':');
        const flags = flagsText.split(',');
        const number = Number(numberText);
        Object.assign(types[number], { flags1: Number(flags[0]) >>> 0, flags2: Number(flags[1]) >>> 0, maxHitPoints: Number(flags[2]) | 0 });
        const shape = shapes[number];
        shape.loaded = true;
        shape.frameCount = Number(flags[3]);
        const geometry = unhex(geometryText);
        const view = new DataView(geometry.buffer, geometry.byteOffset, geometry.byteLength);
        // 각 16바이트 입력은 셀 폭/높이와 월드 기준점 둘이다.
        for (let offset = 0; offset < geometry.length; offset += 16) {
            shape.frames.push({ cellWidth: view.getFloat32(offset, true), cellHeight: view.getFloat32(offset + 4, true), cellHotspotX: view.getFloat32(offset + 8, true), cellHotspotY: view.getFloat32(offset + 12, true) });
        }
    }
    const whole = new Uint8Array(pool.physical(2).buffer);
    const before = Uint8Array.from(whole);
    const grids = [0, 1, 2, 3].map((level) => hash.entries(level));
    const collector = new ObjectCollection(hash, pool, types, shapes, OBJECT_POOL);
    const guarded = new Uint8Array((CAPACITY + 2) * ENTRY).fill(0xa5);
    const output = guarded.subarray(ENTRY, ENTRY + CAPACITY * ENTRY);
    const view = { left: bitsToFloat(Number(config[0])), top: bitsToFloat(Number(config[1])), right: bitsToFloat(Number(config[2])), bottom: bitsToFloat(Number(config[3])) };
    const count = collector.collect(view, output, Number(config[4]) | 0);
    const issues = [];
    const check = (name, actual, expected) => { if (actual !== expected) issues.push(`${name}: ${actual} != ${expected}`); };
    check('count', count, Number(row[6]));
    check('output', hex(output), row[7]);
    check('guards', hex(guarded.subarray(0, ENTRY)) + hex(guarded.subarray((CAPACITY + 1) * ENTRY)), row[10]);
    check('selected', hash.selected, 2);
    check('pool', Buffer.compare(before, whole), 0);
    for (let level = 0; level < 4; level += 1) check(`grid${level}`, Buffer.compare(Buffer.from(grids[level].buffer), Buffer.from(hash.entries(level).buffer)), 0);
    // 사용 항목의 포인터 대리 토큰은 모두 같은 raw 풀 번호로 돌아간다.
    for (let index = 0; index < count; index += 1) {
        const token = new DataView(output.buffer, output.byteOffset + index * ENTRY, ENTRY).getUint32(8, true);
        check('object', collector.object(token), (token - OBJECT_POOL) / pool.layout.stride);
    }
    return issues;
}

test('일반 자산 수집기: 원본 10.78 관측 477행의 본체/그림자·정밀도·NaN·작업 칸 전체 바이트·보호 항목이 모두 일치하고 해시/풀은 보존된다', { skip: rows === null }, () => {
    const originals = rows.filter((row) => row[0] === 'originals');
    assert.equal(originals.length, 477);
    const pool = new SidPool(24000, { server: false });
    const hash = new SquidHash();
    for (const row of originals) assert.deepEqual(replay(pool, hash, row), [], `ObjectCollection ${row[1]}`);
});

// 계약과 공통 제출 검사용 최소 타입/프레임 저장소이다(cpppj ObjectScene): 논리 2/물리 4 프레임, 같은 유효 월드 경계. (2026-11-13 WB04-E389)
function scene() {
    const pool = new SidPool(24000, { server: false });
    const hash = new SquidHash();
    const types = [{ flags1: 0, flags2: 0, maxHitPoints: 0 }, { flags1: 0, flags2: 0, maxHitPoints: 100 }];
    const shapes = [{ loaded: false, frameCount: 0, frames: [] }, { loaded: true, frameCount: 2, frames: Array.from({ length: 4 }, () => ({ cellWidth: 2, cellHeight: 3, cellHotspotX: 0.25, cellHotspotY: 0.5 })) }];
    // next/type/frame/깊이를 포함한 슬롯을 매번 깨끗한 입력으로 채운다.
    const place = (sid, x, y, depth) => {
        const raw = pool.physical(sid);
        raw.fill(0);
        raw[10] = 1;
        raw[11] = 255;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        raw[35] = depth & 0xff;
    };
    place(5, 20, 25, 10);
    hash.setCell(3, 1, 1, 5);
    return { pool, hash, types, shapes, place };
}

test('일반 자산 수집기: strict 한도는 기록 후, 실제 버퍼 크기는 기록 전에 검사하고 실패 시 현재 해시 선택/기록을 보존한다(ObjectCollection_StrictLimitsAndCapacity)', () => {
    const w = scene();
    const collector = new ObjectCollection(w.hash, w.pool, w.types, w.shapes);
    const view = { left: 10, top: 20, right: 30, bottom: 40 };
    const work = new Uint8Array(ENTRY * 4);
    assert.ok(throwsOf(() => collector.collect(view, work, 1)));
    assert.equal(w.hash.selected, 3);
    assert.equal(new DataView(work.buffer).getUint32(8, true), 250);
    assert.ok(throwsOf(() => collector.collect(view, new Uint8Array(0), 10)));
    // 그림자 비트가 있으면 본체와 그림자 두 항목이며 한도 2 는 둘째를 쓴 뒤 거부한다(그림자 tail 4).
    w.types[1].flags1 = 0x40000;
    work.fill(0);
    assert.ok(throwsOf(() => collector.collect(view, work, 2)));
    assert.equal(work[ENTRY + 15], 4);
    assert.equal(collector.collect(view, work, 3), 2);
    assert.equal(w.hash.selected, 2);
    w.hash.reset();
    assert.equal(collector.collect(view, new Uint8Array(0), 0), 0);
    assert.equal(w.hash.selected, 2);
});

test('일반 자산 수집기: 손상 체인/타입/프레임과 대리 토큰을 안전하게 거부한다(ObjectCollection_ContractsAndEditionFrames 의 패치 부분)', () => {
    const w = scene();
    const collector = new ObjectCollection(w.hash, w.pool, w.types, w.shapes);
    const view = { left: 10, top: 20, right: 30, bottom: 40 };
    const work = new Uint8Array(ENTRY * 8);
    const run = () => collector.collect(view, work, 8);
    put(w.pool.physical(5), 36, 2, 4);
    assert.throws(run, /frameCheck/);
    put(w.pool.physical(5), 36, 0, 4);
    w.types[1].maxHitPoints = -0x22222223;
    assert.throws(run, /frameCheck/);
    w.types[1].maxHitPoints = -0x32323233;
    assert.throws(run, /frameCheck/);
    w.types[1].maxHitPoints = 100;
    w.shapes[1].loaded = false;
    assert.throws(run, /physical frame/);
    w.shapes[1].loaded = true;
    // 자기 자신을 가리키는 순환 체인(next = 5), 타입 0, 타입 2(표 밖)는 거부한다.
    put(w.pool.physical(5), 4, 5, 2);
    assert.throws(run, /SID cycle/);
    put(w.pool.physical(5), 4, 0, 2);
    w.pool.physical(5)[10] = 0;
    assert.throws(run, /nonzero type/);
    w.pool.physical(5)[10] = 2;
    assert.throws(run, /nonzero type/);
    w.pool.physical(5)[10] = 1;
    // 풀 용량 밖의 칸 SID(65535)는 슬롯 조회에서 거부한다.
    w.hash.setCell(3, 1, 1, 65535);
    assert.ok(throwsOf(run));
    w.hash.setCell(3, 1, 1, 5);
    // 토큰 되돌리기와 생성 검증.
    assert.equal(collector.object(250), 5);
    for (const bad of [0, 251, 24000 * 50]) assert.throws(() => collector.object(bad), RangeError, String(bad));
    assert.throws(() => new ObjectCollection(w.hash, w.pool, w.types, [], 0), /type\/shape tables/);
    assert.throws(() => new ObjectCollection(w.hash, w.pool, [], [], 0), /type\/shape tables/);
    assert.throws(() => new ObjectCollection(w.hash, w.pool, w.types, w.shapes, 0xffff0000), /token range/);
});

// 투명 한 칸/불투명 0·255 를 포함한 실제 VFX RLE 3×1 그림이다(E388 과 같은 입력). (2026-11-13 WB04-E389)
function pixelShape() {
    const bytes = new Uint8Array(46);
    put(bytes, 0, 0x30312e31);
    put(bytes, 4, 1);
    put(bytes, 8, 16);
    put(bytes, 32, 2);
    bytes.set([1, 1, 5, 0, 255, 0], 40);
    return new ShapeDatabase(bytes);
}

test('일반 자산 수집기: 실제 raw 해시 수집→CRT 정렬→공통 분배→SHP 픽셀 출력이 한 흐름으로 이어진다(SpriteDraw_ObjectCollectionPixels)', () => {
    const pool = new SidPool(24000, { server: false });
    const hash = new SquidHash();
    const raw = pool.physical(5);
    raw.fill(0);
    raw[10] = 70;
    put(raw, 14, floatToBits(0.25));
    put(raw, 18, floatToBits(0.25));
    put(raw, 22, 4, 2);
    put(raw, 24, 3, 2);
    hash.setCell(1, 0, 0, 5);
    const frame = { width: 4, height: 4, hotspotX: 1, hotspotY: 1, cellWidth: 1, cellHeight: 1, cellHotspotX: 0, cellHotspotY: 0 };
    const typeRecord = { hotspotX: 0, hotspotY: 0, footX: 0, footY: 0, flags1: 0, flags2: 0, maxHitPoints: 0, cost: 0 };
    const types = Array.from({ length: 71 }, () => ({ ...typeRecord }));
    const shape = { frameCount: 1, loaded: true, frames: [frame] };
    const views = Array.from({ length: 71 }, () => ({ shape: { loaded: false, frameCount: 0, frames: [] }, gumpFrame: 0, capacity: 0, constructionTime: 0, duration: 0 }));
    views[70].shape = shape;
    const shapes = views.map((view) => view.shape);
    const state = new SpriteDrawState({ shadowAliasType: 69, costType: 69, timerTypes: [69, 69, 69], ownerPalettes: [0], selectionColors: [7], ownerMaps: new Array(71).fill(0) });
    const collector = new ObjectCollection(hash, pool, types, shapes, 0x16001000 - 250);
    const database = pixelShape();
    const pixels = new Uint8Array(64).fill(9);
    const target = new SpritePixelTarget(pixels, 8, 8, { left: 0, top: 0, right: 8, bottom: 8 }, () => ({ database, block: 0 }), null);
    const draw = new SpriteDraw(pool, types, views, state, target.hooks(), 0x16001000 - 250);
    const collection = {};
    bindObjectCollection(collection, collector);
    const dispatch = {};
    bindSpriteDraw(dispatch, draw);
    const work = new Uint8Array(ENTRY * 4);
    const request = { screen: { left: 0, top: 0, right: 8, bottom: 8 }, visible: { left: -1, top: -1, right: 2, bottom: 2 }, unclipped: { left: -1, top: -1, right: 2, bottom: 2 }, dirtyFlags: 0, groupRowLimit: 4 };
    const result = collectDrawEntries(request, work, collection);
    assert.equal(result.count, 1);
    assert.equal(hash.selected, 2);
    const entries = work.subarray(0, result.count * ENTRY);
    sortRenderEntries(entries);
    dispatchDrawEntries(entries, {}, dispatch);
    assert.deepEqual([pixels[29], pixels[30]], [0, 255]);
});

test('일반 자산 수집기 계약: 그림자 y 보정·walker y 보정·abstract tail·깊이 부호·판정 구간·해시 단계 확장·순서(y 바깥 x 안쪽)·한 칸 SID 반복', () => {
    // 그림자와 walker: 본체 y 는 walker 이면 +0.5, 그림자 y 는 −0.1(float 로 저장)이며 tail 은 그림자 4·본체 0 이다.
    const w = scene();
    w.types[1].flags1 = 0x40000;
    w.types[1].flags2 = 0x10000;
    const collector = new ObjectCollection(w.hash, w.pool, w.types, w.shapes, 0);
    const out = new Uint8Array(ENTRY * 4);
    const view = { left: 10, top: 20, right: 30, bottom: 40 };
    assert.equal(collector.collect(view, out, 4), 2);
    const data = new DataView(out.buffer);
    assert.deepEqual([data.getFloat32(4, true), data.getFloat32(ENTRY + 4, true), out[15], out[ENTRY + 15]], [Math.fround(25.5), Math.fround(25 - Math.fround(0.1)), 0, 4]);
    // 깊이는 +35 의 signed BYTE 이다(0x80 → −128).
    w.place(5, 20, 25, 0x80);
    assert.equal(collector.collect(view, out, 4), 2);
    assert.equal(data.getInt16(12, true), -128);
    // abstract 타입은 프레임이 논리 프레임 수 이상이면 tail 4 이다(논리 2, 프레임 2·3 은 물리 4 안).
    w.types[1].flags1 = 0x400000;
    for (const [frame, tail] of [[0, 0], [1, 0], [2, 4], [3, 4]]) {
        put(w.pool.physical(5), 36, frame, 4);
        out.fill(0);
        assert.equal(collector.collect(view, out, 4), 1, `프레임 ${frame}`);
        assert.equal(out[15], tail, `프레임 ${frame}`);
    }
    // 가시 영역 밖 객체는 기록하지 않고(월드 위치가 영역 + 셀 크기 + 1 밖), 닫힌 경계 위의 객체는 기록한다.
    const edge = scene();
    const edgeCollector = new ObjectCollection(edge.hash, edge.pool, edge.types, edge.shapes, 0);
    edge.place(5, 31, 25, 0);
    assert.equal(edgeCollector.collect(view, new Uint8Array(ENTRY * 4), 4), 0);
    edge.place(5, 30.25, 25, 0);
    assert.equal(edgeCollector.collect(view, new Uint8Array(ENTRY * 4), 4), 1);
    edge.place(5, 30.3, 25, 0);
    assert.equal(edgeCollector.collect(view, new Uint8Array(ENTRY * 4), 4), 0);
    edge.place(5, 30.25, 25, 0);
    edge.place(5, 7.2, 25, 0);
    assert.equal(edgeCollector.collect(view, new Uint8Array(ENTRY * 4), 4), 0);
    edge.place(5, 7.25, 25, 0);
    assert.equal(edgeCollector.collect(view, new Uint8Array(ENTRY * 4), 4), 1);
    // 같은 칸의 체인은 끝까지 방문하고 next 는 현재 슬롯에서 다시 읽는다(5 → 6 → 끝): 체인 순서대로 기록한다.
    const chain = scene();
    chain.place(6, 21, 26, 3);
    put(chain.pool.physical(5), 4, 6, 2);
    const chainCollector = new ObjectCollection(chain.hash, chain.pool, chain.types, chain.shapes, 0);
    const chainOut = new Uint8Array(ENTRY * 4);
    assert.equal(chainCollector.collect(view, chainOut, 4), 2);
    const chainData = new DataView(chainOut.buffer);
    assert.deepEqual([chainData.getUint32(8, true), chainData.getUint32(ENTRY + 8, true)], [250, 300]);
});

test('일반 자산 수집기 계약: 타입 비트 마스크·프레임 배수·물리 프레임 경계·출력 용량 문구·토큰 경계·월드 끝 칸·확장 칸·2바이트 next 폭', () => {
    const view = { left: 10, top: 20, right: 30, bottom: 40 };
    const make = (mutate = () => {}) => { const w = scene(); mutate(w); return { w, collector: new ObjectCollection(w.hash, w.pool, w.types, w.shapes, 0) }; };
    const count = (mutate) => { const { collector } = make(mutate); return collector.collect(view, new Uint8Array(ENTRY * 8), 8); };
    // flags1 의 낮은 비트(1)만 있으면 그림자/abstract 가 아니다: 본체 하나, tail 0.
    assert.equal(count((w) => { w.types[1].flags1 = 0x1; }), 1);
    assert.equal(count((w) => { w.types[1].flags1 = 0x40000; }), 2);
    assert.equal(count((w) => { w.types[1].flags1 = 0x40001; }), 2);
    // abstract 타입의 프레임은 논리 프레임 수 × 2 미만이어야 한다(논리 1·물리 4: 프레임 1 은 허용, 2 는 frameCheck 거부).
    const abstractFrame = (frame, flags1 = 0x400000) => () => count((w) => { w.shapes[1].frameCount = 1; w.types[1].flags1 = flags1; put(w.pool.physical(5), 36, frame, 4); });
    assert.doesNotThrow(abstractFrame(1));
    assert.throws(abstractFrame(2), /frameCheck/);
    assert.throws(abstractFrame(1, 0), /frameCheck/);
    // 그림자 타입은 그림자 프레임(프레임 + 논리 프레임 수)도 같은 범위 안이어야 한다: 프레임 0 은 허용, 1 은 그림자 프레임 2 가 배수 범위를 넘어 거부한다.
    assert.doesNotThrow(abstractFrame(0, 0x40000));
    assert.throws(abstractFrame(1, 0x40000), /frameCheck/);
    // 논리 프레임 수가 물리 프레임 수보다 커도 물리 프레임 밖은 물리 범위 오류이다.
    assert.throws(() => count((w) => { w.shapes[1].frameCount = 9; put(w.pool.physical(5), 36, 4, 4); }), /^RangeError: Object collection physical frame$/);
    assert.throws(() => count((w) => { put(w.pool.physical(5), 36, 0xffffffff, 4); }), /physical frame/);
    // 실제 출력 칸이 한 개뿐이고 본체+그림자 둘을 낼 때: 둘째 기록 전에 용량 오류 문구로 거부한다.
    const { collector: shadowCollector } = make((w) => { w.types[1].flags1 = 0x40000; });
    assert.throws(() => shadowCollector.collect(view, new Uint8Array(ENTRY), 10), /^RangeError: Object collection output capacity$/);
    // 토큰 경계 문구: 기준 미만과 기준 자체(오프셋 0)는 서로 다른 오류이다.
    const { collector: tokenCollector } = make();
    const based = new ObjectCollection(make().w.hash, make().w.pool, make().w.types, make().w.shapes, 1000);
    assert.throws(() => based.object(999), /^RangeError: Object collection token$/);
    assert.throws(() => based.object(1000), /^RangeError: Object collection token SID$/);
    assert.equal(based.object(1250), 5);
    assert.equal(tokenCollector.object(250), 5);
    // 토큰 범위 정확히 2^32 는 허용하고 하나 더 크면 거부한다.
    const edge = make().w;
    const capacity = edge.pool.capacity;
    assert.doesNotThrow(() => new ObjectCollection(edge.hash, edge.pool, edge.types, edge.shapes, 0x100000000 - capacity * 50));
    assert.throws(() => new ObjectCollection(edge.hash, edge.pool, edge.types, edge.shapes, 0x100000000 - capacity * 50 + 1), /token range/);
    // 월드 끝 칸: 가시 사각형이 월드 밖까지 가도 마지막 칸(3단계 15,15)은 방문한다.
    const last = make((w) => { w.hash.setCell(3, 1, 1, 0); w.hash.setCell(3, 15, 15, 5); w.place(5, 250, 250, 0); });
    assert.equal(last.collector.collect({ left: 240, top: 240, right: 400, bottom: 400 }, new Uint8Array(ENTRY * 4), 4), 1);
    // 가시 구간을 한 칸씩 넓힌다: 구간 가장자리 칸 바로 바깥 칸에 들어 있어도 위치가 가시이면 수집한다(위·아래·왼쪽·오른쪽).
    const around = (cellX, cellY, x, y) => count((w) => { w.hash.setCell(3, 1, 1, 0); w.hash.setCell(3, cellX, cellY, 5); w.place(5, x, y, 0); });
    assert.equal(around(1, 3, 20, 39), 1); // 아래 확장: 3단계 아래 칸 2 → 3
    assert.equal(around(1, 0, 20, 21), 1); // 위 확장: 위 칸 1 → 0
    assert.equal(around(2, 1, 31, 25), 0); // 오른쪽 칸 2 의 객체는 위치가 구간 밖이면 수집하지 않는다.
    assert.equal(around(2, 1, 30, 25), 1); // 오른쪽 확장: 오른쪽 칸 1 → 2
    // next 는 2바이트 WORD 전체이다: SID 300(0x12c)을 체인 둘째로 둔다.
    const chain = make((w) => { w.place(300, 21, 26, 3); put(w.pool.physical(5), 4, 300, 2); });
    const out = new Uint8Array(ENTRY * 4);
    assert.equal(chain.collector.collect(view, out, 4), 2);
    assert.equal(new DataView(out.buffer).getUint32(ENTRY + 8, true), 300 * 50);
    // flags2 의 낮은 비트(1)만 있으면 walker 가 아니다(y 보정 없음): 본체 y 는 25 그대로이다.
    const plain = make((w) => { w.types[1].flags2 = 0x1; });
    const plainOut = new Uint8Array(ENTRY * 2);
    assert.equal(plain.collector.collect(view, plainOut, 2), 1);
    assert.equal(new DataView(plainOut.buffer).getFloat32(4, true), 25);
    // flags1 의 낮은 비트(1)만 있는 타입은 abstract 도 그림자도 아니므로 프레임 수 배수는 1 이다(프레임 1 은 논리 1 이상 → frameCheck 거부).
    assert.throws(abstractFrame(1, 1), /frameCheck/);
    // 가시 사각형이 월드 위쪽/왼쪽 끝 밖이어도 상단 칸 제한 뒤 한 칸만 넓힌다: 3단계 칸(13,13)의 순환 슬롯은 방문하지 않고(제한 15 → 왼쪽/위 14), 칸(14,14)은 방문한다.
    const farView = { left: 400, top: 400, right: 500, bottom: 500 };
    const farCollect = (cellX, cellY) => make((w) => { w.hash.setCell(3, 1, 1, 0); w.hash.setCell(3, cellX, cellY, 5); put(w.pool.physical(5), 4, 5, 2); }).collector.collect(farView, new Uint8Array(ENTRY * 2), 2);
    assert.equal(farCollect(13, 13), 0);
    assert.throws(() => farCollect(14, 14), /SID cycle/);
    // 아래쪽/오른쪽은 가시 구간 끝 칸에서 정확히 한 칸만 넓힌다: 3단계 아래 칸 2 → 3 까지이고 4 는 방문하지 않는다.
    const farBelow = (cellY) => make((w) => { w.hash.setCell(3, 1, 1, 0); w.hash.setCell(3, 1, cellY, 5); put(w.pool.physical(5), 4, 5, 2); }).collector.collect(view, new Uint8Array(ENTRY * 2), 2);
    assert.throws(() => farBelow(3), /SID cycle/);
    assert.equal(farBelow(4), 0);
    const farRight = (cellX) => make((w) => { w.hash.setCell(3, 1, 1, 0); w.hash.setCell(3, cellX, 1, 5); put(w.pool.physical(5), 4, 5, 2); }).collector.collect(view, new Uint8Array(ENTRY * 2), 2);
    assert.throws(() => farRight(2), /SID cycle/);
    assert.equal(farRight(3), 0);
    // 프레임 DWORD 의 상위 바이트까지 읽는다(프레임 0x01000000 은 물리 범위 밖 거부).
    assert.throws(() => count((w) => { put(w.pool.physical(5), 36, 0x01000000, 4); }), /physical frame/);
});

test('ObjectCollection_CommonSubmissionIntegration: UI·지면 묶음·일반 객체(본체/그림자) 세 수집기가 한 공통 목록에서 병합·정렬되고 kind 별로 분배된다 (E389 에서 미룬 시험, WB04-E395)', () => {
    const w = scene();
    w.types[1].flags1 = 0x40000;
    w.types[1].flags2 = 0x10000;
    // 일반 객체 SID 6 은 해시 3단계에, 지면 SID 5 는 0단계 칸 (20,25)에 둔다.
    w.place(6, 20, 25, 10);
    w.hash.setCell(3, 1, 1, 6);
    w.place(5, 20, 25, 5);
    w.hash.setCell(0, 20, 25, 5);
    const objects = new ObjectCollection(w.hash, w.pool, w.types, w.shapes);
    const tiles = new TileGroupCollection(w.hash, w.pool);
    // UI 트리: 루트 1 아래 노드 2 (10,10) 10×10, 깊이 15.
    const nodes = new Map([
        [1, { flags: 0, next: 0, parent: 0, child: 2, object: 0, depth: 0, width: 0, height: 0, x: 0, y: 0, cached: { x: 0, y: 0 } }],
        [2, { flags: 1, next: 0, parent: 1, child: 0, object: 0, depth: 15, width: 10, height: 10, x: 10, y: 10, cached: { x: 0, y: 0 } }],
    ]);
    const gumps = new GumpCollection({ root: 1, overrideHead: 0 }, { node: (token) => nodes.get(token) ?? null });
    const collection = {};
    const dispatch = {};
    bindGumpCollection(collection, gumps);
    bindTileGroupCollection(collection, dispatch, tiles);
    bindObjectCollection(collection, objects);
    const request = { screen: { left: 0, top: 0, right: 100, bottom: 100 }, visible: { left: 10, top: 20, right: 30, bottom: 40 }, unclipped: { left: 0, top: 0, right: 0, bottom: 0 }, dirtyFlags: 0, groupRowLimit: 100 };
    const work = new Uint8Array(12 * ENTRY);
    const result = collectDrawEntries(request, work, collection);
    assert.equal(result.count, 4);
    const entries = work.subarray(0, result.count * ENTRY);
    sortRenderEntries(entries);
    const drawn = [];
    // 그림자 y 가 본체보다 작아 같은 깊이에서 먼저 분배된다. 지면 내부는 kind 1 을 유지한다.
    dispatch.sprite = (entry) => drawn.push(`${objects.object(new DataView(entry.buffer, entry.byteOffset, ENTRY).getUint32(8, true))}:${entry[14]}:${entry[15]}`);
    dispatch.ui = (token) => drawn.push(`ui:${token}`);
    dispatchDrawEntries(entries, {}, dispatch);
    assert.deepEqual(drawn, ['ui:2', '6:0:4', '6:0:0', '5:1:0']);
    drawn.length = 0;
    dispatchDrawEntries(entries, { hideGroups: true, debugAfterEach: false }, dispatch);
    assert.deepEqual(drawn, ['ui:2', '6:0:4', '6:0:0']);
    // UI 전용 dirty(4)는 UI 만 수집하고 해시 선택을 바꾸지 않는다.
    request.dirtyFlags = 4;
    w.hash.select(1);
    assert.equal(collectDrawEntries(request, work, collection).count, 1);
    assert.equal(w.hash.selected, 1);
});
