// 캠페인 건설/간헐천 연결 raw-campaign-construction-bindings.js(RawCampaignConstructionBindings)와 자료 묶음 priest-placement-assets.js(PriestPlacementAssets)를 cpppj 계약으로 대조한다(4-D C 구획, WB04-E562).
// cpppj 시험 대응(`CampaignConstructionTests.cpp` 의 ProtectedConstructionScene: 보호 TYPE 188/프레임/SHP 와 같은 실제 공간/삭제/간헐천 후처리):
//  - CampaignConstruction_ProtectedNoIslandPacketAndSelfNotice: 실제 서버 SID 9개·0x3f 가변 패킷·같은 공간 Pop 과 예측 조각 삭제의 종단 대조, 일반 TYPE 받침 확정 거부.
//  - CampaignConstruction_ProtectedLocalGeysersUseCurrentWorld: 현재 Player flags·실제 Bounds·전용 최근 5목록·같은 보상 pool·실제 Geyser 후처리, 공유 전역 불일치 거부.
// 웹 계약 추가: 구성 거부(저장소 정체성·같은 풀/장부·프레임 표·Player 9기록·전용 목록·소유자 모드·필수 서비스), validateState 의 전역 불일치 항목, 선택되지 않은 일반 건설 경계의 명시 오류,
//  0x3e/0x3f 등록과 통지 직접 수신, 표면/받침 삭제 분배(destroyHooks/destroy), PriestPlacementAssets 의 프레임·기준점·셰이프·패턴 타입.
// 기대값은 cpppj 시험과 같은 관계이며 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E562)
import assert from 'node:assert/strict';
import test from 'node:test';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { get, put } from './raw-support.mjs';
import { ChunkMap } from '../www/assets/territory.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { buildTypeRecords } from '../www/assets/type-flags.js';
import { PriestPlacementAssets } from '../www/core/priest-placement-assets.js';
import { RawAssetPostPop } from '../www/core/raw-asset-post-pop.js';
import { AssetPostPopState } from '../www/core/raw-asset-post-pop-state.js';
import { RawBattleBroadcast } from '../www/core/raw-battle-broadcast.js';
import { BridgeConnectState, ISLAND_VTABLE, STALAG_VTABLE } from '../www/core/raw-bridge-connect.js';
import { RawCampaignConstructionBindings, RawCampaignConstructionState, bindConstructionCore, bindConstructionRuntime, makePlayerGeyserBoundsState } from '../www/core/raw-campaign-construction-bindings.js';
import { CarrierPostPopState, RawCarrierPostPop, makeCarrierPostPopHooks } from '../www/core/raw-carrier-post-pop.js';
import { RawFormPacket, createFormDeletionStamp, createFormPacketHeaders } from '../www/core/raw-form-packet.js';
import { RawGeyserAiRegistration } from '../www/core/raw-geyser-ai-registration.js';
import { RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { RawSquidFinder } from '../www/core/raw-squid-finder.js';
import { RawSurfaceWorld } from '../www/core/raw-surface-world.js';
import { NO_ISLAND_VTABLE } from '../www/core/raw-surface-lifecycle.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { DirtyRegions } from '../www/render/dirty-regions.js';

// Player 한 칸 폭과 flags/지역 위치, 타입 번호이다. (2026-11-15 WB04-E562)
const STRIDE = 0xb4;
const PLAYER_FLAGS = 0x50;
const PLAYER_REGION = 0x78;
const NO_ISLAND = 157;
const GEYSER = 122;
const ISLAND = 94;
const STALAG = 95;
const STUMP = 96;
const BASE_VTABLE = 0x501dd0;

// 실제 미복원 효과를 기본 관찰/성공으로 채우지 않는다. (2026-11-15 WB04-E562)
const forbidden = (name) => () => { throw new Error(`보호 받침/간헐천 검사에서 선택되지 않아야 할 효과: ${name}`); };

// 보호 자료의 실제 TYPE 188·프레임 코드·사제 배치 자료 묶음이다. 한 번만 읽는다. (2026-11-15 WB04-E562)
let inputCache = null;
function inputs() {
    if (inputCache) return inputCache;
    const capture = realCaptureAssets();
    const records = buildTypeRecords(capture.table.definitions);
    const metadata = new PriestPlacementAssets(capture.table, capture.database, records);
    inputCache = { capture, records, metadata, types: realPlayerAssets().types };
    return inputCache;
}

// 장치 없이 실제 보호 자산/공간/후처리/삭제/송신 분배를 구성한다(cpppj ProtectedConstructionScene). 부모 로그인/미션 진행 검사는 별도다. (2026-11-15 WB04-E562)
class ProtectedConstructionScene {
    constructor(options = {}) {
        const { metadata, types } = inputs();
        this.metadata = metadata;
        this.types = types.map((record) => ({ ...record }));
        this.frames = metadata.frames.map((entry) => new FrameCodeTable(entry.frames));
        const links = new BridgeConnectState();
        Object.assign(links, { bridgeType: 82, islandType: 94, stalagType: 95, connectorType: 155, noIslandType: 157, battle: true });
        this.world = new RawSurfaceWorld(this.types, this.frames, metadata.shapes, links, 97);
        this.core = this.world.core();
        this.random = new GameRandom(0x12345678);
        this.sp = new ScrambledSpStore(this.random, () => 1);
        this.reward = new SquidRewardState();
        this.hp = new SquidReward(this.core.pool, this.core.types, this.core.bookkeeping, this.reward, this.sp);
        this.players = new Uint8Array(9 * STRIDE);
        this.peers = new Array(9).fill(null);
        this.localOwner = { value: 1 };
        this.server = { value: true };
        // 이 검사에서 선택한 비미션 전투 배치 입력과 현재 옵션 0, 영역 패턴 TYPE, 보호 PE 의 Player 0 값까지 보존한 방향표이다.
        this.source = { mission: false, mapSize: 0, puzzlePieceType: 107, playerDirections: [16448, 4, 0, 6, 2, 7, 3, 5, 1] };
        this.chunks = new ChunkMap();
        this.broadcast = new RawBattleBroadcast(this.players, this.peers, this.localOwner, this.server);
        this.formWriter = new RawFormPacket(this.core.pool, createFormPacketHeaders(), createFormDeletionStamp());
        this.formBuffer = new Uint8Array(2048);
        this.sent = [];
        this.carrierState = new CarrierPostPopState();
        this.assetState = new AssetPostPopState();
        this.ai = new RawGeyserAiRegistration(this.players);
        this.state = new RawCampaignConstructionState();
        this.recent = new SquidPostPopList();
        this.dirty = new DirtyRegions(1024, 768);
        this.predictable = 1;
        this.sp.initialize();
        this.recent.entries.length = 5;
        this.recent.entries.fill(0);
        this.assetState.geysers.entries.length = 100;
        this.assetState.geysers.entries.fill(0);
        this.assetState.inBattle = true;
        this.state.notice.localPlayer = 1;
        this.state.place.localPlayer = 1;
        // 공통 삭제는 기본 관찰 대신 같은 hp/SP/자원 pool 을 직접 갱신하고 삭제 패킷을 실제 writer 로 만든다.
        this.world.bindDeletion(this.hp, () => 0, {
            emit: forbidden('공통 삭제 UI 효과'),
            forward: (event) => {
                if (event.effect !== SQUID_DESTROY_EFFECT.transmit) throw new Error('보호 건설 검사 다른 삭제 파생 효과');
                const size = this.formWriter.writeDeletion(event.sid, event.flags, this.formBuffer);
                this.broadcast.send(this.formBuffer.subarray(0, size));
            },
        });
        this.world.setDisplay({ cameraX: 0, cameraY: 0, zoom: 65536, viewport: { left: 0, top: 0, right: 1024, bottom: 768 } }, {
            suppressed: () => this.dirty.fullRedraw,
            invalidate: (rectangle, flags) => { this.dirty.add({ left: rectangle.left, top: rectangle.top, right: rectangle.right, bottom: rectangle.bottom }, flags); },
        });
        this.carrier = new RawCarrierPostPop(this.core.pool, this.core.types, this.carrierState,
            makeCarrierPostPopHooks(this.core.pool, this.core.postPop, forbidden('Carrier 확인'), (x, y, owner) => { this.world.setSupportOwner(x, y, owner); }));
        // 이 장면이 선택하는 Geyser 하위를 모두 실제 몸체에 연결한다.
        this.assetPostPop = new RawAssetPostPop(this.core.pool, this.core.types, this.core.postPop, this.carrier, this.assetState, {
            reserveRegular: () => true,
            addRegular: (sid, event, payload) => {
                if (!this.core.processes.addRegular(sid, event, payload)) throw new Error('실제 Geyser Regular 부착 실패');
            },
            geyserRegistration: (sid, operation) => { this.ai.register(sid, operation); },
            atPosition: (x, y) => {
                const finder = new RawSquidFinder(this.core.pool, this.core.hash, this.core.types);
                const result = [];
                // 현재 해시 후보를 기존 일반 탐색기에서 순서대로 수집한다.
                for (let sid = finder.begin({ left: Math.trunc(x), top: Math.trunc(y), right: Math.trunc(x), bottom: Math.trunc(y) }); sid; sid = finder.next()) result.push(sid);
                return result;
            },
            unpop: (sid, flags) => { this.world.unpop(sid, flags); },
            repop: (sid, flags) => { this.world.repop(sid, flags); },
        });
        this.assetPostPop.bindPlayerRecords(this.players);
        this.core.postPop.setBody('asset', this.assetPostPop);
        this.bounds = makePlayerGeyserBoundsState(this.core.ownerMode, this.source);
        this.effects = {
            configInt: (key, fallback) => {
                if (key !== 'predictableGeysers' || fallback !== 1) throw new Error('다른 설정 조회');
                return this.predictable;
            },
            wallSeconds: () => 100,
            refresh: () => { this.dirty.invalidateAll(); },
            negativeQuantity: forbidden('수량 진단'),
            boundsAssertion: forbidden('범위 진단'),
            ...(options.effects ?? {}),
        };
        this.gameplay = options.gameplay ?? {};
        this.construction = null;
        if (options.bind !== false) {
            this.construction = this.make();
            this.world.load([]);
            this.dirty.clear();
        }
    }

    // 같은 입력으로 연결을 만든다. 거부 시험은 개별 입력을 덮어쓴다. (2026-11-15 WB04-E562)
    make(input = {}) {
        const use = { source: this.world, hp: this.hp, sp: this.sp, reward: this.reward, frames: this.metadata.frames, chunks: this.chunks, players: this.players, broadcast: this.broadcast,
            state: this.state, bounds: this.bounds, recent: this.recent, allGeysers: this.assetState.geysers, effects: this.effects, gameplay: this.gameplay, ...input };
        return new RawCampaignConstructionBindings(use.source, use.hp, use.sp, use.reward, use.frames, use.chunks, use.players, use.broadcast, use.state, use.bounds, use.recent,
            use.allGeysers, use.effects, use.gameplay);
    }
}

// 원본 열 개 인자 순서의 확정 요청이다. (2026-11-15 WB04-E562)
function confirmRequest(type, x, y, player = 0, argument = 0, direction = 0, flags = 0, timeLow = 0, timeHigh = 0, quality = 0) {
    return { type, x, y, player, argument, direction, flags, timeLow, timeHigh, quality };
}

// 슬롯의 float 좌표를 절삭한 칸 번호이다. (2026-11-15 WB04-E562)
function cellOf(raw, offset) {
    return Math.trunc(new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true));
}

test('PriestPlacementAssets: 프레임 코드·기본 프레임·기준점·SHP 표·패턴 타입을 같은 TYPE 번호로 묶는다', () => {
    const { capture, records, metadata } = inputs();
    assert.equal(metadata.frames.length, 188);
    assert.equal(metadata.hotspots.length, 188);
    assert.equal(metadata.shapes.length, 188);
    // 내장 타입은 빈 프레임 표와 기본 프레임 0 이다.
    for (let type = 0; type < 70; type += 1) assert.deepEqual(metadata.frames[type], { frames: [], defaultFrame: 0 });
    // 자산 번호 70 부터 정의의 프레임 순서와 특수 기본 프레임, 타입 기준점을 그대로 복사한다.
    for (let block = 0; block < capture.table.definitions.length; block += 1) {
        const definition = capture.table.definitions[block];
        const type = 70 + block;
        assert.deepEqual(metadata.frames[type].frames, definition.clusters.map((cluster) => cluster.code));
        assert.equal(metadata.frames[type].defaultFrame, definition.defaultFrame);
        assert.deepEqual(metadata.hotspots[type], { x: records[type].hotspotX, y: records[type].hotspotY });
    }
    // 기본 프레임이 0 이 아닌 타입과 기준점이 0 이 아닌 타입이 실제 자료에 있어 복사 누락이 드러난다.
    assert.ok(metadata.frames.some((entry) => entry.defaultFrame !== 0));
    assert.ok(metadata.hotspots.some((entry) => entry.x !== 0) && metadata.hotspots.some((entry) => entry.y !== 0));
    // 셰이프는 실제 SHP 추가 헤더의 물리 프레임 표이다.
    assert.deepEqual(metadata.shapes, capture.shapes);
    // 패턴 여덟 타입은 로딩 파일 이름으로 해석한 번호(원본 전역 순서)이다.
    assert.deepEqual(metadata.geometry.patternTypes, [107, 82, 94, 157, 131, 129, 140, 142]);
    assert.equal(metadata.geometry.decoderDiagnostics, null);
    // 판본이 다른 표는 거부한다.
    assert.throws(() => new PriestPlacementAssets(capture.table, capture.database, records.slice(1)), RangeError);
    assert.throws(() => new PriestPlacementAssets({ definitions: capture.table.definitions.slice(1) }, capture.database, records), RangeError);
});

test('RawCampaignConstructionBindings: 보호 noIsland 받침 확정이 서버 SID 9개·0x3f 패킷·같은 공간 Pop 과 예측 조각 삭제를 실행한다', () => {
    const scene = new ProtectedConstructionScene();
    const { core } = scene;
    assert.equal(core.types.length, 188);
    // 명시 로컬 예측 입력이며 서버 확정에서 번호를 재사용하지 않는다.
    const preview = scene.world.create(NO_ISLAND, 2);
    core.pool.allocated(preview)[40] |= 1;
    scene.world.pop(preview, 100, 100, 2);
    const before = core.pool.freeCount;
    assert.equal(scene.construction.lastPacket().length, 0);
    assert.equal(scene.construction.confirmSupport(confirmRequest(NO_ISLAND, 100, 100)), true);
    const bytes = scene.construction.lastPacket();
    assert.deepEqual([bytes.length, bytes[0], bytes[1], bytes[26]], [45, 1, 0x3f, 9]);
    // 진단 사본은 실제 512BYTE 물리 범위의 앞부분이다.
    assert.equal(bytes.buffer.byteLength, 512);
    assert.ok((core.pool.slot(preview)[11] & 1) !== 0 && core.pool.freeCount < before);
    // 실제 wire 의 각 서버 SID 가 같은 살아 있는 raw 표면/spot/공통 장부에 등록되어야 한다.
    const entries = core.hash.entries(0);
    for (let index = 0; index < bytes[26]; index += 1) {
        const sid = get(bytes, 27 + index * 2, 2);
        const raw = core.pool.slot(sid);
        assert.ok(sid >= core.pool.layout.serverFirst && sid !== preview && raw[10] === NO_ISLAND && !(raw[11] & 15) && !(raw[40] & 1));
        const x = cellOf(raw, 14);
        const y = cellOf(raw, 18);
        assert.equal(entries[y * 256 + x], sid);
        assert.ok(core.spots[y * 256 + x] & 2);
    }
    assert.deepEqual([core.bookkeeping.depth, core.destroy.preDepth, core.destroy.postDepth, scene.dirty.fullRedraw], [0, 0, 0, true]);
    // 일반 TYPE 는 실제 factory 첫 쓰기 전에 거부한다.
    const free = core.pool.freeCount;
    assert.throws(() => scene.construction.confirmSupport(confirmRequest(108, 140, 140, 1)), /noIsland\/Player0 요청만 지원/u);
    assert.equal(core.pool.freeCount, free);
    // noIsland 라도 Player·인자·방향·flags·시각·품질 가운데 하나라도 0 이 아니면 같은 경계에서 거부한다.
    for (let field = 3; field < 10; field += 1) {
        const values = [NO_ISLAND, 140, 140, 0, 0, 0, 0, 0, 0, 0];
        values[field] = 1;
        assert.throws(() => scene.construction.confirmSupport(confirmRequest(...values)), /noIsland\/Player0 요청만 지원/u, `field ${field}`);
    }
    assert.equal(core.pool.freeCount, free);
});

test('RawCampaignConstructionBindings: 로컬 간헐천이 현재 Player·실제 Bounds·전용 최근 목록·같은 보상 pool 로 생성된다', () => {
    const scene = new ProtectedConstructionScene();
    const { core } = scene;
    scene.reward.geyserPool = 18000;
    // 자원 pool 은 보상 상태와 같은 저장소이고 최근 목록/청크 지도/풀은 장면의 것 그대로다.
    assert.equal(scene.construction.localGeysers.readResourcePool(), 18000);
    scene.reward.storeGeyserPool(17999);
    assert.equal(scene.construction.localGeysers.readResourcePool(), 17999);
    scene.reward.geyserPool = 18000;
    assert.equal(scene.construction.localGeysers.recent, scene.recent);
    assert.equal(scene.construction.playerBounds.map, scene.chunks);
    assert.equal(scene.construction.pool, core.pool);
    scene.recent.count = 1;
    scene.recent.entries[0] = 123;
    // 비활성 Player 는 실제로 생성하지 않는다.
    scene.construction.generateLocalGeysers();
    assert.deepEqual([scene.recent.count, scene.assetState.geysers.count, scene.reward.geyserPool], [1, 0, 18000]);
    put(scene.players, STRIDE + PLAYER_FLAGS, 2, 1);
    put(scene.players, STRIDE + PLAYER_REGION, 1);
    scene.state.geyser.firstPlayerQuantity = 2;
    scene.construction.generateLocalGeysers();
    assert.deepEqual([scene.recent.count, scene.assetState.geysers.count], [2, 2]);
    assert.deepEqual([scene.reward.geyserPool, scene.state.geyser.resourcePool, scene.construction.lastPacket().length], [14000, 0, 45]);
    // 두 목록은 같은 실제 SID 를 가리키지만 count/저장소는 서로 독립적이다.
    const all = scene.assetState.geysers.entries.slice(0, scene.assetState.geysers.count);
    for (const sid of scene.recent.entries.slice(0, scene.recent.count)) {
        const raw = core.pool.slot(sid);
        assert.deepEqual([raw[10], get(raw, 12, 2), raw[11] & 15], [GEYSER, 2000, 0]);
        assert.ok(all.includes(sid));
    }
    assert.equal(core.bookkeeping.depth, 0);
    assert.notEqual(core.kernel.size, 0);
    // 공유 전역 불일치는 새 생성 전에 드러낸다.
    const free = core.pool.freeCount;
    scene.state.place.authority = false;
    assert.throws(() => scene.construction.generateLocalGeysers(), /현재 전역 불일치/u);
    assert.equal(core.pool.freeCount, free);
});

test('RawCampaignConstructionBindings: 구성 불일치는 첫 생성 전에 거부한다', () => {
    const scene = new ProtectedConstructionScene({ bind: false });
    const other = new ProtectedConstructionScene({ bind: false });
    const free = scene.core.pool.freeCount;
    const runtime = bindConstructionRuntime(scene.world);
    // 실행 부모: 저장소/Core 정체성과 네 가상 경계(생성·Pop·삭제·방송)가 모두 있어야 한다.
    const identity = /동일 장기 물리\/실제 가상 경계 누락/u;
    assert.throws(() => scene.make({ source: { ...runtime, storage: null } }), identity);
    assert.throws(() => scene.make({ source: { ...runtime, storage: other.world.storage } }), identity);
    assert.throws(() => scene.make({ source: { ...runtime, core: null } }), identity);
    assert.throws(() => scene.make({ source: { ...runtime, core: { ...runtime.core, pool: other.core.pool } } }), identity);
    assert.throws(() => scene.make({ source: { ...runtime, core: { ...runtime.core, hash: other.core.hash } } }), identity);
    assert.throws(() => scene.make({ source: { ...runtime, core: { ...runtime.core, types: scene.core.types.slice() } } }), identity);
    assert.throws(() => scene.make({ source: { ...runtime, core: { ...runtime.core, spots: scene.core.spots.slice() } } }), identity);
    for (const name of ['create', 'pop', 'destroy']) assert.throws(() => scene.make({ source: { ...runtime, [name]: null } }), identity, name);
    assert.throws(() => scene.make({ broadcast: null }), identity);
    // 월드/모드/목록/서비스 계약.
    const contract = /같은 월드\/현재 모드\/전용 목록\/실제 서비스 계약 오류/u;
    assert.throws(() => scene.make({ hp: other.hp }), contract);
    assert.throws(() => scene.make({ hp: { pool: other.core.pool, bookkeeping: scene.core.bookkeeping } }), contract);
    assert.throws(() => scene.make({ hp: new SquidReward(scene.core.pool, scene.core.types, other.core.bookkeeping, scene.reward, scene.sp) }), contract);
    assert.throws(() => scene.make({ frames: scene.metadata.frames.slice(1) }), contract);
    assert.throws(() => scene.make({ players: scene.players.subarray(1) }), contract);
    assert.throws(() => scene.make({ players: Array.from(scene.players) }), contract);
    assert.throws(() => scene.make({ recent: scene.assetState.geysers }), contract);
    const short = new SquidPostPopList();
    short.entries.length = 4;
    assert.throws(() => scene.make({ recent: short }), contract);
    assert.throws(() => scene.make({ bounds: makePlayerGeyserBoundsState(other.core.ownerMode, scene.source) }), contract);
    for (const name of ['configInt', 'wallSeconds', 'refresh', 'negativeQuantity', 'boundsAssertion']) {
        assert.throws(() => scene.make({ effects: { ...scene.effects, [name]: undefined } }), contract, name);
    }
    assert.equal(scene.core.pool.freeCount, free);
    // 최근 목록 5칸은 허용하고, 실행 부모에 이미 방송이 있으면 broadcast 객체 없이도 구성한다.
    const exact = new SquidPostPopList();
    exact.entries.length = 5;
    assert.ok(scene.make({ recent: exact }) instanceof RawCampaignConstructionBindings);
    assert.ok(scene.make({ source: { ...runtime, broadcast: () => {} }, broadcast: null }) instanceof RawCampaignConstructionBindings);
    // 상태 뷰 생성기는 소유자 모드와 현재 값 소유자가 모두 있어야 한다.
    assert.throws(() => makePlayerGeyserBoundsState(null, scene.source), TypeError);
    assert.throws(() => makePlayerGeyserBoundsState(scene.core.ownerMode, null), TypeError);
});

test('RawCampaignConstructionBindings: Core 선택과 범위 상태 뷰는 사본이 아니라 같은 현재 소유자를 읽는다', () => {
    const scene = new ProtectedConstructionScene({ bind: false });
    const core = bindConstructionCore(scene.core);
    assert.deepEqual(Object.keys(core), ['pool', 'hash', 'spots', 'types', 'frames', 'bookkeeping', 'ownerMode', 'factory', 'owner', 'building', 'virtualDestroy', 'surfaces', 'bridgeState']);
    // 열세 참조 모두 월드의 같은 객체이다.
    for (const [name, value] of Object.entries(core)) assert.equal(value, scene.core[name], name);
    const runtime = bindConstructionRuntime(scene.world);
    assert.equal(runtime.storage, scene.world.storage);
    assert.equal(runtime.broadcast, null);
    const sid = runtime.create(NO_ISLAND, 0);
    assert.equal(scene.core.pool.slot(sid)[10], NO_ISLAND);
    // 범위 상태는 소유자 모드의 battle/challenge 와 현재 값 소유자의 네 칸을 매번 다시 읽는다.
    const bounds = scene.bounds;
    assert.equal(bounds.ownerMode, scene.core.ownerMode);
    scene.core.ownerMode.battle = false;
    scene.core.ownerMode.challenge = true;
    assert.deepEqual([bounds.battle, bounds.metamap], [false, true]);
    scene.core.ownerMode.battle = true;
    scene.core.ownerMode.challenge = false;
    assert.deepEqual([bounds.battle, bounds.metamap], [true, false]);
    scene.source.mission = true;
    scene.source.mapSize = 3;
    scene.source.puzzlePieceType = 108;
    assert.deepEqual([bounds.mission, bounds.mapSize, bounds.puzzlePieceType, bounds.playerDirections, bounds.decoderDiagnostics], [true, 3, 108, scene.source.playerDirections, null]);
    const diagnostics = {};
    scene.source.decoderDiagnostics = diagnostics;
    assert.equal(bounds.decoderDiagnostics, diagnostics);
});

test('RawCampaignConstructionBindings: validateState 가 현재 전역 불일치 항목을 하나씩 가른다', () => {
    const scene = new ProtectedConstructionScene();
    const { state, core } = scene;
    const free = core.pool.freeCount;
    const mismatch = /현재 전역 불일치/u;
    // 한 항목만 어긋나게 한 뒤 되돌린다. 모든 공개 입구(확정·요청·수신·환불·취소·부족·받침·간헐천)가 같은 검증을 먼저 한다.
    const cases = [
        ['place noIsland', () => { state.place.noIslandType = 156; }, () => { state.place.noIslandType = 157; }],
        ['world noIsland', () => { core.bridgeState.noIslandType = 156; }, () => { core.bridgeState.noIslandType = 157; }],
        ['geyser noIsland', () => { state.geyser.noIslandType = 156; }, () => { state.geyser.noIslandType = 157; }],
        ['confirm pattern 3', () => { state.confirm.patternTypes = [107, 82, 94, 156, 131, 129, 140, 142]; }, () => { state.confirm.patternTypes = [107, 82, 94, 157, 131, 129, 140, 142]; }],
        ['clear pattern', () => { state.clear.patternTypes = [107, 82, 94, 157, 131, 129, 140, 143]; }, () => { state.clear.patternTypes = [107, 82, 94, 157, 131, 129, 140, 142]; }],
        ['clear pattern 길이', () => { state.clear.patternTypes = [107, 82, 94, 157, 131, 129, 140]; }, () => { state.clear.patternTypes = [107, 82, 94, 157, 131, 129, 140, 142]; }],
        ['place pattern', () => { state.place.patternTypes = [108, 82, 94, 157, 131, 129, 140, 142]; }, () => { state.place.patternTypes = [107, 82, 94, 157, 131, 129, 140, 142]; }],
        ['clear bridge', () => { state.clear.bridgeType = 83; }, () => { state.clear.bridgeType = 82; }],
        ['world bridge', () => { core.bridgeState.bridgeType = 83; state.clear.bridgeType = 83; }, () => { core.bridgeState.bridgeType = 82; state.clear.bridgeType = 82; }],
        ['notice local', () => { state.notice.localPlayer = 2; }, () => { state.notice.localPlayer = 1; }],
        ['clear authority', () => { state.clear.authority = false; }, () => { state.clear.authority = true; }],
        ['notice authority', () => { state.notice.authority = false; }, () => { state.notice.authority = true; }],
        ['geyser boss', () => { state.geyser.boss = false; }, () => { state.geyser.boss = true; }],
        ['geyser fort', () => { state.geyser.fort = true; }, () => { state.geyser.fort = false; }],
        ['world fort', () => { core.ownerMode.fort = true; }, () => { core.ownerMode.fort = false; }],
        ['noIsland flags1', () => { core.types[NO_ISLAND].flags1 |= 0x10; }, () => { core.types[NO_ISLAND].flags1 &= ~0x10; }],
        ['noIsland flags1 운반', () => { core.types[NO_ISLAND].flags1 |= 0x8000; }, () => { core.types[NO_ISLAND].flags1 &= ~0x8000; }],
        ['noIsland dais genus', () => { core.types[NO_ISLAND].flags2 |= 0x400000; }, () => { core.types[NO_ISLAND].flags2 &= ~0x400000; }],
        ['dais type', () => { state.notice.daisType = 157; }, () => { state.notice.daisType = 165; }],
    ];
    // 각 항목이 어긋난 동안 여덟 입구가 모두 거부한다.
    for (const [name, breakIt, restore] of cases) {
        breakIt();
        const request = confirmRequest(NO_ISLAND, 100, 100);
        assert.throws(() => scene.construction.confirm(request), mismatch, name);
        assert.throws(() => scene.construction.request(request), mismatch, name);
        assert.throws(() => scene.construction.receive(1, new Uint8Array(25)), mismatch, name);
        assert.throws(() => scene.construction.refund(NO_ISLAND, 1), mismatch, name);
        assert.throws(() => scene.construction.reject(NO_ISLAND, 1, 10, 10, 0, 0), mismatch, name);
        assert.throws(() => scene.construction.chargeShortfall(NO_ISLAND, 1, 10, 10, 0, 0), mismatch, name);
        assert.throws(() => scene.construction.confirmSupport(request), mismatch, name);
        assert.throws(() => scene.construction.generateLocalGeysers(), mismatch, name);
        assert.throws(() => scene.construction.populateGeysers(1, scene.random, { left: 10, top: 10, right: 20, bottom: 20 }, {}), mismatch, name);
        restore();
    }
    assert.equal(core.pool.freeCount, free);
    // 받침 타입 번호가 표 밖이면 TYPE 조회 전에 거부한다(세 전역을 같은 값으로 맞춰도).
    state.place.noIslandType = 188;
    core.bridgeState.noIslandType = 188;
    state.geyser.noIslandType = 188;
    for (const each of [state.confirm, state.clear, state.place]) each.patternTypes = [107, 82, 94, 188, 131, 129, 140, 142];
    assert.throws(() => scene.construction.confirm(confirmRequest(188, 100, 100)), mismatch);
    // 다이스 genus 는 flags2 의 0x400000 뿐이고 미복원 flags1 은 0x8010 뿐이다: 이웃 비트는 불일치가 아니다.
    core.types[NO_ISLAND].flags2 |= 1;
    core.types[NO_ISLAND].flags1 |= 0x20;
    state.place.noIslandType = 157;
    core.bridgeState.noIslandType = 157;
    state.geyser.noIslandType = 157;
    for (const each of [state.confirm, state.clear, state.place]) each.patternTypes = [107, 82, 94, 157, 131, 129, 140, 142];
    scene.construction.refund(NO_ISLAND, 1);
    core.types[NO_ISLAND].flags2 &= ~1;
    core.types[NO_ISLAND].flags1 &= ~0x20;
    // 모두 같은 다른 값(공유 owner 가 정상 변경한 경우)은 준비 사본으로 거부하지 않는다: 권한을 함께 끄면 통과한다.
    state.place.noIslandType = 157;
    core.bridgeState.noIslandType = 157;
    state.geyser.noIslandType = 157;
    for (const each of [state.confirm, state.clear, state.place]) each.patternTypes = [107, 82, 94, 157, 131, 129, 140, 142];
    for (const each of [state.place, state.clear, state.notice]) each.authority = false;
    state.geyser.boss = false;
    scene.construction.generateLocalGeysers();
    assert.equal(core.pool.freeCount, free);
});

test('RawCampaignConstructionBindings: 선택되지 않은 일반 건설 경계는 그 호출에서 이름과 함께 거부한다', () => {
    const scene = new ProtectedConstructionScene();
    const request = confirmRequest(NO_ISLAND, 100, 100, 1);
    // 일반 요청은 전체 MayPlace 가 없으면 첫 경계에서 거부한다.
    assert.throws(() => scene.construction.request(request), /캠페인 일반 건설 미연결: 전체 MayPlace/u);
    // 실제 MayPlace 가 거짓이면 다른 하위를 요구하지 않고 거짓이다.
    const calls = [];
    const refused = new ProtectedConstructionScene({ gameplay: { request: { mayPlace: (query) => { calls.push(query); return false; } } } });
    assert.equal(refused.construction.request(request), false);
    assert.deepEqual(calls, [{ type: NO_ISLAND, argument: 0, x: 100, y: 100, flags: 0, owner: 1, mode: 0 }]);
    // 0x3e 수신: 비용 선검사(다른 Player)는 SP 를 읽고, 모자라면 같은 Place 의 부족 처리로 들어가 비용 안내 경계에서 거부한다.
    const packet = new Uint8Array(25);
    packet[4] = GEYSER;
    new DataView(packet.buffer).setFloat32(5, 100, true);
    new DataView(packet.buffer).setFloat32(9, 100, true);
    assert.throws(() => scene.construction.receive(2, packet), /캠페인 일반 건설 미연결: /u);
    assert.throws(() => scene.construction.receiveCommandPacket(2, packet), /캠페인 일반 건설 미연결: /u);
    // 로컬 Player 의 요청은 비용 선검사 없이 Request 로 가서 MayPlace 경계에서 거부한다.
    assert.throws(() => scene.construction.receive(1, packet), /캠페인 일반 건설 미연결: 전체 MayPlace/u);
    // 수신 차단 전역이 켜져 있으면 본문을 읽기 전에 돌아온다(검증 없는 직접 입구도 같다).
    scene.state.command.blocked = 1;
    scene.construction.receive(1, packet);
    scene.construction.receiveCommandPacket(1, packet);
    scene.state.command.blocked = 0;
    // 환불은 SP 통지 경계를 요구한다(금액이 양수일 때).
    assert.throws(() => scene.construction.refund(GEYSER, 2), /캠페인 일반 건설 미연결: SP 통지/u);
    // 취소 통지는 환불 뒤 취소 송신 경계를 요구한다(비용 0 타입은 SP 통지를 건너뛴다).
    assert.throws(() => scene.construction.reject(NO_ISLAND, 2, 10, 10, 0, 0), /캠페인 일반 건설 미연결: (배치 취소 통지|SP 통지)/u);
    // 일반 확정: 사제가 지어야 하는 타입은 기준점 조회 경계를 요구한다.
    const builder = scene.core.types.findIndex((type, number) => number >= 70 && ((type.flags1 >>> 0) & 0x8000));
    assert.ok(builder >= 70);
    assert.throws(() => scene.construction.confirm(confirmRequest(builder, 100, 100, 1)), /캠페인 일반 건설 미연결: 작업장\/Graph 기준점/u);
});

test('RawCampaignConstructionBindings: 외부 효과와 gameplay 경계가 실제 하위에 연결된다', () => {
    const events = [];
    const money = [];
    const scene = new ProtectedConstructionScene({
        effects: {
            presentation: {
                sendMoney: (owner, notice) => { money.push([owner, notice.amount ?? notice.delta ?? null]); },
                sendReject: (owner, notice) => { events.push(['reject', owner, notice.type]); },
            },
        },
        gameplay: {
            setOwner: (sid, owner) => { events.push(['setOwner', owner]); scene.core.owner.set(sid, owner); },
            confirm: { recorderActive: () => true, record: (sid) => { events.push(['record', sid]); } },
        },
    });
    const { core } = scene;
    // 가상 소유자 몸체: 배치가 조각마다 gameplay.setOwner 를 부른다(공통 소유자 대신).
    assert.equal(scene.construction.confirm(confirmRequest(NO_ISLAND, 100, 100)), true);
    assert.equal(events.filter((item) => item[0] === 'setOwner').length, 9);
    // 기록기가 꺼져 있으면 기록 경계를 부르지 않고, 켜면 지역 첫 SID 를 기록한다.
    assert.equal(events.some((item) => item[0] === 'record'), false);
    scene.state.confirm.recorderEnabled = true;
    assert.equal(scene.construction.confirm(confirmRequest(NO_ISLAND, 120, 120)), true);
    const packet = scene.construction.lastPacket();
    assert.deepEqual(events.filter((item) => item[0] === 'record'), [['record', get(packet, 27, 2)]]);
    // 환불/취소 통지는 호출자가 준 송신 경계로 나간다.
    events.length = 0;
    scene.construction.refund(GEYSER, 2);
    assert.equal(money.length, 1);
    assert.equal(money[0][0], 2);
    scene.construction.reject(GEYSER, 2, 10, 10, 0, 0);
    assert.deepEqual(events, [['reject', 2, GEYSER]]);
    assert.equal(money.length, 2);
    // 로컬 예측 훅은 같은 실제 배치기를 쓴다(풀이 다르면 어댑터가 거부한다).
    const local = scene.construction.bindLocal({ marker: 1 });
    assert.equal(local.marker, 1);
    assert.equal(typeof local.place, 'function');
    assert.equal(typeof local.create, 'function');
    assert.equal(core.bookkeeping.depth, 0);
});

test('RawCampaignConstructionBindings: register 가 0x3e 수신과 0x3f 통지를 분배기에 연결한다', () => {
    const scene = new ProtectedConstructionScene();
    const { core } = scene;
    const diagnostics = [];
    scene.world.storage.bindDisplayDiagnostics({ value: null }, (expression, file, line) => { diagnostics.push([expression, file, line]); });
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    const prepared = [];
    scene.construction.register(dispatch, () => { prepared.push(prepared.length); });
    // 0x3f: 버전 1·정책 0·가변 길이, 직접 수신기와 packet observer 를 등록한다. 0x3e: 버전 1·정책 0·길이 25 의 일반 수신기이다.
    const notice = dispatch.registrations.entry(0x3f);
    assert.deepEqual([notice.version, notice.mismatch, notice.length, typeof notice.directReceive, typeof notice.packetObserver], [1, 0, 0xffff, 'function', 'function']);
    const command = dispatch.registrations.entry(0x3e);
    assert.deepEqual([command.version, command.mismatch, command.length, typeof command.receive], [1, 0, 25, 'function']);
    // observer: 송신 방향에서만 SID 개수로 선언 길이를 쓰고, 개수 255 는 원문 진단 뒤 계속한다. 수신 방향은 손대지 않는다.
    const sample = new Uint8Array(64);
    sample[26] = 3;
    notice.packetObserver(sample, 0);
    assert.equal(get(sample, 2, 2), 0);
    notice.packetObserver(sample, 1);
    assert.equal(get(sample, 2, 2), 27 + 3 * 2);
    sample[26] = 255;
    notice.packetObserver(sample, 1);
    assert.deepEqual(diagnostics, [['sidCount != (unsigned char)-1', '\\Ns\\O\\_p_packets.cpp', 0x7c9]]);
    // 서버 확정이 만든 실제 통지 wire 의 SID 를 새 서버 void 객체로 바꾸고 좌표를 옮겨 직접 수신 입구에 넣으면 같은 통지 본문이 조각을 놓는다.
    assert.equal(scene.construction.confirmSupport(confirmRequest(NO_ISLAND, 100, 100)), true);
    const wire = Uint8Array.from(scene.construction.lastPacket());
    const fresh = [];
    // 통지의 SID 를 새로 확보한 서버 void 객체로 바꾼다.
    for (let index = 0; index < wire[26]; index += 1) {
        const sid = scene.world.create(NO_ISLAND, 0);
        fresh.push(sid);
        put(wire, 27 + index * 2, sid, 2);
    }
    const coordinates = new DataView(wire.buffer);
    assert.deepEqual([coordinates.getFloat32(5, true), coordinates.getFloat32(9, true)], [100, 100]);
    coordinates.setFloat32(5, 130, true);
    coordinates.setFloat32(9, 130, true);
    for (const sid of fresh) assert.equal(core.pool.slot(sid)[11] & 4, 4);
    dispatch.invokeDirect(0x3f, 9, null, wire);
    assert.deepEqual(prepared, [0]);
    for (const sid of fresh) assert.equal(core.pool.slot(sid)[11] & 15, 0);
    // 0x3e 일반 수신기도 같은 준비 장치를 거쳐 실제 command 본문으로 들어간다(로컬 Player 요청은 MayPlace 경계에서 거부).
    const packet = new Uint8Array(25);
    packet[4] = GEYSER;
    assert.throws(() => dispatch.invokeDirect(0x3e, 1, {}, packet), /캠페인 일반 건설 미연결: 전체 MayPlace/u);
    assert.deepEqual(prepared, [0, 1]);
    // prepare 없이 등록해도 통지를 받는다.
    const plain = new ProtectedConstructionScene();
    const second = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    plain.construction.register(second);
    assert.equal(plain.construction.confirmSupport(confirmRequest(NO_ISLAND, 100, 100)), true);
    const again = Uint8Array.from(plain.construction.lastPacket());
    const placed = [];
    // 같은 방식으로 새 서버 void 객체와 다른 좌표의 통지를 만든다.
    for (let index = 0; index < again[26]; index += 1) {
        placed.push(plain.world.create(NO_ISLAND, 0));
        put(again, 27 + index * 2, placed[index], 2);
    }
    new DataView(again.buffer).setFloat32(5, 130, true);
    new DataView(again.buffer).setFloat32(9, 130, true);
    second.invokeDirect(0x3f, 9, null, again);
    for (const sid of placed) assert.equal(plain.core.pool.slot(sid)[11] & 15, 0);
});

test('RawCampaignConstructionBindings: 표면/받침의 Pre/Post 삭제는 표면 수명으로, 나머지는 fallback 으로 분배한다', () => {
    const scene = new ProtectedConstructionScene();
    const { core } = scene;
    assert.equal(scene.construction.confirmSupport(confirmRequest(NO_ISLAND, 100, 100)), true);
    const support = get(scene.construction.lastPacket(), 27, 2);
    assert.equal(get(core.pool.slot(support), 0), NO_ISLAND_VTABLE);
    const geyser = scene.world.create(GEYSER, 0);
    const seen = [];
    const hooks = scene.construction.destroyHooks({ selected: () => 7, emit: (event) => { seen.push([event.effect, event.sid]); } });
    // fallback 의 다른 칸은 그대로 보존한다.
    assert.equal(hooks.selected(), 7);
    // 받침이 아닌 자산과 받침의 다른 효과는 fallback 이 맡는다.
    hooks.emit({ effect: SQUID_DESTROY_EFFECT.preDestroy, sid: geyser, parent: 0, flags: 0 });
    hooks.emit({ effect: SQUID_DESTROY_EFFECT.transmit, sid: support, parent: 0, flags: 0 });
    hooks.emit({ effect: SQUID_DESTROY_EFFECT.clearSelection, sid: support, parent: 0, flags: 0 });
    assert.deepEqual(seen, [[SQUID_DESTROY_EFFECT.preDestroy, geyser], [SQUID_DESTROY_EFFECT.transmit, support], [SQUID_DESTROY_EFFECT.clearSelection, support]]);
    // fallback 이 없으면 다른 자산 효과를 거부한다.
    assert.throws(() => scene.construction.destroyHooks().emit({ effect: SQUID_DESTROY_EFFECT.preDestroy, sid: geyser, parent: 0, flags: 0 }), /다른 캠페인 건설 자산 삭제 효과 미연결/u);
    // 가상 삭제: 받침은 같은 월드의 가상 삭제기로, 다른 자산은 fallback 으로 간다.
    const calls = [];
    const destroy = scene.construction.destroy((sid, flags, given) => { calls.push([sid, flags, given]); return 'fallback'; });
    const marker = { marker: true };
    assert.equal(destroy(geyser, 5, marker), 'fallback');
    assert.deepEqual(calls, [[geyser, 5, marker]]);
    assert.throws(() => scene.construction.destroy()(geyser, 0, marker), /다른 캠페인 건설 자산 가상 삭제 미연결/u);
    // 받침의 실제 삭제는 월드의 공통 삭제 경로로 실행한다(Pre/Post 가 표면 수명으로 분배된다).
    const before = core.pool.freeCount;
    assert.equal(scene.world.destroy(support, 0), true);
    assert.ok(core.pool.freeCount > before);
    assert.deepEqual([core.destroy.preDepth, core.destroy.postDepth], [0, 0]);
    // 받침(157)·섬 받침(94)·종유석(95)·그루터기(96)는 (TYPE, 가상 표)가 함께 맞을 때만 같은 월드의 가상 삭제기와 표면 수명으로 끝까지 삭제된다.
    // Pre/Post 는 표면 수명이 맡고 그 밖의 효과(전송 등)만 fallback 훅이 본다.
    const effects = [];
    const lifetime = scene.construction.destroyHooks({ selected: () => 0, emit: (event) => { effects.push([event.effect, event.sid]); } });
    for (const [type, table] of [[NO_ISLAND, NO_ISLAND_VTABLE], [ISLAND, ISLAND_VTABLE], [STALAG, STALAG_VTABLE], [STUMP, BASE_VTABLE]]) {
        const sid = scene.world.create(type, 0);
        assert.equal(get(core.pool.slot(sid), 0), table, `TYPE ${type}`);
        effects.length = 0;
        assert.equal(destroy(sid, 0, lifetime), true, `TYPE ${type}`);
        assert.equal(core.pool.slot(sid)[11] & 1, 1, `TYPE ${type}`);
        assert.equal(effects.some(([effect]) => effect === SQUID_DESTROY_EFFECT.preDestroy || effect === SQUID_DESTROY_EFFECT.postDestroy), false, `TYPE ${type}`);
        assert.deepEqual([core.destroy.preDepth, core.destroy.postDepth], [0, 0], `TYPE ${type}`);
    }
    assert.equal(calls.length, 1);
    // 어긋난 조합은 모두 fallback 이다: 다른 TYPE 의 같은 가상 표(빈 간헐천 84·지면 97 의 공통 표), 같은 TYPE 의 다른 가상 표, 상위 BYTE 만 다른 가상 표.
    const mismatched = [];
    for (const [type, table] of [[84, null], [97, null], [STUMP, NO_ISLAND_VTABLE], [ISLAND, STALAG_VTABLE], [STALAG, ISLAND_VTABLE], [NO_ISLAND, BASE_VTABLE], [NO_ISLAND, 0x80506f08], [ISLAND, 0x00516d98],
        [STALAG, 0x00506f50]]) {
        const sid = scene.world.create(type, 0);
        if (table !== null) put(core.pool.allocated(sid), 0, table);
        mismatched.push(destroy(sid, 0, marker));
    }
    assert.deepEqual(mismatched, new Array(9).fill('fallback'));
    // 받침·섬 받침·종유석 TYPE 은 현재 전역(배치 상태·월드 BridgeState)에서 매번 읽는다.
    scene.state.place.noIslandType = 156;
    assert.equal(destroy(scene.world.create(NO_ISLAND, 0), 0, marker), 'fallback');
    scene.state.place.noIslandType = 157;
    core.bridgeState.islandType = 95;
    core.bridgeState.stalagType = 94;
    assert.equal(destroy(scene.world.create(ISLAND, 0), 0, marker), 'fallback');
    assert.equal(destroy(scene.world.create(STALAG, 0), 0, marker), 'fallback');
    core.bridgeState.islandType = 94;
    core.bridgeState.stalagType = 95;
    calls.length = 1;
    // 가상 표와 TYPE 이 함께 맞아야 받침이다: 같은 TYPE 이라도 다른 가상 표, 같은 가상 표라도 다른 TYPE 은 fallback 이다.
    const forged = scene.world.create(NO_ISLAND, 0);
    put(core.pool.allocated(forged), 0, ISLAND_VTABLE);
    assert.equal(destroy(forged, 0, marker), 'fallback');
    const island = scene.world.create(ISLAND, 0);
    assert.equal(get(core.pool.slot(island), 0), ISLAND_VTABLE);
    core.pool.allocated(island)[10] = NO_ISLAND;
    assert.equal(destroy(island, 0, marker), 'fallback');
    assert.equal(calls.length, 3);
});
