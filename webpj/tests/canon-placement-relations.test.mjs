// 일반 배치 최종 관계 raw-canon-placement-relations.js(RawCanonPlacementRelations·지형 훅 어댑터)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: canonrelations-x86.tsv 의 `originals` 2307행 — 실제 MayPlace 최종 구간(표면 소유 관계·특수 섬 지역·최종 소유자 거부·표시 차단 전역)을 타입 genus/플래그, 이전 누적(지면 완성·받침 전용·다리 겹침·권한),
//    지면/권한 우회, 현재 표면·요청 소유자(부호 BYTE), 편집기/동맹/관계 표 값, 표면 SID·좌표, 지역 배열·sentinel·제한 값, 이전 표시 차단·이전 거부 입력으로 실제 PE 에서 실행해 최종 허용·권한·지면 허용·표시 차단 전역·
//    내부 거부·소유자 거부·이전 거부와 128슬롯 raw·표면 지도·관계 표 Adler-32 가 같다(자료 불변성 포함). 실제 E148 지형의 `begin`/`finish` 를 거쳐 원본 지면 누적 뒤 관계 모듈에 도달한다.
//    `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 필수 자료·풀·크기 거부, 없는 지역/개수 밖 지역/sentinel 의 지연 읽기 거부, 비유한 좌표 거부(항상 허용 genus 는 좌표를 읽지 않음), 표시 차단 초기화 마스크, 표 밖 소유자 진단.
//  - 이월: C++ 의 일반/패턴 완전 파이프라인 통합 시험(`canon_relations_normal_complete_pipeline`·`pattern_complete_pipeline`)은 충돌/미리보기/주변/배치 접두 모듈 이식 뒤 그 단위의 통합 시험에서 다룬다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E150: 일반 배치 최종 관계 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawCanonPlacementTerrain, createCanonPlacementTerrainState } from '../www/core/raw-canon-placement-terrain.js';
import { RawCanonPlacementPermission, createCanonPlacementPermissionState } from '../www/core/raw-canon-placement-permission.js';
import { RawCanonPlacementRelations, createCanonPlacementRelationsState, makeCanonRelationsTerrainHooks } from '../www/core/raw-canon-placement-relations.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E150)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'canonrelations-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본 행 수, 풀 크기, 원본 checksum 대상 슬롯 수와 지역 수, 타입 수, 슬롯 폭이다. (2026-11-09 WB04-E150)
const ROWS = 2307;
const CAPACITY = 24000;
const SLOTS = 128;
const REGIONS = 128;
const TYPE_COUNT = 188;
const STRIDE = 50;

// 예외가 나는지 본다. (2026-11-09 WB04-E150)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 새 타입 표와 프레임 표이다. (2026-11-09 WB04-E150)
const makeTypes = () => Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0 }));
const makeFrames = () => Array.from({ length: TYPE_COUNT }, () => ({ frames: [], defaultFrame: 0 }));

test('일반 배치 최종 관계: 원본 10.78 관측 2307행의 최종 허용·거부값·자료 불변성이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(allRows.length, 3 * ROWS);
    assert.equal(rows.length, ROWS);
    const pool = new SidPool(CAPACITY, { server: false });
    const types = makeTypes();
    const frames = makeFrames();
    const surfaces = new Uint16Array(65536);
    const islands = new Uint16Array(65536);
    const regions = Array.from({ length: REGIONS }, () => ({ restricted: 0, exists: 1 }));
    const mapBytes = new Uint8Array(surfaces.buffer, surfaces.byteOffset, 131072);
    const relationBytes = new Uint8Array(324);
    const raw = new Uint8Array(SLOTS * STRIDE);
    const permissionState = createCanonPlacementPermissionState();
    const permission = new RawCanonPlacementPermission(pool, permissionState, () => assert.fail('권한 조회는 호출되지 않는다'));
    const state = createCanonPlacementRelationsState();
    state.islandCount = REGIONS;
    const placement = { blockedRelation: 0 };
    const relations = new RawCanonPlacementRelations(pool, types, surfaces, regions, state, placement, permission);
    let allowedRows = 0;
    let rejectedRows = 0;
    // 같은 raw·지도·관계 표·이전 누적 상태를 각 독립 관찰 행에 공급한다.
    for (const row of rows) {
        assert.equal(row.length, 31);
        // 처음 128슬롯 전체를 0xcd 로 채워 원본 checksum 대상과 같은 입력을 만든다.
        for (let sid = 0; sid < SLOTS; sid += 1) pool.physical(sid).fill(0xcd);
        pool.physical(50)[34] = Number(row[8]) & 0xff;
        pool.physical(51)[34] = (Number(row[8]) + 1) % 3;
        const surface = Number(row[13]) & 0xffff;
        surfaces.fill(surface);
        surfaces[21 * 256 + 20] = surface;
        surfaces[21 * 256 + 21] = surface === 50 ? 51 : surface;
        surfaces[22 * 256 + 20] = surface === 50 ? 51 : surface;
        surfaces[22 * 256 + 21] = surface;
        types[82].flags2 = Number(row[1]) >>> 0;
        types[82].flags1 = Number(row[2]) >>> 0;
        types[82].footX = types[82].footY = 1;
        permissionState.editor = Number(row[10]) >>> 0;
        permissionState.useAlliances = Number(row[11]) >>> 0;
        permissionState.alliances.fill(0);
        // 실제 한 방향 칸만 채워 전치 표와 보정 후 다른 표면의 소유자를 구별한다.
        const owner = ((Number(row[9]) << 24) >> 24) >>> 0;
        const index = ((Number(row[8]) * 9) + owner) >>> 0;
        if (index < permissionState.alliances.length) permissionState.alliances[index] = Number(row[12]);
        state.bypassGroundPermission = Number(row[7]) >>> 0;
        placement.blockedRelation = Number(row[19]) >>> 0;
        // 실제 특수 지역 레코드의 제한/존재 값만 명시적으로 입력한다.
        for (const region of regions) { region.restricted = Number(row[18]) >>> 0; region.exists = 1; }
        const query = { type: 82, argument: 0, x: bitsToFloat(Number(row[14])), y: bitsToFloat(Number(row[15])), flags: 0, owner: Number(row[9]) >>> 0, mode: 0 };
        const terrain = createCanonPlacementTerrainState();
        terrain.emptyRegion = Number(row[17]) >>> 0;
        terrain.regions.fill(Number(row[16]) & 0xff);
        let observed = null;
        // 기존 지면 누적을 실제 finish 로 수행한 뒤 원본 최종 지역 변수 입력을 관계 모듈에 전달한다.
        const pipeline = new RawCanonPlacementTerrain(pool, types, frames, islands, terrain, {
            candidatePermission: () => assert.fail('후보 권한은 호출되지 않는다'),
            shapePermission: () => assert.fail('주변 권한은 호출되지 않는다'),
            finishRelations: (q, current) => { observed = relations.inspect(q, current, Number(row[20])); return observed.allowed; }
        });
        pipeline.begin(query);
        // begin 의 초기값 대신 실제 앞부분 누적 상태를 회복하고 빈 모양의 finish 만 호출한다.
        terrain.groundComplete = Number(row[3]) !== 0;
        terrain.noIslandOnly = Number(row[4]) !== 0;
        terrain.bridgeOverlap = Number(row[5]) !== 0;
        terrain.permission = Number(row[6]) !== 0;
        const allowed = pipeline.finish(query);
        // 물리 지도/관계 표도 원본 checksum 과 대조해 입력 자료의 뜻하지 않은 변경을 찾는다.
        for (let sid = 0; sid < SLOTS; sid += 1) raw.set(pool.physical(sid), sid * STRIDE);
        // 직접 DWORD 관계 표의 행 방향과 비트값을 유지한다.
        for (let item = 0; item < permissionState.alliances.length; item += 1) put(relationBytes, 4 * item, permissionState.alliances[item]);
        const where = row.slice(0, 22).join(' ');
        assert.equal(allowed, Number(row[21]) !== 0, where);
        assert.equal(terrain.permission, Number(row[22]) !== 0, where);
        assert.equal(terrain.canPlaceGround, Number(row[23]) !== 0, where);
        assert.equal(placement.blockedRelation >>> 0, Number(row[24]), where);
        assert.equal(observed.relationRejected, Number(row[25]) !== 0, where);
        assert.equal(observed.foreignRejected >>> 0, Number(row[26]), where);
        assert.equal(observed.priorRejection >>> 0, Number(row[27]), where);
        assert.equal(adler32(raw), Number(row[28]), where);
        assert.equal(adler32(mapBytes), Number(row[29]), where);
        assert.equal(adler32(relationBytes), Number(row[30]), where);
        if (allowed) allowedRows += 1; else rejectedRows += 1;
    }
    // 허용과 거부가 모두 실제로 관찰되었는지 확인한다.
    assert.ok(allowedRows > 0 && rejectedRows > 0, `${allowedRows}/${rejectedRows}`);
});

test('일반 배치 최종 관계: 필수 자료·풀·크기를 거부하고 지역/표면 읽기는 필요할 때만 한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const other = new SidPool(CAPACITY, { server: true });
    const types = makeTypes();
    const surfaces = new Uint16Array(65536);
    const regions = Array.from({ length: 10 }, () => ({ restricted: 0, exists: 1 }));
    const state = createCanonPlacementRelationsState();
    state.islandCount = 10;
    const placement = { blockedRelation: 0 };
    const permissions = createCanonPlacementPermissionState();
    const permission = new RawCanonPlacementPermission(pool, permissions, () => assert.fail('권한 조회는 호출되지 않는다'));
    const relations = new RawCanonPlacementRelations(pool, types, surfaces, regions, state, placement, permission);
    const terrain = createCanonPlacementTerrainState();
    terrain.groundComplete = terrain.permission = terrain.canPlaceGround = true;
    terrain.regions.fill(9);
    const query = { type: 82, argument: 0, x: 20, y: 21, flags: 0, owner: 1, mode: 0 };
    assert.ok(throws(() => makeCanonRelationsTerrainHooks(other, relations, {})));
    assert.ok(throws(() => new RawCanonPlacementRelations(other, types, surfaces, regions, state, placement, permission)));
    assert.ok(throws(() => new RawCanonPlacementRelations(pool, types, new Uint16Array(0), regions, state, placement, permission)));
    // 존재하지 않는 지역과 개수 밖 지역은 임의 허용/0 으로 대신하지 않고 거부한다.
    types[82].flags2 = 0x200;
    regions[9].exists = 0;
    assert.ok(throws(() => relations.inspect(query, terrain)));
    regions[9].exists = 1;
    state.islandCount = 9;
    assert.ok(throws(() => relations.inspect(query, terrain)));
    state.islandCount = 10;
    // sentinel 과 같은 지역 번호는 지역 표를 읽지 않으나 다르면 읽는다.
    terrain.regions.fill(255);
    terrain.emptyRegion = 0xffffffff;
    assert.equal(relations.inspect(query, terrain).allowed, true);
    terrain.emptyRegion = 127;
    assert.ok(throws(() => relations.inspect(query, terrain)));
    // 항상 허용 genus 는 표면 좌표를 읽지 않는다. 직접 조회는 비유한 좌표를 거부한다.
    types[82].flags2 = 0x200000;
    const bad = { ...query, x: Number.NaN, y: Infinity };
    assert.equal(relations.inspect(bad, terrain, 99).allowed, true);
    assert.equal(relations.inspect(bad, terrain, 99).foreignRejected, 99);
    assert.ok(throws(() => relations.surfaceAt(bad.x, bad.y)));
    // 표시 차단 초기화 genus 는 내부 거부만 남기고 표시 전역은 0 이다.
    types[82].flags2 = 0x4200;
    terrain.regions.fill(9);
    placement.blockedRelation = 7;
    assert.equal(relations.inspect(query, terrain).allowed, false);
    assert.equal(placement.blockedRelation, 0);
    // 표 밖 소유자는 편집기가 켜져도 최종 소유자 거부에서 진단한다.
    types[82].flags2 = 0;
    const surface = pool.allocate();
    pool.allocated(surface)[34] = 255;
    surfaces[21 * 256 + 20] = surface;
    permissions.editor = 1;
    assert.ok(throws(() => relations.inspect(query, terrain)));
    // 지도 밖 좌표는 SID 0 이다.
    assert.equal(relations.surfaceAt(-5, 21), 0);
    assert.equal(relations.surfaceAt(300, 21), 0);
    assert.equal(relations.surfaceAt(20, 21), surface);
});

test('일반 배치 최종 관계: 지형 훅 어댑터는 최종 관계 경계만 바꾸고 다른 훅을 유지한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = makeTypes();
    const surfaces = new Uint16Array(65536);
    const regions = [{ restricted: 0, exists: 1 }];
    const permission = new RawCanonPlacementPermission(pool, createCanonPlacementPermissionState(), () => 0);
    const relations = new RawCanonPlacementRelations(pool, types, surfaces, regions, createCanonPlacementRelationsState(), { blockedRelation: 0 }, permission);
    const kept = () => true;
    const hooks = makeCanonRelationsTerrainHooks(pool, relations, { candidatePermission: kept, shapePermission: kept });
    assert.equal(hooks.candidatePermission, kept);
    assert.equal(hooks.shapePermission, kept);
    // genus 0 이고 표면이 없으면 권한과 지면 허용에 따라 결정한다.
    const terrain = createCanonPlacementTerrainState();
    const query = { type: 82, argument: 0, x: 20, y: 21, flags: 0, owner: 1, mode: 0 };
    assert.equal(hooks.finishRelations(query, terrain), false);
    terrain.permission = terrain.canPlaceGround = true;
    assert.equal(hooks.finishRelations(query, terrain), true);
});
