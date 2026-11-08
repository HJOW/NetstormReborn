// 일반 배치 전체 파이프라인 raw-canon-placement-pipeline.js(RawCanonPlacementPipeline)를 원본 10.78 실제 PE 의 전체 MayPlace 관측과 대조한다.
//  - 원본 관측: canonmayplace-x86.tsv 의 `originals` 144행 — 전체 MayPlace(진입 → 실제 helper → 정상 반환)를 모양(일반/패턴)·인자·방향·좌표·타입 genus/flags·장면 6종(기본·작업장 없음·다른 소유자 섬·섬 칸 빠짐·장애물·빈 지도)·
//    소유자·로컬 플레이어·그래프/편집기/동맹/관계 값·지면 우회·제한 값·모드·강제 허용으로 실행해 허용 결과, 표시 차단 전역, 144바이트 미리보기 배열, 144바이트 지역 배열, 128슬롯 raw, 해시 네 단계 전체,
//    고정 섬 지도의 Adler-32 가 같다. 접두·미리보기·기하·충돌·지형·권한·관계·주변·Player 기준점·픽셀 모양이 한 저장소를 공유한다. `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 현재 표면이 해시 0단계와 같은 저장소를 공유(해시 쓰기가 표면 판정에 즉시 반영), 공통 전역(편집기·동맹·그래프·noIsland)을 진입마다 현재 값으로 맞춤, 자료/상태 누락 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E154: 일반 배치 파이프라인 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawCanonPlacementPipeline, createCanonPlacementPipelineState } from '../www/core/raw-canon-placement-pipeline.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E154)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'canonmayplace-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본 행 수, 풀 크기, checksum 대상 슬롯 수와 슬롯 폭, 타입 수이다. (2026-11-09 WB04-E154)
const ROWS = 144;
const CAPACITY = 24000;
const SLOTS = 128;
const STRIDE = 50;
const TYPE_COUNT = 188;

// 예외가 나는지 본다. (2026-11-09 WB04-E154)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E154)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 새 타입 표와 프레임 표이다. (2026-11-09 WB04-E154)
const makeTypes = () => Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0 }));
const makeFrames = () => Array.from({ length: TYPE_COUNT }, () => ({ frames: [], defaultFrame: 0 }));

// 한 가지 입력 구성(풀·해시·표·상태·파이프라인)을 만든다. (2026-11-09 WB04-E154)
function makeScene() {
    const pool = new SidPool(CAPACITY, { server: false });
    const hash = new SquidHash();
    const types = makeTypes();
    const frames = makeFrames();
    const shapes = Array.from({ length: TYPE_COUNT }, () => ({ frameCount: 0, loaded: false, frames: [] }));
    const hotspots = Array.from({ length: TYPE_COUNT }, () => ({ x: 0, y: 0 }));
    const islands = new Uint16Array(65536);
    const spots = new Uint8Array(65536).fill(6);
    const regions = Array.from({ length: 128 }, () => ({ restricted: 0, exists: 1 }));
    const state = createCanonPlacementPipelineState();
    state.geometry.patternTypes = [107, 82, 94, 157, 131, 129, 140, 142];
    state.relations.islandCount = 128;
    const books = new SquidPostPopState();
    const additional = new SquidPostPopList();
    const contained = new ContainedFinderState();
    const pipeline = new RawCanonPlacementPipeline(pool, hash, { types, frames, shapes, hotspots, spots, islands, regions }, books, additional, contained, state);
    return { pool, hash, types, frames, shapes, hotspots, islands, spots, regions, state, books, pipeline };
}

test('일반 배치 파이프라인: 원본 10.78 전체 MayPlace 관측 144행의 허용·전역·배열·checksum 이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(allRows.length, 3 * ROWS);
    assert.equal(rows.length, ROWS);
    const { pool, hash, types, frames, shapes, islands, regions, state, books, pipeline } = makeScene();
    const raw = new Uint8Array(SLOTS * STRIDE);
    const fixedBytes = new Uint8Array(islands.buffer, islands.byteOffset, 131072);
    const counts = { allowed: 0, rejected: 0, pattern: 0 };
    // 같은 슬롯/타입/목록/공간 자료를 원본 입력 순서로 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 26);
        for (let sid = 0; sid < SLOTS; sid += 1) pool.physical(sid).fill(0);
        hash.reset();
        islands.fill(0);
        const pattern = Number(row[1]) !== 0;
        const placing = pattern ? 157 : 80;
        types[placing].group = 0;
        types[placing].flags1 = Number(row[7]) >>> 0;
        types[placing].flags2 = Number(row[6]) >>> 0;
        types[placing].footX = types[placing].footY = 1;
        types[83].flags1 = 0x800;
        types[83].flags2 = 2;
        types[83].footX = types[83].footY = 1;
        types[84].flags1 = 0x800;
        types[84].flags2 = 6;
        types[84].footX = types[84].footY = 1;
        types[86].flags1 = 2;
        types[86].flags2 = 0;
        types[86].footX = types[86].footY = 1;
        const codes = [];
        // 정상 원본 받침/일반 자산과 동일한 논리 프레임 코드를 넣는다.
        for (let side = 65; side <= (pattern ? 73 : 65); side += 1) codes.push({ side, variant: 80, number: 1, flags: 0 });
        frames[placing].frames = codes;
        frames[83].frames = [{ side: 80, variant: 80, number: 1, flags: 0 }];
        shapes[placing] = { frameCount: codes.length, loaded: true, frames: codes.map(() => ({ width: 16, height: 11, hotspotX: 8, hotspotY: 5 })) };
        const scene = Number(row[8]);
        const owner = Number(row[9]);
        let sid = 50;
        // 원본 장면과 같은 6×6 칸의 raw/고정 섬/현재 해시를 준비한다.
        for (let y = 19; y < 25; y += 1) {
            // 현재 표면과 finder 의 0단계는 같은 hash 저장소에 쓴다.
            for (let x = 18; x < 24; x += 1) {
                const slot = pool.physical(sid);
                slot[10] = 83;
                slot[34] = scene === 2 ? 2 : 1;
                slot[30] = 7;
                slot[40] = 1;
                put(slot, 8, 9, 2);
                put(slot, 14, floatToBits(x));
                put(slot, 18, floatToBits(y));
                if (scene !== 3 || x !== 20 || y !== 21) {
                    hash.setBucket(0, x, y, sid);
                    islands[y * 256 + x] = sid;
                }
                sid += 1;
            }
        }
        const work = pool.physical(90);
        work[10] = 84;
        work[34] = 1;
        work[30] = 7;
        put(work, 14, floatToBits(18));
        put(work, 18, floatToBits(20));
        books.ownerFactories[owner].entries = [90];
        books.ownerFactories[owner].count = scene === 1 ? 0 : 1;
        if (scene === 4) {
            const obstacle = pool.physical(91);
            obstacle[10] = 86;
            put(obstacle, 14, floatToBits(20));
            put(obstacle, 18, floatToBits(21));
            hash.setBucket(1, 20, 21, 91);
        }
        if (scene === 5) {
            hash.reset();
            islands.fill(0);
        }
        state.placement.forcePlacement = Number(row[18]) >>> 0;
        state.placement.localPlayer = Number(row[10]) | 0;
        state.placement.blockedRelation = 73;
        state.permission.graphReady = Number(row[11]) >>> 0;
        state.permission.editor = Number(row[12]) >>> 0;
        state.permission.useAlliances = Number(row[13]) >>> 0;
        state.permission.alliances.fill(0);
        state.permission.alliances[9 + owner] = state.permission.alliances[18 + owner] = Number(row[14]) >>> 0;
        state.relations.bypassGroundPermission = Number(row[15]) >>> 0;
        state.preview.blocked.fill(0x55);
        state.terrain.regions.fill(0x55);
        // 패치 전용 목록의 제한 값과 정상 존재 비트도 원본과 같다.
        for (const region of regions) { region.restricted = Number(row[16]) >>> 0; region.exists = 1; }
        const query = { type: placing, argument: Number(row[2]) >>> 0, x: bitsToFloat(Number(row[4])), y: bitsToFloat(Number(row[5])), flags: Number(row[3]) >>> 0, owner, mode: Number(row[17]) >>> 0 };
        const allowed = pipeline.mayPlace(query);
        // 네 단계 배열을 원본 메모리 순서로 직렬화한다.
        const hashBytes = [];
        for (let level = 0; level < 4; level += 1) {
            // 각 머리 WORD 의 물리 바이트를 보존한다.
            for (const head of hash.entries(level)) hashBytes.push(head & 0xff, head >> 8);
        }
        for (let sid2 = 0; sid2 < SLOTS; sid2 += 1) raw.set(pool.physical(sid2), sid2 * STRIDE);
        const where = row.slice(0, 19).join(' ');
        assert.equal(allowed, Number(row[19]) !== 0, where);
        assert.equal(state.placement.blockedRelation >>> 0, Number(row[20]), where);
        assert.equal(hex(state.preview.blocked), row[21], where);
        assert.equal(hex(state.terrain.regions), row[22], where);
        assert.equal(adler32(raw), Number(row[23]), where);
        assert.equal(adler32(Uint8Array.from(hashBytes)), Number(row[24]), where);
        assert.equal(adler32(fixedBytes), Number(row[25]), where);
        if (allowed) counts.allowed += 1; else counts.rejected += 1;
        if (pattern) counts.pattern += 1;
    }
    // 허용·거부·패턴 장면이 모두 실제로 관찰되었는지 확인한다.
    assert.ok(counts.allowed > 0 && counts.rejected > 0 && counts.pattern > 0, JSON.stringify(counts));
});

test('일반 배치 파이프라인: 현재 표면은 해시 0단계와 같은 저장소를 공유하고 공통 전역을 진입마다 현재 값으로 맞춘다', () => {
    const { pool, hash, types, frames, shapes, islands, state, books, pipeline } = makeScene();
    // 일반 자산 한 칸짜리 배치와 섬 후보 한 칸을 준비한다.
    types[80].flags1 = 2;
    types[80].footX = types[80].footY = 1;
    types[83].flags1 = 0x800;
    types[83].flags2 = 2;
    types[83].footX = types[83].footY = 1;
    frames[80].frames = [{ side: 65, variant: 80, number: 1, flags: 0 }];
    frames[83].frames = [{ side: 80, variant: 80, number: 1, flags: 0 }];
    shapes[80] = { frameCount: 1, loaded: true, frames: [{ width: 16, height: 11, hotspotX: 8, hotspotY: 5 }] };
    const island = pool.physical(60);
    island[10] = 83;
    island[34] = 1;
    island[30] = 7;
    island[40] = 1;
    put(island, 14, floatToBits(20));
    put(island, 18, floatToBits(21));
    islands[21 * 256 + 20] = 60;
    books.ownerFactories[1].entries = [];
    books.ownerFactories[1].count = 0;
    // 공통 전역: 권한 쪽 값만 바꾸고 진입 뒤 다른 모듈의 값이 같아졌는지 본다.
    state.permission.editor = 1;
    state.permission.useAlliances = 1;
    state.permission.alliances[5] = 7;
    state.permission.graphReady = 1;
    state.geometry.patternTypes[3] = 157;
    state.placement.localPlayer = 1;
    pipeline.mayPlace({ type: 80, argument: 80, x: 20, y: 21, flags: 0, owner: 1, mode: 0 });
    assert.equal(state.preview.editor, 1);
    assert.equal(state.preview.useAlliances, 1);
    assert.equal(state.preview.alliances[5], 7);
    assert.equal(state.anchor.graphReady, 1);
    assert.equal(state.terrain.noIslandType, 157);
    // 표면 지도는 해시 0단계 배열 그대로이다(쓰기가 즉시 보인다).
    assert.equal(hash.view(0), hash.view(0));
    hash.setBucket(0, 20, 21, 60);
    assert.equal(hash.view(0)[21 * 256 + 20], 60);
    assert.equal(pipeline.pool, pool);
});

test('일반 배치 파이프라인: 자료/상태 누락과 다른 풀을 거부한다', () => {
    const scene = makeScene();
    const books = new SquidPostPopState();
    const data = { types: scene.types, frames: scene.frames, shapes: scene.shapes, hotspots: scene.hotspots, spots: scene.spots, islands: scene.islands, regions: scene.regions };
    const make = (override = {}) => new RawCanonPlacementPipeline(override.pool ?? scene.pool, override.hash ?? scene.hash, 'data' in override ? override.data : data, books, new SquidPostPopList(), new ContainedFinderState(), 'state' in override ? override.state : createCanonPlacementPipelineState());
    assert.ok(!throws(() => make()));
    assert.ok(throws(() => make({ data: null })));
    assert.ok(throws(() => make({ state: null })));
    assert.ok(throws(() => make({ hash: {} })));
    assert.ok(throws(() => make({ data: { ...data, spots: new Uint8Array(3) } })));
    assert.ok(throws(() => make({ data: { ...data, regions: undefined } })));
});
