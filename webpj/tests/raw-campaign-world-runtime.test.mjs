// 실제 캠페인 월드 설치 연결 raw-campaign-world-runtime.js(RawCampaignWorldRuntime)와 로더 훅 팩토리 `makeFortObjectLoadHooks`를 cpppj 계약으로 대조한다(4-D C 구획, WB04-E519).
// cpppj 시험 대응(`CampaignWorldRuntimeTests.cpp`의 설치 장면 중 이 단위가 직접 소유하는 부분):
//  - 구성 거부: 다른 풀의 섬/Player/지형/HP/내용물·HP 장부 불일치·Player 9기록 불일치·9 Template/Territory/180 header/handle/필수 효과 누락은 최초 등록/쓰기 전에 거부한다.
//  - Load: BeginLoad → 깊이 1 → 번역 You·이름 config·창 제목 갱신·Player 생성·실제 섬 배치(보호 Tutorial 1 Territory 120BYTE)·정상 지형 상위·Objects 부모·Graph(false)·카메라 → 종료(깊이 0)를 한 번 실행하고,
//    중복/실패 뒤 재실행을 거부한다.
//  - 로더 훅: 소유권(SetOwner)·월드 로딩 깊이·청크 섬 부호 BYTE·섬 목록 영역 허용·finish 후처리·생성/HP/내용물을 같은 월드에 연결한다.
//  - 일반 Fort: 기존 Money/Meta/Object child 선택 순서(Money 선택은 깊이/진단 뒤 LoadSelected 안에서)와 alternate 분기, 부모 Graph 중복 호출 없음.
// 정상 지형 상위와 저장 객체 본문은 이 단위의 몸체가 아니므로 호출 형태만 기록하는 장치로 대신한다(실제 정상 지형은 E302, 실제 Tutorial1 저장 객체 적재는 Session 조립의 몫).
// 기대값은 cpppj 계약과 같은 관계이며 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E519)
import assert from 'node:assert/strict';
import test from 'node:test';
import { parseFort } from '../www/assets/fort.js';
import { ARCHIVE_PATH } from '../www/assets/game-data.js';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { createFortTypes } from '../www/assets/type-flags.js';
import { FIRST_ASSET_TYPE_NUMBER, TypeTable } from '../www/assets/type-table.js';
import { ChunkMap } from '../www/assets/territory.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { AssetPostPopState } from '../www/core/raw-asset-post-pop-state.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { CampaignWorldGlobals } from '../www/core/raw-campaign-world-load.js';
import { RawCampaignOwnership } from '../www/core/raw-campaign-ownership.js';
import { RawCampaignWorldRuntime } from '../www/core/raw-campaign-world-runtime.js';
import { RawContainedCreate, makeContainedCreateHooks } from '../www/core/contained-create.js';
import { FortObjectLoad, createFortObjectLoadState, makeFortObjectLoadHooks } from '../www/core/fort-object-load.js';
import { RawFortFinishOwner } from '../www/core/raw-fort-finish-owner.js';
import { RawIslandRuntime, createFortFinishOwnerState, createRawIslandStorage } from '../www/core/raw-island-runtime.js';
import { PlayerWorldState, RawPlayerWorld } from '../www/core/raw-player-world.js';
import { RawSurfaceWorld } from '../www/core/raw-surface-world.js';
import { RegionOwnershipState } from '../www/core/raw-region-ownership.js';
import { RegionPaintState } from '../www/core/raw-region-paint.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { loadOriginalForts, readData } from './helpers.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { get, put } from './raw-support.mjs';

// Player 한 칸 폭·첫 지역 위치, 정상 상태 번호와 지면 타입이다. (2026-11-15 WB04-E519)
const STRIDE = 0xb4;
const REGION = 0x78;
const TERRAIN = 97;
const EDGE_FARM = 121;
const WIND_TEMPLE = 123;

// 보호 Tutorial 1 의 Territory 120BYTE 이다. 처음 필요할 때 한 번만 읽는다. (2026-11-15 WB04-E519)
let territoryCache = null;
function warBeginsTerritory() {
    if (territoryCache) return territoryCache;
    const archive = new TaffArchive(readData(ARCHIVE_PATH));
    const entry = loadOriginalForts(archive).find((candidate) => /thewarbegins.fort$/i.test(candidate.source));
    territoryCache = parseFort(entry.bytes, createFortTypes(TypeTable.fromArchive(archive, decodeOriginalText))).territory;
    return territoryCache;
}

// 선택되지 않은 하위 효과는 성공으로 덮지 않고 필요한 단계를 보고한다. (2026-11-15 WB04-E519)
const forbidden = (name) => () => { throw new Error(`초기 캠페인 검사에서 미연결 하위 효과 요청: ${name}`); };

// 보호 TYPE 188칸 월드와 같은 풀의 섬·Player·HP·소유권·finish·내용물, 실제 Tutorial1 Territory 를 한 번 구성한다. (2026-11-15 WB04-E519)
class InstallScene {
    constructor() {
        const real = realPlayerAssets();
        this.types = real.types.map((record) => ({ ...record }));
        // 섬 배치(Builder)가 읽는 실제 타입 프레임 코드와 셀 크기이다(보호 자료의 `.type` 정의와 SHP, 그림 크기는 합성이다).
        const archive = new TaffArchive(readData(ARCHIVE_PATH));
        const table = TypeTable.fromArchive(archive, decodeOriginalText);
        const codes = this.types.map((_, number) => {
            const definition = number >= FIRST_ASSET_TYPE_NUMBER ? table.definitions[number - FIRST_ASSET_TYPE_NUMBER] : null;
            return definition ? definition.clusters.map((cluster) => cluster.code) : [];
        });
        const frames = codes.map((list) => new FrameCodeTable(list));
        const shapes = this.types.map((_, number) => {
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
        Object.assign(links, { bridgeType: 82, islandType: 94, stalagType: 95, connectorType: 155, noIslandType: 157, battle: true, mission: true });
        this.world = new RawSurfaceWorld(this.types, frames, shapes, links, TERRAIN);
        this.core = this.world.core();
        this.random = new GameRandom(0x12345678);
        this.sp = new ScrambledSpStore(this.random, () => 1);
        this.hp = new SquidReward(this.core.pool, this.core.types, this.core.bookkeeping, new SquidRewardState(), this.sp);
        this.chunks = new ChunkMap();
        this.assets = new AssetPostPopState();
        this.assets.residences.entries.length = 100;
        this.finishState = createFortFinishOwnerState();
        this.finishState.chaffMode = true;
        this.islandBytes = createRawIslandStorage();
        this.players = new Uint8Array(9 * STRIDE);
        for (let owner = 0; owner < 9; owner += 1) put(this.players, owner * STRIDE + REGION, 127);
        this.playerState = new PlayerWorldState();
        this.playerState.playerKnown = 1;
        this.playerState.battleOwner = 1;
        this.playerState.templateOwner = 1;
        this.events = [];
        // 섬 목록·소유권은 같은 풀·9기록을 공유한다. 섬 owner 변경이 가상 소유권을 부르는 경계는 실제 소유권 연결이다.
        this.islands = new RawIslandRuntime(this.core.pool, this.core.hash, this.islandBytes, this.finishState, this.assets.residences, { value: 0 }, { value: 0 }, {
            setResidenceOwner: (sid, owner) => { this.ownership.setOwner(sid, owner); },
            assertion: forbidden('섬 목록 진단'),
        });
        this.islands.reset();
        this.themes = new Array(this.types.length).fill(0);
        this.plain = this.types.map((_, type) => ({ frames: this.core.frames[type].codes, defaultFrame: 0 }));
        this.otherOwners = [];
        this.ownership = new RawCampaignOwnership(this.world, this.islands, this.assets, this.players, this.plain, this.themes, this.random, new RegionPaintState(), new RegionOwnershipState(),
            { value: 0 }, TERRAIN, EDGE_FARM, {
                otherOwner: (sid, owner) => { this.otherOwners.push([sid, owner]); },
                refreshProduction: () => { this.events.push('생산 Gump 요청'); },
                themeMode: () => 0,
                missingIsland: forbidden('신전 섬 진단'),
            });
        this.player = new RawPlayerWorld(this.core.pool, this.core.postPop, this.players, this.playerState, {
            assertion: forbidden('Player 진단'),
            find: () => 0,
            create: (type, flags) => this.world.create(type, flags),
            setOwner: (sid, owner) => { this.core.owner.set(sid, owner); },
            pop: (sid, x, y, flags) => { this.events.push(`L:${this.playerState.battleOwner}:${x}:${y}:${flags}`); },
            reserveRegular: () => true,
            addRegular: () => {},
        });
        this.finish = new RawFortFinishOwner(this.core.pool, this.core.types, this.assets.residences, this.finishState, {
            regionBounds: forbidden('finish 영역 경계'), wallSeconds: forbidden('finish 시계'), place: forbidden('finish 배치'), invalidType: forbidden('finish 타입 진단'),
        });
        // 저장 내용물은 같은 풀의 Factory 로 만들고 표면 알림/부착은 관찰 장치로 둔다(이 단위가 직접 만들지는 않는다).
        this.contained = new RawContainedCreate(this.core.pool, { containedType: () => 1, bridgeType: () => 82 },
            makeContainedCreateHooks(this.core.pool, this.core.factory, { notifySurface: forbidden('내용물 표면 통지'), attach: forbidden('내용물 부착') }));
        this.objectState = createFortObjectLoadState({ value: 0 });
        this.globals = new CampaignWorldGlobals({ my: 1, login: 2, editor: 0, missionPost: true });
        this.puzzlePiece = { value: 107 };
        this.noRegion = { value: 127 };
        this.headers = new Uint8Array(180);
        this.handle = new Uint8Array(64);
        this.translated = Uint8Array.from([89, 111, 117, 0]);
        this.templates = Array.from({ length: 9 }, (_, slot) => ({ flag: slot }));
        const territory = warBeginsTerritory();
        assert.equal(territory.length, 120);
        this.streams = Array.from({ length: 9 }, () => ({ bytes: territory.slice(), position: 0, end: 120 }));
        this.headers.set([1, 0, 0, 0], 20);
        this.generated = [];
        this.campaignObjectCalls = [];
        // 정상 지형 상위는 Builder 가 준 owner/경계/지역만 기록한다(실제 정상 지형은 E302).
        this.normal = { pool: this.core.pool, generateOriginal: (owner, bounds, region) => { this.generated.push({ owner, bounds, region }); } };
    }

    // 설치 효과 묶음이다. 개별 항목을 덮어써서 거부 시험에 쓴다. (2026-11-15 WB04-E519)
    effects(overrides = {}) {
        return {
            translateYou: () => { this.events.push('Y'); return this.translated; },
            setConfig: (key, bytes) => { assert.equal(key, 'fort.mySubHandle'); assert.equal(bytes.length, 4); this.events.push('S'); },
            refreshWindowTitle: () => { this.events.push('V'); },
            resetCamera: () => { this.events.push('K'); },
            builderAssertion: forbidden('Builder 진단'),
            territoryAssertion: forbidden('Territory 진단'),
            notifySurface: (sid) => { this.core.building.notifySurface(sid); },
            setState: forbidden('Factory 상태'),
            virtualCount: forbidden('가상 개수'),
            ...overrides,
        };
    }

    // 상위 저장소 묶음이다. campaignObjects 는 Session 이 raw 스트림으로 실행하는 원문 004be610 의 관찰 장치이다. (2026-11-15 WB04-E519)
    data(overrides = {}) {
        return {
            templates: this.templates,
            territories: this.streams,
            headers: this.headers,
            handle: this.handle,
            campaignObjects: (objects, slot) => { this.campaignObjectCalls.push([objects, slot]); this.events.push(`O:${slot}`); },
            ...overrides,
        };
    }

    // 런타임을 만든다. 거부 시험은 개별 입력을 덮어쓴다. (2026-11-15 WB04-E519)
    runtime(input = {}) {
        const arguments_ = {
            world: this.world, chunks: this.chunks, islands: this.islands, player: this.player, normal: this.normal, random: this.random, hp: this.hp, ownership: this.ownership,
            finish: this.finish, contained: this.contained, objectState: this.objectState, globals: this.globals, puzzlePiece: this.puzzlePiece, noRegion: this.noRegion,
            data: this.data(), effects: this.effects(), ...input,
        };
        return new RawCampaignWorldRuntime(arguments_.world, arguments_.chunks, arguments_.islands, arguments_.player, arguments_.normal, arguments_.random, arguments_.hp, arguments_.ownership,
            arguments_.finish, arguments_.contained, arguments_.objectState, arguments_.globals, arguments_.puzzlePiece, arguments_.noRegion, arguments_.data, arguments_.effects);
    }
}

test('RawCampaignWorldRuntime: Load 가 BeginLoad·깊이 1·캠페인 부모 순서를 실행하고 깊이 0 으로 마친다', () => {
    const scene = new InstallScene();
    const runtime = scene.runtime();
    assert.equal(runtime.pool, scene.core.pool);
    assert.ok(runtime.objects instanceof FortObjectLoad);
    assert.equal(runtime.objects.pool, scene.core.pool);
    runtime.load();
    // 번역 → 이름 config → 창 제목 → Player 생성(실제 Player 152 팝, battleOwner 1 의 좌표 x=11 y=1) → Objects → 카메라 순서이다.
    assert.deepEqual(scene.events, ['Y', 'S', 'V', 'L:1:11:1:0', 'O:1', 'K']);
    // 정상 지형 상위는 Builder 의 owner 1 / 경계 1..15 / 현재 noRegion 127 로 한 번 불린다.
    assert.equal(scene.generated.length, 1);
    assert.deepEqual(scene.generated[0].bounds, { left: 1, top: 1, right: 15, bottom: 15 });
    assert.equal(scene.generated[0].owner, 1);
    assert.equal(scene.generated[0].region, 127);
    // campaignObjects 는 같은 실제 저장 객체 로더를 받는다.
    assert.equal(scene.campaignObjectCalls[0][0], runtime.objects);
    // 보호 Tutorial1 Territory 가 실제 섬 목록을 만들었고 로딩이 끝났다.
    assert.ok(scene.islands.exists(0));
    assert.equal(scene.objectState.loadingDepth, 0);
    // 최초 Template 헤더 +8 DWORD 는 1 이다.
    assert.equal(get(scene.headers, 20), 1);
});

test('RawCampaignWorldRuntime: 중복 Load 와 실패 뒤 재실행을 거부한다', () => {
    const scene = new InstallScene();
    const runtime = scene.runtime();
    runtime.load();
    assert.throws(() => runtime.load(), /중복\/실패 후 설치 재실행/);
    const failing = new InstallScene();
    const broken = failing.runtime({ effects: failing.effects({ translateYou: () => { throw new Error('번역 실패'); } }) });
    assert.throws(() => broken.load(), /번역 실패/);
    // 실패는 로드 완료로 바뀌지 않고 같은 연결의 재실행도 거부한다.
    assert.throws(() => broken.load(), /중복\/실패 후 설치 재실행/);
});

test('RawCampaignWorldRuntime: 구성 거부는 월드와 공유 기록을 바꾸기 전에 일어난다', () => {
    const scene = new InstallScene();
    const other = new InstallScene();
    const playersBefore = Uint8Array.from(scene.players);
    const islandsBefore = Uint8Array.from(scene.islandBytes);
    const headersBefore = Uint8Array.from(scene.headers);
    // 다른 월드의 섬/Player/지형/HP/내용물/finish 는 같은 Core 가 아니다.
    assert.throws(() => scene.runtime({ islands: other.islands }), TypeError);
    assert.throws(() => scene.runtime({ player: other.player }), TypeError);
    assert.throws(() => scene.runtime({ normal: other.normal }), TypeError);
    assert.throws(() => scene.runtime({ hp: other.hp }), TypeError);
    assert.throws(() => scene.runtime({ contained: other.contained }), TypeError);
    assert.throws(() => scene.runtime({ finish: other.finish }), TypeError);
    // 신전 Owner 의 Player 9기록이 설치의 9기록과 다르다.
    assert.throws(() => scene.runtime({ ownership: other.ownership }), TypeError);
    // 9 Template/Territory 가 아니거나 getter 가 없다.
    assert.throws(() => scene.runtime({ data: scene.data({ templates: scene.templates.slice(1) }) }), TypeError);
    assert.throws(() => scene.runtime({ data: scene.data({ territories: scene.streams.slice(1) }) }), TypeError);
    // 180 header 와 NUL 포함 handle 저장소 크기이다.
    assert.throws(() => scene.runtime({ data: scene.data({ headers: new Uint8Array(179) }) }), TypeError);
    assert.throws(() => scene.runtime({ data: scene.data({ handle: new Uint8Array(0) }) }), TypeError);
    // 필수 효과 아홉 개는 하나라도 빠지면 거부한다.
    for (const name of ['translateYou', 'setConfig', 'refreshWindowTitle', 'resetCamera', 'builderAssertion', 'territoryAssertion', 'notifySurface', 'setState', 'virtualCount']) {
        assert.throws(() => scene.runtime({ effects: scene.effects({ [name]: undefined }) }), TypeError, name);
    }
    assert.deepEqual(scene.players, playersBefore);
    assert.deepEqual(scene.islandBytes, islandsBefore);
    assert.deepEqual(scene.headers, headersBefore);
    assert.equal(scene.events.length, 0);
});

test('RawCampaignWorldRuntime: 저장 객체 로더 훅이 같은 소유권·월드·청크 지도·섬 목록·생성 모듈에 연결된다', () => {
    const scene = new InstallScene();
    const runtime = scene.runtime();
    const hooks = runtime.objects.hooks;
    // 소유권: TYPE 123 신전의 가상 소유자 변경이 같은 Player 기록에 쓰인다.
    const sid = scene.world.create(WIND_TEMPLE);
    hooks.setOwner(sid, 1);
    assert.equal(scene.core.pool.slot(sid)[34], 1);
    assert.equal(get(scene.players, STRIDE + 0x7c), sid);
    // 청크 섬 번호는 부호 있는 BYTE 이다.
    scene.chunks.at(2, 3).island = 0xfe;
    assert.equal(hooks.chunkRegion(2, 3), -2);
    scene.chunks.at(2, 3).island = 5;
    assert.equal(hooks.chunkRegion(2, 3), 5);
    // 영역 허용은 섬 목록의 Matches 이다(없는 섬은 거짓, 일반 모드는 원래 owner BYTE 를 비교한다).
    scene.finishState.chaffMode = false;
    assert.equal(hooks.territoryAllowed(0, 1, 0), false);
    scene.islands.add(1, 0);
    assert.equal(hooks.territoryAllowed(0, 1, 0), true);
    assert.equal(hooks.territoryAllowed(0, 2, 0), false);
    // 월드 로딩 깊이는 월드와 같은 값이다.
    hooks.loadingDepthChanged(3);
    // 생성/HP 는 같은 풀의 실제 모듈이다.
    const created = hooks.create(WIND_TEMPLE, 0);
    assert.ok(created > 0);
    assert.equal(hooks.hasHitPoints(created), scene.hp.hasHitPoints(created));
    assert.equal(hooks.maxHitPoints(created), scene.hp.maxHitPoints(created));
});

test('makeFortObjectLoadHooks: 다른 풀의 생성/HP/내용물은 어떤 훅도 연결하지 않고 거부한다', () => {
    const scene = new InstallScene();
    const other = new InstallScene();
    const base = { chunkRegion: () => 0 };
    assert.throws(() => makeFortObjectLoadHooks(scene.core.pool, other.core.factory, scene.hp, scene.contained, base), TypeError);
    assert.throws(() => makeFortObjectLoadHooks(scene.core.pool, scene.core.factory, other.hp, scene.contained, base), TypeError);
    assert.throws(() => makeFortObjectLoadHooks(scene.core.pool, scene.core.factory, scene.hp, other.contained, base), TypeError);
    const hooks = makeFortObjectLoadHooks(scene.core.pool, scene.core.factory, scene.hp, scene.contained, base);
    // 입력 훅은 보존하고 네 개만 채운다.
    assert.equal(hooks.chunkRegion, base.chunkRegion);
    for (const name of ['create', 'hasHitPoints', 'maxHitPoints', 'createContent']) assert.equal(typeof hooks[name], 'function', name);
});

test('RawCampaignWorldRuntime: 일반 Fort 는 Money 선택을 LoadSelected 안에서 하고 부모 Graph 를 중복 호출하지 않는다', () => {
    const scene = new InstallScene();
    const order = [];
    const existingFort = (objects) => ({
        loadObjects: (owner) => { order.push(`objects:${owner}`); },
        money: { loadSelected: (select) => { order.push('money:loadSelected'); select(); } },
        meta: { pool: scene.core.pool, loadBadgesSelected: (select) => { select(); }, loadTechnologySelected: (select) => { select(); } },
        selectMoney: (fort, owner) => { order.push(`selectMoney:${owner}`); },
        moneyStream: (fort, owner) => { order.push(`moneyStream:${owner}`); return {}; },
        selectMeta: () => {},
        metaStream: () => ({}),
        objects,
    });
    const runtime = scene.runtime({ data: scene.data({ existingFort }) });
    // 일반 Fort 만 loadExistingFort 를 받으며 beforeWorld 가 없으면 거부한다.
    assert.throws(() => runtime.loadExistingFort(), /일반Fort실제자원/);
    let before = 0;
    // alternate 1: Objects/Money/Badges/Technology 읽기를 생략하고 섬 0 이 있으면 populate 경계를 요구한다(미연결 오류).
    assert.throws(() => runtime.loadExistingFort(() => { before += 1; }, 1), /alternate Fort populate 미연결/);
    assert.equal(before, 1);
    // 실패 뒤 재실행은 거부한다.
    assert.throws(() => runtime.loadExistingFort(() => {}, 0), /일반Fort실제자원/);
    // 일반 Fort 가 아닌 연결은 loadExistingFort 를 거부한다.
    const plain = new InstallScene();
    assert.throws(() => plain.runtime().loadExistingFort(() => {}), /일반Fort실제자원/);
});

test('RawCampaignWorldRuntime: readTerritory 는 같은 현재 stream 에서 120BYTE 를 읽고 위치를 120 올린다', () => {
    const scene = new InstallScene();
    const runtime = scene.runtime();
    const output = new Uint8Array(120);
    // 현재 Template 헤더가 있는 칸(1)에서 읽는다.
    runtime.readTerritory(1, output);
    assert.deepEqual(output, warBeginsTerritory());
    assert.equal(scene.streams[1].position, 120);
    // 헤더가 비어 있는 칸은 진단 요청(territoryAssertion) 뒤 계속한다. 이 장면의 진단은 미연결 오류이다.
    assert.throws(() => runtime.readTerritory(0, new Uint8Array(120)), /미연결 하위 효과/);
    // 9칸 밖은 거부한다.
    assert.throws(() => runtime.readTerritory(9, new Uint8Array(120)), RangeError);
});

test('RawCampaignWorldRuntime: 일반 Fort alternate 0 은 Objects → Money(선택 뒤 LoadSelected) → Badges → Technology 를 현재 owner 로 읽고 종료 깊이 0 이다', () => {
    const scene = new InstallScene();
    const order = [];
    const existingFort = () => ({
        loadObjects: (owner) => { order.push(`objects:${owner}`); },
        money: { loadSelected: (select) => { order.push('money:loadSelected'); select(); } },
        meta: {
            pool: scene.core.pool,
            loadBadgesSelected: (select) => { order.push('badges:loadSelected'); select(); },
            loadTechnologySelected: (select) => { order.push('technology:loadSelected'); select(); },
        },
        selectMoney: (fort, owner) => { order.push(`selectMoney:${owner}`); },
        moneyStream: (fort, owner) => { order.push(`moneyStream:${owner}`); return {}; },
        selectMeta: (fort, owner, technology) => { order.push(`selectMeta:${owner}:${technology}`); },
        metaStream: (fort, technology) => { order.push(`metaStream:${technology}`); return {}; },
    });
    const runtime = scene.runtime({ data: scene.data({ existingFort }) });
    let before = 0;
    runtime.loadExistingFort(() => { before += 1; order.push('before'); }, 0);
    assert.equal(before, 1);
    // 외곽 준비 → 네 child 순서이며 Money 의 stream 선택은 wrapper 의 selectMoney 뒤 LoadSelected 안에서 일어난다.
    assert.deepEqual(order, ['before', 'objects:1', 'selectMoney:1', 'money:loadSelected', 'moneyStream:1', 'selectMeta:1:false', 'badges:loadSelected', 'metaStream:false',
        'selectMeta:1:true', 'technology:loadSelected', 'metaStream:true']);
    assert.equal(scene.objectState.loadingDepth, 0);
    // 보호 Tutorial1 Territory 의 정상 지형 상위는 Builder 경계로 한 번 불렸다.
    assert.equal(scene.generated.length, 1);
    // 같은 연결은 다시 읽지 않는다.
    assert.throws(() => runtime.loadExistingFort(() => {}), /일반Fort실제자원/);
});
