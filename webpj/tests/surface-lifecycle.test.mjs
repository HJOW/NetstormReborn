// 섬 생성부터 삭제·Regular 낙하·끝 칸 교체·연결 객체 정리까지 실제 raw 모듈로 이어 검사한다(cpppj `SurfaceLifecycleTests.cpp` 이식, WB04-E248).
//  - 대상: raw-surface-lifecycle.js(RawSurfaceLifecycle, 섬·다리·noIsland 삭제 훅을 공통 삭제에 잇는 가상 표 분배기).
//  - 장면: 9칸 noIsland(받침·종유석 자동 생성) 위에 다리 하나와 이웃 다리, 섬 받침 삭제가 유일한 낙하 예약 트리거이다. 실제 SquidFactory·SquidOwner·SquidPop/Unpop·SquidDisplay·SquidFrame·RawBridgeConnect·
//    RawIslandPostPop·RawBridgeEvents·RawSquidDestroy/Dispatch·SquidDestroyLifecycle·SquidProcessHost·Kernel 을 한 월드로 조립하며, 입력(TYPE 표·합성 SHP 셀)만 합성이고 기대 관계식은 모듈이 낸 결과의 관계이다.
//  - 시나리오: Replace(끝 칸 O 교체와 옛 다리·연결 정리) · Isolated(이웃 없는 다리 삭제) · Hard(단단한 다리는 수명만 바꾸고 예약·연결 유지, Graph 활성이면 가상 삭제 거부 뒤 편집기에서 정리) · Keep(삭제 flags 0x1000 은 예약 생략).
//    변형: 표면 칸을 먼저 삭제해 walker 낙하 분배를 확인(removeSurfaceBefore), Graph 활성(표면 수 장부), Native 연결 TYPE 주소 대여(초기 사본은 다른 값).
//  - 계약: 훅 누락·연결 TYPE 범위·다른 풀 거부, 타입 번호가 다리여도 가상 표가 다르면 공통 훅만 사용.
// CD 10.72 판본 장면은 웹 범위 밖이다(cpppj 는 두 판본). 실제 SHP 로 표시 변경 영역을 보는 선택 검사(InspectSurfaceLifecycle)와 Renderer 합성은 이 검사에 포함하지 않는다. (2026-11-10 WB04-E248)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { Kernel } from '../www/core/kernel.js';
import { DEFAULT_BRIDGE_DECAY_MODE } from '../www/core/bridge-decay.js';
import { BRIDGE_FALL_EVENT, RawBridgeLifecycle } from '../www/core/bridge-lifecycle.js';
import { BRIDGE_DESTROY_EVENT, BRIDGE_EVENT_KEEP, RawBridgeEvents, makeBridgeNeighborHooks, makeBridgeRegularHandler } from '../www/core/raw-bridge-events.js';
import { BridgeConnectState, RawBridgeConnect, makeOwnerDispatch } from '../www/core/raw-bridge-connect.js';
import { RawGraph } from '../www/core/raw-graph.js';
import { ISLAND_DESTROY_KEEPS_BRIDGES } from '../www/core/raw-island-lifecycle.js';
import { RawIslandPostPop } from '../www/core/raw-island-post-pop.js';
import { RawSquidDestroyDispatch } from '../www/core/raw-squid-destroy-dispatch.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { RawSurfaceLifecycle } from '../www/core/raw-surface-lifecycle.js';
import { SidPool } from '../www/core/sid-pool.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { SquidDeletionState, SquidDestroyLifecycle } from '../www/core/squid-destroy-lifecycle.js';
import { SquidDisplay } from '../www/core/squid-display.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { makeSquidFrameHooks } from '../www/core/squid-frame-binding.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { RAW_POP_RESULT, SquidPop } from '../www/core/squid-pop.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidProcessHost, SquidProcessState, hasScheduledBridgeFall, scheduleBridgeFall } from '../www/core/squid-process.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { realPlayerAssets } from './player-scene-support.mjs';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, get, put, readRows } from './raw-support.mjs';

// 실제 타입 번호와 공통 raw 필드 위치, 상태 비트, 3×3 표면 글자 순서이다. 연결 객체는 155, 표면(noIsland)은 157 이다. (2026-11-10 WB04-E248)
const BRIDGE = 82;
const ISLAND = 94;
const STALAG = 95;
const CONNECTOR = 155;
const SURFACE = 157;
const TYPE = 10;
const STATE = 11;
const WORD = 12;
const X = 14;
const Y = 18;
const FREE = 1;
const DEAD = 2;
const SURFACE_LETTERS = 'FCGBADIEH';
const BRIDGE_LIFE_MASK = 0x78;
const FRAME_OFFSET = 36;
const OWNER_OFFSET = 34;
const EXTRA_OFFSET = 40;
const VTABLE = 0x11010000;

// 원본 bridge.type 에서 추출해 저장한 실제 프레임 코드 표(Frames 0 행)이다. 파일이 없으면 null 이고 검사를 건너뛴다. (2026-11-10 WB04-E248)
const bridgeRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bridgedecay-x86.tsv'));
function bridgeFrames() {
    const row = bridgeRows.find((item) => item[0] === 'Frames' && item[1] === '0');
    assert.ok(row, 'Frames 0 행');
    const codes = [];
    // 네 바이트(방향 글자·변형 글자·번호·플래그)가 하나의 실제 코드이다.
    for (let index = 0; index + 8 <= row[2].length; index += 8) {
        const byte = (offset) => parseInt(row[2].slice(index + offset * 2, index + offset * 2 + 2), 16);
        codes.push({ side: byte(0), variant: byte(1), number: byte(2), flags: byte(3) });
    }
    return codes;
}

// 표면은 첫 프레임만, 받침/종유석/연결 객체는 방향 A 의 합성 프레임을 공급한다. (2026-11-10 WB04-E248)
function frameTable(count) {
    const frames = Array.from({ length: count }, () => []);
    frames[BRIDGE] = bridgeFrames();
    frames[SURFACE] = [...SURFACE_LETTERS].map((side) => ({ side: side.charCodeAt(0), variant: 0x50, number: 1, flags: 0 }));
    // 방향은 이 세 비표면 타입의 연결 판단에 쓰이지 않는다.
    for (const type of [ISLAND, STALAG, CONNECTOR]) frames[type] = Array.from({ length: 9 }, () => ({ side: 0x41, variant: 0x50, number: 1, flags: 0 }));
    return frames;
}

// 예외가 발생하는지 본다. (2026-11-10 WB04-E248)
function throws(action, kind = Error) {
    try { action(); } catch (error) { return error instanceof kind; }
    return false;
}

// 한 월드에서 실제 모듈을 구성하고 섬 받침 삭제를 유일한 낙하 예약 트리거로 쓴다. (2026-11-10 WB04-E248)
function run(scenario, { removeSurfaceBefore = false, graphActive = false, nativeConnector = false } = {}) {
    const pool = new SidPool(32768, { server: true });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const types = realPlayerAssets().types.map((record) => ({ ...record }));
    // 실제 생성자·플래그·발자국을 입력하고 다리만 검사 비용 5 를 둔다.
    const define = (number, flags1, flags2, foot) => {
        Object.assign(types[number], { constructorAddress: typeConstructorAddress(number), flags1, flags2, footX: foot, footY: foot });
    };
    define(BRIDGE, 0x802, 4, 1);
    types[BRIDGE].cost = 5;
    define(ISLAND, 0x28000003, 0x01000000, 3);
    define(STALAG, 0x28000002, 0, 3);
    define(CONNECTOR, 0x28000002, 0x02000000, 1);
    define(SURFACE, 0x08000803, 0x01000002, 1);
    // 표면 삭제의 분배만 확인하는 합성 walker 타입이다.
    Object.assign(types[85], { flags2: 0x10000, footX: 1, footY: 1 });
    const frames = frameTable(types.length);
    const graph = new RawGraph(pool, hash, spots, types, frames);
    const activeGraph = graphActive ? graph : null;
    // 빈 월드의 원본 전체 초기화는 0번을 예약한다. 생략하면 첫 Flood 가 같은 번호로 끝난다.
    if (graphActive) graph.rebuild(true);
    const book = new SquidPostPopState();
    book.localOwner = 3;
    book.graphsEnabled = graphActive;
    let linker = null;
    let surfaceLinker = null;
    // 순환하는 생성/Pop 경로는 구성이 끝난 뒤 연결한다.
    const postPop = new SquidPostPop(pool, types, book, activeGraph, (sid) => linker.connect(sid));
    postPop.setIslandPrefix((sid, flags) => linker.islandPostPopPrefix(sid, flags));
    postPop.setSurfacePrefix((sid, flags) => surfaceLinker.prefix(sid, flags));
    // 단위 검사는 별도 셀 크기를 공급한다. 크기가 있는 타입만 표시 대상이다.
    const shapes = types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
    for (const type of [BRIDGE, ISLAND, STALAG, CONNECTOR, SURFACE]) {
        shapes[type] = { loaded: true, frameCount: frames[type].length, frames: frames[type].map(() => ({ width: 20, height: 16, hotspotX: 8, hotspotY: 12, cellWidth: types[type].footX, cellHeight: types[type].footY })) };
    }
    let notices = 0;
    const sink = { suppressed: () => false, invalidate: () => { notices += 1; } };
    const display = new SquidDisplay(types, shapes, sink, { cameraX: 0, cameraY: 0, zoom: 65536, viewport: { left: 0, top: 0, right: 640, bottom: 480 } });
    const popRequests = [];
    const unpop = new SquidUnpop(pool, hash, spots, display);
    const pop = new SquidPop(pool, hash, spots, { display, postPop });
    const factory = new SquidFactory(pool, types, { unpop });
    // 실제 Pop 관문을 통과한 요청만 기록하며 원래 flags/공간 효과를 바꾸지 않는다.
    pop.bindTransmission((sid, flags) => { popRequests.push([sid, flags]); });
    const ownerMode = new SquidOwnerMode();
    ownerMode.battle = true;
    const owner = new SquidOwner(pool, types, book, ownerMode);
    const neighbors = new RawSquidNeighbors(pool, hash, spots, types, frames);
    const typeState = new BridgeConnectState();
    Object.assign(typeState, { bridgeType: BRIDGE, islandType: ISLAND, stalagType: STALAG, connectorType: CONNECTOR, noIslandType: SURFACE, battle: true });
    const links = [];
    let born = 0;
    const objects = {
        // 연결 생성도 같은 Factory 와 raw 가상 표를 사용한다.
        create: (type, flags) => {
            const sid = factory.create(type, flags);
            if (type === CONNECTOR) links.push(sid);
            return sid;
        },
        setOwner: makeOwnerDispatch(pool, typeState, (sid, player) => owner.set(sid, player)),
        // 실제 Pop 의 공간 쓰기와 파생 postPop 을 모두 실행한다.
        pop: (sid, x, y, flags) => {
            const raw = pool.slot(sid);
            const size = display.frameSize(raw, get(raw, FRAME_OFFSET, 4));
            assert.equal(pop.pop(sid, types[raw[TYPE]], size[0], size[1], x, y, flags), RAW_POP_RESULT.registered);
        },
        notifySurface: () => {},
    };
    const frame = new SquidFrame(pool, types, makeSquidFrameHooks(pool, types, display, unpop, pop));
    objects.setFrame = (sid, value, flags) => frame.set(sid, value, flags);
    const connect = new RawBridgeConnect(pool, neighbors, typeState, objects);
    linker = connect;
    let displays = 0;
    const surface = new RawIslandPostPop(pool, hash.view(0), types, frames, typeState, { loadingDepth: 1, savedGame: () => false }, {
        objects,
        connect: (sid) => connect.connect(sid),
        findTypeAt: (x, y, type) => connect.findTypeAt(x, y, type),
        updateDisplay: (sid) => { displays += 1; display.update(pool.slot(sid)); },
        incompleteSupport: () => { throw new Error('받침 구성 누락'); },
    });
    surfaceLinker = surface;
    // 요새 로드와 같은 9칸 등록이 받침과 종유석을 실제 생성한다. 첫 칸의 소유자로 마지막 H 에서 묶음 소유자가 전파된다.
    for (let y = 28; y <= 30; y += 1) {
        for (let x = 28; x <= 30; x += 1) {
            const cell = factory.create(SURFACE);
            owner.set(cell, x === 28 && y === 28 ? 3 : 0);
            objects.pop(cell, x, y, 0);
        }
    }
    const island = connect.findTypeAt(30, 30, ISLAND);
    const stalag = connect.findTypeAt(30, 30, STALAG);
    assert.ok(island && stalag);
    assert.equal(displays, 11);
    assert.ok(popRequests.length > 0, '현재 Pop 의 실제 요청 관측 경계까지 도달해야 한다');
    const terrainCost = book.totalCost;
    const source = factory.create(BRIDGE);
    owner.set(source, 3);
    // 실제 K 프레임 첫 번호 41, hard 번호 46 이다. 소유자/수명/추가 비트 복사도 확인한다.
    put(pool.allocated(source), FRAME_OFFSET, scenario === 'Hard' ? 46 : 41, 4);
    objects.pop(source, 31, 29, 0);
    put(pool.allocated(source), 8, 23, 2);
    pool.allocated(source)[EXTRA_OFFSET] |= 0x10;
    assert.equal(links.length, 1);
    assert.equal(get(pool.slot(links[0]), WORD, 2), source);
    const link = links[0];
    const second = get(pool.slot(link), 8, 2);
    const linkLevel = pool.slot(link)[33];
    assert.equal(pool.slot(second)[TYPE], SURFACE);
    assert.equal(hash.bucket(linkLevel, 30, 29), link);
    let survivor = 0;
    if (scenario !== 'Isolated') {
        survivor = factory.create(BRIDGE);
        owner.set(survivor, 3);
        put(pool.allocated(survivor), FRAME_OFFSET, 41, 4);
        objects.pop(survivor, 32, 29, 0);
    }
    const destroy = new RawSquidDestroy(pool, unpop, types);
    const mode = { ...DEFAULT_BRIDGE_DECAY_MODE };
    const virtualDestroy = new RawSquidDestroyDispatch(destroy, neighbors, mode, activeGraph);
    const deletionState = new SquidDeletionState();
    const common = new SquidDestroyLifecycle(pool, types, book, deletionState, destroy, {
        emit: () => {},
        forward: (event) => assert.equal(event.effect, SQUID_DESTROY_EFFECT.transmit),
    }, activeGraph);
    const kernel = new Kernel();
    const processState = new SquidProcessState();
    processState.now = 10;
    let host = null;
    const removedLinks = [];
    const fallen = [];
    const effects = [];
    const order = [];
    const lifecycle = new RawSurfaceLifecycle(pool, neighbors, nativeConnector ? CONNECTOR + 1 : CONNECTOR, common.hooks(), {
        // 예약 직전에는 두 참조가 살아 있으므로 연결 객체를 지우지 않는다.
        scheduleFall: (sid, x, y) => {
            assert.equal(sid, source);
            order.push('schedule');
            assert.ok(scheduleBridgeFall(host, sid, x, y));
        },
        // 다리 preDestroy 안의 연결 삭제도 최종 ProcessHost 훅으로 재진입한다.
        destroy: (sid, flags) => {
            assert.equal(sid, link);
            assert.ok(pool.slot(source)[STATE] & DEAD);
            removedLinks.push(sid);
            order.push('link');
            assert.ok(virtualDestroy.destroy(sid, flags, host.hooks()));
        },
        fallWalker: (sid) => { fallen.push(sid); },
        bridgeEffect: (event) => { effects.push(event.effect); order.push('effect'); },
    });
    // 다른 초기 사본 대신 실제 현재 155 전역을 읽어야 링크가 정리된다.
    if (nativeConnector) lifecycle.bindCurrentConnectorType(() => typeState.connectorType);
    const events = new RawBridgeEvents(pool, { authority: true, debugKeep: false, bridgeType: BRIDGE }, makeBridgeNeighborHooks(neighbors, {
        // 수명 단어 쓰기의 진단 경계만 대체한다.
        notifySurface: () => {},
        create: (type) => { born = factory.create(type); return born; },
        // 가상 삭제의 거부도 정상 결과다. 이벤트 처리기는 원본처럼 삭제 성공 여부를 읽지 않는다.
        destroy: (sid, flags) => {
            assert.equal(sid, source);
            order.push('bridge');
            virtualDestroy.destroy(sid, flags, host.hooks());
        },
        setOwner: (sid, player) => objects.setOwner(sid, player),
        pop: objects.pop,
    }));
    host = new SquidProcessHost(pool, types, kernel, destroy, processState, { regular: makeBridgeRegularHandler(pool, events), asset: lifecycle.hooks() });
    const linkNeeds = () => new RawBridgeLifecycle(pool, types).linkNeedsDestroy(link);
    assert.equal(linkNeeds(), false);
    // 살아 있는 표면의 실제 raw 번호로 그래프 표의 연결 성분과 표면 수를 검사한다.
    const checkGraph = (expected) => {
        if (!graphActive) return;
        const counts = new Map();
        // 해시 0단계에 등록된 모든 표면이 유효한 활성 그래프에 속해야 한다.
        for (let value = 5; value < pool.capacity; value += 1) {
            const raw = pool.slot(value);
            if ((raw[STATE] & (1 | 2 | 4 | 8)) || raw[TYPE] < 70 || !(types[raw[TYPE]].flags1 & 0x800)) continue;
            const number = raw[30];
            assert.notEqual(number, 254);
            assert.notEqual(graph.records()[number].inUse, 0);
            counts.set(number, (counts.get(number) ?? 0) + 1);
        }
        // 첫 객체의 기본 번호 0 은 Flood 에서 감소한다. 예약 레코드의 -1 흔적을 표면 수로 세지 않는다.
        assert.equal(counts.get(0) ?? 0, 0);
        assert.deepEqual([graph.records()[0].surfaces, graph.records()[0].inUse, graph.records()[0].reserved], [-1, 1, 1]);
        let total = 0;
        // 예약 0번 다음 활성 레코드의 signed WORD 수와 실제 슬롯 수를 하나씩 대조한다.
        for (let index = 1; index < 251; index += 1) {
            const count = counts.get(index) ?? 0;
            assert.equal(graph.records()[index].surfaces, count, `레코드 ${index}`);
            assert.equal(graph.records()[index].inUse !== 0, count !== 0, `사용 ${index}`);
            total += count;
        }
        assert.deepEqual([total, book.depth, destroy.preDepth, destroy.postDepth], [expected, 0, 0, 0]);
    };
    checkGraph(scenario === 'Isolated' ? 10 : 11);
    let walker = 0;
    if (removeSurfaceBefore) {
        // walker 는 합성 raw 입력이며 표면 삭제/Unpop/탐색/통계 갱신은 실제 모듈이다.
        walker = pool.allocate(2);
        const raw = pool.allocated(walker);
        put(raw, 0, VTABLE);
        raw[TYPE] = 85;
        raw[STATE] = 0;
        put(raw, X, floatToBits(29));
        put(raw, Y, floatToBits(29));
        put(raw, 4, hash.bucket(1, 29, 29), 2);
        hash.setBucket(1, 29, 29, walker);
        const cell = connect.findTypeAt(29, 29, SURFACE);
        assert.notEqual(cell, 0);
        assert.ok(virtualDestroy.destroy(cell, 0, host.hooks()));
        assert.deepEqual(fallen, [walker]);
        assert.equal(hash.bucket(0, 29, 29), 0);
        assert.ok(pool.slot(cell)[STATE] & FREE);
    }
    const freeBefore = pool.freeCount;
    // 이 실제 삭제가 다리 예약을 만든다. 받침은 해시에서 빠지고 종유석/표면/연결은 각각 남는다.
    notices = 0;
    assert.ok(virtualDestroy.destroy(island, scenario === 'Keep' ? ISLAND_DESTROY_KEEPS_BRIDGES : 0, host.hooks()));
    assert.ok(pool.slot(island)[STATE] & FREE);
    assert.equal(connect.findTypeAt(30, 30, ISLAND), 0);
    assert.equal(pool.slot(link)[STATE] & FREE, 0);
    assert.equal(linkNeeds(), false);
    const counts = book.globalCounts();
    assert.deepEqual([counts[ISLAND], counts[SURFACE], counts[STALAG]], [0, removeSurfaceBefore ? 8 : 9, 1]);
    if (scenario === 'Keep') {
        assert.deepEqual(order, []);
        assert.equal(kernel.size, 0);
        assert.equal(hasScheduledBridgeFall(host, source), false);
    } else {
        const scheduled = host.findEvent(source, BRIDGE_FALL_EVENT);
        assert.ok(scheduled);
        assert.equal(kernel.size, 1);
        assert.deepEqual(order, ['schedule']);
        assert.deepEqual([scheduled.payload, scheduled.count], [7196, 0]);
        assert.equal(get(pool.slot(source), FRAME_OFFSET, 4), scenario === 'Hard' ? 46 : 41);
        kernel.runFrame();
    }
    if (scenario === 'Hard' || scenario === 'Keep') {
        assert.equal(born, 0);
        assert.deepEqual(removedLinks, []);
        assert.deepEqual(effects, []);
        assert.equal(pool.slot(source)[STATE] & FREE, 0);
        assert.equal(get(pool.slot(source), FRAME_OFFSET, 4), scenario === 'Hard' ? 46 : 41);
        if (scenario === 'Hard') {
            assert.equal(kernel.size, 1);
            assert.equal((get(pool.slot(source), WORD, 2) & BRIDGE_LIFE_MASK) >> 3, 4);
        }
        if (scenario === 'Hard' && graphActive) {
            const before = Buffer.from(pool.bytes()).toString('hex');
            const beforeCost = book.totalCost;
            const beforeNotices = notices;
            // 실제 자기 삭제 이벤트도 같은 가상 재정의를 거쳐 부모/예약/연결을 보존한다.
            assert.equal(events.handle(source, BRIDGE_DESTROY_EVENT, 0, 0), BRIDGE_EVENT_KEEP);
            assert.equal(Buffer.from(pool.bytes()).toString('hex'), before);
            assert.deepEqual([book.totalCost, notices, kernel.size], [beforeCost, beforeNotices, 1]);
            checkGraph(11);
            // 편집기에서는 재정의의 거부 조건이 풀린다. 실제 삭제로 Regular 와 연결을 정리한다.
            mode.editor = true;
        }
        assert.ok(virtualDestroy.destroy(source, 0, host.hooks()));
    } else {
        assert.deepEqual(order, ['schedule', 'bridge', 'link', 'effect', 'effect']);
        if (scenario === 'Replace') {
            assert.notEqual(born, 0);
            assert.equal(hash.bucket(0, 31, 29), born);
            assert.deepEqual([get(pool.slot(born), FRAME_OFFSET, 4), pool.slot(born)[OWNER_OFFSET]], [56, 3]);
            assert.equal((get(pool.slot(born), WORD, 2) & BRIDGE_LIFE_MASK) >> 3, 4);
            assert.equal(get(pool.slot(born), 8, 2), 23);
            assert.ok(pool.slot(born)[EXTRA_OFFSET] & 0x10);
            const rawBorn = pool.slot(born);
            assert.deepEqual([get(rawBorn, X, 4), get(rawBorn, Y, 4)], [floatToBits(31), floatToBits(29)]);
            assert.equal(book.totalCost, terrainCost + 2 * types[BRIDGE].cost);
            assert.deepEqual([book.globalCounts()[BRIDGE], spots[29 * 256 + 31]], [2, 4]);
        } else {
            assert.equal(born, 0);
            assert.equal(hash.bucket(0, 31, 29), 0);
            assert.equal(book.totalCost, terrainCost);
            assert.deepEqual([book.globalCounts()[BRIDGE], spots[29 * 256 + 31]], [0, 0]);
        }
    }
    assert.deepEqual(removedLinks, [link]);
    assert.ok(pool.slot(link)[STATE] & FREE);
    assert.equal(hash.bucket(linkLevel, 30, 29), 0);
    assert.ok(pool.slot(source)[STATE] & FREE);
    assert.equal(hasScheduledBridgeFall(host, source), false);
    assert.equal(kernel.size, 0);
    assert.deepEqual(effects, ['notifyRemoval', 'fallSound']);
    assert.deepEqual([book.globalCounts()[CONNECTOR], book.depth, destroy.preDepth, destroy.postDepth], [0, 0, 0, 0]);
    assert.deepEqual(fallen, removeSurfaceBefore ? [walker] : []);
    // 받침/옛 다리/연결은 반납되고 교체 시 새 다리 하나만 남는다. ProcessForm 은 할당/반납이 상쇄된다.
    assert.equal(pool.freeCount, freeBefore + (scenario === 'Replace' ? 2 : 3));
    checkGraph((removeSurfaceBefore ? 8 : 9) + (scenario === 'Replace' ? 2 : scenario === 'Isolated' ? 0 : 1));
    assert.ok(notices > 0);
}


const ready = bridgeRows !== null;

// 섬 삭제 뒤 O 끝 칸을 생성하고 old bridge/ProcessForm/연결을 정리한다.
test('RawSurfaceLifecycle: 섬 삭제가 끝 칸 O 다리로 교체하고 옛 다리·연결 객체를 정리한다', { skip: !ready && '원본 fixture 없음' }, () => run('Replace'));
// 새 끝 방향에 살아 있는 이웃이 없으면 새 다리를 만들지 않는다.
test('RawSurfaceLifecycle: 이웃 없는 다리는 새 다리 없이 삭제된다', { skip: !ready && '원본 fixture 없음' }, () => run('Isolated'));
// hard 다리는 수명만 바꾸며 예약/연결을 유지한다. 후속 실제 삭제가 둘 다 정리한다.
test('RawSurfaceLifecycle: 단단한 다리는 실제 삭제 때까지 연결을 유지한다', { skip: !ready && '원본 fixture 없음' }, () => run('Hard'));
// flags 0x1000 은 받침만 삭제하고 예약을 막는다. 후속 실제 다리 삭제의 연결 정리는 유지된다.
test('RawSurfaceLifecycle: 삭제 flags 0x1000 은 낙하 예약을 생략한다', { skip: !ready && '원본 fixture 없음' }, () => run('Keep'));
// noIsland postDestroy 의 실제 분배가 같은 칸의 walker 에 도달하고 뒤따르는 섬/다리 삭제도 정상 종료한다.
test('RawSurfaceLifecycle: 표면 삭제가 walker 낙하를 분배한 뒤 섬/다리 삭제도 정상 종료한다', { skip: !ready && '원본 fixture 없음' }, () => run('Replace', { removeSurfaceBefore: true }));
// Graph 활성 생성·분할·끝 칸 재등록을 표시와 공통 장부까지 함께 검사한다.
test('RawSurfaceLifecycle: Graph 활성에서 교체·고립·hard 거부 뒤 편집기 정리·flags 보존이 장부까지 일치한다', { skip: !ready && '원본 fixture 없음' }, () => {
    for (const scenario of ['Replace', 'Isolated', 'Hard', 'Keep']) run(scenario, { graphActive: true });
});
// 같은 현재 연결 TYPE 을 빌린 표면 분배에서 실제 Graph/Host/예약/끝 칸/연결 정리가 모두 이어진다.
test('RawSurfaceLifecycle: Native 연결 TYPE 대여가 네 시나리오를 같은 결과로 재생한다', { skip: !ready && '원본 fixture 없음' }, () => {
    for (const scenario of ['Replace', 'Isolated', 'Hard', 'Keep']) run(scenario, { graphActive: true, nativeConnector: true });
});

// 미연결 효과를 구성 때 거부하고 공통 선택/form 계약과 실제 가상 표 분배를 유지한다.
test('RawSurfaceLifecycle: 훅 누락·범위·다른 풀을 구성 때 거부하고 다른 가상 표는 공통 훅만 쓴다', () => {
    const pool = new SidPool(32768, { server: true });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const types = realPlayerAssets().types.map((record) => ({ ...record }));
    const neighbors = new RawSquidNeighbors(pool, hash, spots, types, frameTable(types.length));
    let baseCalls = 0;
    let formCalls = 0;
    const common = { emit: () => { baseCalls += 1; }, selected: () => 9, unpopForm: () => { formCalls += 1; } };
    // 잘못된 분배가 곧바로 드러나는 외부 경계를 공급한다.
    const effects = {
        scheduleFall: () => assert.fail('예상 밖 예약'),
        destroy: () => assert.fail('예상 밖 연결 삭제'),
        fallWalker: () => assert.fail('예상 밖 walker 낙하'),
        bridgeEffect: () => assert.fail('예상 밖 다리 효과'),
    };
    assert.ok(throws(() => new RawSurfaceLifecycle(pool, neighbors, CONNECTOR, common, { ...effects, destroy: null }), TypeError));
    assert.ok(throws(() => new RawSurfaceLifecycle(pool, neighbors, CONNECTOR, { ...common, selected: null }, effects), TypeError));
    assert.ok(throws(() => new RawSurfaceLifecycle(pool, neighbors, 188, common, effects), RangeError));
    assert.ok(throws(() => new RawSurfaceLifecycle(pool, neighbors, 69, common, effects), RangeError));
    assert.ok(throws(() => new RawSurfaceLifecycle(new SidPool(32768, { server: true }), neighbors, CONNECTOR, common, effects), TypeError));
    const lifecycle = new RawSurfaceLifecycle(pool, neighbors, CONNECTOR, common, effects);
    assert.ok(throws(() => lifecycle.bindCurrentConnectorType(155), TypeError));
    const hooks = lifecycle.hooks();
    const sid = pool.allocate(2);
    const raw = pool.allocated(sid);
    put(raw, 0, VTABLE);
    raw[TYPE] = BRIDGE;
    // 타입 번호가 bridge 여도 실제 가상 표가 다른 입력은 공통 훅만 사용한다.
    hooks.emit({ effect: SQUID_DESTROY_EFFECT.preDestroy, sid, parent: 0, flags: 0 });
    hooks.emit({ effect: SQUID_DESTROY_EFFECT.postDestroy, sid, parent: 0, flags: 0 });
    hooks.emit({ effect: SQUID_DESTROY_EFFECT.releaseDependent, sid, parent: 0, flags: 0 });
    hooks.unpopForm(sid);
    assert.deepEqual([baseCalls, hooks.selected(), formCalls], [3, 9, 1]);
});

// 가상 표 분배의 인자 전달 계약(변형 시험에서 살아남은 입력): 다리 postDestroy 의 walker 낙하는 fallWalker 훅으로, 파편/소리는 bridgeEffect 로 가고,
// 섬 받침 preDestroy 와 noIsland postDestroy 는 받은 삭제 flags 를 그대로 공통 훅에 넘긴다. (neighbors 의 풀 일치는 RawIslandLifecycle 도 확인하는 중복 방어라 동등 변형이다.) (2026-11-10 WB04-E248)
test('RawSurfaceLifecycle: 다리 walker 낙하·파편/소리 분배와 섬/표면 삭제 flags 전달', () => {
    const pool = new SidPool(32768, { server: true });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const types = realPlayerAssets().types.map((record) => ({ ...record }));
    Object.assign(types[85], { flags2: 0x10000, footX: 1, footY: 1 });
    const neighbors = new RawSquidNeighbors(pool, hash, spots, types, frameTable(types.length));
    const common = [];
    const walkers = [];
    const bridgeEvents = [];
    const lifecycle = new RawSurfaceLifecycle(pool, neighbors, CONNECTOR, { emit: (event) => common.push(event), selected: () => 0 }, {
        scheduleFall: () => assert.fail('예상 밖 예약'),
        destroy: () => assert.fail('예상 밖 연결 삭제'),
        fallWalker: (sid) => walkers.push(sid),
        bridgeEffect: (event) => bridgeEvents.push(event.effect),
    });
    const hooks = lifecycle.hooks();
    // 가상 표 기록값·타입·extra 를 입력하고 필요하면 좌표를 둔다. extra 9(abstract/buried)는 이웃 탐색 없이 공통 훅으로 간다.
    const object = (vtable, type, extra, x = 0, y = 0) => {
        const sid = pool.allocate(2);
        const raw = pool.allocated(sid);
        raw.fill(0);
        put(raw, 0, vtable);
        raw[TYPE] = type;
        raw[EXTRA_OFFSET] = extra;
        put(raw, X, floatToBits(x));
        put(raw, Y, floatToBits(y));
        return sid;
    };
    // 섬 받침(가상 표 0x00506d98) preDestroy 와 noIsland(0x00506f08) postDestroy 는 받은 flags 를 그대로 공통 훅에 넘긴다.
    const island = object(0x00506d98, ISLAND, 9);
    const noIsland = object(0x00506f08, SURFACE, 9);
    hooks.emit({ effect: SQUID_DESTROY_EFFECT.preDestroy, sid: island, parent: 0, flags: 0x1234 });
    hooks.emit({ effect: SQUID_DESTROY_EFFECT.postDestroy, sid: noIsland, parent: 0, flags: 0x4321 });
    assert.deepEqual(common, [
        { effect: SQUID_DESTROY_EFFECT.preDestroy, sid: island, parent: 0, flags: 0x1234 },
        { effect: SQUID_DESTROY_EFFECT.postDestroy, sid: noIsland, parent: 0, flags: 0x4321 },
    ]);
    common.length = 0;
    // 다리(가상 표 0x005034c8) postDestroy: 파편 요청·소리는 bridgeEffect, 같은 칸 walker 는 fallWalker, 마지막에 공통 postDestroy 한 번.
    const bridge = object(0x005034c8, BRIDGE, 0, 20, 20);
    const walker = object(VTABLE, 85, 0, 20, 20);
    put(pool.allocated(walker), 4, hash.bucket(1, 20, 20), 2);
    hash.setBucket(1, 20, 20, walker);
    hooks.emit({ effect: SQUID_DESTROY_EFFECT.postDestroy, sid: bridge, parent: 0, flags: 0x77 });
    assert.deepEqual(bridgeEvents, ['notifyRemoval', 'fallSound']);
    assert.deepEqual(walkers, [walker]);
    assert.deepEqual(common, [{ effect: SQUID_DESTROY_EFFECT.postDestroy, sid: bridge, parent: 0, flags: 0x77 }]);
});
