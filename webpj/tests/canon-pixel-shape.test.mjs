// 일반 타입·패턴 전체의 픽셀 범위 getter raw-canon-pixel-shape.js(RawCanonPixelShape)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: canonshape-x86.tsv 의 `originals` 3150행 — 타입 전역 9종(영역·다리·섬·받침·전투 넷·일반 사제)×인자×방향(홀수·signed −1 포함)×프레임 표 3종(정상·역순·빈 표)×전역 별칭×기본 프레임×
//    SHP 머리 입력 3종(폭/높이/기준점 변화·16비트 경계 감싸기·모두 0)×배율×핫스폿에서 범위(왼쪽·위·오른쪽·아래)와 기준점 보정(X·Y)의 단정도 비트 여섯 개가 같다. `originalCD`·`original1037` 행은 사용하지 않는다.
//  - 계약: 일반 자산에는 사제 플래그가 필요 없고 현재 SHP/배율/핫스폿을 매 호출 다시 읽음, 빈 모양의 시작값, 표 밖 타입·누락된 표·해제된 타입·논리 프레임 범위 밖·SHP 없음·비유한 배율 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E147: 일반 픽셀 모양 getter 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawCanonPixelShape } from '../www/core/raw-canon-pixel-shape.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E147)
const rows = (readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'canonshape-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');

// 실제 타입 전역의 패턴 번호와 마지막 비패턴 사제 번호, 판본 행 수, 풀 크기, 타입 수이다. (2026-11-09 WB04-E147)
const TYPES = [107, 82, 94, 157, 131, 129, 140, 142, 158];
const ROWS = 3150;
const CAPACITY = 24000;
const TYPE_COUNT = 188;

// 예외가 나는지 본다. (2026-11-09 WB04-E147)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 기계어 분석 입력과 같은 프레임 코드 표이다. profile 0 정상, 1 역순, 2 빈 표이다. (2026-11-09 WB04-E147)
function inputFrames(profile) {
    const codes = [];
    if (profile !== 2) {
        // 정상 방향 A..P 와 원본 셀의 번호 영역을 모두 공급한다(번호 10 도 포함).
        for (let side = 65; side <= 80; side += 1) {
            for (let number = 0; number <= 10; number += 1) codes.push({ side, variant: 80, number, flags: 0 });
        }
    }
    if (profile === 1) codes.reverse();
    if (profile !== 2) codes.push(codes[0]);
    return codes;
}

// 부호 있는 16비트 저장 폭을 유지하며 큰 입력은 감싼다. (2026-11-09 WB04-E147)
const short = (value) => (value << 16) >> 16;

// 입력 SHP 머리는 선택/전체 순회/동률이 드러나도록 물리 번호마다 다르다. (2026-11-09 WB04-E147)
function header(frame, metric) {
    if (metric === 0) return { width: short(24 + (frame % 7) * 3), height: short(19 + (frame % 5) * 2), hotspotX: short(12 + (frame % 11)), hotspotY: short(7 - (frame % 13)) };
    if (metric === 1) return { width: short(32767 + frame * 37), height: short(-32768 + frame * 29), hotspotX: short(32767 - frame * 31), hotspotY: short(-32768 + frame * 17) };
    return { width: 0, height: 0, hotspotX: 0, hotspotY: 0 };
}

// 출력 float 여섯 개의 비트를 원본 순서로 보존한다. (2026-11-09 WB04-E147)
function bits(shape) {
    return [shape.bounds.left, shape.bounds.top, shape.bounds.right, shape.bounds.bottom, shape.anchorX, shape.anchorY].map(floatToBits);
}

// 풀·타입·프레임·SHP·기준점·전역·배율을 만든다. (2026-11-09 WB04-E147)
function makeWorld() {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
    const frames = Array.from({ length: TYPE_COUNT }, () => ({ frames: [], defaultFrame: 0 }));
    const shapes = Array.from({ length: TYPE_COUNT }, () => ({ frameCount: 0, loaded: false, frames: [] }));
    const hotspots = Array.from({ length: TYPE_COUNT }, () => ({ x: 0, y: 0 }));
    const geometryState = { patternTypes: TYPES.slice(0, 8), decoderDiagnostics: null };
    const state = { scaleX: 16, scaleY: 11 };
    return { pool, types, frames, shapes, hotspots, geometryState, state, shape: new RawCanonPixelShape(pool, types, frames, shapes, hotspots, geometryState, state) };
}

test('일반 픽셀 모양: 원본 10.78 관측 3150행의 범위·기준점 보정 비트가 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    const { frames, shapes, hotspots, geometryState, state, shape } = makeWorld();
    // 각 행은 모든 타입별 패턴 선택과 현재 메타 자료를 독립적으로 구성한다.
    for (const row of rows) {
        assert.equal(row.length, 19);
        const type = TYPES[Number(row[1])];
        geometryState.patternTypes = TYPES.slice(0, 8);
        if (row[6] === '1') geometryState.patternTypes[0] = geometryState.patternTypes[1];
        if (row[6] === '2') geometryState.patternTypes[2] = geometryState.patternTypes[4];
        frames[type].frames = inputFrames(Number(row[4]));
        frames[type].defaultFrame = Number(row[7]) | 0;
        hotspots[type] = { x: Number(row[11]) | 0, y: Number(row[12]) | 0 };
        state.scaleX = bitsToFloat(Number(row[9]));
        state.scaleY = bitsToFloat(Number(row[10]));
        const count = frames[type].frames.length;
        // 비연속 원본 SHP 주소 표와 같은 물리 순서의 머리 입력이다.
        shapes[type] = { frameCount: count, loaded: true, frames: Array.from({ length: count }, (_, frame) => header(frame, Number(row[8]))) };
        const expected = [13, 14, 15, 16, 17, 18].map((index) => Number(row[index]));
        const where = row.slice(0, 13).join(' ');
        const actual = bits(shape.measure(type, Number(row[2]) | 0, Number(row[3]) | 0, row[5] !== '0'));
        assert.deepEqual(actual, expected, where);
    }
});

test('일반 픽셀 모양: 일반 자산에는 사제 플래그가 필요 없고 현재 SHP/배율/핫스폿을 매 호출 다시 읽는다', () => {
    const { types, frames, shapes, hotspots, geometryState, state, shape, pool } = makeWorld();
    // 사제/패턴에 해당하지 않는 내장 번호로 일반 getter 계약을 검사한다.
    const type = 70;
    geometryState.patternTypes = Array(8).fill(0);
    frames[type].frames = inputFrames(0);
    shapes[type] = { frameCount: 177, loaded: true, frames: Array.from({ length: 177 }, () => ({ width: 16, height: 11, hotspotX: 8, hotspotY: 5 })) };
    assert.equal(shape.measure(type, type, 0).bounds.left, -8);
    hotspots[type].x = 8;
    assert.equal(shape.measure(type, type, 0).bounds.left, 0);
    shapes[type].frames[0].width = 32;
    assert.equal(shape.measure(type, type, 0).bounds.right, 32);
    // 기본 프레임이 없고 SHP 가 없으면 빈 범위이며 기준점은 현재 배율이다.
    frames[type].defaultFrame = -1;
    shapes[type].loaded = false;
    state.scaleX = 0.5;
    state.scaleY = 1.25;
    const empty = shape.measure(type, type, 0);
    assert.deepEqual(empty.bounds, { left: 1000, top: 1000, right: 0, bottom: 0 });
    assert.equal(empty.anchorX, 0.5);
    assert.equal(empty.anchorY, 1.25);
    assert.equal(shape.pool, pool);
    assert.ok(throws(() => shape.measure(188, 0, 0)));
    assert.ok(throws(() => new RawCanonPixelShape(pool, types, frames, [], hotspots, geometryState, state)));
});

test('일반 픽셀 모양: SHP 없음·논리 프레임 범위 밖·해제된 타입·비유한 배율을 거부하고 이중 프레임 플래그는 논리 프레임 수를 늘린다', () => {
    const { types, frames, shapes, geometryState, state, shape } = makeWorld();
    const type = 70;
    geometryState.patternTypes = Array(8).fill(0);
    frames[type].frames = inputFrames(0);
    shapes[type] = { frameCount: 177, loaded: true, frames: Array.from({ length: 300 }, () => ({ width: 16, height: 11, hotspotX: 8, hotspotY: 5 })) };
    // 기본 프레임이 있는데 SHP 가 없으면 거부한다.
    shapes[type].loaded = false;
    assert.ok(throws(() => shape.measure(type, type, 0)));
    shapes[type].loaded = true;
    // 논리 프레임 수(177)를 넘는 기본 프레임은 거부하나 이중 프레임 플래그가 있으면 두 배(354)까지 SHP 물리 프레임(300) 안에서 허용한다.
    frames[type].defaultFrame = 200;
    assert.ok(throws(() => shape.measure(type, type, 0)));
    types[type].flags1 = 0x440000;
    assert.equal(shape.measure(type, type, 0).bounds.right, 16 - 8);
    frames[type].defaultFrame = 299;
    assert.ok(Number.isFinite(shape.measure(type, type, 0).bounds.right));
    frames[type].defaultFrame = 300;
    assert.ok(throws(() => shape.measure(type, type, 0)));
    // 두 해제 표식은 거부한다.
    frames[type].defaultFrame = 0;
    types[type].maxHitPoints = -0x22222223;
    assert.ok(throws(() => shape.measure(type, type, 0)));
    types[type].maxHitPoints = -0x32323233;
    assert.ok(throws(() => shape.measure(type, type, 0)));
    types[type].maxHitPoints = 0;
    // 비유한 배율은 유효 칸이 있을 때만 거부한다.
    state.scaleX = Infinity;
    assert.ok(throws(() => shape.measure(type, type, 0)));
    state.scaleX = 16;
    state.scaleY = Number.NaN;
    assert.ok(throws(() => shape.measure(type, type, 0)));
    frames[type].defaultFrame = -1;
    assert.equal(shape.measure(type, type, 0).bounds.left, 1000);
});
