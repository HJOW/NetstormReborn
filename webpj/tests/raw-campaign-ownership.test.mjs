// 실제 캠페인 소유권 연결 raw-campaign-ownership.js(RawCampaignOwnership)를 cpppj 계약으로 대조한다(4-D C 구획, WB04-E518).
// cpppj 시험의 대응:
//  - `VortexOwner_CampaignSameCoreAndFirstRegion`(VortexOwnerTests.cpp): 같은 Core/9 Player 기록, 신전 SID 의 가상 소유자 변경이 Player 신전 두 DWORD·공통 Owner·생산 갱신을 만들고,
//    섬 소유자 변경이 Player 첫 지역을 갱신하지 않는 별도 경로를 유지하는지(원본 10.78 신전 가상 표 0x5196f0 의 실제 TYPE 123 생성자).
//  - `ClientRegions_WrapperSharesBodyAndCurrentDevicesPinSelectedCall`(ClientRegionOwnerTests.cpp): Native 경로가 같은 장기 신전 본체를 빌리고 비신전 분배는 현재 짧은 Session 에 남긴다.
// 웹 전용 검사: 생성자 거부 4종(다른 풀·잘못된 표 크기·필수 효과 누락)이 첫 공유 쓰기 전에 일어나는지, plain getter 의 CRT 절삭·범위 거부, Native 덕타입 계약.
// 입력은 실제 TYPE 188칸(`realPlayerAssets`)과 합성 SHP 이며 기대값은 cpppj 시험이 단언하는 관계이다. 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E518)
import assert from 'node:assert/strict';
import test from 'node:test';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { AssetPostPopState } from '../www/core/raw-asset-post-pop-state.js';
import { RawCampaignOwnership } from '../www/core/raw-campaign-ownership.js';
import { RawIslandRuntime, createFortFinishOwnerState, createRawIslandStorage } from '../www/core/raw-island-runtime.js';
import { RawSurfaceWorld } from '../www/core/raw-surface-world.js';
import { RegionPaintState } from '../www/core/raw-region-paint.js';
import { RegionOwnershipState } from '../www/core/raw-region-ownership.js';
import { GameRandom } from '../www/core/random.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { realPlayerAssets } from './player-scene-support.mjs';
import { get, put } from './raw-support.mjs';

// 신전 가상 표 중 첫 번째(TYPE 123 신전)와 Player 한 칸 폭·첫 지역/신전 DWORD 위치, 표면 지도 한 변이다. (2026-11-15 WB04-E518)
const VORTEX_TABLE = 0x519860;
const STRIDE = 0xb4;
const REGION = 0x78;
const TEMPLE = 0x7c;
const WIND_TEMPLE = 123;
const TERRAIN = 97;
const EDGE_FARM = 121;

// 실제 TYPE 표와 빈 프레임 코드/셰이프로 월드를 만들고 같은 풀을 쓰는 섬 런타임·자산 목록을 준비한 장면이다. (2026-11-15 WB04-E518)
function makeScene() {
    const real = realPlayerAssets();
    const types = real.types.map((record) => ({ ...record }));
    const frames = types.map(() => new FrameCodeTable([]));
    const shapes = types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
    const links = new BridgeConnectState();
    Object.assign(links, { bridgeType: 82, islandType: 94, stalagType: 95, connectorType: 155, noIslandType: 157, battle: true });
    const world = new RawSurfaceWorld(types, frames, shapes, links, 97);
    const core = world.core();
    const assets = new AssetPostPopState();
    const modes = createFortFinishOwnerState();
    const records = createRawIslandStorage();
    const redraw = { value: 0 };
    const mapRevision = { value: 0 };
    const islands = new RawIslandRuntime(core.pool, core.hash, records, modes, assets.residences, redraw, mapRevision, {
        setResidenceOwner: (sid, owner) => core.owner.set(sid, owner),
        assertion: () => { throw new Error('정상 섬 입력의 예상하지 않은 진단'); },
    });
    islands.reset();
    const players = new Uint8Array(9 * STRIDE);
    // 최초 첫 지역은 원본 sentinel 127 이다.
    for (let player = 0; player < 9; player += 1) put(players, player * STRIDE + REGION, 127);
    const plain = types.map((_, type) => ({ frames: core.frames[type].codes, defaultFrame: 0 }));
    const themes = new Array(types.length).fill(0);
    const counters = { refresh: 0, others: [] };
    const effects = {
        otherOwner: (sid, owner) => { counters.others.push([sid, owner]); },
        refreshProduction: () => { counters.refresh += 1; },
        themeMode: () => 1,
        missingIsland: () => { throw new Error('아직 void 인 실제 신전은 지역 조회를 요청하지 않습니다'); },
    };
    return { world, core, assets, modes, records, islands, players, plain, themes, effects, counters, types, redraw, mapRevision };
}

// 같은 장면의 입력으로 소유권 연결을 만든다. 고르는 값만 바꿔 거부 시험에 재사용한다. (2026-11-15 WB04-E518)
function bind(scene, overrides = {}) {
    const input = { islands: scene.islands, players: scene.players, frames: scene.plain, themes: scene.themes, effects: scene.effects, ...overrides };
    return new RawCampaignOwnership(scene.world, input.islands, scene.assets, input.players, input.frames, input.themes, new GameRandom(7), new RegionPaintState(), new RegionOwnershipState(),
        { value: 0 }, TERRAIN, EDGE_FARM, input.effects);
}

test('RawCampaignOwnership: 같은 Core 와 첫 지역 — 신전 가상 소유자가 Player 신전 두 DWORD 와 생산 갱신을 만들고 섬 소유자는 별도 경로', () => {
    const scene = makeScene();
    scene.islands.add(1, 0);
    scene.islands.setBounds(0, { left: 10, top: 10, right: 50, bottom: 50 });
    const binding = bind(scene);
    const sid = scene.world.create(WIND_TEMPLE);
    // TYPE 123 신전 생성자는 가상 표 0x519860 를 쓰며 같은 풀/9기록을 신전 Owner 가 공유한다.
    assert.equal(get(scene.core.pool.slot(sid), 0), VORTEX_TABLE);
    assert.equal(binding.vortex().records, scene.players);
    assert.equal(binding.vortex().pool, scene.core.pool);
    binding.setOwner(sid, 1);
    assert.equal(scene.core.pool.slot(sid)[34], 1);
    assert.equal(get(scene.players, STRIDE + TEMPLE), sid);
    assert.equal(get(scene.players, STRIDE + TEMPLE + 4), sid);
    assert.equal(scene.counters.refresh, 1);
    assert.deepEqual(scene.counters.others, []);
    // 섬 소유자 변경은 Player 2 의 첫 지역만 쓰고 Player 1 의 신전/첫 지역은 건드리지 않는다.
    scene.islands.setOwner(0, 2);
    assert.equal(get(scene.players, 2 * STRIDE + REGION), 0);
    assert.equal(scene.modes.playerRegions[2], 127);
    assert.equal(get(scene.players, STRIDE + TEMPLE), sid);
    assert.equal(get(scene.players, STRIDE + REGION), 127);
});

test('RawCampaignOwnership: 신전이 아닌 가상 표는 상위 분배기에 전달하고 신전 Owner 를 건드리지 않는다', () => {
    const scene = makeScene();
    const binding = bind(scene);
    const sid = scene.world.create(WIND_TEMPLE);
    // 비신전 가상 표(원본 0x503360)로 바꾼 슬롯은 다른 파생 분배기로만 간다.
    put(scene.core.pool.allocated(sid), 0, 0x503360);
    binding.setOwner(sid, 8);
    assert.deepEqual(scene.counters.others, [[sid, 8]]);
    assert.equal(scene.counters.refresh, 0);
    assert.equal(get(scene.players, 8 * STRIDE + TEMPLE), 0);
});

test('RawCampaignOwnership: 다른 풀·잘못된 표 크기·필수 효과 누락은 공유 상태를 쓰기 전에 거부한다', () => {
    const scene = makeScene();
    const other = makeScene();
    const beforePlayers = Uint8Array.from(scene.players);
    const beforeRecords = Uint8Array.from(scene.records);
    // 다른 월드의 섬 런타임은 같은 Core 가 아니다.
    assert.throws(() => bind(scene, { islands: other.islands }), TypeError);
    // 프레임/테마 표의 188칸 크기가 어긋난다.
    assert.throws(() => bind(scene, { frames: scene.plain.slice(1) }), TypeError);
    assert.throws(() => bind(scene, { themes: scene.themes.slice(1) }), TypeError);
    // 필수 효과 네 개를 하나씩 빠뜨린다.
    for (const name of ['otherOwner', 'refreshProduction', 'themeMode', 'missingIsland']) {
        assert.throws(() => bind(scene, { effects: { ...scene.effects, [name]: undefined } }), TypeError, name);
    }
    // 잘못된 9 Player 크기는 신전 Owner 생성에서 거부되고 섬 런타임에 연결되지 않는다.
    assert.throws(() => bind(scene, { players: scene.players.slice(0, 10 * 8) }), TypeError);
    assert.deepEqual(scene.players, beforePlayers);
    assert.deepEqual(scene.records, beforeRecords);
});

test('RawCampaignOwnership: plain getter 는 float 좌표를 먼저 절삭하고 범위 밖·비유한 좌표를 거부한다', () => {
    const scene = makeScene();
    const binding = bind(scene);
    // 빈 지도 칸과 지도 밖은 원본 sentinel 127 이다(signed 로 127).
    assert.equal(binding.islandNumber(10.9, 20.9), 127);
    assert.equal(binding.islandNumber(-5, 3), 127);
    assert.equal(binding.islandNumber(300.5, 3), 127);
    // 비유한·int32 밖 좌표는 거부한다.
    assert.throws(() => binding.islandNumber(Number.NaN, 0), RangeError);
    assert.throws(() => binding.islandNumber(Infinity, 0), RangeError);
    assert.throws(() => binding.islandNumber(3e9, 0), RangeError);
});

test('RawCampaignOwnership: Native 경로는 같은 장기 신전 본체를 빌리고 비신전 분배만 현재 Session 에 남긴다', () => {
    const scene = makeScene();
    const real = bind(scene);
    const calls = [];
    // 실제 ClientConstructionGlobals 의 접근자 형태를 가진 덕타입이다(장기 신전 본체는 독립 조립의 것을 빌린다).
    const owner = {
        vortexOwner: () => real.vortex(),
        islandNumber: (x, y) => { calls.push(['island', x, y]); return 5; },
        regions: () => ({ updateNative: (x, y) => { calls.push(['native', x, y]); } }),
    };
    assert.throws(() => RawCampaignOwnership.native(null, () => {}), TypeError);
    assert.throws(() => RawCampaignOwnership.native(owner, null), TypeError);
    const others = [];
    const wrapper = RawCampaignOwnership.native(owner, (sid, player) => { others.push([sid, player]); });
    assert.equal(wrapper.vortex(), real.vortex());
    assert.equal(wrapper.islandNumber(1.5, 2.5), 5);
    wrapper.updateRegion(3, 4);
    wrapper.updateRegionNative(5, 6);
    assert.deepEqual(calls, [['island', 1.5, 2.5], ['native', 3, 4], ['native', 5, 6]]);
    const sid = scene.world.create(WIND_TEMPLE);
    wrapper.setOwner(sid, 1);
    assert.equal(scene.core.pool.slot(sid)[34], 1);
    assert.equal(scene.counters.refresh, 1);
    put(scene.core.pool.allocated(sid), 0, 0x503360);
    wrapper.setOwner(sid, 8);
    assert.deepEqual(others, [[sid, 8]]);
});
