// 4-B 적재 통합 시험(WB04-E304): 실제 Tutorial 1(`\d\thewarbegins.fort`)의 Territory 로 실제 월드·정상 지형·섬 목록·Builder 부모를 돌려 지형과 섬 적재 결과를 독립 기대값과 대조한다.
//  - 지형: 적재가 만든 지면(97)/edgeFarm(121) 객체의 칸과 지역 번호로 65,536바이트 마스크(y × 256 + x, 빈 칸 255)를 만들면 독립 Python 재현(`terrain-expected.tsv`)·문서의 SHA-256, 지면 칸 수 1505, 영역별 954/551 과 같다.
//  - 섬/청크: 실제 `RawIslandBuilder` 가 놓은 청크 256칸(섬·방향·record·순번)과 섬 목록이 타입 모델 `IslandBuilder`(layoutFort)의 결과와 같다.
//  - 후처리: Graph 0 재구성, 깊이 1 복귀, 후처리 순서(generate → 저장 후처리 4종 → graph0), 모든 객체가 같은 풀의 실제 슬롯.
// 저장 객체/Money/Meta child 는 호출 순서만 기록하는 어댑터이다: Tutorial 1 의 저장 객체를 실제 child 로 적재하려면 파생 후처리 몸체와 Session 소유 장치가 필요해 4-D 의 Session 조립·session-dump 비교(L2)에서 확인한다.
// cpppj 쪽 값은 실행하지 않고 독립 기대값 표와 문서의 값만 읽는다. 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E304)
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import test from 'node:test';
import { parseFort } from '../www/assets/fort.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { createFortTypes } from '../www/assets/type-flags.js';
import { FIRST_ASSET_TYPE_NUMBER, TypeTable } from '../www/assets/type-table.js';
import { ChunkMap, layoutFort, pieceFrameCodes } from '../www/assets/territory.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { CarrierPostPopState, RawCarrierPostPop, makeCarrierPostPopHooks } from '../www/core/raw-carrier-post-pop.js';
import { createCanonPlacementPipelineState, RawCanonPlacementPipeline } from '../www/core/raw-canon-placement-pipeline.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { FarmClusterState } from '../www/core/raw-farm-cluster.js';
import { FortRandomPlacementState } from '../www/core/raw-fort-random-placement.js';
import { RawFortWorldLoad } from '../www/core/raw-fort-world-load.js';
import { makeFortWorldLoadBinding } from '../www/core/raw-fort-world-load-binding.js';
import { RawIslandRuntime, createFortFinishOwnerState, createRawIslandStorage } from '../www/core/raw-island-runtime.js';
import { RawNormalTerrainWorldBinding } from '../www/core/raw-normal-terrain-world-binding.js';
import { RawSurfaceWorld } from '../www/core/raw-surface-world.js';
import { TerrainContextGlobals } from '../www/core/raw-terrain-context.js';
import { TerrainStampState } from '../www/core/raw-terrain-stamp.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { readData } from './helpers.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { get } from './raw-support.mjs';

// 두 공식 맵 중 Tutorial 1 의 독립 기대값이다(docs/exe/terrain-and-bridges.md · terrain-expected.tsv): 마스크 SHA-256, 지면 칸 수와 영역별 칸 수. (2026-11-11 WB04-E304)
const FORT_NAME = '\\d\\thewarbegins.fort';
const MASK_SHA256 = 'b24af563875ef935aa1053d521a6abf8490c4ddea840ac1845b43223fd0436d1';
const CELLS = 1505;
const REGION_CELLS = { 0: 954, 1: 551 };

// 실제 타입 번호이다: 지면 isle(97)·edgeFarm(121)·puzzlePiece(107). (2026-11-11 WB04-E304)
const ISLE = 97;
const FARM = 121;
const PIECE = 107;

// SHA-256 16진 문자열이다. (2026-11-11 WB04-E304)
const sha256 = (bytes) => crypto.createHash('sha256').update(bytes).digest('hex');

// 실제 판독용 타입 표/프레임 코드/Tutorial 1 의 Territory 120BYTE 를 읽는다. (2026-11-11 WB04-E304)
function loadTutorial() {
    const archive = new TaffArchive(readData('netstorm.tarc'));
    const table = TypeTable.fromArchive(archive, decodeOriginalText);
    const fort = parseFort(archive.read(FORT_NAME), createFortTypes(table));
    return { table, fort, pieceCodes: pieceFrameCodes(table) };
}

// 실제 TYPE 188칸·프레임 코드·셰이프(발자국 크기)로 월드를 만든다. 타입마다 실제 자료의 프레임 수와 셀 크기를 쓰며 그림 크기는 합성이다. (2026-11-11 WB04-E304)
function makeWorld(table) {
    const real = realPlayerAssets();
    const types = real.types.map((record) => ({ ...record }));
    const codes = types.map((_, number) => {
        const definition = number >= FIRST_ASSET_TYPE_NUMBER ? table.definitions[number - FIRST_ASSET_TYPE_NUMBER] : null;
        return definition ? definition.clusters.map((cluster) => cluster.code) : [];
    });
    const frames = codes.map((list) => new FrameCodeTable(list));
    const shapes = types.map((_, number) => {
        const view = number >= FIRST_ASSET_TYPE_NUMBER ? real.view(number) : null;
        if (!view || view.block < 0 || view.frameCount < 1) return { frameCount: 0, loaded: false, frames: [] };
        const list = [];
        // 프레임마다 실제 셀 크기를 읽는다.
        for (let frame = 0; frame < view.frameCount; frame += 1) {
            const [cellWidth, cellHeight] = real.frameSize(number, frame);
            list.push({ width: 20, height: 16, hotspotX: 8, hotspotY: 12, cellWidth, cellHeight });
        }
        return { frameCount: view.frameCount, loaded: true, frames: list };
    });
    const links = new BridgeConnectState();
    Object.assign(links, { bridgeType: 82, islandType: 94, stalagType: 95, connectorType: 155, noIslandType: 157, battle: true });
    return { world: new RawSurfaceWorld(types, frames, shapes, links, ISLE), types };
}

// 기대값에 쓰지 않는 하위 효과는 호출 순서만 기록하는 어댑터로 대신한다(Tutorial 1 의 저장 객체 적재는 4-D). (2026-11-11 WB04-E304)
function recordingChildren(core, effects) {
    const noop = () => {};
    return {
        objects: { pool: core.pool, load: () => { effects.push('objects'); } },
        meta: { pool: core.pool, loadBadgesSelected: (select) => { select(); }, loadTechnologySelected: (select) => { select(); } },
        money: { loadSelected: (select) => { select(); } },
        select: { money: () => { effects.push('money'); return {}; }, meta: noop, stream: () => { return {}; } },
    };
}

// 실제 Tutorial 1 을 적재해 지형 마스크와 섬/청크를 독립 기대값과 대조한다. (2026-11-11 WB04-E304)
test('4-B 적재 통합: 실제 Tutorial 1 의 지형 마스크·섬·청크', () => {
    const { table, fort, pieceCodes } = loadTutorial();
    assert.equal(fort.territory.length, 120);
    const { world } = makeWorld(table);
    const core = world.core();
    const fail = () => { throw new Error('통합 시험의 미복원 하위 효과 요청'); };
    const chunks = new ChunkMap();
    const islandBytes = createRawIslandStorage();
    const islands = new RawIslandRuntime(core.pool, core.hash, islandBytes, createFortFinishOwnerState(), new SquidPostPopList(), { value: 0 }, { value: 0 }, {
        setResidenceOwner: (sid, player) => core.owner.set(sid, player),
        assertion: fail,
    });
    islands.reset();
    const random = new GameRandom(0x12345678);
    const crt = { value: 7 };
    const hp = new SquidReward(core.pool, core.types, core.bookkeeping, new SquidRewardState(), new ScrambledSpStore(random, () => 0x12345678));
    const plain = core.types.map((_, type) => ({ frames: core.frames[type].codes, defaultFrame: 0 }));
    const pipelineState = createCanonPlacementPipelineState();
    pipelineState.geometry.patternTypes = [107, 82, 94, 157, 131, 129, 140, 142];
    pipelineState.placement.localPlayer = 1;
    pipelineState.permission.graphReady = 0;
    pipelineState.permission.alliances.fill(1);
    const pipeline = new RawCanonPlacementPipeline(core.pool, core.hash, { types: core.types, frames: plain, shapes: core.shapes, hotspots: core.types.map(() => ({ x: 0, y: 0 })), spots: core.spots, islands: core.hash.view(0), regions: [] },
        core.bookkeeping, new SquidPostPopList(), new ContainedFinderState(), pipelineState);
    const carrierState = new CarrierPostPopState();
    carrierState.loadingDepth = 1;
    const carrier = new RawCarrierPostPop(core.pool, core.types, carrierState, makeCarrierPostPopHooks(core.pool, core.postPop, fail, (x, y, owner) => world.setSupportOwner(x, y, owner)));
    const piece = { value: PIECE };
    const normal = new RawNormalTerrainWorldBinding(world, chunks, islands, random, crt, new TerrainContextGlobals(), new TerrainStampState(), new FortRandomPlacementState(), new FarmClusterState(), piece,
        pipeline, hp, carrier, { context: fail, stamp: fail, region: fail, farm: fail, postpass: fail });
    normal.bindGenerationInputs({
        enabled: () => 0, battle: () => core.ownerMode.battle, mission: () => core.bridgeState.mission, layout: () => 0,
        span: { value: 1 }, minimum: { value: 15 }, maximum: { value: 35 }, percent: { value: 0 }, archipelago: { value: 0 },
    });
    const effects = [];
    const children = recordingChildren(core, effects);
    const territory = { bytes: Uint8Array.from(fort.territory), position: 0, end: 120 };
    const template = {};
    const base = {
        templatePresent: () => true, territory: () => territory, currentTemplate: () => template,
        assertion: fail, builderAssertion: fail, populateIsland: fail,
    };
    const hooks = makeFortWorldLoadBinding(world, normal, children.objects, children.money, children.meta, children.select.money, children.select.meta, children.select.stream, base);
    const generate = hooks.generate;
    const rebuild = hooks.rebuildGraph;
    hooks.generate = (state, region) => { effects.push('generate'); generate(state, region); };
    hooks.rebuildGraph = (reset) => { effects.push('graph0'); rebuild(reset); };
    world.beginLoad();
    const parent = new RawFortWorldLoad(core.pool, chunks, islands, core.frames.map((table2) => table2.codes), piece, { value: 1 }, { value: 127 }, hooks);
    parent.load(0);
    // 지형: 지면/edgeFarm 객체의 칸과 지역 번호로 만든 마스크가 독립 기대값과 같다.
    const mask = new Uint8Array(65536).fill(255);
    const counts = {};
    let cells = 0;
    // 살아 있는 지면 계열 객체만 마스크 칸으로 센다(fringe 등 다른 객체는 제외).
    for (const item of world.objects()) {
        const raw = core.pool.slot(item.sid);
        if (raw[10] !== ISLE && raw[10] !== FARM) continue;
        const region = get(raw, 8, 2);
        mask[item.object.y * 256 + item.object.x] = region;
        counts[region] = (counts[region] ?? 0) + 1;
        cells += 1;
    }
    assert.equal(cells, CELLS);
    assert.deepEqual(counts, REGION_CELLS);
    assert.equal(sha256(mask), MASK_SHA256);
    // 섬/청크: 실제 Builder 의 청크 256칸과 섬 수가 타입 모델 결과와 같다.
    const layout = layoutFort(fort, pieceCodes);
    assert.equal(islands.count, 2);
    // 16×16 청크 전체를 같은 좌표끼리 비교한다.
    for (let x = 0; x < 16; x += 1) {
        // 한 열의 16개 청크이다.
        for (let y = 0; y < 16; y += 1) {
            const raw = chunks.at(x, y);
            const typed = layout.map.at(x, y);
            const side = (value) => (typeof value === 'number' ? String.fromCharCode(value) : value);
            assert.deepEqual([raw.island, side(raw.side), Array.from(raw.record), raw.ordinal], [typed.island, typed.side, Array.from(typed.record), typed.ordinal], `청크 ${x},${y}`);
        }
    }
    // 후처리 순서와 깊이: generate → 저장 후처리 → graph0, 깊이는 1 로 돌아온다.
    assert.deepEqual(effects, ['generate', 'objects', 'money', 'graph0']);
    assert.equal(core.kernel.size, 0);
    assert.equal(core.bookkeeping.depth, 0);
    world.dispose();
});
