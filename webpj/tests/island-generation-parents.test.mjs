// 12차 스커미시 월드 생성에 쓰는 섬 생성 부모 셋(군도 분리 RawIslandPartition, 일반 중앙섬 RawCentralIsland, 전투섬 RawBattleIsland)을 실제 SidPool·ChunkMap·섬 목록·RawIslandBuilder 로 검사한다.
// cpppj 에는 이 셋을 직접 검사하는 시험이 없어(Session 통합뿐) 원문 소스의 호출 순서·계산·진단 조건을 독립 기대값으로 대조한다. 게임을 실행하지 않는다. (2026-11-20 WB04-E1104)
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { ChunkMap } from '../www/assets/territory.js';
import { RawIslandRuntime, createFortFinishOwnerState, createRawIslandStorage } from '../www/core/raw-island-runtime.js';
import { RawIslandPartition } from '../www/core/raw-island-partition.js';
import { RawCentralIsland } from '../www/core/raw-central-island.js';
import { RawBattleIsland } from '../www/core/raw-battle-island.js';
import { TERRITORY_PATTERNS } from '../www/assets/territory-patterns.js';

// 실제 puzzlePiece 타입 번호와 합성 프레임이다(16방향 × 번호 0..10, island-builder 시험의 기본 profile 과 같다). (2026-11-20 WB04-E1104)
const PIECE = 107;
function syntheticFrames() {
    const codes = [];
    for (let side = 65; side <= 80; side += 1) for (let number = 0; number < 11; number += 1) codes.push({ side, variant: 80, number, flags: 0 });
    codes.push({ ...codes[0] });
    return codes;
}
// 실제 풀·월드 지도·섬 목록을 가진 장면이다. (2026-11-20 WB04-E1104)
function makeScene() {
    const pool = new SidPool(32768, { server: false }), world = new ChunkMap(), hash = new SquidHash();
    const frames = Array.from({ length: 188 }, () => []); frames[PIECE] = syntheticFrames();
    const islands = new RawIslandRuntime(pool, hash, createRawIslandStorage(), createFortFinishOwnerState(), new SquidPostPopList(), { value: 0 }, { value: 0 }, { setResidenceOwner() {}, assertion() {} });
    return { pool, world, hash, frames, islands, log: [] };
}

test('전투섬: 0x04 플래그 첫 영역(없으면 0번)을 골라 배치하고 섬 번호·지형·객체·Graph 를 원문 순서로 부른다', () => {
    for (const [flagged, expected, boss] of [[[2, 5], 2, true], [[], 0, false], [[7], 7, true], [[19], 19, false]]) {
        const scene = makeScene();
        const hooks = {
            assertion: (line) => scene.log.push(['assertion', line]),
            territory: (owner, output) => {
                assert.equal(output.length, 120); scene.log.push(['territory', owner]);
                // 모든 영역을 3x3 패턴 0 의 서로 다른 회전으로 채우고 플래그를 지정한다.
                for (let index = 0; index < 20; index += 1) output.set([0 | ((index & 3) << 6), flagged.includes(index) ? 4 : 0, 0x11, index, 0, 0], index * 6); // 위치 바이트 0x11 은 위치 무시(true) 때문에 배치에 쓰이지 않는다.
            },
            placement: (owner, record) => { scene.log.push(['placement', owner, [...record]]); return { left: 4, top: 6, right: 7, bottom: 9 }; },
            region: (owner, island) => scene.log.push(['region', owner, island]),
            generate: (state, island) => scene.log.push(['generate', state.owner, { ...state.bounds }, island]),
            boss: () => boss, objects: (owner, first, end) => scene.log.push(['objects', owner, first, end]),
            graph: (reset) => scene.log.push(['graph', reset]), builderAssertion: (kind) => scene.log.push(['builder', kind]),
        };
        new RawBattleIsland(scene.pool, scene.world, scene.islands, scene.frames, () => PIECE, hooks).build(3);
        const names = scene.log.map((entry) => entry[0]);
        assert.deepEqual(names, boss ? ['territory', 'placement', 'region', 'generate', 'objects', 'graph'] : ['territory', 'placement', 'region', 'generate', 'graph'], `선택 ${expected}`);
        assert.deepEqual(scene.log[1][2], [0 | ((expected & 3) << 6), flagged.length ? 4 : 0, 0x11, expected, 0, 0], '선택한 영역의 기록이 배치에 전달된다');
        assert.equal(scene.log[1][1], 3);
        // 새 섬은 목록의 첫 지역이고 owner 3·영역 번호로 기록되며 지형 생성은 같은 지역과 현재 경계를 받는다.
        assert.equal(scene.log[2][2], 0); assert.equal(scene.islands.count, 1); assert.equal(scene.islands.owner(0), 3);
        assert.deepEqual(scene.log[3], ['generate', 3, { left: 4, top: 6, right: 7, bottom: 9 }, 0]);
        if (boss) assert.deepEqual(scene.log[4], ['objects', 3, expected, expected + 1]);
        assert.deepEqual(scene.log.at(-1), ['graph', false]);
        // 실제 월드 지도에 3x3 패턴 0 이 지역 0 으로 놓였다: 패턴의 빈 칸('.')을 뺀 칸 수만큼 섬 번호 0 이고 그 칸의 레코드 4번째 바이트가 선택 영역 번호이다.
        const cells = [];
        for (let x = 0; x < 16; x += 1) for (let y = 0; y < 16; y += 1) if (scene.world.at(x, y).island === 0) cells.push([x, y]);
        assert.equal(cells.length, [...TERRITORY_PATTERNS[0].sides].filter((side) => side !== '.').length, `선택 ${expected} 의 칸 수`);
        assert.ok(cells.every(([x, y]) => x >= 4 && x < 7 && y >= 6 && y < 9)); assert.ok(cells.every(([x, y]) => scene.world.at(x, y).record[3] === expected));
    }
});

test('전투섬: owner 0 은 진단 뒤 계속하고 필수 장치가 없으면 효과 전에 거부한다', () => {
    const scene = makeScene(), calls = [];
    const hooks = { assertion: (line) => calls.push(line), territory: () => {}, placement: () => ({ left: 1, top: 1, right: 2, bottom: 2 }), region: () => {}, generate: () => {}, boss: () => false, objects: () => {}, graph: () => {}, builderAssertion: () => {} };
    new RawBattleIsland(scene.pool, scene.world, scene.islands, scene.frames, () => PIECE, hooks).build(0);
    assert.deepEqual(calls, [0x10f]); assert.equal(scene.islands.count, 1);
    for (const missing of ['territory', 'placement', 'region', 'generate', 'boss', 'graph', 'builderAssertion']) {
        const broken = makeScene();
        assert.throws(() => new RawBattleIsland(broken.pool, broken.world, broken.islands, broken.frames, () => PIECE, { ...hooks, [missing]: undefined }).build(2), /미연결/u, missing);
    }
    // 다른 풀의 섬 목록은 생성자가 거부한다.
    const other = makeScene();
    assert.throws(() => new RawBattleIsland(scene.pool, scene.world, other.islands, scene.frames, () => PIECE, hooks), /Pool/u);
});

test('중앙섬: 진단 네 개 뒤 중심 기록·섬 생성·거주 구역/군도 분리 조건·Graph 순서', () => {
    // [battle, boss, terrainEnabled, archipelago, 기대 호출 순서]
    for (const [battle, boss, terrain, archipelago, expected] of [
        [true, true, 0, 3, ['placement', 'generate', 'residence', 'partition', 'graph']],
        [true, true, 1, 0, ['placement', 'generate', 'graph']],
        [true, false, 0, 1, ['placement', 'generate', 'partition', 'graph']],
        [false, false, 0, 0, ['assertion:inBattleMode:234', 'placement', 'generate', 'graph']],
    ]) {
        const scene = makeScene(), position = new Float32Array(2);
        const hooks = {
            assertion: (text, line) => scene.log.push(`assertion:${text}:${line}`),
            placement: (record) => { scene.log.push('placement'); assert.deepEqual([...record], [3, 9, 0, 77, 0, 0]); return { left: 5, top: 7, right: 8, bottom: 10 }; },
            generate: (state, island) => { scene.log.push('generate'); assert.equal(state.owner, 0); assert.deepEqual(state.bounds, { left: 5, top: 7, right: 8, bottom: 10 }); assert.equal(island, 0); },
            residence: (island) => { scene.log.push('residence'); assert.equal(island, 0); }, partition: () => scene.log.push('partition'),
            graph: (reset) => { scene.log.push('graph'); assert.equal(reset, false); }, builderAssertion: () => scene.log.push('builder'),
        };
        const globals = { battle: () => battle, boss: () => boss, terrainEnabled: () => terrain, archipelago: () => archipelago, position, unassignedTerritory: () => 127 };
        new RawCentralIsland(scene.pool, scene.world, scene.islands, scene.frames, () => PIECE, globals, hooks).build(Uint8Array.of(3, 9, 0, 77, 0, 0));
        assert.deepEqual(scene.log, expected, JSON.stringify([battle, boss, terrain, archipelago]));
        // 중심은 청크 경계 ×16 의 평균이다: X=(8·16+5·16)/2=104, Y=(10·16+7·16)/2=136.
        assert.deepEqual([...position], [104, 136]);
        // 섬 목록의 첫 지역은 owner 0 이고 영역 번호는 현재 invalidRegion(127) 이다(builder owner 0 이 아니다). 월드 지도의 섬 번호도 같다.
        assert.equal(scene.islands.count, 1); assert.equal(scene.islands.owner(0), 0); assert.equal(scene.islands.matches(0, 0, 127), true);
        assert.equal(scene.world.at(5, 7).island, 0);
    }
});

test('중앙섬: 위치 0 이 아닌 기록과 이미 섬이 있는 목록은 진단하고 계속하며 필수 장치가 없으면 거부한다', () => {
    const scene = makeScene(); scene.islands.add(1, 2); // 이미 섬이 하나 있다.
    const position = new Float32Array(2), calls = [];
    const hooks = { assertion: (text, line) => calls.push([text, line]), placement: () => ({ left: 5, top: 5, right: 8, bottom: 8 }), generate: () => {}, residence: () => {}, partition: () => {}, graph: () => {}, builderAssertion: () => {} };
    const globals = { battle: () => true, boss: () => false, terrainEnabled: () => 0, archipelago: () => 0, position, unassignedTerritory: () => 127 };
    new RawCentralIsland(scene.pool, scene.world, scene.islands, scene.frames, () => PIECE, globals, hooks).build(Uint8Array.of(3, 9, 0x21, 1, 0, 0));
    assert.deepEqual(calls, [['terr.xChunkCoord == 0 && terr.yChunkCoord == 0', 0xeb], ['islandList.getTotal() == 0', 0xec]]);
    assert.equal(scene.islands.count, 2);
    for (const missing of ['placement', 'generate', 'graph', 'builderAssertion']) {
        const broken = makeScene();
        assert.throws(() => new RawCentralIsland(broken.pool, broken.world, broken.islands, broken.frames, () => PIECE, globals, { ...hooks, [missing]: undefined }).build(Uint8Array.of(3, 9, 0, 1, 0, 0)), /미연결/u, missing);
    }
    const other = makeScene();
    assert.throws(() => new RawCentralIsland(scene.pool, scene.world, other.islands, scene.frames, () => PIECE, globals, hooks), /Pool/u);
});

// 합성 지면 하나(SID, 칸 좌표, 프레임 번호)를 풀의 raw 에 쓴다: 오프셋 10 TYPE, 14 X float32, 18 Y float32, 36 프레임(signed 32). (2026-11-20 WB04-E1104)
function surfaceScene(tiles, { terrain = 5, frameSides = {} } = {}) {
    const scene = makeScene(), surfaces = new Uint16Array(65536), regions = new Map(), writes = [], slots = new Map(), redraws = { count: 0 };
    const codes = Object.keys(frameSides).sort().map((frame) => ({ side: frameSides[frame].charCodeAt(0) }));
    const frames = Array.from({ length: 188 }, () => []); frames[terrain] = codes;
    if (terrain >= 188) frames.length = 188; // 표 밖 TYPE 시험은 표를 늘리지 않는다.
    // SID 는 1 부터 칸마다 하나씩 준다.
    tiles.forEach((tile, index) => {
        const sid = index + 1, raw = new Uint8Array(64), view = new DataView(raw.buffer);
        raw[10] = tile.type ?? terrain; view.setFloat32(14, tile.fx ?? tile.x, true); view.setFloat32(18, tile.fy ?? tile.y, true); view.setInt32(36, tile.frame ?? 0, true);
        slots.set(sid, raw); surfaces[tile.y * 256 + tile.x] = sid; tile.sid = sid;
    });
    const pool = { edition: 'patch1078', slot: (sid) => slots.get(sid).slice() };
    const islands = { pool, count: 1, regions: [], add(owner, territory) { this.regions.push(['add', owner, territory]); const region = this.count; this.count += 1; return region; }, setBounds(region, bounds) { this.regions.push(['bounds', region, { ...bounds }]); } };
    const part = new RawIslandPartition(pool, surfaces, frames, () => terrain, islands, () => { redraws.count += 1; },
        (sid) => regions.get(sid) ?? 0, (sid, region) => { writes.push([sid, region]); regions.set(sid, region); });
    return { part, islands, writes, redraws, regions, tiles };
}

test('군도 분리: 가로 3칸 섬과 외딴 한 칸을 y→x 순서로 새 지역으로 만들고 경계·다시 그리기를 기록한다', () => {
    // 프레임 번호 → 방향 글자: O(E 만 연결) K(E·W) M(W 만) P(연결 없음). 연결 비트 표: O=2 K=10 M=8 P=0.
    const sides = { 0: 'O', 1: 'K', 2: 'M', 3: 'P' };
    // 맨 아래 행(y=0)의 외딴 칸이 가장 먼저 스캔되고, 오른쪽 끝 칸(frame 1 = E·W)은 동쪽의 다른 TYPE 칸(13,20)을 가리켜도 그 칸은 칠하지 않는다.
    const { part, islands, writes, redraws, regions, tiles } = surfaceScene([
        { x: 50, y: 50, frame: 3 }, { x: 10, y: 20, frame: 0 }, { x: 11, y: 20, frame: 1 }, { x: 12, y: 20, frame: 1 }, { x: 13, y: 20, frame: 2, type: 4 }, { x: 100, y: 0, frame: 3 },
    ], { frameSides: sides });
    assert.equal(part.split(), 3);
    // 스캔 순서: y=0 의 한 칸(지역 1)·y=20 행의 섬(지역 2)·y=50 의 한 칸(지역 3). 새 기록은 add(0, 0xff) 이다.
    assert.deepEqual(islands.regions, [['add', 0, 0xff], ['bounds', 1, { left: 100, top: 0, right: 100, bottom: 0 }], ['add', 0, 0xff], ['bounds', 2, { left: 10, top: 20, right: 12, bottom: 20 }],
        ['add', 0, 0xff], ['bounds', 3, { left: 50, top: 50, right: 50, bottom: 50 }]]);
    assert.deepEqual(writes, [[tiles[5].sid, 1], [tiles[1].sid, 2], [tiles[2].sid, 2], [tiles[3].sid, 2], [tiles[0].sid, 3]]); // 시작 칸에서 동쪽으로 번진다.
    assert.equal(regions.get(tiles[4].sid), undefined); assert.equal(redraws.count, 3); // flood 마다 월드 다시 그리기를 올린다.
});

test('군도 분리: 십자 섬의 방문 순서는 중심 → 북 → 동 → 남 → 서이다', () => {
    // 중심(A: 사방 연결)과 한 방향씩만 중심으로 연결된 네 칸: 북칸은 남쪽(4) 연결 'L'(mask 4)… 연결 비트 표에서 4=L, 8=M 이 아닌 값은 글자 표로 찾는다.
    // 글자별 연결 비트(원문 표 A~P): A15 B7 C14 D13 E11 F6 G12 H9 I3 J5 K10 L4 M8 N1 O2 P0. 북칸은 남쪽 한 방향(4)=L, 동칸은 서쪽(8)=M, 남칸은 북쪽(1)=N, 서칸은 동쪽(2)=O 이다.
    const { part, writes, tiles } = surfaceScene([{ x: 20, y: 20, frame: 0 }, { x: 20, y: 19, frame: 1 }, { x: 21, y: 20, frame: 2 }, { x: 20, y: 21, frame: 3 }, { x: 19, y: 20, frame: 4 }],
        { frameSides: { 0: 'A', 1: 'L', 2: 'M', 3: 'N', 4: 'O' } });
    assert.equal(part.split(), 1);
    // 스캔의 첫 칸은 y=19 의 북칸이라 그 raw 좌표에서 시작해 중심으로, 중심에서 N(이미 방문)·E·S·W 로 번진다. 방문 순서를 고정한다.
    assert.deepEqual(writes.map(([sid]) => tiles.findIndex((tile) => tile.sid === sid)), [1, 0, 2, 3, 4]);
});

test('군도 분리: 시작 좌표가 섬의 아래쪽 칸이면 북쪽으로 번진다(세로 3칸)', () => {
    // 위 칸(남 연결 L) · 가운데(북·남 연결 J) · 아래 칸(북 연결 N). 스캔은 가장 위 칸에서 하지만 raw 좌표는 맨 아래 칸(5,12)을 가리킨다.
    const { part, writes, tiles, islands } = surfaceScene([{ x: 5, y: 10, frame: 0, fx: 5, fy: 12 }, { x: 5, y: 11, frame: 1 }, { x: 5, y: 12, frame: 2 }], { frameSides: { 0: 'L', 1: 'J', 2: 'N' } });
    assert.equal(part.split(), 1);
    assert.deepEqual(writes.map(([sid]) => tiles.findIndex((tile) => tile.sid === sid)), [2, 1, 0]); // 아래 → 가운데 → 위
    assert.deepEqual(islands.regions[1], ['bounds', 1, { left: 5, top: 10, right: 5, bottom: 12 }]);
});

test('군도 분리: raw 좌표가 지도 밖·비유한이면 빈 경계의 섬이 만들어지고 범위 밖 프레임·TYPE 은 거부한다', () => {
    // 좌표가 NaN 인 한 칸은 시작점이 0,0(빈 칸)이라 아무것도 칠하지 못하지만 섬 기록은 만들어진다.
    const nan = surfaceScene([{ x: 30, y: 30, frame: 0, fx: Number.NaN }], { frameSides: { 0: 'P' } });
    assert.equal(nan.part.split(), 1); assert.deepEqual(nan.islands.regions[1], ['bounds', 1, { left: 257, top: 257, right: -1, bottom: -1 }]); assert.equal(nan.writes.length, 0); assert.equal(nan.redraws.count, 1);
    // 큰 float 는 64비트 정수 범위 안이면 하위 32비트로 감겨 지도 밖이 된다(그래도 오류는 아니다).
    const wrapped = surfaceScene([{ x: 0, y: 30, frame: 0, fx: 4294967296, fy: 30 }], { frameSides: { 0: 'P' } });
    assert.equal(wrapped.part.split(), 1); assert.equal(wrapped.writes.length, 1); // 2^32 → 하위 32비트 0 이라 칸 (0,30) 자신에서 시작한다.
    const huge = surfaceScene([{ x: 0, y: 30, frame: 0, fx: 1e30, fy: 30 }], { frameSides: { 0: 'P' } });
    assert.equal(huge.part.split(), 1); assert.equal(huge.writes.length, 1); // 64비트 범위 밖은 0 이다(위와 같은 시작 칸).
    const negative = surfaceScene([{ x: 7, y: 30, frame: 0, fx: -1.5, fy: 30 }], { frameSides: { 0: 'P' } });
    assert.equal(negative.part.split(), 1); assert.equal(negative.writes.length, 0); // -1.5 → -1 → 지도 밖이라 칠하지 못한다.
    // 프레임 번호가 코드 표 밖(음수 포함)이면 flood 가 오류를 던진다.
    assert.throws(() => surfaceScene([{ x: 5, y: 5, frame: -1 }], { frameSides: { 0: 'P' } }).part.split(), /프레임 코드 범위/u);
    assert.throws(() => surfaceScene([{ x: 5, y: 5, frame: 9 }], { frameSides: { 0: 'P' } }).part.split(), /프레임 코드 범위/u);
    // 현재 지면 TYPE 이 프레임 표 밖이면 군도 분리가 오류를 던진다.
    assert.throws(() => surfaceScene([{ x: 5, y: 5, frame: 0, type: 200 }], { terrain: 200, frameSides: { 0: 'P' } }).part.split(), /지면 TYPE 범위/u);
    // 생성자 검사: 표 크기·장치·판본·풀 동일성.
    const base = surfaceScene([], { frameSides: {} });
    assert.throws(() => new RawIslandPartition(base.islands.pool, new Uint16Array(10), [], () => 5, base.islands, () => {}, () => 0, () => {}), /불일치/u);
    assert.throws(() => new RawIslandPartition({ ...base.islands.pool, edition: 'cd1072' }, new Uint16Array(65536), [], () => 5, base.islands, () => {}, () => 0, () => {}), /불일치/u);
    assert.throws(() => new RawIslandPartition(base.islands.pool, new Uint16Array(65536), [], () => 5, base.islands, () => {}, null, () => {}), /불일치/u);
    assert.throws(() => new RawIslandPartition(base.islands.pool, new Uint16Array(65536), [], () => 5, base.islands, null, () => 0, () => {}), /미연결/u);
});
