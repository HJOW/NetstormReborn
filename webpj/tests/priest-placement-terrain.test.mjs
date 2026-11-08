// 사제 배치 지형 raw-priest-placement-terrain.js(RawPriestPlacementTerrain·기하/충돌 훅 어댑터)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestterrain-x86.tsv 의 `originals` 2240행 — 후보 지형(C)·모양 종료(E)·최종 판정(F)·지역 조회(R)를 사제 배치 타입 flags1/후보 genus/프레임 지면 플래그/좌표/지역값/섬 지도 유무/배열 씨앗 6종/sentinel/초기 지면 완성 조합으로
//    실제 PE 에서 실행해 결과(반환값·다리 겹침·받침 전용·지면 완성·권한·지면 허용)와 144바이트 지역 배열, 후보 raw 슬롯 전체가 같다. `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 실제 사제 기하/finder 가 무시한 섬·다리도 지형 효과를 받고 사제의 최종 지면 우회를 보존, 유효 모양이 없으면 배열을 초기화하지 않음, 잘못된 순회/자료/타입/풀 거부,
//    사제 비패턴 모양/미리보기 범위 → finder → 사제 생성자 통합(E156 이월): 모양 시작·종료·최종 호출 수와 미리보기 16칸 표시.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E158: 사제 배치 지형 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { createPlacementState } from '../www/core/raw-canon-placement.js';
import { createCanonPlacementCollisionState } from '../www/core/raw-canon-placement-collision.js';
import { createCanonPlacementPreviewState } from '../www/core/raw-canon-placement-preview.js';
import { RawPriestPlacement, makePriestSpawnPlacementHooks } from '../www/core/raw-priest-placement.js';
import { RawPriestPlacementCollision, makePriestPlacementCollisionHooks } from '../www/core/raw-priest-placement-collision.js';
import { RawPriestPlacementGeometry, makePriestGeometryCollisionHooks, makePriestGeometryPreviewHooks } from '../www/core/raw-priest-placement-geometry.js';
import { RawPriestPlacementPreview, makePriestPlacementPreviewHooks } from '../www/core/raw-priest-placement-preview.js';
import { RawPriestPlacementShape, makePriestShapePlacementHooks } from '../www/core/raw-priest-placement-shape.js';
import { RawPriestPlacementTerrain, createPriestPlacementTerrainState, makePriestTerrainCollisionHooks, makePriestTerrainGeometryHooks } from '../www/core/raw-priest-placement-terrain.js';
import { PriestSpawnState, RawPriestSpawn } from '../www/core/raw-priest-spawn.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E158)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestterrain-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본 행 수, 풀 크기, 합성 배치/후보 타입, 후보 슬롯 SID, 타입 수, 사제 타입 번호이다. (2026-11-09 WB04-E158)
const ROWS = 2240;
const CAPACITY = 24000;
const OWN = 82;
const OTHER = 83;
const SOURCE = 50;
const TYPE_COUNT = 188;
const PRIEST = 158;

// 예외가 나는지 본다. (2026-11-09 WB04-E158)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E158)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 새 타입 표와 프레임 표이다. (2026-11-09 WB04-E158)
const makeTypes = () => Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0 }));
const makeFrames = () => Array.from({ length: TYPE_COUNT }, () => ({ frames: [], defaultFrame: 0 }));
const blankCodes = (count) => Array.from({ length: count }, () => ({ side: 0, variant: 0, number: 0, flags: 0 }));

// 원본 실행기의 입력 배열을 복구한다. 기대 배열은 fixture 에서 그대로 읽는다. (2026-11-09 WB04-E158)
function seed(state, seedNumber) {
    state.regions.fill([state.emptyRegion & 0xff, 255, 4, 127, 128, 128][seedNumber]);
    if (seedNumber === 3) state.regions[0] = 4;
    if (seedNumber === 5) state.regions[13] = 4;
}

test('사제 배치 지형: 원본 10.78 관측 2240행의 결과·지역 배열·후보 슬롯이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(allRows.length, 3 * ROWS);
    assert.equal(rows.length, ROWS);
    const pool = new SidPool(CAPACITY, { server: false });
    const types = makeTypes();
    const frames = makeFrames();
    const islands = new Uint16Array(65536);
    const state = createPriestPlacementTerrainState();
    const terrain = new RawPriestPlacementTerrain(pool, types, frames, islands, state);
    const counts = { C: 0, E: 0, F: 0, R: 0 };
    // 행마다 입력과 배열을 초기화하여 이전 순회 결과가 다음 행에 섞이지 않게 한다.
    for (const row of rows) {
        assert.equal(row.length, 26);
        const own = types[OWN];
        const other = types[OTHER];
        own.flags1 = Number(row[2]) >>> 0;
        own.flags2 = 0x210000;
        own.footX = Number(row[7]) | 0;
        own.footY = Number(row[8]) | 0;
        other.flags2 = Number(row[3]) >>> 0;
        other.footX = Number(row[9]) | 0;
        other.footY = Number(row[10]) | 0;
        frames[OTHER].frames = [{ side: 0, variant: 0, number: 0, flags: 0 }, { side: 0, variant: 0, number: 0, flags: Number(row[4]) & 0xff }];
        const raw = pool.physical(SOURCE);
        raw.fill(0xcd);
        raw[10] = OTHER;
        put(raw, 8, Number(row[13]), 2);
        put(raw, 14, Number(row[5]));
        put(raw, 18, Number(row[6]));
        raw[40] = Number(row[11]) & 0xff;
        put(raw, 36, 1, 4);
        islands.fill(0);
        const x = Math.trunc(bitsToFloat(Number(row[5])));
        const y = Math.trunc(bitsToFloat(Number(row[6])));
        if (x >= 0 && x < 256 && y >= 0 && y < 256) islands[y * 256 + x] = Number(row[14]) ? SOURCE : 0;
        state.emptyRegion = Number(row[16]) >>> 0;
        state.noIslandType = Number(row[12]) >>> 0;
        const query = { type: OWN, x: 20, y: 21, flags: 0, owner: 1, mode: 0 };
        terrain.begin(query);
        terrain.beginShape(query, 20, 21);
        seed(state, Number(row[15]));
        state.groundComplete = Number(row[17]) !== 0;
        let result = 0;
        if (row[1] === 'C') { terrain.candidate(query, SOURCE); counts.C += 1; }
        else if (row[1] === 'E') { assert.equal(terrain.endShape(query), true); counts.E += 1; }
        else if (row[1] === 'F') {
            state.bridgeOverlap = Number(row[11]) !== 0;
            state.noIslandOnly = Number(row[12]) !== 0;
            state.permission = Number(row[14]) !== 0;
            result = terrain.finish(query) ? 1 : 0;
            counts.F += 1;
        } else { result = terrain.regionAt(x, y); counts.R += 1; }
        const where = row.slice(0, 18).join(' ');
        assert.equal(result, Number(row[18]), where);
        assert.equal(state.bridgeOverlap, Number(row[19]) !== 0, where);
        assert.equal(state.noIslandOnly, Number(row[20]) !== 0, where);
        assert.equal(state.groundComplete, Number(row[21]) !== 0, where);
        assert.equal(state.permission, Number(row[22]) !== 0, where);
        assert.equal(state.canPlaceGround, Number(row[23]) !== 0, where);
        assert.equal(hex(state.regions), row[24], where);
        assert.equal(hex(pool.slot(SOURCE)), row[25], where);
    }
    // 네 종류가 모두 실제로 실행되었는지 확인한다.
    assert.ok(counts.C > 0 && counts.E > 0 && counts.F > 0 && counts.R > 0, JSON.stringify(counts));
});

test('사제 배치 지형: 실제 기하/finder 가 무시한 섬·다리도 지형 효과를 받고 사제의 최종 지면 우회를 보존한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const types = makeTypes();
    const frames = makeFrames();
    const islands = new Uint16Array(65536);
    types[OWN].flags2 = 0x210000;
    types[OWN].flags1 = 2;
    types[OWN].footX = types[OWN].footY = 1;
    types[OTHER].flags2 = 2;
    types[OTHER].footX = types[OTHER].footY = 1;
    frames[OWN].frames = blankCodes(1);
    frames[OTHER].frames = blankCodes(1);
    const island = pool.allocate();
    const raw = pool.allocated(island);
    raw[10] = OTHER;
    put(raw, 8, 9, 2);
    put(raw, 14, floatToBits(20));
    put(raw, 18, floatToBits(21));
    put(raw, 36, 0, 4);
    hash.setBucket(1, 20, 21, island);
    islands[21 * 256 + 20] = island;
    const terrainState = createPriestPlacementTerrainState();
    const geometryState = { patternTypes: Array(8).fill(0), decoderDiagnostics: null };
    const terrain = new RawPriestPlacementTerrain(pool, types, frames, islands, terrainState);
    const geometry = new RawPriestPlacementGeometry(pool, types, frames, geometryState, makePriestTerrainGeometryHooks(pool, terrain));
    const collision = new RawPriestPlacementCollision(pool, hash, types, createCanonPlacementCollisionState(), createCanonPlacementPreviewState(),
        makePriestGeometryCollisionHooks(pool, geometry, makePriestTerrainCollisionHooks(pool, terrain, {})));
    const query = { type: OWN, x: 20, y: 21, flags: 0, owner: 1, mode: 0 };
    assert.equal(collision.inspect(query, true), true);
    assert.ok(terrainState.regions[0] === 9 && terrainState.groundComplete && !terrainState.noIslandOnly && terrainState.canPlaceGround);
    // 허용 지면이 없어도 충돌 무시와 최종 사제 우회는 유지한다. flags1 0x400 은 권한 철회 효과를 가진다.
    types[OWN].flags1 = 0x400;
    assert.equal(collision.inspect(query, true), true);
    assert.ok(!terrainState.groundComplete && !terrainState.permission && !terrainState.canPlaceGround && terrainState.regions[0] === 127);
    types[OTHER].flags2 = 4;
    assert.equal(collision.inspect(query, true), true);
    assert.ok(terrainState.bridgeOverlap && terrainState.noIslandOnly && terrainState.canPlaceGround);
    raw[40] = 1;
    assert.equal(collision.inspect(query, true), true);
    assert.equal(terrainState.bridgeOverlap, false);
    // 유효 모양이 없으면 배열을 초기화하지 않고 초기 groundComplete=참으로 최종 사제 처리를 한다.
    frames[OWN].defaultFrame = -1;
    terrainState.regions.fill(33);
    assert.equal(collision.inspect(query, false), true);
    assert.ok(terrainState.regions[0] === 33 && terrainState.groundComplete);
});

test('사제 배치 지형: 잘못된 순회·자료·타입·프레임·풀 연결과 다리 assert 조건을 거부하고 모양 상태는 끈끈하다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const otherPool = new SidPool(CAPACITY, { server: true });
    const types = makeTypes();
    const frames = makeFrames();
    const islands = new Uint16Array(65536);
    const state = createPriestPlacementTerrainState();
    assert.ok(throws(() => new RawPriestPlacementTerrain(pool, types, frames, new Uint16Array(0), state)));
    const terrain = new RawPriestPlacementTerrain(pool, types, frames, islands, state);
    const query = { type: OWN, x: 20, y: 21, flags: 0, owner: 1, mode: 0 };
    assert.ok(throws(() => terrain.begin(query)));
    types[OWN].flags2 = 0x210000;
    types[OWN].footX = types[OWN].footY = 1;
    assert.ok(throws(() => terrain.candidate(query, SOURCE)));
    terrain.begin(query);
    assert.ok(throws(() => terrain.endShape(query)));
    assert.ok(throws(() => terrain.beginShape(query, Infinity, 21)));
    terrain.beginShape(query, 20, 21);
    assert.equal(terrain.endShape(query), true);
    assert.equal(state.groundComplete, false);
    // 한 모양에서 거짓이 된 지면 완성은 후속 모양에서 되돌리지 않는다.
    terrain.beginShape(query, 20, 21);
    state.regions.fill(9);
    assert.equal(terrain.endShape(query), true);
    assert.ok(!state.groundComplete && terrain.finish(query));
    assert.ok(throws(() => makePriestTerrainGeometryHooks(otherPool, terrain)));
    assert.ok(throws(() => makePriestTerrainCollisionHooks(otherPool, terrain, {})));
    types[OWN].flags2 |= 0x200;
    assert.ok(throws(() => terrain.begin(query)));
    types[OWN].flags2 = 0x210000;
    const sid = pool.allocate();
    pool.allocated(sid)[10] = OTHER;
    types[OTHER].flags2 = 4;
    types[OWN].footY = 2;
    terrain.begin(query);
    terrain.beginShape(query, 20, 21);
    assert.ok(throws(() => terrain.candidate(query, sid)));
    types[OWN].footY = 1;
    types[OTHER].flags2 = 2;
    terrain.begin(query);
    terrain.beginShape(query, 20, 21);
    assert.ok(throws(() => terrain.candidate(query, sid)));
    islands[0] = sid;
    types[OTHER].flags2 = 0;
    assert.ok(throws(() => terrain.regionAt(0, 0)));
});

test('사제 배치 통합: 비패턴 모양/미리보기 범위 → finder → 지형 → 사제 생성자를 모두 실제 구현으로 연결한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const types = makeTypes();
    const frames = makeFrames();
    types[PRIEST].flags2 = 0x210000;
    types[PRIEST].footX = types[PRIEST].footY = 1;
    types[OTHER].footX = types[OTHER].footY = 1;
    frames[PRIEST].frames = blankCodes(1);
    const blocker = pool.allocate();
    const blockerRaw = pool.allocated(blocker);
    blockerRaw[10] = OTHER;
    put(blockerRaw, 14, floatToBits(21));
    put(blockerRaw, 18, floatToBits(22));
    hash.setBucket(1, 21, 22, blocker);
    const spots = new Uint8Array(65536).fill(6);
    const surfaces = new Uint16Array(65536);
    const geometryState = { patternTypes: Array(8).fill(0), decoderDiagnostics: null };
    const previewState = createCanonPlacementPreviewState();
    const collisionState = createCanonPlacementCollisionState();
    collisionState.ignoredGenus = 0x200000;
    const placementState = createPlacementState();
    placementState.localPlayer = 1;
    placementState.blockedRelation = 0xffffffff;
    let begins = 0;
    let ends = 0;
    let finishes = 0;
    let pops = 0;
    let born = 0;
    const terrainState = createPriestPlacementTerrainState();
    const terrain = new RawPriestPlacementTerrain(pool, types, frames, surfaces, terrainState);
    const terrainHooks = makePriestTerrainGeometryHooks(pool, terrain);
    const wrapped = {
        beginRegions: (query) => { terrainHooks.beginRegions(query); begins += 1; },
        endShape: (query, frame, x, y) => { assert.ok(frame === 0 && x === 21 && y === 21); ends += 1; return terrainHooks.endShape(query, frame, x, y); },
        finishRegions: (query) => { finishes += 1; return terrainHooks.finishRegions(query); },
        beginShape: terrainHooks.beginShape
    };
    const geometry = new RawPriestPlacementGeometry(pool, types, frames, geometryState, wrapped);
    const collision = new RawPriestPlacementCollision(pool, hash, types, collisionState, previewState, makePriestGeometryCollisionHooks(pool, geometry, makePriestTerrainCollisionHooks(pool, terrain, {})));
    const preview = new RawPriestPlacementPreview(pool, types, spots, surfaces, previewState, makePriestGeometryPreviewHooks(pool, geometry, makePriestPlacementCollisionHooks(pool, collision, {})));
    const shapes = Array.from({ length: TYPE_COUNT }, () => ({ frameCount: 0, loaded: false, frames: [] }));
    const hotspots = Array.from({ length: TYPE_COUNT }, () => ({ x: 0, y: 0 }));
    shapes[PRIEST] = { frameCount: 1, loaded: true, frames: [{ width: 16, height: 11, hotspotX: 0, hotspotY: 0 }] };
    const shape = new RawPriestPlacementShape(pool, types, frames, shapes, hotspots, geometryState, { scaleX: 16, scaleY: 11 });
    const placement = new RawPriestPlacement(pool, types, placementState, makePriestShapePlacementHooks(pool, shape, makePriestPlacementPreviewHooks(pool, preview, {})));
    const spawn = new RawPriestSpawn(pool, types, spots, new PriestSpawnState(), makePriestSpawnPlacementHooks(pool, placement, {
        create: (type) => { born = pool.allocate(); pool.allocated(born)[10] = type; return born; },
        setOwner: () => {},
        pop: (sid, x, y, flags) => { assert.ok(sid === born && x === 21 && y === 21 && flags === 0); pops += 1; },
        notifySurface: () => assert.fail('다리 타입이 아니므로 표면 알림은 없다'),
        checkCarrier: (sid) => { assert.equal(sid, born); }
    }));
    spawn.spawn(20.75, 21.9, PRIEST, 0x8101);
    assert.ok(begins === 2 && ends === 1 && finishes === 1 && pops === 1 && born === 15001);
    assert.ok(get(pool.slot(born), 12, 2) === 0x8101 && previewState.blocked.filter((value) => value === 1).length === 16);
    assert.ok(!terrainState.groundComplete && !terrainState.canPlaceGround && terrainState.regions[0] === 127);
});
