// 사제 배치 픽셀 모양 raw-priest-placement-shape.js(RawPriestPlacementShape·배치 훅 어댑터)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestshape-x86.tsv 의 `originals` 4032행 — 사제 비패턴 픽셀 getter 를 인자·방향·명시 프레임·프레임 코드 개수·기본 프레임·타입 flags1·SHP 머리(폭/높이/기준점 변화)·핫스폿·배율 입력으로 실행해
//    범위와 기준점 보정의 단정도 비트 여섯 개가 같다. `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 실제 SHP 크기가 지도 여백 거부를 결정하고 허용된 검사만 하위 경계로 진행, 현재 SHP/기본 프레임을 매번 다시 읽음, 패치의 frameCheck(논리 프레임 수·이중 프레임 플래그·해제 표식)와 SHP 없음·표 밖 프레임·명시 프레임·패턴 전역·
//    비사제·비유한 배율·누락 자료·다른 풀 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E156: 사제 배치 픽셀 모양 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawPriestPlacement } from '../www/core/raw-priest-placement.js';
import { createPlacementState } from '../www/core/raw-canon-placement.js';
import { RawPriestPlacementShape, makePriestShapePlacementHooks } from '../www/core/raw-priest-placement-shape.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E156)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestshape-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본 행 수, 전체 행 수(세 PE), 풀 크기, 타입 수, 실제 사제 타입 번호이다. (2026-11-09 WB04-E156)
const ROWS = 4032;
const TOTAL = 8064;
const CAPACITY = 24000;
const TYPE_COUNT = 188;
const PRIEST = 158;

// 예외가 나는지 본다. (2026-11-09 WB04-E156)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 입력 헤더의 부호 있는 16비트 저장 폭을 원본과 같게 유지한다. (2026-11-09 WB04-E156)
const short = (value) => (value << 16) >> 16;

// 새 표 묶음과 사제 모양 계층을 만든다. (2026-11-09 WB04-E156)
function makeWorld() {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0 }));
    const frames = Array.from({ length: TYPE_COUNT }, () => ({ frames: [], defaultFrame: 0 }));
    const shapes = Array.from({ length: TYPE_COUNT }, () => ({ frameCount: 0, loaded: false, frames: [] }));
    const hotspots = Array.from({ length: TYPE_COUNT }, () => ({ x: 0, y: 0 }));
    const geometryState = { patternTypes: Array(8).fill(0), decoderDiagnostics: null };
    const state = { scaleX: 16, scaleY: 11 };
    return { pool, types, frames, shapes, hotspots, geometryState, state, shape: new RawPriestPlacementShape(pool, types, frames, shapes, hotspots, geometryState, state) };
}

// 코드가 없는 프레임 코드를 개수만큼 만든다. (2026-11-09 WB04-E156)
const blankCodes = (count) => Array.from({ length: count }, () => ({ side: 0, variant: 0, number: 0, flags: 0 }));

test('사제 배치 픽셀 모양: 원본 10.78 관측 4032행의 범위·기준점 보정 비트가 같다', { skip: rows.length === 0 }, () => {
    assert.equal(allRows.length, TOTAL);
    assert.equal(rows.length, ROWS);
    const { types, frames, shapes, hotspots, state, shape } = makeWorld();
    types[PRIEST].flags2 = 0x210000;
    // 각 행의 원본 입력으로 같은 프레임 번호 체계/현재 타입 기준점을 구성한다.
    for (const row of rows) {
        assert.equal(row.length, 22);
        const type = Number(row[1]);
        frames[type].frames = blankCodes(Number(row[5]));
        frames[type].defaultFrame = Number(row[6]) | 0;
        types[type].flags1 = Number(row[7]) >>> 0;
        hotspots[type] = { x: Number(row[12]) | 0, y: Number(row[13]) | 0 };
        state.scaleX = bitsToFloat(Number(row[14]));
        state.scaleY = bitsToFloat(Number(row[15]));
        const count = Number(row[5]);
        // 입력 생성기의 비연속 주소 표와 같은 물리 프레임 순서(여덟 프레임)이다.
        shapes[type] = {
            frameCount: count,
            loaded: true,
            frames: Array.from({ length: 8 }, (_, frame) => ({ width: short(Number(row[8]) + frame * 2), height: short(Number(row[9]) - frame), hotspotX: short(Number(row[10]) + frame), hotspotY: short(Number(row[11]) - frame) }))
        };
        const result = shape.measure(type, Number(row[2]) >>> 0, Number(row[3]) | 0, Number(row[4]) !== 0);
        const actual = [result.bounds.left, result.bounds.top, result.bounds.right, result.bounds.bottom, result.anchorX, result.anchorY].map(floatToBits);
        assert.deepEqual(actual, [16, 17, 18, 19, 20, 21].map((index) => Number(row[index])), row.slice(0, 16).join(' '));
    }
});

test('사제 배치 픽셀 모양: 실제 SHP 크기가 지도 여백 거부를 결정하고 허용된 검사만 하위 경계로 진행한다', () => {
    const { pool, types, frames, shapes, hotspots, geometryState, state, shape } = makeWorld();
    const placementState = createPlacementState();
    placementState.localPlayer = 1;
    placementState.blockedRelation = 1;
    types[PRIEST].flags2 = 0x210000;
    frames[PRIEST].frames = blankCodes(2);
    shapes[PRIEST] = { frameCount: 2, loaded: true, frames: [{ width: 160, height: 110, hotspotX: 80, hotspotY: 55 }, { width: 16, height: 11, hotspotX: 8, hotspotY: 5 }] };
    hotspots[PRIEST] = { x: 8, y: 5 };
    let inspections = 0;
    const placement = new RawPriestPlacement(pool, types, placementState, makePriestShapePlacementHooks(pool, shape, {
        inspectGeometry: (_query, local) => { assert.ok(local); inspections += 1; return true; }
    }));
    const query = { type: PRIEST, x: 9, y: 9, flags: 0, owner: 1, mode: 0 };
    // 첫 프레임(160×110)은 여백이 커서 (9,9)를 거부하고 하위 경계에 가지 않는다.
    assert.ok(!placement.mayPlace(query) && inspections === 0 && placementState.blockedRelation === 0);
    frames[PRIEST].defaultFrame = 1;
    assert.ok(placement.mayPlace(query) && inspections === 1);
    // 명시 프레임은 인자를 프레임으로 쓰고, 현재 SHP 가 없으면 빈 범위에 현재 배율이 기준점으로 남는다.
    const selected = shape.measure(PRIEST, 0, 0, true);
    assert.ok(selected.bounds.right - selected.bounds.left === 160 && selected.bounds.bottom - selected.bounds.top === 110);
    frames[PRIEST].defaultFrame = -1;
    shapes[PRIEST].loaded = false;
    const empty = shape.measure(PRIEST, PRIEST, 0);
    assert.deepEqual(empty.bounds, { left: 1000, top: 1000, right: 0, bottom: 0 });
    assert.ok(empty.anchorX === 16 && empty.anchorY === 11);
    assert.ok(geometryState && state);
});

test('사제 배치 픽셀 모양: 패치의 frameCheck 와 표 밖 접근·패턴 전역·비사제·비유한 배율·누락 자료·다른 풀을 거부한다', () => {
    const { pool, types, frames, shapes, hotspots, geometryState, state, shape } = makeWorld();
    types[PRIEST].flags2 = 0x200000;
    frames[PRIEST].frames = blankCodes(1);
    frames[PRIEST].defaultFrame = 1;
    shapes[PRIEST] = { frameCount: 1, loaded: true, frames: [{ width: 0, height: 0, hotspotX: 0, hotspotY: 0 }, { width: 0, height: 0, hotspotX: 0, hotspotY: 0 }] };
    // 논리 프레임 수(1)를 넘는 기본 프레임은 거부하나 이중 프레임 플래그는 두 배까지 허용한다.
    assert.ok(throws(() => shape.measure(PRIEST, PRIEST, 0)));
    types[PRIEST].flags1 = 0x40000;
    assert.equal(shape.measure(PRIEST, PRIEST, 0).bounds.right, 0);
    types[PRIEST].maxHitPoints = -0x22222223;
    assert.ok(throws(() => shape.measure(PRIEST, PRIEST, 0)));
    types[PRIEST].maxHitPoints = 0;
    frames[PRIEST].defaultFrame = 2;
    assert.ok(throws(() => shape.measure(PRIEST, PRIEST, 0)));
    frames[PRIEST].defaultFrame = 0;
    shapes[PRIEST].loaded = false;
    assert.ok(throws(() => shape.measure(PRIEST, PRIEST, 0)));
    shapes[PRIEST].loaded = true;
    // 명시 프레임이 코드 표 밖이면 거부한다.
    assert.ok(throws(() => shape.measure(PRIEST, 1, 0, true)));
    geometryState.patternTypes[2] = PRIEST;
    assert.ok(throws(() => shape.measure(PRIEST, PRIEST, 0)));
    geometryState.patternTypes[2] = 0;
    assert.ok(throws(() => makePriestShapePlacementHooks(new SidPool(CAPACITY, { server: true }), shape, {})));
    assert.ok(throws(() => new RawPriestPlacementShape(pool, types, frames, [], hotspots, geometryState, state)));
    state.scaleX = Infinity;
    assert.ok(throws(() => shape.measure(PRIEST, PRIEST, 0)));
    types[PRIEST].flags2 = 0;
    assert.ok(throws(() => shape.measure(PRIEST, PRIEST, 0)));
});
