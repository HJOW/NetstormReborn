// Fort 적재 부모의 월드 연결(cpppj `client/RawFortWorldLoadBinding`)의 웹 대응 `core/raw-fort-world-load-binding.js` 를 검사한다(4-B, WB04-E303).
// cpppj `FortWorldLoadBindingTests.cpp` 의 장면을 10.78 로 옮겼다: 같은 실제 월드(Pool/Factory/공간/지역/장부)에서 Builder(PlaceAll) → 정상 지형 상위(generateOriginal) → 저장 후처리 → Graph 0 의 순서와,
// 두 난수 저장소(게임 난수·CRT 99 캐시)의 소비를 확인한다. 기존 PlaceAll 의 별도 원본 골든(`fortworldload-builder-consumer-x86.tsv` originals 행)과 청크 3072BYTE·섬 3204BYTE 의 입력/출력이 같다.
// 환원: cpppj 시험은 빈 저장 목록에서 실제 객체/Money/Meta child 모듈을 돌렸지만, 웹은 child 를 호출 순서·인자·깊이 계약을 기록하는 관찰 어댑터로 바꾼다(child 본문은 E276~E286 단위가 소유하고
// Tutorial1 Fort 의 실제 child 통합은 4-B 마지막 적재 통합 시험에서 본다). 월드·정상 지형·Builder·부모·Graph 는 모두 실제 모듈이다. 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E303)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { ChunkMap } from '../www/assets/territory.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { CarrierPostPopState, RawCarrierPostPop, makeCarrierPostPopHooks } from '../www/core/raw-carrier-post-pop.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { createCanonPlacementPipelineState, RawCanonPlacementPipeline } from '../www/core/raw-canon-placement-pipeline.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { FarmClusterState } from '../www/core/raw-farm-cluster.js';
import { RawFortWorldLoad } from '../www/core/raw-fort-world-load.js';
import { makeFortWorldLoadBinding } from '../www/core/raw-fort-world-load-binding.js';
import { FortRandomPlacementState } from '../www/core/raw-fort-random-placement.js';
import { RawIslandRuntime, createFortFinishOwnerState, createRawIslandStorage } from '../www/core/raw-island-runtime.js';
import { RawNormalTerrainWorldBinding } from '../www/core/raw-normal-terrain-world-binding.js';
import { RawSurfaceWorld } from '../www/core/raw-surface-world.js';
import { TerrainContextGlobals } from '../www/core/raw-terrain-context.js';
import { TerrainStampState } from '../www/core/raw-terrain-stamp.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { REPO_ROOT } from './helpers.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { get, put, readRows } from './raw-support.mjs';

// 원본 PlaceAll 소비자 골든(originals 행)이다. 파일이 없으면 검사를 건너뛴다. 열: edition, Territory120, physicalPuzzleCodes, initialChunk3072, initialIsland3204, placedChunk3072, placedIsland3204, PlaceCalls, AddCalls, BoundsCalls, asserts. (2026-11-11 WB04-E303)
const GOLDEN = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fortworldload-builder-consumer-x86.tsv'))?.find((row) => row[0] === 'originals') ?? null;

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-11 WB04-E303)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 현재 청크 전체 3072BYTE 를 원본 X열 우선 12BYTE(섬·방향 글자·record 6·순번)로 직렬화한다. (2026-11-11 WB04-E303)
function chunkHex(chunks) {
    const bytes = new Uint8Array(256 * 12);
    // 원본 저장소는 X열을 먼저 고정한다.
    for (let x = 0; x < 16; x += 1) {
        // 각 X열의 Y 항목은 padding 없는 실제 12BYTE 필드이다.
        for (let y = 0; y < 16; y += 1) {
            const entry = chunks.at(x, y);
            const offset = (x * 16 + y) * 12;
            bytes[offset] = entry.island;
            bytes[offset + 1] = typeof entry.side === 'string' ? entry.side.charCodeAt(0) : entry.side & 0xff;
            bytes.set(entry.record, offset + 2);
            put(bytes, offset + 8, entry.ordinal >>> 0);
        }
    }
    return hex(bytes);
}

// 물리 프레임 코드(방향·변형·번호·플래그)를 원본 순서의 BYTE 로 직렬화한다. 검색 결과를 재계산하지 않는다. (2026-11-11 WB04-E303)
function codeHex(codes) {
    const bytes = [];
    // 네 BYTE 물리 코드를 원본 순서대로 보존한다.
    for (const code of codes) bytes.push(code.side, code.variant, code.number, code.flags);
    return hex(bytes);
}

// 미복원 하위 효과를 기본 성공으로 넘기지 않는 실패 함수이다. (2026-11-11 WB04-E303)
const fail = () => { throw new Error('선택 FortWorld 소비자의 미복원 하위 효과 요청'); };

// 선택 Fort 월드 적재 장면이다: 실제 월드·정상 지형·섬 목록·Builder 부모와 관찰용 child 를 한 수명으로 조립한다. (2026-11-11 WB04-E303)
class FortBindingScene {
    // 실제 월드·정상 지형·섬 목록·Builder 부모가 쓰는 입력 표와 상태를 만들고 서로 연결한다.
    constructor() {
        const types = realPlayerAssets().types.map((record) => ({ ...record }));
        const codes = types.map(() => []);
        // quarter 에 충분한 16 후보/방향과 puzzlePiece 의 P 변형 번호 0..10 을 같은 방향 루프에서 공급한다.
        for (let side = 0x41; side <= 0x50; side += 1) {
            // 같은 방향의 A 변형 16개 중 선택은 기존 원본 Choose 몸체가 결정한다.
            for (let number = 0; number < 16; number += 1) codes[97].push({ side, variant: 0x41, number: 1, flags: 0 });
            // puzzlePiece 의 실제 P 변형 번호를 모두 공급한다.
            for (let number = 0; number < 11; number += 1) codes[107].push({ side, variant: 0x50, number, flags: 0 });
        }
        codes[121] = codes[97].slice();
        const frames = codes.map((list) => new FrameCodeTable(list));
        const shapes = types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
        // 원본 97 의 base 계약과 121 의 실제 Damageable 생성자를 그대로 쓴다. 그 밖의 연결 타입은 실제 생성자 주소를 비워 둔다.
        for (const type of [107, 122]) types[type].constructorAddress = 0;
        // 두 타입에 실제 생성자와 합성 속성·SHP 를 넣는다.
        for (const type of [97, 121]) {
            const record = types[type];
            Object.assign(record, { constructorAddress: typeConstructorAddress(type), flags1: type === 97 ? 0x28000803 : 0x00200012, flags2: type === 97 ? 2 : 0x80000, footX: 1, footY: 1, maxHitPoints: 73 });
            shapes[type] = { frameCount: 256, loaded: true, frames: Array.from({ length: 256 }, () => ({ width: 20, height: 16, hotspotX: 8, hotspotY: 12, cellWidth: 1, cellHeight: 1 })) };
        }
        const links = new BridgeConnectState();
        Object.assign(links, { bridgeType: 82, islandType: 94, stalagType: 95, connectorType: 155, noIslandType: 157, battle: true });
        this.world = new RawSurfaceWorld(types, frames, shapes, links, 97);
        const core = this.world.core();
        this.core = core;
        this.chunks = new ChunkMap();
        this.islandBytes = createRawIslandStorage();
        this.piece = { value: 107 };
        this.localOwner = { value: 1 };
        this.noRegion = { value: 127 };
        this.depth = { value: 1 };
        this.islands = new RawIslandRuntime(core.pool, core.hash, this.islandBytes, createFortFinishOwnerState(), new SquidPostPopList(), { value: 0 }, { value: 0 }, {
            setResidenceOwner: (sid, player) => core.owner.set(sid, player),
            assertion: fail,
        });
        this.islands.reset();
        this.random = new GameRandom(0x12345678);
        this.crt = { value: 7 };
        this.sp = new ScrambledSpStore(this.random, () => 0x12345678);
        const hp = new SquidReward(core.pool, core.types, core.bookkeeping, new SquidRewardState(), this.sp);
        const plain = types.map((_, type) => ({ frames: core.frames[type].codes, defaultFrame: 0 }));
        const pipelineState = createCanonPlacementPipelineState();
        pipelineState.geometry.patternTypes = [107, 82, 94, 157, 131, 129, 140, 142];
        pipelineState.placement.localPlayer = 1;
        pipelineState.permission.graphReady = 0;
        pipelineState.permission.alliances.fill(1);
        const pipeline = new RawCanonPlacementPipeline(core.pool, core.hash, { types: core.types, frames: plain, shapes: core.shapes, hotspots: types.map(() => ({ x: 0, y: 0 })), spots: core.spots, islands: core.hash.view(0), regions: [] },
            core.bookkeeping, new SquidPostPopList(), new ContainedFinderState(), pipelineState);
        this.carrierState = new CarrierPostPopState();
        this.carrierState.loadingDepth = 1;
        const carrier = new RawCarrierPostPop(core.pool, core.types, this.carrierState, makeCarrierPostPopHooks(core.pool, core.postPop, fail, (x, y, owner) => this.world.setSupportOwner(x, y, owner)));
        this.stamp = new TerrainStampState();
        this.normal = new RawNormalTerrainWorldBinding(this.world, this.chunks, this.islands, this.random, this.crt, new TerrainContextGlobals(), this.stamp,
            new FortRandomPlacementState(), new FarmClusterState(), this.piece, pipeline, hp, carrier, { context: fail, stamp: fail, region: fail, farm: fail, postpass: fail });
        // 기존 독립 일반 생성 입력을 원문 부모의 명시 군도 후보 꺼짐으로 표현한다.
        this.normal.bindGenerationInputs({
            enabled: () => 0, battle: () => core.ownerMode.battle, mission: () => core.bridgeState.mission, layout: () => 0,
            span: { value: 1 }, minimum: { value: 15 }, maximum: { value: 35 }, percent: { value: 0 }, archipelago: { value: 0 },
        });
    }
}

// 실제 단일 지면/121 생성·지역·청크·두 난수·현재 장부에 후처리 순서와 Graph 0 을 연결한다. (2026-11-11 WB04-E303)
test('Fort 월드 적재 연결: 실제 월드·정상 지형·두 난수 저장소', { skip: GOLDEN === null }, () => {
    const scene = new FortBindingScene();
    const { world, normal, chunks, islands, islandBytes, core } = scene;
    const effects = [];
    const territoryBytes = new Uint8Array(120);
    territoryBytes.set([3, 1, 0x11, 7, 0xaa, 0xbb]);
    const fort = { territory: territoryBytes };
    const territory = { bytes: territoryBytes, position: 0, end: 120 };
    // 현재 깊이는 한 DWORD 이고 모든 child/Carrier/표면에 같은 값을 전달한다.
    const sync = (value) => { scene.depth.value = value; scene.carrierState.loadingDepth = value; world.setLoadingDepth(value); };
    // 실제 wrapper 는 깊이를 올린 뒤 현재 Money 섹션을 선택한다.
    const money = { loadSelected: (select) => { sync(scene.depth.value + 1); select(); sync(scene.depth.value - 1); } };
    const objects = { pool: core.pool, load: (current, owner, from, count) => { assert.deepEqual([current, owner, from, count], [fort, 1, 0, 20]); effects.push('objects'); } };
    const meta = {
        pool: core.pool,
        loadBadgesSelected: (select) => { select(); },
        loadTechnologySelected: (select) => { effects.push('parent-boundary'); sync(2); select(); sync(1); effects.push('ui-boundary'); },
    };
    const moneyCursor = { position: 0, end: 4 };
    const base = {
        templatePresent: () => true,
        territory: (owner) => { assert.equal(owner, 1); return territory; },
        currentTemplate: (owner) => { assert.equal(owner, 1); return fort; },
        assertion: fail,
        builderAssertion: fail,
        populateIsland: fail,
    };
    const hooks = makeFortWorldLoadBinding(world, normal, objects, money, meta,
        (current, owner) => { assert.deepEqual([current, owner, scene.depth.value], [fort, 1, 2]); effects.push('money'); moneyCursor.position = 4; return moneyCursor; },
        (current, owner, technology) => { assert.equal(current, fort); effects.push(technology ? 'technology' : 'badges'); },
        (current, technology) => {
            assert.equal(current, fort);
            assert.equal(scene.depth.value, technology ? 2 : 1);
            if (technology) assert.equal(effects.at(-1), 'parent-boundary');
            effects.push(technology ? 'technology-select' : 'badges-select');
            return {};
        }, base);
    // 실제 generate/objects/Graph 만 관찰로 감싸고 완료 body 호출은 그대로 유지한다.
    const generate = hooks.generate;
    const loadObjects = hooks.objects;
    const rebuild = hooks.rebuildGraph;
    hooks.generate = (state, region) => {
        assert.equal(chunkHex(chunks), GOLDEN[5]);
        assert.equal(hex(islandBytes), GOLDEN[6]);
        assert.equal(islands.count, Number(GOLDEN[8]));
        assert.deepEqual([GOLDEN[7], GOLDEN[8], GOLDEN[9], GOLDEN[10]], ['1', '1', '1', '0']);
        effects.push('generate');
        generate(state, region);
    };
    hooks.objects = (current, owner) => { loadObjects(current, owner); };
    hooks.rebuildGraph = (reset) => { effects.push('graph0'); assert.equal(reset, false); rebuild(reset); };
    assert.equal(hex(fort.territory), GOLDEN[1]);
    assert.equal(codeHex(core.frames[107].codes), GOLDEN[2]);
    assert.equal(chunkHex(chunks), GOLDEN[3]);
    assert.equal(hex(islandBytes), GOLDEN[4]);
    world.beginLoad();
    sync(1);
    const parent = new RawFortWorldLoad(core.pool, chunks, islands, core.frames.map((table) => table.codes), scene.piece, scene.localOwner, scene.noRegion, hooks);
    parent.load(0);
    assert.equal(chunkHex(chunks), GOLDEN[5]);
    assert.equal(hex(islandBytes), GOLDEN[6]);
    assert.deepEqual([islands.count, islands.exists(0), islands.owner(0), chunks.at(2, 2).island, territory.position, territory.end], [1, 1, 1, 0, 120, 120]);
    assert.deepEqual(effects, ['generate', 'objects', 'money', 'badges', 'badges-select', 'technology', 'parent-boundary', 'technology-select', 'ui-boundary', 'graph0']);
    assert.deepEqual([scene.depth.value, scene.carrierState.loadingDepth, moneyCursor.position], [1, 1, 4]);
    // 두 난수 저장소: 게임 난수와 CRT 99 캐시를 소비했고 SP 저장소는 아직 초기화하지 않았다.
    assert.notEqual(scene.random.state, 0x12345678);
    assert.notEqual(scene.crt.value, 7);
    assert.notEqual(scene.stamp.variants, null);
    const list = world.objects();
    assert.ok(list.length > 0);
    let farms = 0;
    // 모든 생성 SID 는 같은 Factory/풀의 현재 TYPE/지역이며 Farm 도 실제 HP 후처리를 사용한다.
    for (const object of list) {
        const raw = core.pool.slot(object.sid);
        assert.equal(get(raw, 8, 2), 0);
        if (raw[10] === 121) {
            farms += 1;
            assert.equal(get(raw, 26, 4), 73);
        }
    }
    assert.ok(farms > 0 && farms <= 60);
    assert.equal(core.kernel.size, 0);
    assert.equal(normal.pool, parent.pool);
    world.dispose();
});

// 연결 입력의 누락/다른 풀은 부모 실행 전에 거부한다. (2026-11-11 WB04-E303)
test('Fort 월드 적재 연결: 누락 선택과 다른 월드는 거부', () => {
    const scene = new FortBindingScene();
    const other = new FortBindingScene();
    const noop = () => {};
    const objects = { pool: scene.core.pool, load: noop };
    const meta = { pool: scene.core.pool, loadBadgesSelected: noop, loadTechnologySelected: noop };
    const money = { loadSelected: noop };
    assert.throws(() => makeFortWorldLoadBinding(scene.world, other.normal, objects, money, meta, noop, noop, noop, {}), /정상 월드/);
    assert.throws(() => makeFortWorldLoadBinding(scene.world, scene.normal, objects, money, meta, null, noop, noop, {}), /Money 선택 누락/);
    assert.throws(() => makeFortWorldLoadBinding(scene.world, scene.normal, { pool: other.core.pool, load: noop }, money, meta, noop, noop, noop, {}), /풀/);
    scene.world.dispose();
    other.world.dispose();
});

// 훅 배선: generate 는 builder 의 owner/경계와 호출 때의 현재 선택 지역을 정상 지형 상위에 그대로 넘기고, money 는 현재 Template/owner 로 선택을 지연하며, rebuildGraph 는 월드 Graph 의 인자 그대로이다. (2026-11-11 WB04-E303)
test('Fort 월드 적재 연결: 훅 배선', () => {
    const scene = new FortBindingScene();
    const calls = [];
    const noop = () => {};
    const normal = { pool: scene.core.pool, generateOriginal: (...args) => { calls.push(args); } };
    const selected = [];
    const money = { loadSelected: (select) => { calls.push('begin'); select(); calls.push('end'); } };
    const hooks = makeFortWorldLoadBinding(scene.world, normal, { pool: scene.core.pool, load: noop }, money, { pool: scene.core.pool, loadBadgesSelected: noop, loadTechnologySelected: noop },
        (fort, owner) => { selected.push([fort, owner]); calls.push('select'); return {}; }, noop, noop, {});
    const bounds = { left: 1, top: 1, right: 15, bottom: 15 };
    hooks.generate({ owner: 1, bounds }, 22);
    assert.deepEqual(calls, [[1, bounds, 22]]);
    // Money 선택은 loadSelected 가 begin 한 뒤에만 호출된다.
    calls.length = 0;
    const fort = {};
    hooks.money(fort, 3);
    assert.deepEqual(calls, ['begin', 'select', 'end']);
    assert.deepEqual(selected, [[fort, 3]]);
    // Graph 재구성은 인자를 그대로 전한다: false 는 표를 지우지 않고 true 는 전체를 지운다.
    const record = scene.core.graph.records()[200];
    record.inUse = 1;
    hooks.rebuildGraph(false);
    assert.equal(record.inUse, 1);
    hooks.rebuildGraph(true);
    assert.equal(record.inUse, 0);
    scene.world.dispose();
});
