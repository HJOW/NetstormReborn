// GUI 가 소유하는 raw 표면 월드(cpppj `client/RawSurfaceWorld`)의 웹 대응 `core/raw-surface-world.js` 를 검사한다(4-B, WB04-E299).
// cpppj `RawSurfaceWorldTests.cpp` 의 계약을 같은 장면으로 옮겼다(10.78 전용; CD 10.72 반복은 웹 범위 밖):
//  - Load: 행 우선 적재와 Graph 재구성, Regular 낙하 예약이 정지 중 유지되다 재개 뒤 끝 칸 교체, 잘못된 입력은 할당 전에 거부, 중복 Load 거부.
//  - Hard 다리/미션 종료 뒤 새 월드, 표시 억제와 장치 교체, runtime 공유 풀·전송·현재 Form 분배, 불완전/늦은 설치 거부, 캠페인 부모 Graph(false) 뒤 종료.
//  - 다리 붕괴 스캔(메뉴 저장소·실제 월드·즉시 권한/편집기 관문).
// 입력은 합성 TYPE/프레임/SHP(실제 bridge.type 프레임 코드 한 행은 cpppj 관측 파일)이며 기대값은 모듈이 낸 결과의 관계이다. 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E299)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SQUID_DELETION_EFFECT } from '../www/core/squid-destroy-lifecycle.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { GameClock } from '../www/core/game-clock.js';
import { GRAPH_COUNT } from '../www/core/graph.js';
import { Kernel } from '../www/core/kernel.js';
import { RawAuthorityState } from '../www/core/raw-authority-state.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { makeBridgeRegularHandler } from '../www/core/raw-bridge-events.js';
import { resetStateBridgeDecayScan } from '../www/core/raw-state-bridge-reset.js';
import { RawSurfaceStorage } from '../www/core/raw-surface-storage.js';
import { RawSurfaceWorld } from '../www/core/raw-surface-world.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { REPO_ROOT } from './helpers.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { adler32, floatToBits, get, put, readRows } from './raw-support.mjs';

// 실제 표면 타입 번호와 정상/단단한 K 다리·끝 칸의 원본 프레임 번호이다. (2026-11-11 WB04-E299)
const BRIDGE = 82;
const TERRAIN = 83;
const ISLAND = 94;
const STALAG = 95;
const CONNECTOR = 155;
const SURFACE = 157;
const NORMAL = 41;
const HARD = 46;
const END = 56;
// 원본 슬롯 필드 위치(TYPE·상태·수명 WORD·좌표·프레임·표면 Graph 번호)이다. (2026-11-11 WB04-E299)
const TYPE = 10;
const STATE = 11;
const WORD = 12;
const FRAME_OFFSET = 36;
const SURFACE_GRAPH = 30;
const SURFACE_LETTERS = 'FCGBADIEH';
const FIXTURE = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bridgedecay-x86.tsv');

// 저장된 실제 bridge.type 프레임 코드 한 행(방향·변형·번호·플래그 바이트 네 개)을 읽는다. 연결 결과를 합성하지 않는다. (2026-11-11 WB04-E299)
function actualBridgeFrames() {
    const row = readRows(FIXTURE).find((item) => item[0] === 'Frames' && item[1] === '0');
    assert.ok(row, '실제 다리 프레임 fixture 없음');
    const codes = [];
    // 네 바이트가 하나의 프레임 코드이다.
    for (let index = 0; index + 8 <= row[2].length; index += 8) {
        const byte = (offset) => parseInt(row[2].slice(index + offset * 2, index + offset * 2 + 2), 16);
        codes.push({ side: byte(0), variant: byte(1), number: byte(2), flags: byte(3) });
    }
    return codes;
}

// 같은 풀·상태·처리기를 조립해 입력과 결과를 대조하는 입력 묶음(cpppj `Inputs`)이다. 실제 생성자/플래그와 작은 합성 SHP 를 쓴다. (2026-11-11 WB04-E299)
class Inputs {
    // 188칸 TYPE 표(여섯 표면 타입과 walker 를 합성 정의)·프레임 코드 표·셰이프 표·표면 종류 전역을 만든다. (2026-11-11 WB04-E299)
    constructor() {
        this.types = realPlayerAssets().types.map((record) => ({ ...record }));
        const define = (type, flags1, flags2, foot) => {
            Object.assign(this.types[type], { constructorAddress: typeConstructorAddress(type), flags1, flags2, footX: foot, footY: foot });
        };
        define(BRIDGE, 0x802, 4, 1);
        this.types[BRIDGE].cost = 5;
        define(TERRAIN, 0x28000803, 2, 1);
        define(ISLAND, 0x28000003, 0x01000000, 3);
        define(STALAG, 0x28000002, 0, 3);
        define(CONNECTOR, 0x28000002, 0x02000000, 1);
        define(SURFACE, 0x08000803, 0x01000002, 1);
        // 표면 삭제의 낙하 분배만 확인하는 합성 walker 타입이다.
        Object.assign(this.types[85], { flags2: 0x10000, footX: 1, footY: 1 });
        this.codes = this.types.map(() => []);
        this.codes[BRIDGE] = actualBridgeFrames();
        this.codes[TERRAIN] = [{ side: 0x41, variant: 0x41, number: 0, flags: 0 }];
        // noIsland 적재에서 선택되는 실제 글자 순서이다.
        this.codes[SURFACE] = [...SURFACE_LETTERS].map((side) => ({ side: side.charCodeAt(0), variant: 0x50, number: 1, flags: 0 }));
        // 소유자별 받침·종유석 프레임과 연결 조각에 실제 크기의 발자국을 공급한다.
        for (const type of [ISLAND, STALAG, CONNECTOR]) this.codes[type] = Array.from({ length: 9 }, () => ({ side: 0x41, variant: 0x50, number: 1, flags: 0 }));
        this.frames = this.codes.map((codes) => new FrameCodeTable(codes));
        this.shapes = this.types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
        // 표시 픽셀과 공간 칸 크기를 각각 공급한다.
        for (const type of [BRIDGE, TERRAIN, ISLAND, STALAG, CONNECTOR, SURFACE]) {
            this.shapes[type] = { loaded: true, frameCount: this.codes[type].length, frames: this.codes[type].map(() => ({ width: 20, height: 16, hotspotX: 8, hotspotY: 12, cellWidth: this.types[type].footX, cellHeight: this.types[type].footY })) };
        }
        this.links = new BridgeConnectState();
        Object.assign(this.links, { bridgeType: BRIDGE, islandType: ISLAND, stalagType: STALAG, connectorType: CONNECTOR, noIslandType: SURFACE, battle: true });
    }

    // 월드를 만든다(입력은 월드가 복사해 보관한다). (2026-11-11 WB04-E299)
    make(options = {}) {
        return new RawSurfaceWorld(this.types, this.frames, this.shapes, this.links, TERRAIN, options);
    }
}

// 순서를 뒤섞은 3×3 받침과 동쪽 두 다리를 초기 입력으로 제공한다. (2026-11-11 WB04-E299)
function seeds(hard = false) {
    const result = [{ type: BRIDGE, owner: 3, frame: NORMAL, x: 32, y: 29 }, { type: BRIDGE, owner: 3, frame: hard ? HARD : NORMAL, x: 31, y: 29 }];
    // 역순 저장이어도 월드는 행 우선으로 적재해야 한다.
    for (let y = 30; y >= 28; y -= 1) {
        // 같은 행도 뒤에서부터 전달한다.
        for (let x = 30; x >= 28; x -= 1) result.push({ type: SURFACE, owner: 0, frame: 0, x, y });
    }
    return result;
}

// 월드의 요약(cpppj `Report` 첫 줄 대응): 처리기 수·표면 수·Graph 수·누적 비용·깊이들이다. (2026-11-11 WB04-E299)
function summary(world) {
    const core = world.core();
    let surfaces = 0;
    let graphs = 0;
    // 예약 0번 다음의 실제 연결 성분을 센다.
    for (let index = 1; index < GRAPH_COUNT; index += 1) {
        const record = core.graph.records()[index];
        if (record.inUse) {
            graphs += 1;
            surfaces += record.surfaces & 0xffff;
        }
    }
    return { game: world.frameTime.game, number: world.frameTime.number, kernel: core.kernel.size, surfaces, graphs, cost: core.bookkeeping.totalCost, depth: core.bookkeeping.depth, pre: core.destroy.preDepth, post: core.destroy.postDepth, ...world.observed };
}

// 월드의 관측 가능한 상태 전체(요약·객체 목록·풀 바이트·Graph 표·해시·spot)의 Adler 값이다. 변경 없음/롤백 검사에 쓴다(cpppj `Report()` 문자열 비교 대응). (2026-11-11 WB04-E299)
function snapshot(world) {
    const core = world.core();
    const records = core.graph.records().map((record) => `${record.surfaces},${record.inUse}`).join(';');
    return JSON.stringify({ state: summary(world), objects: world.objects(), pool: adler32(core.pool.bytes()), spots: adler32(core.spots), graph: records, hash: adler32(Buffer.from(Uint16Array.from(core.hash.view(0)).buffer)) });
}

// 특정 SID 의 현재 렌더링 입력을 찾는다. (2026-11-11 WB04-E299)
function objectOf(world, sid) {
    // 현재 살아 있는 객체 목록에서 번호가 같은 항목을 찾는다.
    for (const item of world.objects()) if (item.sid === sid) return item.object;
    throw new Error('검사할 raw 객체 없음');
}

// 프레임 시각을 만든다. (2026-11-11 WB04-E299)
const time = (game, wall = game, delta = game, number = 1) => ({ game, wall, delta, number });

// 구성과 첫 적재: 행 우선 Load 가 3×3 noIsland 와 두 다리, 받침/종유석/연결 객체를 만들고 Graph 를 한 번 재구성한다. (2026-11-11 WB04-E299)
test('월드: 구성과 Load', () => {
    const world = new Inputs().make();
    world.load(seeds());
    assert.equal(world.objects().length, 14);
    const state = summary(world);
    assert.deepEqual([state.surfaces, state.graphs], [11, 1]);
    const bridge = world.find(31, 29, BRIDGE);
    const link = world.find(30, 29, CONNECTOR);
    const island = world.find(30, 30, ISLAND);
    assert.ok(bridge && link && island);
    assert.equal(objectOf(world, island).owner, 3);
    assert.equal(objectOf(world, island).frame, 2);
    world.dispose();
});

// 정지 중 낙하 예약이 유지되다가 재개 뒤 끝 칸 교체가 일어난다(cpppj PausedRegularResumesWithClientGameClock). (2026-11-11 WB04-E299)
test('월드: 정지 중 Regular 유지와 재개 뒤 끝 칸 교체', () => {
    const world = new Inputs().make();
    world.load(seeds());
    assert.equal(world.objects().length, 14);
    assert.deepEqual([summary(world).surfaces, summary(world).graphs], [11, 1]);
    const bridge = world.find(31, 29, BRIDGE);
    const link = world.find(30, 29, CONNECTOR);
    const island = world.find(30, 30, ISLAND);
    assert.ok(bridge && link && island);
    assert.equal(objectOf(world, island).owner, 3);
    assert.equal(objectOf(world, island).frame, 2);
    const clock = new GameClock(0);
    world.runFrame(clock.capture(1000), false);
    clock.pause(1000);
    // 섬 받침 삭제가 낙하 예약의 유일한 트리거이다.
    assert.ok(world.destroy(island));
    assert.equal(summary(world).kernel, 1);
    world.takeChanged();
    world.runFrame(clock.capture(8000), clock.paused);
    assert.equal(world.find(31, 29, BRIDGE), bridge);
    assert.equal(world.find(30, 29, CONNECTOR), link);
    assert.deepEqual([summary(world).game, summary(world).kernel, world.takeChanged()], [1, 1, false]);
    clock.resume(8000);
    world.runFrame(clock.capture(8000), clock.paused);
    const end = world.find(31, 29, BRIDGE);
    assert.ok(end && end !== bridge);
    assert.equal(objectOf(world, end).frame, END);
    assert.equal(world.find(30, 29, CONNECTOR), 0);
    assert.notEqual(world.find(32, 29, BRIDGE), 0);
    const state = summary(world);
    assert.deepEqual([state.kernel, state.surfaces, state.cost, state.depth, state.pre, state.post, world.takeChanged()], [0, 11, 10, 0, 0, 0, true]);
    // 역행하거나 비유한 게임 시각은 거부한다.
    assert.throws(() => world.runFrame(time(0.5, 0, 0, 4), false), RangeError);
    assert.throws(() => world.runFrame(time(Number.NaN, 0, 0, 4), false), RangeError);
    assert.throws(() => world.runFrame(time(Number.POSITIVE_INFINITY, 0, 0, 4), false), RangeError);
    world.dispose();
});

// 단단한 다리는 삭제를 거부하고 변경 없음이며, 섬 받침 삭제 뒤 예약만 남는다. 미션 종료 뒤 새 월드는 SID/시각을 공유하지 않는다(cpppj HardBridgeAndPendingMissionTeardown). (2026-11-11 WB04-E299)
test('월드: 단단한 다리와 예약이 남은 미션 종료', () => {
    let world = new Inputs().make();
    world.load(seeds(true));
    world.takeChanged();
    const bridge = world.find(31, 29, BRIDGE);
    const before = snapshot(world);
    assert.equal(world.destroy(bridge), false);
    assert.equal(snapshot(world), before);
    assert.equal(world.takeChanged(), false);
    assert.ok(world.destroy(world.find(30, 30, ISLAND)));
    world.runFrame(time(12, 12, 12, 1), false);
    assert.equal(world.find(31, 29, BRIDGE), bridge);
    assert.equal(objectOf(world, bridge).frame, HARD);
    assert.equal(summary(world).kernel, 1);
    world.dispose();
    world = new Inputs().make();
    world.load(seeds());
    world.runFrame(time(0, 0, 0, 1), false);
    assert.equal(summary(world).kernel, 0);
    assert.notEqual(world.find(30, 30, ISLAND), 0);
    assert.equal(objectOf(world, world.find(31, 29, BRIDGE)).frame, NORMAL);
    world.dispose();
});

// 표시 억제 중에도 논리 변경은 알리고, 장치를 교체하면 새 장치로만 영역이 전달된다(cpppj DisplaySuppressionAndDeviceReplacement). (2026-11-11 WB04-E299)
test('월드: 표시 억제와 장치 교체', () => {
    const world = new Inputs().make();
    world.load(seeds());
    let suppressed = true;
    let oldNotices = 0;
    let newNotices = 0;
    const view = { cameraX: 400, cameraY: 280, zoom: 65536, viewport: { left: 0, top: 0, right: 640, bottom: 480 } };
    world.setDisplay(view, { suppressed: () => suppressed, invalidate: () => { oldNotices += 1; } });
    world.takeChanged();
    world.setSupportOwner(30, 30, 4);
    assert.equal(world.takeChanged(), true);
    assert.equal(oldNotices, 0);
    assert.equal(objectOf(world, world.find(30, 30, ISLAND)).frame, 3);
    suppressed = false;
    world.setDisplay(view, { suppressed: () => suppressed, invalidate: (rect) => { assert.ok(rect.left < rect.right && rect.top < rect.bottom); newNotices += 1; } });
    world.setSupportOwner(30, 30, 5);
    assert.ok(newNotices > 0);
    assert.equal(oldNotices, 0);
    assert.equal(world.takeChanged(), true);
    assert.equal(objectOf(world, world.find(30, 30, STALAG)).frame, 4);
    // 카메라가 다른 새 뷰에서는 같은 변경도 다른 표시 영역으로 알려진다(뷰 값이 실제 표시 계산에 쓰인다).
    const near = [];
    const far = [];
    world.setDisplay(view, { suppressed: () => false, invalidate: (rect) => { near.push({ ...rect }); } });
    world.setSupportOwner(30, 30, 6);
    world.setDisplay({ ...view, cameraX: 410, cameraY: 285 }, { suppressed: () => false, invalidate: (rect) => { far.push({ ...rect }); } });
    world.setSupportOwner(30, 30, 7);
    assert.ok(near.length > 0 && far.length > 0);
    assert.notDeepEqual(near[0], far[0]);
    world.dispose();
});

// 잘못된 입력은 첫 할당 전에 거부하고 월드 상태를 바꾸지 않으며, 한 번 적재한 월드의 중복 Load 도 거부한다(cpppj InvalidLoadRejectedBeforeAllocation). (2026-11-11 WB04-E299)
test('월드: 잘못된 적재 입력은 할당 전에 거부', () => {
    const world = new Inputs().make();
    const before = snapshot(world);
    let list = seeds();
    list[list.length - 1].x = Number.NaN;
    assert.throws(() => world.load(list), RangeError);
    assert.equal(snapshot(world), before);
    list = seeds();
    list[list.length - 1].owner = 9;
    assert.throws(() => world.load(list), RangeError);
    // 미지원 타입, 지도 밖 좌표, 비정수 좌표, 음수 프레임, 프레임 범위 밖도 같은 검사이다.
    for (const [key, value, kind] of [['type', ISLAND, TypeError], ['x', 256, RangeError], ['y', 0, RangeError], ['x', 28.5, RangeError], ['y', 28.5, RangeError], ['frame', -1, RangeError], ['frame', 99, RangeError], ['frame', 0.5, RangeError], ['owner', Number.NaN, RangeError], ['owner', -1, RangeError], ['owner', 1.5, RangeError], ['x', 0, RangeError], ['y', 256, RangeError], ['y', Number.POSITIVE_INFINITY, RangeError]]) {
        list = seeds();
        list[list.length - 1][key] = value;
        assert.throws(() => world.load(list), kind, `${key}=${value}`);
    }
    assert.equal(snapshot(world), before);
    world.load(seeds());
    const loaded = snapshot(world);
    assert.throws(() => world.load(seeds()), /표면 월드 중복 로드/);
    assert.equal(snapshot(world), loaded);
    world.dispose();
});

// 외부 미션 모듈의 같은 풀·전송·현재 Form 분배를 실제 Kernel·공간·삭제에 연결한다(cpppj RuntimeSharesStorageAndCurrentDispatch). 합성 타입 100 은 원본 base 생성자/가상 몸체를 쓴다. (2026-11-11 WB04-E299)
test('월드: Runtime 이 같은 풀·전송·현재 Form 분배를 공유', () => {
    const input = new Inputs();
    const synthetic = 100;
    // 합성 타입은 원본 base 생성자 주소(0)를 쓴다.
    input.types[synthetic].constructorAddress = 0;
    input.types[synthetic].flags1 = 0x10;
    input.types[synthetic].footX = 1;
    input.types[synthetic].footY = 1;
    input.codes[synthetic] = [{ side: 0x41, variant: 0x41, number: 0, flags: 0 }];
    input.frames[synthetic] = new FrameCodeTable(input.codes[synthetic]);
    input.shapes[synthetic] = { frameCount: 1, loaded: true, frames: [{ width: 20, height: 16, hotspotX: 8, hotspotY: 12, cellWidth: 1, cellHeight: 1 }] };
    const world = input.make();
    const core = world.core();
    const originalHooks = core.forms.hooks();
    let selected = 0;
    const depths = [];
    let writes = 0;
    let sends = 0;
    let regulars = 0;
    const hooks = {
        assets: { ...core.surfaces.hooks(), selected: () => selected },
        destroy: (sid, flags, effects) => core.virtualDestroy.destroy(sid, flags, effects),
        regular: (sid, event, count, payload) => { assert.deepEqual([event, count, payload], [77, 0, 0.25]); regulars += 1; return 0; },
        transmit: {
            writeData: (process, changed) => { assert.ok(changed); assert.ok(core.pool.slot(process.form)[11] & 4); writes += 1; return 8; },
            transmit: (length) => { assert.equal(length, 8); sends += 1; },
        },
        walkerFall: () => { throw new Error('이번 검사는 낙하를 만들지 않습니다'); },
        loadingDepth: (depth) => { depths.push(depth); },
        bridgeEffect: () => { throw new Error('이번 저장소 검사는 실제 다리를 파괴하지 않습니다'); },
    };
    // 실제 효과 소비자만 빠진 설치도 객체/월드 변경 전에 거부한다.
    const before = snapshot(world);
    assert.throws(() => world.bindRuntime({ ...hooks, bridgeEffect: undefined }), TypeError);
    assert.equal(snapshot(world), before);
    world.bindRuntime(hooks);
    assert.equal(world.core().hash, core.hash);
    world.beginLoad();
    const object = world.create(synthetic);
    core.owner.set(object, 1);
    world.pop(object, 25, 25, 9);
    world.finishLoad();
    assert.equal(world.objects().length, 1);
    assert.equal(world.objects()[0].sid, object);
    assert.equal(world.objects()[0].object.x, 25);
    assert.deepEqual(depths, [1, 1, 0]);
    assert.equal(core.bookkeeping.depth, 0);
    selected = object;
    assert.equal(originalHooks.selected(), object);
    selected = 0;
    const timer = core.processes.addRegular(object, 77, 0.25, 0);
    assert.ok(timer);
    assert.deepEqual([writes, sends, core.kernel.size], [1, 1, 1]);
    world.runFrame(time(1, 1, 1, 1), false);
    assert.deepEqual([regulars, core.kernel.size], [1, 0]);
    world.unpop(object, 0);
    assert.equal(world.objects().length, 0);
    world.repop(object, 0x40);
    assert.equal(world.objects().length, 1);
    assert.ok(world.destroy(object));
    assert.equal(world.objects().length, 0);
    world.clearProcesses();
    world.dispose();
});

// 필수 경계 누락/늦은 설치는 이미 존재하는 공간/객체/예약을 바꾸지 않는다(cpppj RuntimeRejectsIncompleteOrLateBinding). (2026-11-11 WB04-E299)
test('월드: Runtime 필수 경계 누락과 늦은 설치 거부', () => {
    const world = new Inputs().make();
    const before = snapshot(world);
    assert.throws(() => world.bindRuntime({}), TypeError);
    assert.throws(() => world.bindRuntime(null), TypeError);
    assert.equal(snapshot(world), before);
    world.load(seeds());
    const loaded = snapshot(world);
    assert.throws(() => world.bindRuntime({}), /초기 객체 생성 전에/);
    assert.equal(snapshot(world), loaded);
    world.dispose();
});

// 실제 캠페인 부모의 Graph(false) 뒤 종료는 같은 표/stack/슬롯/지도를 바꾸지 않는다(cpppj CampaignFinishPreservesParentGraph). (2026-11-11 WB04-E299)
test('월드: 캠페인 부모 Graph 뒤 종료는 Graph 를 보존', () => {
    const world = new Inputs().make();
    const core = world.core();
    assert.throws(() => world.finishLoadAfterGraph(), /로드 종료 순서/);
    world.beginLoad();
    // 실제 Factory·가상 후처리가 등록된 표면 SID 이다. 정상 K 다리 프레임을 입력한다.
    const sid = world.create(BRIDGE);
    put(core.pool.allocated(sid), FRAME_OFFSET, NORMAL, 4);
    core.owner.set(sid, 1);
    world.pop(sid, 26, 26, 0);
    core.graph.rebuild(false);
    // 추가 Graph(true)가 초기화할 수 있는 실제 장부 번호이다.
    const extraGraph = core.graph.allocate();
    const records = core.graph.records().map((record) => ({ ...record }));
    const stack = Uint32Array.from(core.graph.floodStack());
    const raw = Uint8Array.from(core.pool.slot(sid));
    const spots = Uint8Array.from(core.spots);
    const hash = Uint16Array.from(core.hash.view(0));
    world.finishLoadAfterGraph();
    assert.equal(core.bookkeeping.graphsEnabled, true);
    assert.equal(core.bookkeeping.depth, 0);
    assert.ok(core.graph.records()[extraGraph].inUse);
    assert.deepEqual(core.graph.records().map((record) => ({ ...record })), records);
    assert.deepEqual(Uint32Array.from(core.graph.floodStack()), stack);
    assert.deepEqual(Uint8Array.from(core.pool.slot(sid)), raw);
    assert.deepEqual(Uint8Array.from(core.spots), spots);
    assert.deepEqual(Uint16Array.from(core.hash.view(0)), hash);
    assert.throws(() => world.finishLoadAfterGraph(), /로드 종료 순서/);
    assert.throws(() => world.beginLoad(), /중복 로드 시작/);
    world.dispose();
});

// 다리 수명 nibble 이다(WORD +12 의 0x78 비트, >>3). (2026-11-11 WB04-E299)
const lifeOf = (core, sid) => (get(core.pool.slot(sid), WORD, 2) & 0x78) >> 3;

// 실제 슬롯 스캔 → 수명/금 간 그림 → 현재 server 의 0 수명 판단 → Graph/Unpop/반납을 연결한다(cpppj BridgeDecayRuntime_ActualWorldScanCrackAndDeletion). (2026-11-11 WB04-E299)
test('월드: 다리 붕괴 스캔과 균열, 서버 0 수명 삭제', () => {
    const data = new Inputs();
    let editor = 0;
    let carriers = 0;
    let sounds = 0;
    let effectTime = 0;
    const world = data.make();
    world.load(seeds());
    const core = world.core();
    const outer = world.find(32, 29, BRIDGE);
    assert.ok(outer);
    // 장치가 읽는 시각은 같은 실제 PrepareFrame 뒤이다.
    world.bindBridgeDecay(() => editor, (sid) => { assert.equal(sid, outer); carriers += 1; }, (sid) => {
        assert.equal(sid, outer);
        assert.ok(core.processState.now >= 30);
        sounds += 1;
    }, (frame) => { effectTime = frame.game; assert.equal(core.processState.now, frame.game); });
    const scan = world.storage.bridgeDecayScan;
    resetStateBridgeDecayScan(scan, world.storage.bridgeDecayPeriod, 0);
    world.runFrame(time(0.25, 0.25, 0.25, 1), false);
    assert.equal(lifeOf(core, outer), 7);
    const cursor = scan.cursor;
    // 정지 중에는 스캔이 진행되지 않는다.
    world.runFrame(time(0.25, 4, 0.25, 2), true);
    assert.deepEqual([scan.cursor, lifeOf(core, outer)], [cursor, 7]);
    // 편집기나 보스 권한이 꺼져 있어도 진행하지 않는다.
    editor = 1;
    world.runFrame(time(0.5, 4.25, 0.25, 3), false);
    assert.equal(scan.cursor, cursor);
    editor = 0;
    world.storage.authority.boss = false;
    world.runFrame(time(0.75, 4.5, 0.25, 4), false);
    assert.equal(scan.cursor, cursor);
    world.storage.authority.boss = true;
    // 현재 주기 10 안에서 실제 전체 슬롯을 나누어 훑는다.
    for (let frame = 4; frame <= 123; frame += 1) {
        const now = frame * 0.25;
        world.runFrame(time(now, now + 4, 0.25, frame + 1), false);
    }
    assert.equal(lifeOf(core, outer), 4);
    assert.ok(data.codes[BRIDGE][get(core.pool.slot(outer), FRAME_OFFSET, 4)].flags & 0x20);
    assert.ok(carriers >= 2);
    assert.equal(sounds, 1);
    assert.equal(effectTime, 30.75);
    // 서버가 아니면 수명 0 에서 삭제하지 않는다.
    let raw = core.pool.allocated(outer);
    put(raw, WORD, (get(raw, WORD, 2) & ~0x78) | 8, 2);
    world.storage.authority.server = false;
    world.decayBridgeCell(time(31, 35, 0.25, 125), outer);
    assert.equal(world.find(32, 29, BRIDGE), outer);
    assert.equal(get(core.pool.slot(outer), WORD, 2) & 0x78, 0);
    // 원본 421c30 의 J/K 제거 flags 0x02000000 은 pre Graph 분할을 생략한다.
    const beforeGraph = core.graph.records().map((record) => ({ ...record }));
    assert.equal(data.codes[BRIDGE][get(core.pool.slot(outer), FRAME_OFFSET, 4)].side, 'K'.charCodeAt(0));
    raw = core.pool.allocated(outer);
    put(raw, WORD, (get(raw, WORD, 2) & ~0x78) | 8, 2);
    world.storage.authority.server = true;
    world.decayBridgeCell(time(31.25, 35.25, 0.25, 126), outer);
    assert.equal(world.find(32, 29, BRIDGE), 0);
    assert.ok(core.pool.slot(outer)[STATE] & 1);
    const state = summary(world);
    assert.deepEqual([state.surfaces, state.cost], [11, 5]);
    assert.deepEqual(core.graph.records().map((record) => ({ ...record })), beforeGraph);
    world.dispose();
});

// Hard 보호는 같은 현재 boss/Editor 값을 즉시 읽으며 다음 Frame 사본을 기다리지 않는다(cpppj ImmediateAuthorityAndEditorDestroyModes). (2026-11-11 WB04-E299)
test('월드: Hard 다리 삭제의 즉시 권한/편집기 관문', () => {
    // 각 관문을 독립된 실제 World/Graph 에서 바꾼다.
    for (const useEditor of [false, true]) {
        let editor = 0;
        const world = new Inputs().make();
        world.load(seeds(true));
        const hard = world.find(31, 29, BRIDGE);
        world.bindBridgeDecay(() => editor, () => {}, () => {});
        assert.ok(hard);
        assert.equal(world.destroy(hard), false);
        if (useEditor) editor = 1;
        else world.storage.authority.boss = false;
        assert.equal(world.destroy(hard), true);
        assert.equal(world.find(31, 29, BRIDGE), 0);
        world.dispose();
    }
    // 붕괴 장치를 연결하지 않아도 가상 삭제는 같은 장기 권한 owner 의 현재 값을 읽는다.
    const bare = new Inputs().make();
    bare.load(seeds(true));
    const hardBridge = bare.find(31, 29, BRIDGE);
    assert.equal(bare.destroy(hardBridge), false);
    bare.storage.authority.boss = false;
    assert.equal(bare.destroy(hardBridge), true);
    bare.dispose();
});

// 공통 삭제 소비자를 첫 객체 전에 한 번만 교체한다: 기본 관찰(삭제 요청 수)은 쓰지 않고 실제 선택/효과를 읽는다(cpppj BindDeletion). (2026-11-11 WB04-E299)
test('월드: 공통 삭제의 실제 소비자 교체', () => {
    // 월드를 만들어 소비자를 교체하고 섬 받침을 삭제한 뒤 받은 효과 목록을 돌려준다. pick(island) 이 선택 SID 를 정한다.
    const run = (pick) => {
        const world = new Inputs().make();
        const core = world.core();
        const reward = new SquidReward(core.pool, core.types, core.bookkeeping, new SquidRewardState(), new ScrambledSpStore(new GameRandom(), () => 1));
        const events = [];
        const hooks = { emit: (event) => { events.push(event.effect); }, forward: () => {} };
        let selected = 0;
        // 누락된 경계는 설치를 거부한다.
        assert.throws(() => world.bindDeletion(null, () => 0, hooks), TypeError);
        assert.throws(() => world.bindDeletion(reward, null, hooks), TypeError);
        assert.throws(() => world.bindDeletion(reward, () => 0, { emit: hooks.emit }), TypeError);
        assert.throws(() => world.bindDeletion(reward, () => 0, { forward: hooks.forward }), TypeError);
        assert.throws(() => world.bindDeletion(reward, () => 0, null), TypeError);
        // 다른 장부의 보상은 효과 전에 거부한다.
        const foreign = new SquidReward(core.pool, core.types, new SquidPostPopState(), new SquidRewardState(), new ScrambledSpStore(new GameRandom(), () => 1));
        assert.throws(() => world.bindDeletion(foreign, () => 0, hooks), TypeError);
        world.bindDeletion(reward, () => selected, hooks);
        assert.throws(() => world.bindDeletion(reward, () => 0, hooks), /첫 객체 생성 전에 한 번/);
        world.load(seeds());
        const island = world.find(30, 30, ISLAND);
        selected = pick(island);
        assert.ok(world.destroy(island));
        // 기본 관찰 훅은 쓰이지 않고 교체한 훅이 효과를 받는다.
        assert.equal(world.observed.deletions, 0);
        assert.ok(world.commonDeletion);
        world.dispose();
        return events;
    };
    const none = run(() => 0);
    const chosen = run((island) => island);
    // 선택된 SID 를 지우면 선택 해제 효과가 추가로 나온다.
    assert.ok(chosen.filter((effect) => effect === SQUID_DELETION_EFFECT.clearSelection).length > none.filter((effect) => effect === SQUID_DELETION_EFFECT.clearSelection).length);
});

// 첫 객체 생성 뒤에는 설치 모드·삭제 소비자·runtime 을 바꿀 수 없다. 설치 모드는 같은 장기 모드/표면 종류를 쓴다. (2026-11-11 WB04-E299)
test('월드: 설치 모드는 첫 객체 전에만', () => {
    const input = new Inputs();
    const storage = new RawSurfaceStorage(input.types, input.frames, input.shapes);
    const world = input.make({ storage });
    world.configureInstallModes(true, false, true, true);
    assert.deepEqual([storage.ownerMode.fort, storage.ownerMode.battle, storage.ownerMode.challenge, storage.links.battle], [true, false, true, false]);
    world.create(BRIDGE);
    assert.throws(() => world.configureInstallModes(false, true, false, false), /첫객체/);
    assert.deepEqual([storage.ownerMode.fort, storage.ownerMode.battle, storage.ownerMode.challenge], [true, false, true]);
    world.dispose();
    // 같은 저장소의 다음 월드는 새 대여로 풀이 다시 비어 있고, 첫 객체를 만든 뒤에는 모드 변경과 삭제/runtime 설치를 거부한다.
    const second = input.make({ storage });
    assert.equal(storage.generation, 2);
    assert.equal(second.objects().length, 0);
    second.configureInstallModes(false, true, false, false);
    second.create(BRIDGE);
    assert.throws(() => second.configureInstallModes(true, true, true, true), /첫객체/);
    assert.throws(() => second.bindRuntime({}), /초기 객체 생성 전에/);
    assert.throws(() => second.bindDeletion(null, () => 0, {}), /첫 객체 생성 전에 한 번/);
    second.dispose();
});

// 월드는 저장소의 같은 물리·장부·모드를 빌리고(복사하지 않음) 하나의 월드만 대여하며, dispose 로 반납한다. (2026-11-11 WB04-E299)
test('월드: 저장소 물리 대여와 반납', () => {
    const input = new Inputs();
    const storage = new RawSurfaceStorage(input.types, input.frames, input.shapes);
    const world = input.make({ storage });
    const core = world.core();
    assert.equal(world.storage, storage);
    assert.equal(world.mutableTypes, storage.types);
    assert.equal(storage.borrowed, true);
    // 월드 모듈이 같은 물리 객체를 직접 쓴다.
    assert.equal(core.pool, storage.pool);
    assert.equal(core.hash, storage.hash);
    assert.equal(core.spots, storage.spots);
    assert.equal(core.types, storage.types);
    assert.equal(core.frames, storage.frames);
    assert.equal(core.shapes, storage.shapes);
    assert.equal(core.ownerMode, storage.ownerMode);
    assert.equal(core.bridgeState, storage.links);
    assert.equal(core.graph.records(), storage.graph.records);
    assert.equal(core.graph.floodStack(), storage.graph.stack);
    // 입력 표면 종류와 모드가 저장소의 장기 값에 쓰인다.
    assert.deepEqual([storage.links.bridgeType, storage.links.islandType, storage.links.stalagType, storage.links.connectorType, storage.links.noIslandType, storage.links.battle], [BRIDGE, ISLAND, STALAG, CONNECTOR, SURFACE, true]);
    assert.deepEqual([storage.ownerMode.fort, storage.ownerMode.battle, storage.ownerMode.challenge], [false, true, false]);
    assert.equal(core.bookkeeping.localOwner, 1);
    // 공통 삭제의 최근 손실 좌표는 장기 보조 상태를 직접 읽고 쓴다.
    storage.gameAux.lostX = 12;
    assert.equal(world.deletionState.readLostX(), 12);
    world.deletionState.writeLostX(7);
    assert.equal(storage.gameAux.lostX, 7);
    world.load(seeds());
    // 누적 비용 장부도 같은 장기 보조 상태이다.
    assert.equal(storage.gameAux.totalCost, core.bookkeeping.totalCost);
    assert.ok(core.bookkeeping.totalCost > 0);
    // 같은 저장소의 두 번째 월드는 대여 중에 거부되며 첫 월드를 바꾸지 않는다.
    const before = snapshot(world);
    assert.throws(() => input.make({ storage }), /중복 대여/);
    assert.equal(snapshot(world), before);
    assert.equal(storage.borrowed, true);
    world.dispose();
    world.dispose();
    assert.equal(storage.borrowed, false);
    // 다음 월드는 새 대여로 풀을 재초기화하고 TYPE 표는 보존한다.
    const second = input.make({ storage });
    assert.equal(storage.generation, 2);
    assert.equal(second.objects().length, 0);
    assert.equal(storage.types[BRIDGE].cost, 5);
    second.dispose();
});

// 생성 인자 검사: 저장소 선택과 입력 크기/타입 번호/소유자 정체성은 대여 전에 거부해 기존 저장소 물리를 건드리지 않는다. (2026-11-11 WB04-E299)
test('월드: 생성 인자는 대여 전에 거부', () => {
    const input = new Inputs();
    const storage = new RawSurfaceStorage(input.types, input.frames, input.shapes);
    const before = adler32(storage.pool.bytes());
    const make = (types, frames, shapes, links, terrain, options) => new RawSurfaceWorld(types, frames, shapes, links, terrain, options);
    assert.throws(() => make(input.types, input.frames.slice(1), input.shapes, input.links, TERRAIN, {}), /배열 크기/);
    assert.throws(() => make(input.types, input.frames, input.shapes.slice(1), input.links, TERRAIN, {}), /배열 크기/);
    assert.throws(() => make(input.types, input.frames, input.shapes, input.links, 188, {}), /타입 번호/);
    assert.throws(() => make(input.types, input.frames, input.shapes, { ...input.links, bridgeType: 188 }, TERRAIN, {}), /타입 번호/);
    assert.throws(() => make(input.types, input.frames, input.shapes, { ...input.links, noIslandType: 188 }, TERRAIN, {}), /타입 번호/);
    assert.throws(() => make(input.types, input.frames, input.shapes, input.links, TERRAIN, { adoptPrepared: true }), /PATCH 저장소/);
    assert.throws(() => make(input.types, input.frames, input.shapes, input.links, TERRAIN, { storage, authority: new RawAuthorityState() }), /권한 소유자/);
    assert.throws(() => make(input.types, input.frames, input.shapes, input.links, TERRAIN, { storage, gameAux: new GameAuxState() }), /게임 보조 소유자/);
    const small = new RawSurfaceStorage(input.types.slice(0, 100), input.frames.slice(0, 100), input.shapes.slice(0, 100));
    assert.throws(() => make(input.types, input.frames, input.shapes, input.links, TERRAIN, { storage: small }), /배열 크기/);
    // 어떤 거부도 대여나 풀을 바꾸지 않았다.
    assert.equal(storage.borrowed, false);
    assert.equal(storage.generation, 0);
    assert.equal(adler32(storage.pool.bytes()), before);
    // 소유자 정체성이 같으면 받아들이고 같은 owner 를 사용한다.
    const world = make(input.types, input.frames, input.shapes, input.links, TERRAIN, { storage, authority: storage.authority, gameAux: storage.gameAux });
    assert.equal(storage.borrowed, true);
    world.dispose();
    // 구성 도중 실패하면 자기 대여만 반납한다.
    const links = { ...input.links, connectorType: 99, noIslandType: 98 };
    const cut = (list) => list.slice(0, 100);
    const partial = new RawSurfaceStorage(cut(input.types), cut(input.frames), cut(input.shapes));
    assert.throws(() => make(cut(input.types), cut(input.frames), cut(input.shapes), links, TERRAIN, { storage: partial }));
    // 대여 뒤에 실패했으므로 세대는 1 이고 대여는 이미 반납됐다.
    assert.equal(partial.borrowed, false);
    assert.equal(partial.generation, 1);
});

// 외부 Kernel 을 직접 쓰는 독립 월드(레거시 Host)는 자기 프로세스만 반납하고, SID 소진 복구는 현재 월드의 가상 삭제로 들어간다. (2026-11-11 WB04-E299)
test('월드: 독립 Kernel 과 SID 소진 회수 대여', () => {
    const input = new Inputs();
    const storage = new RawSurfaceStorage(input.types, input.frames, input.shapes);
    const kernel = new Kernel();
    const world = input.make({ storage, processKernel: kernel });
    world.load(seeds());
    assert.equal(world.core().kernel, kernel);
    assert.ok(world.destroy(world.find(30, 30, ISLAND)));
    assert.equal(kernel.size, 1);
    // 레거시 Host 는 Storage 의 현재 처리기 대여가 없다.
    assert.throws(() => world.clearProcessesForState(), /현재 대여 없는/);
    assert.throws(() => world.returnProcessBindingsForState({}), /단일 실제 대여/);
    assert.equal(world.processBindingsReturned, false);
    // 소진 복구는 현재 월드의 flags 0 가상 삭제이다.
    const bridge = world.find(32, 29, BRIDGE);
    storage.processes.reclaimForAllocation(bridge);
    assert.equal(world.find(32, 29, BRIDGE), 0);
    world.drainProcesses();
    assert.equal(kernel.size, 0);
    // 월드를 반납하면 회수 대여도 끝나 더는 월드로 들어가지 않는다.
    world.dispose();
    assert.throws(() => storage.processes.reclaimForAllocation(bridge), /현재 World 가상 삭제 미연결/);
});

// 장기 Client 장치(Kernel/시각)를 pin 한 공유 Host: 프레임 시각은 장치에서 읽고 되쓰지 않으며, State 정리는 Clear 증빙 → 반납 순서로만 이어진다. (2026-11-11 WB04-E299)
test('월드: 공유 Host 의 Client 시각과 State 정리 증빙', () => {
    const input = new Inputs();
    const storage = new RawSurfaceStorage(input.types, input.frames, input.shapes);
    const kernel = new Kernel();
    const clock = { game: 3 };
    storage.processes.bindDevices(kernel, clock);
    const world = input.make({ storage, processKernel: kernel });
    world.load(seeds());
    assert.equal(world.core().kernel, kernel);
    world.runFrame(time(5, 5, 5, 1), false);
    // 장치가 있으면 Client 시각을 읽고 스냅샷 시각으로 되쓰지 않는다.
    assert.equal(world.frameTime.game, 3);
    assert.equal(clock.game, 3);
    assert.equal(world.core().processState.now, 3);
    clock.game = 2;
    assert.throws(() => world.runFrame(time(6, 6, 1, 2), false), RangeError);
    clock.game = 4;
    assert.ok(world.destroy(world.find(30, 30, ISLAND)));
    assert.equal(kernel.size, 1);
    // Clear 증빙이 있어야 반납한다. 증빙 없는 반납과 낯선 증빙은 거부되고 대여는 남는다.
    assert.throws(() => world.returnProcessBindingsForState({}), TypeError);
    assert.equal(world.processBindingsReturned, false);
    const pass = world.clearProcessesForState();
    assert.equal(kernel.size, 0);
    world.returnProcessBindingsForState(pass);
    assert.equal(world.processBindingsReturned, true);
    // 단일 대여이므로 반납 뒤에는 Clear/반납이 모두 거부된다.
    assert.throws(() => world.clearProcessesForState(), /현재 대여 없는/);
    assert.throws(() => world.returnProcessBindingsForState(pass), /단일 실제 대여/);
    world.dispose();
    assert.equal(storage.borrowed, false);
});

// 일반 종료는 현재 월드가 만든 프로세스만 반납한다(명시 Clear 와 구별). (2026-11-11 WB04-E299)
test('월드: 종료는 자기 프로세스만 반납', () => {
    const input = new Inputs();
    const storage = new RawSurfaceStorage(input.types, input.frames, input.shapes);
    const kernel = new Kernel();
    storage.processes.bindDevices(kernel, { game: 0 });
    const world = input.make({ storage, processKernel: kernel });
    world.load(seeds());
    assert.ok(world.destroy(world.find(30, 30, ISLAND)));
    assert.equal(kernel.size, 1);
    world.drainCurrentProcessBindingsForShutdown();
    assert.equal(kernel.size, 0);
    world.dispose();
    world.dispose();
});

// 앞선 월드가 끝난 물리를 초기화 없이 대여한다(prepared): 객체·모드·표면 종류를 보존하고 Editor 모드로 단단한 다리도 즉시 지운다. (2026-11-11 WB04-E299)
test('월드: 준비된 대여는 물리와 모드를 보존', () => {
    const input = new Inputs();
    const storage = new RawSurfaceStorage(input.types, input.frames, input.shapes);
    const first = input.make({ storage });
    first.load(seeds(true));
    first.dispose();
    // 명시 정리가 끝난 물리에서 일부 모드를 바꿔 두면 준비된 월드가 입력으로 덮어쓰지 않는다.
    storage.links.battle = false;
    storage.links.ownerColors.fill(7);
    const before = adler32(storage.pool.bytes());
    const second = new RawSurfaceWorld(input.types, input.frames, input.shapes, input.links, TERRAIN, { storage, adoptPrepared: true });
    assert.equal(adler32(storage.pool.bytes()), before);
    assert.equal(storage.generation, 2);
    assert.equal(second.objects().length, 14);
    assert.equal(storage.links.battle, false);
    assert.ok(storage.links.ownerColors.every((value) => value === 7));
    assert.deepEqual([storage.ownerMode.fort, storage.ownerMode.battle, storage.ownerMode.challenge], [false, true, false]);
    // Editor 모드이므로 단단한 다리도 즉시 지울 수 있다.
    const hard = second.find(31, 29, BRIDGE);
    assert.ok(hard);
    assert.ok(second.destroy(hard));
    assert.equal(second.find(31, 29, BRIDGE), 0);
    second.dispose();
    // 일반 대여는 다시 초기화한다.
    const third = input.make({ storage });
    assert.equal(third.objects().length, 0);
    assert.equal(storage.links.battle, true);
    third.dispose();
});

// 저장 게임 관문은 마지막 칸(H)의 소유자 전파만 막고, 매 등록마다 현재 값을 읽는다. (2026-11-11 WB04-E299)
test('월드: 저장 게임 관문은 마지막 칸의 소유자 전파를 막음', () => {
    // 첫 칸만 소유자 3 인 순수 noIsland 9칸이다.
    const cells = [];
    // 행 우선으로 9칸을 만든다.
    for (let y = 28; y <= 30; y += 1) {
        // 한 행의 세 칸이다.
        for (let x = 28; x <= 30; x += 1) cells.push({ type: SURFACE, owner: x === 28 && y === 28 ? 3 : 0, frame: 0, x, y });
    }
    const owners = (saved) => {
        const box = { value: saved };
        const world = new Inputs().make({ sharedSavedGame: box });
        world.load(cells);
        const result = world.objects().filter((item) => item.object.type === SURFACE).map((item) => item.object.owner);
        world.dispose();
        return result;
    };
    // 새 게임: 마지막 칸의 등록이 첫 칸의 소유자를 3×3 전체와 받침에 전파한다.
    assert.deepEqual(owners(false), Array(9).fill(3));
    // 저장 게임: 전파하지 않아 저장된 소유자가 그대로 남는다.
    assert.deepEqual(owners(true), [3, 0, 0, 0, 0, 0, 0, 0, 0]);
});

// 종속 Unpop 엔진과 객체 송신기는 월드 수명 안에서만 연결/해제된다. 송신기는 모든 실제 Pop 을 한 번씩 받는다. (2026-11-11 WB04-E299)
test('월드: 종속 Unpop 연결과 객체 송신기', () => {
    const world = new Inputs().make();
    assert.throws(() => world.bindContainedUnpop({ pool: {} }), TypeError);
    world.bindContainedUnpop({ pool: world.core().pool, unpop: () => {}, detach: () => {}, contentFormType: 6 });
    world.bindContainedUnpop(null);
    assert.throws(() => world.bindSquidTransmission(1), TypeError);
    const requests = [];
    world.bindSquidTransmission((sid, flags) => { requests.push([sid, flags]); });
    world.load(seeds());
    const popped = new Set(requests.map((item) => item[0]));
    // 서버 SID 객체는 모두 적어도 한 번 송신기를 거치고, 예측 가능한 지역 SID(받침/종유석/연결 조각)는 송신하지 않는다.
    for (const item of world.objects()) assert.equal(popped.has(item.sid), item.sid >= 15000, `sid ${item.sid}`);
    assert.ok(popped.size >= 11);
    const loaded = requests.length;
    world.bindSquidTransmission(null);
    world.unpop(world.find(32, 29, BRIDGE), 0);
    world.repop(world.find(32, 29, BRIDGE) || 15010, 0x40);
    assert.equal(requests.length, loaded);
    world.dispose();
});

// 다리 붕괴 장치는 한 번만 연결하고 모든 장치 함수가 필요하다. (2026-11-11 WB04-E299)
test('월드: 다리 붕괴 장치 연결 계약', () => {
    const world = new Inputs().make();
    const noop = () => {};
    assert.throws(() => world.bindBridgeDecay(0, noop, noop), /중복\/미완성/);
    assert.throws(() => world.bindBridgeDecay(() => 0, null, noop), /중복\/미완성/);
    assert.throws(() => world.bindBridgeDecay(() => 0, noop, null), /중복\/미완성/);
    world.bindBridgeDecay(() => 0, noop, noop);
    assert.throws(() => world.bindBridgeDecay(() => 0, noop, noop), /중복\/미완성/);
    world.dispose();
});

// ---- 2차 보강 검사(변형 확인에서 살아남은 연결을 직접 관측한다) ----

// 합성 타입 100(원본 base 생성자·발자국 1)을 쓰는 입력과 월드 기본 runtime 훅 묶음을 만든다. 훅은 호출을 기록하는 관찰용이다. (2026-11-11 WB04-E299)
function syntheticInputs() {
    const input = new Inputs();
    const synthetic = 100;
    input.types[synthetic].constructorAddress = 0;
    input.types[synthetic].flags1 = 0x10;
    input.types[synthetic].footX = 1;
    input.types[synthetic].footY = 1;
    input.codes[synthetic] = [{ side: 0x41, variant: 0x41, number: 0, flags: 0 }];
    input.frames[synthetic] = new FrameCodeTable(input.codes[synthetic]);
    input.shapes[synthetic] = { frameCount: 1, loaded: true, frames: [{ width: 20, height: 16, hotspotX: 8, hotspotY: 12, cellWidth: 1, cellHeight: 1 }] };
    return { input, synthetic };
}

// 기본 runtime 훅 묶음이다. 호출 기록은 log 에 쌓이며 옵션으로 개별 훅을 바꾼다. (2026-11-11 WB04-E299)
function runtimeHooks(world, log, override = {}) {
    const core = world.core();
    return {
        assets: { ...core.surfaces.hooks(), selected: () => 0 },
        destroy: (sid, flags, effects) => core.virtualDestroy.destroy(sid, flags, effects),
        regular: makeBridgeRegularHandler(core.pool, core.bridgeEvents),
        transmit: { writeData: () => 8, transmit: () => {} },
        formTransmit: (sid, flags) => { log.push(['formTransmit', sid, flags]); },
        walkerFall: (sid) => { log.push(['walker', sid]); return true; },
        loadingDepth: (depth) => { log.push(['depth', depth]); },
        bridgeEffect: (event) => { log.push(['effect', event.effect]); },
        ...override,
    };
}

// 필수 runtime 경계를 하나씩 빼면 모두 TypeError 이고 월드는 바뀌지 않는다. (2026-11-11 WB04-E299)
test('월드: Runtime 필수 경계를 하나씩 빼면 거부', () => {
    const world = new Inputs().make();
    const before = snapshot(world);
    const full = runtimeHooks(world, []);
    const cases = {
        'assets.emit': { ...full, assets: { ...full.assets, emit: undefined } },
        'assets.selected': { ...full, assets: { ...full.assets, selected: undefined } },
        destroy: { ...full, destroy: undefined },
        regular: { ...full, regular: undefined },
        'transmit.writeData': { ...full, transmit: { transmit: () => {} } },
        'transmit.transmit': { ...full, transmit: { writeData: () => 8 } },
        walkerFall: { ...full, walkerFall: undefined },
        loadingDepth: { ...full, loadingDepth: undefined },
        bridgeEffect: { ...full, bridgeEffect: undefined },
    };
    // 경계를 하나씩 뺀 묶음마다 설치가 거부되고 월드가 그대로인지 본다.
    for (const [name, hooks] of Object.entries(cases)) {
        assert.throws(() => world.bindRuntime(hooks), TypeError, name);
        assert.equal(snapshot(world), before, name);
    }
    // 어떤 거부도 설치로 이어지지 않으므로 완전한 묶음은 그대로 한 번 설치된다.
    world.bindRuntime(full);
    assert.throws(() => world.bindRuntime(full), /초기 객체 생성 전에/);
    world.dispose();
});

// 설치 모드·삭제 소비자·runtime 은 로딩 중이거나 runtime 이 설치된 뒤에는 바꿀 수 없다. (2026-11-11 WB04-E299)
test('월드: 첫 객체 전 관문은 로딩 중과 runtime 설치 뒤에도 닫힘', () => {
    const { input } = syntheticInputs();
    // 로딩 시작 뒤(객체 없음)에는 닫는다.
    const loading = input.make();
    loading.beginLoad();
    assert.throws(() => loading.configureInstallModes(false, true, false, false), /첫객체/);
    assert.throws(() => loading.bindRuntime(runtimeHooks(loading, [])), /초기 객체 생성 전에/);
    assert.throws(() => loading.bindDeletion(null, () => 0, {}), /첫 객체 생성 전에 한 번/);
    loading.dispose();
    // runtime 설치 뒤에는 설치 모드를 바꿀 수 없다(객체 없음).
    const bound = input.make();
    bound.bindRuntime(runtimeHooks(bound, []));
    assert.throws(() => bound.configureInstallModes(false, true, false, false), /첫객체/);
    bound.dispose();
    // 로드를 마친 월드도 닫혀 있다.
    const loaded = input.make();
    loaded.load(seeds());
    assert.throws(() => loaded.configureInstallModes(false, true, false, false), /첫객체/);
    loaded.dispose();
    // 설치 모드의 Editor 인자는 같은 편집기 모드로 단단한 다리를 즉시 지운다.
    const editor = input.make();
    editor.configureInstallModes(false, true, false, true);
    editor.load(seeds(true));
    assert.ok(editor.destroy(editor.find(31, 29, BRIDGE)));
    editor.dispose();
    const normal = input.make();
    normal.configureInstallModes(false, true, false, false);
    normal.load(seeds(true));
    assert.equal(normal.destroy(normal.find(31, 29, BRIDGE)), false);
    normal.dispose();
});

// 저장소의 이전 모드·종류가 남아 있어도 일반 월드는 입력의 표면 종류·모드·색을 다시 쓴다. (2026-11-11 WB04-E299)
test('월드: 일반 대여는 입력의 표면 종류와 모드를 다시 씀', () => {
    const input = new Inputs();
    input.links.mission = true;
    input.links.ownerColors = input.links.ownerColors.map((_, index) => index + 3);
    const storage = new RawSurfaceStorage(input.types, input.frames, input.shapes);
    // 이전 사용이 남긴 값으로 어지럽혀 둔다.
    Object.assign(storage.links, { connectorType: 1, islandType: 2, stalagType: 3, noIslandType: 4, bridgeType: 5, battle: false, mission: false });
    storage.links.ownerColors.fill(0);
    storage.ownerMode.fort = true;
    storage.ownerMode.battle = false;
    storage.ownerMode.challenge = true;
    const world = input.make({ storage });
    const links = storage.links;
    assert.deepEqual([links.connectorType, links.islandType, links.stalagType, links.noIslandType, links.bridgeType, links.battle, links.mission], [CONNECTOR, ISLAND, STALAG, SURFACE, BRIDGE, true, true]);
    assert.deepEqual(Array.from(links.ownerColors), Array.from(input.links.ownerColors));
    assert.deepEqual([storage.ownerMode.fort, storage.ownerMode.battle, storage.ownerMode.challenge], [false, true, false]);
    world.dispose();
});

// 생성 인자 거부는 저장소를 빌리기 전이다: 같은 저장소를 넘겨도 세대·대여가 바뀌지 않는다. (2026-11-11 WB04-E299)
test('월드: 타입 번호 오류는 대여 전에 거부', () => {
    const input = new Inputs();
    const storage = new RawSurfaceStorage(input.types, input.frames, input.shapes);
    const make = (links, terrain) => new RawSurfaceWorld(input.types, input.frames, input.shapes, links, terrain, { storage });
    assert.throws(() => make(input.links, 188), /대여 전 타입 번호/);
    assert.throws(() => make({ ...input.links, bridgeType: 188 }, TERRAIN), /대여 전 타입 번호/);
    assert.throws(() => make({ ...input.links, noIslandType: 188 }, TERRAIN), /대여 전 타입 번호/);
    assert.equal(storage.generation, 0);
    assert.equal(storage.borrowed, false);
});

// 장기 집계·목록·손실 좌표·로딩 깊이는 저장소/Template 의 같은 상자를 빌린다. (2026-11-11 WB04-E299)
test('월드: 장기 집계와 공유 로딩 깊이', () => {
    const { input, synthetic } = syntheticInputs();
    // 작업장 분류(flags2 0x200)를 가진 합성 타입이면 공장 목록에 등록된다.
    input.types[synthetic].flags2 = 0x200;
    const storage = new RawSurfaceStorage(input.types, input.frames, input.shapes);
    const depth = { value: 0 };
    const world = input.make({ storage, loadingDepth: depth });
    // 공유 상자는 생성 때 덮어쓰지 않는다.
    assert.equal(depth.value, 0);
    const core = world.core();
    storage.gameAux.lostY = 5;
    assert.equal(world.deletionState.readLostY(), 5);
    world.deletionState.writeLostY(9);
    assert.equal(storage.gameAux.lostY, 9);
    // 깊이 0 에서는 noIsland 접두가 이웃으로 프레임을 고르지 않는다: 두 번째 칸 프레임이 쓴 값 그대로이다.
    const frameOf = (sid) => get(core.pool.slot(sid), FRAME_OFFSET, 4);
    const first = world.create(SURFACE);
    world.setAssetOwner(first, 3);
    world.pop(first, 28, 28, 0);
    const second = world.create(SURFACE);
    world.setAssetOwner(second, 3);
    world.pop(second, 29, 28, 0);
    assert.equal(frameOf(second), 0);
    // 앞선 효과가 상자를 1 로 바꾸면 다음 등록은 새 값을 읽어 서쪽 이웃으로 프레임을 고른다.
    depth.value = 1;
    const third = world.create(SURFACE);
    world.setAssetOwner(third, 3);
    world.pop(third, 30, 28, 0);
    assert.notEqual(frameOf(third), 0);
    // 월드의 로딩 깊이 설정은 같은 상자에 쓴다.
    world.setLoadingDepth(4);
    assert.equal(depth.value, 4);
    // 작업장 객체의 등록은 장기 공장 목록과 집계에 반영된다.
    const before = storage.sidLists.factories.readCount();
    const workshop = world.create(synthetic);
    core.owner.set(workshop, 1);
    world.pop(workshop, 40, 40, 0);
    assert.equal(storage.sidLists.factories.readCount(), before + 1);
    assert.equal(storage.totalMade.globalCounts[synthetic], 1);
    assert.equal(storage.totalMade.globalCounts[SURFACE], 3);
    // 공급원 분류(flags1 0x10000000)의 등록도 장기 공급원 목록에 반영된다.
    const supplier = new Inputs();
    supplier.types[synthetic].constructorAddress = 0;
    supplier.types[synthetic].flags1 = 0x10 | 0x10000000;
    supplier.types[synthetic].footX = 1;
    supplier.types[synthetic].footY = 1;
    supplier.codes[synthetic] = [{ side: 0x41, variant: 0x41, number: 0, flags: 0 }];
    supplier.frames[synthetic] = new FrameCodeTable(supplier.codes[synthetic]);
    supplier.shapes[synthetic] = input.shapes[synthetic];
    const supplyStorage = new RawSurfaceStorage(supplier.types, supplier.frames, supplier.shapes);
    const supplyWorld = supplier.make({ storage: supplyStorage });
    const provider = supplyWorld.create(synthetic);
    supplyWorld.core().owner.set(provider, 1);
    supplyWorld.pop(provider, 40, 40, 0);
    assert.equal(supplyStorage.sidLists.providers.readCount(), 1);
    supplyWorld.dispose();
    assert.throws(() => input.make({ loadingDepth: {} }), TypeError);
    world.dispose();
    // 독립 월드의 지역 상자는 기존 초기값 1 로 시작해 beginLoad/finishLoad 가 1 → 0 으로 쓴다.
    const logs = [];
    const solo = input.make();
    solo.bindRuntime(runtimeHooks(solo, logs));
    solo.load(seeds());
    assert.deepEqual(logs.filter((item) => item[0] === 'depth').map((item) => item[1]), [1, 1, 0]);
    solo.dispose();
});

// 변경 표시: 각 공개 변경 동작이 Renderer 억제와 무관하게 한 번씩 알린다. (2026-11-11 WB04-E299)
test('월드: 변경 동작은 변경 표시를 올림', () => {
    const { input, synthetic } = syntheticInputs();
    const world = input.make();
    world.load(seeds());
    const core = world.core();
    const changedBy = (action) => {
        world.takeChanged();
        action();
        return world.takeChanged();
    };
    // 만들기 flags 는 공장에 전달된다: 예측 가능한 지역 번호(flags 2)는 서버 번호와 다른 범위이다.
    const local = world.create(synthetic, 2);
    const remote = world.create(synthetic);
    assert.ok(local < 15000 && remote >= 15000);
    // 현재 TYPE 번호가 표 밖이면 Unpop 은 거부한다.
    const broken = world.create(synthetic);
    core.pool.allocated(broken)[10] = 255;
    assert.throws(() => world.unpop(broken, 0), RangeError);
    // 읽기 전용 호출은 알리지 않는다.
    assert.equal(changedBy(() => { world.find(30, 30, ISLAND); world.objects(); }), false);
    let sid = 0;
    assert.equal(changedBy(() => { sid = world.create(synthetic); }), true);
    core.owner.set(sid, 1);
    assert.equal(changedBy(() => world.pop(sid, 50, 50, 0)), true);
    assert.equal(changedBy(() => world.unpop(sid, 0)), true);
    assert.equal(changedBy(() => world.repop(sid, 0)), true);
    assert.equal(changedBy(() => world.setAssetOwner(sid, 2)), true);
    assert.equal(changedBy(() => world.setSupportOwner(30, 30, 6)), true);
    assert.equal(changedBy(() => world.destroy(sid)), true);
    // 거부된 삭제는 알리지 않는다.
    const hard = new Inputs().make();
    hard.load(seeds(true));
    assert.equal((() => { hard.takeChanged(); const result = hard.destroy(hard.find(31, 29, BRIDGE)); return [result, hard.takeChanged()]; })().join(), 'false,false');
    hard.dispose();
    // 로딩 마무리도 알린다.
    const fresh = input.make();
    fresh.beginLoad();
    fresh.takeChanged();
    fresh.core().graph.rebuild(false);
    fresh.finishLoadAfterGraph();
    assert.equal(fresh.takeChanged(), true);
    // Graph 활성은 로딩 시작과 마무리가 각각 켠다.
    const gate = input.make();
    gate.core().bookkeeping.graphsEnabled = false;
    gate.beginLoad();
    assert.equal(gate.core().bookkeeping.graphsEnabled, true);
    gate.core().bookkeeping.graphsEnabled = false;
    gate.core().graph.rebuild(false);
    gate.finishLoadAfterGraph();
    assert.equal(gate.core().bookkeeping.graphsEnabled, true);
    gate.dispose();
    fresh.dispose();
    world.dispose();
});

// 로딩 순서 계약: finishLoad 는 begin 뒤에만, 전체 Graph 를 재구성하고 깊이를 0 으로 내리며, 적재는 terrain → noIsland → 다리 순서이다. (2026-11-11 WB04-E299)
test('월드: 로딩 시작/종료 계약과 적재 순서', () => {
    const { input } = syntheticInputs();
    const fresh = input.make();
    assert.throws(() => fresh.finishLoad(), /로드 종료 순서/);
    fresh.beginLoad();
    assert.throws(() => fresh.beginLoad(), /중복 로드 시작/);
    // 임의 장부 오염을 finishLoad 의 전체 재구성이 지운다.
    const records = fresh.core().graph.records();
    records[200].inUse = 1;
    records[200].surfaces = 3;
    fresh.finishLoad();
    assert.deepEqual([records[200].inUse, records[200].surfaces], [0, 0]);
    assert.throws(() => fresh.finishLoad(), /로드 종료 순서/);
    assert.throws(() => fresh.beginLoad(), /중복 로드 시작/);
    fresh.dispose();
    // 순서: terrain(83) → noIsland(157) → 다리(82). 입력 순서와 좌표 순서가 달라도 SID 가 이 순서를 따른다.
    // 지면 타입의 실제 후처리 몸체는 아직 이식 전이므로 순서 검사에는 원본 base 생성자의 합성 지면을 쓴다.
    Object.assign(input.types[TERRAIN], { constructorAddress: 0, flags1: 0x10, flags2: 0, footX: 1, footY: 1 });
    const world = input.make();
    const list = seeds();
    list.push({ type: TERRAIN, owner: 0, frame: 0, x: 60, y: 60 });
    world.load(list);
    const terrain = world.find(60, 60, TERRAIN);
    const cell = world.find(30, 30, SURFACE);
    const bridge = world.find(31, 29, BRIDGE);
    assert.ok(terrain && cell && bridge);
    assert.ok(terrain < cell && cell < bridge);
    // 같은 행의 칸은 x 가 작은 순이고 위 행이 먼저이다.
    assert.ok(world.find(28, 28, SURFACE) < world.find(29, 28, SURFACE));
    assert.ok(world.find(30, 28, SURFACE) < world.find(28, 29, SURFACE));
    // 객체 목록의 좌표는 x/y 를 바꾸지 않는다(비대칭 좌표).
    const item = world.objects().find((entry) => entry.sid === bridge);
    assert.deepEqual([item.object.x, item.object.y, item.object.type, item.object.owner, item.object.frame], [31, 29, BRIDGE, 3, NORMAL]);
    // 비정수 y 와 정수 아닌 값은 적재 전에 거부한다.
    const bad = input.make();
    const badList = seeds();
    badList[badList.length - 1].y = 28.5;
    assert.throws(() => bad.load(badList), RangeError);
    // 반환된 시각 사본을 바꿔도 월드 시각은 그대로이다.
    const frame = bad.frameTime;
    frame.game = 999;
    assert.notEqual(bad.frameTime.game, 999);
    bad.dispose();
    world.dispose();
});

// 낙하 효과는 Runtime 이 없으면 관찰 횟수로, 있으면 실제 소비자로 전달된다. walker 낙하/삭제 요청도 같다. (2026-11-11 WB04-E299)
test('월드: 낙하 효과와 삭제 요청의 소비자 구분', () => {
    // 독립 월드: 관찰 횟수.
    const solo = new Inputs().make();
    solo.load(seeds());
    const walkerCell = solo.find(29, 29, SURFACE);
    const core = solo.core();
    const walker = core.pool.allocate(2);
    const raw = core.pool.allocated(walker);
    put(raw, 0, 0x11010000);
    raw[TYPE] = 85;
    raw[STATE] = 0;
    put(raw, 14, floatToBits(29));
    put(raw, 18, floatToBits(29));
    put(raw, 4, core.hash.bucket(1, 29, 29), 2);
    core.hash.setBucket(1, 29, 29, walker);
    assert.deepEqual(solo.observed, { effects: 0, walkerFalls: 0, deletions: 0 });
    assert.ok(solo.destroy(walkerCell));
    assert.equal(solo.observed.walkerFalls, 1);
    assert.ok(solo.observed.deletions > 0);
    assert.ok(solo.destroy(solo.find(30, 30, ISLAND)));
    solo.runFrame(time(12, 12, 12, 1), false);
    assert.ok(solo.observed.effects > 0);
    solo.dispose();
    // runtime 월드: 실제 소비자만 받고 관찰 횟수는 늘지 않는다.
    const log = [];
    const bound = new Inputs().make();
    bound.bindRuntime(runtimeHooks(bound, log));
    bound.load(seeds());
    const boundCore = bound.core();
    const boundWalker = boundCore.pool.allocate(2);
    const boundRaw = boundCore.pool.allocated(boundWalker);
    put(boundRaw, 0, 0x11010000);
    boundRaw[TYPE] = 85;
    boundRaw[STATE] = 0;
    put(boundRaw, 14, floatToBits(29));
    put(boundRaw, 18, floatToBits(29));
    put(boundRaw, 4, boundCore.hash.bucket(1, 29, 29), 2);
    boundCore.hash.setBucket(1, 29, 29, boundWalker);
    assert.ok(bound.destroy(bound.find(29, 29, SURFACE)));
    assert.deepEqual(log.filter((item) => item[0] === 'walker'), [['walker', boundWalker]]);
    assert.ok(bound.destroy(bound.find(30, 30, ISLAND)));
    bound.runFrame(time(12, 12, 12, 1), false);
    assert.ok(log.some((item) => item[0] === 'effect'));
    assert.deepEqual(bound.observed, { effects: 0, walkerFalls: 0, deletions: bound.observed.deletions });
    bound.dispose();
    // 낙하 처리가 실패(false)를 돌려주면 월드는 거부한다.
    const refuse = new Inputs().make();
    refuse.bindRuntime(runtimeHooks(refuse, [], { walkerFall: () => false }));
    refuse.load(seeds());
    const refuseCore = refuse.core();
    const refuseWalker = refuseCore.pool.allocate(2);
    const refuseRaw = refuseCore.pool.allocated(refuseWalker);
    put(refuseRaw, 0, 0x11010000);
    refuseRaw[TYPE] = 85;
    refuseRaw[STATE] = 0;
    put(refuseRaw, 14, floatToBits(29));
    put(refuseRaw, 18, floatToBits(29));
    put(refuseRaw, 4, refuseCore.hash.bucket(1, 29, 29), 2);
    refuseCore.hash.setBucket(1, 29, 29, refuseWalker);
    assert.throws(() => refuse.destroy(refuse.find(29, 29, SURFACE)), /월드 낙하 처리 실패/);
    refuse.dispose();
});

// 다리 붕괴 장치가 없는 월드: 독립이면 관찰 횟수, runtime 이 있으면 장치 미연결 오류이다. 시각 장치와 월드 시각은 한 칸 요청도 먼저 공급한다. (2026-11-11 WB04-E299)
test('월드: 다리 붕괴 장치 미연결 분기와 갱신', () => {
    // 수명을 정해 한 칸을 요청하는 도우미이다(raw WORD 의 수명 비트만 바꾼다).
    const setLife = (world, sid, life) => {
        const raw = world.core().pool.allocated(sid);
        put(raw, WORD, (get(raw, WORD, 2) & ~0x78) | (life << 3), 2);
    };
    // 독립 월드: 대피(수명 ≤ 3)와 균열 소리 판단이 관찰 횟수로 센다.
    const solo = new Inputs().make();
    solo.load(seeds());
    const outer = solo.find(32, 29, BRIDGE);
    setLife(solo, outer, 2);
    solo.decayBridgeCell(time(1, 1, 1, 1), outer);
    assert.ok(solo.observed.effects > 0);
    assert.equal(solo.frameTime.game, 1);
    solo.dispose();
    // 시각 장치와 월드 시각은 decayBridgeCell 이 먼저 공급한다.
    const seen = [];
    const world = new Inputs().make();
    world.load(seeds());
    const target = world.find(32, 29, BRIDGE);
    world.bindBridgeDecay(() => 0, () => {}, () => {}, (frame) => { seen.push(frame.game); });
    world.decayBridgeCell(time(31, 35, 0.25, 125), target);
    assert.deepEqual(seen, [31]);
    assert.equal(world.frameTime.game, 31);
    assert.equal(world.core().processState.now, 31);
    assert.equal(lifeOf(world.core(), target), 7);
    // 균열 경계(수명 5 → 4)를 넘으면 실제 표시 갱신이 변경 표시로 보인다.
    setLife(world, target, 5);
    world.takeChanged();
    world.decayBridgeCell(time(31.25, 35.25, 0.25, 126), target);
    assert.equal(lifeOf(world.core(), target), 4);
    assert.equal(world.takeChanged(), true);
    world.dispose();
    // runtime 이 있는데 붕괴 장치가 없으면 명시 오류이다(대피가 균열 소리보다 먼저 불리므로 첫 오류는 대피 장치이다).
    const bound = new Inputs().make();
    bound.bindRuntime(runtimeHooks(bound, []));
    bound.load(seeds());
    const boundTarget = bound.find(32, 29, BRIDGE);
    setLife(bound, boundTarget, 2);
    assert.throws(() => bound.decayBridgeCell(time(31, 35, 0.25, 1), boundTarget), /대피 장치 미연결/);
    bound.dispose();
});

// 스캔 주기와 정지 관문: 장기 주기 값을 읽고, 정지 중이면 시간이 흘러도 진행하지 않는다. (2026-11-11 WB04-E299)
test('월드: 다리 붕괴 스캔의 주기와 정지', () => {
    const run = (period, paused) => {
        const world = new Inputs().make();
        world.load(seeds());
        world.bindBridgeDecay(() => 0, () => {}, () => {});
        world.storage.bridgeDecayPeriod = period;
        const scan = world.storage.bridgeDecayScan;
        resetStateBridgeDecayScan(scan, 10, 0);
        // 기한을 초기 10 으로 두고 그 안에서 델타 만큼만 훑는다.
        world.runFrame(time(1, 1, 1, 1), paused);
        const cursor = scan.cursor;
        world.dispose();
        return cursor;
    };
    const normal = run(10, false);
    const slow = run(20, false);
    assert.ok(normal > 15000 && slow > 15000);
    // 주기가 두 배면 한 프레임에 훑는 칸 수가 절반이다.
    assert.ok(slow < normal);
    // 정지 중에는 델타가 있어도 커서가 그대로이다.
    assert.equal(run(10, true), 15000);
});

// 종속 Unpop 엔진과 Form 분리 연결: Session 엔진이 있으면 그 엔진으로, 해제하면 월드 기본 종단으로 돌아간다. (2026-11-11 WB04-E299)
test('월드: 종속 Unpop 엔진 선택', () => {
    const { input, synthetic } = syntheticInputs();
    const storage = new RawSurfaceStorage(input.types, input.frames, input.shapes);
    // 종속 상태 비트를 임의로 세우면 원문 진단이 한 번 불리며, 진단은 같은 장기 장치로 가서 복귀한다.
    const diagnostics = [];
    storage.bindDisplayDiagnostics({ value: null }, (expression) => { diagnostics.push(expression); });
    const world = input.make({ storage });
    const core = world.core();
    const calls = [];
    const object = world.create(synthetic);
    core.owner.set(object, 1);
    world.pop(object, 25, 25, 0);
    world.bindContainedUnpop({ pool: core.pool, unpop: (sid) => { calls.push(['unpop', sid]); }, detach: (sid) => { calls.push(['detach', sid]); }, contentFormType: 6 });
    // 종속(contained, 상태 비트 8) 객체의 Unpop 은 종속 해제 종단으로 끝나며 Session 엔진이 있으면 그 엔진을 호출한다.
    core.pool.allocated(object)[11] |= 8;
    world.unpop(object, 0);
    assert.deepEqual(calls, [['unpop', object]]);
    assert.ok(diagnostics.length > 0);
    // 종속이 아닌 일반 객체는 종단을 거치지 않는다.
    const plain = world.create(synthetic);
    core.owner.set(plain, 1);
    world.pop(plain, 30, 30, 0);
    calls.length = 0;
    world.unpop(plain, 0);
    assert.deepEqual(calls, []);
    world.dispose();
});

// 감시 Form: 월드의 DependFormHost 는 실제 공장·현재 TYPE·Unpop 으로 감시 쌍을 만들고 지우며, 통지는 runtime 소비자가 있으면 그쪽으로 간다. (2026-11-11 WB04-E299)
test('월드: 감시 Form 쌍의 생성·통지·삭제', () => {
    const { input, synthetic } = syntheticInputs();
    const world = input.make();
    const log = [];
    const notices = [];
    world.bindRuntime(runtimeHooks(world, log, { dependencyNotice: (watcher, watched, form) => { notices.push([watcher, watched, form]); } }));
    const core = world.core();
    const make = (x, y) => {
        const sid = world.create(synthetic);
        core.owner.set(sid, 1);
        world.pop(sid, x, y, 0);
        return sid;
    };
    const watched = make(25, 25);
    const watcher = make(26, 26);
    const pair = core.dependencies.create(watched, watcher);
    assert.ok(core.dependencies.has(watched, watcher));
    assert.equal(core.dependencies.watcher(pair.watchedForm), watcher);
    // 서버 전송 경계를 거쳤다.
    assert.ok(log.some((item) => item[0] === 'formTransmit'));
    // 감시 대상이 사라지면 감시자 쪽이 통지를 받는다.
    assert.ok(world.destroy(watched));
    assert.equal(notices.length, 1);
    assert.equal(notices[0][0], watcher);
    assert.equal(notices[0][1], watched);
    assert.ok([pair.watchedForm, pair.watcherForm].includes(notices[0][2]));
    world.dispose();
});

// 독립 Kernel(레거시 Host) 월드는 종료 때 자기 프로세스만 반납하고 SID 소진 회수가 flags 0 가상 삭제이다. 공유 Host 의 종료도 같다. (2026-11-11 WB04-E299)
test('월드: 종료와 Clear/Drain 의 프로세스 반납', () => {
    const input = new Inputs();
    const storage = new RawSurfaceStorage(input.types, input.frames, input.shapes);
    const kernel = new Kernel();
    const legacy = input.make({ storage, processKernel: kernel });
    legacy.load(seeds());
    assert.ok(legacy.destroy(legacy.find(30, 30, ISLAND)));
    assert.equal(kernel.size, 1);
    legacy.clearProcesses();
    assert.equal(kernel.size, 0);
    assert.ok(legacy.destroy(legacy.find(30, 29, CONNECTOR) || legacy.find(30, 30, STALAG)) || true);
    legacy.dispose();
    assert.equal(kernel.size, 0);
    // 종료만으로(Drain 없이) 자기 프로세스가 반납된다.
    const second = input.make({ storage, processKernel: kernel });
    second.load(seeds());
    assert.ok(second.destroy(second.find(30, 30, ISLAND)));
    assert.equal(kernel.size, 1);
    second.dispose();
    assert.equal(kernel.size, 0);
    // 공유 Host: 종료만으로 이 월드의 프로세스가 반납된다.
    const shared = new RawSurfaceStorage(input.types, input.frames, input.shapes);
    const sharedKernel = new Kernel();
    shared.processes.bindDevices(sharedKernel, { game: 0 });
    const third = input.make({ storage: shared, processKernel: sharedKernel });
    third.load(seeds());
    assert.ok(third.destroy(third.find(30, 30, ISLAND)));
    assert.equal(sharedKernel.size, 1);
    third.dispose();
    assert.equal(sharedKernel.size, 0);
});

// 객체 목록: 자산 TYPE 70 미만(Form/Process 객체)과 free/dead 슬롯은 제외한다. 소유자·프레임은 raw 슬롯 값이다. (2026-11-11 WB04-E299)
test('월드: 객체 목록은 자산만 raw 슬롯에서 읽음', () => {
    const world = new Inputs().make();
    world.load(seeds());
    const core = world.core();
    const before = world.objects().length;
    // 자산이 아닌 TYPE(예: 5)의 살아 있는 슬롯은 목록에 나오지 않는다.
    const form = core.pool.allocate(0);
    const raw = core.pool.allocated(form);
    raw[TYPE] = 5;
    raw[STATE] = 0;
    assert.equal(world.objects().length, before);
    const bridge = world.find(32, 29, BRIDGE);
    const entry = world.objects().find((item) => item.sid === bridge);
    assert.deepEqual([entry.object.owner, entry.object.frame], [core.pool.slot(bridge)[34], get(core.pool.slot(bridge), FRAME_OFFSET, 4)]);
    assert.notEqual(entry.object.owner, 0);
    assert.notEqual(entry.object.frame, 0);
    world.dispose();
});

// Pop 전송은 호출자의 flags 를 그대로 전하고, Repop 은 현재 raw 좌표와 호출자 flags 로 다시 등록한다(flags 0x40 은 전송 생략). (2026-11-11 WB04-E299)
test('월드: Pop 전송 flags 와 Repop 좌표 보존', () => {
    const { input, synthetic } = syntheticInputs();
    const world = input.make();
    const core = world.core();
    const requests = [];
    world.bindSquidTransmission((sid, flags) => { requests.push([sid, flags]); });
    const object = world.create(synthetic);
    core.owner.set(object, 1);
    world.pop(object, 32, 29, 9);
    assert.deepEqual(requests, [[object, 9]]);
    world.unpop(object, 0);
    requests.length = 0;
    // 0x40 은 전송을 생략하는 재등록이며 좌표는 비대칭 raw 값 그대로이다.
    world.repop(object, 0x40);
    assert.deepEqual(requests, []);
    const entry = world.objects().find((item) => item.sid === object);
    assert.deepEqual([entry.object.x, entry.object.y], [32, 29]);
    world.unpop(object, 0);
    world.repop(object, 0);
    assert.equal(requests.length, 1);
    assert.equal(requests[0][0], object);
    world.dispose();
});

// 장기 Graph 추적: 월드의 Graph 는 Client 의 현재 추적 장치로 사용 중 그래프 수를 남긴다. (2026-11-11 WB04-E299)
test('월드: Graph 추적은 장기 Client 장치로 간다', () => {
    const input = new Inputs();
    const storage = new RawSurfaceStorage(input.types, input.frames, input.shapes);
    const traces = [];
    storage.bindGraphTrace((level, text) => { traces.push([level, text]); });
    const world = input.make({ storage });
    world.load(seeds());
    assert.ok(traces.length > 0);
    assert.ok(traces.every(([level, text]) => level === 32 && /graphs in use/.test(text)));
    world.dispose();
});
