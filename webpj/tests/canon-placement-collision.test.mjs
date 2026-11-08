// 일반 배치 충돌 raw-canon-placement-collision.js(RawCanonPlacementCollision·placementIgnoreValue·훅 어댑터)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: canoncollision-x86.tsv 의 `originals` 7560행 — 일반 genus 9종×후보 7종×문맥 10종×좌표 6종×전역 2종으로 실제 PE 의 무시 함수(0049ade0)와 한 후보 인라인 구간(0049b9c4..0049bb1a)을 실행해
//    후보 결과(거부/통과), 144바이트 미리보기 배열, 후보 raw 슬롯 전체(불변성)가 같다. 예측 가능한 클라이언트 SID(4·5·14999·15000·15001)와 50 번 슬롯을 모두 쓴다. `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 실제 finder 의 buried 제외·SID/전역 무시와 첫 거부 이후 탐색 중단, 무시한 후보의 지형 효과 뒤 현재 genus 와 미리 저장한 next 사용, 실제 패턴/미리보기/접두/finder 연결에서 두 번째 모양의 거부가 후속 효과를 생략,
//    누락된 정책·잘못된 풀/타입·비유한 좌표 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E153: 일반 배치 충돌 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { RawCanonPlacementGeometry } from '../www/core/raw-canon-placement-geometry.js';
import { RawCanonPlacement, createPlacementState } from '../www/core/raw-canon-placement.js';
import { RawCanonPlacementCollision, createCanonPlacementCollisionState, makeCanonGeometryCollisionHooks, makeCanonPlacementCollisionHooks, placementIgnoreValue } from '../www/core/raw-canon-placement-collision.js';
import { RawCanonPlacementPreview, createCanonPlacementPreviewState, makeCanonGeometryPreviewHooks, makeCanonPlacementPreviewHooks } from '../www/core/raw-canon-placement-preview.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E153)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'canoncollision-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본 행 수, 풀 크기, 타입 수이다. (2026-11-09 WB04-E153)
const ROWS = 7560;
const CAPACITY = 24000;
const TYPE_COUNT = 188;

// 예외가 나는지 본다. (2026-11-09 WB04-E153)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E153)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 새 타입 표와 프레임 표이다. (2026-11-09 WB04-E153)
const makeTypes = () => Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0 }));
const makeFrames = () => Array.from({ length: TYPE_COUNT }, () => ({ frames: [], defaultFrame: 0 }));

test('일반 배치 충돌: 원본 10.78 관측 7560행의 후보 결과·미리보기 배열·후보 raw 불변성이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(allRows.length, 3 * ROWS);
    assert.equal(rows.length, ROWS);
    const pool = new SidPool(CAPACITY, { server: false });
    const hash = new SquidHash();
    const types = makeTypes();
    const state = createCanonPlacementCollisionState();
    const preview = createCanonPlacementPreviewState();
    const collision = new RawCanonPlacementCollision(pool, hash, types, state, preview, { inspectRegions: () => true, inspectCandidateTerrain: () => {} });
    const counts = { passed: 0, rejected: 0, marked: 0 };
    // 각 입력 행을 독립적으로 복구해 앞 행의 표시나 raw 상태가 섞이지 않게 한다.
    for (const row of rows) {
        assert.equal(row.length, 23);
        const own = types[82];
        const other = types[83];
        own.flags2 = Number(row[2]) >>> 0;
        own.flags1 = Number(row[3]) >>> 0;
        other.flags2 = Number(row[4]) >>> 0;
        const mode = Number(row[5]) >>> 0;
        state.clientMode = Number(row[7]) >>> 0;
        state.ignoredGenus = Number(row[6]) >>> 0;
        const sid = Number(row[8]);
        own.footX = Number(row[14]) | 0;
        own.footY = Number(row[15]) | 0;
        other.footX = Number(row[16]) | 0;
        other.footY = Number(row[17]) | 0;
        const raw = pool.physical(sid);
        raw.fill(0xcd);
        raw[10] = 83;
        put(raw, 14, Number(row[12]));
        put(raw, 18, Number(row[13]));
        raw[40] = Number(row[18]) & 0xff;
        preview.blocked.fill(Number(row[19]) ? 0xa5 : 0);
        assert.equal(row[1], 'C');
        const query = { type: 82, argument: 25, x: bitsToFloat(Number(row[10])), y: bitsToFloat(Number(row[11])), flags: 0, owner: 1, mode };
        const result = collision.inspectCandidate(query, Number(row[9]) !== 0, sid) ? 1 : 0;
        const where = row.slice(0, 20).join(' ');
        assert.equal(result, Number(row[20]), where);
        assert.equal(hex(preview.blocked), row[21], where);
        assert.equal(hex(pool.slot(sid)), row[22], where);
        if (result) counts.passed += 1; else counts.rejected += 1;
        if (preview.blocked.some((value) => value === 1)) counts.marked += 1;
    }
    // 통과·거부·미리보기 표시가 모두 실제로 관찰되었는지 확인한다.
    assert.ok(counts.passed > 0 && counts.rejected > 0 && counts.marked > 0, JSON.stringify(counts));
});

test('일반 배치 충돌: 무시 함수의 조기 분기 순서와 반환 비트값을 유지한다', () => {
    const own = { flags1: 0, flags2: 0 };
    const other = { flags1: 0, flags2: 0 };
    assert.equal(placementIgnoreValue(own, other, 0, 0), 0);
    // mode 0 일 때만 배치 genus 0x20000 이 즉시 무시이다.
    own.flags2 = 0x20000;
    assert.equal(placementIgnoreValue(own, other, 0, 0), 1);
    assert.equal(placementIgnoreValue(own, other, 1, 0), 0);
    // 전역이 배치 genus 와 겹치면 후보 genus 의 마스크 비트 그대로(비트값)를 돌려준다.
    own.flags2 = 0x8000;
    other.flags2 = 0x8000;
    assert.equal(placementIgnoreValue(own, other, 1, 0x8000), 0x8000);
    // 후보 genus 가 반환 마스크 밖이면 전역이 겹쳐도 0 이다.
    other.flags2 = 0x4000;
    assert.equal(placementIgnoreValue(own, other, 1, 0x8000), 0);
    // 후보 0x30006 과 배치 0x208100 의 겹침은 전역보다 먼저 1 로 무시한다.
    other.flags2 = 0x10000;
    assert.equal(placementIgnoreValue(own, other, 1, 0x8000), 1);
    // 후보 섬은 배치 타입에 표면 비트가 없을 때만 무시한다. 다리는 후보 0x200000 을 무시한다.
    own.flags2 = 0;
    other.flags2 = 2;
    assert.equal(placementIgnoreValue(own, other, 1, 0), 1);
    own.flags1 = 0x800;
    assert.equal(placementIgnoreValue(own, other, 1, 0), 0);
    own.flags2 = 4;
    other.flags2 = 0x200000;
    assert.equal(placementIgnoreValue(own, other, 1, 0), 1);
});

test('일반 배치 충돌: 실제 finder 의 buried 제외·SID/전역 무시와 첫 거부 이후 탐색 중단을 지킨다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const types = makeTypes();
    types[82].flags2 = 0x8000;
    types[82].footX = types[82].footY = 1;
    types[83].footX = types[83].footY = 1;
    const sid = pool.allocate();
    const raw = pool.allocated(sid);
    raw[10] = 83;
    put(raw, 14, floatToBits(20));
    put(raw, 18, floatToBits(21));
    // 다음 후보 링크를 잘못된 값으로 두어 첫 거부 뒤에 더 읽지 않음을 드러낸다.
    put(raw, 4, 0xffff, 2);
    hash.setBucket(1, 20, 21, sid);
    const state = createCanonPlacementCollisionState();
    const preview = createCanonPlacementPreviewState();
    let requests = 0;
    const collision = new RawCanonPlacementCollision(pool, hash, types, state, preview, {
        inspectRegions: (_query, scan) => {
            requests += 1;
            assert.equal(scan({ left: 20, top: 21, right: 20, bottom: 21 }), false);
            assert.equal(scan({ left: 20, top: 21, right: 20, bottom: 21 }), false);
            return true;
        },
        inspectCandidateTerrain: () => {}
    });
    const query = { type: 82, argument: 25, x: 20, y: 21, flags: 0, owner: 1, mode: 1 };
    assert.equal(collision.inspect(query, true), false);
    assert.ok(requests === 1 && preview.blocked[13] === 1);
    // buried 후보는 finder 가 제외하므로 표시도 거부도 없다.
    put(raw, 4, 0, 2);
    raw[40] = 8;
    preview.blocked.fill(0);
    assert.equal(collision.inspectArea(query, true, { left: 20, top: 21, right: 20, bottom: 21 }), true);
    assert.equal(preview.blocked[13], 0);
    // 전역 무시 마스크가 겹치면 후보 genus 의 비트값이 있어 무시된다.
    raw[40] = 0;
    state.ignoredGenus = 0x8000;
    types[83].flags2 = 0x8000;
    assert.equal(collision.inspectArea(query, false, { left: 20, top: 21, right: 20, bottom: 21 }), true);
});

test('일반 배치 충돌: 무시한 후보의 지형 효과 뒤 현재 genus 와 미리 저장한 next 를 사용한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const types = makeTypes();
    types[82].flags2 = 0x10000;
    types[82].footX = types[82].footY = 1;
    types[83].footX = types[83].footY = 1;
    const first = pool.allocate();
    const second = pool.allocate();
    const firstRaw = pool.allocated(first);
    const secondRaw = pool.allocated(second);
    // 같은 버킷의 체인에 두 후보를 공급한다.
    for (const raw of [firstRaw, secondRaw]) {
        raw[10] = 83;
        put(raw, 14, floatToBits(20));
        put(raw, 18, floatToBits(21));
    }
    put(firstRaw, 4, second, 2);
    hash.setBucket(1, 20, 21, first);
    const state = createCanonPlacementCollisionState();
    const preview = createCanonPlacementPreviewState();
    const visited = [];
    const collision = new RawCanonPlacementCollision(pool, hash, types, state, preview, {
        inspectRegions: (query, scan) => { assert.equal(query.argument, 25); return scan({ left: 20, top: 21, right: 20, bottom: 21 }); },
        inspectCandidateTerrain: (query, sid) => {
            assert.ok(query.owner === 1 && query.mode === 0);
            visited.push(sid);
            put(firstRaw, 4, 0, 2);
            state.ignoredGenus = 0xffffffff;
        }
    });
    assert.equal(collision.inspect({ type: 82, argument: 25, x: 20, y: 21, flags: 0, owner: 1, mode: 0 }, true), false);
    assert.deepEqual(visited, [first]);
    assert.equal(preview.blocked[13], 1);
    assert.equal(get(firstRaw, 4, 2), 0);
});

test('일반 배치 충돌: 실제 패턴·미리보기·접두·finder 를 연결하면 두 번째 모양의 거부가 후속 효과를 생략한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const types = makeTypes();
    const frames = makeFrames();
    const geometryState = { patternTypes: [107, 82, 94, 157, 131, 129, 140, 142], decoderDiagnostics: null };
    types[157].flags2 = 0x10000;
    types[157].footX = types[157].footY = 1;
    types[83].footX = types[83].footY = 1;
    const codes = [];
    // 받침 3×3 패턴은 방향 A..I 의 변형 1 을 사용한다.
    for (let side = 65; side <= 73; side += 1) codes.push({ side, variant: 80, number: 1, flags: 0 });
    frames[157].frames = codes;
    const geometry = new RawCanonPlacementGeometry(pool, types, frames, geometryState);
    const first = pool.allocate();
    const second = pool.allocate();
    const firstRaw = pool.allocated(first);
    const secondRaw = pool.allocated(second);
    firstRaw[10] = secondRaw[10] = 83;
    put(firstRaw, 14, floatToBits(20));
    put(firstRaw, 18, floatToBits(21));
    put(secondRaw, 14, floatToBits(22));
    put(secondRaw, 18, floatToBits(21));
    hash.setBucket(1, 20, 21, first);
    hash.setBucket(1, 22, 21, second);
    const state = createCanonPlacementCollisionState();
    const previewState = createCanonPlacementPreviewState();
    const placementState = createPlacementState();
    placementState.localPlayer = 1;
    const spots = new Uint8Array(65536).fill(6);
    const surfaces = new Uint16Array(65536);
    const events = [];
    const collision = new RawCanonPlacementCollision(pool, hash, types, state, previewState, makeCanonGeometryCollisionHooks(pool, geometry,
        (query) => {
            assert.ok(query.type === 157 && query.argument === 0 && query.owner === 1 && query.mode === 0);
            return {
                beginRegions: () => events.push('begin'),
                beginShape: () => events.push('shape'),
                endShape: () => { events.push('end'); return true; },
                finishRegions: () => { events.push('finish'); return true; }
            };
        },
        { inspectCandidateTerrain: (_query, sid) => { assert.equal(sid, first); events.push('terrain'); state.ignoredGenus = 0xffffffff; } }));
    const preview = new RawCanonPlacementPreview(pool, types, spots, surfaces, previewState, makeCanonGeometryPreviewHooks(pool, geometry, makeCanonPlacementCollisionHooks(pool, collision, {})));
    const placement = new RawCanonPlacement(pool, types, placementState, makeCanonPlacementPreviewHooks(pool, preview, {
        shape: (type, argument) => {
            assert.ok(type === 157 && argument === 0);
            return { left: 0, top: 0, right: 48, bottom: 33 };
        }
    }));
    assert.equal(placement.mayPlace({ type: 157, argument: 0, x: 20.5, y: 21.5, flags: 0, owner: 1, mode: 0 }), false);
    assert.deepEqual(events, ['begin', 'shape', 'terrain', 'end', 'shape']);
    assert.equal(previewState.blocked.filter((value) => value === 1).length, 36);
});

test('일반 배치 충돌: 누락된 정책·잘못된 풀/타입·비유한 좌표를 구별해 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const other = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const types = makeTypes();
    const state = createCanonPlacementCollisionState();
    const preview = createCanonPlacementPreviewState();
    assert.ok(throws(() => new RawCanonPlacementCollision(pool, hash, types, state, preview, {})));
    const collision = new RawCanonPlacementCollision(pool, hash, types, state, preview, { inspectRegions: () => false, inspectCandidateTerrain: () => {} });
    assert.equal(collision.inspect({ type: 82, argument: 25, x: 20, y: 21, flags: 0, owner: 1, mode: 0 }, true), false);
    assert.ok(throws(() => collision.inspect({ type: 188, argument: 0, x: 20, y: 21, flags: 0, owner: 1, mode: 0 }, true)));
    assert.ok(throws(() => collision.inspect({ type: 82, argument: 25, x: Infinity, y: 21, flags: 0, owner: 1, mode: 0 }, true)));
    assert.ok(throws(() => makeCanonPlacementCollisionHooks(other, collision, {})));
    const geometry = new RawCanonPlacementGeometry(pool, types, makeFrames(), { patternTypes: Array(8).fill(0), decoderDiagnostics: null });
    assert.ok(throws(() => makeCanonGeometryCollisionHooks(pool, geometry, undefined, {})));
    assert.ok(throws(() => makeCanonGeometryCollisionHooks(other, geometry, () => ({}), {})));
});
