// 일반 배치 주변 표면 순회 raw-canon-placement-surrounding.js(RawCanonSurfaceWalk·RawCanonPlacementSurrounding·지형 훅 어댑터)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: canonsurrounding-x86.tsv 의 `originals` 524행 — 모양 표면 finder 전체(W: 생성 직후부터 마지막 0 까지 모든 단계의 커서·중복 캐시 스냅샷)와 MayPlace 주변 구간(그 밖: 모양 종료의 최종 판정)을
//    모양 크기·좌표·프레임·타입 flags·표면 노드(소유자·좌표·프레임·상태)·섬 지도·spot 입력, 그래프/편집기/동맹/다른 소유자 허용 전역, Player 조회 응답·조회 중 변경 4종으로 실제 PE 에서 실행해
//    스냅샷 목록, 최종 판정, Player 조회 인자 목록, 128슬롯 raw·섬 지도·spot Adler-32 가 같다. `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 실제 Player 기준점 조회와 주변 표면을 통해 모양 종료의 권한 누적까지 연결, 실제 Squid 슬롯을 원천으로 하는 순회가 같은 입력의 배치 원천 순회와 같은 커서를 만듦, 필수 자료·풀·프레임·발자국 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E151: 일반 배치 주변 표면 순회 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { PlayerPlacementAnchorState, RawPlayerPlacementAnchor, makePlayerAnchorQuery } from '../www/core/raw-player-placement-anchor.js';
import { RawCanonPlacementTerrain, createCanonPlacementTerrainState } from '../www/core/raw-canon-placement-terrain.js';
import { RawCanonPlacementPermission, createCanonPlacementPermissionState, makeCanonPermissionTerrainHooks } from '../www/core/raw-canon-placement-permission.js';
import { RawCanonPlacementSurrounding, RawCanonSurfaceWalk, makeCanonSurroundingTerrainHooks } from '../www/core/raw-canon-placement-surrounding.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E151)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'canonsurrounding-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본 행 수, 풀 크기, native 실행기가 checksum 을 기록한 슬롯 수, 슬롯 폭, 타입 수이다. (2026-11-09 WB04-E151)
const ROWS = 524;
const CAPACITY = 24000;
const SLOTS = 128;
const STRIDE = 50;
const TYPE_COUNT = 188;

// 예외가 나는지 본다. (2026-11-09 WB04-E151)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 입력/기계어 관찰의 쉼표 목록을 숫자 배열로 읽는다. (2026-11-09 WB04-E151)
const numbers = (text) => text.split(',').map(Number);

// 프레임 코드 한 개를 만든다(방향 글자·변형 글자·번호·플래그). (2026-11-09 WB04-E151)
const code = (side, variant, number) => ({ side: side.charCodeAt(0), variant: variant.charCodeAt(0), number, flags: 0 });

// 새 타입 표와 프레임 표이다. (2026-11-09 WB04-E151)
const makeTypes = () => Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0 }));
const makeFrames = () => Array.from({ length: TYPE_COUNT }, () => ({ frames: [], defaultFrame: 0 }));

// 원본의 커서/중복 캐시 순서로 비교 가능한 목록을 만든다: 현재, x, y, right, top, left, bottom, 캐시 수, 캐시 목록. (2026-11-09 WB04-E151)
function snapshot(walk) {
    const state = walk.state;
    return [state.current, state.x, state.y, state.right, state.top, state.left, state.bottom, state.returned.length, ...state.returned].map((value) => value >>> 0);
}

test('일반 배치 주변 표면: 원본 10.78 관측 524행의 커서 스냅샷·최종 판정·Player 조회·자료 불변성이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(allRows.length, 3 * ROWS);
    assert.equal(rows.length, ROWS);
    const pool = new SidPool(CAPACITY, { server: false });
    const types = makeTypes();
    const frames = makeFrames();
    const islands = new Uint16Array(65536);
    const spots = new Uint8Array(65536);
    const mapBytes = new Uint8Array(islands.buffer, islands.byteOffset, 131072);
    const raw = new Uint8Array(SLOTS * STRIDE);
    let anchors = [];
    let selected = 0;
    let mutation = 0;
    // 미래 후보/지도만 바꾸고 이미 반환한 현재 번호와 캐시는 되돌리지 않는다.
    const mutate = () => {
        if (mutation === 1) pool.physical(53)[11] = 2;
        else if (mutation === 2) islands[22 * 256 + 20] = 0;
        else if (mutation === 3) pool.physical(53)[34] = 2;
        else if (mutation === 4) frames[82].frames = frames[83].frames = [code('P', 'P', 1), code('P', 'P', 2), code('J', 'P', 3)];
    };
    const permissionState = createCanonPlacementPermissionState();
    const permission = new RawCanonPlacementPermission(pool, permissionState, (owner, type, _x, _y, xBits, yBits) => {
        anchors.push([owner, type, xBits, yBits]);
        if (anchors.length === 1) mutate();
        return selected;
    });
    const surrounding = new RawCanonPlacementSurrounding(pool, types, frames, islands, spots, permission);
    const counts = { W: 0, other: 0, nonEmptyWalk: 0, anchored: 0 };
    // 원본 기계어에서 기록한 각 입력의 자료와 조회 상태를 준비한다.
    for (const row of rows) {
        assert.equal(row.length, 28);
        for (let sid = 0; sid < SLOTS; sid += 1) pool.physical(sid).fill(0);
        islands.fill(0);
        spots.fill(0);
        anchors = [];
        types[82].group = 0;
        types[82].footX = Number(row[2]) | 0;
        types[82].footY = Number(row[3]) | 0;
        types[82].flags1 = 0x800;
        types[82].flags2 = Number(row[7]) >>> 0;
        types[83].footX = types[83].footY = 1;
        types[83].flags1 = Number(row[9]) >>> 0;
        types[83].flags2 = Number(row[8]) >>> 0;
        frames[82].frames = frames[83].frames = [code('P', 'P', 1), code('A', 'B', 2), code('J', 'P', 3)];
        permissionState.graphReady = Number(row[10]) >>> 0;
        permissionState.editor = Number(row[11]) >>> 0;
        permissionState.useAlliances = Number(row[12]) >>> 0;
        permissionState.allowOtherOwners = Number(row[13]) >>> 0;
        permissionState.alliances.fill(Number(row[15]));
        selected = Number(row[16]) >>> 0;
        mutation = Number(row[18]);
        // 입력 슬롯의 필드를 실행기와 같은 순서로 쓴다.
        for (const part of row[19].split(';')) {
            const node = numbers(part);
            const slot = pool.physical(node[0]);
            slot[10] = 83;
            slot[11] = node[5];
            slot[34] = node[1];
            slot[40] = node[6];
            put(slot, 14, node[2]);
            put(slot, 18, node[3]);
            put(slot, 36, node[4], 4);
        }
        // 지도는 현재 등록 머리 번호를 그대로 보존한다.
        for (const part of row[20].split(';')) {
            const item = numbers(part);
            islands[item[1] * 256 + item[0]] = item[2];
        }
        if (row[21] !== '-') {
            // 기준점 및 전체 발자국의 내부 spot 입력을 옮긴다.
            for (const part of row[21].split(';')) {
                const item = numbers(part);
                spots[item[1] * 256 + item[0]] = item[2];
            }
        }
        const x = bitsToFloat(Number(row[4]));
        const y = bitsToFloat(Number(row[5]));
        const query = { type: 82, argument: 0, x, y, flags: 0, owner: Number(row[14]) >>> 0, mode: 0 };
        const cell = { frame: Number(row[6]) | 0, label: 0, side: 0, x, y, snappedX: Math.trunc(x), snappedY: Math.trunc(y), area: {} };
        const where = row.slice(0, 22).join(' ');
        if (row[1] === 'W') {
            const walk = RawCanonSurfaceWalk.forPlacement(pool, types, frames, islands, spots, query, cell);
            const snapshots = [snapshot(walk)];
            if (walk.current) mutate();
            // 마지막 0 관찰까지 비교하며 그 이후에는 원본 함수를 재호출하지 않는다.
            while (walk.current) {
                walk.next();
                snapshots.push(snapshot(walk));
            }
            const expected = row[23].split(';');
            assert.equal(snapshots.length, expected.length, where);
            // 행마다 실제 커서/캐시가 같아야 반환 SID 순서의 우연한 일치를 배제할 수 있다.
            for (let index = 0; index < snapshots.length; index += 1) assert.deepEqual(snapshots[index], numbers(expected[index]), `${where} #${index}`);
            counts.W += 1;
            if (snapshots.length > 1) counts.nonEmptyWalk += 1;
        } else {
            const state = createCanonPlacementTerrainState();
            const hooks = makeCanonSurroundingTerrainHooks(pool, surrounding, makeCanonPermissionTerrainHooks(pool, permission, { finishRelations: (_q, current) => current.permission }));
            const terrain = new RawCanonPlacementTerrain(pool, types, frames, islands, state, hooks);
            terrain.begin(query);
            terrain.beginShape(query, cell);
            state.permission = Number(row[17]) !== 0;
            terrain.endShape(query);
            assert.equal(terrain.finish(query), Number(row[22]) !== 0, where);
            counts.other += 1;
        }
        if (row[24] === '-') assert.equal(anchors.length, 0, where);
        else {
            const expected = row[24].split(';');
            assert.equal(anchors.length, expected.length, where);
            // 실제 Player 경계의 타입/owner/원본 좌표와 계속 순회한 호출 횟수를 확인한다.
            for (let index = 0; index < anchors.length; index += 1) assert.deepEqual(anchors[index], numbers(expected[index]), `${where} anchor#${index}`);
            counts.anchored += 1;
        }
        // 지도 번호를 리틀엔디언으로 만들어 전체 입력/변화의 checksum 도 비교한다.
        for (let sid = 0; sid < SLOTS; sid += 1) raw.set(pool.physical(sid), sid * STRIDE);
        assert.equal(adler32(raw), Number(row[25]), where);
        assert.equal(adler32(mapBytes), Number(row[26]), where);
        assert.equal(adler32(spots), Number(row[27]), where);
    }
    // 두 종류와 비어 있지 않은 순회, Player 조회 관찰이 실제로 실행되었는지 확인한다.
    assert.ok(counts.W > 0 && counts.other > 0 && counts.nonEmptyWalk > 0 && counts.anchored > 0, JSON.stringify(counts));
});

test('일반 배치 주변 표면: 실제 Player 기준점 조회와 주변 표면을 통해 모양 종료의 권한 누적까지 연결된다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = makeTypes();
    const frames = makeFrames();
    const islands = new Uint16Array(65536);
    const surfaces = new Uint16Array(65536);
    const spots = new Uint8Array(65536);
    types[82].group = 0;
    types[82].flags2 = 4;
    types[82].footX = types[82].footY = 1;
    types[83].flags1 = 0x800;
    types[83].flags2 = 2;
    types[83].footX = types[83].footY = 1;
    types[84].flags1 = 0x800;
    types[84].flags2 = 6;
    frames[82].frames = [code('A', 'B', 1)];
    frames[83].frames = [code('P', 'P', 1)];
    const island = pool.allocate();
    const workshop = pool.allocate();
    const land = pool.allocated(island);
    const work = pool.allocated(workshop);
    land[10] = 83;
    land[11] = 0;
    land[34] = 1;
    land[30] = 7;
    put(land, 14, floatToBits(20));
    put(land, 18, floatToBits(20));
    put(land, 36, 0);
    work[10] = 84;
    work[34] = 1;
    work[30] = 7;
    islands[20 * 256 + 20] = island;
    surfaces[20 * 256 + 20] = island;
    const books = new SquidPostPopState();
    books.ownerFactories[1].entries = [workshop];
    books.ownerFactories[1].count = 1;
    const anchorState = new PlayerPlacementAnchorState();
    anchorState.graphReady = 1;
    const anchor = new RawPlayerPlacementAnchor(pool, types, surfaces, books, new SquidPostPopList(), new ContainedFinderState(), anchorState);
    const permissions = createCanonPlacementPermissionState();
    permissions.graphReady = 1;
    const permission = new RawCanonPlacementPermission(pool, permissions, makePlayerAnchorQuery(pool, anchor));
    const surrounding = new RawCanonPlacementSurrounding(pool, types, frames, islands, spots, permission);
    const state = createCanonPlacementTerrainState();
    const terrain = new RawCanonPlacementTerrain(pool, types, frames, islands, state, makeCanonSurroundingTerrainHooks(pool, surrounding,
        makeCanonPermissionTerrainHooks(pool, permission, { finishRelations: (_q, current) => current.permission && current.canPlaceGround })));
    const query = { type: 82, argument: 0, x: 20, y: 21, flags: 0, owner: 1, mode: 0 };
    const cell = { frame: 0, label: 0, side: 0, x: 20, y: 21, snappedX: 20, snappedY: 21, area: {} };
    terrain.begin(query);
    terrain.beginShape(query, cell);
    assert.equal(state.permission, false);
    assert.ok(terrain.endShape(query) && state.permission && terrain.finish(query));
    // 현재 섬의 소유자를 중립으로 바꾸면 후보 권한은 작업장을 돌려주고 주변 권한은 바깥 관계에 막힌다.
    land[34] = 0;
    assert.equal(permission.candidate(island, 1), workshop);
    assert.equal(permission.surrounding(island, 1), 0);
    permissions.allowOtherOwners = 1;
    assert.equal(surrounding.inspect(query, cell), true);
    permissions.graphReady = 0;
    assert.equal(surrounding.inspect(query, cell), false);
});

test('일반 배치 주변 표면: 실제 Squid 슬롯을 원천으로 한 순회는 같은 입력의 배치 원천 순회와 같은 커서를 만든다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = makeTypes();
    const frames = makeFrames();
    const islands = new Uint16Array(65536);
    const spots = new Uint8Array(65536);
    types[82].footX = types[82].footY = 1;
    types[82].flags1 = 0x800;
    types[83].footX = types[83].footY = 1;
    types[83].flags1 = 0x800;
    types[83].flags2 = 2;
    // 'P' 코드는 연결 마스크가 0 이므로 서쪽으로 연결되는 'A' 코드를 쓴다.
    frames[82].frames = frames[83].frames = [code('A', 'A', 1)];
    const sourceSid = pool.allocate();
    const source = pool.allocated(sourceSid);
    source[10] = 82;
    put(source, 14, floatToBits(20));
    put(source, 18, floatToBits(21));
    put(source, 36, 0);
    const neighbor = pool.allocate();
    const raw = pool.allocated(neighbor);
    raw[10] = 83;
    put(raw, 14, floatToBits(21));
    put(raw, 18, floatToBits(21));
    put(raw, 36, 0);
    islands[21 * 256 + 21] = neighbor;
    const placement = RawCanonSurfaceWalk.forPlacement(pool, types, frames, islands, spots, { type: 82 }, { frame: 0, x: 20, y: 21 });
    const codes = frames.map((entry) => entry.frames);
    const actual = RawCanonSurfaceWalk.forSquid(pool, types, codes, islands, spots, sourceSid);
    // 이웃이 실제로 찾아져야 비교가 의미 있다.
    assert.equal(placement.current, neighbor);
    assert.deepEqual(actual.state, placement.state);
    assert.equal(actual.next(), 0);
    assert.equal(placement.next(), 0);
    assert.deepEqual(actual.state, placement.state);
});

test('일반 배치 주변 표면: 필수 자료·풀·프레임·발자국을 거부하고 암묵 허용하지 않는다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const other = new SidPool(CAPACITY, { server: true });
    const types = makeTypes();
    const frames = makeFrames();
    const islands = new Uint16Array(65536);
    const spots = new Uint8Array(65536);
    const permission = new RawCanonPlacementPermission(pool, createCanonPlacementPermissionState(), () => 0);
    const surrounding = new RawCanonPlacementSurrounding(pool, types, frames, islands, spots, permission);
    assert.ok(throws(() => makeCanonSurroundingTerrainHooks(other, surrounding, {})));
    assert.ok(throws(() => new RawCanonPlacementSurrounding(other, types, frames, islands, spots, permission)));
    const query = { type: 82, argument: 0, x: 20, y: 21, flags: 0, owner: 1, mode: 0 };
    const cell = { frame: 0, label: 0, side: 0, x: 20, y: 21, snappedX: 20, snappedY: 21, area: {} };
    // 발자국이 0 이면 거부하고, 프레임 코드가 비어 있어도 거부한다.
    assert.ok(throws(() => surrounding.inspect(query, cell)));
    types[82].footX = types[82].footY = 1;
    assert.ok(throws(() => surrounding.inspect(query, cell)));
    frames[82].frames = [code('P', 'P', 1)];
    assert.ok(throws(() => RawCanonSurfaceWalk.forPlacement(pool, types, frames, new Uint16Array(1), spots, query, cell)));
    // 좌표가 지도 밖이거나 비유한이면 거부한다.
    assert.ok(throws(() => RawCanonSurfaceWalk.forPlacement(pool, types, frames, islands, spots, query, { ...cell, x: Number.NaN })));
    assert.ok(throws(() => RawCanonSurfaceWalk.forPlacement(pool, types, frames, islands, spots, query, { ...cell, y: 256 })));
    // 한 개의 빈 모양은 후보가 없으면 권한을 얻지 못한다.
    assert.equal(surrounding.inspect(query, cell), false);
});
