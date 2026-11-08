// 일반 배치 모듈 사이의 통합 시험이다. 앞선 단위(E148 지형·E149 권한·E150 관계)가 충돌/미리보기/접두 이식 뒤로 미룬 cpppj 통합 시험을 옮겼다.
//  - 지형 통합(canon_terrain_pattern_preview_finder_and_permission_lifetime): 실제 받침 디코더·미리보기·finder 를 통과한 아홉 모양의 지역·권한·최종 경계(후보 권한 1회, 지면 종류가 맞지 않으면 후보 권한을 생략하고
//    모양마다 주변 경계, 충돌 거부는 지형·모양 종료·최종 처리에 도달하지 않음).
//  - 권한 통합(canon_permission_pattern_terrain_integration): 실제 3×3 디코더/미리보기/finder/지형에 권한 helper 를 연결해 모양 사이 누적을 검사(Player 조회 2회, 그래프 비활성이면 조회 없이 모양마다 주변 권한 경계).
//  - 관계 통합(canon_relations_normal_complete_pipeline·pattern_complete_pipeline): 일반 자산과 3×3 받침에서 픽셀 getter부터 디코더·finder·Player·주변/지형·최종 관계를 모두 실제 구현으로 연결하고
//    동맹/그래프/표면 소유자/지면 우회/편집기/특수 지역 제한 변화에 따른 허용·표시 차단·지면 허용을 검사한다.
// 모든 모듈이 실제 구현이며 정책 훅만 시험이 공급한다. 10.78 만 대상이다. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E154: 일반 배치 파이프라인 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { PlayerPlacementAnchorState, RawPlayerPlacementAnchor, makePlayerAnchorQuery } from '../www/core/raw-player-placement-anchor.js';
import { RawCanonPixelShape } from '../www/core/raw-canon-pixel-shape.js';
import { RawCanonPlacement, createPlacementState, makeCanonShapePlacementHooks } from '../www/core/raw-canon-placement.js';
import { RawCanonPlacementCollision, createCanonPlacementCollisionState, makeCanonGeometryCollisionHooks, makeCanonPlacementCollisionHooks } from '../www/core/raw-canon-placement-collision.js';
import { RawCanonPlacementGeometry } from '../www/core/raw-canon-placement-geometry.js';
import { RawCanonPlacementPermission, createCanonPlacementPermissionState, makeCanonPermissionTerrainHooks } from '../www/core/raw-canon-placement-permission.js';
import { RawCanonPlacementPreview, createCanonPlacementPreviewState, makeCanonGeometryPreviewHooks, makeCanonPlacementPreviewHooks } from '../www/core/raw-canon-placement-preview.js';
import { RawCanonPlacementRelations, createCanonPlacementRelationsState, makeCanonRelationsTerrainHooks } from '../www/core/raw-canon-placement-relations.js';
import { RawCanonPlacementSurrounding, makeCanonSurroundingTerrainHooks } from '../www/core/raw-canon-placement-surrounding.js';
import { RawCanonPlacementTerrain, createCanonPlacementTerrainState, makeCanonTerrainCollisionHooks, makeCanonTerrainGeometryHooks } from '../www/core/raw-canon-placement-terrain.js';

// 풀 크기, 타입 수, 시험용 타입 번호(배치 받침·후보 섬)이다. (2026-11-09 WB04-E154)
const CAPACITY = 24000;
const TYPE_COUNT = 188;
const ISLAND = 83;
const SUPPORT = 157;
const PATTERN_TYPES = [107, 82, 94, 157, 131, 129, 140, 142];

// 새 타입 표와 프레임 표이다. (2026-11-09 WB04-E154)
const makeTypes = () => Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0 }));
const makeFrames = () => Array.from({ length: TYPE_COUNT }, () => ({ frames: [], defaultFrame: 0 }));

// 받침 3×3 패턴이 쓰는 방향 A..I 의 변형 1 코드(또는 한 칸이면 A 하나)이다. (2026-11-09 WB04-E154)
function supportCodes(count) {
    const codes = [];
    // 방향 글자를 차례로 만든다.
    for (let side = 65; side < 65 + count; side += 1) codes.push({ side, variant: 80, number: 1, flags: 0 });
    return codes;
}

// 3×3 섬 칸을 만들어 해시 0단계와 고정 섬 지도(그리고 요청 시 현재 표면 지도)에 등록한다. 반환은 (20,21) 칸의 SID 이다. (2026-11-09 WB04-E154)
function placeIslands(pool, hash, islands, surfaces, options) {
    let source = 0;
    // 모양별로 별도의 한 칸 섬 SID 를 공급한다.
    for (let x = 20; x <= options.maxX; x += 1) {
        // 행마다 현재 raw 지역 번호와 프레임을 설정한다.
        for (let y = 21; y <= options.maxY; y += 1) {
            const sid = pool.allocate();
            const raw = pool.allocated(sid);
            raw[10] = ISLAND;
            raw[11] = 0;
            if (options.owner !== undefined) raw[34] = options.owner;
            if (options.graph !== undefined) raw[30] = options.graph;
            if (options.extra !== undefined) raw[40] = options.extra;
            put(raw, 8, 9, 2);
            put(raw, 14, floatToBits(x));
            put(raw, 18, floatToBits(y));
            put(raw, 36, 0, 4);
            hash.setBucket(0, x, y, sid);
            islands[y * 256 + x] = sid;
            if (surfaces) surfaces[y * 256 + x] = sid;
            if (x === 20 && y === 21) source = sid;
        }
    }
    return source;
}

// 충돌→미리보기→접두를 실제 모듈로 엮는다(지형·기하·모양 훅은 호출자가 공급). (2026-11-09 WB04-E154)
function assemble(pool, hash, types, frames, geometryState, terrain, previewSpots, surfaces, placementState, shapeHook) {
    const geometry = new RawCanonPlacementGeometry(pool, types, frames, geometryState);
    const collisionState = createCanonPlacementCollisionState();
    const previewState = createCanonPlacementPreviewState();
    const collision = new RawCanonPlacementCollision(pool, hash, types, collisionState, previewState, makeCanonGeometryCollisionHooks(pool, geometry,
        (query) => makeCanonTerrainGeometryHooks(pool, terrain, query), makeCanonTerrainCollisionHooks(pool, terrain, {})));
    const preview = new RawCanonPlacementPreview(pool, types, previewSpots, surfaces, previewState, makeCanonGeometryPreviewHooks(pool, geometry, makeCanonPlacementCollisionHooks(pool, collision, {})));
    return { geometry, collision, preview, placement: new RawCanonPlacement(pool, types, placementState, makeCanonPlacementPreviewHooks(pool, preview, { shape: shapeHook })) };
}

test('일반 배치 통합: 실제 받침 디코더·미리보기·finder 를 통과한 아홉 모양의 지역·권한·최종 경계(지형)', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const types = makeTypes();
    const frames = makeFrames();
    types[SUPPORT].group = 0;
    types[SUPPORT].flags2 = 0x10000;
    types[SUPPORT].flags1 = 2;
    types[SUPPORT].footX = types[SUPPORT].footY = 1;
    types[ISLAND].flags2 = 2;
    types[ISLAND].footX = types[ISLAND].footY = 1;
    frames[SUPPORT].frames = supportCodes(9);
    frames[ISLAND].frames = [{ side: 0, variant: 0, number: 0, flags: 0 }];
    const islands = new Uint16Array(65536);
    const surfaces = new Uint16Array(65536);
    const spots = new Uint8Array(65536).fill(6);
    placeIslands(pool, hash, islands, null, { maxX: 22, maxY: 23 });
    const state = createCanonPlacementTerrainState();
    state.noIslandType = ISLAND;
    const geometryState = { patternTypes: PATTERN_TYPES.slice(), decoderDiagnostics: null };
    const placementState = createPlacementState();
    placementState.localPlayer = 2;
    let grants = 0;
    let neighbors = 0;
    let finishes = 0;
    const terrain = new RawCanonPlacementTerrain(pool, types, frames, islands, state, {
        candidatePermission: (query, sid) => { assert.ok(query.owner === 2 && query.mode === 0xffffffff && query.argument === 0 && pool.slot(sid)[10] === ISLAND); grants += 1; return true; },
        shapePermission: () => { neighbors += 1; return false; },
        finishRelations: (query, value) => { assert.ok(query.owner === 2 && query.mode === 0xffffffff); finishes += 1; return value.permission && value.canPlaceGround; }
    });
    const { placement } = assemble(pool, hash, types, frames, geometryState, terrain, spots, surfaces, placementState, () => ({ left: 0, top: 0, right: 48, bottom: 33 }));
    const query = { type: SUPPORT, argument: 0, x: 20, y: 21, flags: 0, owner: 2, mode: 0xffffffff };
    assert.equal(placement.mayPlace(query), true);
    assert.ok(grants === 1 && neighbors === 0 && finishes === 1);
    assert.ok(state.permission && state.groundComplete && state.noIslandOnly && state.canPlaceGround && state.regions[0] === 9);
    // 지면 종류가 맞지 않으면 후보 권한 조회를 생략하고 매 모양의 주변 경계를 유지한다.
    types[SUPPORT].flags1 = 4;
    grants = neighbors = finishes = 0;
    assert.equal(placement.mayPlace(query), false);
    assert.ok(grants === 0 && neighbors === 9 && finishes === 1 && !state.permission && !state.groundComplete);
    // 충돌 거부는 후보 지형·모양 종료·최종 처리에 도달하지 않는다.
    types[ISLAND].flags2 = 0;
    grants = neighbors = finishes = 0;
    assert.equal(placement.mayPlace(query), false);
    assert.ok(grants === 0 && neighbors === 0 && finishes === 0);
});

test('일반 배치 통합: 실제 3×3 디코더·미리보기·finder·지형에 권한 helper 를 연결하고 모양 사이 누적을 검사한다(권한)', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const types = makeTypes();
    const frames = makeFrames();
    types[SUPPORT].group = 0;
    types[SUPPORT].flags2 = 0x10000;
    types[SUPPORT].flags1 = 2;
    types[SUPPORT].footX = types[SUPPORT].footY = 1;
    types[ISLAND].flags2 = 2;
    types[ISLAND].footX = types[ISLAND].footY = 1;
    frames[SUPPORT].frames = supportCodes(9);
    frames[ISLAND].frames = [{ side: 0, variant: 0, number: 0, flags: 0 }];
    const islands = new Uint16Array(65536);
    const surfaces = new Uint16Array(65536);
    const spots = new Uint8Array(65536).fill(6);
    placeIslands(pool, hash, islands, null, { maxX: 22, maxY: 23, owner: 2 });
    const permissionState = createCanonPlacementPermissionState();
    permissionState.graphReady = 1;
    let lookups = 0;
    let neighbors = 0;
    let finishes = 0;
    const permission = new RawCanonPlacementPermission(pool, permissionState, (player, type, x, y) => {
        assert.ok(player === 2 && type === ISLAND && x >= 20 && x <= 22 && y >= 21 && y <= 23);
        lookups += 1;
        return lookups === 1 ? 0 : 0xffffffff;
    });
    const state = createCanonPlacementTerrainState();
    state.noIslandType = ISLAND;
    const geometryState = { patternTypes: PATTERN_TYPES.slice(), decoderDiagnostics: null };
    const placementState = createPlacementState();
    placementState.localPlayer = 2;
    const terrain = new RawCanonPlacementTerrain(pool, types, frames, islands, state, makeCanonPermissionTerrainHooks(pool, permission, {
        shapePermission: (query) => { assert.ok(query.owner === 2 && query.mode === 0xffffffff); neighbors += 1; return false; },
        finishRelations: (_query, value) => { finishes += 1; return value.permission && value.canPlaceGround; }
    }));
    const { placement } = assemble(pool, hash, types, frames, geometryState, terrain, spots, surfaces, placementState, () => ({ left: 0, top: 0, right: 48, bottom: 33 }));
    const query = { type: SUPPORT, argument: 0, x: 20, y: 21, flags: 0, owner: 2, mode: 0xffffffff };
    assert.equal(placement.mayPlace(query), true);
    assert.ok(lookups === 2 && neighbors === 1 && finishes === 1 && state.permission && state.groundComplete && state.canPlaceGround);
    // 그래프 비활성 상태는 Player 조회 없이 각 모양의 주변 권한 경계까지 이어진다.
    permissionState.graphReady = 0;
    lookups = neighbors = finishes = 0;
    assert.equal(placement.mayPlace(query), false);
    assert.ok(lookups === 0 && neighbors === 9 && finishes === 1 && !state.permission);
});

// 일반 자산(80)과 3×3 받침(157)에 픽셀 getter부터 최종 관계까지 모두 실제 구현으로 연결한 완전 파이프라인 시험이다. (2026-11-09 WB04-E154)
function completePipeline(pattern) {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const types = makeTypes();
    const frames = makeFrames();
    const shapes = Array.from({ length: TYPE_COUNT }, () => ({ frameCount: 0, loaded: false, frames: [] }));
    const hotspots = Array.from({ length: TYPE_COUNT }, () => ({ x: 0, y: 0 }));
    const islands = new Uint16Array(65536);
    const surfaces = new Uint16Array(65536);
    const spots = new Uint8Array(65536).fill(6);
    const regions = Array.from({ length: 128 }, () => ({ restricted: 0, exists: 1 }));
    const placing = pattern ? SUPPORT : 80;
    types[placing].group = 0;
    types[placing].flags1 = 0x802;
    types[placing].flags2 = pattern ? 0x10000 : 0x4000;
    types[placing].footX = types[placing].footY = 1;
    types[ISLAND].flags1 = 0x800;
    types[ISLAND].flags2 = 2;
    types[ISLAND].footX = types[ISLAND].footY = 1;
    types[84].flags1 = 0x800;
    types[84].flags2 = 6;
    frames[placing].frames = supportCodes(pattern ? 9 : 1);
    frames[ISLAND].frames = [{ side: 80, variant: 80, number: 1, flags: 0 }];
    const count = frames[placing].frames.length;
    shapes[placing] = { frameCount: count, loaded: true, frames: Array.from({ length: count }, () => ({ width: 16, height: 11, hotspotX: 8, hotspotY: 5 })) };
    const source = placeIslands(pool, hash, islands, surfaces, { maxX: pattern ? 22 : 20, maxY: pattern ? 23 : 21, owner: 1, graph: 7, extra: 1 });
    const work = pool.allocate();
    const workshop = pool.allocated(work);
    workshop[10] = 84;
    workshop[34] = 1;
    workshop[30] = 7;
    const books = new SquidPostPopState();
    books.ownerFactories[1].entries = [work];
    books.ownerFactories[1].count = 1;
    const anchorState = new PlayerPlacementAnchorState();
    anchorState.graphReady = 1;
    const anchor = new RawPlayerPlacementAnchor(pool, types, surfaces, books, new SquidPostPopList(), new ContainedFinderState(), anchorState);
    const permissionState = createCanonPlacementPermissionState();
    permissionState.graphReady = 1;
    const permission = new RawCanonPlacementPermission(pool, permissionState, makePlayerAnchorQuery(pool, anchor));
    const placementState = createPlacementState();
    placementState.localPlayer = 1;
    const relationState = createCanonPlacementRelationsState();
    relationState.islandCount = 128;
    const relations = new RawCanonPlacementRelations(pool, types, surfaces, regions, relationState, placementState, permission);
    const surrounding = new RawCanonPlacementSurrounding(pool, types, frames, islands, spots, permission);
    const terrainState = createCanonPlacementTerrainState();
    terrainState.noIslandType = ISLAND;
    const terrain = new RawCanonPlacementTerrain(pool, types, frames, islands, terrainState,
        makeCanonRelationsTerrainHooks(pool, relations, makeCanonSurroundingTerrainHooks(pool, surrounding, makeCanonPermissionTerrainHooks(pool, permission, {}))));
    const geometryState = { patternTypes: PATTERN_TYPES.slice(), decoderDiagnostics: null };
    const geometry = new RawCanonPlacementGeometry(pool, types, frames, geometryState);
    const shape = new RawCanonPixelShape(pool, types, frames, shapes, hotspots, geometryState, { scaleX: 16, scaleY: 11 });
    const collisionState = createCanonPlacementCollisionState();
    const previewState = createCanonPlacementPreviewState();
    const collision = new RawCanonPlacementCollision(pool, hash, types, collisionState, previewState, makeCanonGeometryCollisionHooks(pool, geometry,
        (query) => makeCanonTerrainGeometryHooks(pool, terrain, query), makeCanonTerrainCollisionHooks(pool, terrain, {})));
    const preview = new RawCanonPlacementPreview(pool, types, spots, surfaces, previewState, makeCanonGeometryPreviewHooks(pool, geometry, makeCanonPlacementCollisionHooks(pool, collision, {})));
    const placement = new RawCanonPlacement(pool, types, placementState, makeCanonShapePlacementHooks(pool, shape, makeCanonPlacementPreviewHooks(pool, preview, {})));
    const query = { type: placing, argument: 0, x: 20, y: 21, flags: 0, owner: 1, mode: 0 };
    assert.ok(!placement.mayPlace(query) && terrainState.permission && terrainState.groundComplete && placementState.blockedRelation === 0);
    permissionState.alliances[10] = 1;
    assert.ok(placement.mayPlace(query));
    permissionState.useAlliances = 0;
    assert.ok(placement.mayPlace(query));
    permissionState.graphReady = 0;
    assert.ok(!placement.mayPlace(query));
    permissionState.graphReady = 1;
    // 현재 표면만 다른 소유자로 바꿔 고정 섬/Player 권한이 있어도 최종 관계가 거부함을 확인한다.
    const foreign = pool.allocate();
    const other = pool.allocated(foreign);
    other[10] = ISLAND;
    other[34] = 2;
    other[30] = 7;
    surfaces[21 * 256 + 20] = foreign;
    // 일반 0x4000 은 0x4200 표시 초기화 마스크에 포함되므로 내부 거부만 남는다.
    assert.ok(!placement.mayPlace(query) && placementState.blockedRelation === (pattern ? 1 : 0));
    relationState.bypassGroundPermission = 1;
    assert.ok(!placement.mayPlace(query));
    relationState.bypassGroundPermission = 0;
    permissionState.editor = 1;
    assert.ok(!placement.mayPlace(query) && placementState.blockedRelation === 0);
    permissionState.alliances[19] = 1;
    assert.ok(placement.mayPlace(query));
    permissionState.editor = 0;
    surfaces[21 * 256 + 20] = source;
    // 특수 지역은 10.78 에서 최종 지면을 취소한다.
    types[placing].flags2 = 0x200;
    regions[9].restricted = 1;
    assert.equal(placement.mayPlace(query), false);
    assert.equal(terrainState.canPlaceGround, false);
    regions[9].restricted = 0;
    assert.ok(placement.mayPlace(query));
}

test('일반 배치 통합: 일반 자산에 픽셀 getter 부터 최종 판정까지 모든 복원 단계를 연결한다(관계)', () => { completePipeline(false); });

test('일반 배치 통합: 3×3 받침의 실제 픽셀/디코더/finder/권한/지역과 원래 요청 좌표의 최종 판정을 연결한다(관계)', () => { completePipeline(true); });
