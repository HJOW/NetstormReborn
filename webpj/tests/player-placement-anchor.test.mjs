// Player 배치 기준점 raw-player-placement-anchor.js(RawPlayerPlacementAnchor: 작업장/추가 목록·그래프·종속 조건·거리 선택)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: playeranchor-x86.tsv 의 10.78(originals) 1598행(collector L 1296·graphAt G 60·graphFor S 180·nearest N 62). cpppj player_anchor_patch_x86 과 같은 raw/TYPE/목록/지도 입력으로
//    반환·임시 후보 순서·raw 128슬롯 Adler·표면 지도 Adler 를 비교한다.
//  - 계약: player_anchor_required_data_and_lazy_guards, player_anchor_shared_full_deck_current_candidates, player_anchor_owner_bookkeeping_permission_and_terrain 중 기준점 부분
//    (실제 SquidOwner 장부 → Player 조회, contained 타입 재조회, 그래프 비활성). 후보 권한·지형 권한(RawCanonPlacementPermission/Terrain) 연결은 그 이식 단위 몫이다.
//  - 범위 밖: originalCD·original1037 행은 웹 범위(10.78) 밖이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E57: Player 배치 기준점 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, adler32, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { PLAYER_ANCHOR_GRAPH_ASSERTION, PlayerPlacementAnchorState, RawPlayerPlacementAnchor, makePlayerAnchorQuery } from '../www/core/raw-player-placement-anchor.js';

// 원본 관측 10.78 행이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E57)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const anchorRows = readRows(path.join(FIXTURES, 'playeranchor-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 판본당 관찰 행 수, 대조하는 raw 슬롯 수, 풀 크기이다(cpppj 와 같음). (2026-11-08 WB04-E57)
const ROWS = 1598;
const SLOTS = 128;
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-08 WB04-E57)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 쉼표 DWORD 목록을 읽되 빈 입력은 빈 목록으로 둔다(순서·중복 유지). (2026-11-08 WB04-E57)
const numbers = (text) => (text ? text.split(',').map((part) => Number(part) >>> 0) : []);

// 188칸 빈 TYPE 레코드이다. (2026-11-08 WB04-E57)
const emptyTypes = () => Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0 }));

// 기준점 장면(풀·TYPE·표면 지도·장부·추가 목록·내용물 상태·기준점 상태)이다. (2026-11-08 WB04-E57)
function anchorScene(server = false) {
    const pool = new SidPool(CAPACITY, { server });
    const types = emptyTypes();
    const surfaces = new Uint16Array(65536);
    const bookkeeping = new SquidPostPopState();
    const additional = new SquidPostPopList();
    const contained = new ContainedFinderState();
    const state = new PlayerPlacementAnchorState();
    const anchor = new RawPlayerPlacementAnchor(pool, types, surfaces, bookkeeping, additional, contained, state);
    return { pool, types, surfaces, bookkeeping, additional, contained, state, anchor };
}

test('PlayerPlacementAnchor: 원본 10.78 collector·그래프·거리 1598행의 반환·후보·raw·지도가 같다', { skip: !anchorRows && '원본 fixture 없음' }, () => {
    const scene = anchorScene(false);
    const { pool, types, surfaces, bookkeeping, additional, state, anchor } = scene;
    const stride = pool.layout.stride;
    const mapBytes = new Uint8Array(131072);
    let mapSurface = 0;
    const raw = new Uint8Array(SLOTS * stride);
    let count = 0;
    // 판본별 모든 입력의 물리 자료를 원본 실행기와 같은 초기 상태로 만든다.
    for (const row of anchorRows) {
        assert.equal(row.length, 20);
        // 앞 128 슬롯 전체를 0 으로 둔다.
        for (let sid = 0; sid < SLOTS; sid += 1) pool.physical(sid).fill(0);
        types[82].flags1 = 0x800;
        types[82].flags2 = Number(row[7]) >>> 0;
        types[83].flags1 = 0;
        types[83].flags2 = 0;
        state.graphReady = Number(row[4]) >>> 0;
        state.invalidGraph = Number(row[5]) >>> 0;
        state.ignoreRestrictions = Number(row[6]) >>> 0;
        const owner = Number(row[2]) >>> 0;
        const workshops = numbers(row[12]);
        const extras = numbers(row[13]);
        bookkeeping.ownerFactories[owner].entries = workshops.slice();
        bookkeeping.ownerFactories[owner].count = workshops.length;
        additional.entries = extras.slice();
        additional.count = extras.length;
        const surface = Number(row[9]) ? 116 : 0;
        surfaces.fill(surface);
        // native 지도와 같은 little endian 바이트를 checksum 대조용으로 준비한다(값이 바뀔 때만 다시 쓴다).
        if (surface !== mapSurface) {
            for (let index = 0; index < surfaces.length; index += 1) put(mapBytes, 2 * index, surface, 2);
            mapSurface = surface;
        }
        // 실제 raw 입력의 head/next 와 겹치는 종속 kind/mask 까지 같은 순서로 쓴다.
        for (const entry of row[14].split(';')) {
            if (!entry) continue;
            const node = numbers(entry);
            assert.equal(node.length, 10);
            const slot = pool.physical(node[0]);
            slot[10] = node[1];
            slot[34] = node[2];
            slot[30] = node[3];
            put(slot, 14, node[4]);
            put(slot, 18, node[5]);
            put(slot, 6, node[6], 2);
            put(slot, 4, node[7], 2);
            if (node[1] === 5 || node[1] === 6) {
                put(slot, 18, node[8], 2);
                put(slot, 20, node[9], 2);
            }
        }
        const x = bitsToFloat(Number(row[10]));
        const y = bitsToFloat(Number(row[11]));
        let observed = { sid: 0, candidates: [] };
        if (row[1] === 'L') observed = anchor.inspect(owner, Number(row[3]) >>> 0, x, y);
        else if (row[1] === 'G') observed.sid = anchor.graphAt(x, y);
        else if (row[1] === 'S') observed.sid = anchor.graphFor(Number(row[15]) >>> 0);
        else observed.sid = anchor.nearest(workshops, owner, x, y);
        // 앞 128 슬롯의 현재 물리 바이트를 이어 붙인다.
        for (let sid = 0; sid < SLOTS; sid += 1) raw.set(pool.physical(sid), sid * stride);
        assert.deepEqual([observed.sid, observed.candidates, adler32(raw), adler32(mapBytes)], [Number(row[16]), numbers(row[17]), Number(row[18]), Number(row[19])], `${row[1]} 행 ${count}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('PlayerPlacementAnchor: 손상 자료는 실제 접근 때 진단하고 무효 그래프/빈 목록의 조기 경로는 유지한다', () => {
    const scene = anchorScene(true);
    const { anchor, bookkeeping, additional, contained, state, pool, types } = scene;
    assert.equal(anchor.locate(0xffffffff, 0xffffffff, 20, 21), 0);
    assert.ok(throws(() => makePlayerAnchorQuery(new SidPool(CAPACITY), anchor)));
    assert.ok(throws(() => new RawPlayerPlacementAnchor(pool, types, new Uint16Array(0), bookkeeping, additional, contained, state)));
    state.invalidGraph = 0;
    assert.ok(throws(() => anchor.locate(9, 82, 20, 21)));
    additional.count = 1;
    assert.equal(anchor.locate(1, 0xffffffff, 20, 21), 0);
    bookkeeping.ownerFactories[1].count = 1;
    assert.ok(throws(() => anchor.locate(1, 82, 20, 21)));
    assert.equal(anchor.graphFor(0), 0);
    contained.checkingDead = true;
    assert.ok(throws(() => anchor.graphFor(0)));
    // 진단 장치가 복귀하면 원문대로 0 을 돌려준다.
    const calls = [];
    state.graphAssertion = (expression, file, line) => calls.push([expression, file, line]);
    assert.equal(anchor.graphFor(0), 0);
    assert.deepEqual(calls, [[PLAYER_ANCHOR_GRAPH_ASSERTION.expression, PLAYER_ANCHOR_GRAPH_ASSERTION.file, 0x93]]);
    assert.ok(throws(() => anchor.graphFor(65536)));
});

test('PlayerPlacementAnchor: 실제 작업장 후보가 같은 FullDeck DWORD 의 현재 값을 읽는다', () => {
    const scene = anchorScene(true);
    const { pool, types, surfaces, bookkeeping, state, anchor } = scene;
    const workshop = pool.allocate();
    const island = pool.allocate();
    const work = pool.allocated(workshop);
    const land = pool.allocated(island);
    types[82].flags1 = 0x800;
    types[84].flags1 = 0x800;
    types[84].flags2 = 2;
    work[10] = 82;
    work[11] = 0;
    work[30] = 7;
    work[34] = 1;
    put(work, 6, 0, 2);
    put(work, 14, floatToBits(20));
    put(work, 18, floatToBits(21));
    land[10] = 84;
    land[11] = 0;
    land[30] = 7;
    land[34] = 1;
    put(land, 36, 0);
    surfaces[21 * 256 + 20] = island;
    bookkeeping.ownerFactories[1].entries = [workshop];
    bookkeeping.ownerFactories[1].count = 1;
    state.graphReady = 1;
    state.ignoreRestrictions = 0xffffffff;
    let fullDeck = 0;
    state.bindFullDeck(() => fullDeck);
    // 공유 0 은 fallback 비영을 무시하고 공유 비영은 종속 없는 실제 작업장을 허용한다.
    for (const value of [0, 0x80000000, 0xffffffff, 0]) {
        fullDeck = value;
        const selected = anchor.inspect(1, 83, 20, 21);
        assert.deepEqual([selected.sid, selected.candidates.length], [value ? workshop : 0, value ? 1 : 0]);
    }
    state.bindFullDeck(null);
    assert.equal(anchor.locate(1, 83, 20, 21), workshop);
});

test('PlayerPlacementAnchor: 실제 소유자 작업장 장부 → Player 조회와 contained 타입·그래프 비활성 재조회(권한/지형 연결 제외)', () => {
    const scene = anchorScene(true);
    const { pool, types, surfaces, bookkeeping, additional, contained, state, anchor } = scene;
    // 소유자 지정이 현재 목록의 저장소에 실제로 추가/제거할 용량을 준비한다.
    for (const list of bookkeeping.ownerFactories) list.entries = new Array(4).fill(0);
    types[82].flags1 = 0x800;
    types[82].flags2 = 0x4000;
    types[84].flags1 = 0x800;
    types[84].flags2 = 2;
    types[83].flags1 = 2;
    const workshop = pool.allocate();
    const island = pool.allocate();
    const child = pool.allocate();
    const work = pool.allocated(workshop);
    const land = pool.allocated(island);
    const dependent = pool.allocated(child);
    work[10] = 82;
    work[11] = 0;
    work[30] = 7;
    put(work, 14, floatToBits(18));
    put(work, 18, floatToBits(20));
    land[10] = 84;
    land[11] = 0;
    land[30] = 7;
    land[34] = 1;
    put(land, 14, floatToBits(20));
    put(land, 18, floatToBits(21));
    surfaces[21 * 256 + 20] = island;
    const mode = new SquidOwnerMode();
    mode.battle = true;
    const owner = new SquidOwner(pool, types, bookkeeping, mode);
    owner.set(workshop, 1);
    assert.equal(bookkeeping.ownerFactories[1].count, 1);
    state.graphReady = 1;
    // 종속 없는 작업장은 그래프가 같아도 후보가 아니다(추가 목록도 보지 않음).
    assert.equal(anchor.locate(1, 83, 20, 21), 0);
    owner.set(workshop, 2);
    assert.deepEqual([bookkeeping.ownerFactories[1].count, bookkeeping.ownerFactories[2].count], [0, 1]);
    owner.set(workshop, 1);
    work[30] = 3;
    put(work, 6, child, 2);
    dependent[10] = 6;
    put(dependent, 18, 83, 2);
    put(dependent, 20, 16, 2);
    additional.entries = [island];
    additional.count = 1;
    // 종속으로 허용된 작업장(그래프 3)은 후보가 아니지만 linked 로 추가 목록을 열어 같은 그래프의 섬을 고른다.
    assert.equal(anchor.locate(1, 83, 20, 21), island);
    contained.containedType = 7;
    assert.equal(anchor.locate(1, 83, 20, 21), 0);
    state.graphReady = 0;
    assert.deepEqual([anchor.graphAt(20, 21), anchor.graphFor(workshop)], [254, 3]);
});
