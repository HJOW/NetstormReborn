// 일반 배치 지형 raw-canon-placement-terrain.js(RawCanonPlacementTerrain·훅 어댑터)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: canonterrain-x86.tsv 의 `originals` 3740행 — 후보 지형(C)·모양 종료(E)·최종 지면 누적(F)·지역 getter(R)·시작(B)을 소유 genus/그룹/플래그/발자국/후보 genus/프레임 지면 플래그/좌표/지역값/
//    섬 지도 유무/배열 씨앗 6종/sentinel/초기 지면 완성 조합으로 실제 PE 에서 실행해 결과(반환값·다리 겹침·받침 전용·지면 완성·권한·지면 허용)와 144바이트 지역 배열, 후보 raw 슬롯 전체가 같다.
//    `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 무권한 모양에서만 주변 권한 조회, 빈 모양(디코더)이 이전 배열을 유지한 채 최종 경계를 받음, 필수 경계·자료 크기·풀·순회 순서 거부, 0 발자국과 다리 우선 지면 누적, 발자국 13/비유한 좌표/표 밖 타입 거부.
//  - 이월: C++ 의 실제 받침 decoder/미리보기/finder 연결 시험(`canon_terrain_pattern_preview_finder_and_permission_lifetime`)은 충돌/미리보기/배치 접두 모듈 이식 뒤 그 단위의 통합 시험에서 다룬다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E148: 일반 배치 지형 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawCanonPlacementGeometry } from '../www/core/raw-canon-placement-geometry.js';
import { RawCanonPlacementTerrain, createCanonPlacementTerrainState, makeCanonTerrainCollisionHooks, makeCanonTerrainGeometryHooks } from '../www/core/raw-canon-placement-terrain.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E148)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'canonterrain-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본 행 수, 풀 크기, 합성 배치/후보 타입, 후보 슬롯 SID, 타입 수이다. (2026-11-09 WB04-E148)
const ROWS = 3740;
const CAPACITY = 24000;
const OWN = 82;
const OTHER = 83;
const SOURCE = 50;
const TYPE_COUNT = 188;

// 예외가 나는지 본다. (2026-11-09 WB04-E148)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E148)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 모든 훅이 거짓을 돌려주는 기본 훅이다. (2026-11-09 WB04-E148)
const denyHooks = () => ({ candidatePermission: () => false, shapePermission: () => false, finishRelations: () => false });

// 풀·타입·프레임·섬 지도·상태·지형을 만든다. (2026-11-09 WB04-E148)
function makeWorld(hooks = denyHooks()) {
    const pool = new SidPool(CAPACITY, { server: false });
    const types = Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0 }));
    const frames = Array.from({ length: TYPE_COUNT }, () => ({ frames: [], defaultFrame: 0 }));
    const islands = new Uint16Array(65536);
    const state = createCanonPlacementTerrainState();
    return { pool, types, frames, islands, state, terrain: new RawCanonPlacementTerrain(pool, types, frames, islands, state, hooks) };
}

// 원본 실행기의 입력 배열을 복구한다. 기대 배열은 fixture 에서 그대로 읽는다. (2026-11-09 WB04-E148)
function seed(state, seedNumber) {
    state.regions.fill([state.emptyRegion & 0xff, 255, 4, 127, 128, 128][seedNumber]);
    if (seedNumber === 3) state.regions[0] = 4;
    if (seedNumber === 5) state.regions[13] = 4;
}

test('일반 배치 지형: 원본 10.78 관측 3740행의 결과·지역 배열·후보 슬롯이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(allRows.length, ROWS * 3);
    assert.equal(rows.length, ROWS);
    const { pool, types, frames, islands, state, terrain } = makeWorld();
    const counts = { B: 0, C: 0, E: 0, F: 0 };
    let regionRows = 0;
    // 행마다 입력과 배열을 초기화해 이전 순회 결과가 다음 행에 섞이지 않게 한다.
    for (const row of rows) {
        assert.equal(row.length, 28);
        const own = types[OWN];
        const other = types[OTHER];
        own.flags1 = Number(row[4]) >>> 0;
        own.flags2 = Number(row[1]) >>> 0;
        own.group = Number(row[2]) | 0;
        own.footX = Number(row[9]) | 0;
        own.footY = Number(row[10]) | 0;
        other.flags2 = Number(row[5]) >>> 0;
        other.footX = Number(row[11]) | 0;
        other.footY = Number(row[12]) | 0;
        frames[OTHER].frames = [{ side: 0, variant: 0, number: 0, flags: 0 }, { side: 0, variant: 0, number: 0, flags: Number(row[6]) & 0xff }];
        const raw = pool.physical(SOURCE);
        raw.fill(0xcd);
        raw[10] = OTHER;
        put(raw, 8, Number(row[15]), 2);
        put(raw, 14, Number(row[7]));
        put(raw, 18, Number(row[8]));
        raw[40] = Number(row[13]) & 0xff;
        put(raw, 36, 1, 4);
        islands.fill(0);
        const x = Math.trunc(bitsToFloat(Number(row[7])));
        const y = Math.trunc(bitsToFloat(Number(row[8])));
        if (x >= 0 && x < 256 && y >= 0 && y < 256) islands[y * 256 + x] = Number(row[16]) ? SOURCE : 0;
        state.emptyRegion = Number(row[18]) >>> 0;
        state.noIslandType = Number(row[14]) >>> 0;
        const query = { type: OWN, argument: OWN, x: 20, y: 21, flags: 0, owner: 1, mode: 0 };
        const kind = row[3];
        terrain.begin(query);
        if (kind !== 'B') terrain.beginShape(query, { frame: 0, label: 0, side: 0, x: 20, y: 21, snappedX: 20, snappedY: 21, area: {} });
        seed(state, Number(row[17]));
        state.groundComplete = Number(row[19]) !== 0;
        if (kind !== 'B') state.permission = true;
        let result = 0;
        if (kind === 'B') counts.B += 1;
        else if (kind === 'C') { terrain.candidate(query, SOURCE); counts.C += 1; }
        else if (kind === 'E') { assert.equal(terrain.endShape(query), true); counts.E += 1; }
        else if (kind === 'F') {
            state.bridgeOverlap = Number(row[13]) !== 0;
            state.noIslandOnly = Number(row[14]) !== 0;
            state.permission = Number(row[16]) !== 0;
            result = terrain.finish(query) ? 1 : 0;
            counts.F += 1;
        } else { result = terrain.regionAt(x, y); regionRows += 1; }
        const where = row.slice(0, 27).join(' ');
        assert.equal(result, Number(row[20]), where);
        assert.equal(state.bridgeOverlap, Number(row[21]) !== 0, where);
        assert.equal(state.noIslandOnly, Number(row[22]) !== 0, where);
        assert.equal(state.groundComplete, Number(row[23]) !== 0, where);
        assert.equal(state.permission, Number(row[24]) !== 0, where);
        assert.equal(state.canPlaceGround, Number(row[25]) !== 0, where);
        assert.equal(hex(state.regions), row[26], where);
        assert.equal(hex(pool.slot(SOURCE)), row[27], where);
    }
    // 모든 종류가 실제로 실행되었는지 확인한다.
    assert.ok(counts.B > 0 && counts.C > 0 && counts.E > 0 && counts.F > 0 && regionRows > 0, JSON.stringify(counts));
    assert.equal(counts.B + counts.C + counts.E + counts.F + regionRows, ROWS);
});

test('일반 배치 지형: 무권한 모양에서만 주변 조회를 하며 빈 디코더는 이전 배열을 유지한 채 최종 경계를 받는다', () => {
    let neighbors = 0;
    let finishes = 0;
    const query = { type: OWN, argument: 25, x: 20, y: 21, flags: 0, owner: 3, mode: 7 };
    const { pool, types, frames, state, terrain } = makeWorld({
        candidatePermission: () => false,
        shapePermission: (q, cell) => { assert.ok(q.argument === 25 && q.owner === 3 && q.mode === 7 && cell.label === 11); neighbors += 1; return true; },
        finishRelations: (_q, value) => { finishes += 1; return value.permission; }
    });
    types[OWN].group = 0;
    const cell = (label) => ({ frame: 0, label, side: 0, x: 20, y: 21, snappedX: 20, snappedY: 21, area: {} });
    terrain.begin(query);
    terrain.beginShape(query, cell(11));
    assert.equal(terrain.endShape(query), true);
    terrain.beginShape(query, cell(12));
    assert.equal(terrain.endShape(query), true);
    assert.equal(terrain.finish(query), true);
    assert.ok(neighbors === 1 && finishes === 1 && !state.groundComplete && state.permission);
    // 빈 디코더(기본 프레임 −1)는 모양 준비 없이 시작과 최종 경계만 받고 이전 배열을 그대로 둔다.
    frames[OWN].defaultFrame = -1;
    const geometry = new RawCanonPlacementGeometry(pool, types, frames, { patternTypes: Array(8).fill(0), decoderDiagnostics: null });
    state.regions.fill(33);
    types[OWN].group = 10;
    assert.equal(geometry.inspect({ type: OWN, argument: 25, direction: 0, x: 20, y: 21, explicitFrame: false }, () => assert.fail('칸이 없다'), makeCanonTerrainGeometryHooks(pool, terrain, query)), true);
    assert.ok(state.regions[0] === 33 && state.groundComplete && state.permission && neighbors === 1 && finishes === 2);
});

test('일반 배치 지형: 필수 경계·자료·풀·순회 순서를 거부하고 0 발자국과 다리 우선 지면 누적을 구분한다', () => {
    const { pool, types, frames, islands, state, terrain } = makeWorld();
    const otherPool = new SidPool(CAPACITY, { server: true });
    const query = { type: OWN, argument: 0, x: 20, y: 21, flags: 0, owner: 1, mode: 0 };
    const cell = { frame: 0, label: 0, side: 0, x: 20, y: 21, snappedX: 20, snappedY: 21, area: {} };
    assert.ok(throws(() => new RawCanonPlacementTerrain(pool, types, frames, islands, state, {})));
    assert.ok(throws(() => new RawCanonPlacementTerrain(pool, types, frames, new Uint16Array(10), state, denyHooks())));
    assert.ok(throws(() => terrain.endShape(query)));
    terrain.begin(query);
    terrain.beginShape(query, cell);
    assert.equal(terrain.endShape(query), true);
    assert.ok(!terrain.finish(query) && !state.groundComplete);
    // 다리 genus 의 배치 대상은 지면이 불완전해도 허용되고 권한은 시작에서 거부된다.
    types[OWN].flags2 = 4;
    terrain.begin(query);
    state.groundComplete = false;
    assert.ok(!terrain.finish(query) && state.canPlaceGround && !state.permission);
    assert.ok(throws(() => makeCanonTerrainGeometryHooks(otherPool, terrain, query)));
    assert.ok(throws(() => makeCanonTerrainCollisionHooks(otherPool, terrain, {})));
    assert.ok(typeof makeCanonTerrainCollisionHooks(pool, terrain, { keep: 1 }).inspectCandidateTerrain === 'function');
    assert.equal(makeCanonTerrainCollisionHooks(pool, terrain, { keep: 1 }).keep, 1);
    types[OWN].footX = 13;
    assert.ok(throws(() => terrain.begin(query)));
    types[OWN].footX = 0;
    terrain.begin(query);
    assert.ok(throws(() => terrain.beginShape(query, { ...cell, snappedX: Infinity })));
    assert.ok(throws(() => terrain.begin({ ...query, type: 188 })));
    // 지도 밖 좌표는 127 이고 SID 0 인 칸도 127 이다. 지역 genus 가 아닌 섬 SID 는 10.78 에서 거부한다.
    assert.equal(terrain.regionAt(-1, 0), 127);
    assert.equal(terrain.regionAt(256, 0), 127);
    assert.equal(terrain.regionAt(5, 5), 127);
    const raw = pool.physical(SOURCE);
    raw[10] = OTHER;
    islands[5 * 256 + 5] = SOURCE;
    types[OTHER].flags2 = 0;
    assert.ok(throws(() => terrain.regionAt(5, 5)));
});
