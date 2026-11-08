// 사제 배치 충돌 raw-priest-placement-collision.js(RawPriestPlacementCollision·미리보기 훅 어댑터)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestcollision-x86.tsv 의 `originals` 3840행 — 원본 무시 함수 반환(I: 반환 비트값 전체)과 한 후보 인라인 구간(C: 후보 결과·144바이트 미리보기 배열·후보 raw 슬롯 전체)이 같다.
//    사제 genus 9종×후보 7종×문맥×좌표×전역으로 실행한 관측이며 예측 가능한 클라이언트 SID 경계도 쓴다. `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 실제 finder 의 buried 제외·SID/전역 무시와 첫 거부 이후 탐색 중단, 후보 충돌 거부를 접두·미리보기·finder·사제 생성자에 연결해 다음 위치에서 생성, 필수 경계 누락·타입/연결 계약 위반, 무시한 후보도 지형 경계를 받고
//    finder 가 미리 저장한 next 를 쓰는 순서.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E157: 사제 배치 충돌 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { createPlacementState } from '../www/core/raw-canon-placement.js';
import { createCanonPlacementCollisionState, placementIgnoreValue } from '../www/core/raw-canon-placement-collision.js';
import { createCanonPlacementPreviewState } from '../www/core/raw-canon-placement-preview.js';
import { RawPriestPlacement, makePriestSpawnPlacementHooks } from '../www/core/raw-priest-placement.js';
import { RawPriestPlacementCollision, makePriestPlacementCollisionHooks } from '../www/core/raw-priest-placement-collision.js';
import { RawPriestPlacementPreview, makePriestPlacementPreviewHooks } from '../www/core/raw-priest-placement-preview.js';
import { PriestSpawnState, RawPriestSpawn } from '../www/core/raw-priest-spawn.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E157)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestcollision-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본 행 수, 풀 크기, 타입 수, 사제 타입 번호이다. (2026-11-09 WB04-E157)
const ROWS = 3840;
const CAPACITY = 24000;
const TYPE_COUNT = 188;
const PRIEST = 158;

// 예외가 나는지 본다. (2026-11-09 WB04-E157)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E157)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 새 타입 표이다. (2026-11-09 WB04-E157)
const makeTypes = () => Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0 }));

test('사제 배치 충돌: 원본 10.78 관측 3840행의 무시 함수 반환·후보 결과·미리보기 배열·후보 raw 불변성이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(allRows.length, 3 * ROWS);
    assert.equal(rows.length, ROWS);
    const pool = new SidPool(CAPACITY, { server: false });
    const hash = new SquidHash();
    const types = makeTypes();
    const state = createCanonPlacementCollisionState();
    const preview = createCanonPlacementPreviewState();
    const collision = new RawPriestPlacementCollision(pool, hash, types, state, preview, { inspectRegions: () => true, inspectCandidateTerrain: () => {} });
    const counts = { I: 0, C: 0, passed: 0, rejected: 0 };
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
        let result;
        if (row[1] === 'I') { result = placementIgnoreValue(own, other, mode, state.ignoredGenus); counts.I += 1; }
        else {
            result = collision.inspectCandidate({ type: 82, x: bitsToFloat(Number(row[10])), y: bitsToFloat(Number(row[11])), flags: 0, owner: 1, mode }, Number(row[9]) !== 0, sid) ? 1 : 0;
            counts.C += 1;
            if (result) counts.passed += 1; else counts.rejected += 1;
        }
        const where = row.slice(0, 20).join(' ');
        assert.equal(result >>> 0, Number(row[20]), where);
        assert.equal(hex(preview.blocked), row[21], where);
        assert.equal(hex(pool.slot(sid)), row[22], where);
    }
    // 두 종류와 통과/거부가 모두 실제로 관찰되었는지 확인한다.
    assert.ok(counts.I > 0 && counts.C > 0 && counts.passed > 0 && counts.rejected > 0, JSON.stringify(counts));
});

test('사제 배치 충돌: 실제 finder 의 buried 제외·SID/전역 무시와 첫 거부 이후 탐색 중단을 지킨다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const types = makeTypes();
    types[82].flags2 = 0x200000;
    types[82].footX = types[82].footY = 1;
    types[83].footX = types[83].footY = 1;
    const sid = pool.allocate();
    const raw = pool.allocated(sid);
    raw[10] = 83;
    put(raw, 14, floatToBits(20));
    put(raw, 18, floatToBits(21));
    put(raw, 4, 0xffff, 2);
    hash.setBucket(1, 20, 21, sid);
    const state = createCanonPlacementCollisionState();
    const preview = createCanonPlacementPreviewState();
    let requests = 0;
    const area = { left: 20, top: 21, right: 20, bottom: 21 };
    const collision = new RawPriestPlacementCollision(pool, hash, types, state, preview, {
        inspectRegions: (_query, scan) => { requests += 1; assert.equal(scan(area), false); assert.equal(scan(area), false); return true; },
        inspectCandidateTerrain: () => {}
    });
    assert.equal(collision.inspect({ type: 82, x: 20, y: 21, flags: 0, owner: 1, mode: 1 }, true), false);
    assert.ok(requests === 1 && preview.blocked[13] === 1);
    put(raw, 4, 0, 2);
    raw[40] = 8;
    preview.blocked.fill(0);
    assert.equal(collision.inspectArea({ type: 82, x: 20, y: 21, flags: 0, owner: 1, mode: 1 }, true, area), true);
    assert.equal(preview.blocked[13], 0);
    raw[40] = 0;
    state.ignoredGenus = 0x200000;
    types[83].flags2 = 0x8000;
    assert.equal(collision.inspectArea({ type: 82, x: 20, y: 21, flags: 0, owner: 1, mode: 1 }, false, area), true);
});

test('사제 배치 충돌: 후보 충돌 거부를 접두·미리보기·finder 에 연결하면 사제 생성이 다음 위치에서 이루어진다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const types = makeTypes();
    types[PRIEST].flags2 = 0x210000;
    types[PRIEST].footX = types[PRIEST].footY = 1;
    types[83].footX = types[83].footY = 1;
    const blocker = pool.allocate();
    const blockerRaw = pool.allocated(blocker);
    blockerRaw[10] = 83;
    put(blockerRaw, 14, floatToBits(21));
    put(blockerRaw, 18, floatToBits(22));
    hash.setBucket(1, 21, 22, blocker);
    const spots = new Uint8Array(65536).fill(6);
    const surfaces = new Uint16Array(65536);
    const previewState = createCanonPlacementPreviewState();
    const state = createCanonPlacementCollisionState();
    state.ignoredGenus = 0x200000;
    const placementState = createPlacementState();
    placementState.localPlayer = 1;
    placementState.blockedRelation = 0xffffffff;
    let scans = 0;
    let pops = 0;
    let born = 0;
    const cell = (query) => ({ x: Math.trunc(query.x), y: Math.trunc(query.y) });
    const collision = new RawPriestPlacementCollision(pool, hash, types, state, previewState, {
        inspectRegions: (query, scan) => { scans += 1; const { x, y } = cell(query); return scan({ left: x, top: y, right: x, bottom: y }); },
        inspectCandidateTerrain: () => {}
    });
    const preview = new RawPriestPlacementPreview(pool, types, spots, surfaces, previewState, makePriestPlacementCollisionHooks(pool, collision, {
        bounds: (query) => { const { x, y } = cell(query); return { left: x, top: y, right: x, bottom: y }; }
    }));
    const placement = new RawPriestPlacement(pool, types, placementState, makePriestPlacementPreviewHooks(pool, preview, { shape: () => ({ left: 0, top: 0, right: 16, bottom: 11 }) }));
    const spawn = new RawPriestSpawn(pool, types, spots, new PriestSpawnState(), makePriestSpawnPlacementHooks(pool, placement, {
        create: (type) => { born = pool.allocate(); pool.allocated(born)[10] = type; return born; },
        setOwner: () => {},
        pop: (sid, x, y, flags) => { assert.ok(sid === born && x === 21 && y === 21 && flags === 0); pops += 1; },
        notifySurface: () => assert.fail('다리 타입이 아니므로 표면 알림은 없다'),
        checkCarrier: (sid) => { assert.equal(sid, born); }
    }));
    spawn.spawn(20.75, 21.9, PRIEST, 0x8101);
    assert.ok(scans === 2 && pops === 1 && born === 15001 && get(pool.slot(born), 12, 2) === 0x8101);
});

test('사제 배치 충돌: 외부 경계 누락·타입/연결 계약 위반·비유한 좌표를 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const otherPool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const types = makeTypes();
    const state = createCanonPlacementCollisionState();
    const preview = createCanonPlacementPreviewState();
    assert.ok(throws(() => new RawPriestPlacementCollision(pool, hash, types, state, preview, {})));
    const collision = new RawPriestPlacementCollision(pool, hash, types, state, preview, { inspectRegions: () => false, inspectCandidateTerrain: () => {} });
    // 사제 genus 가 없으면 거부하고, 있으면 지역 경계의 거부를 그대로 돌려준다.
    assert.ok(throws(() => collision.inspect({ type: 82, x: 20, y: 21, flags: 0, owner: 1, mode: 0 }, true)));
    types[82].flags2 = 0x200000;
    assert.equal(collision.inspect({ type: 82, x: 20, y: 21, flags: 0, owner: 1, mode: 0 }, true), false);
    assert.ok(throws(() => collision.inspect({ type: 82, x: Infinity, y: 21, flags: 0, owner: 1, mode: 0 }, true)));
    assert.ok(throws(() => makePriestPlacementCollisionHooks(otherPool, collision, {})));
});

test('사제 배치 충돌: 충돌에서 무시한 후보도 지형 경계를 받고 finder 가 미리 저장한 next 를 쓴다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const types = makeTypes();
    types[82].flags2 = 0x210000;
    types[82].footX = types[82].footY = 1;
    types[83].footX = types[83].footY = 1;
    const first = pool.allocate();
    const second = pool.allocate();
    const firstRaw = pool.allocated(first);
    const secondRaw = pool.allocated(second);
    // 같은 위치의 두 후보를 동일 버킷 체인에 둔다.
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
    const collision = new RawPriestPlacementCollision(pool, hash, types, state, preview, {
        inspectRegions: (_query, scan) => scan({ left: 20, top: 21, right: 20, bottom: 21 }),
        inspectCandidateTerrain: (request, sid) => {
            assert.equal(request.mode, 0);
            visited.push(sid);
            if (sid === first) { put(firstRaw, 4, 0, 2); state.ignoredGenus = 0x200000; }
        }
    });
    assert.equal(collision.inspect({ type: 82, x: 20, y: 21, flags: 0, owner: 1, mode: 0 }, true), false);
    assert.deepEqual(visited, [first]);
    assert.equal(preview.blocked[13], 1);
    // 첫 후보의 mode=0 walker 무시는 지형 효과를 호출했다. 다음 후보는 현재 전역에 의해 실제 충돌로 거부했다.
    assert.ok(get(firstRaw, 4, 2) === 0 && state.ignoredGenus === 0x200000);
});
