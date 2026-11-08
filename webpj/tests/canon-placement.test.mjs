// 일반 배치 접두 raw-canon-placement.js(RawCanonPlacement·공통 모양 훅 어댑터)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: canonplacement-x86.tsv 의 `originals` 1120행 — 모양 10종×좌표 16종×전역/소유자 7종을 실제 PE 의 접두(차단값 초기화·강제/즉시 허용·부호 확장한 소유자와 로컬 비교·모양 여백·지도 경계)로 실행해
//    허용 결과, 전역 차단값, 강제 허용·로컬 플레이어 전역, 타입 genus, 모양/후반 경계 호출 기록(`H:`/`G:` 사건 열)이 같다. 모양 조회 중 전역 변경(1)과 후반 경계의 전역 변경(2)도 공급한다.
//    `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 사제 플래그 없이 일반 타입을 처리하고 입력/로컬 비교를 값으로 캡처, 진입마다 차단값 초기화, 타입 69/255 거부·비유한 좌표·누락된 경계·표 크기 거부, 실제 복원 패턴(다리)과 SHP 를 접두에 연결해 argument/방향 전달.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E152: 일반 배치 접두 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawCanonPixelShape } from '../www/core/raw-canon-pixel-shape.js';
import { RawCanonPlacement, createPlacementState, currentForcePlacement, makeCanonShapePlacementHooks } from '../www/core/raw-canon-placement.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E152)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'canonplacement-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본 행 수, 풀 크기, 타입 수이다. (2026-11-09 WB04-E152)
const ROWS = 1120;
const CAPACITY = 24000;
const TYPE_COUNT = 188;

// 예외가 나는지 본다. (2026-11-09 WB04-E152)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 새 타입 표와 프레임 표이다. (2026-11-09 WB04-E152)
const makeTypes = () => Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0 }));
const makeFrames = () => Array.from({ length: TYPE_COUNT }, () => ({ frames: [], defaultFrame: 0 }));

test('일반 배치 접두: 원본 10.78 관측 1120행의 허용·전역·사건 기록이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(allRows.length, 3 * ROWS);
    assert.equal(rows.length, ROWS);
    const pool = new SidPool(CAPACITY, { server: false });
    const types = makeTypes();
    const counts = { allowed: 0, rejected: 0, shapeCalled: 0 };
    // 구현으로 기대 여백이나 허용 여부를 계산하지 않고 원본 관찰만 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 24);
        const events = [];
        const query = { type: Number(row[1]), argument: Number(row[2]), x: bitsToFloat(Number(row[3])), y: bitsToFloat(Number(row[4])), flags: Number(row[6]), owner: Number(row[5]), mode: Number(row[7]) };
        types[query.type].flags2 = Number(row[8]) >>> 0;
        const state = createPlacementState();
        state.forcePlacement = Number(row[9]) >>> 0;
        state.localPlayer = Number(row[10]) | 0;
        state.blockedRelation = 0xffffffff;
        const rect = { left: bitsToFloat(Number(row[11])), top: bitsToFloat(Number(row[12])), right: bitsToFloat(Number(row[13])), bottom: bitsToFloat(Number(row[14])) };
        const lower = Number(row[15]);
        const change = Number(row[16]);
        const placement = new RawCanonPlacement(pool, types, state, {
            shape: (type, argument, flags) => {
                assert.ok(type === query.type && argument === query.argument && flags === query.flags && state.blockedRelation === 0);
                events.push(`H:${type}:${argument}:${flags}`);
                counts.shapeCalled += 1;
                if (change === 1) { state.localPlayer = 17; state.forcePlacement = 1; types[type].flags2 = 0; }
                return rect;
            },
            inspectGeometry: (input, localOwner) => {
                assert.ok(input.type === query.type && input.argument === query.argument && input.x === query.x && input.y === query.y && state.blockedRelation === 0);
                events.push(`G:${localOwner ? 1 : 0}:${input.owner}:${input.flags}:${input.mode}:${lower}`);
                state.blockedRelation = 0x77;
                if (change === 2) { state.forcePlacement = 9; state.localPlayer = -7; }
                return lower !== 0;
            }
        });
        const allowed = placement.mayPlace(query);
        const where = row.slice(0, 17).join(' ');
        assert.equal(allowed, Number(row[17]) !== 0, where);
        assert.equal(state.blockedRelation >>> 0, Number(row[19]), where);
        assert.equal(state.forcePlacement >>> 0, Number(row[20]), where);
        assert.equal(state.localPlayer >>> 0, Number(row[21]), where);
        assert.equal(types[query.type].flags2 >>> 0, Number(row[22]), where);
        assert.equal(events.join(';'), row[23], where);
        if (allowed) counts.allowed += 1; else counts.rejected += 1;
    }
    // 허용과 거부, 모양 조회가 모두 실제로 실행되었는지 확인한다.
    assert.ok(counts.allowed > 0 && counts.rejected > 0 && counts.shapeCalled > 0, JSON.stringify(counts));
});

test('일반 배치 접두: 사제 플래그 없이 일반 타입을 처리하고 입력과 로컬 비교를 값으로 캡처한다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const types = makeTypes();
    const state = { forcePlacement: 0, localPlayer: 8, blockedRelation: 99, sharedForcePlacement: null };
    let submitted = { type: 82, argument: 25, x: 20, y: 21, flags: 6, owner: 0x108, mode: 1 };
    let calls = 0;
    const hooks = {
        shape: (type, argument, flags) => {
            assert.ok(type === 82 && argument === 25 && flags === 6 && state.blockedRelation === 0);
            // 호출자가 요청 객체를 바꾸고 로컬 플레이어를 바꿔도 접두가 캡처한 값은 유지된다.
            submitted.type = 70;
            submitted.argument = 67;
            state.localPlayer = 17;
            return { left: 0, top: 0, right: 16, bottom: 11 };
        },
        inspectGeometry: (query, localOwner) => {
            assert.ok(query.type === 82 && query.argument === 25 && query.x === 20 && query.y === 21 && query.flags === 6 && query.owner === 0x108 && query.mode === 1 && localOwner);
            calls += 1;
            return false;
        }
    };
    const placement = new RawCanonPlacement(pool, types, state, hooks);
    assert.ok(!placement.mayPlace(submitted) && calls === 1 && state.localPlayer === 17);
    // 강제 허용 전역은 모양 조회 없이 참이며 차단값은 먼저 지운다.
    state.blockedRelation = 99;
    state.forcePlacement = 1;
    assert.ok(placement.mayPlace({ type: 82, argument: 0xffffffff, x: 0, y: 0, flags: 0, owner: 255, mode: 0xffffffff }) && state.blockedRelation === 0 && calls === 1);
    // 즉시 허용 genus 도 같다.
    state.forcePlacement = 0;
    types[82].flags2 = 0x02000000;
    assert.ok(placement.mayPlace({ type: 82, argument: 67, x: -1, y: -1, flags: 0, owner: 0, mode: 0 }) && calls === 1);
    // 타입 번호와 좌표, 경계 누락, 표 크기는 거부한다.
    assert.ok(throws(() => placement.mayPlace({ type: 69, argument: 0, x: 0, y: 0, flags: 0, owner: 0, mode: 0 })));
    assert.ok(throws(() => placement.mayPlace({ type: 255, argument: 0, x: 0, y: 0, flags: 0, owner: 0, mode: 0 })));
    assert.ok(throws(() => placement.mayPlace({ type: 82, argument: 0, x: Infinity, y: 0, flags: 0, owner: 0, mode: 0 })));
    assert.ok(throws(() => new RawCanonPlacement(pool, types, state, { ...hooks, inspectGeometry: undefined })));
    assert.ok(throws(() => new RawCanonPlacement(pool, types.slice(0, 1), state, hooks)));
    // 공유 강제 허용 getter 가 있으면 그 현재 값을 읽는다.
    let shared = 0;
    state.sharedForcePlacement = () => shared;
    state.forcePlacement = 5;
    assert.equal(currentForcePlacement(state), 0);
    shared = 0x80000000;
    assert.equal(currentForcePlacement(state), 0x80000000);
});

test('일반 배치 접두: 실제 복원 패턴과 SHP 를 접두에 연결해 argument 와 방향을 전달하고 지도 경계를 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const types = makeTypes();
    const frames = makeFrames();
    const shapes = Array.from({ length: TYPE_COUNT }, () => ({ frameCount: 0, loaded: false, frames: [] }));
    const hotspots = Array.from({ length: TYPE_COUNT }, () => ({ x: 0, y: 0 }));
    const geometryState = { patternTypes: [107, 82, 94, 157, 131, 129, 140, 142], decoderDiagnostics: null };
    const pixelState = { scaleX: 16, scaleY: 11 };
    const state = createPlacementState();
    state.localPlayer = 8;
    state.blockedRelation = 99;
    const codes = [];
    // 유효 원본 패턴에 필요한 방향/프레임 라벨을 정상 메타 입력으로 제공한다.
    for (let side = 65; side <= 80; side += 1) {
        // 한 자리 프레임 라벨 전체를 물리 SHP 표에 대응시킨다.
        for (let number = 0; number <= 10; number += 1) codes.push({ side, variant: 80, number, flags: 0 });
    }
    frames[82].frames = codes;
    shapes[82] = { frameCount: codes.length, loaded: true, frames: codes.map(() => ({ width: 16, height: 11, hotspotX: 8, hotspotY: 5 })) };
    const shape = new RawCanonPixelShape(pool, types, frames, shapes, hotspots, geometryState, pixelState);
    let calls = 0;
    const placement = new RawCanonPlacement(pool, types, state, makeCanonShapePlacementHooks(pool, shape, {
        inspectGeometry: (query, localOwner) => {
            assert.ok(query.type === 82 && query.argument === 25 && query.flags === 6 && localOwner);
            calls += 1;
            return true;
        }
    }));
    assert.ok(placement.mayPlace({ type: 82, argument: 25, x: 20, y: 21, flags: 6, owner: 8, mode: 0 }) && calls === 1);
    assert.ok(!placement.mayPlace({ type: 82, argument: 25, x: 0, y: 0, flags: 6, owner: 8, mode: 0 }) && calls === 1 && state.blockedRelation === 0);
    assert.ok(throws(() => makeCanonShapePlacementHooks(new SidPool(CAPACITY, { server: false }), shape, {})));
});
