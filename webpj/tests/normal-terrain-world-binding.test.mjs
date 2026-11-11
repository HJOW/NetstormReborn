// 일반 지형 소비자(cpppj `client/RawNormalTerrainWorldBinding`)의 웹 대응 `core/raw-normal-terrain-world-binding.js` 를 검사한다(4-B, WB04-E302).
// cpppj `NormalTerrainWorldBindingTests.cpp` 의 세 장면을 10.78 로 옮겼다:
//  - 같은 Factory/Pool/공간/지역/장부로 두 섬을 실제 성장·도장·군집 후처리하며 CRT 99 캐시는 첫 요청만 소비한다(독립 FrameAt 사전 관찰과 난수·CRT·캐시 대조).
//  - 새 배치 분기는 실제 121 생성/HP/게임 프레임 난수/현재 섬 owner 를 같은 공간·장부에 쓴다.
//  - 필수 효과/다른 월드/미복원 분기를 생성 전 거부한다.
// 입력은 합성 지면(97)·edgeFarm(121)·puzzlePiece(107) 타입과 합성 SHP 이며 원본 관측 파일이 없는 단위 계약 검사이다. 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E302)
import assert from 'node:assert/strict';
import test from 'node:test';
import { ChunkMap } from '../www/assets/territory.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { CarrierPostPopState, RawCarrierPostPop, makeCarrierPostPopHooks } from '../www/core/raw-carrier-post-pop.js';
import { createCanonPlacementPipelineState, RawCanonPlacementPipeline } from '../www/core/raw-canon-placement-pipeline.js';
import { FarmClusterState } from '../www/core/raw-farm-cluster.js';
import { FortRandomPlacementState, RawFortRandomPlacement } from '../www/core/raw-fort-random-placement.js';
import { RawIslandRuntime, createFortFinishOwnerState, createRawIslandStorage } from '../www/core/raw-island-runtime.js';
import { RawNormalTerrainWorldBinding, NormalTerrainBranch, makeNormalTerrainFarmWorldHooks } from '../www/core/raw-normal-terrain-world-binding.js';
import { RawSurfaceWorld } from '../www/core/raw-surface-world.js';
import { RawTerrainContext, TerrainContextGlobals } from '../www/core/raw-terrain-context.js';
import { RawTerrainGrowth } from '../www/core/raw-terrain-growth.js';
import { RawTerrainStamp, TerrainStampState } from '../www/core/raw-terrain-stamp.js';
import { makeTerrainStampWorldHooks } from '../www/core/raw-terrain-world-binding.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { realPlayerAssets } from './player-scene-support.mjs';
import { get } from './raw-support.mjs';

// 실제 원본 타입 번호(지면 97·edgeFarm 121·puzzlePiece 107)와 10.78 의 base/121 가상 표 주소이다. (2026-11-11 WB04-E302)
const ISLE = 97;
const FARM = 121;
const PIECE = 107;
const BASE_VTABLE = 0x501dd0;
const FARM_VTABLE = 0x50a8e8;

// 두 실제 섬과 수명이 긴 원본 전역/배치/HP/후처리를 가진 같은 실제 월드이다(cpppj `NormalWorldScene`). (2026-11-11 WB04-E302)
class NormalWorldScene {
    // randomFarm 이 참이면 121 의 flags1 이 무작위 프레임 분기(0x00100012)를 고르고, 아니면 지면 프레임 교체 분기(0x00200012)이다.
    constructor(randomFarm = false, islandCount = 2) {
        const types = realPlayerAssets().types.map((record) => ({ ...record }));
        // 각 16 방향의 16 후보는 실제 fallback 과 패치 quartile 의 최소 8 후보 조건을 함께 만족한다.
        const terrainCodes = [];
        // 16방향 각각에 16 후보를 넣는다.
        for (let side = 0x41; side <= 0x50; side += 1) {
            // 같은 방향의 16 변형이다(원본 시험도 변형 글자는 'A' 로 고정).
            for (let variant = 0x41; variant <= 0x50; variant += 1) terrainCodes.push({ side, variant: 0x41, number: 1, flags: 0 });
        }
        const codes = types.map(() => []);
        codes[ISLE] = terrainCodes;
        codes[FARM] = terrainCodes.slice();
        // 영역 패턴 칸의 실제 회전된 방향·P 변형·번호 1 검색을 지원한다.
        for (let side = 0x41; side <= 0x50; side += 1) codes[PIECE].push({ side, variant: 0x50, number: 1, flags: 0 });
        const frames = codes.map((list) => new FrameCodeTable(list));
        const shapes = types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
        // 지면과 121 은 실제 원본 생성자를 사용하며 121 의 Damageable 가상표를 base 로 대신하지 않는다.
        for (const type of [ISLE, FARM]) {
            const record = types[type];
            record.constructorAddress = typeConstructorAddress(type);
            record.flags1 = type === ISLE ? 0x28000803 : randomFarm ? 0x00100012 : 0x00200012;
            record.flags2 = type === ISLE ? 2 : 0x80000;
            record.footX = 1;
            record.footY = 1;
            record.maxHitPoints = 73;
            shapes[type] = { frameCount: 256, loaded: true, frames: Array.from({ length: 256 }, () => ({ width: 20, height: 16, hotspotX: 8, hotspotY: 12, cellWidth: 1, cellHeight: 1 })) };
        }
        // 원본 시험의 TYPE 입력은 지면/121 만 실제 생성자를 가지므로 다른 타입의 실제 생성자 주소는 비워 둔다(122 가 연결 누락으로 거부되어야 함).
        for (const type of [PIECE, 122]) types[type].constructorAddress = 0;
        // 영역 패턴 칸의 puzzlePiece 는 합성 기본 속성만 가진다.
        types[PIECE].footX = 1;
        types[PIECE].footY = 1;
        const links = new BridgeConnectState();
        Object.assign(links, { bridgeType: 82, islandType: 94, stalagType: 95, connectorType: 155, noIslandType: 157, battle: true });
        this.world = new RawSurfaceWorld(types, frames, shapes, links, ISLE);
        const core = this.world.core();
        this.diagnostics = 0;
        this.chunks = new ChunkMap();
        this.islandBytes = createRawIslandStorage();
        this.modes = createFortFinishOwnerState();
        this.residences = new SquidPostPopList();
        this.islands = new RawIslandRuntime(core.pool, core.hash, this.islandBytes, this.modes, this.residences, { value: 0 }, { value: 0 }, {
            setResidenceOwner: (sid, player) => core.owner.set(sid, player),
            assertion: () => { this.diagnostics += 1; },
        });
        this.islands.reset();
        assert.equal(this.islands.add(2, 3), 0);
        if (islandCount > 1) assert.equal(this.islands.add(3, 4), 1);
        // 실제 섬은 서로 다른 청크/원래 owner/현재 owner 를 유지한다.
        for (let region = 0; region < islandCount; region += 1) {
            const chunk = this.chunks.at(3 + region, 3);
            chunk.island = region;
            chunk.side = 'A';
            chunk.record = Uint8Array.of(3, 1, 0, 7 + region * 13, 0, 0);
        }
        // 기존 pipeline 의 래퍼형 프레임은 입력이 동일하며 현재 SHP/spots/hash/타입은 실제 Core 를 빌린다.
        const plainFrames = types.map((_, type) => ({ frames: core.frames[type].codes, defaultFrame: 0 }));
        const hotspots = types.map(() => ({ x: 0, y: 0 }));
        this.pipelineState = createCanonPlacementPipelineState();
        this.pipelineState.geometry.patternTypes = [107, 82, 94, 157, 131, 129, 140, 142];
        this.pipelineState.placement.localPlayer = 1;
        this.pipelineState.permission.graphReady = 0;
        this.pipelineState.permission.alliances.fill(1);
        this.pipeline = new RawCanonPlacementPipeline(core.pool, core.hash, { types: core.types, frames: plainFrames, shapes: core.shapes, hotspots, spots: core.spots, islands: core.hash.view(0), regions: [] },
            core.bookkeeping, new SquidPostPopList(), new ContainedFinderState(), this.pipelineState);
        this.random = new GameRandom(0x12345678);
        this.crt = { value: 7 };
        this.globals = new TerrainContextGlobals();
        this.stamp = new TerrainStampState();
        this.placement = new FortRandomPlacementState();
        this.farm = new FarmClusterState();
        this.puzzlePiece = { value: PIECE };
        const hpState = new SquidRewardState();
        this.sp = new ScrambledSpStore(this.random, () => 0x12345678);
        this.hp = new SquidReward(core.pool, core.types, core.bookkeeping, hpState, this.sp);
        const carrierState = new CarrierPostPopState();
        carrierState.loadingDepth = 1;
        this.carrier = new RawCarrierPostPop(core.pool, core.types, carrierState,
            makeCarrierPostPopHooks(core.pool, core.postPop, () => { throw new Error('121은 Carrier +cc를 요청하지 않는다'); },
                (x, y, owner) => this.world.setSupportOwner(x, y, owner)));
        this.binding = new RawNormalTerrainWorldBinding(this.world, this.chunks, this.islands, this.random, this.crt, this.globals, this.stamp,
            this.placement, this.farm, this.puzzlePiece, this.pipeline, this.hp, this.carrier, this.makeDiagnostics());
    }

    // 진단의 실제 요청 여부를 관찰하며 누락 효과를 기본값으로 채우지 않는다. (2026-11-11 WB04-E302)
    makeDiagnostics() {
        const count = () => { this.diagnostics += 1; };
        return { context: count, stamp: count, region: count, farm: count, postpass: count };
    }

    // 예상 결과를 생성하는 모형 없이 기존 frameAt 만 호출해 같은 게임/CRT 난수 소비를 사전 관찰한다. (2026-11-11 WB04-E302)
    expectedFrames(request, expected, expectedCrt, expectedState) {
        const core = this.world.core();
        const context = new RawTerrainContext('patch1078', request.owner, request.selectedRegion, request.bounds, this.globals, () => { this.diagnostics += 1; });
        new RawTerrainGrowth(context, this.chunks, this.globals).runNormal();
        context.markEdges(0x78, 0x3d);
        const effects = { chunkRegion: (x, y) => (this.chunks.at(x, y).island << 24) >> 24, assertion: () => { this.diagnostics += 1; } };
        const chooser = new RawTerrainStamp(core.pool, core.types, core.frames, context.map, expected, expectedCrt, expectedState,
            makeTerrainStampWorldHooks(this.world, expectedState, () => { this.diagnostics += 1; }, effects));
        const map = context.map;
        let count = 0;
        // 실제 초기 도장의 Y/X 순서에서 매 실제 지면 프레임 요청만 관찰한다.
        for (let y = map.bounds[1]; y < map.bounds[3]; y += 1) {
            // 생성/Pop 없이 실제 frameAt 이 게임/CRT 저장소만 갱신한다.
            for (let x = map.bounds[0]; x < map.bounds[2]; x += 1) {
                const code = context.codes[x * 256 + y];
                if (map.aliases[code] === 0x78 && map.typeMapping[code]) {
                    chooser.frameAt(x, y);
                    count += 1;
                }
            }
        }
        return count;
    }
}

// 같은 Factory/Pool/공간/지역/장부로 두 섬을 실제 성장·도장·군집 후처리하며 캐시는 첫 요청만 소비한다. (2026-11-11 WB04-E302)
test('일반 지형 월드 연결: 실제 성장·도장·군집 후처리', () => {
    const scene = new NormalWorldScene();
    const core = scene.world.core();
    scene.world.beginLoad();
    let total = 0;
    let farmCount = 0;
    assert.equal(scene.binding.pool, core.pool);
    assert.equal(scene.hp.bookkeeping, core.bookkeeping);
    // 첫/두 번째 요청의 CRT cache 는 하나이며 선택 지역만 바뀐다.
    for (let region = 0; region < 2; region += 1) {
        const request = { owner: 1, bounds: { left: 3, top: 3, right: 5, bottom: 4 }, selectedRegion: region, branch: NormalTerrainBranch.Normal };
        const expected = new GameRandom(scene.random.state);
        const expectedCrt = { value: scene.crt.value };
        const expectedState = Object.assign(new TerrainStampState(), scene.stamp);
        const count = scene.expectedFrames(request, expected, expectedCrt, expectedState);
        assert.ok(count > 0);
        total += count;
        const previousCrt = scene.crt.value;
        const last = scene.binding.generate(request);
        assert.ok(last !== 0);
        assert.equal(scene.random.state, expected.state);
        assert.equal(scene.crt.value, expectedCrt.value);
        assert.deepEqual(scene.stamp.variants, expectedState.variants);
        assert.equal(region === 0 ? scene.crt.value !== previousCrt : scene.crt.value === previousCrt, true);
    }
    const objects = scene.world.objects();
    assert.equal(objects.length, total);
    assert.equal(scene.diagnostics, 0);
    // 실제 생성한 모든 SID 의 지면/121 가상표와 현재 hash 단계·지역 WORD·HP 를 확인한다.
    for (const object of objects) {
        const raw = core.pool.slot(object.sid);
        assert.ok(get(raw, 8, 2) < 2);
        assert.equal(object.object.owner, 0);
        assert.ok(object.object.x >= 48 && object.object.x < 80 && object.object.y >= 48 && object.object.y < 64);
        assert.equal(get(raw, 0, 4), raw[10] === FARM ? FARM_VTABLE : BASE_VTABLE);
        assert.equal(scene.world.find(object.object.x, object.object.y, raw[10]), object.sid);
        // 일반 해시는 여러 SID 를 한 머리에 연결하므로 대상이 머리인지를 요구하지 않고 같은 버킷 체인에 있는지를 본다.
        const level = raw[33];
        let found = false;
        let inspected = 0;
        // 같은 버킷의 SID 체인을 순회 한도 안에서 따라간다.
        for (let current = core.hash.bucket(level, object.object.x, object.object.y); current && inspected < core.pool.capacity; current = get(core.pool.slot(current), 4, 2)) {
            inspected += 1;
            if (current === object.sid) { found = true; break; }
        }
        assert.ok(found);
        if (raw[10] === FARM) {
            assert.equal(get(raw, 26, 4), 73);
            farmCount += 1;
        } else assert.equal(raw[10], ISLE);
    }
    assert.ok(farmCount > 0);
    assert.equal(core.bookkeeping.globalCounts()[ISLE], total);
    assert.deepEqual([core.bookkeeping.depth, scene.pipelineState.placement.forcePlacement], [0, 0]);
    scene.world.finishLoad();
    assert.deepEqual([scene.islands.owner(0), scene.islands.owner(1)], [2, 3]);
    scene.world.dispose();
});

// 새 배치 분기는 실제 121 생성/HP/게임 프레임 난수/현재 섬 owner 를 같은 공간·장부에 쓴다. (2026-11-11 WB04-E302)
test('일반 지형 월드 연결: 121 생성·owner·HP·게임 난수', () => {
    const scene = new NormalWorldScene(true);
    const core = scene.world.core();
    scene.world.beginLoad();
    const context = new RawTerrainContext('patch1078', 1, 0, { left: 3, top: 3, right: 4, bottom: 4 }, scene.globals, () => {});
    context.codes[50 * 256 + 53] = 0x78;
    context.regions[50 * 256 + 53] = 0;
    const effects = { chunkRegion: (x, y) => (scene.chunks.at(x, y).island << 24) >> 24, assertion: () => { scene.diagnostics += 1; } };
    const stamp = new RawTerrainStamp(core.pool, core.types, core.frames, context.map, scene.random, scene.crt, scene.stamp,
        makeTerrainStampWorldHooks(scene.world, scene.stamp, () => { scene.diagnostics += 1; }, effects));
    stamp.stamp(0x78);
    assert.notEqual(context.map.lastSid.value, 0);
    const single = new RawFortRandomPlacement(core.pool, core.hash, core.spots, core.types, core.frames, scene.random, scene.placement,
        makeNormalTerrainFarmWorldHooks(scene.world, scene.chunks, scene.islands, scene.pipeline, scene.hp));
    const local = new GameRandom(77);
    const expected = new GameRandom(scene.random.state);
    const expectedFrame = 240 + expected.next(16);
    const previousCrt = scene.crt.value;
    const created = single.place(local, { type: FARM, x: 50, y: 53, region: 0, mode: 0, force: 0 });
    assert.notEqual(created, 0);
    const raw = core.pool.slot(created);
    assert.equal(scene.world.find(50, 53, FARM), created);
    assert.deepEqual([scene.random.state, local.state, scene.crt.value], [expected.state, 77, previousCrt]);
    assert.equal(get(raw, 36, 4), expectedFrame);
    // 소유자는 현재 섬 owner(2), HP 는 타입 HP(73)이다.
    assert.deepEqual([raw[34], get(raw, 26, 4)], [2, 73]);
    assert.deepEqual([core.bookkeeping.globalCounts()[FARM], core.bookkeeping.totalCost, core.bookkeeping.depth], [1, 0, 0]);
    assert.equal(scene.diagnostics, 0);
    assert.equal(scene.pipeline.pool, core.pool);
    scene.world.finishLoad();
    scene.world.dispose();
});

// 필수 효과/다른 월드/미복원 분기를 생성 전 거부하며 원본을 기본 생성자/허용값으로 대신하지 않는다. (2026-11-11 WB04-E302)
test('일반 지형 월드 연결: 누락 효과와 미복원 분기는 생성 전 거부', () => {
    const scene = new NormalWorldScene();
    const core = scene.world.core();
    const before = scene.world.objects().length;
    const random = scene.random.state;
    const crt = scene.crt.value;
    const bounds = { left: 3, top: 3, right: 4, bottom: 4 };
    assert.throws(() => scene.binding.generate({ owner: 1, bounds, selectedRegion: 0, branch: NormalTerrainBranch.Archipelago }), /군도\/전체 부모/);
    assert.throws(() => scene.binding.generate({ owner: 1, bounds, selectedRegion: 0, branch: NormalTerrainBranch.UnrestoredParent }), /군도\/전체 부모/);
    // 군도 전역이나 잘못된 edgeFarm 번호는 효과 전에 거부한다.
    scene.farm.archipelago = true;
    assert.throws(() => scene.binding.postprocessFarms(0), /군도 상태/);
    scene.farm.archipelago = false;
    scene.placement.edgeFarmType = 122;
    assert.throws(() => scene.binding.postprocessFarms(0), /생성자 연결 누락/);
    scene.placement.edgeFarmType = FARM;
    const missing = scene.makeDiagnostics();
    missing.region = undefined;
    assert.throws(() => new RawNormalTerrainWorldBinding(scene.world, scene.chunks, scene.islands, scene.random, scene.crt, scene.globals, scene.stamp, scene.placement,
        scene.farm, scene.puzzlePiece, scene.pipeline, scene.hp, scene.carrier, missing), /필수 진단 누락/);
    const other = new NormalWorldScene();
    assert.throws(() => makeNormalTerrainFarmWorldHooks(scene.world, scene.chunks, other.islands, scene.pipeline, scene.hp), /섬\/HP 장부 불일치/);
    assert.throws(() => makeNormalTerrainFarmWorldHooks(scene.world, scene.chunks, scene.islands, other.pipeline, scene.hp), /pool 불일치/);
    assert.deepEqual([scene.world.objects().length, scene.random.state, scene.crt.value, scene.stamp.variants], [before, random, crt, null]);
    // 패턴 번호 3 은 세 칸(회전해도 같음)이며 번호 범위 밖은 거부한다.
    const record = Uint8Array.of(3, 1, 0, 7, 0, 0);
    assert.equal(scene.binding.patternCount(record), 3);
    const rotated = Uint8Array.from(record);
    rotated[0] |= 0xc0;
    assert.equal(scene.binding.patternCount(rotated), 3);
    assert.equal(scene.binding.pool, core.pool);
    scene.world.dispose();
    other.world.dispose();
});

// 군도 후보 상위(004c1510): 후보 조건이 참이고 layout 이 0 이면 확장 범위·흩뿌리기·목표 넓이·플레이어 배치·gap 정리를 거친 뒤 군도 DWORD 를 켠다. 원본 관측 전체가 없는 단계라 기대값은 입력에서 정해지는 관계식이다. (2026-11-11 WB04-E302)
test('일반 지형 월드 연결: 군도 후보 상위의 확장·백분율·플레이어 배치', () => {
    const scene = new NormalWorldScene(false, 1);
    const core = scene.world.core();
    scene.world.beginLoad();
    const calls = [];
    const inputs = {
        enabled: () => 1, battle: () => 1, mission: () => 0, layout: () => 0,
        spacing: () => { calls.push('spacing'); return 4; },
        span: { value: 9 }, minimum: { value: 15 }, maximum: { value: 35 }, percent: { value: 25 }, archipelago: { value: 0 },
        subscriber: (player) => { calls.push(`subscriber${player}`); return player === 1 ? 7 : 0; },
        placement: (player, record) => { calls.push(`placement${player}`); assert.deepEqual(Array.from(record), [7, 1, 0, 0, 0, 0]); return { left: 3, top: 3, right: 4, bottom: 4 }; },
        logPercent: (percent) => { calls.push(`percent${percent}`); },
    };
    scene.binding.bindGenerationInputs(inputs);
    const last = scene.binding.generateOriginal(1, { left: 3, top: 3, right: 5, bottom: 4 }, 127);
    // spacing 은 두 번 따로 조회하고 4 이면 span 2 이다. 백분율 25 는 넓이 25% 목표의 환산 25 로 돌아온다.
    assert.deepEqual(calls.filter((name) => name === 'spacing'), ['spacing', 'spacing']);
    assert.equal(inputs.span.value, 2);
    assert.ok(calls.includes('percent25'));
    // 플레이어 순회는 1..8 을 모두 읽고 구독자가 있는 플레이어만 배치 장치를 부른다.
    assert.deepEqual(calls.filter((name) => name.startsWith('subscriber')), Array.from({ length: 8 }, (_, index) => `subscriber${index + 1}`));
    assert.deepEqual(calls.filter((name) => name.startsWith('placement')), ['placement1']);
    // 군도 DWORD 는 도장/정규화 뒤 켜지고 군집이 같은 상자를 읽는다(이후 일반 전용 요청은 거부).
    assert.equal(inputs.archipelago.value, 1);
    assert.throws(() => scene.binding.postprocessFarms(0), /군도 상태/);
    assert.ok(last !== 0);
    assert.ok(scene.world.objects().length > 0);
    assert.equal(core.bookkeeping.depth, 0);
    // 같은 상자를 다른 상자로 다시 연결하는 시도는 거부한다.
    assert.throws(() => scene.binding.bindGenerationInputs({ ...inputs, archipelago: { value: 0 } }), /같은 군도\/확장 owner/);
    assert.throws(() => scene.binding.bindGenerationInputs({ ...inputs, span: { value: 0 } }), /같은 군도\/확장 owner/);
    scene.world.dispose();
});

// 요청 범위와 전역/타입 연결 오류는 효과(생성·난수·월드 쓰기) 전에 거부한다. 한 번에 하나의 조건만 틀리게 하여 각 검사를 따로 확인한다. (2026-11-11 WB04-E302)
test('일반 지형 월드 연결: 범위와 전역 연결 오류는 효과 전에 거부', () => {
    const scene = new NormalWorldScene();
    const before = [scene.world.objects().length, scene.random.state, scene.crt.value];
    const request = (bounds) => ({ owner: 1, bounds, selectedRegion: 0, branch: NormalTerrainBranch.Normal });
    // 청크 범위는 내부 1..15 이고 right/bottom 이 left/top 보다 커야 한다.
    for (const bounds of [{ left: 0, top: 3, right: 4, bottom: 4 }, { left: 3, top: 0, right: 4, bottom: 4 }, { left: 3, top: 3, right: 16, bottom: 4 },
        { left: 3, top: 3, right: 4, bottom: 16 }, { left: 4, top: 3, right: 4, bottom: 4 }, { left: 3, top: 4, right: 4, bottom: 4 }]) {
        assert.throws(() => scene.binding.generate(request(bounds)), RangeError, JSON.stringify(bounds));
    }
    const good = { left: 3, top: 3, right: 4, bottom: 4 };
    // 군도 전역은 일반 요청을 막는다.
    scene.farm.archipelago = true;
    assert.throws(() => scene.binding.generate(request(good)), /군도 상태/);
    scene.farm.archipelago = false;
    // 서로 다른 전역: 각각 하나만 어긋나게 한다.
    const mismatches = [
        () => { scene.stamp.islandType = 96; }, () => { scene.placement.islandType = 96; }, () => { scene.stamp.edgeType = 99; }, () => { scene.stamp.noRegion = 100; }, () => { scene.globals.noRegion = 100; },
    ];
    const restore = () => { scene.stamp.islandType = 97; scene.placement.islandType = 97; scene.stamp.edgeType = 98; scene.stamp.noRegion = 127; scene.globals.noRegion = 127; };
    // 전역을 하나씩 어긋나게 한 뒤 생성과 후처리 모두 거부되는지 본다.
    for (const mismatch of mismatches) {
        mismatch();
        assert.throws(() => scene.binding.generate(request(good)), /같은 타입\/지역 전역/);
        assert.throws(() => scene.binding.postprocessFarms(0), /같은 타입\/지역 전역/);
        restore();
    }
    // 세 전역이 일치해도 지면 TYPE 의 실제 생성자 주소가 없으면 거부한다.
    scene.globals.islandType = 122;
    scene.stamp.islandType = 122;
    scene.placement.islandType = 122;
    assert.throws(() => scene.binding.generate(request(good)), /생성자 연결 누락/);
    scene.globals.islandType = 97;
    scene.stamp.islandType = 97;
    scene.placement.islandType = 97;
    // 구성도 같은 검사를 한다: 어긋난 전역으로 새 소비자를 만들 수 없다.
    scene.stamp.edgeType = 99;
    assert.throws(() => new RawNormalTerrainWorldBinding(scene.world, scene.chunks, scene.islands, scene.random, scene.crt, scene.globals, scene.stamp,
        scene.placement, scene.farm, scene.puzzlePiece, scene.pipeline, scene.hp, scene.carrier, scene.makeDiagnostics()), /같은 타입\/지역 전역/);
    scene.stamp.edgeType = 98;
    // 필수 진단 다섯과 CRT/puzzlePiece 상자, 다른 월드의 Carrier 는 구성에서 거부한다.
    for (const name of ['context', 'stamp', 'region', 'farm', 'postpass']) {
        const diagnostics = scene.makeDiagnostics();
        diagnostics[name] = undefined;
        assert.throws(() => new RawNormalTerrainWorldBinding(scene.world, scene.chunks, scene.islands, scene.random, scene.crt, scene.globals, scene.stamp,
            scene.placement, scene.farm, scene.puzzlePiece, scene.pipeline, scene.hp, scene.carrier, diagnostics), /필수 진단 누락/, name);
    }
    // CRT 상자와 puzzlePiece 상자를 각각 비워 구성 거부를 본다.
    for (const [crt, piece] of [[null, scene.puzzlePiece], [scene.crt, null]]) {
        assert.throws(() => new RawNormalTerrainWorldBinding(scene.world, scene.chunks, scene.islands, scene.random, crt, scene.globals, scene.stamp,
            scene.placement, scene.farm, piece, scene.pipeline, scene.hp, scene.carrier, scene.makeDiagnostics()), TypeError);
    }
    const other = new NormalWorldScene();
    assert.throws(() => new RawNormalTerrainWorldBinding(scene.world, scene.chunks, scene.islands, scene.random, scene.crt, scene.globals, scene.stamp,
        scene.placement, scene.farm, scene.puzzlePiece, scene.pipeline, scene.hp, other.carrier, scene.makeDiagnostics()), /필수 진단 누락/);
    assert.deepEqual([scene.world.objects().length, scene.random.state, scene.crt.value], before);
    // 상위 전역/장치가 연결되지 않으면 원본 상위를 대신하지 않는다.
    assert.throws(() => scene.binding.generateOriginal(1, good, 0), /상위의 현재 전역/);
    scene.world.dispose();
    other.world.dispose();
});

// 패턴 셈은 현재 puzzlePiece 타입의 프레임과 패턴 번호 범위를 읽고, 섬 목록에서 사라진 지역은 보고 뒤 계속한다. (2026-11-11 WB04-E302)
test('일반 지형 월드 연결: 패턴 셈의 범위와 사라진 섬 보고', () => {
    const scene = new NormalWorldScene();
    // 현재 puzzlePiece 가 프레임 표 밖이면 거부한다(패턴 번호는 하위 6비트라 표 68개 안에 항상 있어 번호 범위 검사는 도달 불가).
    scene.puzzlePiece.value = 188;
    assert.throws(() => scene.binding.patternCount(Uint8Array.of(3, 1, 0, 7, 0, 0)), RangeError);
    scene.puzzlePiece.value = PIECE;
    // 사라진 섬 0 은 Bounds 앞 Exists 진단을 한 번 보고하고 후처리를 계속한다.
    scene.world.beginLoad();
    scene.islands.delete(0);
    const before = scene.diagnostics;
    scene.binding.postprocessFarms(0);
    assert.equal(scene.diagnostics, before + 1);
    scene.world.dispose();
});

// 군집/단일 배치 훅의 월드 몸체: 청크 지역은 signed BYTE, 지역 owner 는 현재 섬 목록, 공간 효과는 월드의 Pop/Unpop/Repop 이다. (2026-11-11 WB04-E302)
test('일반 지형 월드 연결: 배치 훅은 월드 몸체로 간다', () => {
    const scene = new NormalWorldScene();
    const core = scene.world.core();
    const hooks = makeNormalTerrainFarmWorldHooks(scene.world, scene.chunks, scene.islands, scene.pipeline, scene.hp);
    scene.chunks.at(3, 3).island = 200;
    assert.equal(hooks.chunkRegion(3, 3), -56);
    scene.chunks.at(3, 3).island = 0;
    assert.deepEqual([hooks.regionOwner(0), hooks.regionOwner(1)], [2, 3]);
    scene.world.beginLoad();
    const sid = hooks.create(ISLE, 0);
    hooks.setOwner(sid, 3);
    assert.equal(core.pool.slot(sid)[34], 3);
    hooks.pop(sid, 50, 53, 0x21);
    assert.equal(scene.world.find(50, 53, ISLE), sid);
    hooks.unpop(sid, 0);
    assert.equal(scene.world.find(50, 53, ISLE), 0);
    hooks.repop(sid, 0x40);
    assert.equal(scene.world.find(50, 53, ISLE), sid);
    // 다른 월드의 HP 장부는 효과 전에 거부한다.
    const other = new NormalWorldScene();
    assert.throws(() => makeNormalTerrainFarmWorldHooks(scene.world, scene.chunks, scene.islands, scene.pipeline, other.hp), /섬\/HP 장부 불일치/);
    scene.world.dispose();
    other.world.dispose();
});

// 군도 후보 조건은 네 조건이 모두 참이고 layout 이 0 일 때만 군도 분기를 탄다: 하나라도 거짓이면 일반 성장이며 군도 DWORD 와 spacing 조회가 일어나지 않는다. (2026-11-11 WB04-E302)
test('일반 지형 월드 연결: 군도 후보는 모든 조건과 layout 0 이 필요', () => {
    // [enabled, battle, mission, layout, 섬 수] 조합 중 하나만 후보에서 벗어나게 한다.
    for (const [enabled, battle, mission, layout, count] of [[0, 1, 0, 0, 1], [1, 0, 0, 0, 1], [1, 1, 1, 0, 1], [1, 1, 0, 1, 1], [1, 1, 0, 0, 2]]) {
        const scene = new NormalWorldScene(false, count);
        scene.world.beginLoad();
        let spacing = 0;
        const inputs = { enabled: () => enabled, battle: () => battle, mission: () => mission, layout: () => layout, spacing: () => { spacing += 1; return 3; },
            span: { value: 9 }, minimum: { value: 15 }, maximum: { value: 35 }, percent: { value: 0 }, archipelago: { value: 0 } };
        scene.binding.bindGenerationInputs(inputs);
        scene.binding.generateOriginal(1, { left: 3, top: 3, right: 5, bottom: 4 }, 127);
        assert.deepEqual([spacing, inputs.archipelago.value, inputs.span.value], [0, 0, 9], JSON.stringify([enabled, battle, mission, layout, count]));
        scene.world.dispose();
    }
});

// 상위 입력 묶음은 군도/확장 상자가 없으면 연결 전에 거부한다. (2026-11-11 WB04-E302)
test('일반 지형 월드 연결: 상위 입력의 상자 누락은 연결 전 거부', () => {
    const scene = new NormalWorldScene();
    assert.throws(() => scene.binding.bindGenerationInputs(null), TypeError);
    assert.throws(() => scene.binding.bindGenerationInputs({ archipelago: {}, span: { value: 1 } }), TypeError);
    assert.throws(() => scene.binding.bindGenerationInputs({ archipelago: { value: 0 }, span: {} }), TypeError);
    // 거부된 입력은 군집 전역에 연결되지 않아 이후 일반 전용 요청은 그대로 허용된다.
    assert.equal(scene.farm.isArchipelago(), false);
    scene.binding.postprocessFarms(0);
    scene.world.dispose();
});
